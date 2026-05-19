# W22-23 — Squad system (ping + radial command) — Build Devlog

**Status:** Implementation complete (C++ side). Awaiting designer
authoring + 4-player playtest in UE 5.4 Editor.
**Roadmap reference:** `Documents/PALDARK/03-Roadmap_1_Year.md` § Tuần
22-23 — outcome "4-player team coordinate qua ping + radial command".
**Branch:** `devin/1779011363-w22-23-pal-squad-system`
**Closes Q2 step 2/4:** social loop coordination layer; following W18-19
(Pal combat) + W20-21 (hostile-Pal AI). Q2 remaining: W24-25 extraction
flow + W26 Q2 milestone build.

---

## 0. TL;DR

W22-23 adds a 4-player squad coordination layer on top of the existing
combat + perception pipeline. The shape:

1. **`UPaldarkSquadSubsystem`** (UWorldSubsystem) — per-`SquadTag`
   player registry + multicast delegate for radial commands.
2. **`UPaldarkPingSubsystem`** (UWorldSubsystem) + **`APaldarkPingMarker`**
   (replicated actor, 3 s default lifetime) — server-authored world-
   space markers that follow the marked enemy actor for Enemy pings.
3. **`UPaldarkSquadCommandSet`** (UPrimaryDataAsset) +
   **`UPaldarkSquadCommandComponent`** — designer-authored radial wheel
   loadout (Attack / Follow / Stay / Regroup) routed through a Server
   RPC.
4. **`UPaldarkSquadMembershipComponent`** — auto-registers player into
   `Paldark.Squad.Default` on `BeginPlay`. Per-pawn override knob
   (`SquadTag`) sets up future PvP teams without code changes.
5. **`UPaldarkActivity_FollowSquadCommand`** — Priority 22 Pal activity
   that listens on `SquadSub->OnSquadCommandIssued` and steers the Pal
   accordingly. Sits between Follow (P20) and Investigate (P30); only
   active on friendly Pal (hostile-Pal subclasses override
   `CandidateActivities` and intentionally drop the listener).
6. **`APaldarkPlayerController`** — `Server_RequestPing` +
   `Server_RequestMarkUnderCrosshair` RPCs (server re-traces for
   anti-cheat).
7. **3 console commands** (`Paldark.Squad.Dump`, `Paldark.Squad.Ping`,
   `Paldark.Squad.Command`) for designer playtest without UI.
8. **13 new native gameplay tags** under `Paldark.Squad.*`,
   `Paldark.Ping.Type.*`, `Paldark.InputTag.*`,
   `Paldark.Pal.Activity.FollowSquadCommand`.

The VM has no UE5 compiler so this PR ships C++ + config + docs +
validator extension. CI green = structural validation passes; designer
runs Generate Project Files + UE 5.4 build to verify compile, then runs
the 4-player playtest procedure in §6.

---

## 1. Roadmap deliverable + scope discipline

From `Documents/PALDARK/03-Roadmap_1_Year.md` § Tuần 22-23:

> **Outcome:** 4-player team coordinate qua ping + radial command.
> Squad system (membership + commands), ping system (T key spot
> location, V key mark enemy), radial wheel (Q hold, Attack / Follow /
> Stay / Regroup), Pal listener activity. **Defer:** Vivox VOIP, UMG
> radial widget, Pal animation montage cues, persistent ping decals,
> bandwidth optimization, cross-squad coordination.

What W22-23 ships (this PR):

| Component | Lives in |
|-----------|----------|
| `UPaldarkSquadSubsystem` | `Source/PaldarkLab/Public/Squad/` |
| `UPaldarkPingSubsystem` + `APaldarkPingMarker` | same |
| `UPaldarkSquadCommandSet` (PrimaryDataAsset) | same |
| `UPaldarkSquadCommandComponent` | same |
| `UPaldarkSquadMembershipComponent` | same |
| `UPaldarkActivity_FollowSquadCommand` | `…/Pal/Activities/` |
| Player slot wiring (2 new component slots) | `Player/PaldarkCharacter.{h,cpp}` |
| Server RPCs + 3 console commands | `Player/PaldarkPlayerController.{h,cpp}` |
| 13 native tags | `PaldarkLabCore/Public/PaldarkGameplayTags.{h,cpp}` |
| Validator `check_squad_shape` | `scripts/ci/validate_paldarklab.py` |
| README "Squad system (W22-23)" section | `PaldarkLab/README.md` |

