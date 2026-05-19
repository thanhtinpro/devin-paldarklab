# W24-25 — Extraction flow (build log)

**Roadmap reference:** `Documents/PALDARK/03-Roadmap_1_Year.md` § Tuần
24–25. **Outcome:** *"Full raid loop functional (drop → fight →
extract)."*

This is the match-orchestration week — not a single-feature week. We
finally tie W7-8 GAS damage + W14-15 dedicated server + W18-19 Pal
combat + W22-23 squad system into a real raid cycle with explicit
phases, per-player outcomes, and a designer-placeable extraction
beacon. After W24-25 ships, the Q2 milestone build (W26) has all the
moving parts required for a real 4-player playtest end-to-end.

## TL;DR

| Subsystem            | Class                        | Layer        |
|----------------------|------------------------------|--------------|
| Phase FSM            | `UPaldarkMatchSubsystem`     | World subsys |
| Per-player registry  | `FPaldarkMatchPlayerRow`     | Subsys field |
| Extraction trigger   | `APaldarkExtractionBeacon`   | Actor        |
| GameMode hook        | `APaldarkGameMode_Extraction`| GameMode     |
| Replicated mirror    | `APaldarkGameStateBase` ext  | GameState    |
| Per-player wire      | `APaldarkPlayerState` ext    | PlayerState  |
| Death hook           | `APaldarkCharacter` patch    | Character    |
| Console commands     | `APaldarkPlayerController`   | 3 commands   |
| Validator            | `check_match_shape`          | CI           |

8 new gameplay tags. 5 REPNOTIFY scalar properties. 3 server-only
multicast delegates. 0 new include cycles.

## What the week ships

### 1. Phase FSM

A 4-state monotonic FSM lives on `UPaldarkMatchSubsystem`:

```
Warmup ──▶ Active ──▶ (banner) Extracting
               │
               └─▶ Ended (terminal)
```

State transitions:

