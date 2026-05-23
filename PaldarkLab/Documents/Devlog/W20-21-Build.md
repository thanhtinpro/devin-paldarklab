# PALDARK — W20-21 Build: Hostile-Pal AI subclasses

> **Roadmap reference:** `Documents/PALDARK/03-Roadmap_1_Year.md` § Tuần 20–21.
> **Status:** ✅ C++ infrastructure shipped — designer compile + playtest pending (VM has no UE5.4 compiler).
> **Date:** 2026-05-17.

## 1. Goal

The roadmap pins one outcome for W20-21:

> **Map test có 4 Direhound patrol, attack player khi gần.**

This is the first week the Pal stack ships an actor that is *not* a
companion. The W18-19 perception + combat + utility-AI plumbing already
treats any actor with `TeamTag = Paldark.Team.Hostile` as a threat; the
only thing missing was a *hostile* Pal class to play that role. W20-21
adds two of them — Direhound (ground pack predator) and Razorbird
(aerial dive-bomber) — plus the two activities (Patrol P15, Stalk P25)
they need to behave plausibly, a `UWorldSubsystem` pack registry for
shared threat broadcasting, and a designer-placeable spawner actor.

W20-21 does **NOT** ship:

- A full `UAIPerceptionComponent` integration (W48-49 — registry subsystem).
- New C++ ability classes — `BP_GA_Direhound_Bite` /
  `BP_GA_Razorbird_DivePeck` are designer-side Blueprint subclasses of
  the W18-19 `UPaldarkGameplayAbility_PalAttack`.
- Pal animation montages — Pal attack still applies damage instantly.
- Flock cohesion / boids — Razorbird is solo, Direhound pack only
  shares threat sightings (not formation).
- Squad / alpha hierarchy — that's the W22-23 deliverable.
- Save/load pack composition across levels — pack registry is `Transient`.

## 2. What shipped

### Gameplay tags (`Source/PaldarkLabCore/{Public,Private}/PaldarkGameplayTags.{h,cpp}`)

| Tag | Purpose |
|-----|---------|
| `Paldark.Pal.Species.Direhound` | Direhound subclass identity (set in ctor). |
| `Paldark.Pal.Species.Razorbird` | Razorbird subclass identity (set in ctor). |
| `Paldark.Pal.Activity.Patrol` | P15 activity identity, used by activity FSM match. |
| `Paldark.Pal.Activity.Stalk` | P25 activity identity. |
| `Paldark.Pal.Pack.Direhound` | Default pack tag for the Direhound spawner / console helper. Razorbird leaves PackTag empty. |

All 5 tags follow the existing `UE_DECLARE_GAMEPLAY_TAG_EXTERN` /
`UE_DEFINE_GAMEPLAY_TAG_COMMENT` pattern. Validator
(`check_gameplay_tags`) asserts both the declaration and the definition.

### Patrol component (`Source/PaldarkLab/{Public,Private}/Pal/Components/PaldarkPalPatrolComponent.{h,cpp}`)

The 9th Pal component slot. Lives next to the W3-4 slot family but is
purely additive — no existing slot changes.

| Field / method | Purpose |
|----------------|---------|
| `TArray<FVector> Waypoints` | Designer-authored patrol points (world-space). |
| `float ArrivalRadius = 150.f` | Distance threshold for `AdvanceToNextWaypoint`. |
| `bool bRandomizeStartIndex = true` | Different pack members start at different waypoints so they spread out. |
| `int32 CurrentWaypointIndex` | Wraps around `(Index + 1) % Waypoints.Num()`. |
| `HasWaypoints() / GetCurrentWaypoint() / AdvanceToNextWaypoint()` | Activity uses these — no UPROPERTY access from the activity. |
| `SetWaypointsFromLocations(TArray<FVector>&)` | Spawner calls this after converting `WaypointActors` to `FVector`s. |
| `DumpToLog()` | Used by `Paldark.Pal.DumpPackState`. |