Explicitly **not** in this PR (roadmap deferrals + scope hygiene):

- **Vivox VOIP** — vendor SDK + license keys + plugin install. Voice
  ships in a follow-up "comms" week with its own playtest budget.
- **UMG radial widget** — UI artist + UX cycle. The C++ channel + 3
  console commands let designers exercise behaviour today; widget is
  W22+ polish.
- **Pal animation montage cues** (e.g. Pal "look at target" turn
  reaction when Attack ping fires) — W22+ AnimBP work.
- **Persistent ping decals on world geometry** — W30+ VFX pass.
- **Bandwidth optimization for ping spam** — W48-49 perf pass; W22-23
  ships the `MinSecondsBetweenPings` per-issuer cooldown as a coarse
  guard.
- **Cross-squad / multi-squad arbitration** — W40+ hub town (PvP red
  vs blue).

---

## 2. Implementation walk-through

### 2.1 Native gameplay tags (`PaldarkLabCore`)

13 new tag handles + macros, declared in
`PaldarkLabCore/Public/PaldarkGameplayTags.h` and defined in the
matching .cpp via `UE_DEFINE_GAMEPLAY_TAG_STATIC`:

| Tag | Purpose |
|-----|---------|
| `Paldark.Squad.Default` | Membership fallback when no per-pawn override. |
| `Paldark.Squad.Command.Attack` | Radial cmd — move to last enemy ping. |
| `Paldark.Squad.Command.Follow` | Re-leash on issuer. |
| `Paldark.Squad.Command.Stay` | Hold position. |
| `Paldark.Squad.Command.Regroup` | All Pals walk to issuer. |
| `Paldark.Ping.Type.Spot` | Generic "look here" ping. |
| `Paldark.Ping.Type.Enemy` | Marker follows the marked actor. |
| `Paldark.Ping.Type.Loot` | Reserved for W11-12 inventory pickups. |
| `Paldark.Ping.Type.Danger` | Reserved for hostile-Pal early warning. |
| `Paldark.InputTag.Ping` | T — fire spot ping at pawn / crosshair. |
| `Paldark.InputTag.MarkEnemy` | V — server re-traces from camera. |
| `Paldark.InputTag.RadialOpen` | Q (hold) — opens UMG (deferred). |
| `Paldark.Pal.Activity.FollowSquadCommand` | Identity tag for the P22 activity. |

All tags live in `PaldarkLabCore` (PreDefault) so both the game module
and a future editor module can resolve them without circular deps —
same pattern W1 day 8-10 established. Tag registration happens
through `UE_DEFINE_GAMEPLAY_TAG_STATIC` (not the project-settings
config) so they're guaranteed-present even when the project's
`DefaultGameplayTags.ini` is stale.

### 2.2 `UPaldarkSquadSubsystem` (UWorldSubsystem)

Mirrors W20-21's `UPaldarkHostilePackSubsystem` shape — reading both
side-by-side keeps the mental model unified:

- `TMap<FGameplayTag, FPaldarkSquadRoster> SquadRegistry` — per-tag
  list of `TWeakObjectPtr<APaldarkCharacter>`. Weak refs so a
  logged-out player doesn't pin the actor for GC.
- `FPaldarkOnSquadCommandIssued OnSquadCommandIssued` — 3-arg
  multicast delegate `(Issuer, CommandTag, OptionalLocation)`. The Pal
  listener activity binds in its `InitActivity`.
- `StandingCommandPerSquad` + `StandingCommandLocationPerSquad` — cache
  of the most recent command per squad tag. A freshly-spawned Pal
  reads the standing command on activity init so it "joins the
  formation" instead of standing idle until the next broadcast.

`ShouldCreateSubsystem` returns true only for `NM_DedicatedServer`,
`NM_ListenServer`, and `NM_Standalone` — clients don't need a copy of
the registry because all the gameplay decisions happen server-side and
the resulting Pal movement replicates through `ACharacter`'s native
movement replication (already enabled W3-4).

### 2.3 `UPaldarkPingSubsystem` + `APaldarkPingMarker`