| From     | To         | Trigger                                                |
|----------|------------|--------------------------------------------------------|
| Warmup   | Active     | `WarmupTimerHandle` expires (default 30 s) **or** designer console (`Paldark.Match.Phase Active`) |
| Active   | Extracting | First `RecordExtraction()` (banner-only, doesn't gate any logic) |
| Active   | Ended      | `EvaluateEndCondition()` returns `AllExtracted / TeamWipe / Timeout` or designer `Paldark.Match.ForceEnd` |
| Extracting | Ended    | Same conditions as Active → Ended |
| *Ended*  | —          | Terminal. All `Record*` are no-ops. |

The FSM is **monotonic** — Warmup re-entry is intentionally rejected
(console + subsystem both). Without that constraint, the per-player
outcome ladder (`Alive → terminal`) could be invalidated by a designer
rewinding state during testing.

### 2. Per-player outcome registry

```cpp
struct FPaldarkMatchPlayerRow
{
    TWeakObjectPtr<APlayerState> PlayerState;
    EPaldarkPlayerOutcome        Outcome = EPaldarkPlayerOutcome::Alive;
    float                        ExtractionProgressNormalized = 0.f;
    float                        RegisteredAtServerTime = 0.f;
    float                        DiedAtServerTime = 0.f;
    float                        ExtractedAtServerTime = 0.f;
    float                        DisconnectedAtServerTime = 0.f;
};

UPROPERTY(Transient)
TMap<TObjectPtr<APlayerState>, FPaldarkMatchPlayerRow> PlayerRows;
```

The TMap stays **server-only** and is never replicated directly.
Per-player wire state lives on `APaldarkPlayerState` (which is
already replicated) via two REPNOTIFY scalars:

- `EPaldarkPlayerOutcome MatchOutcome` (1 byte, REPNOTIFY)
- `float ExtractionProgress` (4 bytes, REPNOTIFY, throttled to 0.25 s)

End-condition reduction (called after every `Record*`):

| Condition | Trigger |
|-----------|---------|
| `AllExtracted` | every registered player is `Extracted` |
| `TeamWipe` | every player is `KIA` or `Disconnected` (no `Alive`, no `Extracted`) |
| `Timeout` | `ServerTime > MatchStartedAtServerTime + MaxMatchDuration` (default 600 s) |
| `ForcedByConsole` | designer ran `Paldark.Match.ForceEnd` |

### 3. Extraction beacon

```
APaldarkExtractionBeacon (replicated AActor)
  ├── USphereComponent  ExtractionTrigger (root)
  ├── float             TriggerRadius            = 400.f
  ├── float             ExtractionDuration       = 5.f
  ├── bool              bRequireExplicitConsent  = false
  ├── bool              bDecayOnLeave            = true
  └── float             WireUpdateInterval       = 0.25f
```

The beacon owns a per-player `FProgressEntry` accumulator. Per-frame
loop (server-only):

1. Skip rows for players who left, were GC'd, or hit a terminal
   outcome.
2. Skip if `ShouldAcceptOverlap()` rejects (FSM in `Warmup` or
   `Ended`).
3. Advance progress if the player is still overlapping + consent OK.
4. Decay if they left and `bDecayOnLeave` is true.
5. Throttle the REPNOTIFY pushback to one update / `WireUpdateInterval`
   (default 0.25 s) per player.
6. On progress ≥ 1.0 — call `MatchSub->RecordExtraction(PS)` and erase
   the row.

`bRequireExplicitConsent` is the designer escape hatch from
*"passive-overlap = extract"* to *"E to confirm"*. When true, the
beacon ignores the overlap until the player presses the
`Paldark.InputTag.InteractExtraction` IA (which routes through
`RegisterConsentToExtract(PS)`).

### 4. Death hook

```
GE_Damage_Standard applied  → AttributeSet::Health → 0
   → UPaldarkAttributeSet::OnHealthZeroed.Broadcast(ASC)
      → APaldarkCharacter::InitAbilitySystem-bound weak lambda
         → MatchSub->RecordDeath(PlayerState)
            → Outcome: Alive → KIA
            → EvaluateEndCondition()
```

The lambda is **server-only**, bound exactly once per character (the
`MatchDeathHookHandle.IsValid()` guard prevents re-binding on respawn),
and captures a `TWeakObjectPtr<APaldarkPlayerState>` so a stale PS is
a silent no-op (e.g. seamless travel between maps). No separate death
path — we re-use the W7-8 GAS attribute hook that already exists for
the dummy target's death.

### 5. GameMode hook

`APaldarkGameMode_Extraction` is a concrete `APaldarkGameModeBase`
subclass that:

- `PostLogin` → `MatchSub->RegisterPlayer(PS)`.
- `Logout` → `MatchSub->RecordDisconnect(PS)` + `UnregisterPlayer(PS)`.
- `BeginPlay` → bind `MatchSub->OnMatchEnded` for log + future
  `RestartGame` hook.
- `EndPlay` → unbind.

Designers wire it on the Raid_Sandbox experience by setting `Default
Pawn / GameMode` override to `APaldarkGameMode_Extraction` (or
authoring a BP subclass). The default `APaldarkGameModeBase` does NOT
auto-flip the match — without this swap, the subsystem stays in
`Warmup` forever.

### 6. Replicated state

GameState extension (3 REPNOTIFY scalars):

```cpp
UPROPERTY(ReplicatedUsing = OnRep_MatchPhase, Transient)
EPaldarkMatchPhase MatchPhase = EPaldarkMatchPhase::Warmup;

UPROPERTY(ReplicatedUsing = OnRep_MatchEndReason, Transient)
EPaldarkMatchEndReason MatchEndReason = EPaldarkMatchEndReason::None;

UPROPERTY(ReplicatedUsing = OnRep_PhaseEndServerTime, Transient)
float PhaseEndServerTime = 0.f;
```

PlayerState extension (2 REPNOTIFY scalars):

```cpp
UPROPERTY(ReplicatedUsing = OnRep_MatchOutcome, Transient)
EPaldarkPlayerOutcome MatchOutcome = EPaldarkPlayerOutcome::Alive;

UPROPERTY(ReplicatedUsing = OnRep_ExtractionProgress, Transient)
float ExtractionProgress = 0.f;
```

Both classes expose a delegate so HUD widgets can bind without
polling:

- `APaldarkPlayerState::OnMatchOutcomeChanged` (one-arg: outcome)
- `APaldarkGameStateBase::OnMatchPhaseReplicated` (one-arg: phase)

### 7. Console commands

3 host-side commands registered via `FAutoConsoleCommandWithWorldAndArgs`
in `PaldarkPlayerController.cpp`:

| Command | Effect |
|---------|--------|
| `Paldark.Match.Dump` | dump phase + per-player table to `LogPaldarkPlayer` |
| `Paldark.Match.Phase <Warmup\|Active\|Extracting\|Ended>` | force the FSM forward (Warmup rejected) |
| `Paldark.Match.ForceEnd [AllExtracted\|TeamWipe\|Timeout\|ForcedByConsole]` | end the match immediately (reason defaults to `ForcedByConsole`) |

### 8. Gameplay tags (8 new)

```
Paldark.Match.Phase.Warmup
Paldark.Match.Phase.Active
Paldark.Match.Phase.Extracting
Paldark.Match.Phase.Ended
Paldark.Match.Outcome.Extracted
Paldark.Match.Outcome.KIA
Paldark.Match.Outcome.Disconnected
Paldark.InputTag.InteractExtraction
```

`Alive` intentionally has no tag — its absence is the signal. Phase
tags are broadcast by `UPaldarkMatchSubsystem::SetPhase` via the
helper `PaldarkPhaseToTag()`; outcome tags are stamped on the
PlayerState's loose tag container via `PaldarkOutcomeToTag()`.

### 9. CI

`scripts/ci/validate_paldarklab.py` grows a new `check_match_shape`
function (~300 lines) that asserts:

1. `PaldarkMatchTypes.h` declares the 3 enums + per-player row struct
   + tag helpers + all enumerators.
2. `UPaldarkMatchSubsystem` exposes the FSM, registry, delegates, and
   designer knobs (`MaxMatchDuration / WarmupDuration`).
3. `APaldarkExtractionBeacon` declares the sphere trigger, designer
   knobs, overlap callbacks, and `RegisterConsentToExtract`.
4. PlayerState/GameStateBase have the REPNOTIFY properties +
   `DOREPLIFETIME` + delegate.
5. `APaldarkGameMode_Extraction` overrides the lifecycle hooks.
6. `APaldarkCharacter.cpp` wires `OnHealthZeroed → RecordDeath →
   UPaldarkMatchSubsystem`.
7. `APaldarkPlayerController.cpp` registers the 3 `Paldark.Match.*`
   console commands.

The check shipped passes 0 errors against the W24-25 code in the same
PR. The existing CI markdown-link check + structural validation also
pass.

## Architectural decisions

### A. Match subsystem as `UWorldSubsystem`, not `UGameInstanceSubsystem`

Match state is **per-world**, not per-process. Multiple seamless
travels (e.g. lobby → raid → results → lobby) need separate match
instances. A `UGameInstanceSubsystem` would carry stale state across
worlds, requiring explicit reset hooks. World subsystem boundary is
the right fit; `ShouldCreateSubsystem` gates clients out so the
subsystem never instantiates off-server.

### B. Per-player TMap is server-only, never replicated

Replicating `TMap<TObjectPtr<APlayerState>, FPaldarkMatchPlayerRow>`
would double the authoritative state (TMap on server + replicated
mirror on every client) and require custom `FastArraySerializer`
plumbing. We instead split the state:

- **Authoritative** state lives in the subsystem's TMap on the server.
- **Wire** state (the bits clients actually need to render) lives on
  the already-replicated PlayerState as REPNOTIFY scalars.

This mirrors how the W14-15 / W22-23 subsystems handle their own
per-player data — server-only registry + replicated facade.

### C. ExtractionProgress is throttled, not REPNOTIFY-at-tick

Naive: drive the REPNOTIFY scalar from beacon Tick. At 60 Hz × 4
players × N beacons, this would dominate the PlayerState's bandwidth
budget.

Fix: the beacon writes a *local* `FProgressEntry::ProgressSeconds`
every tick; the wire write goes through `SetExtractionProgress` only
once every `WireUpdateInterval` (default 0.25 s) per player. At 4
updates/s × 4 bytes, this is < 1 kbps. Smooth client UI is achieved
by interpolating between RepNotifies (HUD task) — same pattern as
Source engine's `m_flProgress` for capture points.

### D. Death hook is server-only via weak lambda

The hook lives on `APaldarkCharacter::InitAbilitySystem` (called once
after PossessedBy + ASC init) and binds to the AttributeSet's
`OnHealthZeroed` multicast delegate. Two guards:

1. **Server-only** — `if (!HasAuthority()) return;` before the bind.
   Clients never run the lambda.
2. **Weak PS capture** — the lambda captures `TWeakObjectPtr<
   APaldarkPlayerState>`, so a Pin-and-null-check is enough; stale PS
   = silent no-op.

Cleanup: we cache the AttributeSet pointer + bound handle on the
character. The weak lambda binding's `(this, ...)` form means the
delegate auto-cleans when the character is destroyed (UE5 weak
lambda lifetime semantics) — no explicit EndPlay unbind required.

### E. GameMode subclass, not GameMode patch

We could have folded the PostLogin / Logout hooks into the existing
`APaldarkGameModeBase`. We chose a subclass because:

1. **Default GameMode stays neutral** — the Raid_Sandbox experience
   uses extraction; future experiences (deathmatch, story mode, lobby)
   wouldn't. Patching the base would force the extraction subsystem
   on every experience.
2. **Designer-side opt-in is explicit** — to enable extraction, the
   designer swaps the GameMode class override on the experience asset
   to `APaldarkGameMode_Extraction`. This is more legible than a
   silent class field.
3. **Subclass lets us add experience-specific state later** — e.g.
   spawn-point pools, loot tables, extraction-zone selection — without
   leaking into the base GameMode.

### F. `Extracting` phase is banner-only, not a gate

Earlier draft: `Extracting` would gate damage application (no PvP
during extraction). We backed off because:

1. Real raid games (Tarkov, Hunt) keep PvP live until the
   extraction completes — that's the whole tension.
2. The phase tag is still useful for HUD ("X is extracting…"
   banner) so we keep it as a side-effect flag set on the first
   `RecordExtraction` (and reset on `Ended`).
3. Future PvE-only mode could re-purpose the phase tag to mean
   "tutorial extraction" without breaking the FSM contract.

### G. Reused W7-8 OnHealthZeroed, not a custom death event

W7-8 already exposed `UPaldarkAttributeSet::OnHealthZeroed` for the
dummy target's auto-despawn. Reusing it for the player death hook:

- **DRY** — one death signal, two consumers (despawn + match
  registration).
- **No additional GA surface** — death isn't an ability, it's an
  attribute observation. Routing through an ability would force every
  damage path to bind a death notify.
- **Free coverage** — any new damage source that lands on a player's
  ASC (W26+ environmental hazards, future trap actors, fall damage)
  automatically routes to `RecordDeath` without code change.

## What didn't make this week (explicit roadmap deferrals)

- **Lobby travel** — `ServerTravel("Lobby")` requires a hub-town map,
  lobby UI, and matchmaking. Roadmap W40+ owns this. W24-25 GameMode
  logs the end reason; designer reloads the level manually.
- **UMG end-of-match screen** — UI work goes through artist + UX
  cycle. C++ exposes the delegates the widget binds to; widget lands
  in W22+ polish work.
- **Persistent match stats** — DynamoDB write-back is W42-43 (AWS).
  W24-25 keeps results in-memory; world teardown wipes them.
- **Reconnect mid-match** — `Disconnected` is terminal. Re-joining
  the same match is intentionally not supported; W14-15 follow-up
  adds reconnect.
- **Spectator camera after death** — `KIA` players stay in their
  ragdoll; W22+ polish adds a free-cam.
- **Multi-match-per-session** — one match per world. Designer reloads
  the level for the next match. Auto-restart with seamless travel is
  W40+.
- **AnimMontage on extract** — beacon doesn't drive any pose / hand-on-
  device animation. W22+ polish or art pass adds it.

## Risk register

| Risk | Mitigation |
|------|------------|
| Designer forgets to swap the GameMode class → match stays in Warmup | README authoring section + console command warns "subsystem unavailable, run on host"; future validator could grep DA_PX_RaidSandbox |
| `OnHealthZeroed` binding fires before PlayerState is valid | Lambda captures weak PS + `if (StrongPS == nullptr) return;` guard. Plus the bind sits inside `InitAbilitySystem` which only runs after PossessedBy. |
| Beacon Tick on a client → crash dereferencing null subsystem | `Tick` early-returns on `!HasAuthority()`. Sphere overlap delegate likewise. |
| `Paldark.Match.Phase Warmup` console call rewinds the FSM | Explicitly rejected with a warning log in the console handler. |
| `RecordExtraction` called twice for the same player (e.g. two beacons) | `SetPlayerOutcomeInternal` is idempotent; second call is a no-op once outcome is terminal. |
| `EndMatch` re-entry | `SetPhase(Ended)` checks current == new and early-returns. |
| `MaxMatchDuration` timer fires after manual `EndMatch` | Hard-timeout callback re-checks `CurrentPhase == Active/Extracting`; otherwise no-op. |
| Beacon trigger sphere blocks player movement | `USphereComponent::SetCollisionResponseToAllChannels(ECR_Overlap)` in the ctor — explicitly overlap, never block. |
| ExtractionProgress REPNOTIFY storm | Throttled to `WireUpdateInterval` (0.25 s). |
| FollowSquadCommand (W22-23) takes over the Pal when its player extracts | Activity FSM is independent of the match subsystem. The Pal continues to follow until the level is torn down. Acceptable: Pal extracts with player. |

## Verification

### Unit-shape checks (in this PR)

- `validate_paldarklab.py` ships `check_match_shape` (~300 lines).
- Locally: `python3 scripts/ci/validate_paldarklab.py` → `OK`.
- CI: green (2/2 checks expected — structural + markdown link).

### Editor playtest (designer side, NOT in this PR)

The designer must run this after the C++ compiles cleanly. Steps live
in `PaldarkLab/README.md` § W24-25 "Playtest loop". Summary:

1. Compile `PaldarkLab.uproject` in UE Editor 5.4.
2. Author `BP_ExtractionBeacon` (subclass of `APaldarkExtractionBeacon`)
   and drop into `Raid_Sandbox` map.
3. Swap the Raid_Sandbox experience's GameMode override to
   `APaldarkGameMode_Extraction`.
4. PIE with 4 clients (or W14-15 dedicated test).
5. Walk through: Warmup → Active → kill one teammate → confirm KIA →
   walk other 3 onto beacon → confirm Extracted → confirm `Ended` with
   `AllExtracted`.
6. (Optional) record 30-60 s video as Q2 milestone artefact:
   `Documents/Devlog/Q2-W24-Extraction-Demo.mp4`.

### Bug-bash candidates

- **Race**: extract + die on the same frame. Outcome should resolve to
  whichever `Record*` lands first; the loser is a no-op on the now-
  terminal row. Designer should manually verify with `Paldark.Combat.
  DamagePlayer 1000` (instant kill) while on beacon at 0.9 progress.
- **Disconnect during extraction**: `RecordDisconnect` flips
  `Disconnected` (terminal). The beacon's next Tick sees the terminal
  outcome and removes the row. Test by Alt+F4 client mid-ramp.
- **Multiple beacons**: drop 3 beacons in one map. Each is independent.
  Two players on two different beacons should both extract.
- **Hard-timeout fires while extracting**: set `MaxMatchDuration=10`
  in the BP defaults, force `Active`, walk onto beacon at t=9 s, watch
  the match end with `Timeout` while you're at progress=0.8. Beacon
  should drop progress, no spurious `RecordExtraction`.

## Q2 status after W24-25

| Week | Topic | PR | Status |
|------|-------|----|--------|
| W14-15 | 4-player dedicated test | #18 | ✅ merged |
| W16-17 | Lag compensation | #19 | ✅ merged |
| W18-19 | Pal combat activity | #20 | ✅ merged |
| W20-21 | Hostile-Pal AI | #21 | ✅ merged |
| W22-23 | Squad system | #22 | ✅ merged |
| **W24-25** | **Extraction flow** | **THIS** | **In review** |
| W26 | Q2 Milestone Build | TBD | Next |

6/7 of Q2 weeks shipped. W26 is the **Q2 Milestone Build** week —
internal playtest + bug bash 1 day. Designer drives this in the
editor; Devin produces the Q2 status devlog + Q2 playtest checklist
along the same shape as Q1 (`Documents/Devlog/Q1-*.md`).

## File index

### New files (this PR)

| File | Lines | Notes |
|------|-------|-------|
| `PaldarkLab/Source/PaldarkLab/Public/Match/PaldarkMatchTypes.h` | ~120 | Enums + row struct + tag helpers |
| `PaldarkLab/Source/PaldarkLab/Private/Match/PaldarkMatchTypes.cpp` | ~80 | Helper implementations |
| `PaldarkLab/Source/PaldarkLab/Public/Match/PaldarkMatchSubsystem.h` | ~180 | UWorldSubsystem interface |
| `PaldarkLab/Source/PaldarkLab/Private/Match/PaldarkMatchSubsystem.cpp` | ~400 | FSM + registry + delegates |
| `PaldarkLab/Source/PaldarkLab/Public/Match/PaldarkExtractionBeacon.h` | ~280 | Replicated beacon actor |
| `PaldarkLab/Source/PaldarkLab/Private/Match/PaldarkExtractionBeacon.cpp` | ~380 | Per-player progress + sphere |
| `PaldarkLab/Source/PaldarkLab/Public/Framework/PaldarkGameMode_Extraction.h` | ~80 | GameMode subclass |
| `PaldarkLab/Source/PaldarkLab/Private/Framework/PaldarkGameMode_Extraction.cpp` | ~150 | PostLogin / Logout / OnMatchEnded |
| `Documents/Devlog/W24-25-Build.md` | ~450 | This file |

### Modified files (this PR)

| File | Why |
|------|-----|
| `PaldarkLab/Source/PaldarkLabCore/Public/PaldarkGameplayTags.h` | 8 new tag declarations |
| `PaldarkLab/Source/PaldarkLabCore/Private/PaldarkGameplayTags.cpp` | 8 new tag definitions |
| `PaldarkLab/Source/PaldarkLab/Public/Player/PaldarkPlayerState.h` | 2 REPNOTIFY props + delegate |
| `PaldarkLab/Source/PaldarkLab/Private/Player/PaldarkPlayerState.cpp` | DOREPLIFETIME + setters |
| `PaldarkLab/Source/PaldarkLab/Public/Framework/PaldarkGameStateBase.h` | 3 REPNOTIFY props + delegate |
| `PaldarkLab/Source/PaldarkLab/Private/Framework/PaldarkGameStateBase.cpp` | DOREPLIFETIME + setter |
| `PaldarkLab/Source/PaldarkLab/Public/Player/PaldarkCharacter.h` | Death hook member declarations |
| `PaldarkLab/Source/PaldarkLab/Private/Player/PaldarkCharacter.cpp` | OnHealthZeroed lambda bind |
| `PaldarkLab/Source/PaldarkLab/Private/Player/PaldarkPlayerController.cpp` | 3 Match console commands |
| `scripts/ci/validate_paldarklab.py` | `check_match_shape` + 8 new tag entries |
| `PaldarkLab/README.md` | W24-25 section (~250 lines) |