Component is *not* replicated — perception + combat are server-only,
so waypoints + index live on the server only. Clients see Pals moving
via the existing `SetReplicateMovement(true)` on `APaldarkPalCharacter`.

### Patrol activity P15 (`Source/PaldarkLab/{Public,Private}/Pal/Activities/PaldarkActivity_Patrol.{h,cpp}`)

```cpp
ActivityTag = Paldark.Pal.Activity.Patrol;
Priority    = 15;
```

| Lifecycle | Behaviour |
|-----------|-----------|
| `CanRun_Implementation` | `Patrol->HasWaypoints() && !Perception->HasThreat()`. |
| `ShouldContinue_Implementation` | Same as `CanRun` — no hysteresis needed (Patrol vs Idle delta is just "do we have waypoints"). |
| `EnterActivity_Implementation` | `Locomotion->SetFollowEnabled(false)` — pauses the W5-6 follow leash so Patrol's `AddMovementInput` doesn't fight it. |
| `TickActivity_Implementation` | Steers toward `Patrol->GetCurrentWaypoint()` with `AddMovementInput(Direction)`. When within `ArrivalRadius`, calls `Patrol->AdvanceToNextWaypoint()`. |
| `ExitActivity_Implementation` | `Locomotion->SetFollowEnabled(true)`. |

The W5-6 follow leash is unrelated to hostile-Pal patrolling — hostile
Pals never `Follow` a player — but the activity still toggles the
follow flag so that mixed-role Pals (e.g. a Pal that's friendly until
it gets attacked, then becomes hostile, W30+ task) don't end up with
both systems pumping `AddMovementInput`.

### Stalk activity P25 (`Source/PaldarkLab/{Public,Private}/Pal/Activities/PaldarkActivity_Stalk.{h,cpp}`)

```cpp
ActivityTag = Paldark.Pal.Activity.Stalk;
Priority    = 25;
```

| Field / lifecycle | Behaviour |
|-------------------|-----------|
| `StalkSpeedScale = 0.6f` | Multiplier applied to `Movement->MaxWalkSpeed` on enter; restored on exit. |
| `LoseSightDistance = 2200.f` | Above this, Stalk's `CanRun` returns false even if `Perception->HasThreat()` (Combat's `DisengageDistance` is 2000 cm — gives Stalk a tiny grace). |
| `CanRun_Implementation` | `Perception->HasThreat() && distance > Combat->MinEngageRange && distance <= LoseSightDistance`. |
| `EnterActivity_Implementation` | Cache `Movement->MaxWalkSpeed`, set `Movement->MaxWalkSpeed *= StalkSpeedScale`. |
| `TickActivity_Implementation` | Steer `AddMovementInput` toward `Perception->GetCurrentThreat()->GetActorLocation()`. |
| `ExitActivity_Implementation` | Restore cached `MaxWalkSpeed`. |

Combat (P40) preempts Stalk the moment `distance <= MinEngageRange` —
that's how the FSM transitions from "close in" to "attack". The split
between Stalk and Combat is what gives the Direhound its readable
"creep up + lunge" silhouette.

### Pack subsystem (`Source/PaldarkLab/{Public,Private}/Pal/PaldarkHostilePackSubsystem.{h,cpp}`)

`UPaldarkHostilePackSubsystem : public UWorldSubsystem`. World-scoped
shared blackboard for hostile-Pal pack coordination.

```cpp
USTRUCT() struct FPaldarkHostilePack
{
    UPROPERTY(Transient) TArray<TWeakObjectPtr<APaldarkPalCharacter>> Members;
};
UPROPERTY(Transient) TMap<FGameplayTag, FPaldarkHostilePack> PackRegistry;
```