- Subsystem (server-only) exposes `SpawnPing(Issuer, Location,
  MarkedActor?, PingType)` and 3 designer knobs:
  - `TSubclassOf<APaldarkPingMarker> MarkerClass`
    (default = `APaldarkPingMarker::StaticClass()`).
  - `float DefaultPingLifetime = 3.0f` — matches the visual lifetime
    contract documented in the README.
  - `float MinSecondsBetweenPings = 0.5f` — per-issuer cooldown.
  - Internally tracks `TMap<TWeakObjectPtr<APaldarkCharacter>, float>
    LastPingTimePerPlayer` for the rate-limit guard.
- Marker actor is a plain `AActor` with `bReplicates = true`, no mesh
  by default (designer subclasses to add a billboard / icon). Four
  replicated properties:
  - `FGameplayTag PingType` — drives HUD icon selection.
  - `TWeakObjectPtr<APaldarkCharacter> OwningPlayer` — who pinged.
  - `TWeakObjectPtr<AActor> MarkedActor` — non-null for Enemy pings,
    drives per-frame `SetActorLocation` so the marker follows the
    target.
  - `float ExpirationServerTime` — server time at which the marker
    should die; `Tick` checks against `GetWorld()->GetTimeSeconds()`
    on the authority and calls `Destroy()`.

The replication choice ("replicated actor" vs "client RPC fan-out") is
deliberate: actors get free relevancy + late-join handling from the
engine. A late-joining 5th player (e.g. reconnect after a disconnect)
sees the same pings any other client sees, for as long as they're
still alive — no manual replay logic.

### 2.4 `UPaldarkSquadCommandSet` (UPrimaryDataAsset)

Tag-keyed loadout for the radial wheel:

```cpp
USTRUCT(BlueprintType)
struct FPaldarkSquadCommandRow
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FGameplayTag CommandTag;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TSoftObjectPtr<UTexture2D> Icon;
};

UCLASS(BlueprintType)
class PALDARKLAB_API UPaldarkSquadCommandSet : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

    UFUNCTION(BlueprintPure, Category = "Paldark|Squad")
    bool FindRowByTag(FGameplayTag InTag, FPaldarkSquadCommandRow& OutRow) const;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Squad",
              meta = (TitleProperty = "CommandTag"))
    TArray<FPaldarkSquadCommandRow> Commands;
};
```

`Config/DefaultGame.ini` registers `PaldarkSquadCommandSet` as a
PrimaryAssetType scanning `/Game/Paldark/Squad/`, so designers see
"Squad Command Set" in the Asset Manager UI without manual config.

### 2.5 `UPaldarkSquadCommandComponent` (player only)

Lives on `APaldarkCharacter` (new slot). Public surface:

- `void IssueCommand(FGameplayTag CommandTag, FVector OptionalLocation)`
  — entry point used both by the UMG radial widget (future) and by
  the `Paldark.Squad.Command` console command.
- Routes through a `UFUNCTION(Server, Reliable) Server_IssueCommand`
  RPC so the gameplay decision (broadcast → all squadmates) happens on
  authority.
- `_Implementation` resolves the player's squad tag via
  `SquadSub->GetSquadTagForPlayer(GetOwnerPlayerCharacter())`, then
  calls `BroadcastSquadCommand(Issuer, CommandTag, OptionalLocation)`.
- `TSoftObjectPtr<UPaldarkSquadCommandSet> CommandSetSoft` — designer
  attaches the asset on the player BP. The component lazily loads on
  first `FindRowByTag` lookup so the synchronous load only happens
  when actually used (typically on the first command issue).

### 2.6 `UPaldarkSquadMembershipComponent` (player only)

Auto-registers on `BeginPlay` (authority only):

```cpp
void UPaldarkSquadMembershipComponent::BeginPlay()
{
    Super::BeginPlay();
    if (!GetOwner() || !GetOwner()->HasAuthority()) { return; }

    if (UPaldarkSquadSubsystem* Sub = GetWorld()->GetSubsystem<UPaldarkSquadSubsystem>())
    {
        const FGameplayTag Resolved = SquadTag.IsValid()
            ? SquadTag
            : PaldarkGameplayTags::TAG_Paldark_Squad_Default;
        Sub->RegisterPlayerToSquad(Cast<APaldarkCharacter>(GetOwner()), Resolved);
    }
}
```

`SetSquadTag(FGameplayTag NewTag)` re-registers (unregister current →
register new). Designer can override per-pawn on the BP default
object or at runtime for PvP code.

