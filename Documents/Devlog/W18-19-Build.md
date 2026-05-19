# PALDARK — W18-19 Build: Pal Combat Activity

> **Roadmap reference:** `Documents/PALDARK/03-Roadmap_1_Year.md` § Tuần 18–19.
> **Status:** ✅ C++ infrastructure shipped — designer compile + playtest pending (VM has no UE5.4 compiler).
> **Date:** 2026-05-17.

## 1. Goal

Port the [04] ReadyOrNot Activity FSM idea onto the Pal companion side and
add a Utility-AI-lite scoring layer (3 considerations) borrowed from
[12] UE4 Pro. The outcome the roadmap pins is:

> **Pal companion auto-defend player khi enemy approach.**

W18-19 is a **Pal subsystem week** (like W5-6 was for the human player
side of the Activity FSM). The PR ships the threat detection +
ability-grant + activity wiring so a designer can drop a hostile dummy
near the player's Pal and watch the Pal switch into Combat, fire its
attack ability on a cadence, and switch back to Follow when the threat
clears.

W18-19 does **NOT** ship:

- Full UAIPerceptionComponent integration (W48-49 — registry subsystem).
- Hostile-Pal subclasses (Direhound pack, Razorbird aerial) — W20-21.
- GA_HitscanFire LocalPredicted refactor — W22+ (Q3 prediction loop).
- Animation montage trigger on Pal attack — W22+ (Pal AnimBP).
- Client-side prediction for Pal abilities — W22+.

## 2. What shipped

### Utility-AI-lite scoring (`Public/Pal/Combat/PaldarkPalConsideration.h`)

| Symbol | Purpose |
|--------|---------|
| `FPaldarkConsideration` | USTRUCT — `Name / NormalizedScore (0..1) / Weight (≥0)`. |
| `ComputeUtilityScore(TArray<FPaldarkConsideration>&)` | Returns the weighted average `Σ(score × weight) / Σ(weight)`. Returns 0 when the table is empty or all weights are 0. |

Designers populate the table per-Pal in
`UPaldarkActivity_Combat::BuildConsiderations` and the activity's
`CanRun` gates on `score ≥ MinUtilityScore` (default 0.45).

### Perception component (W3-4 stub → W18-19 real)

`UPaldarkPalPerceptionComponent` at
`Source/PaldarkLab/{Public,Private}/Pal/Components/PaldarkPalPerceptionComponent.{h,cpp}`:

- **Server-only tick** — `bStartWithTickEnabled = false`; `BeginPlay`
  flips it to `true` only when `GetOwner()->HasAuthority()`.
- **Scan loop** — every `ScanInterval` (0.25 s), iterate
  `TActorIterator<AActor>`, filter via `IsActorThreatening` (TeamTag
  match against `HostileTeamTag`, skip friendlies via `FriendlyTeamTag`),
  pick closest within `ThreatRadius` (1500 cm).
- **Aggro hysteresis** — new candidate must beat current `CurrentThreatDistance`
  by at least `AggroSwitchHysteresisCm` (200 cm) before switching.
- **Grace timer** — when no candidate is in range, hold `CurrentThreat`
  for `ThreatGracePeriodSeconds` (3 s) before clearing.
  `OnThreatChanged.Broadcast(old, nullptr)` fires exactly once on expiry.
- **Push-driven** — exposes `FOnPaldarkThreatChanged` multicast delegate
  bound by the combat component in its `BeginPlay`.
- **Debug** — `DumpToLog` + `ForceThreat` (authority only) for the
  console commands.

### Combat component (W3-4 stub → W18-19 real)

`UPaldarkPalCombatComponent` at
`Source/PaldarkLab/{Public,Private}/Pal/Components/PaldarkPalCombatComponent.{h,cpp}`:

- **Designer knobs** — `AttackAbilityClass` (TSoftClassPtr), `AttackInterval`
  (1.5 s default), `MinEngageRange / MaxEngageRange` (200 / 1200 cm),
  `BasePalDamage` (12).
- **BeginPlay** — server-only: `LoadSynchronous` the ability class, call
  `ASC->GiveAbility(FGameplayAbilitySpec(Class, Level=1, InputId=-1, this))`,
  stash the returned `FGameplayAbilitySpecHandle`, bind to the
  perception's `OnThreatChanged` so the target pointer auto-refreshes.