| Method | Behaviour |
|--------|-----------|
| `ShouldCreateSubsystem(Outer)` | Returns true only on `EWorldType::Game` and `EWorldType::PIE` — skips editor preview / inactive worlds. |
| `Initialize(Collection)` | Logs the configured `PackBroadcastRadius` to `LogPaldarkPal`. |
| `Deinitialize()` | Resets `PackRegistry`. |
| `RegisterPalToPack(Pal, Tag)` | Authority-only, idempotent. Adds the Pal as a `TWeakObjectPtr` under `Tag`. |
| `UnregisterPalFromPack(Pal)` | Iterates every pack entry and removes the Pal's weak ref. |
| `BroadcastPackThreat(Initiator, NewThreat)` | Authority-only. Iterates the initiator's pack, filters by `PackBroadcastRadius` (default 2000 cm), calls `Perception->ForceThreat(NewThreat)` on each packmate that doesn't already have the same target. |
| `GetPackSize(Tag)` | Debug accessor — count of currently-registered Pals under `Tag`. Stale weak refs counted as members. |
| `DumpToLog()` | Iterate registry + log per-pack member list with locations + species. |

The "Pal A broadcasts to B, B broadcasts to A" recursion is prevented
by the `Perception->GetCurrentThreat() == NewThreat` guard inside
`BroadcastPackThreat` — if a packmate already has the same target,
`ForceThreat` is skipped (which would otherwise re-broadcast
`OnThreatChanged` with the same value and recurse forever).

### Base APaldarkPalCharacter changes

`Source/PaldarkLab/{Public,Private}/Pal/PaldarkPalCharacter.{h,cpp}` gains:

- New UPROPERTY fields: `FGameplayTag SpeciesTag` (default empty),
  `FGameplayTag PackTag` (default empty).
- New accessors: `GetSpeciesTag()`, `GetPackTag()`, `SetPackTag(Tag)`
  (authority check + log), `GetPatrolSlot()`.
- New 9th component slot constructed in the constructor:
  `PatrolSlot = CreateDefaultSubobject<UPaldarkPalPatrolComponent>(TEXT("PatrolSlot"));`.

The friendly companion (`BP_Pal_Default` from W18-19) leaves all the
new fields empty — `SpeciesTag.IsValid() == false`, `PackTag.IsValid()
== false`, `PatrolSlot->Waypoints.Num() == 0` — so Patrol's `CanRun`
returns false and the Pal falls through to Follow / Investigate as
before. Backward-compatible.

### Combat component change (pack broadcast)

`Source/PaldarkLab/Private/Pal/Components/PaldarkPalCombatComponent.cpp`
gains a single hook in `HandleThreatChanged` — when a new (non-null,
changed) threat arrives:

```cpp
if (UPaldarkHostilePackSubsystem* PackSub = World->GetSubsystem<UPaldarkHostilePackSubsystem>())
{
    PackSub->BroadcastPackThreat(PalOwner, NewThreat);
}
```

The subsystem itself filters by `PackTag.IsValid()` (lone Pals
short-circuit) so this is safe to call from *every* Pal's combat
component, friendly or hostile.

### Direhound subclass (`Source/PaldarkLab/{Public,Private}/Pal/PaldarkPalCharacter_Direhound.{h,cpp}`)

Ground pack predator. Constructor is the entire data definition (no
data asset, per roadmap "spawn manager cơ bản"):

- `TeamTag = Paldark.Team.Hostile`.
- `SpeciesTag = Paldark.Pal.Species.Direhound`.
- `PackTag = Paldark.Pal.Pack.Direhound` (default — designer can flip
  per-instance for sub-packs).
- Perception: `FriendlyTeamTag = Hostile, HostileTeamTag = Player`.
- Combat: `MinEngageRange = 60, MaxEngageRange = 250, AttackInterval =
  1.0, BasePalDamage = 18` (melee bite).
- Movement: `MaxWalkSpeed = 650`.
- ActivitySlot: `CandidateActivities = [Idle, Patrol, Stalk, Combat]`
  (no Follow / Investigate — replaced, not added).