### 2.7 `UPaldarkActivity_FollowSquadCommand` (Pal P22)

- Priority 22 → between Follow (P20) and Stalk (P25). Investigate (P30)
  and Combat (P40) preempt.
- `InitActivity` binds `SquadSub->OnSquadCommandIssued.AddUObject(this,
  &UPaldarkActivity_FollowSquadCommand::HandleSquadCommandIssued)`.
- `HandleSquadCommandIssued(Issuer, Tag, Loc)`:
  - Verifies the activity's owning Pal is following a player who's in
    the same squad as the Issuer (so a Direhound that somehow inherits
    the activity doesn't react to player commands).
  - Caches `CurrentCommandTag` + `CurrentCommandLocation`.
  - The owning activity component re-evaluates on the next tick.
- `CanRun_Implementation` returns true iff `CurrentCommandTag.IsValid()`
  AND the Pal has a friendly squad context.
- `TickActivity_Implementation` (called from the activity component at
  `SelectionInterval = 0.25 s`):
  - `Attack`: `AddMovementInput` toward `CurrentCommandLocation`,
    arrival distance `ArrivalToleranceCm` (default 200 cm). The Pal
    perception loop (W18-19) will preempt with Combat once a hostile
    enters range.
  - `Follow` / `Regroup`: leash on the issuer's pawn (re-route
    through the same `AddMovementInput` path the W3-4
    `UPaldarkPalLocomotionComponent` uses).
  - `Stay`: zero movement input; the activity stays selected until
    another command is issued.

### 2.8 `APaldarkCharacter` + `APaldarkPlayerController` wiring

- Character gains 2 new component slots: `SquadMembershipSlot`,
  `SquadCommandSlot`. Both instantiated in the constructor via
  `CreateDefaultSubobject`. Getter pair added matching the W3-4 12-slot
  convention.
- Player controller gains 2 Server RPCs + 1 designer knob:
  - `Server_RequestPing(FVector WorldLocation, FGameplayTag PingType)` —
    validates issuer, resolves type (default Spot), calls
    `PingSub->SpawnPing(Issuer, WorldLocation, nullptr, PingType)`.
  - `Server_RequestMarkUnderCrosshair()` — server-side re-trace from
    `GetPlayerViewPoint()` against `ECC_Visibility`, distance =
    `MarkEnemyTraceDistance` (5000 cm default). If the hit actor's
    `TeamTag == Hostile`, file as Enemy with the hit actor cached;
    otherwise file as Spot.
  - `float MarkEnemyTraceDistance = 5000.f` — `EditDefaultsOnly`,
    `ClampMin = 100.f`.

### 2.9 Friendly Pal default candidate list

`UPaldarkPalActivityComponent::UPaldarkPalActivityComponent` now seeds
the candidate list with the new activity:

```cpp
CandidateActivities = {
    UPaldarkActivity_Idle::StaticClass(),
    UPaldarkActivity_Follow::StaticClass(),
    UPaldarkActivity_FollowSquadCommand::StaticClass(),  // W22-23
    UPaldarkActivity_Investigate::StaticClass(),
    UPaldarkActivity_Combat::StaticClass(),
};
```

Hostile-Pal subclasses (`APaldarkPalCharacter_Direhound`,
`APaldarkPalCharacter_Razorbird`) overwrite the array in their own
constructors and intentionally **don't include** the listener — so
they continue to use Patrol / Stalk / Combat as before.

### 2.10 Console commands (`PaldarkPlayerController.cpp`)

Three `FAutoConsoleCommandWithWorldAndArgs` registered at module
startup:

| Command | Args | Action |
|---------|------|--------|
| `Paldark.Squad.Dump` | none | Dumps `SquadSub->DumpToLog()` + `PingSub->GetActivePingCount()` + standing commands. |
| `Paldark.Squad.Ping` | `<Spot\|Enemy\|Loot\|Danger> [X Y Z]` | Files a ping. Coords default to local pawn location. |
| `Paldark.Squad.Command` | `<Attack\|Follow\|Stay\|Regroup> [X Y Z]` | Issues via the local pawn's `SquadCommandComponent`. |

Helper functions live in an unnamed namespace
(`PaldarkSquadConsole::*`) so they're TU-local and don't bleed into the
linker namespace.

---

## 3. Replication topology