- **EndPlay** — `OnThreatChanged.RemoveAll(this)` + `ClearAbility(GrantedAttackSpec)`.
- **TryFireAttack** — gate by cooldown (`Time - LastAttackTime ≥ AttackInterval`),
  range (`MinEngageRange ≤ distance ≤ MaxEngageRange`), then
  `ASC->TryActivateAbility(GrantedAttackSpec)`. Bumps `LastAttackTime`
  only on activation success.
- **Diagnostics** — `IsAttackReady`, `GetCurrentTarget`,
  `GetCooldownRemaining`, `DumpToLog`.

### Combat activity (new)

`UPaldarkActivity_Combat` at
`Source/PaldarkLab/{Public,Private}/Pal/Activities/PaldarkActivity_Combat.{h,cpp}`:

- **Priority 40** — preempts Investigate (30), Follow (20), Idle (10).
- **InitActivity** — caches `CombatRef` + `PerceptionRef` as
  `TWeakObjectPtr<UPaldarkPalCombatComponent / Perception>` via
  `FindComponentByClass` on the owning Pal once. No per-tick re-resolve.
- **CanRun** — perception locked + target alive + utility score ≥
  `MinUtilityScore`.
- **ShouldContinue** — same as CanRun, plus a `DisengageDistance` (2000 cm)
  upper bound so the Pal escapes properly when the player + Pal flee
  together.
- **EnterActivity** — disable `FollowOwnerComponent` (so the Pal stops
  trailing while it fights) and tag the Pal with
  `Paldark.Pal.Activity.Combat`. Resets the activity-side rotation
  bookkeeping.
- **TickActivity** — every frame:
  1. Re-orient toward the target via `RInterpConstantTo` at
     `FaceTargetRateDegPerSec` (180°/s default). Uses `SetActorRotation`
     so it respects the Pal movement comp's
     `bUseControllerDesiredRotation` flag.
  2. Call `Combat->TryFireAttack()` — internally throttled by cooldown
     so safe to call every frame.
- **ExitActivity** — re-enable `FollowOwnerComponent`, remove the Combat
  tag from the ASC.