`BeginPlay` (authority only): auto-assigns the default
`Paldark.Pal.Pack.Direhound` tag if `PackTag` is empty (covers
console-spawned and direct-C++-spawned Direhounds), calls
`PackSub->RegisterPalToPack(this, PackTag)`. `EndPlay` calls
`UnregisterPalFromPack`.

### Razorbird subclass (`Source/PaldarkLab/{Public,Private}/Pal/PaldarkPalCharacter_Razorbird.{h,cpp}`)

Aerial dive-bomber. Lone Pal by default — designer can group multiple
into one pack via Blueprint subclass `PackTag` override.

- `TeamTag = Paldark.Team.Hostile`.
- `SpeciesTag = Paldark.Pal.Species.Razorbird`.
- `PackTag` left empty (lone).
- Perception: `ThreatRadius = 2200` (wider — birds see further),
  team flip same as Direhound.
- Combat: `MinEngageRange = 100, MaxEngageRange = 800, AttackInterval =
  2.0, BasePalDamage = 14` (ranged peck).
- Movement: `DefaultLandMovementMode = MOVE_Flying`, `GravityScale = 0`,
  `MaxFlySpeed = 700`.
- ActivitySlot: same `[Idle, Patrol, Stalk, Combat]` quartet.

`BeginPlay` registers with the pack subsystem **only** if `PackTag` is
valid — covers the designer override case without forcing a pack tag
on every lone bird.

### Spawner actor (`Source/PaldarkLab/{Public,Private}/Pal/PaldarkHostilePalSpawner.{h,cpp}`)

Designer-placeable in the level. Fields:

```cpp
UPROPERTY(EditAnywhere) TSubclassOf<APaldarkPalCharacter> PalClassToSpawn;
UPROPERTY(EditAnywhere) int32                              SpawnCount       = 4;
UPROPERTY(EditAnywhere) FGameplayTag                       PackTag;
UPROPERTY(EditAnywhere) TArray<TObjectPtr<AActor>>         WaypointActors;
UPROPERTY(EditAnywhere) float                              SpawnSpreadRadius = 300.f;
UPROPERTY(EditAnywhere) TEnumAsByte<ECollisionChannel>     GroundTraceChannel = ECC_Visibility;
UPROPERTY(EditAnywhere) float                              GroundTraceUp    = 200.f;
UPROPERTY(EditAnywhere) float                              GroundTraceDown  = 2000.f;
```

`BeginPlay` (server / standalone only):

1. Distribute `SpawnCount` spawn positions on a ring of radius
   `SpawnSpreadRadius` around the actor location (even angular
   spacing).
2. For each position: line-trace `[+Up, -Down]` against `GroundTraceChannel`
   to ground-snap (so the spawner can be placed at a designer-friendly
   high altitude without each Pal popping into the air).
3. `World->SpawnActor<APaldarkPalCharacter>(PalClassToSpawn, …)` at
   the snapped transform.
4. `Pal->SetPackTag(PackTag)` (stamp the runtime pack tag).
5. `Pal->GetPatrolSlot()->SetWaypointsFromLocations(WaypointLocations)`
   — converts the designer's `WaypointActors` array into a
   `TArray<FVector>` and assigns to the patrol component.

The static helper `SpawnHostilePackAt(World, Class, PackTag, Origin,
Count, SpreadRadius, Waypoints)` is exposed for console-driven
spawning (used by all 3 W20-21 console commands).

### Console commands (`Source/PaldarkLab/Private/Pal/PaldarkPalConsoleCommands.cpp`)

The W18-19 console TU gains 3 new commands:

| Command | Behaviour |
|---------|-----------|
| `Paldark.Pal.SpawnDirehoundPack [Count=4] [DistanceCm=1200]` | Spawn N Direhound hostiles in a ring `DistanceCm` cm in front of the local player + auto-generate 4 ring waypoints (radius 800 cm) so the pack immediately has a patrol path. Default `Paldark.Pal.Pack.Direhound` tag. |
| `Paldark.Pal.SpawnRazorbird [DistanceCm=1500]` | Spawn one Razorbird 4 m above ground in front of the player. No pack tag. No waypoints — Patrol falls back to Idle until perception fires. |
| `Paldark.Pal.DumpPackState` | Dump `UPaldarkHostilePackSubsystem` registry + every hostile Pal's species/pack/activity state + patrol waypoint list to `LogPaldarkPal`. |

All 3 commands are server / standalone only (`FindLocalPlayerController`
filter). They all route through `APaldarkHostilePalSpawner::SpawnHostilePackAt`
so the spawn / patrol / pack-register flow is identical between
"placed in level" and "console-triggered" paths.

### Validator extension (`scripts/ci/validate_paldarklab.py`)

New `check_hostile_pal_shape()` function runs in `main()` after
`check_pal_combat_shape()`. Asserts (per the docstring):

1. `UPaldarkPalPatrolComponent` shape — `Waypoints`,
   `CurrentWaypointIndex`, `ArrivalRadius`, accessors + matching `.cpp`.
2. `UPaldarkActivity_Patrol` shape — overrides + Priority=15 +
   `AddMovementInput` in cpp.
3. `UPaldarkActivity_Stalk` shape — `StalkSpeedScale`,
   `LoseSightDistance`, Priority=25 + `MaxWalkSpeed` in cpp.
4. `UPaldarkHostilePackSubsystem` shape — `UWorldSubsystem` base,
   designer knobs, broadcast + dump entry points.
5. Base `APaldarkPalCharacter` exposes `SpeciesTag` / `PackTag` /
   `PatrolSlot` / `SetPackTag`.
6. `APaldarkPalCharacter_Direhound` + `APaldarkPalCharacter_Razorbird`
   exist with the right SpeciesTag stamp + activity list override +
   pack register/unregister calls. Razorbird must set `MOVE_Flying`.
7. `APaldarkHostilePalSpawner` exposes the designer knobs +
   `SpawnHostilePackAt` helper + uses `SpawnActor<APaldarkPalCharacter>`
   / `SetWaypointsFromLocations` / `SetPackTag` in the body.
8. `UPaldarkPalCombatComponent::HandleThreatChanged` references the
   pack subsystem + calls `BroadcastPackThreat`.
9. Console TU registers all 3 W20-21 commands.

5 new gameplay tags added to `EXPECTED_GAMEPLAY_TAGS`. 2 new activity
classes added to `EXPECTED_ACTIVITY_CLASSES`. 1 new Pal slot added to
`EXPECTED_PAL_COMPONENT_CLASSES` (count goes from 8 → 9; the count
assertion in `check_pal_slot_classes` is bumped accordingly).

## 3. Architecture decisions

### Why `UWorldSubsystem` for the pack registry

Considered three alternatives:

1. **`TActorIterator` at broadcast time** — every `HandleThreatChanged`
   call would iterate every actor in the world to find packmates.
   O(actors × Pals) — fine for 4 Direhounds but melts at W48-49 scale.
2. **Per-Pal RPC fan-out** — `MulticastPackBroadcast` on the Pal
   itself. Cleaner ownership but no shared state, every Pal would
   need to know about every other Pal independently, and the
   broadcast couldn't be filtered by `PackTag` without the same
   `TActorIterator` cost.
3. **`UWorldSubsystem` registry** ✅ chosen. Server-authoritative,
   constant-time pack-mate lookup, lifetime matches the world (resets
   on level transition / ServerTravel), no per-Pal iteration cost.

The cost: a `TMap<FGameplayTag, FPaldarkHostilePack>` per world, one
weak ref per Pal — negligible memory. Cheap enough that W22-23
(Squad system) can layer on top without re-architecting.