```
Client A (Issuer)                       Server                          Clients B/C/D
─────────────────                       ──────                          ─────────────

T key press
└─ APaldarkPlayerController::Server_RequestPing(Loc, Spot)
                                        Validate Issuer
                                        PingSub->SpawnPing(...)
                                        └─ SpawnActor<APaldarkPingMarker>
                                            bReplicates = true
                                                                        APaldarkPingMarker
                                                                        replicates in;
                                                                        client tick reads
                                                                        ExpirationServerTime
                                                                        → renders icon

V key press
└─ Server_RequestMarkUnderCrosshair()
                                        GetPlayerViewPoint
                                        LineTraceSingleByChannel(Vis)
                                        if (hit.TeamTag == Hostile)
                                            PingType = Enemy
                                            MarkedActor = hit.GetActor()
                                        else
                                            PingType = Spot
                                        PingSub->SpawnPing(...)
                                                                        Marker replicated;
                                                                        per-frame Tick on
                                                                        every client follows
                                                                        MarkedActor (if set)

Q radial → select Follow
└─ SquadCommandComponent->IssueCommand(Follow, Vec0)
                                        Server_IssueCommand_Implementation
                                        SquadSub->BroadcastSquadCommand
                                            OnSquadCommandIssued.Broadcast
                                            (server-side delegate)
                                        UPaldarkActivity_FollowSquadCommand
                                        listener (on every friendly Pal)
                                        caches command → next Tick
                                        steers via AddMovementInput
                                                                        Pal position
                                                                        replicates via
                                                                        ACharacter::
                                                                        SetReplicateMovement
```