`BuildConsiderations` populates the 3 rows per-tick (re-evaluated every
0.25 s by the activity component's selection cadence):

| Name | Formula | Default weight |
|------|---------|----------------|
| `ThreatDistance` | `1 - clamp(D / Combat->MaxEngageRange, 0, 1)` | `ThreatDistanceWeight = 1.0` |
| `PalHealth` | `AS->GetHealth() / AS->GetMaxHealth()` | `PalHealthWeight = 0.6` |
| `AttackReady` | `Combat->IsAttackReady() ? 1.0 : 0.0` | `AttackReadyWeight = 0.4` |

### Activity component seed

`UPaldarkPalActivityComponent::Initialize` (cpp) now adds
`UPaldarkActivity_Combat::StaticClass()` to the default
`CandidateActivities` array alongside Idle/Follow/Investigate (W5-6). No
behavior change for designers who already overrode the array — the seed
only fills when the array is empty (Blueprint subclass takes
precedence).

### Pal attack ability (new)

`UPaldarkGameplayAbility_PalAttack` at
`Source/PaldarkLab/{Public,Private}/Gas/Abilities/PaldarkGameplayAbility_PalAttack.{h,cpp}`:

- **AI-only / server-only** — `NetExecutionPolicy = ServerOnly`.
  `InstancingPolicy = InstancedPerActor`. Tag
  `Paldark.Ability.PalAttack`.
- **No input tag** — Pal attacks are AI-driven, not input-driven. The
  combat component activates by tag, not input.
- **ActivateAbility** —
  1. Resolve owner Pal → `UPaldarkPalCombatComponent::GetCurrentTarget`.
  2. Resolve target's ASC via `IAbilitySystemInterface`.
  3. Build the GE spec, set `SetByCaller.PalDamage = Combat->BasePalDamage`,
     `ApplyGameplayEffectSpecToTarget`.
  4. `EndAbility(true)` — semi-auto, no montage gate (W22+).
- **DamageEffectClass** — null safe: when missing, logs a warning at
  `Verbose` and ends the ability without applying anything. Lets
  designers compile + validate the activation path before authoring the
  GE Blueprint.

### Damage execution extension

`UPaldarkDamageExecutionCalculation::Execute_Implementation`:

```cpp
const float WeaponDamage =
    Spec.GetSetByCallerMagnitude(TAG_Paldark_SetByCaller_BaseDamage, false, 0.f);
const float PalDamage =
    Spec.GetSetByCallerMagnitude(TAG_Paldark_SetByCaller_PalDamage,  false, 0.f);
const float TotalBase = WeaponDamage + PalDamage;
```

Both keys default to 0 when absent (`bWarnIfNotFound = false`) so
existing player GEs (W9-10) keep working unchanged. The mitigation curve
+ headshot multiplier apply to the sum.

### Team tag identity (3 actors)

| Class | Default tag | Where the default lives |
|-------|-------------|-------------------------|
| `APaldarkCharacter` | `Paldark.Team.Player` | Constructor `TeamTag = PaldarkGameplayTags::TAG_Paldark_Team_Player;` |
| `APaldarkPalCharacter` | `Paldark.Team.Player` | Constructor — companion Pal defaults to player team. |
| `APaldarkDummyTarget` | `Paldark.Team.Hostile` | Constructor — W9-10 dummy now doubles as the W18-19 hostile target. |

All three expose `UFUNCTION(BlueprintPure) FGameplayTag GetTeamTag()`.
Perception reads via the small dispatch helper
`UPaldarkPalPerceptionComponent::GetActorTeamTag`.

### Gameplay tags (5 new)

Registered in `PaldarkLabCore/PaldarkGameplayTags.{h,cpp}`:

| Tag | Used by |
|-----|---------|
| `Paldark.Pal.Activity.Combat` | `UPaldarkActivity_Combat::EnterActivity / ExitActivity` adds/removes on the Pal's ASC. |
| `Paldark.Ability.PalAttack` | AbilityTag on `UPaldarkGameplayAbility_PalAttack`. |
| `Paldark.Team.Player` | Default TeamTag on `APaldarkCharacter` + `APaldarkPalCharacter`. |
| `Paldark.Team.Hostile` | Default TeamTag on `APaldarkDummyTarget`; default `HostileTeamTag` on perception. |
| `Paldark.SetByCaller.PalDamage` | Magnitude key on the Pal attack damage GE; `UPaldarkDamageExecutionCalculation` reads it alongside `Paldark.SetByCaller.BaseDamage`. |

### Console commands (3)

| Console command | What it does |
|-----------------|--------------|
| `Paldark.Pal.DumpThreat` | Iterate every Pal in the world. For each: dump perception state (threat actor, distance, grace remaining, friendly + hostile tag, radius), combat state (target, cooldown remaining, attack-ready flag, base damage), and current activity tag to `LogPaldarkPal`. |
| `Paldark.Pal.SpawnHostileDummy [DistanceCm=600]` | Spawn one `APaldarkDummyTarget` in front of the local player at the requested distance. Defaults to 600 cm. The dummy's `Paldark.Team.Hostile` makes the nearest Pal aggro on the next scan tick. |
| `Paldark.Pal.ForceCombat [PalName] [TargetName]` | Bypass perception — call `UPaldarkPalPerceptionComponent::ForceThreat` on the named Pal with the named target. No args = first Pal in world + closest hostile dummy. Validates the activity path independent of the scan. |

All 3 register via `FAutoConsoleCommandWithWorldAndArgs` in
`Source/PaldarkLab/Private/Pal/PaldarkPalConsoleCommands.cpp`.

### Validator (`scripts/ci/validate_paldarklab.py`)

Added `check_pal_combat_shape()` (~280 LOC) — validates:

1. `FPaldarkConsideration` struct + `ComputeUtilityScore` helper exist in
   `Public/Pal/Combat/PaldarkPalConsideration.{h,cpp}`.
2. Perception component has the W18-19 shape (`ScanInterval`,
   `ThreatRadius`, `AggroSwitchHysteresisCm`, `ThreatGracePeriodSeconds`,
   `FriendlyTeamTag`, `HostileTeamTag`, `OnThreatChanged`,
   `GetCurrentThreat`, `HasThreat`, `ForceThreat`, `DumpToLog`) — not
   the W3-4 stub.
3. Combat component has `AttackAbilityClass`, `AttackInterval`,
   `MinEngageRange / MaxEngageRange`, `BasePalDamage`, `TryFireAttack`,
   `IsAttackReady`, `GetCurrentTarget`, `GetCooldownRemaining`,
   `HandleThreatChanged`, `GrantedAttackSpec`. Body has `GiveAbility`,
   `TryActivateAbility`, `LoadSynchronous`, `OnThreatChanged.AddUObject`,
   `ClearAbility`, `LastAttackTime`.
4. Combat activity has `MinUtilityScore`, `DisengageDistance`, weight
   knobs, `FaceTargetRateDegPerSec`, weak refs, the 5 activity
   overrides; body has `Priority = 40`, `ComputeUtilityScore`,
   `TryFireAttack`, `SetFollowEnabled`, `RInterpConstantTo`.
5. Pal attack ability declares the GE class + fallback knob; body uses
   `ServerOnly` policy + the 2 native tags + `ApplyGameplayEffectSpecToTarget`.
6. 3 character classes (`Player`, `Pal`, `Dummy`) expose `GetTeamTag` +
   the FGameplayTag UPROPERTY; cpp constructors initialise to the
   expected defaults.
7. `DamageExecution` reads both `BaseDamage` + `PalDamage` SetByCallers
   and sums them.
8. `UPaldarkPalActivityComponent` seeds `UPaldarkActivity_Combat::StaticClass()`
   in default `CandidateActivities`.
9. Console commands TU registers the 3 cmds via
   `FAutoConsoleCommandWithWorldAndArgs`.

5 new tags added to `EXPECTED_GAMEPLAY_TAGS` (W18-19 row).

Validator passes on the current branch (0 errors).

## 3. Files touched

```
PaldarkLab/Source/PaldarkLab/Public/Pal/Combat/PaldarkPalConsideration.h        (new)
PaldarkLab/Source/PaldarkLab/Private/Pal/Combat/PaldarkPalConsideration.cpp     (new)
PaldarkLab/Source/PaldarkLab/Public/Pal/Components/PaldarkPalPerceptionComponent.h  (W3-4 stub → W18-19 real)
PaldarkLab/Source/PaldarkLab/Private/Pal/Components/PaldarkPalPerceptionComponent.cpp (W3-4 stub → W18-19 real)
PaldarkLab/Source/PaldarkLab/Public/Pal/Components/PaldarkPalCombatComponent.h      (W3-4 stub → W18-19 real)
PaldarkLab/Source/PaldarkLab/Private/Pal/Components/PaldarkPalCombatComponent.cpp   (W3-4 stub → W18-19 real)
PaldarkLab/Source/PaldarkLab/Public/Pal/Activities/PaldarkActivity_Combat.h     (new)
PaldarkLab/Source/PaldarkLab/Private/Pal/Activities/PaldarkActivity_Combat.cpp  (new)
PaldarkLab/Source/PaldarkLab/Private/Pal/Components/PaldarkPalActivityComponent.cpp (+ Combat seed in CandidateActivities)
PaldarkLab/Source/PaldarkLab/Public/Gas/Abilities/PaldarkGameplayAbility_PalAttack.h     (new)
PaldarkLab/Source/PaldarkLab/Private/Gas/Abilities/PaldarkGameplayAbility_PalAttack.cpp  (new)
PaldarkLab/Source/PaldarkLab/Private/Gas/PaldarkDamageExecutionCalculation.cpp  (+ PalDamage sum)
PaldarkLab/Source/PaldarkLab/Public/Player/PaldarkCharacter.h                   (+ TeamTag + GetTeamTag)
PaldarkLab/Source/PaldarkLab/Private/Player/PaldarkCharacter.cpp                (+ TeamTag default)
PaldarkLab/Source/PaldarkLab/Public/Pal/PaldarkPalCharacter.h                   (+ TeamTag + GetTeamTag)
PaldarkLab/Source/PaldarkLab/Private/Pal/PaldarkPalCharacter.cpp                (+ TeamTag default)
PaldarkLab/Source/PaldarkLab/Public/Combat/PaldarkDummyTarget.h                 (+ TeamTag + GetTeamTag)
PaldarkLab/Source/PaldarkLab/Private/Combat/PaldarkDummyTarget.cpp              (+ TeamTag default Hostile)
PaldarkLab/Source/PaldarkLab/Private/Pal/PaldarkPalConsoleCommands.cpp          (new — 3 console cmds)
PaldarkLab/Source/PaldarkLabCore/Public/PaldarkGameplayTags.h                   (+ 5 tag decls)
PaldarkLab/Source/PaldarkLabCore/Private/PaldarkGameplayTags.cpp                (+ 5 tag defs)
PaldarkLab/README.md                                                            (+ §"Pal Combat Activity (W18-19)")
scripts/ci/validate_paldarklab.py                                               (+ check_pal_combat_shape + 5 tags)
Documents/Devlog/W18-19-Build.md                                                (new — this file)
```

Approximate counts: ~23 files touched (8 new C++ TUs, 7 modified, 4 ini/markdown, 4 stub→real promotion). Plan markdown was attached to chat — not committed to repo (per project convention).

## 4. Compile + test plan (designer side)

VM that built this PR has **no UE 5.4 compiler**, so the structural CI
validator is the only Devin-side gate. Compile + PIE testing must run on
the designer's machine.

### Compile

1. Right-click `PaldarkLab.uproject` → **Generate Visual Studio project files**.
2. Open `PaldarkLab.sln` in Visual Studio 2022.
3. Build **Development Editor | Win64** — expect 0 errors.

Likely risk areas (call-sites added in this PR — if any fail to compile,
the error message localises directly to one file):

- **`UPaldarkPalCombatComponent` ASC resolution** — `BeginPlay` calls
  `Pal->GetAbilitySystemComponent()` (declared via `IAbilitySystemInterface`).
  `APaldarkPalCharacter` already implements the interface (W7-8); if a
  build-fail localises here, ensure `Pal/PaldarkPalCharacter.h` is
  included before the cast.
- **`TSoftClassPtr<UPaldarkGameplayAbility_PalAttack>::LoadSynchronous`** —
  returns `UClass*`. The combat cpp uses
  `Cast<UClass>(AttackAbilityClass.LoadSynchronous())`. If
  `LoadSynchronous` already returns the right type, the cast is
  redundant but harmless.
- **`UEnhancedInputComponent` not needed here** — Pal abilities are
  AI-driven so the ability ctor must NOT call `SetupInputBinding`. The
  base `UPaldarkGameplayAbility` already handles this correctly via the
  empty `ActivationInputTag`.
- **`Movement->bUseControllerDesiredRotation`** — the Pal must already
  have its movement comp configured with
  `bOrientRotationToMovement=false; bUseControllerDesiredRotation=true`
  in the constructor (W3-4 set this). The combat activity's
  `SetActorRotation` would fight `bOrientRotationToMovement=true` every
  frame. If the Pal twitches in Combat, this is the culprit.
- **`UPaldarkActivity_Combat::InitActivity` cast targets** — needs
  `UPaldarkPalCombatComponent` + `UPaldarkPalPerceptionComponent`
  headers; both are forward-declared in the activity .h and pulled in
  the cpp.
- **Native tag UE_DECLARE / UE_DEFINE pairing** — the 5 new tags in
  `PaldarkGameplayTags.{h,cpp}` must match exactly between the
  `UE_DECLARE_GAMEPLAY_TAG_EXTERN` and `UE_DEFINE_GAMEPLAY_TAG` lines.
  Mismatched names produce link errors at module init.

### PIE test loop

1. **Designer authoring** (one-time):
   - Author `GE_Damage_Pal` (or reuse `GE_Damage_Standard` from W9-10 —
     the execution sums both keys).
   - Author `BP_GA_Pal_Attack` subclass `UPaldarkGameplayAbility_PalAttack`,
     set `DamageEffectClass = GE_Damage_Pal`.
   - Author `BP_Pal_Default` subclass `APaldarkPalCharacter`. On its
     CombatSlot default, set `AttackAbilityClass = BP_GA_Pal_Attack`.

2. **Smoke (single Pal, single dummy)** — Standalone, 1 player.
   - Open `Raid_Sandbox?Experience=PaldarkExperience.PX_RaidSandbox`.
   - `Paldark.Pal.SpawnTestCompanion` — spawn one Pal.
   - `Paldark.Pal.SpawnHostileDummy 800` — spawn dummy 8 m forward.
   - Within ~0.25 s `LogPaldarkPal` logs `SwitchToActivity` from
     `Paldark.Pal.Activity.Follow` → `Paldark.Pal.Activity.Combat`.
   - Pal rotates to face the dummy (visible in 3rd-person camera).
   - Every 1.5 s Pal fires; `LogPaldarkPal` logs
     `TryFireAttack ok target=PaldarkDummyTarget_C_X distance=…`.
   - Dummy Health drops 12 per shot (or BasePalDamage value).
   - After 9 shots dummy enters `Paldark.State.IsDead`, perception loses
     it; after 3 s grace, Pal switches Combat → Follow.

3. **Aggro hysteresis** — Standalone, 1 player.
   - Spawn 2 dummies at ~600 + ~900 cm:
     `Paldark.Pal.SpawnHostileDummy 600; Paldark.Pal.SpawnHostileDummy 900`.
   - `Paldark.Pal.DumpThreat` → confirm Pal targets the closer one
     (600 cm).
   - Walk so the further dummy is now ~100 cm closer than current pick
     (< 200 cm hysteresis). **Pal SHOULD NOT switch.** Confirm via
     `Paldark.Pal.DumpThreat`.
   - Walk another 150 cm so the delta is ~250 cm. Pal switches —
     `LogPaldarkPal` logs `aggro switch old=… new=… new_distance=…`.

4. **Grace timer** — Standalone, 1 player.
   - Spawn 1 dummy at 1000 cm. Pal enters Combat.
   - Destroy the dummy via `Paldark.Combat.KillDummy` or walk it out of
     `ThreatRadius` (1500 cm).
   - `Paldark.Pal.DumpThreat` → grace counts down 3.00 → 2.75 → … → 0.00.
   - On expiry: activity switches Combat → Follow.

5. **ForceCombat fallback** — Standalone, 1 player.
   - Spawn 1 dummy 5000 cm away (out of `ThreatRadius`).
   - `Paldark.Pal.ForceCombat` (no args) — Pal jumps straight to Combat
     and tries to fire (will log `out_of_range` because dummy >
     `MaxEngageRange`). Walks toward dummy — once inside 1200 cm, Pal
     starts firing. Validates the activity / ability path is healthy
     even when perception didn't drive it.

6. **2-player dedicated** (after compile xanh + DS target builds):
   - Launch `PaldarkLabServer` headless on port 7777.
   - 2 PIE clients join. Each client spawns 1 companion Pal +
     `Paldark.Pal.SpawnHostileDummy 600`.
   - Both Pals enter Combat **on the server only** (perception is
     authority-side). Clients see the Pal rotating + Health drops on
     dummy (replicated). `Paldark.Pal.DumpThreat` on a client returns
     null perception (correct — clients don't run the scan).

### Recording

W18-19 milestone artefact (per roadmap): 30-60 s PIE video showing the
following sequence end-to-end:

1. PIE start with empty `Raid_Sandbox`.
2. `Paldark.Pal.SpawnTestCompanion` — Pal appears.
3. `Paldark.Pal.SpawnHostileDummy 800` — dummy appears.
4. Pal switches Follow → Combat, rotates, fires 9 shots, dummy dies.
5. After grace timer, Pal switches Combat → Follow.

Save as `Documents/Devlog/Q2-W18-PalCombat-Demo.mp4`.

## 5. What's NOT in W18-19 (deferred)

| Item | Lands in | Why |
|------|----------|-----|
| Direhound pack / Razorbird aerial AI | W20-21 | Hostile-Pal subclasses with shared blackboard + flock cohesion. Different content week from "wire the Combat activity". |
| GA_HitscanFire → LocalPredicted refactor | W22+ (Q3) | Player Fire ability stays ServerOnly. W18-19 already wires `ServerScoreRequest_Hitscan` from W16-17; the predicted path requires the full prediction loop. |
| Pal montage trigger | W22+ | Need Pal AnimBP + Anim Notify + Montage State + Wait Montage Notify ability task. Out of scope for "activity drives an ability". |
| Per-species ability authoring | W30+ | Each Pal species needs its own AttackAbilityClass + montage + damage GE. W18-19 ships one generic Pal attack as the activity smoke test. |
| Pal client-side prediction | W22+ | Whole prediction loop is the Q3 milestone. W18-19 stays server-authoritative. |
| UAIPerceptionComponent integration | W48-49 | TActorIterator scan is fine for 1-4 Pals in the W18-19 sandbox; registry subsystem lands when significance manager work does. |
| Pal HUD / threat indicator | W22+ (Q3 UI) | Lyra Indicator / CommonUI integration is its own milestone. |
| Anti-cheat for Pal damage spec | W42-43 (live ops) | Server-only execution already prevents the obvious cheats; backend rate limits + correlation analysis is the AWS week. |

## 6. Pillar coverage (vs the 18 unified pillars)

| Pillar | W18-19 contribution |
|--------|--------------------|
| P02 L1 / L4 Class framework + subsystem-vs-component | Combat + perception are per-Pal components (per-actor state), not subsystems. |
| P03 L2 Component slots | Existing slot pattern reused — PerceptionSlot / CombatSlot / ActivitySlot promoted from W3-4 stubs without re-shaping the slot table. |
| P08 L1-L3 GAS (ASC + GA + GE) | Pal attack uses the existing ASC, lifecycle managed by combat comp (GiveAbility/ClearAbility), damage applied via `SetByCaller.PalDamage`. |
| P09 L1 Activity FSM (RoN-style) | Idle/Follow/Investigate/Combat quartet now complete with priority preemption. |
| P09 L3 Utility AI lite | First implementation of the consideration / weighted-average pattern in the Paldark codebase. |
| P09 L5 Custom perception | Bespoke TActorIterator + radius filter, no UAIPerceptionComponent dependency. |
| P12 L1 Data-driven | Every knob is `UPROPERTY(EditDefaultsOnly)` on Blueprint subclass — designer tunes 14 floats + 1 ability class without C++ changes. |

## 7. Anti-patterns to avoid (Pal combat)

- **`bOrientRotationToMovement = true`** — fights `SetActorRotation` in
  Combat tick. Use `bUseControllerDesiredRotation = true` (W3-4 already
  sets this — keep it).
- **Granting ability from constructor** — ASC ability grants need the
  ASC + AvatarActor fully initialised. Right hook is combat
  component's `BeginPlay` on authority.
- **Per-tick `FindComponentByClass`** — cache combat + perception in
  `InitActivity`. Selection cadence (0.25 s) makes the cost stack up
  with many Pals.
- **Reading attribute raw members** — go through `GetX` accessors.
  `FGameplayAttributeData` mid-application has stale values.
- **Forgetting `OnThreatChanged.RemoveAll(this)` in EndPlay** — multicast
  holds raw `this`; perception can outlive the combat comp on actor
  destroy ordering edge cases.
- **`TryActivateAbility` ignoring return** — when ASC rejects (cooldown,
  blocked tag, missing GE), don't bump `LastAttackTime`. Current code
  guards via `if (bActivated) LastAttackTime = …;`.
- **Hostile detection by class cast alone** — friends-of-friends pattern
  (Pal A team-tags itself Hostile by accident) silently aggro's. Always
  filter Friendly first, then check Hostile.
- **Server-only tick but missing authority gate** — listen-server hosts
  that `ServerTravel` can flip authority underneath the component.
  `TickComponent` re-checks `HasAuthority()` belt-and-braces even though
  `BeginPlay` already gates `SetComponentTickEnabled`.

## 8. Next

- **W20-21** — Hostile-Pal AI subclasses (Direhound pack, Razorbird
  aerial). Reuses W18-19 perception + combat shape with overridden
  abilities + flock behavior. Per roadmap § Tuần 20–21.
- **W17.5 (optional polish, if designer wants UI before Q3)** —
  Inventory UMG widget + pickup actor. Slips before W20 if visible
  inventory beats the W20 content week priority.
- **Q3 (W22+)** — Client-side prediction loop on top of W16-17 rewind +
  W18-19 server abilities.

## 9. Q2 milestone alignment

W18-19 closes the **Q2 "Pal companion fully combat-capable"** scope
from the roadmap. With W14-15 (4-player DS) + W16-17 (lag comp) +
W18-19 (Pal combat), the Q2 milestone is:

- ✅ Networking infrastructure (W14-15).
- ✅ Lag compensation for hitscan (W16-17).
- ✅ Pal companion auto-defends player (W18-19).

The Q2 milestone build (analog of W13's Q1 milestone) is now the
designer's task — author the 2-3 Blueprints listed in §4 step 1, run
the PIE test loop in §4 steps 2-6, record the 30-60 s video, and ship
a `Documents/Devlog/Q2-Milestone-Build.md` summary closing the quarter.