### Why constructor-as-data (not `UPaldarkHostilePalData`)

The roadmap text explicitly says "spawn manager cơ bản". The W11-12
inventory ships full data-driven assets (`UPaldarkItemDefinition`)
because there are dozens of items; W20-21 ships exactly 2 species in
its first cut. Folding the species-level data into the C++ subclass
constructor keeps the data flow simple: designer authors a Blueprint
subclass of `APaldarkPalCharacter_Direhound`, overrides
`AttackAbilityClass` / mesh / anim BP, and that's it. No data asset
to round-trip.

When the species roster grows past ~4 species (W30+ Pal expansion),
the migration path is well-trodden: add `UPaldarkPalSpeciesData :
UPrimaryDataAsset`, register the primary asset type in
`UPaldarkAssetManager`, move the ctor data into the asset. Same
pattern as `UPaldarkItemDefinition`.

### Why Patrol gates on `!Perception->HasThreat()`

Patrol's priority is 15, lower than Stalk (25) and Combat (40). If
Patrol's `CanRun` only checked `HasWaypoints()`, the FSM would still
correctly preempt with Stalk on threat — Patrol's `TickActivity`
would just stop firing. So why the explicit threat gate?

Two reasons:

1. **DumpThreat readability** — when a designer runs
   `Paldark.Pal.DumpThreat` mid-combat, the activity tag should read
   `Paldark.Pal.Activity.Stalk` or `Paldark.Pal.Activity.Combat`, not
   "Patrol is still 'valid'". The gate makes the dump match the
   designer's mental model.
2. **Activity transition log noise** — without the gate, every
   re-evaluation cycle (0.25 s) would re-check Patrol's `CanRun`,
   succeed, and the FSM would never bother logging "Patrol → Stalk".
   With the gate, the log shows a clean Patrol → Stalk transition on
   the first scan that fires `Perception->HasThreat()`.

### Why Stalk caches `MaxWalkSpeed` instead of using a movement modifier

The W7-8 sprint ability uses a GE-based MoveSpeed modifier. Could
Stalk do the same — author `GE_PalStalk_MoveSpeedScale` with `Override
* 0.6` and apply on enter?

Considered + rejected for W20-21:

- Hostile Pals don't have a `UPaldarkAttributeSet` yet. Setting up
  the AttributeSet on every hostile Pal subclass would be a 2-day
  detour into GAS plumbing for marginal gameplay value.
- The MaxWalkSpeed scale is a behavioural knob, not a stat. Designers
  tune `StalkSpeedScale` per species in the C++ subclass ctor; it
  doesn't need to be a GE.
- The cache-and-restore pattern is `O(1)` work per Enter/Exit. No GC
  pressure, no GE spec lifetime to manage.

When hostile-Pal AttributeSet lands (W30+ for damage-over-time effects
etc.), Stalk migrates to a proper GE. For now the direct field write
is the right move.

## 4. Test plan (designer side — VM has no UE5 compiler)

Devin VM cannot compile UE5 C++. Designer playtest is required for
W20-21 closure. The roadmap deliverable target is:

> Map test có 4 Direhound patrol, attack player khi gần.

### 4.1 Smoke test — console only