**Server-only** delegates means the Pal listener doesn't need to be
running on clients (it isn't — Pal AI lives on authority since W3-4).
Clients only see the **outcome** (Pal moved, ping marker visible).

---

## 4. Validator extension (`check_squad_shape`)

`scripts/ci/validate_paldarklab.py` gains a 10-step shape check
mirroring the W18-19 / W20-21 patterns:

1. `UPaldarkSquadSubsystem` shape + body.
2. `UPaldarkSquadCommandSet` shape.
3. `UPaldarkSquadMembershipComponent` + `UPaldarkSquadCommandComponent`
   shape; verifies the Server RPC declaration.
4. `APaldarkPingMarker` shape + `DOREPLIFETIME` registration for all
   4 replicated properties.
5. `UPaldarkPingSubsystem` shape + body.
6. `UPaldarkActivity_FollowSquadCommand` shape + body (Priority = 22
   enforced via regex `\bPriority\s*=\s*22\b`).
7. `APaldarkCharacter` declares + constructs the 2 new slots.
8. `APaldarkPlayerController` declares + implements the 2 RPCs +
   registers the 3 console commands.
9. `UPaldarkPalActivityComponent.cpp` seeds
   `UPaldarkActivity_FollowSquadCommand::StaticClass()` into the
   default `CandidateActivities` list.
10. `Config/DefaultGame.ini` registers
    `PrimaryAssetType="PaldarkSquadCommandSet"`.

`EXPECTED_GAMEPLAY_TAGS` gains 13 new entries.
`EXPECTED_ACTIVITY_CLASSES` gains the FollowSquadCommand row.

Local run on the VM:

```
$ python3 scripts/ci/validate_paldarklab.py
validate_paldarklab.py: OK — PaldarkLab/ structure looks healthy.
$ python3 scripts/ci/check_markdown_links.py
check_markdown_links.py: OK — 71 markdown file(s) checked.
```

Expected CI: **2/2 green**.

---

## 5. Files touched

### New (17 files)

```
PaldarkLab/Source/PaldarkLab/Public/Squad/PaldarkSquadSubsystem.h
PaldarkLab/Source/PaldarkLab/Public/Squad/PaldarkSquadCommandSet.h
PaldarkLab/Source/PaldarkLab/Public/Squad/PaldarkSquadCommandComponent.h
PaldarkLab/Source/PaldarkLab/Public/Squad/PaldarkSquadMembershipComponent.h
PaldarkLab/Source/PaldarkLab/Public/Squad/PaldarkPingSubsystem.h
PaldarkLab/Source/PaldarkLab/Public/Squad/PaldarkPingMarker.h
PaldarkLab/Source/PaldarkLab/Private/Squad/PaldarkSquadSubsystem.cpp
PaldarkLab/Source/PaldarkLab/Private/Squad/PaldarkSquadCommandSet.cpp
PaldarkLab/Source/PaldarkLab/Private/Squad/PaldarkSquadCommandComponent.cpp
PaldarkLab/Source/PaldarkLab/Private/Squad/PaldarkSquadMembershipComponent.cpp
PaldarkLab/Source/PaldarkLab/Private/Squad/PaldarkPingSubsystem.cpp
PaldarkLab/Source/PaldarkLab/Private/Squad/PaldarkPingMarker.cpp
PaldarkLab/Source/PaldarkLab/Public/Pal/Activities/PaldarkActivity_FollowSquadCommand.h
PaldarkLab/Source/PaldarkLab/Private/Pal/Activities/PaldarkActivity_FollowSquadCommand.cpp
Documents/Devlog/W22-23-Build.md
```

### Modified

```
PaldarkLab/Source/PaldarkLabCore/Public/PaldarkGameplayTags.h            (+13 tag decls)
PaldarkLab/Source/PaldarkLabCore/Private/PaldarkGameplayTags.cpp         (+13 tag defs)
PaldarkLab/Source/PaldarkLab/Public/Player/PaldarkCharacter.h            (+2 slots + getters)
PaldarkLab/Source/PaldarkLab/Private/Player/PaldarkCharacter.cpp         (+2 slot ctor calls)
PaldarkLab/Source/PaldarkLab/Public/Player/PaldarkPlayerController.h     (+2 RPCs + knob)
PaldarkLab/Source/PaldarkLab/Private/Player/PaldarkPlayerController.cpp  (+RPC impls + 3 cmds)
PaldarkLab/Source/PaldarkLab/Private/Pal/Components/PaldarkPalActivityComponent.cpp
                                                                          (+FollowSquadCommand seed)
PaldarkLab/Config/DefaultGame.ini                                         (+1 PrimaryAssetType)
PaldarkLab/README.md                                                      (+W22-23 section)
scripts/ci/validate_paldarklab.py                                         (+check_squad_shape)
```

---

## 6. Designer playtest checklist (UE 5.4 Editor)

The VM has no UE5 compiler. Designer runs these steps after pulling
the branch.

### 6.1 Compile

1. Right-click `PaldarkLab/PaldarkLab.uproject` → Generate Visual
   Studio project files.
2. Open `PaldarkLab.sln` in VS 2022 (or `JetBrains Rider 2024`).
3. Build target = `PaldarkLabEditor` (Development Editor, Win64).
4. Expected: 0 errors. Warnings about unused parameters in stub bodies
   are acceptable (some helpers compile to no-ops for builds without
   debug logging).

### 6.2 Author assets (one-time)

1. `Content/Paldark/Squad/DA_SquadCommandSet_Default` — `UPaldarkSquadCommandSet`.
   Populate the `Commands` array with 4 rows (Attack / Follow / Stay /
   Regroup), each tagged with the matching `Paldark.Squad.Command.*`
   gameplay tag. Icons can be placeholder textures from the engine
   `EditorResources` or a custom set.
2. `BP_Paldark_Default_Player` — open the player Blueprint, confirm
   `SquadMembershipSlot` + `SquadCommandSlot` appear in the
   Components panel. Set `SquadCommandSlot.CommandSetSoft` →
   `DA_SquadCommandSet_Default`.
3. `IMC_Default` — add 3 input rows:
   - `IA_Ping` → `Paldark.InputTag.Ping`, key `T`, `Triggered` event.
   - `IA_MarkEnemy` → `Paldark.InputTag.MarkEnemy`, key `V`,
     `Triggered`.
   - `IA_RadialOpen` → `Paldark.InputTag.RadialOpen`, key `Q`,
     `Hold > 0.15 s`. (Routes to the UMG widget — for this PR the
     handler can be a no-op; designer will exercise via console.)
4. **Optional**: subclass `APaldarkPingMarker` to give it a billboard
   sprite + sound for visual feedback. Replace
   `UPaldarkPingSubsystem::MarkerClass` to point at the subclass.

### 6.3 4-player playtest loop

1. `open Raid_Sandbox?Experience=PaldarkExperience.PX_RaidSandbox?listen`.
2. Set Multiplayer Options → Number of Players = 4, Net Mode = Play as
   Listen Server. PIE → Play.
3. **Squad registration sanity** — host console:
   `Paldark.Squad.Dump`. Log should show:
   ```
   [SquadSub] Roster Paldark.Squad.Default = 4 player(s)
       - Player0 (Host)
       - Player1
       - Player2
       - Player3
   [PingSub] Active pings = 0
   [SquadSub] Standing command per squad: (none)
   ```
4. **Spot ping (T)** — Player1 → `Paldark.Squad.Ping Spot`. Within
   200 ms every client sees a world-space marker at the issuer's
   location. After 3 s it disappears.
5. **Enemy ping (V)** — `Paldark.Pal.SpawnTestCompanion` (host) →
   `Paldark.Pal.SpawnHostileDummy 1000` (host) → Player2 aims at the
   dummy and presses V (or runs
   `Paldark.Squad.Ping Enemy`). The marker should attach to the
   dummy and visibly follow it (drag dummy around with
   `Paldark.Pal.ForceCombat` to verify). Marker dies after 3 s.
6. **Radial — Stay** — Player1 → `Paldark.Squad.Command Stay`. Their
   companion Pal stops in place. Activity log shows P22 active.
7. **Radial — Follow** — `Paldark.Squad.Command Follow`. Pal re-leashes
   on Player1.
8. **Radial — Attack** — `Paldark.Squad.Command Attack <ping X> <Y>
   <Z>` (or via the V → marker workflow → fire Attack and let the
   activity read the cached location). Pal walks toward the location;
   Combat (P40) preempts when the dummy enters perception range.
9. **Investigate preemption** — fire a Spot ping near a dummy; the Pal
   should switch to Investigate (P30) — which overrides
   FollowSquadCommand (P22). Confirm with
   `Paldark.Pal.CurrentActivity` console cmd.
10. **Combat preemption** — drop a hostile dummy directly in front of
    the Pal. Pal switches Follow / FollowSquadCommand → Combat (P40).
11. **Late-join replication** — Player3 disconnects + reconnects. On
    reconnect they should see any active pings (those with
    `ExpirationServerTime > Now`) immediately, courtesy of the
    replicated-actor model.

### 6.4 Acceptance criteria

| Criterion | How to verify |
|-----------|---------------|
| 4 clients all see the same ping markers | Compare PIE windows visually. |
| Server-only registration | `Paldark.Squad.Dump` on a client logs nothing (subsystem absent on client). |
| Per-issuer rate limit | Mash T 10× in 1 s → max ~2 markers (3 s lifetime × 0.5 s cooldown). |
| Standing command persistence | Spawn a fresh Pal after Stay was issued → it adopts Stay on init. |
| Hostile-Pal skip | Direhound spawned after Stay was issued → keeps Patrol / Stalk / Combat ladder; no P22 in `CurrentActivity` log. |
| Anti-cheat re-trace | Aim at a hostile dummy, pause game, run `Server_RequestMarkUnderCrosshair` via cheat manager → server still hits the dummy at its current location, not the client's stale impression. |

Record 30-60 s PIE video (`Documents/Devlog/Q2-W22-Squad-Demo.mp4`) as
the Q2 milestone artefact.

---

## 7. Open questions for the designer

1. **Ping icon set** — 4 ping types share the same `APaldarkPingMarker`
   class. Subclass per type or one class with a tag-keyed icon table?
   The README sketches the table approach, but designer preference
   wins.
2. **Attack ping → Pal aggression heuristic** — currently the Pal walks
   to the cached location and waits for natural perception. Should it
   force-aggro the marked actor as well (i.e. set
   `Perception->ForceTarget(MarkedActor)`)? Player intent suggests yes,
   but a server-side aggro override skips the visual perception loop
   players expect. Tabled for designer ruling.
3. **Standing-command timeout** — currently the standing command per
   squad has no expiry; a player issuing "Stay" once permanently
   anchors the Pal until another command. Should it expire after N
   seconds, or only on explicit override? W22+ polish call.
4. **PvP squad-tag default** — when W40+ adds Red / Blue PvP, what's
   the bootstrap? Per-controller assignment via GameMode `Login`
   override, or designer-authored player starts that stamp the
   `SquadTag` field? Tabled.

---

## 8. Risk register (post-implementation)

| # | Risk | Likelihood | Mitigation |
|---|------|------------|------------|
| 1 | Compile error from `class APaldarkCharacter` forward-decl vs include in `UPaldarkSquadSubsystem.h` | Low | Header forward-declares; cpp includes. Validator catches missing includes via the `UPaldarkSquadMembershipComponent` shape rules. |
| 2 | `OnSquadCommandIssued.AddUObject` lifetime — Pal dies while bound | Low | Activity unbinds in `ExitActivity` + `BeginDestroy`. Weak-ref UObject delegate signatures are GC-safe by design. |
| 3 | Marker actor stuck at 0,0,0 if `SpawnActor` fails | Low | Spawn params use `FActorSpawnParameters::SpawnCollisionHandlingOverride = AlwaysSpawn`; logging on failure. |
| 4 | `ExpirationServerTime` skew across hosts due to time-sync drift | Low | W16-17 time-sync handshake covers ±50 ms; ping lifetime is 3 s so worst-case 2% drift = invisible. |
| 5 | Pal listener spam-thrashing between Follow (P20) and FollowSquadCommand (P22) when player walks while a Stay command stands | Medium | `CanRun` requires `CurrentCommandTag.IsValid()`; Stay sets a non-zero tag so P22 wins as long as the command exists. Verified in activity FSM eval logic (W5-6 `EvaluateAndSwitch`). |
| 6 | Designer forgets to author `DA_SquadCommandSet_Default` | High | Console commands resolve tag → behaviour directly without needing the asset; the asset is only required for the UMG widget to draw the icon set. Logged warning on first `IssueCommand` when asset is null. |
| 7 | `MarkEnemyTraceDistance` too short on large outdoor maps | Medium | Designer knob — clamp `100..*`. Default 5000 cm covers typical FPS sightlines; raid bosses may need 8000+. |
| 8 | Vivox VOIP not in this PR — players expect "comms" outcome | Low | Roadmap deferral documented; outcome is "coordinate via ping + radial command" (text not voice). |

---

## 9. Q2 milestone progress

Q2 = W14-26. Outcome target = "Pal companion fully combat-capable +
4-player coordinate".

| Week | Status | Deliverable |
|------|--------|-------------|
| W14-15 | ✅ PR #18 merged | 4-player dedicated server foundation. |
| W16-17 | ✅ PR #19 merged | Lag compensation server-side rewind. |
| W18-19 | ✅ PR #20 merged | Pal Combat Activity. |
| W20-21 | ✅ PR #21 merged | Hostile-Pal AI subclasses (Direhound / Razorbird). |
| **W22-23** | **🟡 This PR** | **Squad system (ping + radial command).** |
| W24-25 | Pending | Extraction flow (lobby → match → results loop). |
| W26 | Pending | Q2 milestone build (devlog + playtest checklist + bug bash). |

Once this PR merges, Q2 is 5/7 weeks complete (71%). On track.

---

## 10. Next session — proposals

After this PR merges and the designer playtest signs off, candidates
for the next session (W24-25 in the roadmap):

1. **W24-25 — Extraction flow** (lobby → match → results loop). Brings
   in `UExtractionGameMode`, match phases (Warmup → Active → Extraction
   → Results), and the per-player extraction state for "did Player N
   survive the raid?". Heavy on GameState + RPC + UI.
2. **W22-23 polish PR** — UMG radial widget + Pal montage cues + ping
   audio. Smaller scope, useful if the W22-23 playtest reveals UX
   issues.
3. **W17.5 optional — Procedural mesh inventory icon renderer**.
   Skipped at the W17 boundary; can be picked up if Q2 budget allows.

User decides. Em block on user feedback after PR is green.

---

## 11. References

- Roadmap: `Documents/PALDARK/03-Roadmap_1_Year.md` § Tuần 22-23.
- Architecture: `PaldarkLab/README.md` § Squad system (W22-23).
- Activity FSM lineage: `Documents/Devlog/W18-19-Build.md` §
  "Activity FSM priority ladder".
- Pack subsystem pattern (mirrored by squad subsystem): see
  `PaldarkLab/Source/PaldarkLab/Public/Pal/PaldarkHostilePackSubsystem.h`.
- Replicated actor pattern: see
  `PaldarkLab/Source/PaldarkLab/Public/Combat/PaldarkDummyTarget.h`
  (W9-10).
- Vendor-neutral framework foundation: `Documents/UE5_Course/P17 — Experience-Driven Architecture.md`.

— Devin, W22-23 build.