1. Generate Project Files + build in UE 5.4 Editor.
2. Open `Raid_Sandbox?Experience=PaldarkExperience.PX_RaidSandbox`.
3. `Paldark.Pal.SpawnDirehoundPack 4 1500` — expect:
   - Output Log: `RegisterPalToPack` × 4, then 4 spawn lines.
   - In-world: 4 Direhound capsules patrol an 8 m ring around the
     spawn origin (no mesh yet — that's authored later).
4. Walk toward the pack. As soon as one Direhound's perception locks
   onto the player:
   - Output Log: `OnThreatChanged → BroadcastPackThreat → notified=3/4`.
   - All 4 Direhounds (within 2000 cm of the first to detect) switch
     to Stalk / Combat on the next 0.25 s eval.
5. Direhounds close to within `MinEngageRange = 60` cm and start
   biting at `AttackInterval = 1.0 s`. (No animation montage —
   damage applies instantly via the W18-19 PalAttack ability.)
6. `Paldark.Pal.DumpPackState` — confirm registry shows 4 members
   under `Paldark.Pal.Pack.Direhound` + each Pal's activity tag is
   `Paldark.Pal.Activity.Combat`.

### 4.2 Razorbird smoke test

1. `Paldark.Pal.SpawnRazorbird 1500` — expect:
   - Output Log: 1 spawn line, no `RegisterPalToPack` (lone Pal).
   - In-world: 1 Razorbird hovers 4 m above ground in front of the
     player.
2. Razorbird perception radius is 2200 cm — walk toward it.
3. On lock, Stalk activates, Razorbird closes to `MinEngageRange =
   100` cm and pecks every 2 s.

### 4.3 Pack feedback-loop check (optional, designer-authored)

1. Author 2 separate Blueprint spawner subclasses, each with a
   different `PackTag` (`Paldark.Pal.Pack.Direhound` vs
   `Paldark.Pal.Pack.Direhound.Subpack_A`).
2. Place both in level, separated by > 2000 cm.
3. Aggro one pack. Confirm `Paldark.Pal.DumpPackState` shows the
   second pack's members still patrolling (no broadcast leaked).

### 4.4 Recording artefact

For Q2 closure, designer should record 30-60 s of PIE footage at
`Documents/Devlog/Q2-W20-DirehoundPack-Demo.mp4` showing the spawn →
patrol → aggro → attack → cleanup loop. Sharing the video confirms
the FSM transitions visually + provides a regression baseline for
W22+ refactors.

## 5. Risks

| Risk | Likelihood | Mitigation |
|------|-----------|-----------|
| `Movement->bOrientRotationToMovement` fights `SetActorRotation` (Stalk steering) | Medium | Same as W18-19 — designer must verify `bUseControllerDesiredRotation` on hostile-Pal Blueprint subclasses. |
| Razorbird `MOVE_Flying` ignores navmesh — could fly through geometry | High | Acknowledged. The current `AddMovementInput` is world-space and oblivious to walls. Acceptable for W20-21 demo; W22+ adds a collision sphere sweep. |
| Pack broadcast feedback loop crashes server | Low | Guarded by `Perception->GetCurrentThreat() == NewThreat` check in `BroadcastPackThreat`. Validator asserts the call is present. |
| Dead Pal stays in `PackRegistry` → broadcast wastes time | Low | Weak ptr filter in broadcast + explicit `UnregisterPalFromPack` in `EndPlay`. Stale weak refs are O(1) to skip. |
| Spawner `SpawnHostilePackAt` ground trace misses → Pal spawns underground | Medium | `GroundTraceUp` = 200, `GroundTraceDown` = 2000 — covers reasonable level geometry. Trace failure leaves Pal at requested origin (log warning). |
| `SetPackTag` called from non-authority crashes / silently does nothing | Low | Method body early-returns when `!HasAuthority()` + logs warning. Designer-side authoring path runs from BeginPlay which is server-authoritative. |
| New 9th Pal slot trips the W3-4 component-count validator | High | **Fixed** — validator's `check_pal_slot_classes` now asserts `len == 9` with the comment "W3-4 8 slots + W20-21 PatrolSlot". |

## 6. Pillar coverage

| Pillar | Lesson exercised |
|--------|------------------|
| P09 L1 Activity FSM | Patrol (P15) + Stalk (P25) complete the hostile-Pal ladder. |
| P09 L4 Pack / squad blackboard | `UPaldarkHostilePackSubsystem` as the shared threat channel. |
| P02 L7 WorldSubsystem vs ActorComponent | Pack registry on the world (cross-actor state) vs Patrol on the actor (per-Pal state). |
| P03 L2 Component slot extension | 9th Pal slot (`PatrolSlot`) is purely additive. |
| P12 L1 Data-driven knobs | Spawner exposes designer fields (`SpawnCount`, `PackTag`, `WaypointActors`, `SpawnSpreadRadius`). |
| P12 L2 Constructor-as-data | Species subclass constructor stamps `SpeciesTag` / `TeamTag` / movement mode. |

## 7. Files changed

### Created (17)

- `Source/PaldarkLab/Public/Pal/Components/PaldarkPalPatrolComponent.h`
- `Source/PaldarkLab/Private/Pal/Components/PaldarkPalPatrolComponent.cpp`
- `Source/PaldarkLab/Public/Pal/Activities/PaldarkActivity_Patrol.h`
- `Source/PaldarkLab/Private/Pal/Activities/PaldarkActivity_Patrol.cpp`
- `Source/PaldarkLab/Public/Pal/Activities/PaldarkActivity_Stalk.h`
- `Source/PaldarkLab/Private/Pal/Activities/PaldarkActivity_Stalk.cpp`
- `Source/PaldarkLab/Public/Pal/PaldarkHostilePackSubsystem.h`
- `Source/PaldarkLab/Private/Pal/PaldarkHostilePackSubsystem.cpp`
- `Source/PaldarkLab/Public/Pal/PaldarkPalCharacter_Direhound.h`
- `Source/PaldarkLab/Private/Pal/PaldarkPalCharacter_Direhound.cpp`
- `Source/PaldarkLab/Public/Pal/PaldarkPalCharacter_Razorbird.h`
- `Source/PaldarkLab/Private/Pal/PaldarkPalCharacter_Razorbird.cpp`
- `Source/PaldarkLab/Public/Pal/PaldarkHostilePalSpawner.h`
- `Source/PaldarkLab/Private/Pal/PaldarkHostilePalSpawner.cpp`
- `PaldarkLab/Documents/W20-21-Plan.md`
- `Documents/Devlog/W20-21-Build.md` (this file)

### Modified (8)

- `Source/PaldarkLabCore/Public/PaldarkGameplayTags.h` — 5 tag declarations.
- `Source/PaldarkLabCore/Private/PaldarkGameplayTags.cpp` — 5 tag definitions.
- `Source/PaldarkLab/Public/Pal/PaldarkPalCharacter.h` — `SpeciesTag` / `PackTag` / `PatrolSlot` / `SetPackTag`.
- `Source/PaldarkLab/Private/Pal/PaldarkPalCharacter.cpp` — `PatrolSlot` ctor + `SetPackTag` impl.
- `Source/PaldarkLab/Private/Pal/Components/PaldarkPalCombatComponent.cpp` — pack broadcast hook.
- `Source/PaldarkLab/Private/Pal/PaldarkPalConsoleCommands.cpp` — 3 new console commands.
- `scripts/ci/validate_paldarklab.py` — `check_hostile_pal_shape` +  tag/activity/slot table updates.
- `PaldarkLab/README.md` — `## Hostile-Pal AI subclasses (W20-21)` section + "Sau scaffold này" bullet.

## 8. Closes Q2 milestone

W18-19 + W20-21 together complete the Q2 Pal AI milestone:

- W18-19: Pal companion auto-defend player.
- W20-21: Hostile-Pal AI (Direhound pack + Razorbird aerial).

Q2 milestone artefact lives at `Documents/Devlog/Q2-W18-PalCombat-Demo.mp4`
(W18-19 video) + `Documents/Devlog/Q2-W20-DirehoundPack-Demo.mp4`
(W20-21 video — designer-side recording deferred).

Next: **W22-23 — Squad system** (multi-Pal coordination + alpha
hierarchy). Layers on top of W20-21's pack subsystem with role
assignment and formation movement.
