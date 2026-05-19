# PaldarkLab — Week 1-3-4-5-6-7-8-9-10-11-12 C++ Skeleton

> Sandbox project cho P01 capstone (xem [`Documents/UE5_Course/P01_CPP_Build.md`](../Documents/UE5_Course/P01_CPP_Build.md)) — đặt nền cho 52 tuần roadmap PALDARK.
>
> **Trạng thái:** scaffold C++ + data-driven experience system (W1 day 6-7) + action sets / player framework / gameplay tags (W1 day 8-10) + Enhanced Input + tag-keyed Input Config (W1 day 11-14) + Player & Pal pawn skeleton + locomotion follow (W3-4) + Activity FSM (Idle / Follow / Investigate) + ping console cmd (W5-6) + GAS basic (AttributeSet / ASC / GA_Sprint, W7-8) + Damage chain (GE_Damage_Standard + DamageExecutionCalculation + GA_HitscanFire + DummyTarget, W9-10) + **Inventory fragment system (Item Fragment composition + ItemDefinition primary data asset + replicated InventoryComponent + weight cap + 4 console cmds, W11-12)**. UMG widget defer sang W12+ — W11-12 chỉ ship C++ backbone, designer test bằng console. Mở trong UE 5.4 Editor → Generate Project Files → Compile.

## Mục tiêu (P01 capstone + W1 day 6-7 + W1 day 8-10 + W1 day 11-14 + W3-4)

- **3 module:**
  - `PaldarkLabCore` (Runtime, PreDefault loading) — log category + shared types, dependency cho mọi module khác.
  - `PaldarkLab` (Runtime, Default loading) — game module chính, console command, sample actor + subsystem, **vendor-neutral experience system** (W1 day 6-7).
  - `PaldarkLabEditor` (Editor only) — placeholder cho editor utility / asset validator sau này.
- **3 target:**
  - `PaldarkLab.Target.cs` (Game).
  - `PaldarkLabEditor.Target.cs` (Editor).
  - `PaldarkLabServer.Target.cs` (Server) — dedicated server, dùng cho P07 sau.
- **5 log category** (declared in `PaldarkLabCore/Public/PaldarkLogCategories.h`):
  - `LogPaldark` — general.
  - `LogPaldarkPal` — Pal AI / behaviour.
  - `LogPaldarkInventory` — inventory & equipment.
  - `LogPaldarkNet` — replication & RPC.
  - `LogPaldarkGAS` — GAS / abilities / effects.
- **Console commands** (registered in `PaldarkLab.cpp`):
  - `Paldark.HelloWorld [optional message]` — generic hello-world.
  - `Paldark.Experience.Current` — print current `FPrimaryAssetId` of loaded experience.
  - `Paldark.Experience.Hello` — log `HelloWorldMessage` of current experience (W1 day 6-7).
  - `Paldark.Experience.ListExtensions` — list resolved class overrides, granted tags, and action sets on the active server game mode (W1 day 8-10).
  - `Paldark.Input.ListBindings` — for each `APaldarkPlayerController` in the active world, log the cached `PawnData`, mapping contexts + priorities, and `InputConfig` rows (Native + Ability) (W1 day 11-14).
  - `Paldark.Pal.SpawnTestCompanion [index|class_path]` — server-only debug helper that spawns one Pal companion behind the local player and wires `SetFollowedPawn` so the locomotion component leashes back (W3-4).
  - `Paldark.Pal.CurrentActivity` — log the active activity class + tag for every Pal in the current world (W5-6).
  - `Paldark.Pal.SetActivity <Idle|Follow|Investigate>` — force-switch every Pal's FSM by short name or full `Paldark.Pal.Activity.*` tag (W5-6).
  - `Paldark.Pal.Ping [X Y Z]` — file an Investigate request on every Pal at `(X, Y, Z)`; defaults to a point 6 m in front of the local player when args are omitted (W5-6).
  - `Paldark.Gas.DumpAttributes` — for every `IAbilitySystemInterface` actor in the world, log the live `Health / MaxHealth / Stamina / MaxStamina / MoveSpeed / Armor` values pulled directly from the AttributeSet (W7-8 + W9-10).
  - `Paldark.Gas.Damage [Amount]` — server-side placeholder that subtracts `Amount` (default 10) from `Health` on every GAS pawn. Replaced by the real damage chain in W9-10 but kept as a quick-poke helper.
  - `Paldark.Combat.SpawnDummy [Distance]` — server-only debug helper that spawns one `APaldarkDummyTarget` at `Distance` cm in front of the local player (default 500 cm), facing back at the player so head-bone hits are reachable from the camera (W9-10).
  - `Paldark.Combat.Fire` — activates the Fire ability on the local player's ASC via `TryActivateAbilityByActivationTag(Paldark.Ability.Fire)`. Skips the keyboard binding so the damage formula can be iterated without rebinding input (W9-10).
- **1 sample actor:** `APaldarkLabSampleActor` (mesh + offset + rotation speed, replicated).
- **1 manager subsystem:** `UPaldarkLabWorldSubsystem` (UWorldSubsystem, ticks, registers spawned sample actors).
- **Vendor-neutral experience system** (W1 day 6-7):
  - `UPaldarkAssetManager` (UAssetManager) — registers `PaldarkExperience`, `PaldarkPawnData`, and `PaldarkExperienceActionSet` primary asset types.
  - `UPaldarkExperienceDefinition` (UPrimaryDataAsset) — DisplayName + DefaultPawnData + HelloWorldMessage + **ActionSets** + **PlayerControllerClass** + **PlayerStateClass** + **IntrinsicTags** (W1 day 8-10).
  - `UPaldarkPawnData` (UPrimaryDataAsset) — pawn class + optional IMC + **HUDClass** (W1 day 8-10).
  - `UPaldarkGameInstance` (UGameInstance) — Init/Shutdown logging, future online-subsystem slot.
  - `APaldarkGameModeBase` (AGameModeBase) — parses `?Experience=PaldarkExperience.<Name>` URL option, async-loads via AssetManager, **applies class overrides + action sets from experience** (W1 day 8-10).
  - `APaldarkGameStateBase` (AGameStateBase) — replicates current experience id via RepNotify, broadcasts `OnExperienceLoaded`.
- **Action set + player framework** (W1 day 8-10):
  - `UPaldarkExperienceActionSet` (UPrimaryDataAsset) — additive bundle of `GrantedTags` + `DebugNote`; experience composes a list of these.
  - `PaldarkGameplayTags` namespace (PaldarkLabCore) — native tags via `UE_DECLARE/UE_DEFINE_GAMEPLAY_TAG`:
    - W1 day 8-10: `Paldark.Experience.Default`, `Paldark.Experience.RaidSandbox`, `Paldark.Pawn.Player`, `Paldark.Pawn.Pal`.
    - W1 day 11-14: `Paldark.InputTag.Move`, `Paldark.InputTag.Look`, `Paldark.InputTag.Jump` (lookup keys for `UPaldarkInputConfig`).
  - `APaldarkPlayerController` (APlayerController) — logs OnPossess / OnUnPossess; **resolves PawnData from active experience, pushes IMCs into `UEnhancedInputLocalPlayerSubsystem`, hands PawnData to the character so `SetupPlayerInputComponent` can bind via tags** (W1 day 11-14).
  - `APaldarkPlayerState` (APlayerState) — replicated `TeamIndex` (RepNotify), server-only `SetTeamIndex`; future GAS ASC owner (W7+).
  - `APaldarkCharacter` (ACharacter) — third-person skeleton with spring arm + camera composition (P03 pillar); logs BeginPlay / PossessedBy / OnRep_Controller. **Implements `SetupPlayerInputComponent` to bind `Move`/`Look`/`Jump` handlers via tag lookup on the active `UPaldarkInputConfig`** (W1 day 11-14).
- **Enhanced Input + tag-keyed Input Config** (W1 day 11-14):
  - `UPaldarkInputConfig` (UPrimaryDataAsset) — `NativeInputActions[]` (Move/Look/Jump etc.) + `AbilityInputActions[]` (reserved for W7+ GAS), each row is a `(FGameplayTag, UInputAction*)` pair. Lookup helpers `FindNativeInputActionForTag` / `FindAbilityInputActionForTag` return the action for a tag.
  - `UPaldarkPawnData` gained `DefaultMappingContexts[]` (each row = soft IMC + priority + `bRegisterWithSettings`) and `InputConfig` (TSoftObjectPtr).
  - `APaldarkPlayerController` owns the Enhanced Input plumbing: resolves PawnData from the active experience, hands it to the character, then iterates `DefaultMappingContexts` and calls `AddMappingContext` on `UEnhancedInputLocalPlayerSubsystem`. Symmetric `ClearMappingContexts` on `OnUnPossess`.
  - `APaldarkCharacter::SetupPlayerInputComponent` casts to `UEnhancedInputComponent`, loads the `UPaldarkInputConfig` from the cached `PawnData`, and binds the Move/Look/Jump handlers using `Paldark.InputTag.*` lookup keys. `Config/DefaultInput.ini` pins `DefaultPlayerInputClass = EnhancedPlayerInput` and `DefaultInputComponentClass = EnhancedInputComponent` so the cast always succeeds.
- **Player & Pal pawn skeleton + follow locomotion** (W3-4):
  - `APaldarkCharacter` gained **12 empty player component slots** (HealthSlot / StaminaSlot / CombatSlot / InventorySlot / EquipmentSlot / PalCompanionSlot / LocomotionExtSlot / ActivitySlot / InteractionSlot / CameraExtSlot / NetworkSlot / DamageSlot). Each slot is an empty `UActorComponent` subclass under `Player/Components/` reserved for future weeks (Health/Stamina/Combat/Inventory/Equipment land in W7+, Activity FSM in W5-6, etc.) so designers can attach future logic via Blueprint subclass without touching C++.
  - `APaldarkPalCharacter` (`ACharacter` subclass) is the Pal companion pawn root. Spring arm + camera mirror the player rig (P03 composition), `SetReplicateMovement(true)`. It owns **8 empty Pal component slots** (HealthSlot / CombatSlot / BondSlot / ActivitySlot / DataSlot / PerceptionSlot / AnimDriverSlot / **LocomotionSlot**). The Bond slot is the future home of the bond/affection logic (W18+), Perception drives the future Custom Sense (W19+), AnimDriver is the future linked anim instance (W22+).
  - `UPaldarkPalLocomotionComponent` is the only Pal slot with real W3-4 behaviour: caches the owning `APaldarkPalCharacter`, exposes `SetFollowedPawn(APawn*)`, and on tick computes the **planar** distance to the followed pawn. When `Distance > TargetFollowDistance + StopDeadbandDistance` (defaults: 500 cm + 80 cm), it calls `AddMovementInput(DirectionToTarget)`. `UCharacterMovementComponent` then handles the navmesh stepping. Configurable knobs: `TargetFollowDistance`, `MaxLeashDistance`, `StopDeadbandDistance`. Full navmesh pathfinding (`UNavigationSystemV1::SimpleMoveToActor` / behaviour tree EQS) is deferred to the AI pillar in W18+.
  - `UPaldarkPawnData` gained a `DefaultPalCompanions[]` array of `FPaldarkPalCompanionSpec` rows (`TSoftClassPtr<APaldarkPalCharacter> PalClass`, `FVector SpawnOffset`, `float FollowDistanceOverride`). One PawnData asset = N companions configurable per experience (e.g. `PD_RaidPlayer` can declare zero companions, `PD_RaidPlayerWithFox` declares one fox at offset `{-300,0,0}`). No hard reference — `LoadSynchronous` only fires when the spawn command resolves it.
  - `Paldark.Pal.SpawnTestCompanion [index|class_path]` console command: takes either an integer index into `DefaultPalCompanions` (default `0`) **or** a soft class path (`/Script/PaldarkLab.PaldarkPalCharacter` or a BP path) for overriding. Spawns one Pal behind the local player using the spec's offset, calls `SetFollowedPawn(LocalPlayer)` on the new Pal so the locomotion component leashes back. Server-only path; PIE is fine because PIE Listen-Server has authority.
- **Activity FSM — Idle / Follow / Investigate + ping** (W5-6):
  - `UPaldarkBaseActivity` (UObject, abstract) is the root activity class. Lifecycle is `CanRun() → EnterActivity() → TickActivity(DeltaSeconds) → ExitActivity()`, plus `ShouldContinue()` for hysteresis. Each activity carries `ActivityTag` (FGameplayTag identity) + `Priority` (int32, higher wins). All lifecycle methods are `BlueprintNativeEvent` so designers can subclass in Blueprint without touching C++. Helpers expose `GetPalOwner()`, `GetFollowedPawn()`, `GetLocomotionSlot()`, `GetPlanarDistanceToFollowedPawn()` so subclasses don't need to know the component layout.
  - 3 concrete activities ship in C++:
    - `UPaldarkActivity_Idle` (priority 10) — `CanRun` returns `true` (always-available fallback). On Enter calls `SetFollowEnabled(false)` so the leash pauses; on Exit re-enables it.
    - `UPaldarkActivity_Follow` (priority 20) — `CanRun` requires a valid followed pawn + planar distance `> EnterDistance` (default 700 cm). `ShouldContinue` uses a smaller `ExitDistance` (default 450 cm) so Idle↔Follow flips around the locomotion threshold (500 cm) do **not** chatter. Tick is a no-op; the locomotion component drives the real `AddMovementInput` once `bFollowEnabled` is back on.
    - `UPaldarkActivity_Investigate` (priority 30, highest) — `CanRun` is gated by `UPaldarkPalActivityComponent::HasActiveInvestigateRequest()`, set via `RequestInvestigate(Location)` (the Pal Ping console cmd files this). On Enter pauses the leash; on Tick steers toward the request location with `AddMovementInput`, clears the request once inside `ArrivalRadius` (default 150 cm), and hard-times-out after `MaxInvestigateTime` (default 8 s) so a stuck Pal returns to the leader. On Exit re-enables the leash.
  - `UPaldarkPalActivityComponent` is the FSM driver. On `BeginPlay` (authority only), it instantiates every class in `CandidateActivities` (default: Idle / Follow / Investigate) as a `UObject` owned by the component and calls `InitActivity(this)` so each activity caches its owner. Every `SelectionInterval` seconds (default 0.25 s) it re-evaluates: if the current activity's `ShouldContinue()` is still true, any **higher-priority** activity that returns `CanRun()` can still preempt (this is how Investigate interrupts Follow); otherwise it scans all candidates and picks the highest-priority `CanRun()`. On switch, the component calls `ExitActivity()` on the outgoing then `EnterActivity()` on the incoming. The current activity's `TickActivity(DeltaSeconds)` fires every component tick.
  - `UPaldarkPalLocomotionComponent` gained the FSM gate: `SetFollowEnabled(bool)` / `IsFollowEnabled()` / `GetPlanarDistanceToFollowedPawn()`. The `TickComponent` early-returns when `bFollowEnabled == false`, so Idle / Investigate can pause the leash **without losing the `FollowedPawn` cache** — flipping the flag back to true resumes the leash seamlessly. This decouples "who decides whether to move" (the activity FSM) from "how to move" (the locomotion component).
  - 3 new gameplay tags under `PaldarkLabCore/PaldarkGameplayTags`: `Paldark.Pal.Activity.Idle` / `.Follow` / `.Investigate`. The activity component matches a candidate activity by tag so the console-driven force-switch + future data-driven activity sets resolve to one of these tags before forwarding.
  - 3 new console commands (all authority-only, registered in `PaldarkLab.cpp` startup): `Paldark.Pal.CurrentActivity`, `Paldark.Pal.SetActivity <Idle|Follow|Investigate>`, `Paldark.Pal.Ping [X Y Z]`. The Ping command also raises an `AddOnScreenDebugMessage` so testing is visible without the Output Log open.

## Cấu trúc

```
PaldarkLab/
├── PaldarkLab.uproject            # UE 5.4 project descriptor
├── Config/
│   ├── DefaultEngine.ini          # log verbosity, GameMode/GameInstance/AssetManager wiring
│   └── DefaultGame.ini            # project id + primary asset types
├── Source/
│   ├── PaldarkLab.Target.cs
│   ├── PaldarkLabEditor.Target.cs
│   ├── PaldarkLabServer.Target.cs
│   ├── PaldarkLab/                # main game module
│   │   ├── PaldarkLab.Build.cs
│   │   ├── PaldarkLab.h / .cpp    # IMPLEMENT_PRIMARY_GAME_MODULE + console commands
│   │   ├── Public/
│   │   │   ├── Actors/PaldarkLabSampleActor.h
│   │   │   ├── Subsystems/PaldarkLabWorldSubsystem.h
│   │   │   ├── Framework/         # W1 day 6-7
│   │   │   │   ├── PaldarkAssetManager.h
│   │   │   │   ├── PaldarkGameInstance.h
│   │   │   │   ├── PaldarkGameModeBase.h
│   │   │   │   └── PaldarkGameStateBase.h
│   │   │   ├── Experience/        # W1 day 6-7 + W1 day 8-10
│   │   │   │   ├── PaldarkExperienceDefinition.h
│   │   │   │   ├── PaldarkExperienceActionSet.h    # W1 day 8-10
│   │   │   │   └── PaldarkPawnData.h
│   │   │   ├── Player/            # W1 day 8-10
│   │   │   │   ├── PaldarkPlayerController.h
│   │   │   │   ├── PaldarkPlayerState.h
│   │   │   │   └── PaldarkCharacter.h
│   │   │   ├── Pal/                # W3-4 + W5-6
│   │   │   │   ├── PaldarkPalCharacter.h
│   │   │   │   ├── Components/*.h   # 8 W3-4 component slots (Locomotion + 7 stubs)
│   │   │   │   └── Activities/      # W5-6
│   │   │   │       ├── PaldarkBaseActivity.h           # abstract base (UObject)
│   │   │   │       ├── PaldarkActivity_Idle.h          # priority 10
│   │   │   │       ├── PaldarkActivity_Follow.h        # priority 20, hysteresis
│   │   │   │       └── PaldarkActivity_Investigate.h   # priority 30, pinged
│   │   │   ├── Gas/                # W7-8 + W9-10
│   │   │   │   ├── PaldarkAttributeSet.h               # 5 W7-8 attrs + Armor + IncomingDamage (W9-10)
│   │   │   │   ├── PaldarkAbilitySystemComponent.h     # tag-keyed activate/cancel helpers
│   │   │   │   ├── PaldarkGameplayAbility.h            # base — ActivationInputTag lookup key
│   │   │   │   ├── PaldarkDamageExecutionCalculation.h # W9-10 — Armor capture + SetByCaller formula
│   │   │   │   └── Abilities/
│   │   │   │       ├── PaldarkGameplayAbility_Sprint.h        # W7-8 — apply Sprint cost / move speed GE
│   │   │   │       └── PaldarkGameplayAbility_HitscanFire.h   # W9-10 — line trace + apply damage GE
│   │   │   └── Combat/             # W9-10
│   │   │       └── PaldarkDummyTarget.h                # ASC + AttributeSet + init GE + die-on-zero-health
│   │   └── Private/{Actors,Subsystems,Framework,Experience,Player,Pal,Pal/Components,Pal/Activities,Gas,Gas/Abilities,Combat}/*.cpp
│   ├── PaldarkLabCore/            # shared (loads PreDefault)
│   │   ├── PaldarkLabCore.Build.cs
│   │   ├── Public/PaldarkLabCore.h
│   │   ├── Public/PaldarkLogCategories.h
│   │   ├── Public/PaldarkGameplayTags.h            # W1 day 8-10
│   │   ├── Private/PaldarkLabCore.cpp
│   │   └── Private/PaldarkGameplayTags.cpp         # W1 day 8-10
│   └── PaldarkLabEditor/          # editor-only placeholder
│       ├── PaldarkLabEditor.Build.cs
│       ├── Public/PaldarkLabEditor.h
│       └── Private/PaldarkLabEditor.cpp
└── README.md (file này)
```

## Mở project

1. Yêu cầu: **Unreal Engine 5.4** đã cài qua Epic Games Launcher.
2. Right-click `PaldarkLab.uproject` → "Generate Visual Studio project files".
3. Mở `PaldarkLab.sln` → set startup project = `PaldarkLab` → build `Development Editor | Win64`.
4. Run editor → New empty level → drag `PaldarkLabSampleActor` từ Place Actors panel vào scene.
5. Open Output Log, lọc theo `LogPaldark` — thấy log từ subsystem khi PIE start/stop.
6. Trong console (`~`) chạy: `Paldark.HelloWorld Hi` → log line emit.

## Build target server (P07 chuẩn bị)

```
RunUAT.bat BuildCookRun -project=PaldarkLab.uproject -platform=Win64 ^
   -target=PaldarkLabServer -clientconfig=Development -serverconfig=Development ^
   -build -cook -stage -archive -archivedirectory=out/
```

Server target packages headless build dùng cho dedicated server flow (Week 14+).

## Experience flow (W1 day 6-7)

Vendor-neutral mirror of Lyra's data-driven experience pattern — no Lyra plugin required, so the hello-world runs on a vanilla UE 5.4 install. Reused/replaced at W33 if Lyra adoption goes ahead.

```
URL ?Experience=PaldarkExperience.<Name>     (1) client requests
        │
        ▼
APaldarkGameModeBase::InitGame               (2) parses URL option
        │
        ▼
UAssetManager::LoadPrimaryAsset(AssetId)     (3) async load via primary asset id
        │
        ▼
APaldarkGameModeBase::OnExperienceAssetReady (4) cache UPaldarkExperienceDefinition
        │                                        + resolve pawn class from PawnData
        ▼
APaldarkGameStateBase::SetCurrentExperience  (5) server stores + replicates id
        │
        ├── server-side: OnExperienceLoaded.Broadcast(Experience)
        ▼
OnRep_CurrentExperienceId on clients         (6) client resolves soft pointer
        │                                        and fires same delegate
        ▼
APaldarkGameModeBase::PostLogin              (7) hello-world: log HelloWorldMessage
                                                + on-screen debug
```

Authoring a sample experience (after `Generate Project Files` + compile):

1. In the editor: `Content/Paldark/PawnData/` → right-click → Data Asset → **PaldarkPawnData** → name it `PD_HelloPawn` → set `PawnClass` (e.g. `Engine/DefaultPawn`).
2. `Content/Paldark/Experiences/` → right-click → Data Asset → **PaldarkExperience** → name it `PX_HelloWorld` → set `DefaultPawnData = PD_HelloPawn`, `HelloWorldMessage = "Hello from Paldark Experience."`.
3. Open a level via console: `open Test_Map?Experience=PaldarkExperience.PX_HelloWorld`.
4. In console: `Paldark.Experience.Current` (prints id) → `Paldark.Experience.Hello` (logs HelloWorldMessage).

The Asset Manager already knows where to scan because `Config/DefaultGame.ini` declares the `PaldarkExperience`, `PaldarkPawnData`, and `PaldarkExperienceActionSet` primary asset types under `[/Script/Engine.AssetManagerSettings]`.

## Authoring "Raid_Sandbox" (W1 day 8-10)

The Raid_Sandbox experience is the W1 → W3 sandbox we use to verify the action-set + player-framework wiring. It layers on top of the day 6-7 hello-world. Authoring order in the editor (one-time, after compile):

1. **Pawn data + character.** In `Content/Paldark/PawnData/` create a **PaldarkPawnData** asset named `PD_RaidPlayer`:
   - `PawnClass` → `PaldarkCharacter` (or a Blueprint subclass such as `BP_RaidPlayer` if you want a custom mesh / camera tweaks).
   - `HUDClass` → leave null for now (wired in W11+).
   - `DefaultInputMappingContext` → leave null for now (wired in W1 day 11-14).
2. **Action sets.** In `Content/Paldark/ActionSets/` create one or more **PaldarkExperienceActionSet** assets — examples:
   - `AS_RaidSandbox_Core`  → `GrantedTags = { Paldark.Experience.RaidSandbox }`, `DebugNote = "Raid sandbox core extensions."`.
   - `AS_DebugCommands`     → `GrantedTags = { Paldark.Experience.Default }`, `DebugNote = "Debug console commands."`.
3. **Experience.** In `Content/Paldark/Experiences/` create a **PaldarkExperience** asset named `PX_RaidSandbox`:
   - `DefaultPawnData` → `PD_RaidPlayer`.
   - `HelloWorldMessage` → `"Welcome to the Raid Sandbox."`.
   - `ActionSets` → add `AS_RaidSandbox_Core` and `AS_DebugCommands`.
   - `PlayerControllerClass` → `PaldarkPlayerController` (or a Blueprint subclass `BP_RaidPC`).
   - `PlayerStateClass` → `PaldarkPlayerState` (or a Blueprint subclass `BP_RaidPS`).
   - `IntrinsicTags` → optionally `{ Paldark.Pawn.Player }` (merged with action set tags at boot).
4. **Map.** Save a level under `Content/Paldark/Maps/Raid_Sandbox.umap`. Empty default level is fine for the W1 sanity check.
5. **Launch.** In the editor console (`~`) run:
   - `open Raid_Sandbox?Experience=PaldarkExperience.PX_RaidSandbox` — boots the experience.
   - `Paldark.Experience.Current` — prints id + experience path.
   - `Paldark.Experience.ListExtensions` — prints class overrides, granted tags, and action sets. Run on the server (in PIE this is the same process).
6. **Verify logs.** `LogPaldark` should show:
   - `Experience PaldarkExperience.PX_RaidSandbox loaded. PawnClass=... PlayerControllerClass=... PlayerStateClass=... ActionSets=2 Tags=2`
   - One `[ActionSet] AS_*` line per action set (with the `DebugNote`).
   - `APaldarkPlayerController::OnPossess` + `APaldarkCharacter::PossessedBy` lines when the local player spawns.

> The action set / class override flow uses **synchronous** `LoadSynchronous` inside `OnExperienceAssetReady` for clarity in the W1 scaffold. Switching to async `FStreamableManager::RequestAsyncLoad` is a P14 (AssetMgr) follow-up, scheduled for the dedicated-server pass in W2+.

## Authoring Enhanced Input + Input Config (W1 day 11-14)

Drives `Move`/`Look`/`Jump` through the same data-driven path as the experience system: designers fill in IMCs + tag→action rows on a couple of data assets, C++ never names a specific `UInputAction*`. Authoring order (one-time, after compile):

1. **Input Actions.** `Content/Paldark/Input/Actions/` — right-click → Input → **Input Action**:
   - `IA_Move` → `ValueType = Axis2D (Vector2D)`.
   - `IA_Look` → `ValueType = Axis2D (Vector2D)`.
   - `IA_Jump` → `ValueType = Digital (bool)`.
2. **Input Mapping Context(s).** `Content/Paldark/Input/` — right-click → Input → **Input Mapping Context** → name `IMC_Default`. Add mappings:
   - `IA_Move` → `WASD` (each key with a `Swizzle Input Axis Values` + `Negate` modifier as per UE template), `Gamepad Left Thumbstick 2D-Axis`.
   - `IA_Look` → `Mouse XY 2D-Axis`, `Gamepad Right Thumbstick 2D-Axis`.
   - `IA_Jump` → `Space Bar`, `Gamepad Face Button Bottom`.
   - Higher-priority contexts (vehicle, UI) can be added later as additional IMCs; the engine resolves conflicts by priority (higher wins).
3. **Input Config.** `Content/Paldark/Input/` — right-click → Data Asset → **PaldarkInputConfig** → name `InputConfig_Default`:
   - `NativeInputActions` → add 3 rows:
     - `InputAction = IA_Move`, `InputTag = Paldark.InputTag.Move`.
     - `InputAction = IA_Look`, `InputTag = Paldark.InputTag.Look`.
     - `InputAction = IA_Jump`, `InputTag = Paldark.InputTag.Jump`.
   - `AbilityInputActions` → leave empty (filled in W7+ when GAS lands).
4. **PawnData wiring.** Reopen `PD_RaidPlayer` (or whatever PawnData the experience uses):
   - `DefaultMappingContexts` → add one row: `InputMapping = IMC_Default`, `Priority = 0`, `bRegisterWithSettings = false`.
   - `InputConfig` → set to `InputConfig_Default`.
5. **Launch + verify.** `open Raid_Sandbox?Experience=PaldarkExperience.PX_RaidSandbox`. Then:
   - `Paldark.Input.ListBindings` (console) → prints the active PawnData, IMC list with priorities, InputConfig path, and one line per tag→action row. Useful to confirm the soft references actually loaded.
   - `LogPaldark` should show `APaldarkPlayerController::ApplyMappingContexts — ... applied 1 IMC(s).` and `APaldarkCharacter::BindNativeInputActions — ... bound 3 native action(s) from InputConfig_Default.`.
   - Press WASD / move the mouse / press Space → the character should move/look/jump.

Stacking contexts (vehicle / UI / pal command wheel) later is purely additive: create extra IMCs, append more rows to `DefaultMappingContexts` with higher `Priority`, and they take precedence on key conflicts. No C++ change needed.

## Activity FSM authoring + testing flow (W5-6)

The FSM is data-driven through `UPaldarkPalActivityComponent::CandidateActivities` — the C++ default is `{ UPaldarkActivity_Idle, UPaldarkActivity_Follow, UPaldarkActivity_Investigate }`, but a Blueprint subclass of `APaldarkPalCharacter` can replace it with a designer-authored list. End-to-end test loop (after Generate Project Files + compile):

1. **Boot Raid_Sandbox.** `open Raid_Sandbox?Experience=PaldarkExperience.PX_RaidSandbox`. The player character spawns with the configured PawnData; `Paldark.Pal.SpawnTestCompanion 0` spawns one Pal behind the player and wires `SetFollowedPawn(LocalPlayer)`.
2. **Verify the FSM started.** Run `Paldark.Pal.CurrentActivity` in the console. Output should include one line like `pal=BP_Fox_Companion_C_0 activity=UPaldarkActivity_Idle tag=Paldark.Pal.Activity.Idle candidates=3` — confirms `BeginPlay` instantiated all 3 candidates and kicked off Idle as the initial state.
3. **Force a state.** `Paldark.Pal.SetActivity Idle` / `Follow` / `Investigate` (or the full tag `Paldark.Pal.Activity.Idle`). The component calls `SetCurrentActivityByTag` and `LogPaldarkPal` emits one `SwitchToActivity — pal=... from=... to=...` line. Designer-side: the Pal stops/starts moving in sync because Idle/Follow flip `bFollowEnabled` on the locomotion component.
4. **Trigger an investigate ping.** Walk the player forward, then `Paldark.Pal.Ping` (no args = 6 m in front of the player) or `Paldark.Pal.Ping 1000 0 100` for a specific world location. The component calls `RequestInvestigate(...)` and Investigate (priority 30) preempts Follow (priority 20). The Pal steers toward the ping, clears its request when within `ArrivalRadius` (or after `MaxInvestigateTime`), and the FSM falls back to whichever of Idle/Follow now satisfies `CanRun`. A magenta on-screen debug line confirms the ping fired.
5. **Hysteresis check.** Stand exactly at the follow threshold so the planar distance hovers around 500 cm. `Paldark.Pal.CurrentActivity` should stay stable on one state (Idle or Follow) instead of chattering between the two — Follow uses a smaller `ExitDistance` than `EnterDistance` precisely for this case.

### Authoring a custom activity (Blueprint or C++)

- **Blueprint route.** Right-click in Content Browser → *Blueprint Class* → search for `PaldarkBaseActivity` → name e.g. `BP_Activity_Patrol`. Set `ActivityTag` and `Priority` on the class defaults. Override `CanRun` / `EnterActivity` / `TickActivity` / `ExitActivity` (and optionally `ShouldContinue` for hysteresis) in the event graph. Then either edit a Blueprint subclass of `APaldarkPalCharacter` and add `BP_Activity_Patrol` to the activity component's `CandidateActivities`, or feed it through a future data asset.
- **C++ route.** Subclass `UPaldarkBaseActivity`. Override `CanRun_Implementation` / `EnterActivity_Implementation` / `TickActivity_Implementation` / `ExitActivity_Implementation`. Set `ActivityTag` + `Priority` in the constructor. Add a new line to `UPaldarkPalActivityComponent::CandidateActivities` (or the future data asset).
- **Priority guidance.** Reserve `0–19` for ambient / idle states, `20–49` for routine behaviours (Follow, Patrol), `50–79` for player-driven commands (Investigate, GoTo), `80+` for combat / threat reactions (W7+). The FSM scans `CandidateActivities` linearly per `SelectionInterval` — keep the list bounded (< 16 entries) until the W18+ pass swaps in a smarter scheduler.

### Anti-patterns to avoid

- Calling `AddMovementInput` from `EnterActivity` instead of `TickActivity` — movement input must be applied every frame. Use Enter only for state setup (flag flips, animation montage cues).
- Modifying the activity component from inside an activity tick — prefer mutating component-owned request state (e.g. `ClearInvestigateRequest()`) so the FSM picks up the change on the next selection pass.
- Adding the same activity class twice to `CandidateActivities` — the FSM will instantiate duplicates and the priority tiebreaker is order-dependent (later wins).
- Replicating activity state directly — the FSM runs on authority only. Replication should happen via the *outcome* (movement, anim notify, GAS effect) on already-replicated subsystems.

## Damage chain authoring + test loop (W9-10)

The damage chain replaces the W7-8 `Paldark.Gas.Damage` placeholder with a real GAS pipeline:

```
GA_HitscanFire (server)                 (1) ActivateAbility runs line trace from camera
       │                                    bone match → headshot flag
       ▼
SourceASC->MakeOutgoingSpec(GE_Damage_Standard)
       │  + SetSetByCallerMagnitude(Paldark.SetByCaller.BaseDamage, 25)
       │  + SetSetByCallerMagnitude(Paldark.SetByCaller.HeadshotMultiplier, 1.0 or 2.0)
       │  + AddDynamicAssetTag(Paldark.Hit.Headshot) when applicable
       ▼
SourceASC->ApplyGameplayEffectSpecToTarget(Spec, TargetASC)
       │
       ▼
UPaldarkDamageExecutionCalculation        (2) capture Target.Armor
       │                                      read SetByCaller magnitudes
       │                                      final = base × headshot × (100/(100+armor))
       │                                      AddOutputModifier(IncomingDamage, final)
       ▼
UPaldarkAttributeSet::PostGameplayEffectExecute
       │                                  (3) drain IncomingDamage → Health
       │                                      clamp [0, MaxHealth]
       │                                      Health == 0 → OnHealthZeroed.Broadcast(InstigatorASC)
       ▼
APaldarkDummyTarget::HandleHealthZeroed   (4) add Paldark.State.IsDead
                                              disable collision + movement
                                              schedule Destroy() after DestroyDelaySeconds
```

The whole pipeline runs on the **authority** (the server / the PIE listen-server). Clients see the result purely through replicated AttributeSet values (`Health` is `ReplicatedUsing = OnRep_Health`) — no RPC plumbing required.

### Authoring order (one-time, after Generate Project Files + compile)

1. **Damage GE.** `Content/Paldark/Gas/Effects/` → right-click → Blueprint Class → search for **GameplayEffect** → name `GE_Damage_Standard`:
   - `Duration Policy` = `Instant`.
   - `Executions` → add one row → `Calculation Class = PaldarkDamageExecutionCalculation`.
   - Leave `Modifiers` empty — the execution writes the only modifier (IncomingDamage). Do **not** add a Health modifier here; the AttributeSet drains IncomingDamage → Health in `PostGameplayEffectExecute`.
2. **Init attributes GE for the dummy.** Same folder → Blueprint Class → **GameplayEffect** → name `GE_DummyInitAttributes`:
   - `Duration Policy` = `Instant`.
   - `Modifiers` → 3 rows: `Health (Override, 100)`, `MaxHealth (Override, 100)`, `Armor (Override, 0)` — designers can bump `Armor` per dummy variant to verify the mitigation curve.
3. **Fire ability.** `Content/Paldark/Gas/Abilities/` → Blueprint Class → search for **PaldarkGameplayAbility_HitscanFire** → name `BP_GA_HitscanFire`:
   - `DamageEffectClass` → `GE_Damage_Standard`.
   - `FireRange` → 10000 (10 m default).
   - `BaseDamage` → 25.
   - `HeadshotMultiplier` → 2.0.
   - `HeadBoneName` → `head`.
4. **Dummy target Blueprint.** `Content/Paldark/Combat/` → Blueprint Class → search for **PaldarkDummyTarget** → name `BP_DummyTarget_Default`:
   - `InitAttributesEffect` → `GE_DummyInitAttributes`.
   - `DestroyDelaySeconds` → 1.5 (leave default for visible kill confirm).
   - On the inherited `Mesh` component, set a skeletal mesh that has a `head` bone (the UE5 mannequin / Manny / Quinn ships with it).
5. **Grant Fire ability via PawnData.** Reopen `PD_RaidPlayer`:
   - `GrantedAbilities` → append `BP_GA_HitscanFire` (alongside `BP_GA_Sprint`).
   - `StartupEffects` → leave as-is (the dummy uses its own init GE, the player keeps its W7-8 startup).
6. **Bind Fire input.** Reopen `InputConfig_Default`:
   - `AbilityInputActions` → append row `(Paldark.InputTag.Fire, IA_Fire)` — create `IA_Fire` (Digital bool) under `Content/Paldark/Input/Actions/` if it doesn't exist yet.
   - Reopen `IMC_Default` and bind `IA_Fire` to `Left Mouse Button` (and `Gamepad Right Trigger` for controllers).

### Test loop in PIE

1. `open Raid_Sandbox?Experience=PaldarkExperience.PX_RaidSandbox` — boots the sandbox with the player + Fire ability granted.
2. `Paldark.Combat.SpawnDummy` (or `Paldark.Combat.SpawnDummy 800` to push it further out) — spawns one `BP_DummyTarget_Default` 5 m in front of the player, facing back at them.
3. `Paldark.Gas.DumpAttributes` — should list two GAS actors: the player (Health=100, Stamina=100, MoveSpeed=600, Armor=0) and the dummy (Health=100, Armor=0).
4. Aim at the dummy's torso → **Left Click** (or `Paldark.Combat.Fire` from the console). The on-screen damage trace draws a red line on hit, `LogPaldarkGAS` shows:
   - `UPaldarkGameplayAbility_HitscanFire — hit=BP_DummyTarget_Default_C_0 bone=spine_01 headshot=0 distance=502.3 target_asc=...`
   - `UPaldarkDamageExecutionCalculation — base=25.00 headshot=1.00 armor=0.00 mitigation=1.000 final=25.00`
   - `UPaldarkAttributeSet::PostGameplayEffectExecute — IncomingDamage=25.00 -> Health 100.00 → 75.00`
5. Rerun `Paldark.Gas.DumpAttributes` between shots to watch Health drain. Aim at the dummy's **head** to confirm the headshot path: `headshot=1` in the trace log + `final=50.00` (25 × 2.0 × 1.0).
6. On the killing shot (4th body shot or 2nd headshot), `LogPaldarkGAS` should show `UPaldarkAttributeSet — Health hit 0, broadcasting OnHealthZeroed.` followed by `APaldarkDummyTarget::HandleHealthZeroed — instigator=...`. After ~1.5 s the dummy `Destroy()`s. `Paldark.Gas.DumpAttributes` after the kill should only list the player again.

### Authoring a new damage type

The `SetByCaller` magnitude pattern keeps damage tunable without recompiling C++. To add e.g. a Pal claw attack:

1. Subclass `UPaldarkGameplayAbility` (or `UPaldarkGameplayAbility_HitscanFire` if it's also a ranged trace) for the new ability. Override `ActivateAbility` to compute the hit, then call:
   ```cpp
   FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(MyDamageGE, GetAbilityLevel(), Ctx);
   Spec.Data->SetSetByCallerMagnitude(Paldark.SetByCaller.BaseDamage, MyAbilityBaseDamage);
   Spec.Data->SetSetByCallerMagnitude(Paldark.SetByCaller.HeadshotMultiplier, MyAbilityCanCrit ? 2.0f : 1.0f);
   SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
   ```
2. Author a new `GE_<Type>` Blueprint subclass of `UGameplayEffect` with `GE_Damage_Standard`'s shape (instant, one `UPaldarkDamageExecutionCalculation` execution, no modifiers).
3. No execution-calc change needed for variants that share the base damage / armor / headshot math. Element-specific resistances (Fire / Lightning / Arcane) are a W21+ extension — port the Aura resistance capture pattern when those Pal types land.

### Anti-patterns to avoid (damage chain)

- Adding a `Health` modifier to the damage GE alongside the execution calc — both will apply and the damage will double (or worse, get clamped twice). Stick with the IncomingDamage meta-attribute path.
- Reading `Health` directly inside the execution calc to "preview" the result — captured magnitudes are evaluated at execute time, not snapshot time, so the AttributeSet's `PostGameplayEffectExecute` will see a stale value. Write `IncomingDamage` and let `PostGameplayEffectExecute` move it onto Health.
- Skipping `CommitAbility` in the Fire ability — without it, no cost / cooldown GEs ever fire and the W17+ ammo / cooldown work can't reuse the same call site.
- Driving damage purely on the client (without `NetExecutionPolicy = ServerOnly`) — clients can fabricate damage. Keep authority on the server until W22+ predicted weapons land.
- Replicating `IncomingDamage` — it's a meta-attribute (transient), not a persistent stat. Replicating it leaks intermediate state and breaks Aura's "GE → ExecutionCalc → AttributeSet" auditability.
- Calling `Destroy()` directly inside `OnHealthZeroed` — the AttributeSet still holds a strong ref while the GE finishes executing, and destroying the actor mid-broadcast can crash. Schedule via `FTimerHandle` (1.5 s default) and let the GE chain unwind first.

## Inventory fragment authoring + test loop (W11-12)

The W11-12 inventory replaces the W3-4 stub component with a proper Item Fragment + ItemDefinition + replicated component triad. The composition pattern is ported from Udemy [09] *UE5 C++ Inventory Systems* and rephrased to match the rest of PALDARK's tag-keyed data model.

```
UPaldarkItemDefinition (UPrimaryDataAsset)               ← one DataAsset per item (DA_Item_Pistol, …)
├─ ItemTag        : FGameplayTag  ("Paldark.Item.Pistol")   stable lookup key for save data / console
├─ DisplayName    : FText                                   localised label for UI
└─ Fragments      : TArray<UPaldarkItemFragment*>           Instanced + EditInlineNew composition
       │
       ├── UPaldarkItemFragment_Stackable  { MaxStackSize }
       ├── UPaldarkItemFragment_Equipable  { EquipSlot, AbilityToGrantOnEquip (soft) }
       └── UPaldarkItemFragment_Weight     { WeightKgPerUnit }

UPaldarkPlayerInventoryComponent  (replicated, on APaldarkCharacter)
├─ MaxWeightKg : float   = 30.0     (replicated)
├─ Entries     : TArray<FPaldarkInventoryEntry>            (replicated, OnRep_Entries)
│      └── { ItemDef: TSoftObjectPtr<UPaldarkItemDefinition>, StackCount: int32 }
├─ AddItem(Def, Count) → int32       (authority; phase 1 top-off existing stacks, phase 2 new entries)
├─ RemoveItemByTag(Tag, Count) → int32 (authority; backwards iterate + erase on StackCount==0)
├─ DropAllItems()                    (authority; clears Entries; world pickup-actor spawn = follow-up PR)
├─ GetCurrentWeightKg() → float      (sums WeightFragment->WeightKgPerUnit * StackCount per row)
├─ IsOverEncumbered() → bool         (curr > MaxWeightKg + KINDA_SMALL_NUMBER)
├─ FindEntryIndexByTag / FindFirstItemDefByTag             (lookup helpers, INDEX_NONE / nullptr on miss)
├─ DumpToLog()                       (per-entry debug, fragments listed via GetDebugDescription)
└─ OnInventoryChanged : multicast    (fires server-side from AddItem/Remove/Drop and client-side from OnRep)
```

Replication path is **plain DOREPLIFETIME** on `Entries + MaxWeightKg` — fast enough for the W11-12 5-item ceiling. FastArraySerializer migration is parked for W21+ when the inventory grows past ~50 entries.

### Authoring the 5 W11-12 items (one-time, after Generate Project Files + compile)

> Folder convention: `Content/Paldark/Items/DA_Item_<Name>.uasset`. The Asset Manager scans `/Game/Paldark/Items` recursively — both the editor scan rule in `DefaultGame.ini` and the runtime registration in `UPaldarkAssetManager` know this path.

For each of the 5 items below, right-click in Content Browser → *Miscellaneous → Data Asset* → pick `PaldarkItemDefinition` → name e.g. `DA_Item_Pistol`. Set `ItemTag` + `DisplayName` on the asset, then click `+` on `Fragments` and pick the relevant fragment subclasses (the array is `EditInlineNew + DefaultToInstanced`, so the fragment objects live inside the DataAsset — no extra asset files).

| Item                | `ItemTag`                       | Fragments composed                                                                                                         |
|---------------------|---------------------------------|----------------------------------------------------------------------------------------------------------------------------|
| `DA_Item_Pistol`        | `Paldark.Item.Pistol`        | `Equipable(EquipSlot=Paldark.Equip.Slot.Primary)`, `Weight(1.2)`                                                            |
| `DA_Item_Ammo_9mm`      | `Paldark.Item.Ammo_9mm`      | `Stackable(MaxStackSize=60)`, `Weight(0.012)`                                                                               |
| `DA_Item_Bandage`       | `Paldark.Item.Bandage`       | `Stackable(MaxStackSize=10)`, `Weight(0.05)`, `Equipable(EquipSlot=Paldark.Equip.Slot.Consumable)`                          |
| `DA_Item_Pal_Sphere`    | `Paldark.Item.Pal_Sphere`    | `Stackable(MaxStackSize=20)`, `Weight(0.3)`, `Equipable(EquipSlot=Paldark.Equip.Slot.Throwable)`                            |
| `DA_Item_Energy_Drink`  | `Paldark.Item.Energy_Drink`  | `Stackable(MaxStackSize=12)`, `Weight(0.4)`, `Equipable(EquipSlot=Paldark.Equip.Slot.Consumable)`                           |

Items without `Stackable` resolve to `MaxStackSize=1` (one entry per copy — `DA_Item_Pistol` is a good example). Items without `Weight` contribute `0 kg` (the Pistol's 1.2 kg / Ammo's 0.012 kg ensure the cap actually matters once a player loots a few mags).

### Attaching the component to the player

Open `BP_PaldarkCharacter` (or whichever Blueprint subclass `PD_RaidPlayer` references):

1. *Add Component* → search for **PaldarkPlayerInventoryComponent** → drop it on the actor.
2. Set `MaxWeightKg = 30` (default). Increase per Pawn for "pack mule" loadouts later.
3. `Component Replicates` is already on (constructor calls `SetIsReplicatedByDefault(true)`); the actor itself must replicate (Player Character does already).

### Test loop (after compile + the 5 DataAssets exist)

1. `open Raid_Sandbox?Experience=PaldarkExperience.PX_RaidSandbox` — boots the sandbox.
2. `Paldark.Inventory.List` → expect `entries=0 weight=0.00/30.00 kg over=no` on a fresh spawn.
3. `Paldark.Inventory.Add Pistol` → log shows `requested=1 added=1 item=DA_Item_Pistol`. The short form drops the `Paldark.Item.` prefix automatically; `Paldark.Inventory.Add Paldark.Item.Pistol 1` is equivalent.
4. `Paldark.Inventory.Add Ammo_9mm 200` → fills one stack of 60, opens 3 more entries totalling 200. `Paldark.Inventory.List` should print 4 ammo rows (60 / 60 / 60 / 20) + the Pistol row + a weight readout `weight≈3.6 kg`.
5. **Weight cap.** Lower `MaxWeightKg` to `2` on the component instance (via *Edit Defaults* on the dropped component) and re-run step 4. `Paldark.Inventory.Add Ammo_9mm 500` will still accept the items — encumbrance is an *advisory* state in W11-12 (`IsOverEncumbered()` returns true once over the cap); the W13-14 movement penalty will gate it. Restore `MaxWeightKg = 30` afterwards.
6. `Paldark.Inventory.Remove Ammo_9mm 75` → log shows `requested=75 removed=75`. List should now read 4 ammo rows reduced to 125 total (one row drained to 0 is erased).
7. `Paldark.Inventory.Drop` → clears the array. The pickup-actor spawn is intentionally out of scope (W14+ once the pickup actor lands).
8. **Replication smoke test.** Run a 2-player PIE session (one listen-server + one client). Server console: `Paldark.Inventory.Add Pistol`. Open the client's `Paldark.Inventory.List` immediately after — the entry should appear on the client thanks to the `OnRep_Entries` multicast. Server-only commands log a warning on the client (`local player has no authority`).

### Adding a new fragment (e.g. Consumable healing)

Composition over inheritance is the whole point of fragments. To add a new behaviour axis (e.g. "consumable that grants a Heal GE on use"):

1. Subclass `UPaldarkItemFragment` (header under `Public/Inventory/Fragments/PaldarkItemFragment_Consumable.h`):
   ```cpp
   UCLASS(MinimalAPI, DisplayName="Consumable", meta=(DisplayName="Consumable"))
   class UPaldarkItemFragment_Consumable : public UPaldarkItemFragment {
       GENERATED_BODY()
   public:
       UPROPERTY(EditAnywhere, Category="Consumable")
       TSoftClassPtr<UGameplayEffect> EffectToApplyOnUse;
       UPROPERTY(EditAnywhere, Category="Consumable", meta=(ClampMin="1"))
       int32 ChargesPerStack = 1;
       virtual FString GetDebugDescription() const override;
   };
   ```
2. Implement `GetDebugDescription` to return `"Consumable(GE=…,Charges=…)"`.
3. Designer reopens `DA_Item_Bandage`, clicks `+` on `Fragments`, picks `Consumable`, assigns `GE_HealLight`. **No edit needed on `UPaldarkItemDefinition` or `UPaldarkPlayerInventoryComponent`** — the fragment ships isolated.
4. The Use-Item ability (W13-14) reads `FindFragmentByClass<UPaldarkItemFragment_Consumable>()` and applies the GE if present.

### Anti-patterns to avoid (inventory)

- **Storing item state on the player character** (e.g. `int32 PistolAmmo; int32 BandageCount;`) — defeats the whole composition pattern and forces a recompile every time a new item ships. Keep all per-copy state inside `FPaldarkInventoryEntry` (or future per-instance fragment subobjects).
- **Hard pointers (`UPaldarkItemDefinition*` member)** on the inventory entry — pulls every item asset into memory the moment a player spawns. The struct uses `TSoftObjectPtr` and the component calls `LoadSynchronous` lazily inside `LoadItemDefSync`. Async + bundle preloading is the W21+ asset manager polish task.
- **Calling `AddItem` from the client** — the function is authority-gated and logs a warning. Route through a server RPC (W13-14 will add an `ItemPickupActor` that does this) instead of trying to mutate `Entries` client-side.
- **Modifying `Entries` directly inside `OnRep_Entries`** — the rep notify must be read-only. Mutating it triggers another rep cycle and clobbers the server's authoritative snapshot. Fire the multicast (`OnInventoryChanged.Broadcast(this)`) and let the UI react.
- **Stacking non-stackable items** — items without the `Stackable` fragment fall through to `MaxStack=1` and *always* open a new entry per copy. Don't paper over this by assigning `MaxStackSize=1` to a `Stackable` fragment — leave the fragment off entirely so the intent is visible in the editor (`Pistol` is the canonical "no Stackable fragment" example).
- **Mutating the fragment object at runtime** — fragments are CDOs shared across all stacks of an item (Instanced means the ItemDefinition owns its copy, but every player reads the same definition). Per-instance state lives on `FPaldarkInventoryEntry` (and future per-instance fragment data, e.g. ammo loaded into a magazine).

## Dedicated server + 4-player test loop (W14-15)

W14-15 ships a thin C++ net layer on top of W1-12 so the same `Raid_Sandbox`
experience can be played by 4 clients on either a listen server or a dedicated
server, with no auth + no matchmaking (those are Q3/Q4). Everything is C++
authored — there is **no Lobby UMG widget** in this PR; the entry points are
all console commands so a designer can drive the test from PIE.

### Net subsystem (UGameInstanceSubsystem)

The subsystem lives at `Source/PaldarkLab/{Public,Private}/Net/PaldarkNetSubsystem.{h,cpp}`
and survives `ServerTravel` / `ClientTravel` (this is why it's a Game Instance
subsystem, not a World subsystem — the World object is destroyed when the map
changes). It exposes 5 C++ entry points and 4 console commands:

| Console command                  | C++ method                          | What it does                                                                                                                |
|----------------------------------|-------------------------------------|-----------------------------------------------------------------------------------------------------------------------------|
| `Paldark.Net.Host [Map] [Exp]`   | `HostListenServer(Map, ExpId)`      | Calls `UWorld::ServerTravel("Map?listen?Experience=…", bAbsolute=true)`. Local client stays connected (listen server).      |
| `Paldark.Net.Join <Address>`     | `JoinServer(Address)`               | Calls `APlayerController::ClientTravel(Address, TRAVEL_Absolute)`. Address is `IP:Port`, e.g. `127.0.0.1:7777`.              |
| `Paldark.Net.Disconnect`         | `DisconnectFromServer()`            | Calls `ClientTravel("?closed", TRAVEL_Absolute)`. Server sees the client leave via `GameModeBase::Logout`.                    |
| `Paldark.Net.Status`             | `GetNetSnapshot()` + dump to log    | Logs current topology (Role / Mode / Peer count / Server address / Map name) to `LogPaldarkNet`.                              |

The subsystem also caches `FGameplayTagContainer CachedTopologyTags` (always
exactly 1 `Paldark.Net.Role.*` + 1 `Paldark.Net.Mode.*` tag), refreshed on
`PostWorldInitialization` so downstream HUD / activity / spawn code can ask
"are we on a dedicated server?" via a tag query instead of scattering
`GetNetMode()` switches across the codebase.

### Pal replication tuning (4 Pals × 4 clients = 16 replicators)

`APaldarkPalCharacter` is now bandwidth-budgeted for the 4-player target:

```cpp
NetUpdateFrequency     = 30.0f;   // 30 Hz position updates (matches server tick)
MinNetUpdateFrequency  = 5.0f;    // far-from-player floor
NetCullDistanceSquared = 2.5e9f;  // cm^2 ≈ 500 m cull distance
bReplicates            = true;
bReplicateMovement     = true;
```

The locomotion component (`UPaldarkPalLocomotionComponent`) replicates the
follow target + enable flag so non-host clients see the same Pal AI behaviour
as the host:

```cpp
UPROPERTY(Replicated) TObjectPtr<APawn> FollowedPawn;
UPROPERTY(Replicated) bool bFollowEnabled = false;

void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>&) const override;
// DOREPLIFETIME(UPaldarkPalLocomotionComponent, FollowedPawn);
// DOREPLIFETIME(UPaldarkPalLocomotionComponent, bFollowEnabled);
```

The ASC on the Pal uses `EGameplayEffectReplicationMode::Minimal` (set in W7-8
`APaldarkPalCharacter::PostInitializeComponents`), which means GE prediction
runs server-only — for 4 Pals this is the right trade-off (no GE prediction
artefacts, lower bandwidth).

### GameMode net hooks (PreLogin / PostLogin / Logout)

`APaldarkGameModeBase` now overrides the three connection-lifecycle hooks:

- `PreLogin` — rejects the (N+1)th client with `"Server full (4/4). Please try again later."` if `ConnectedPlayerCount >= MaxPlayersPerSession` (default = 4, editable in BP_GameMode CDO).
- `PostLogin` — increments `ConnectedPlayerCount`, logs `[PostLogin] PC_0 joined — 1/4 player(s) connected.` to `LogPaldarkNet`.
- `Logout` — decrements `ConnectedPlayerCount`, logs `[Logout] PC_0 left — 0/4 player(s) remaining.`.

`MaxPlayersPerSession` is a `UPROPERTY(EditDefaultsOnly)` so a designer can
lower the cap (e.g. 2-player co-op test) without recompiling.

### Authoring + test order

Designer side (sau khi compile xanh):

1. Open `Raid_Sandbox` and set the experience to `PaldarkExperience.PX_RaidSandbox`.
2. **1-player listen smoke test**:
   - PIE in **Standalone** with 1 client.
   - In the editor console: `Paldark.Net.Status` → expect `Role=Host Mode=Standalone Peers=0`.
3. **2-player listen test** (host + 1 client, both in PIE):
   - PIE in **Play As Listen Server** with 2 clients.
   - On **Client 1** (the host): no command needed — the host listen path is automatic.
   - On **Client 2**: `Paldark.Net.Join 127.0.0.1:7777` (or whatever IP the host log line shows).
   - On host: `Paldark.Net.Status` → expect `Role=Host Mode=ListenServer Peers=1`.
   - Spawn a Pal via the existing W3-4 console command, confirm Client 2 sees the Pal at the same world position as Client 1 (replication smoke test).
4. **4-player listen test** (host + 3 clients):
   - PIE in **Play As Listen Server** with 4 clients.
   - On host: `Paldark.Net.Status` → expect `Role=Host Mode=ListenServer Peers=3`.
   - Spawn 4 Pals, one following each player. Confirm `FollowedPawn` on each Pal replicates correctly (Client 2 sees Client 3's Pal following Client 3, not Client 1).
5. **4-player dedicated test**:
   - Use `Run > Configurations > Editor > Dedicated Server` (or `PaldarkLabServer.Target.cs` from CLI) to launch the server in headless mode.
   - 4 clients run from PIE / standalone instances. Each: `Paldark.Net.Join <server-ip>:7777`.
   - On any client: `Paldark.Net.Status` → expect `Role=Client Mode=DedicatedServer Peers=0` (clients can't see the peer list — only the server can).
   - Record 30 s video showing 4 players moving + 4 Pals following correctly. This is the Q2 milestone artefact.

### Config wiring

- `Config/DefaultEngine.ini` block `[OnlineSubsystem]` sets `DefaultPlatformService=NULL` — LAN-only for W14-15 (no Steam / EOS / mod.io yet, those are W42-43 + W47-48 backend weeks).
- `Config/DefaultEngine.ini` block `[/Script/Engine.GameSession]` sets `MaxPlayers=4` — base-class hard cap that fires before our `PreLogin` override (defence in depth).
- `Config/DefaultEngine.ini` block `[/Script/OnlineSubsystemUtils.IpNetDriver]` sets `NetServerMaxTickRate=30`, `MaxClientRate=15000`, `MaxInternetClientRate=15000` — matches the 30 Hz `NetUpdateFrequency` on the Pal so the server is not the bottleneck.

### Out of scope (deferred)

- **Auth** — Cognito / Steam / EOS sign-in is Q4 (W42-43). W14-15 is LAN-only (`?listen` + IP:Port).
- **Matchmaking / session find** — Q3 (W26-28, mod.io browser pattern). W14-15 is direct `Paldark.Net.Join <ip>:<port>`.
- **Lobby UMG widget** — Q3 (W23-25, Lyra CommonUI Activatable Widget pattern). W14-15 ships C++ entry points only.
- **Server-side rewind / lag compensation** — ✅ W16-17 (this PR — see `## Lag compensation server-side rewind (W16-17)` below).
- **Dedicated server hosting on EC2 / GameLift Fleet** — W47-48 (AWS deploy week). W14-15 dedicated path runs the local `PaldarkLabServer` target on the designer's machine.

### Anti-patterns to avoid (net)

- **Storing `UPaldarkNetSubsystem` as a `UWorldSubsystem`** — the World object is destroyed when `ServerTravel` swaps maps, so the subsystem (and its console commands + cached topology) would be wiped between Hub and Raid. Game Instance subsystem is the only correct lifetime tier for net entry points.
- **Reading `GetNetMode()` in every gameplay system** — bakes the dedicated-vs-listen question into 40 places and creates a recompile penalty when designers want to A/B test topology. `CachedTopologyTags` on the subsystem gives the same answer via tag query and is mode-agnostic.
- **Replicating `TWeakObjectPtr<APawn>` directly via `UPROPERTY(Replicated)`** — weak pointers don't survive the network serialisation path; the replicated field must be a strong `TObjectPtr<APawn>`. The W3-4 component had it as `Transient TWeakObjectPtr` because it was server-only; W14-15 promotes it to a replicated `TObjectPtr` so clients see the same Pal behaviour as the host.
- **Setting `NetUpdateFrequency = 100.0f` on `APaldarkPalCharacter`** (UE default for `ACharacter`) — 4 Pals × 4 clients × 100 Hz = 1600 actor-replication ticks/s just for the Pal subsystem. 30 Hz is plenty for the W14-15 "4 players see each other" target and leaves bandwidth headroom for player characters + future projectile actors.
- **Calling `ServerTravel` from the client** — `ServerTravel` is server-authoritative; the client must use `ClientTravel`. The subsystem methods enforce this by checking `World->GetNetMode()` before issuing the travel call.
- **Forgetting `GetLifetimeReplicatedProps`** on a component that declares `UPROPERTY(Replicated)` — UE silently drops the replication and the client value stays at the constructor default. Always pair the macro with the override.
- **Hard-coding the server IP** — `Paldark.Net.Join` reads the address from the console arg, so designers can test localhost (`127.0.0.1:7777`), LAN (`192.168.x.y:7777`), or VPN'd Tailscale addresses without recompiling.

### W39 — L-20 closure (BlueprintAssignable login/logout delegates)

Outcome (Q3 bug-bash carry, was Q2 L-20 → carried into Q3): designers can wire UMG lobby widgets to player join/leave events **without** subclassing `APaldarkGameModeBase`. The subsystem exposes `OnPlayerLogin` + `OnPlayerLogout` as `BlueprintAssignable` `DYNAMIC_MULTICAST_DELEGATE_OneParam` fields that any Blueprint actor / widget can bind to with the standard `Bind Event to OnPlayerLogin` node.

Why this matters: Q4 hub town (W40-41) ships a brief room with a player list widget + marketplace player presence indicators. Both want a per-join callback. The Q1 / Q2 way of doing this would be subclassing the GameMode and overriding `PostLogin` — but extraction (`APaldarkGameMode_Extraction`, W24-25) + future hub-town subclass already override `PostLogin` and reach for the match subsystem, so a brief-room widget couldn't slot a third hook without creating a third subclass branch. Routing through a subsystem delegate sidesteps the diamond entirely.

### Authoring contract

Two delegate signatures, defined in <ref_file file="/home/ubuntu/repos/Soliz-Devin-Palworld/PaldarkLab/Source/PaldarkLab/Public/Net/PaldarkNetSubsystem.h" />:

```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPaldarkPlayerLoginSignature,  APlayerController*, NewPlayer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPaldarkPlayerLogoutSignature, AController*,       Exiting);
```

Login carries `APlayerController*` (already non-null by the time `PostLogin` fires); logout widens to `AController*` because UE's `Logout` callback can deliver an `AIController` during seamless travel edge cases (rare, but the engine signature is what it is — match it).

UPROPERTY pairs on the subsystem:

```cpp
UPROPERTY(BlueprintAssignable, Category = "Paldark|Net")
FPaldarkPlayerLoginSignature OnPlayerLogin;

UPROPERTY(BlueprintAssignable, Category = "Paldark|Net")
FPaldarkPlayerLogoutSignature OnPlayerLogout;
```

The base GameMode's `PostLogin` + `Logout` resolve the subsystem via `GetGameInstance()->GetSubsystem<UPaldarkNetSubsystem>()` and call `BroadcastPlayerLogin(NewPlayer)` / `BroadcastPlayerLogout(Exiting)`. `Logout` broadcasts **before** `Super::Logout` so listeners that read `Exiting->PlayerState` still see a valid pointer (the engine often clears it during the super call's `RemovePlayer` path).

### Listener authoring (designer / Blueprint)

From any UMG widget Construct event or BeginPlay on a server-side actor:

```
GetGameInstance → GetSubsystem<UPaldarkNetSubsystem> → OnPlayerLogin → Bind Event
                                                    → OnPlayerLogout → Bind Event
```

Listeners only fire on the authority (the subsystem early-outs on `NM_Client` so a misbinding on the client side doesn't spam Verbose log lines). For a client-side analogue (e.g. "the player list widget in my UI needs to update on a remote player joining"), bind to the existing GameState `PlayerArray` REPNOTIFY — that's the W14-15 supported client path.

### Test loop

After this PR + compile:

1. PIE 2-player listen server.
2. `Paldark.Net.Status` → confirm authority.
3. Drop a `BP_PaldarkLobbyDebugWidget` that binds `OnPlayerLogin` → `PrintString`.
4. Second client joins → server-side log:
   - `LogPaldarkNet: BroadcastPlayerLogin: PlayerController_0 — 1 listener(s) bound.`
   - Widget `PrintString` fires with the new PlayerController name.
5. Second client quits → server-side log:
   - `LogPaldarkNet: BroadcastPlayerLogout: PlayerController_0 — 1 listener(s) bound.`
6. Optional: bind from C++ via `OnPlayerLogin.AddDynamic(this, &UMyActor::HandleLogin)` to confirm UFUNCTION dynamic binding compiles.

### Validator (`check_net_subsystem_shape` extension)

Asserts in source (one error per missing piece):

- Header declares both `DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam` macros with the exact `FPaldarkPlayerLoginSignature` / `FPaldarkPlayerLogoutSignature` type names.
- Header exposes `BroadcastPlayerLogin` / `BroadcastPlayerLogout` UFUNCTION + 2 `BlueprintAssignable` `OnPlayerLogin` / `OnPlayerLogout` UPROPERTY fields.
- Cpp implements both `UPaldarkNetSubsystem::BroadcastPlayer*` methods, fires `OnPlayerLogin.Broadcast(...)` + `OnPlayerLogout.Broadcast(...)`, and guards against `NM_Client` broadcast attempts.
- GameMode cpp includes `Net/PaldarkNetSubsystem.h`, resolves the subsystem via `GetSubsystem<UPaldarkNetSubsystem>`, and calls `BroadcastPlayerLogin(NewPlayer)` / `BroadcastPlayerLogout(Exiting)` from its `PostLogin` / `Logout`.

The validator does NOT verify Blueprint binding — that's authored content (uasset binary, see L-22 carry). The on-screen log line `N listener(s) bound` confirms the delegate is actually multicast'd at runtime.

### Anti-patterns avoided (W39 L-20)

- Subclassing `APaldarkGameModeBase` per widget that needs login awareness → forces a single-inheritance chain through extraction + hub-town + brief-room subclasses. Fix: subsystem multicast delegate.
- Using `DECLARE_MULTICAST_DELEGATE_OneParam` (non-dynamic) → not `BlueprintAssignable`-compatible. Fix: `DYNAMIC_MULTICAST_DELEGATE_OneParam` so Blueprint can bind.
- Calling `OnPlayerLogin.Broadcast(NewPlayer)` directly from `APaldarkGameModeBase::PostLogin` → the delegate field lives on the subsystem; the GameMode would only see it after resolving the subsystem. Fix: route through `BroadcastPlayerLogin` so the authority + null guard is centralised.
- Broadcasting after `Super::Logout` → `Exiting->PlayerState` is often null'd during the super call's RemovePlayer path. Fix: broadcast first, then `Super::Logout`.
- Forgetting `NM_Client` guard in `BroadcastPlayer*` → a Blueprint caller from a client-side widget would silently spam `Broadcast` on an empty listener list. Fix: subsystem checks `World->GetNetMode() != NM_Client`.
- Skipping the `NewPlayer != nullptr` early-out → engine sometimes passes null during seamless travel edge cases (rare but documented). Fix: validate the pointer before calling `GetName()` or `Broadcast`.
- Replacing the existing `++ConnectedPlayerCount` log line in `PostLogin` with the delegate call → the count line is consumed by the W14-15 4-player smoke test ("X/Y player(s) connected"). Fix: keep both, the delegate fires alongside.

### Out of scope (defer)

- **Client-side player-join multicast** — clients use the GameState `PlayerArray` REPNOTIFY for this. A dedicated client-side `OnRemotePlayerJoined` delegate would duplicate that signal and risk drift; deferred to W40-41 hub town widget design if widget authors genuinely need it.
- **Server-side reconnection event** — Q4 polish item. UE5 supports `OnConnectionClosed` on the NetDriver but mapping that back to a per-player delegate requires PlayerState identity tracking (PlayerId / UniqueNetId) which Q4 backend AWS week will introduce alongside Cognito JWT.
- **Login throttling / rate-limit** — the delegate fires for every PostLogin; a Q4 hub-town brief room with 8 listeners would see 8 log lines per join. Fine for 8 players; revisit if hub-town capacity grows past Pillar P11 limit.
- **Travel-failure-aware logout** — if a client crashes mid-game the engine fires `Logout` with the controller still half-valid. Q4 polish: pair the delegate with the existing `OnNetworkFailure` relay for a richer disconnect cause.

## Lag compensation server-side rewind (W16-17)

W16-17 ports the [10] Udemy MP Shooter ("Blaster") server-side rewind pattern
onto PaldarkLab. The outcome the roadmap pins is:

> **Hitscan accurate với 100ms ping artificial.** — `Documents/PALDARK/03-Roadmap_1_Year.md` § Tuần 16–17

This is a **networking infrastructure week**, not a feature week. The PR ships
the machinery so a designer can verify hitscan accuracy in PIE with
`Net PktLag=100`. The HitscanFire ability itself still lives in W18 — see
the "deferred work" subsection at the bottom.

### Component layout

```
APaldarkCharacter
├── LagCompSlot (UPaldarkLagCompensationComponent)  ← server-only tick
│   └── FrameHistory: TDoubleLinkedList<FPaldarkFramePackage>
│       └── HitBoxes: TMap<FName, FPaldarkBoxInformation>
└── HitCollisionBoxes: TMap<FName, UBoxComponent*>  ← 16 per-bone hitboxes
```

- `UPaldarkLagCompensationComponent` lives at `Source/PaldarkLab/{Public,Private}/Combat/PaldarkLagCompensationComponent.{h,cpp}`.
- Per-bone hitbox map lives on `APaldarkCharacter` (private member,
  read-only accessor `GetHitCollisionBoxes()`). The constructor creates 16
  boxes attached to default UE5 mannequin sockets: `head`, `pelvis`,
  `spine_02/03`, `upperarm_{l,r}`, `lowerarm_{l,r}`, `hand_{l,r}`,
  `thigh_{l,r}`, `calf_{l,r}`, `foot_{l,r}`. All default to
  `NoCollision`; the rewind flow flips them to `QueryOnly` for the trace
  window and back.

### Frame buffer (server tick)

Every server tick the lag comp component:

1. Snapshots `Location / Rotation / BoxExtent` of every box on its owner into
   a `FPaldarkFramePackage` and pushes it onto the head of `FrameHistory`.
2. Prunes the tail until the oldest entry is within `MaxRecordTime`
   (default 4 s — covers ~250 ms LAN ping + 100 ms artificial + headroom).

```cpp
struct FPaldarkBoxInformation { FVector Location; FRotator Rotation; FVector BoxExtent; };
struct FPaldarkFramePackage  { float Time; TMap<FName, FPaldarkBoxInformation> HitBoxes; ... };
```

At 30 Hz server tick × 4 s window the buffer caps at ~120 frames × 16 boxes
≈ 1.9k FPaldarkBoxInformation entries per character. Cheap enough that we
don't need a fixed-size circular buffer for W16-17.

### Time sync handshake (client → server)

`APaldarkPlayerController` exposes a 2-RPC handshake plus a public getter:

```cpp
UFUNCTION(Server, Reliable) void ServerRequestServerTime(float TimeOfClientRequest);
UFUNCTION(Client, Reliable) void ClientReportServerTime(float TimeOfClientRequest,
                                                       float TimeServerReceivedClientRequest);
float GetServerTime() const;      // ← shooter samples this at trigger pull
float GetSingleTripTime() const;  // ← shooter subtracts this to back-date HitTime
```

The autonomous client:

1. Calls `ServerRequestServerTime` immediately in `ReceivedPlayer` to seed
   `ClientServerDelta` and `SingleTripTime` before the first input arrives.
2. Re-issues the RPC every `TimeSyncFrequency` seconds (default 5 s, in
   `PlayerTick`) so clock drift over a long match stays sub-millisecond.

The server's own controller short-circuits the handshake (`HasAuthority()`
returns true → no RPC). `GetServerTime()` returns `WorldTime` directly on
the server and `WorldTime + ClientServerDelta` on a client.

### Server-side rewind (server, on score request)

The flow:

1. Predicted shooter (W18+) traces locally, records hit, samples
   `GetServerTime() - GetSingleTripTime()`, and calls
   `UPaldarkLagCompensationComponent::ServerScoreRequest_Hitscan(target, start, hitLoc, hitTime)`.
2. Server-side `ServerSideRewind` looks up the target's lag comp, walks
   `FrameHistory` from newest backward to find the two frames bracketing
   `HitTime`, interpolates a synthetic frame, and calls `ConfirmHit`.
3. `ConfirmHit` caches live box state, moves boxes to the rewound frame,
   traces head-first (for headshot priority) then body, restores live
   state, and returns `FPaldarkServerSideRewindResult`:

```cpp
struct FPaldarkServerSideRewindResult
{
    bool         bHitConfirmed = false;
    bool         bHeadShot     = false;
    FVector      ImpactPoint   = FVector::ZeroVector;
    float        RewindTime    = 0.f;
    FGameplayTag ResultTag;     // Paldark.LagComp.{HitConfirmed,HitRejected}
};
```

Three rejection paths (all logged at `Verbose` on `LogPaldarkNet`):

- `HitCharacter == nullptr` or missing lag comp → reject.
- `FrameHistory` empty (just spawned) → reject.
- `HitTime` older than the oldest sample (latency > `MaxRecordTime`) → reject.

### Console commands (W16-17)

| Console command                                          | What it does                                                                                                          |
|----------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------|
| `Paldark.LagComp.DumpHistory`                            | Dump local Paldark pawn's `FrameHistory` (oldest, newest, count) + server time estimate to `LogPaldarkNet`.            |
| `Paldark.LagComp.SimulateHit [BackdateMs]`               | Camera-trace from the local pawn, then call `ServerScoreRequest_Hitscan` with `HitTime` back-dated by `BackdateMs` (default 100). Logs hit/miss + headshot bit. Lets a designer exercise the rewind path without the W18 predicted-fire ability. |
| `Paldark.LagComp.ArtificialPing [Ms]`                    | Wrapper around UE's `Net PktLag=N`. Default 100 ms; pass `0` to clear. Use in PIE to reproduce a "client behind by 100 ms" scenario. |

### Authoring + test order

Designer side (sau khi compile xanh — VM của Devin không có UE 5.4 compiler nên
PR này chỉ ship C++ + structural CI; bước compile + PIE phải chạy ở máy designer):

1. **Open `Raid_Sandbox`** (W1 day 8-10 sandbox map) and set the experience
   to `PaldarkExperience.PX_RaidSandbox`.
2. **Smoke test (no latency)**:
   - PIE 1-player Standalone.
   - Spawn a dummy: `Paldark.Combat.SpawnDummy 500`.
   - Aim at the dummy. Run `Paldark.LagComp.SimulateHit 0` → expect
     `LagComp::ConfirmHit … hit=1 head=0/1 …` line in `LogPaldarkNet`.
   - Run `Paldark.LagComp.DumpHistory` → expect `count=~120 oldest=… newest=…`.
3. **100 ms artificial ping**:
   - In the **client** PIE instance: `Paldark.LagComp.ArtificialPing 100`.
   - Aim at the dummy. `Paldark.LagComp.SimulateHit 100` → expect
     `hit=1` even though the visible dummy position is 100 ms ahead of the
     rewound geometry. This is the W16-17 milestone.
4. **2-player listen test**:
   - PIE Listen-Server, 2 clients.
   - Client 2: `Paldark.LagComp.ArtificialPing 100` then
     `Paldark.LagComp.SimulateHit 100` aiming at Client 1's pawn.
   - Confirm Client 2 reads `hit=1` in its log + Client 1 (server) reads the
     matching `LagComp::ConfirmHit` line.

### Pillar coverage

| Pillar | Lesson exercised |
|--------|------------------|
| P06 L4 Lag compensation (server-side rewind) | `UPaldarkLagCompensationComponent` + frame buffer + interpolation + bracketing |
| P06 L5 RPCs (Server / Client / Reliable) | `ServerRequestServerTime` + `ClientReportServerTime` + `ServerScoreRequest_Hitscan` |
| P02 L4 Subsystem-vs-component lifetime | Lag comp is a component (per-character state), not a subsystem |
| P03 L3 Composition slot pattern | `LagCompSlot` added as 13th slot on `APaldarkCharacter` without re-shaping the existing 12 |
| P15 L2 Bandwidth budgeting | `SetIsReplicatedByDefault(true)` only because the score-request RPC needs an owner channel; no UPROPERTY replication |

### Deferred work (NOT in W16-17)

The following items are intentionally **not** in this PR — they ship in
later weeks per the roadmap:

- **GA_HitscanFire LocalPredicted refactor** — the W9-10 ability stays
  `NetExecutionPolicy=ServerOnly` for now. W18-19 (Pal Combat Activity)
  flips it to `LocalPredicted` and routes the trace through
  `ServerScoreRequest_Hitscan`. The plumbing is ready (the RPC compiles +
  the result struct carries the headshot bit); only the ability glue is
  deferred so this PR stays focused on rewind infrastructure.
- **Headshot damage path through GE** — the lag comp result carries
  `bHeadShot`, but the W18 ability is where the
  `Paldark.SetByCaller.HeadshotMultiplier` magnitude gets bumped to use
  this. Body / head tags (`Paldark.Hit.{Bodyshot,Headshot}`) are already
  registered (W9-10 + this PR) so the GE side compiles unchanged.
- **Client-side prediction (full lag compensation loop)** — W22+ task.
  W16-17 only ships *server-side* rewind. The client still pays input
  latency until the predicted fire path lands.
- **Anti-cheat heuristics** — variance bounds on the reported HitTime vs
  observed RTT, weapon cone-of-fire validation, etc. Live ops polish in
  W42-43.

### Anti-patterns to avoid (lag compensation)

- **Server-side trace against the live pose** — clients see the dummy
  100 ms ahead. Tracing against live geometry silently misses ≈30 % of
  legitimate hits at 100 ms ping. The whole point of the rewind path is
  to trace against the pose the shooter actually saw.
- **Storing `FrameHistory` as `TArray`** — head-insert + tail-prune are
  O(1) on `TDoubleLinkedList` and O(n) on `TArray`. At 30 Hz × 4 s the
  array shift cost is small enough that it doesn't blow up, but the linked
  list is a free win and matches the [10] reference implementation.
- **Hitting boxes by pointer identity** — designers can rename the head
  socket to `Head` (capitalised) or skin a non-mannequin skeleton. Looking
  up boxes by `FName` keeps the rewind code working as long as the
  hitbox key matches the lag comp's `HeadBoneName` UPROPERTY.
- **Leaving boxes in `QueryOnly`** — the rewind trace enables them just
  long enough to fire the trace, then drops them back to `NoCollision`.
  Forgetting the restore step makes the next live-frame trace hit
  ghosts of the previous rewound pose.
- **Calling `ServerScoreRequest_Hitscan` from the server** — the RPC is
  marked `Server, Reliable`; the server can technically invoke it
  locally, but the resulting trace would already see live geometry. The
  console command `Paldark.LagComp.SimulateHit` is the right way to
  exercise the path on the host without going through a client.

## Pal Combat Activity (W18-19)

W18-19 ports the [04] ReadyOrNot Activity FSM idea onto the Pal companion side
and adds a Utility-AI-lite scoring layer borrowed from [12] UE4 Pro. The
outcome the roadmap pins is:

> **Pal companion auto-defend player khi enemy approach.** —
> `Documents/PALDARK/03-Roadmap_1_Year.md` § Tuần 18–19

This is a **Pal subsystem week**, not a feature week. The PR promotes the
W3-4 `UPaldarkPalPerceptionComponent` + `UPaldarkPalCombatComponent` stubs
into real components, plugs them into the Activity FSM via a new
`UPaldarkActivity_Combat`, and ships a server-only `UPaldarkGameplayAbility_PalAttack`
that the activity drives. Designer must compile + author the damage GE
Blueprint subclass + bind it to the Pal's `AttackAbilityClass` to see real
damage land on a dummy.

### Component / activity layout

```
APaldarkPalCharacter
├── PerceptionSlot (UPaldarkPalPerceptionComponent)  ← server-only, TActorIterator scan
│   └── OnThreatChanged multicast → CombatSlot binds in BeginPlay
├── CombatSlot (UPaldarkPalCombatComponent)
│   ├── AttackAbilityClass (TSoftClassPtr) → GiveAbility on BeginPlay
│   └── CurrentTargetActor (TWeakObjectPtr) ← perception's pick
├── ActivitySlot (UPaldarkPalActivityComponent)
│   └── CandidateActivities = [Idle (10), Follow (20), Investigate (30), Combat (40)]
│       └── UPaldarkActivity_Combat — utility-driven, preempts on threat
└── TeamTag = `Paldark.Team.Player` (default — flip in Blueprint subclass for hostile Pals)
```

| Source path | Role |
|-------------|------|
| `Source/PaldarkLab/Public/Pal/Combat/PaldarkPalConsideration.h` | `FPaldarkConsideration` struct + `ComputeUtilityScore` helper (Utility-AI-lite). |
| `Source/PaldarkLab/Public/Pal/Components/PaldarkPalPerceptionComponent.h` | W3-4 stub → W18-19 real: distance scan + hysteresis + grace timer + `OnThreatChanged`. |
| `Source/PaldarkLab/Public/Pal/Components/PaldarkPalCombatComponent.h` | W3-4 stub → W18-19 real: grant `AttackAbilityClass`, track cooldown, expose `TryFireAttack`. |
| `Source/PaldarkLab/Public/Pal/Activities/PaldarkActivity_Combat.h` | Priority 40 activity. CanRun = perception locked + utility score ≥ `MinUtilityScore`. |
| `Source/PaldarkLab/Public/Gas/Abilities/PaldarkGameplayAbility_PalAttack.h` | AI-only server ability — reads target from combat comp, applies damage GE via `SetByCaller.PalDamage`. |
| `Source/PaldarkLab/Private/Pal/PaldarkPalConsoleCommands.cpp` | 3 W18-19 debug commands (DumpThreat / SpawnHostileDummy / ForceCombat). |

### Threat scan + aggro hysteresis

`UPaldarkPalPerceptionComponent::TickComponent` runs **only on authority**.
Every `ScanInterval` (default 0.25 s) it:

1. Walks `TActorIterator<AActor>` inside `ThreatRadius` (default 1500 cm).
2. Resolves each candidate's TeamTag via `GetActorTeamTag` (cast to
   `APaldarkCharacter` / `APaldarkPalCharacter` / `APaldarkDummyTarget`).
   Friendly matches are skipped; hostile matches become candidates.
3. Picks the closest valid candidate. If different from `CurrentThreat`,
   applies hysteresis: the new pick must be at least
   `AggroSwitchHysteresisCm` (200 cm) closer than the existing one —
   otherwise the perception holds.
4. When no candidate is found, arms a `ThreatGracePeriodSeconds` (3 s)
   timer that ticks down every frame. On expiry, `CurrentThreat` clears
   and `OnThreatChanged.Broadcast(old, nullptr)` fires.

```cpp
DECLARE_MULTICAST_DELEGATE_TwoParams(
    FOnPaldarkThreatChanged, AActor* /*OldThreat*/, AActor* /*NewThreat*/);
```

The combat component binds this in `BeginPlay` so target switching is
push-driven, not poll-driven.

### Utility AI scoring

```cpp
struct FPaldarkConsideration { FName Name; float NormalizedScore; float Weight; };
float ComputeUtilityScore(const TArray<FPaldarkConsideration>& Considerations);
```

`UPaldarkActivity_Combat::BuildConsiderations` populates three rows per
tick:

| Name | Formula | Default weight |
|------|---------|----------------|
| `ThreatDistance` | `1 - clamp(D / MaxEngageRange, 0, 1)` | `1.0` |
| `PalHealth` | `Health / MaxHealth` | `0.6` |
| `AttackReady` | `Combat->IsAttackReady() ? 1 : 0` | `0.4` |

`ComputeUtilityScore` returns the weighted average (clamped by weight
sum). The activity's `CanRun` requires:

1. `Perception->HasThreat() && Combat->GetCurrentTarget() != nullptr`
2. `Score >= MinUtilityScore` (default 0.45).

`ShouldContinue` keeps the activity running while the target stays valid
and `Distance <= DisengageDistance` (default 2000 cm).

### Pal attack ability

```cpp
UCLASS()
class UPaldarkGameplayAbility_PalAttack : public UPaldarkGameplayAbility
{
    // NetExecutionPolicy = ServerOnly
    // AbilityTags += Paldark.Ability.PalAttack
    UPROPERTY(EditDefaultsOnly) TSubclassOf<UGameplayEffect> DamageEffectClass;
    UPROPERTY(EditDefaultsOnly) float FallbackBaseDamage = 10.f;
};
```

`ActivateAbility` runs on the server, resolves the current target from
`UPaldarkPalCombatComponent::GetCurrentTarget` (no aim trace — the Pal
already faces the threat via the activity's `Tick`), and applies
`DamageEffectClass` with `SetByCaller.PalDamage = Combat->BasePalDamage`.
`UPaldarkDamageExecutionCalculation` reads both
`SetByCaller.BaseDamage` (player weapons) and `SetByCaller.PalDamage`
(Pal attack) and sums them — so designers can share a single damage GE
across the player + Pal sources.

### Team tag identity

Three classes now expose `GetTeamTag()` + a `UPROPERTY TeamTag`
initialised in the constructor:

| Class | Default tag | Override path |
|-------|-------------|---------------|
| `APaldarkCharacter` | `Paldark.Team.Player` | Blueprint subclass `TeamTag` for PvP / raid factions. |
| `APaldarkPalCharacter` | `Paldark.Team.Player` | Hostile-Pal subclass (W20-21 Direhound / Razorbird) flips to `Paldark.Team.Hostile`. |
| `APaldarkDummyTarget` | `Paldark.Team.Hostile` | Friendly-dummy variant for VFX tests can override per-instance. |

The Pal perception's `FriendlyTeamTag` / `HostileTeamTag` UPROPERTY pair
drives the filter — set both on the Blueprint subclass for designer-side
team experiments without touching C++.

### Console commands (W18-19)

| Console command | What it does |
|-----------------|--------------|
| `Paldark.Pal.DumpThreat` | Iterate every Pal in the world and dump its perception state (`threat=… distance=… grace=…`) + combat state (`target=… ready=… cooldown_remaining=…`) + activity tag to `LogPaldarkPal`. |
| `Paldark.Pal.SpawnHostileDummy [DistanceCm=600]` | Spawn one `APaldarkDummyTarget` in front of the local player at the requested distance. Default `TeamTag = Paldark.Team.Hostile` makes the nearest Pal aggro on the next scan tick. |
| `Paldark.Pal.ForceCombat [PalName] [TargetName]` | Bypass perception entirely — call `ForceThreat` on the named Pal with the named target. No args = first Pal in world + closest hostile dummy. Useful to repro the activity path without waiting for the scan tick. |

### Authoring + test order

Designer side (sau khi compile xanh — VM của Devin không có UE 5.4 compiler nên
PR này chỉ ship C++ + structural CI):

1. **Author `GE_Damage_Pal`** — Blueprint subclass `UGameplayEffect` with
   `Modifier ▸ Add ▸ Attribute=Health Magnitude=SetByCaller(Paldark.SetByCaller.PalDamage)`.
   Attach `UPaldarkDamageExecutionCalculation` as the execution so the
   armor mitigation curve still runs. (Or reuse `GE_Damage_Standard` from
   W9-10 — the execution already sums both keys.)
2. **Author `BP_GA_Pal_Attack`** subclass of `UPaldarkGameplayAbility_PalAttack`.
   Set `DamageEffectClass = GE_Damage_Pal`. Leave `FallbackBaseDamage` at 10.
3. **Author `BP_Pal_Default`** subclass of `APaldarkPalCharacter`. Add a
   combat-component default override: `AttackAbilityClass = BP_GA_Pal_Attack`,
   `BasePalDamage = 12`, `AttackInterval = 1.5`, `MinEngageRange = 200`,
   `MaxEngageRange = 1200`. The perception component defaults already
   match the sandbox (radius 1500, scan 0.25 s, hysteresis 200, grace 3 s).
4. **Open `Raid_Sandbox`** with `?Experience=PaldarkExperience.PX_RaidSandbox`.
5. **Spawn the test rig**:
   - `Paldark.Pal.SpawnTestCompanion` — spawn one Pal behind the player.
   - `Paldark.Pal.SpawnHostileDummy 800` — spawn a hostile dummy 8 m in front.
   - Within ~0.25 s the Pal switches from Follow → Combat (`LogPaldarkPal`
     logs `SwitchToActivity` from `Paldark.Pal.Activity.Follow` to
     `Paldark.Pal.Activity.Combat`).
   - Pal rotates to face the dummy and fires every 1.5 s. Dummy Health
     ticks down each shot. After 9 hits (108 damage at 12 each → dummy
     dies at 100 Health) the dummy enters `Paldark.State.IsDead` and the
     Pal's perception times out (grace), switching the activity back to
     Follow.
6. **Aggro switch smoke test** (optional):
   - Spawn 2 hostile dummies at ~600 + ~900 cm.
   - `Paldark.Pal.DumpThreat` — confirm the Pal is targeting the closer
     one. Walk so the further dummy is now ~100 cm closer than the
     current pick. The Pal **should NOT** switch (under
     `AggroSwitchHysteresisCm = 200`). Push the further dummy another
     150 cm closer (~250 cm delta) and the Pal switches.
7. **ForceCombat fallback** — if perception fails, run
   `Paldark.Pal.ForceCombat` (no args) and confirm the activity still
   enters Combat. Validates the activity / ability path independent of
   scan.

### Pillar coverage (W18-19)

| Pillar | Lesson exercised |
|--------|------------------|
| P09 L1 Activity FSM | `UPaldarkActivity_Combat` completes the Idle/Follow/Investigate/Combat quartet with priority preemption. |
| P09 L3 Utility AI lite | `FPaldarkConsideration` + `ComputeUtilityScore` weighted-average scoring. |
| P09 L5 Custom perception | `UPaldarkPalPerceptionComponent` — bespoke TActorIterator scan, no UAIPerceptionComponent. |
| P08 L2 GE authoring + SetByCaller | `Paldark.SetByCaller.PalDamage` magnitude on shared damage GE. |
| P08 L3 Ability lifetime | Combat component owns the GiveAbility / ClearAbility pair (BeginPlay / EndPlay). |
| P02 L4 Subsystem vs component | Combat + perception live on the actor — per-Pal state, not world state. |
| P12 L1 Data-driven | Designer knobs (`ScanInterval`, `ThreatRadius`, `MinUtilityScore`, weights, ability class) all UPROPERTY. |

### Deferred work (NOT in W18-19)

The following items are intentionally **not** in this PR — they ship in
later weeks per the roadmap:

- **Direhound pack AI / Razorbird aerial AI** — hostile-Pal subclasses
  with shared blackboard + flock cohesion. W20-21 task.
- **GA_HitscanFire → LocalPredicted refactor** — the player Fire ability
  still uses W9-10's `ServerOnly` policy. W18-19 wires the lag comp
  endpoint but the predicted path is W22+ (Q3 milestone).
- **Pal ability authoring (per-species)** — each Pal subclass needs its
  own `AttackAbilityClass`. W18-19 ships one (`BP_GA_Pal_Attack`); the
  species roster grows W30+.
- **Animation montage trigger** — `UPaldarkGameplayAbility_PalAttack`
  applies damage without a montage hookup. W22+ task when Pal AnimBP
  lands.
- **UAIPerceptionComponent integration** — the current scan is
  `TActorIterator<AActor>` filtered by radius. Sufficient for 1 Pal + 1
  dummy in W18-19 sandbox; W48-49 swaps in a registry subsystem for the
  full open-world scale.
- **Client-side prediction for Pal abilities** — ServerOnly stays. The
  predicted Pal path lands alongside montage support (W22+).

### Anti-patterns to avoid (Pal combat)

- **Using `bOrientRotationToMovement = true`** — `APaldarkPalCharacter`'s
  movement comp must use `bUseControllerDesiredRotation` (or face-target
  via `SetActorRotation` while movement comp's orient flag is off).
  Otherwise `UPaldarkActivity_Combat::TickActivity`'s
  `SetActorRotation` fights the movement component every frame.
- **Granting `AttackAbilityClass` from constructor / OnConstruction** —
  ASC ability grants need to happen **after** the ASC + AvatarActor are
  fully initialised. The right hook is the combat component's
  `BeginPlay` on authority.
- **Resolving `GetCurrentTarget` via `FindComponentByClass` per tick** —
  cache the combat component pointer on `InitActivity` and store a weak
  ptr. The `EvaluateAndSwitch` selection cadence (0.25 s) is fast enough
  that even cheap lookups stack up if you have many Pals.
- **Reading attribute values directly off `UPaldarkAttributeSet`
  members** — go through the `GetX` accessors generated by
  `ATTRIBUTE_ACCESSORS`. They return the post-modifier "current" value;
  the raw member can be a `FGameplayAttributeData` mid-application.
- **Forgetting `OnThreatChanged.RemoveAll(this)`** in `EndPlay` — the
  multicast delegate holds a raw `this` pointer, and the perception
  outlives the combat component on actor destroy ordering edge cases.
- **`TryActivateAbility` without checking the return value** — when the
  ASC rejects activation (cooldown, blocked tag, missing GE), the
  combat component must not bump `LastAttackTime`. The current code
  guards this via `if (bActivated) LastAttackTime = …;`.

## Hostile-Pal AI subclasses (W20-21)

W20-21 turns the W18-19 Pal combat plumbing on a new actor: a **hostile**
Pal subclass. Same perception + combat + activity stack; the only thing
that flips is the team tag, the candidate activity list, and 2 new
activities (Patrol / Stalk). The roadmap deliverable:

> **Map test có 4 Direhound patrol, attack player khi gần.** —
> `Documents/PALDARK/03-Roadmap_1_Year.md` § Tuần 20–21

This is a **Pal subsystem week**. C++ adds 2 hostile-Pal subclasses
(Direhound ground pack + Razorbird aerial solo), a UWorldSubsystem
pack registry, a designer-placeable spawner actor, and 2 new activities
that slot into the existing FSM ladder.

### Activity FSM ladder (hostile-Pal)

```
Priority  Activity                  Owner trigger
────────  ────────                  ────────────────
   10     Idle                      always (fallback)
   15     Patrol     (W20-21 new)   Pal has waypoints + no threat
   25     Stalk      (W20-21 new)   Pal has threat & distance > MinEngageRange
   40     Combat     (W18-19)       Pal has threat & utility ≥ MinUtilityScore
```

The Follow / Investigate activities from W5-6 are intentionally absent
from the hostile-Pal `CandidateActivities` list — a hostile Direhound
doesn't follow the player, doesn't get pinged, and falls back to Idle
when nothing else fires. The constructor of each species subclass
replaces the default candidate list with the hostile quartet
`[Idle, Patrol, Stalk, Combat]`.

### Component / subsystem layout (hostile-Pal)

```
APaldarkPalCharacter_Direhound : APaldarkPalCharacter
├── PerceptionSlot (W18-19)            ← FriendlyTeamTag flipped to Hostile
│   └── HostileTeamTag = Paldark.Team.Player (perceives the player as threat)
├── CombatSlot (W18-19)                ← tuned: MinEngageRange 60, BasePalDamage 18
│   └── On HandleThreatChanged → call BroadcastPackThreat (W20-21 new)
├── ActivitySlot (W5-6)                ← CandidateActivities = [Idle, Patrol, Stalk, Combat]
├── PatrolSlot   (W20-21 new)          ← waypoint list + index
└── SpeciesTag = Paldark.Pal.Species.Direhound
    PackTag    = Paldark.Pal.Pack.Direhound

UPaldarkHostilePackSubsystem (UWorldSubsystem)   ← registry per PackTag,
                                                    distance-gated broadcast
```

| Source path | Role |
|-------------|------|
| `Source/PaldarkLab/Public/Pal/PaldarkHostilePackSubsystem.h` | `UWorldSubsystem` with per-PackTag registry + `BroadcastPackThreat` + `GetPackSize` + `DumpToLog`. Server-only (filtered via `ShouldCreateSubsystem`). |
| `Source/PaldarkLab/Public/Pal/Components/PaldarkPalPatrolComponent.h` | 9th Pal component slot — waypoint list + current index + `ArrivalRadius` + `SetWaypointsFromLocations`. |
| `Source/PaldarkLab/Public/Pal/Activities/PaldarkActivity_Patrol.h` | Priority 15 — walks Pal between waypoints. `CanRun` requires `Patrol->HasWaypoints() && !Perception->HasThreat()`. |
| `Source/PaldarkLab/Public/Pal/Activities/PaldarkActivity_Stalk.h` | Priority 25 — closes in on threat at `StalkSpeedScale * MaxWalkSpeed` until inside `MinEngageRange`, at which point Combat (P40) preempts. |
| `Source/PaldarkLab/Public/Pal/PaldarkPalCharacter_Direhound.h` | Ground pack predator. Ctor flips TeamTag to Hostile + sets `SpeciesTag = Paldark.Pal.Species.Direhound` + tunes melee combat. |
| `Source/PaldarkLab/Public/Pal/PaldarkPalCharacter_Razorbird.h` | Aerial solo. Ctor switches movement to `MOVE_Flying`, widens perception radius, tunes ranged peck combat. |
| `Source/PaldarkLab/Public/Pal/PaldarkHostilePalSpawner.h` | Designer-placeable actor. Spawns N Pals + distributes waypoints + stamps `PackTag` on each. |

### Pack subsystem (shared blackboard)

Each hostile-Pal subclass registers with `UPaldarkHostilePackSubsystem`
in `BeginPlay` (server-only) and unregisters in `EndPlay`. The
subsystem keeps a `TMap<FGameplayTag, FPaldarkHostilePack>` where each
pack is a `TArray<TWeakObjectPtr<APaldarkPalCharacter>>`.

When any Pal's `UPaldarkPalCombatComponent::HandleThreatChanged` fires
with a new (non-null, changed) threat:

```cpp
void UPaldarkPalCombatComponent::HandleThreatChanged(AActor* Old, AActor* New)
{
    // ... existing W18-19 logic (target switch + maybe TryFireAttack) ...
    if (UPaldarkHostilePackSubsystem* PackSub = World->GetSubsystem<UPaldarkHostilePackSubsystem>())
    {
        PackSub->BroadcastPackThreat(GetPalOwner(), New);
    }
}
```

The subsystem iterates the initiator's pack, filters by
`PackBroadcastRadius` (default 2000 cm) around the initiator's
location, and calls `Perception->ForceThreat(NewThreat)` on each
packmate that doesn't already see the same target. The "doesn't
already see the same target" guard breaks the otherwise-infinite
A→B→A feedback loop.

Lone Pals (Razorbird without a pack tag) get a no-op broadcast —
`PackTag.IsValid() == false` returns early.

### Patrol activity (P15)

Designer drops waypoint `AActor`s in the level + assigns them to a
`APaldarkHostilePalSpawner::WaypointActors` array. The spawner calls
`Patrol->SetWaypointsFromLocations(WaypointLocations)` on each spawned
Pal so every pack member shares the same patrol path (with a random
start index if `bRandomizeStartIndex = true`).

```cpp
void UPaldarkActivity_Patrol::TickActivity_Implementation(float DeltaSeconds)
{
    const FVector Current = Patrol->GetCurrentWaypoint();
    const FVector Direction = (Current - Pal->GetActorLocation()).GetSafeNormal2D();
    Pal->AddMovementInput(Direction);

    if (Pal->GetActorLocation().Distance(Current) <= Patrol->ArrivalRadius)
        Patrol->AdvanceToNextWaypoint();   // wraps around
}
```

`CanRun` is gated on `Patrol->HasWaypoints() && !Perception->HasThreat()`
so as soon as the perception locks onto a target, Stalk (P25) or Combat
(P40) preempts. On enter Patrol calls `SetFollowEnabled(false)` so the
locomotion component doesn't fight the activity's `AddMovementInput`.

### Stalk activity (P25)

Closes the gap between Patrol (P15) and Combat (P40). `CanRun` requires:

1. `Perception->HasThreat() == true`
2. `Perception->GetCurrentThreatDistance() > Combat->MinEngageRange`
3. `Perception->GetCurrentThreatDistance() <= LoseSightDistance` (default 2200 cm)

On enter, Stalk caches `Movement->MaxWalkSpeed`, scales it by
`StalkSpeedScale` (default 0.6 — Direhound visibly slows as it closes
in for a melee), restores it on exit. Tick steers `AddMovementInput`
toward the threat. The instant distance drops below `MinEngageRange`,
Stalk's `CanRun` returns false next eval and Combat (P40) takes over
because its `CanRun` is satisfied.

### Direhound (ground pack predator)

```cpp
APaldarkPalCharacter_Direhound::APaldarkPalCharacter_Direhound()
{
    TeamTag = PaldarkGameplayTags::TAG_Paldark_Team_Hostile;
    SpeciesTag = PaldarkGameplayTags::TAG_Paldark_Pal_Species_Direhound;
    PackTag = PaldarkGameplayTags::TAG_Paldark_Pal_Pack_Direhound;  // auto-pack

    // Perception — flip the team filter.
    PerceptionSlot->FriendlyTeamTag = TAG_Paldark_Team_Hostile;
    PerceptionSlot->HostileTeamTag  = TAG_Paldark_Team_Player;

    // Combat — tune for melee bite.
    CombatSlot->MinEngageRange = 60.f;
    CombatSlot->MaxEngageRange = 250.f;
    CombatSlot->AttackInterval = 1.0f;
    CombatSlot->BasePalDamage  = 18.f;

    GetCharacterMovement()->MaxWalkSpeed = 650.f;

    // FSM — drop Follow / Investigate, add Patrol / Stalk / Combat.
    ActivitySlot->CandidateActivities = {
        UPaldarkActivity_Idle::StaticClass(),
        UPaldarkActivity_Patrol::StaticClass(),
        UPaldarkActivity_Stalk::StaticClass(),
        UPaldarkActivity_Combat::StaticClass(),
    };
}
```

`BeginPlay` auto-assigns the default `Paldark.Pal.Pack.Direhound` tag
if empty, then calls `PackSub->RegisterPalToPack(this, PackTag)`.
`EndPlay` calls `UnregisterPalFromPack`. Designer-side
`BP_Direhound_Default` Blueprint subclass adds mesh + anim BP +
`AttackAbilityClass = BP_GA_Direhound_Bite` (subclass of
`UPaldarkGameplayAbility_PalAttack` with bite-specific damage GE).

### Razorbird (aerial solo)

```cpp
APaldarkPalCharacter_Razorbird::APaldarkPalCharacter_Razorbird()
{
    TeamTag = TAG_Paldark_Team_Hostile;
    SpeciesTag = TAG_Paldark_Pal_Species_Razorbird;
    // No default PackTag — Razorbird is solo.

    PerceptionSlot->ThreatRadius = 2200.f;   // wider — birds see further
    CombatSlot->MinEngageRange = 100.f;       // dive-peck
    CombatSlot->MaxEngageRange = 800.f;
    CombatSlot->AttackInterval = 2.0f;
    CombatSlot->BasePalDamage  = 14.f;

    // MOVE_Flying — `AddMovementInput` is world-space, no navmesh.
    GetCharacterMovement()->DefaultLandMovementMode = MOVE_Flying;
    GetCharacterMovement()->GravityScale = 0.f;
    GetCharacterMovement()->MaxFlySpeed = 700.f;

    ActivitySlot->CandidateActivities = {
        UPaldarkActivity_Idle::StaticClass(),
        UPaldarkActivity_Patrol::StaticClass(),
        UPaldarkActivity_Stalk::StaticClass(),
        UPaldarkActivity_Combat::StaticClass(),
    };
}
```

`BeginPlay` registers with the pack subsystem **only** if PackTag was
set (designer-side override). The console helper
`Paldark.Pal.SpawnRazorbird` always spawns a lone bird.

### Spawner actor

```cpp
UPROPERTY(EditAnywhere) TSubclassOf<APaldarkPalCharacter> PalClassToSpawn;
UPROPERTY(EditAnywhere) int32                              SpawnCount       = 4;
UPROPERTY(EditAnywhere) FGameplayTag                       PackTag;
UPROPERTY(EditAnywhere) TArray<TObjectPtr<AActor>>         WaypointActors;
UPROPERTY(EditAnywhere) float                              SpawnSpreadRadius = 300.f;
```

`BeginPlay` (server / standalone only) iterates `SpawnCount` and calls
`World->SpawnActor<APaldarkPalCharacter>(PalClassToSpawn, …)`. Each
spawn is placed on a ring of radius `SpawnSpreadRadius` around the
spawner, then ground-snapped via a line trace
(`GroundTraceChannel` = `ECC_Visibility`, `GroundTraceUp = 200`,
`GroundTraceDown = 2000`).

After spawn, the spawner calls:
- `Pal->SetPackTag(PackTag)` — stamps the pack tag at runtime.
- `Pal->GetPatrolSlot()->SetWaypointsFromLocations(WaypointLocations)`
  — converts the designer's `WaypointActors` into a `TArray<FVector>`
  on the patrol component.

The static helper `APaldarkHostilePalSpawner::SpawnHostilePackAt` is
exposed for console-driven spawning (used by all 3 W20-21 console
commands).

### Console commands (W20-21)

| Console command | What it does |
|-----------------|--------------|
| `Paldark.Pal.SpawnDirehoundPack [Count=4] [DistanceCm=1200]` | Spawn N Direhound hostiles in a ring `DistanceCm` cm in front of the local player, with the default `Paldark.Pal.Pack.Direhound` tag + 4 auto-generated ring waypoints around the spawn origin. |
| `Paldark.Pal.SpawnRazorbird [DistanceCm=1500]` | Spawn one Razorbird 4 m above ground level in front of the player. No pack tag — lone bird. No waypoints — Patrol falls back to Idle until perception fires. |
| `Paldark.Pal.DumpPackState` | Dump `UPaldarkHostilePackSubsystem` registry + every hostile Pal's species/pack/activity state to `LogPaldarkPal`. Also dumps each hostile Pal's patrol waypoint list. |

### Authoring + test order (W20-21)

Designer side (sau khi compile xanh — VM của Devin không có UE 5.4 compiler):

1. **(Optional) Author `BP_Direhound_Default`** subclass of
   `APaldarkPalCharacter_Direhound`. Set mesh + anim BP + author
   `BP_GA_Direhound_Bite` subclass of W18-19's
   `UPaldarkGameplayAbility_PalAttack` with bite-specific damage GE.
   Wire on `CombatSlot->AttackAbilityClass`. (The raw C++ class spawns
   without a mesh — fine for smoke testing the AI logic.)
2. **(Optional) Author `BP_Razorbird_Default`** subclass of
   `APaldarkPalCharacter_Razorbird`. Same shape — different mesh +
   `BP_GA_Razorbird_DivePeck`.
3. **(Optional) Place `BP_HostilePalSpawner_Direhound`** subclass of
   `APaldarkHostilePalSpawner` in the level. Set `PalClassToSpawn =
   BP_Direhound_Default`, `SpawnCount = 4`, drop 4 `TargetPoint`s as
   `WaypointActors`. (The console helper auto-generates a ring of
   waypoints if you skip this step.)
4. **Open `Raid_Sandbox`** with
   `?Experience=PaldarkExperience.PX_RaidSandbox`.
5. **Spawn a Direhound pack via console**:
   - `Paldark.Pal.SpawnDirehoundPack 4 1500`
   - Within ~0.25 s the pack is registered with `UPaldarkHostilePackSubsystem`
     and starts patrolling the auto-generated ring.
   - Walk toward the pack. As soon as the first Direhound's perception
     locks onto the player, the combat component broadcasts via the
     pack subsystem and **all** packmates within 2000 cm switch to
     Stalk on the next eval (0.25 s) — even the ones who haven't yet
     run their own perception scan.
   - Direhounds close to within `MinEngageRange` (60 cm) and start
     biting at `AttackInterval` (1.0 s).
   - `Paldark.Pal.DumpPackState` — confirms registry + activity tags.
6. **Spawn a Razorbird via console**:
   - `Paldark.Pal.SpawnRazorbird 1500`
   - The bird appears 4 m above ground in front of the player. Because
     no waypoints exist + no threat yet, the FSM falls back to Idle.
   - Walk into the perception radius (2200 cm). The bird's perception
     locks onto the player, Stalk kicks in, the bird closes to
     `MinEngageRange = 100` and pecks every 2 s.
7. **Pack feedback-loop check** (optional):
   - Spawn 2 separate packs (different `PackTag` values via Blueprint
     spawner). Aggro one — the other pack must **not** receive the
     broadcast. Confirmed by `Paldark.Pal.DumpPackState` showing the
     untouched pack's `current_threat` still empty.

### Pillar coverage (W20-21)

| Pillar | Lesson exercised |
|--------|------------------|
| P09 L1 Activity FSM | Patrol (P15) + Stalk (P25) complete the hostile-Pal ladder Idle/Patrol/Stalk/Combat. |
| P09 L4 Pack / squad blackboard | `UPaldarkHostilePackSubsystem` as the shared threat channel (P22-23 squad system will reuse). |
| P02 L7 WorldSubsystem vs ActorComponent | Pack registry on the world (cross-actor shared state) vs Patrol on the actor (per-Pal state). |
| P03 L2 Component slot extension | 9th Pal slot (`PatrolSlot`) is purely additive — no other slot changes. |
| P12 L1 Data-driven knobs | Spawner exposes designer fields (`SpawnCount`, `PackTag`, `WaypointActors`, `SpawnSpreadRadius`) so designers tune without C++. |
| P12 L2 Constructor-as-data | Species subclass constructor stamps `SpeciesTag`/`TeamTag`/movement mode — no separate data asset (roadmap says "spawn manager cơ bản"). |

### Deferred work (NOT in W20-21)

- **Per-species ability authoring** — `BP_GA_Direhound_Bite` /
  `BP_GA_Razorbird_DivePeck` are designer-side BP subclasses of the
  W18-19 `UPaldarkGameplayAbility_PalAttack`. No new C++ ability
  classes ship here.
- **Animation montage trigger** — Pal attack still applies damage
  without a montage hookup. W22+ task with Pal AnimBP.
- **Flock cohesion (boids)** — Razorbird is solo. Multi-Razorbird
  formation flying lands W30+.
- **`UAIPerceptionComponent` / behaviour tree integration** — perception
  is still the W18-19 `TActorIterator` scan; pack is bespoke registry.
  Full AI Perception migration lands W48-49 for open-world scale.
- **Client-side prediction for hostile-Pal abilities** — `ServerOnly`
  net execution policy stays. Predicted hostile-Pal abilities land
  alongside player-side prediction W22+.
- **Save / load pack composition across levels** — pack registry is
  Transient. Save-game integration lands W31-32.

### Anti-patterns to avoid (hostile-Pal)

- **Registering a Pal with the pack subsystem from the constructor** —
  `UPaldarkHostilePackSubsystem` only exists on the world after
  `Initialize`, and the Pal's `HasAuthority()` returns false during
  construction. Always register in `BeginPlay` on authority.
- **`MoveTo()` / `AAIController::MoveToLocation` on a `MOVE_Flying`
  pawn** — flying pawns don't use the navmesh. Drive Razorbird with
  raw `AddMovementInput` world-space; pathfinding via navmesh is
  ground-only.
- **Calling `BroadcastPackThreat` without the feedback-loop guard** —
  the subsystem checks `Perception->GetCurrentThreat() == NewThreat`
  before forcing. Skipping this guard makes A→B→A recurse forever.
- **Using a `UDataAsset` for "hostile pal definition"** — the roadmap
  explicitly says "spawn manager cơ bản" — fold the data into the
  C++ subclass constructor (`SpeciesTag` / `TeamTag` / `MaxWalkSpeed`
  / `CandidateActivities`). Designer-side variation goes on a
  Blueprint subclass.
- **`bOrientRotationToMovement = true` on Razorbird** — same issue as
  W18-19: the activity's `SetActorRotation` (Stalk steering toward
  threat) fights the movement comp's auto-orient. Flip to
  `bUseControllerDesiredRotation = false` + drive rotation manually.
- **Strong pointers in the pack registry** — `FPaldarkHostilePack`
  holds `TArray<TWeakObjectPtr<APaldarkPalCharacter>>`. Strong refs
  would prevent dead Pals from getting GC'd until the subsystem
  cleans up (which only happens on world teardown).
- **Spawning hostile Pals in the constructor of the spawner actor** —
  `BeginPlay` is the right hook. Constructor runs at CDO time + on
  every level-load before the world is ready.

## Squad system (W22-23)

W22-23 turns the 4 player characters in `Raid_Sandbox` into a coordinated
squad through three layered subsystems + a Pal listener activity. Roadmap
deliverable: **"4-player team coordinate qua ping + radial command"**
(`Documents/PALDARK/03-Roadmap_1_Year.md` § Tuần 22-23).

### Architecture (UWorldSubsystem trio + replicated marker)

```
┌─────────────────────────────────────────────────────────────────────┐
│  UPaldarkSquadSubsystem (UWorldSubsystem, server-only)              │
│  - TMap<SquadTag, FPaldarkSquadRoster> SquadRegistry                │
│  - StandingCommandPerSquad / StandingCommandLocationPerSquad        │
│  - OnSquadCommandIssued(Issuer, CommandTag, Location) delegate      │
│  - Mirrors W20-21 UPaldarkHostilePackSubsystem shape.               │
└─────────────────────────────────────────────────────────────────────┘
        ▲                                  ▲
        │ Register on BeginPlay            │ Broadcast on Server RPC
        │                                  │
┌───────┴──────────────────────┐  ┌────────┴───────────────────────┐
│  UPaldarkSquadMembershipComp │  │  UPaldarkSquadCommandComponent │
│  - SquadTag (per-pawn knob)  │  │  - CommandSetSoft (UPrimary…)  │
│  - Auto-registers default    │  │  - IssueCommand(Tag, Location) │
│    Paldark.Squad.Default     │  │  - Server_IssueCommand RPC     │
└──────────────────────────────┘  └────────────────────────────────┘
        ▲                                  ▲
        │ Owned by APaldarkCharacter (2 new component slots, W22-23)
        │
┌─────────────────────────────────────────────────────────────────────┐
│  UPaldarkPingSubsystem (UWorldSubsystem, server-only)               │
│  - SpawnPing(Issuer, Location, MarkedActor?, PingType)              │
│  - DefaultPingLifetime (3.0s) + MinSecondsBetweenPings (0.5s)       │
│  - Spawns APaldarkPingMarker (AActor, bReplicates = true)           │
│       └─ DOREPLIFETIME(PingType / OwningPlayer /                    │
│            MarkedActor / ExpirationServerTime)                      │
└─────────────────────────────────────────────────────────────────────┘
        ▲                                  ▲
        │ APaldarkPlayerController          │ Per-frame tick follows the
        │ Server_RequestPing /              │ marked actor (Enemy pings)
        │ Server_RequestMarkUnderCrosshair  │ until ExpirationServerTime
        │
┌─────────────────────────────────────────────────────────────────────┐
│  UPaldarkActivity_FollowSquadCommand (Pal P22 listener)              │
│  - Init binds SquadSub->OnSquadCommandIssued                         │
│  - HandleSquadCommandIssued: caches CurrentCommandTag/Location       │
│  - Tick steers Pal: Attack→move to enemy ping, Follow/Regroup→leash, │
│    Stay→hold position. Investigate (P30) + Combat (P40) preempt.     │
└─────────────────────────────────────────────────────────────────────┘
```

### Activity FSM priority ladder (post-W22-23)

| Priority | Activity | Trigger | Notes |
|---------:|----------|---------|-------|
| 10 | `UPaldarkActivity_Idle` | Always | Fallback when nothing else can run. |
| 15 | `UPaldarkActivity_Patrol` | Hostile-Pal only (W20-21) | Waypoint loop. |
| 20 | `UPaldarkActivity_Follow` | Friendly Pal with `FollowedPawn` set | Leash to the player. |
| **22** | **`UPaldarkActivity_FollowSquadCommand`** | **Friendly Pal with standing squad command (W22-23)** | **Attack/Follow/Stay/Regroup steering.** |
| 25 | `UPaldarkActivity_Stalk` | Hostile-Pal w/ target outside engage range (W20-21) | Close-in at reduced speed. |
| 30 | `UPaldarkActivity_Investigate` | Ping-driven (W5-6) | Walk to ping location. |
| 40 | `UPaldarkActivity_Combat` | Perception locked on hostile (W18-19) | Top-priority preempt. |

P22 deliberately sits **below Investigate (P30)** so an active "go check
this ping" routine isn't yanked by a Follow command, and **below Stalk
(P25)** so a hostile Pal that's somehow inherited the squad listener
(it shouldn't — see hostile-Pal subclasses) still prioritises target
pursuit. The friendly companion BP picks up FollowSquadCommand because
the base `UPaldarkPalActivityComponent` constructor seeds it into the
default `CandidateActivities` array; hostile-Pal subclasses
(`APaldarkPalCharacter_Direhound` / `_Razorbird`) overwrite the array in
their own constructors and intentionally drop it.

### Squad membership tag scheme

`UPaldarkSquadMembershipComponent::BeginPlay` registers the owning
`APaldarkCharacter` on authority under its `SquadTag`. Default is
`Paldark.Squad.Default` so a 4-player co-op lobby behaves as one squad
out of the box. Designer or future PvP code can override per-pawn via:

```cpp
// At runtime — server side only.
if (UPaldarkSquadMembershipComponent* M = PlayerChar->GetSquadMembershipSlot())
{
    M->SetSquadTag(PaldarkGameplayTags::TAG_Paldark_Squad_Red);
}
```

Multi-squad arbitration (e.g. red vs blue PvP) lands W40+ with the hub
town; W22-23 supports the tag mechanism but only ships the Default tag.

### Ping system

| Field | Default | Notes |
|-------|---------|-------|
| `UPaldarkPingSubsystem::DefaultPingLifetime` | `3.0f` | Server publishes `ExpirationServerTime = GetTimeSeconds() + lifetime`. |
| `UPaldarkPingSubsystem::MinSecondsBetweenPings` | `0.5f` | Per-issuer cooldown. Prevents spam. |
| `UPaldarkPingSubsystem::MarkerClass` | `APaldarkPingMarker::StaticClass()` | Designer can subclass for art / SFX overrides. |
| `APaldarkPingMarker` replicated props | `PingType`, `OwningPlayer`, `MarkedActor`, `ExpirationServerTime` | All `DOREPLIFETIME`. |

`APaldarkPlayerController::Server_RequestMarkUnderCrosshair` re-traces
on the server from the controller's `GetPlayerViewPoint()`, ignoring the
issuer's pawn, against `ECC_Visibility` up to
`MarkEnemyTraceDistance` (default 5000 cm). If the hit actor's
`TeamTag == Paldark.Team.Hostile` the ping is filed as
`Paldark.Ping.Type.Enemy` and the marker caches the hit actor so it
visibly follows the target until expiry. Otherwise the ping is filed as
`Paldark.Ping.Type.Spot` at either the impact point or the trace-end.

Anti-cheat note: the client never tells the server _what_ it hit — only
"please mark whatever's under my crosshair". The server reproduces the
trace from its own authoritative view rotation, so a tampered client
can't conjure a fake enemy ping for an actor that isn't actually
visible from the player's pawn.

### Radial command set authoring (designer)

`UPaldarkSquadCommandSet` is a `UPrimaryDataAsset` listing rows of
`(FGameplayTag CommandTag, FText DisplayName, TSoftObjectPtr<UTexture2D> Icon)`.
Author one DataAsset under `/Game/Paldark/Squad/DA_SquadCommandSet_Default`
(the Asset Manager rule in `Config/DefaultGame.ini` scans this directory)
and assign it to the player Blueprint's
`UPaldarkSquadCommandComponent::CommandSetSoft`. Recommended starter
loadout:

| CommandTag | DisplayName | Icon (placeholder) | Behaviour on Pal |
|------------|-------------|---------------------|------------------|
| `Paldark.Squad.Command.Attack` | "Attack" | `T_RadialIcon_Attack` | Move toward last enemy ping. |
| `Paldark.Squad.Command.Follow` | "Follow Me" | `T_RadialIcon_Follow` | Re-leash to issuer. |
| `Paldark.Squad.Command.Stay` | "Stay" | `T_RadialIcon_Stay` | Hold current position. |
| `Paldark.Squad.Command.Regroup` | "Regroup" | `T_RadialIcon_Regroup` | All Pals walk to issuer. |

The UMG radial wheel widget is **deferred to W22+ polish** (this PR
ships the C++ command channel + console commands so designers can wire
inputs and test behaviour without UI). When the widget lands, it
should:

1. Open on `Paldark.InputTag.RadialOpen` (default Q hold).
2. Resolve the icon array from `CommandSet->Commands`.
3. On release, call
   `SquadCommandComponent->IssueCommand(Selected.CommandTag, Selected.OptionalLocation)`.

### Input wiring (designer)

| InputTag | Default key | Handler entry point | Notes |
|----------|-------------|---------------------|-------|
| `Paldark.InputTag.Ping` | T | Client calls `Server_RequestPing(GetPawn()->GetActorLocation(), Spot)` | Or use the focused crosshair location once the HUD is in. |
| `Paldark.InputTag.MarkEnemy` | V | Client calls `Server_RequestMarkUnderCrosshair()` | Server re-traces. |
| `Paldark.InputTag.RadialOpen` | Q (hold) | UMG opens; selection routes to `SquadCommandComponent->IssueCommand` | UI deferred. |

Three new IMC rows. The existing `InputConfig_Default` already maps
`Move`, `Look`, `Jump`, `Sprint`, `Fire`, `Inventory.*`; designers
extend it with the 3 squad rows.

### Console commands (W22-23)

| Command | Effect |
|---------|--------|
| `Paldark.Squad.Dump` | Logs the squad registry + every active ping + the standing command per squad. Server-side only. |
| `Paldark.Squad.Ping <Spot\|Enemy\|Loot\|Danger> [X Y Z]` | Files a ping. Defaults to the local pawn's location if no coords given. |
| `Paldark.Squad.Command <Attack\|Follow\|Stay\|Regroup> [X Y Z]` | Issues the named radial command via the local pawn's `SquadCommandComponent`. |

All three are registered as `FAutoConsoleCommandWithWorldAndArgs` in
`PaldarkPlayerController.cpp`. They only act on the host's local pawn
(no special remote-target handling — that's the radial wheel's job).

### Authoring + 4-player test loop (after compile)

1. Open the friendly companion Pal Blueprint (`BP_Pal_Default`) — no
   changes needed; the squad listener is wired through the base
   `UPaldarkPalActivityComponent` default candidate list.
2. Open the player Blueprint — confirm `SquadMembershipSlot` +
   `SquadCommandSlot` appear in the Components panel. Assign
   `CommandSetSoft` to `DA_SquadCommandSet_Default` if you've authored
   it. Add the 3 IMC rows for T / V / Q.
3. Boot listen-server PIE with 4 clients (W14-15 procedure).
4. As Client 1, type in console:
   `Paldark.Squad.Ping Spot` — every client should see the world-space
   marker for 3 s.
5. Aim at the W18-19 dummy and run `Paldark.Squad.Ping Enemy` (or
   call `Server_RequestMarkUnderCrosshair` via the V keybind). The
   marker should attach to the dummy and follow it for 3 s.
6. `Paldark.Squad.Command Stay` — your companion Pal should stop where
   it stands until the standing command is overwritten.
7. `Paldark.Squad.Command Follow` — companion Pal re-leashes.
8. `Paldark.Squad.Dump` — server logs roster + active pings.

### Bandwidth + perf notes

- Each ping is **one replicated actor** with 4 small props (gameplay
  tag + 2 actor pointers + 1 float). At 4 players × 1 ping/s peak it
  costs <1 kbps. Persistent decals on world geometry are explicitly
  deferred to W30+ (`/Documents/PALDARK/03-Roadmap_1_Year.md` § VFX pass).
- The squad subsystem is **server-only**; no client-side mirror. Pals
  receive commands via `OnSquadCommandIssued` (server delegate);
  movement replicates through `ACharacter::SetReplicateMovement(true)`
  which is already enabled from W3-4.
- The Pal listener Tick runs at `SelectionInterval` (default 0.25 s
  from W5-6) so a 4-player squad with 8 Pals adds 32 evaluations/s of
  cheap arithmetic — negligible.

### Anti-patterns avoided (W22-23)

- **Per-client ping replication via RPC** — would 4× the bandwidth and
  require manual relevancy logic. Fix: `APaldarkPingMarker` is a
  replicated actor; the engine's relevancy + DOREPLIFETIME handle
  delivery + late-join.
- **Strong refs in the squad registry** — `FPaldarkSquadRoster::Members`
  is `TArray<TWeakObjectPtr<APaldarkCharacter>>`. Strong refs would
  prevent logged-out players from GC until world teardown.
- **Storing the marked actor as a name string** — fragile against
  rename / load-order. Fix: `MarkedActor` is a `TWeakObjectPtr<AActor>`
  replicated via DOREPLIFETIME, the engine handles cross-machine
  resolution by NetGUID.
- **Letting clients pick the ping type** — client supplies `PingType`
  but server reproduces enemy traces; `Server_RequestMarkUnderCrosshair`
  is the only path that promotes a ping to `Enemy` because only the
  server can verify the team.
- **Pal squad listener overriding Combat (P40) / Investigate (P30)** —
  designer-expressed intent ("attack that ping") still routes through
  the perception loop; the squad command only acts as a steering hint
  while the higher-priority activities are inactive.
- **Vivox VOIP bolted into W22-23** — explicit roadmap deferral. Vivox
  requires a vendor license, plugin install, and platform-side OSS
  changes that don't belong in the same PR as the gameplay subsystem.
  W22-23 ships the **channel + listener + console hooks**; voice lives
  in a follow-up week.
- **UMG radial wheel in this PR** — UI work goes through a separate
  artist + UX cycle. The C++ command channel + console commands let
  designers test behaviour today; the widget lands in W22+ polish.

## W24-25 — Extraction flow (full raid loop)

`Documents/PALDARK/03-Roadmap_1_Year.md` § Tuần 24–25 outcome: *"Full
raid loop functional (drop → fight → extract)."* This section is the
match-orchestration layer that ties W7-8 GAS + W14-15 dedicated server
+ W18-19 Pal combat + W22-23 squad system into a real raid cycle with
phases, per-player outcomes, and an extraction beacon.

### What ships

| C++ class | Purpose |
|-----------|---------|
| `EPaldarkMatchPhase` | 4-state FSM: `Warmup → Active → Extracting → Ended`. Monotonic (Warmup is never re-entered). |
| `EPaldarkPlayerOutcome` | Per-player terminal flag: `Alive / Extracted / KIA / Disconnected`. Terminal once set; never reverts. |
| `EPaldarkMatchEndReason` | `None / AllExtracted / TeamWipe / Timeout / ForcedByConsole`. Replicated on `APaldarkGameStateBase`. |
| `FPaldarkMatchPlayerRow` | One row per registered player on the host: weak `APlayerState*` + outcome + extraction progress + per-event timestamps. |
| `UPaldarkMatchSubsystem` | `UWorldSubsystem` (server-only) that owns the FSM, the per-player registry, the warmup + hard-timeout timers, and the 3 broadcast delegates. Mirrors the W20-21 / W22-23 subsystem pattern. |
| `APaldarkExtractionBeacon` | Replicated `AActor` with a `USphereComponent` root. Designer places it in the level; players overlap → progress ramps to 1.0 over `ExtractionDuration` seconds → subsystem flips outcome to `Extracted`. |
| `APaldarkPlayerState` | Adds `MatchOutcome` + `ExtractionProgress` REPNOTIFY properties + `FPaldarkOnPlayerMatchOutcomeChanged` delegate. |
| `APaldarkGameStateBase` | Adds `MatchPhase` + `MatchEndReason` + `PhaseEndServerTime` REPNOTIFY properties + `FPaldarkOnMatchPhaseReplicated` delegate. Subsystem mirrors via `SetMatchPhaseFromSubsystem`. |
| `APaldarkGameMode_Extraction` | Concrete `APaldarkGameModeBase` subclass that ferries `PostLogin / Logout` into `RegisterPlayer / RecordDisconnect` and binds `OnMatchEnded` for logging. |

8 new gameplay tags:

- `Paldark.Match.Phase.{Warmup,Active,Extracting,Ended}` — broadcast
  on phase change; HUD widgets and AI activities branch on them.
- `Paldark.Match.Outcome.{Extracted,KIA,Disconnected}` — stamped on
  PlayerState when the outcome flips. (`Alive` has no tag — absence is
  the signal.)
- `Paldark.InputTag.InteractExtraction` — designer binds **E** (or any
  IA) so a player can explicitly consent to extract when a beacon
  has `bRequireExplicitConsent=true`.

3 new console commands (host-side):

- `Paldark.Match.Dump` — dump phase + per-player table to
  `LogPaldarkPlayer`.
- `Paldark.Match.Phase <Warmup|Active|Extracting|Ended>` — force the
  FSM forward. Warmup re-entry is intentionally rejected (the FSM is
  monotonic).
- `Paldark.Match.ForceEnd [reason]` — end the match immediately. Reason
  defaults to `ForcedByConsole`; accepts `AllExtracted / TeamWipe /
  Timeout` for QA scenario authoring.

### Phase FSM

```
                +--------+   timer expired OR all players joined
                | Warmup |--------------------------------------+
                +--------+                                       |
                                                                v
                +--------+   first RecordExtraction        +-----------+
                | Active |-------------------------------->| Extracting|
                +--------+                                  +-----------+
                    |  EvaluateEndCondition()                   |
                    +----------+-----------------+--------------+
                               |                 |              |
                               v                 v              v
                          AllExtracted       TeamWipe       Timeout
                               \___________________________/
                                            |
                                            v
                                      +-------+
                                      | Ended |
                                      +-------+
```

- `Warmup` — match created, players are still joining. Beacons reject
  overlap (returns no-op in `Tick`). The watchdog `WarmupTimerHandle`
  auto-advances to `Active` after `WarmupDuration` (default 30 s).
- `Active` — full raid. Players fight, loot, head for a beacon.
- `Extracting` — bookkeeping flag: set on the first `RecordExtraction`
  so HUD can show an "Extraction in progress" banner. Does not gate any
  logic.
- `Ended` — terminal. Subsystem rejects further `RecordExtraction /
  RecordDeath / RecordDisconnect`. `MatchEndReason` is the final
  verdict; client HUD reads it via `OnRep_MatchEndReason`.

### Per-player outcome ladder

The subsystem owns `PlayerRows` (a `TMap<TObjectPtr<APlayerState>,
FPaldarkMatchPlayerRow>`). Each row tracks:

- `Outcome` — `Alive` (default) → terminal `Extracted / KIA /
  Disconnected`. **Terminal once set; never reverts.**
- `ExtractionProgressNormalized` — 0..1 cached for replay to clients
  via `APaldarkPlayerState::SetExtractionProgress`.
- `RegisteredAt / DiedAt / ExtractedAt / DisconnectedAt` — server time
  for forensic dumps.

End conditions:

1. **AllExtracted** — every registered player is `Extracted`.
2. **TeamWipe** — every player is `KIA` or `Disconnected` (no `Alive`,
   no `Extracted`).
3. **Timeout** — server clock exceeds `MatchStartedAtServerTime +
   MaxMatchDuration` (default 600 s).
4. **ForcedByConsole** — `Paldark.Match.ForceEnd` triggered.

Evaluation runs after every state mutation; the subsystem flips to
`Ended` and broadcasts `OnMatchEnded(Reason)` exactly once.

### Extraction beacon

```
APaldarkExtractionBeacon (replicated)
  ├── USphereComponent  ExtractionTrigger (root, 400 cm default)
  ├── float             TriggerRadius            = 400.f
  ├── float             ExtractionDuration       = 5.f
  ├── bool              bRequireExplicitConsent  = false
  ├── bool              bDecayOnLeave            = true
  ├── float             WireUpdateInterval       = 0.25f
  └── TMap<APlayerState*, FProgressEntry> ProgressByPlayer
```

Per-frame loop (server-only):

1. Iterate `ProgressByPlayer` — for each weak `APlayerState` ref:
   - Skip if the row is stale (PS GC'd) or the player has a terminal
     outcome (`Extracted / KIA / Disconnected`).
   - Skip if `ShouldAcceptOverlap(PhaseTag, OutcomeTag)` rejects
     (subsystem in `Warmup` / `Ended`).
2. If the player is still in `OverlappingPlayerStates` **and** consent
   passes — advance progress by `DeltaSeconds / ExtractionDuration`.
3. If they've left and `bDecayOnLeave` is true — drain at the same
   rate.
4. Throttled replication: only push `SetExtractionProgress(...)` to the
   PlayerState every `WireUpdateInterval` seconds (default 0.25 s) so
   we don't spam DOREPLIFETIME at tick rate.
5. When progress ≥ 1.0 — call `MatchSub->RecordExtraction(PS)` and
   remove the row from the map.

`bRequireExplicitConsent` lets the designer flip the beacon from
*"passive overlap = extract"* to *"E to confirm"*. When true, players
must call `RegisterConsentToExtract(PS)` (wired from the
`Paldark.InputTag.InteractExtraction` input action; W24-25 ships the
tag + the wiring slot but not a default IMC binding — designer adds it
per IMC).

### Death hook

Player death routes through W7-8 GAS without a separate code path:

```
GE_Damage_Standard applies → AttributeSet::Health drops to 0
   → UPaldarkAttributeSet::OnHealthZeroed.Broadcast(ASC)
      → APaldarkCharacter::InitAbilitySystem-bound weak lambda
         → MatchSub->RecordDeath(PlayerState)
            → Outcome flips Alive → KIA
            → EvaluateEndCondition()
```

The lambda is **server-only**, bound exactly once per character (the
`MatchDeathHookHandle` valid-guard), and captures a `TWeakObjectPtr<
APaldarkPlayerState>` so a stale PS is a silent no-op (e.g. seamless
travel between maps).

### Authoring order (Editor side)

1. Open `Raid_Sandbox` map (the W1 day 8-10 experience map).
2. **Wire the extraction GameMode** (W26 L-01 closure — three layers,
   pick any one; all three together form a safety net):
   - **(a) Map World Override** — `Raid_Sandbox → World Settings →
     GameMode Override = APaldarkGameMode_Extraction`. Per-map, sticks
     across PIE / dedicated.
   - **(b) URL flag** — `open Raid_Sandbox?game=Extraction?Experience=PaldarkExperience.PX_RaidSandbox`.
     The `Extraction` alias resolves through the
     `+GameModeClassAliases=(Name="Extraction",GameMode=...)` entry
     shipped in `DefaultEngine.ini` W26. Cheapest fix when a designer
     forgets (a).
   - **(c) Experience assertion** — set
     `PX_RaidSandbox.RequiredGameModeClass = APaldarkGameMode_Extraction`
     on the experience asset. This does NOT swap the GameMode class
     itself (UE locks that at `InitGame`), but
     `APaldarkGameModeBase::OnExperienceAssetReady` checks the value
     and logs `LogPaldark: Error` + a red on-screen
     `AddOnScreenDebugMessage` if the spawned GameMode is not a
     subclass — so a designer who forgets (a) sees a loud red banner
     on the first PIE frame instead of a silent-Warmup match.
   - The validator (`scripts/ci/validate_paldarklab.py
     → check_extraction_gamemode_wired`) enforces (b) + (c) are
     present in source; (a) is per-map asset and lives in editor land.
3. Drop a `BP_ExtractionBeacon` (subclass of
   `APaldarkExtractionBeacon`) in the level. Tune:
   - `TriggerRadius` (default 400 cm — recommended 250-600 cm).
   - `ExtractionDuration` (default 5 s — recommended 3-10 s).
   - `bRequireExplicitConsent` (default false — flip true if you want
     E-to-confirm).
   - `bDecayOnLeave` (default true — flip false if you want
     non-decaying progress while a player stays in the trigger).
4. (Optional) author a UMG widget bound to
   `APaldarkPlayerState::OnMatchOutcomeChanged` +
   `APaldarkGameStateBase::OnMatchPhaseReplicated` for the end-of-
   match screen. The C++ side ships the delegates; W22+ polish covers
   the widget.

### Playtest loop (4 player, listen-server)

1. Spin up the W14-15 4-player dedicated test (or 4 PIE clients).
2. Spawn the host. `Paldark.Match.Dump` should show `Phase=Warmup,
   Players=1, Alive=1`.
3. Connect 3 more players. After `WarmupDuration` (default 30 s) the
   subsystem flips to `Active` automatically — verify via repeat dump.
   (Or use `Paldark.Match.Phase Active` to skip the timer.)
4. Spawn a hostile dummy or Pal — `Paldark.Combat.SpawnDummy` (W9-10)
   — shoot a teammate dead. `Paldark.Match.Dump` should show
   `Outcome=KIA` for the dead player and the subsystem should NOT
   transition Ended yet (other players still `Alive`).
5. Have 2 surviving players walk onto the extraction beacon. Hold for
   5 s. `Paldark.Match.Dump` mid-extract should show `Progress=0.6` or
   similar. On completion: `Outcome=Extracted`.
6. The last `Alive` player either extracts (→ `AllExtracted`) or dies
   (→ `TeamWipe`). Subsystem transitions `Active → Ended`, broadcasts
   `OnMatchEnded(reason)`, logs the table.
7. `Paldark.Match.Dump` post-end should show `Phase=Ended,
   Reason=AllExtracted, Players=4, Extracted=N, KIA=M`.

### Bandwidth + perf notes

- The 5 REPNOTIFY scalar properties (3 on GameState + 2 on PlayerState)
  cost <1 byte/property/replicate. End of match writes them once;
  ExtractionProgress is throttled to 4 updates/s/beacon.
- The match subsystem is **server-only** — `ShouldCreateSubsystem`
  returns false on clients. Clients read mirrored state via the
  REPNOTIFY props on `APaldarkGameStateBase` + `APaldarkPlayerState`.
- The beacon Tick runs at world tick rate but each iteration is a
  small TMap traversal (≤ 4 players, ≤ N beacons). For Q2's 4-player
  test target this is < 0.05 ms/frame.

### Anti-patterns avoided (W24-25)

- **Subsystem on clients** — would double-process every event. Fix:
  `ShouldCreateSubsystem` returns false off-server; clients read
  REPNOTIFY props on GameState / PlayerState.
- **Replicating the per-player TMap directly** — `TMap<TObjectPtr<APlayerState>,
  FPaldarkMatchPlayerRow>` is not replication-friendly and doubles the
  authoritative state. Fix: TMap stays server-only; per-player wire
  state lives on the PlayerState (which is already replicated).
- **REPNOTIFY ExtractionProgress at tick rate** — would dominate the
  PlayerState's bandwidth budget. Fix: beacon throttles via
  `WireUpdateInterval` (default 0.25 s).
- **Re-entry to Warmup via console** — would allow a designer to
  rewind the FSM and confuse the per-player outcome ladder. Fix:
  `Paldark.Match.Phase Warmup` is explicitly rejected with a warning
  log; the subsystem is monotonic.
- **Letting a beacon force the phase to Extracting** — would race the
  end-condition evaluator. Fix: `RecordExtraction` sets the player's
  outcome; `EvaluateEndCondition` decides whether to end the match;
  `Extracting` is a banner-only phase set by the *first* extraction
  recorded.
- **Reusing the W22-23 squad subsystem's broadcast for match events** —
  would couple two unrelated subsystems. Fix: match subsystem owns
  its own 3 delegates (`OnMatchPhaseChanged / OnMatchEnded /
  OnPlayerOutcomeChanged`); squad subsystem is untouched.
- **UMG end-of-match widget in this PR** — UI work goes through a
  separate artist + UX cycle. C++ exposes the delegates the widget
  binds to; the widget lands in W22+ polish.
- **Lobby travel in this PR** — `ServerTravel("Lobby")` requires a
  hub-town map, lobby UI, and matchmaking glue. Explicit roadmap
  deferral to W40+. The W24-25 GameMode logs the end reason and stops;
  designer reloads the level manually.
- **Persistent match stats** — DynamoDB write-back is W42-43 (AWS).
  W24-25 keeps results in-memory on the subsystem; world teardown
  wipes them.
- **Reconnect mid-match** — a disconnected player's outcome is
  `Disconnected` (terminal). Re-joining the same match is intentionally
  not supported; W14-15 follow-up adds reconnect.

## AnimBP Lyra ALS port (W27)

W27 Path A đóng **code-side** cho hai mục P0 trong bug-bash:
- Q1 § K-01 (AnimBP T-pose risk — Path B mannequin chỉ duct-tape).
- Q2 § L-06 (AnimBP T-pose risk — Q1 K-01 carry).

PR này chỉ ship C++ scaffold. Việc author `.uasset` (AnimBP, State
Machine, Blend Space, Layer functions) là designer task trong UE Editor.

### Cấu trúc C++

| File | Class / Interface | Vai trò |
|------|-------------------|---------|
| `Public/Anim/PaldarkAnimInstance.h` + matching `.cpp` | `UPaldarkAnimInstance : UAnimInstance` (abstract) | Base AnimInstance theo pattern Lyra. 7 BlueprintReadOnly fields (`Speed`, `Direction`, `LeanAngle`, `bIsInAir`, `bIsAccelerating`, `bIsCrouching`, `bIsSprinting`, `bShouldMove`). `NativeUpdateAnimation` snapshot game-thread; `NativeThreadSafeUpdateAnimation` derive worker-thread. |
| `Public/Anim/PaldarkPalAnimInstance.h` + matching `.cpp` | `UPaldarkPalAnimInstance : UPaldarkAnimInstance` | Pal subclass. Thêm 4 fields: `bIsHostile` (đọc `TeamTag`), `bIsFlying` (đọc `MOVE_Flying`), `bIsInCombat` (đọc activity tag = `Paldark.Pal.Activity.Combat`), `CurrentActivityTag` (mirror full enum). |
| `Public/Anim/PaldarkAnimLayerInterface.h` (header-only, UINTERFACE) | `IPaldarkAnimLayerInterface` | 3 `BlueprintImplementableEvent`: `Layer_LocomotionIdle / Move / Jump`. Designer-authored AnimBP override per-character. |
| `Public/Experience/PaldarkPawnData.h` | `UPaldarkPawnData::DefaultAnimInstanceClass` | `TSoftClassPtr<UPaldarkAnimInstance>` — chọn AnimBP per-experience. |
| `Private/Player/PaldarkCharacter.cpp` (`SetPawnData`) | — | `LoadSynchronous` + `Mesh->SetAnimInstanceClass(...)` (idempotent; bỏ qua nếu soft pointer null). |

### Thread-safety pattern (Lyra parity)

```
NativeInitializeAnimation()       // GT — cache ACharacter, ASC, tags
        ↓
NativeUpdateAnimation(DT)         // GT — write FPaldarkAnimSnapshot (primitive-only)
        ↓
NativeThreadSafeUpdateAnimation(DT)  // Worker thread — read snapshot, derive
                                     // Speed / Direction / bShouldMove / Lean ...
        ↓
AnimGraph eval                       // Worker thread — read BlueprintReadOnly fields
```

`FPaldarkAnimSnapshot` chỉ chứa primitive + `FGameplayTag`. **KHÔNG** thêm
`UObject*` vào struct này — worker thread không được dereference UObject.
`bUseMultiThreadedAnimationUpdate = true` đã bật trong constructor.

### Authoring order (designer, sau khi PR merge + Generate Project Files + compile)

1. **Compile project** trong UE Editor. Verify `UPaldarkAnimInstance`,
   `UPaldarkPalAnimInstance`, `UPaldarkAnimLayerInterface` xuất hiện trong
   Content Browser "Pick Parent Class" dialog.
2. **Create `ABP_Paldark_Player`** (`Content/Paldark/Anim/`) inherited from
   `UPaldarkAnimInstance`. AnimGraph: State Machine 3 state
   (Idle / Locomotion / InAir):
   - Idle ↔ Locomotion: transition khi `bShouldMove` = true (vào) / false (ra).
   - Locomotion → InAir: `bIsInAir` = true. InAir → Idle: `bIsInAir` = false.
   - Locomotion Blend Space: `X = Direction (-180..180)`, `Y = Speed (0..600)`.
3. **Implement `IPaldarkAnimLayerInterface`** trên `ABP_Paldark_Player` (Class
   Settings → Interfaces). Override `Layer_LocomotionIdle / Move / Jump`
   để return per-character pose montages (hoặc inherit base nếu chưa author).
4. **Create `ABP_Paldark_Pal`** (`Content/Paldark/Anim/`) inherited from
   `UPaldarkPalAnimInstance`. AnimGraph: thêm 2 nhánh ngoài Player State
   Machine:
   - `bIsFlying` = true → Razorbird flap Blend Space (Speed-keyed).
   - `bIsInCombat` = true → Combat additive (đè Locomotion).
5. **Subclass `ABP_Paldark_Pal`** → `ABP_Paldark_Hostile_Direhound` /
   `ABP_Paldark_Hostile_Razorbird`. Override `Layer_LocomotionMove` để swap
   stride length / paw cycle / wing flap.
6. **Wire `UPaldarkPawnData::DefaultAnimInstanceClass`** trên mỗi pawn-data
   asset (e.g. `DA_PawnData_Player.DefaultAnimInstanceClass = ABP_Paldark_Player`).
   Alternative: gán trực tiếp trên `BP_PaldarkCharacter.Mesh.AnimClass` —
   PawnData wiring là override path cho per-experience swap, không bắt buộc.

### Test loop (designer, sau bước 1-6)

1. `open Raid_Sandbox` → spawn player → kiểm tra pawn **KHÔNG** T-pose.
2. WASD → kiểm tra Blend Space chạy walk/run cycle theo Speed.
3. Hold LShift (sprint) → kiểm tra `bIsSprinting` = true → AnimBP có thể
   bật run animation hoặc additive.
4. Jump → kiểm tra State Machine transition Idle → InAir → Idle.
5. `Paldark.Pal.SpawnTestCompanion` → Pal spawn, kiểm tra Pal AnimBP chạy
   Follow locomotion.
6. `Paldark.Pal.SpawnHostileDummy` → hostile-Pal aggro → `bIsInCombat` = true
   → Combat additive bật.

### Out of scope (defer)

- **MotionWarping** plugin integration → Q4 (W42-43+).
- **Distance Matching** (foot-locked stride) → Q4 polish.
- **Aim Offset** (look pitch/yaw blend) → khi có Aim weapon (W30+).
- **Procedural foot IK** (Control Rig) → polish pass, không phải basic ALS.

### Anti-patterns đã tránh (W27)

- `NativeThreadSafeUpdateAnimation` dereference `Character->GetCharacterMovement()`
  → race với physics tick → crash hoặc garbage values. Fix: snapshot
  trong `NativeUpdateAnimation` (GT) + worker chỉ đọc `Snapshot.Velocity`.
- AnimInstance subscribe gameplay tags qua delegate → tick subscriber không
  thread-safe. Fix: cache `UPaldarkAbilitySystemComponent*` trong
  `NativeInitializeAnimation`, query `HasMatchingGameplayTag` từ GT only.
- BlueprintReadOnly fields được set từ BP → race với worker thread eval. Fix:
  fields là `Transient` + comment cấm BP write; chỉ C++ thread-safe pass write.
- Adding `UObject*` field vào `FPaldarkAnimSnapshot` → worker dereference
  UObject. Fix: snapshot chứa primitive + `FGameplayTag` only, validator
  enforce struct tồn tại bằng tên (designer/maintainer review).

## AssetManager + Async load + Game Feature plugin (W27-28)

W27-28 ship the **content-streaming foundation** for the Q3 roadmap:

- Mỗi Pal species là một `UPaldarkPalDefinition` (UPrimaryDataAsset) chứa soft
  refs tới mesh / anim instance / abilities (group bundle "Spawn").
- `UPaldarkPalSpawnSubsystem` (UWorldSubsystem, server-only) async-load
  bundle qua `FStreamableManager` + spawn Pal khi resolve. Pre-warm cache
  giữ handle alive trong subsystem (không GC).
- Experience opt-in danh sách Pal cần pre-warm qua
  `UPaldarkExperienceDefinition::PreWarmPalDefinitions`. Game mode fan-out
  từ `OnExperienceAssetReady` (fire-and-forget — không block PostLogin).
- `Plugins/PaldarkRaidContent/` là **game-feature plugin scaffold** —
  raid-specific content (map, Pal definitions, ability sets) ship streamable
  thay vì root-cooked. C++ chỉ ship module skeleton + `.uplugin`; designer
  author `GameFeatureData_RaidContent.uasset` trong Editor.

Outcome đúng roadmap: **map load không hitch, Pal async pre-warm**.

### Cấu trúc C++

| File | Class | Vai trò |
|------|-------|---------|
| `Public/Pal/PaldarkPalDefinition.h` + matching `.cpp` | `UPaldarkPalDefinition : UPrimaryDataAsset` | Pal species data asset. Identity (`SpeciesTag`, `DisplayName`) + soft refs (`PalClass`, `SkeletalMesh`, `AnimInstanceClass`, `GrantedAbilities`) trong bundle "Spawn" + eager stats (`MaxHealth`, `MoveSpeedBase`). `GetPrimaryAssetId()` → `PaldarkPalDefinition:<RowName>`. |
| `Public/Pal/PaldarkPalSpawnSubsystem.h` + matching `.cpp` | `UPaldarkPalSpawnSubsystem : UWorldSubsystem` | Async load + spawn pipeline. 3 public surfaces: `RequestPreWarmAsync` (idempotent, coalesce in-flight), `SpawnPalAsync` (auto pre-warm khi chưa warm), `IsPreWarmed` (cheap predicate). Cache `TMap<FPrimaryAssetId, FPaldarkPalPreWarmState>` giữ `TSharedPtr<FStreamableHandle>` alive. Server-only. |
| `Public/Experience/PaldarkExperienceDefinition.h` (+1 field) | — | `TArray<TSoftObjectPtr<UPaldarkPalDefinition>> PreWarmPalDefinitions` — designer opt-in. |
| `Private/Framework/PaldarkGameModeBase.cpp` (+1 method) | `APaldarkGameModeBase::RequestPalDefinitionPreWarm` | Gọi từ `OnExperienceAssetReady` để fan-out pre-warm theo experience. |
| `Plugins/PaldarkRaidContent/PaldarkRaidContent.uplugin` | — | Game-feature plugin manifest. `IsGameFeaturePlugin: true`, depends on `GameFeatures` + `ModularGameplay`. `EnabledByDefault: false` (host project flip on khi designer author asset). |
| `Plugins/PaldarkRaidContent/Source/PaldarkRaidContent/` | `FPaldarkRaidContentModule` | Module skeleton (Build.cs + .h + .cpp với `IMPLEMENT_MODULE`). |
| `Plugins/PaldarkRaidContent/Content/GameFeatures/` | — | Placeholder directory cho `GameFeatureData_RaidContent.uasset` (designer task). |

### Lifecycle pipeline (server)

```
APaldarkGameModeBase::InitGame
        ↓
UAssetManager::LoadPrimaryAsset(experience)  ← async
        ↓
APaldarkGameModeBase::OnExperienceAssetReady
        ↓                                       ↓
ApplyExperienceClassOverrides            RequestPalDefinitionPreWarm
ApplyExperienceActionSets                       ↓
        ↓                                       UPaldarkPalSpawnSubsystem::RequestPreWarmAsync (fan-out)
        ↓                                              ↓
PostLogin → spawn pawn                           UAssetManager::LoadPrimaryAsset(PalDef, ["Spawn"])  ← async, parallel
                                                       ↓
                                                 HandleLoadComplete → mark warm, fire pending callbacks
                                                       ↓
[anytime later]                                  Paldark.Pal.SpawnFromDefinition <DefId>
                                                       ↓
                                                 UPaldarkPalSpawnSubsystem::SpawnPalAsync
                                                       ↓ (hit warm cache, fast path)
                                                 SpawnFromWarmedDefinition → SpawnActor + SetSkeletalMesh + SetAnimInstanceClass + GiveAbility
```

`SpawnPalAsync` **không gate** pre-warm — nếu DefId chưa warm, subsystem
chain pre-warm xong rồi spawn. Pre-warm chỉ là tối ưu hit-cache (tránh stall
trong combat).

### Console commands (W27-28)

| Command | Mục đích |
|---------|---------|
| `Paldark.Pal.SpawnFromDefinition <DefId> [X Y Z]` | Async-load + spawn Pal từ primary asset id. `<DefId>` chấp nhận cả `PaldarkPalDefinition:DA_PalDef_Direhound` lẫn `DA_PalDef_Direhound` (bare row). Mặc định spawn 4m trước local player. |
| `Paldark.Pal.DumpDefinitionRegistry` | List mọi DefId subsystem đang track + state (`Resolved` / `InFlight`) + species/class/mesh/anim paths. |

### Authoring order (designer, sau khi PR merge + Generate Project Files + compile)

1. **Compile Editor target**. Verify `UPaldarkPalDefinition` xuất hiện
   trong "Pick Parent Class" dialog (Pal category) + Plugins UI có entry
   "Paldark Raid Content".
2. **Author Pal definitions** trong `Content/Paldark/Pals/`:
   - Right-click → Miscellaneous → Data Asset → `PaldarkPalDefinition`.
   - Save as `DA_PalDef_Direhound`, set `SpeciesTag = Paldark.Pal.Species.Direhound`.
   - Set soft refs: `PalClass = BP_Pal_Direhound`,
     `SkeletalMesh = SK_Direhound`, `AnimInstanceClass = ABP_Paldark_Pal_Direhound`,
     `GrantedAbilities = [BP_GA_PalAttack]`.
   - Set eager stats: `MaxHealth = 80`, `MoveSpeedBase = 700`.
   - Repeat cho `DA_PalDef_Razorbird`.
3. **Wire experience pre-warm** — open `PX_RaidSandbox`, set
   `PreWarmPalDefinitions = [DA_PalDef_Direhound, DA_PalDef_Razorbird]`.
4. **(Optional) Game-feature plugin asset**:
   - Plugins → Game Features → Paldark Raid Content → Create Game Feature
     Data → `GameFeatureData_RaidContent.uasset` in
     `Plugins/PaldarkRaidContent/Content/GameFeatures/`.
   - Set `InitialState = Active`, add Actions cần thiết
     (`UGameFeatureAction_AddComponents`, `_AddAbilities`, etc.).
   - Flip `PaldarkLab.uproject` → `PaldarkRaidContent.Enabled = true`.

### Test loop (designer, sau bước 1-3)

1. `open Raid_Sandbox` (PIE) — kiểm tra LogPaldarkPal có line
   "RequestPalDefinitionPreWarm — requested 2/2 Pal definition pre-warm(s)".
2. Console: `Paldark.Pal.DumpDefinitionRegistry` — verify cả 2 DefId hiện
   state `Resolved` trong vài giây sau khi map load.
3. Console: `Paldark.Pal.SpawnFromDefinition DA_PalDef_Direhound` — Pal
   spawn cách player 4m, đúng mesh + anim + abilities.
4. Console: `Paldark.Pal.SpawnFromDefinition DA_PalDef_Razorbird 0 0 500`
   — Razorbird spawn ở world location explicit.
5. (Stress test) Console:
   `Paldark.Pal.SpawnFromDefinition DA_PalDef_<not-pre-warmed-species>`
   — kiểm tra fallback path: subsystem pre-warm rồi spawn, không crash.

### Out of scope (defer)

- **GameFeatureData_RaidContent.uasset content** — designer task trong UE
  Editor. PR ship plugin scaffold + directory placeholder.
- **Cold spawn budget pacing** — limit số async load đồng thời (Q4
  optimisation pass).
- **Pre-warm chained predictions** — pre-warm "wave kế tiếp" dựa trên
  Activity Director state (W30+).
- **Persistent warm set** — giữ cache qua map travel via
  `UPaldarkAssetManager` (Q4).

### Anti-patterns đã tránh (W27-28)

- `FStreamableHandle` không giữ tham chiếu → loaded assets bị GC sau callback.
  Fix: `FPaldarkPalPreWarmState.Handle` (TSharedPtr) cache trong subsystem
  cho tới khi `ClearWarmCache()` hoặc `Deinitialize()`.
- Caller thứ 2 trùng DefId issue duplicate `LoadPrimaryAsset` → 2 handles
  cùng load assets. Fix: coalesce qua `WarmStates.Find(DefId)` → queue
  callback lên existing handle.
- Spawn subsystem chạy trên client → race với server replication. Fix:
  `OnWorldBeginPlay` log net mode; `RequestPreWarmAsync` / `SpawnPalAsync`
  early-return + log warning trên `NM_Client`.
- Game-feature plugin module phụ thuộc Editor → cooker fail. Fix: module
  `Type=Runtime`, không list Editor module.
- Game-feature plugin `EnabledByDefault: true` khi chưa có `.uasset` →
  startup spam warning. Fix: ship với `false`, designer flip on sau khi
  author asset.
- `SoftObjectPtr.LoadSynchronous()` trong spawn hot-path khi chưa pre-warm
  → frame hitch. Fix: pre-warm là pattern khuyên dùng; cold path vẫn
  fallback qua `SpawnPalAsync` chain (chấp nhận miss latency, không crash).

## W29-30 — 5 Pal loài còn lại (Stoneclad + Vinewraith + Foxparks + Boltmane + Tombat)

W29-30 đóng roadmap Q3 § Tuần 29-30 và đưa roster Pal lên 10 loài. Roadmap
yêu cầu 5 loài (Stoneclad, Vinewraith, Foxparks companion, Boltmane boss,
Tombat companion) cùng với AnimBP + ability per loài. PR này ship phần C++
scaffold + gameplay tag + validator + designer authoring order; AnimBP / ability
damage GE / `DA_PalDef_*` asset / boss montage là designer task trong UE
Editor.

### Roster shape (5 species mới)

| Species    | Vai trò          | Pattern                | Ability subclass                                  | Pack tag                           |
|------------|------------------|------------------------|---------------------------------------------------|------------------------------------|
| Stoneclad  | Hostile / tank   | C++ subclass           | `..._PalAttack_Charge` (heavy melee + knockback)  | `Paldark.Pal.Pack.Stoneclad`       |
| Vinewraith | Hostile / ranger | C++ subclass           | `..._PalAttack_VineWhip` (ranged 900cm)           | `Paldark.Pal.Pack.Vinewraith`      |
| Boltmane   | Hostile / boss   | C++ subclass + BossFSM | `..._PalAttack_ChainLightning` (2 bounces 600cm)  | (solo — no pack)                   |
| Foxparks   | Companion        | DataAsset (W27-28)     | `..._PalAttack_FireBreath` (cone 800cm × 30°)     | (companion — no pack)              |
| Tombat     | Companion        | DataAsset (W27-28)     | `..._PalAttack_Bite` (fast cooldown, low damage)  | (companion — no pack)              |

Tại sao 3 hostile có C++ subclass còn 2 companion không? Hostile cần stamp
`TeamTag = Paldark.Team.Hostile` + override `CandidateActivities` (replace,
không append — L-13) + register/unregister với
`UPaldarkHostilePackSubsystem` trong BeginPlay/EndPlay; những hành vi này
khó express qua data-only. Companion thì giữ `TeamTag = Paldark.Team.Player`
(default) + `CandidateActivities` (default) — chỉ cần đổi mesh / anim /
abilities, đúng use case của `UPaldarkPalDefinition` (W27-28).

### Cấu trúc C++ (W29-30)

| Class                                              | File                                                       | Vai trò |
|----------------------------------------------------|------------------------------------------------------------|---------|
| `UPaldarkGameplayAbility_PalAttack_Charge`         | `Gas/Abilities/PaldarkGameplayAbility_PalAttack_Charge.*`  | Stoneclad heavy melee + LaunchCharacter impulse trên target sau parent damage. |
| `UPaldarkGameplayAbility_PalAttack_VineWhip`       | `Gas/Abilities/..._VineWhip.*`                             | Vinewraith ranged (effective 900cm) — designer tune cooldown trong BP_GA. |
| `UPaldarkGameplayAbility_PalAttack_FireBreath`     | `Gas/Abilities/..._FireBreath.*`                           | Foxparks cone AoE (800cm × ±30°, 6 target cap). Query overlap + filter team tag + apply damage GE. |
| `UPaldarkGameplayAbility_PalAttack_ChainLightning` | `Gas/Abilities/..._ChainLightning.*`                       | Boltmane chain (2 bounces, 600cm range, 0.7 falloff) — bounce target dedupe để không hit lại cùng peer. |
| `UPaldarkGameplayAbility_PalAttack_Bite`           | `Gas/Abilities/..._Bite.*`                                 | Tombat companion fast-attack, recommended interval 0.4s. |
| `APaldarkPalCharacter_Stoneclad`                   | `Pal/PaldarkPalCharacter_Stoneclad.*`                      | Hostile tank — `MaxWalkSpeed=380`, combat 80-280cm, AttackInterval 2.0s, BasePalDamage 28. CandidateActivities = Idle/Patrol/Stalk/Combat. |
| `APaldarkPalCharacter_Vinewraith`                  | `Pal/PaldarkPalCharacter_Vinewraith.*`                     | Hostile ranger — `MaxWalkSpeed=520`, combat 400-900cm, AttackInterval 1.5s, BasePalDamage 14. CandidateActivities = Idle/Patrol/Stalk/Combat. |
| `APaldarkPalCharacter_Boltmane`                    | `Pal/PaldarkPalCharacter_Boltmane.*`                       | Boss — `MaxWalkSpeed=500`, combat 300-1100cm, AttackInterval 2.5s, BasePalDamage 18. CandidateActivities = Idle/Patrol/Stalk/**BossPhase** (REPLACE Combat, không Append). KHÔNG register pack. |
| `UPaldarkActivity_BossPhase`                       | `Pal/Activities/PaldarkActivity_BossPhase.*`               | P50 boss FSM — track `EPaldarkBossPhase` (Default → Enraged @ HP<50% → Telegraph @ HP<25%), broadcast `OnBossPhaseChanged` delegate cho HUD/AnimBP, apply MoveSpeed + AttackInterval modifier per phase. `ShouldContinue` always true (boss never revert). |

### Ability subclass pattern (W18-19 base + per-species ctor override)

Tất cả 5 ability subclass đều derive từ `UPaldarkGameplayAbility_PalAttack`
(W18-19) — giữ shared damage path (target resolve qua combat component,
apply damage GE với `SetByCaller.PalDamage`, server-only execution). Per
species chỉ override ctor cho:

- `DebugName` — tên log + console.
- `AbilityTags` — reset + add `Paldark.Ability.PalAttack` + species tag
  (e.g. `Paldark.Ability.PalAttack.Charge`).
- `FallbackBaseDamage` — designer overrideable qua BP_GA, fallback dùng
  khi `BasePalDamage` trên combat component = 0 (e.g. early prototype).
- Per-species knobs — `KnockbackImpulse` (Charge), `EffectiveRangeCM`
  (VineWhip), `ConeRangeCM` / `ConeHalfAngleDeg` / `MaxConeTargets`
  (FireBreath), `MaxBounces` / `BounceRangeCM` / `BounceFalloff`
  (ChainLightning), `RecommendedAttackInterval` (Bite).

Charge / FireBreath / ChainLightning có `ActivateAbility` override
để chain logic trên parent damage:
- **Charge**: gọi `Super::ActivateAbility(...)` trước (parent end ability
  + damage GE), sau đó `LaunchCharacter` impulse hướng target với
  `VerticalLiftRatio = 0.25` cho hop nhỏ.
- **FireBreath**: KHÔNG gọi parent (parent chỉ hit 1 target từ combat
  component). Tự query `OverlapMultiByObjectType` trong cone, filter
  bằng team tag, apply damage GE per hit, end ability thủ công.
- **ChainLightning**: hit primary target qua parent path; sau đó bounce
  thủ công — find nearest enemy trong `BounceRangeCM`, apply damage
  scaled bởi `BounceFalloff^N`, dedupe để không hit lại cùng peer.

### Hostile-Pal subclass pattern (W20-21 reuse)

3 hostile subclass mới (Stoneclad / Vinewraith / Boltmane) follow đúng
Direhound/Razorbird pattern (W20-21):

1. **Ctor** flip `TeamTag = Paldark.Team.Hostile` + stamp
   `SpeciesTag = Paldark.Pal.Species.<X>`, tune
   `Movement->MaxWalkSpeed`, set `PerceptionSlot->Friendly/HostileTeamTag`
   (Friendly = own hostile team, Hostile = player team), tune
   `CombatSlot->MinEngageRange / MaxEngageRange / AttackInterval / BasePalDamage`,
   **assign** `ActivitySlot->CandidateActivities = { ... }` (KHÔNG `.Add` /
   `.Append` — L-13 enforced qua validator).
2. **BeginPlay** (server only) — default `PackTag` nếu invalid + register
   với `UPaldarkHostilePackSubsystem`. Boltmane KHÔNG register (solo boss).
3. **EndPlay** (server only) — unregister từ pack subsystem.

Boltmane riêng:
- `CandidateActivities` chứa `UPaldarkActivity_BossPhase::StaticClass()`
  thay cho `UPaldarkActivity_Combat::StaticClass()`. Validator
  `check_w29_pal_roster_shape` enforce điều này.
- Boss FSM P50 preempt Combat P40 (xem Activity FSM priority ladder).
- Skip pack registration — boss luôn solo.

### Boss FSM activity (UPaldarkActivity_BossPhase)

```
Phase ladder (HP %):
   Default       → HP ≥ 50%               → base stats
   Enraged       → 25% ≤ HP < 50%         → MoveSpeed +30%, AttackInterval -25%
   Telegraph     → HP < 25%               → MoveSpeed +30%, AttackInterval -50%
                                           + Broadcast OnBossPhaseChanged
                                             (designer: trigger telegraph
                                             VFX + montage 1s trước
                                             ChainLightning ult)
```

Implementation notes:
- `Priority = 50` (set trong ctor) — higher hơn Combat P40, nên Boltmane
  enter BossPhase ngay khi perception detect hostile target (`CanRun`
  identical với Combat — có target + alive + in range).
- `ShouldContinue` always `true` once entered — boss không revert về Idle
  / Patrol; chỉ exit khi boss chết hoặc target invalid hoàn toàn.
- `TickActivity` poll HP % mỗi frame qua `GetPaldarkAttributeSet()` →
  switch phase + broadcast delegate khi vượt threshold.
- Phase modifiers áp dụng qua direct mutation
  (`CharacterMovement->MaxWalkSpeed`, `CombatComponent->AttackInterval`)
  + restore trong `ExitActivity`.
- `OnBossPhaseChanged` delegate signature:
  `(EPaldarkBossPhase OldPhase, EPaldarkBossPhase NewPhase)` — designer
  bind trong AnimBP / HUD để trigger banner + montage cue + camera shake.

### Activity FSM priority ladder (post-W29-30)

| Priority | Activity                            | Áp dụng cho                              |
|----------|-------------------------------------|------------------------------------------|
| 50       | `UPaldarkActivity_BossPhase`        | Boltmane only (replace Combat)           |
| 40       | `UPaldarkActivity_Combat`           | Companion + non-boss hostile             |
| 30       | `UPaldarkActivity_Investigate`      | Companion + non-boss hostile             |
| 25       | `UPaldarkActivity_Stalk`            | Hostile-only (Direhound/Razorbird/Stoneclad/Vinewraith) |
| 22       | `UPaldarkActivity_FollowSquadCommand` | Companion only (W22-23)                |
| 20       | `UPaldarkActivity_Follow`           | Companion only                           |
| 15       | `UPaldarkActivity_Patrol`           | Hostile-only                             |
| 10       | `UPaldarkActivity_Idle`             | Both                                     |

### 14 gameplay tags mới

```
Paldark.Pal.Species.Stoneclad        Paldark.Pal.Species.Vinewraith
Paldark.Pal.Species.Foxparks         Paldark.Pal.Species.Boltmane
Paldark.Pal.Species.Tombat           Paldark.Pal.Pack.Stoneclad
Paldark.Pal.Pack.Vinewraith          Paldark.Pal.Activity.BossPhase
Paldark.Ability.PalAttack.Bite       Paldark.Ability.PalAttack.VineWhip
Paldark.Ability.PalAttack.Charge     Paldark.Ability.PalAttack.FireBreath
Paldark.Ability.PalAttack.ChainLightning
```

(13 declares — boss phase enum dùng `EPaldarkBossPhase` thay vì tag vì
phases mutually exclusive ordinal, không cần tag query.)

### Authoring order (designer, sau khi PR merge + Generate Project Files + compile)

**Bước 1 — Hostile-Pal subclass BP (3 BP, 1 per subclass)**:
1. Content Browser → Pal/Hostile/ → New BP → Parent
   `APaldarkPalCharacter_Stoneclad` (hoặc `_Vinewraith` / `_Boltmane`).
2. Save as `BP_Pal_Stoneclad_Default`, `BP_Pal_Vinewraith_Default`,
   `BP_Pal_Boltmane_Default`.
3. Set Mesh + AnimBP per loài (mesh từ asset team, ABP từ W27 ALS port).
4. (Optional) Override ctor knobs trong Class Defaults nếu cần
   per-instance tuning.

**Bước 2 — Ability BP (5 BP, 1 per ability subclass)**:
1. Content Browser → Gas/Abilities/ → New BP → Parent
   `UPaldarkGameplayAbility_PalAttack_Charge` (hoặc tương ứng).
2. Save as `BP_GA_PalAttack_Charge`, `BP_GA_PalAttack_VineWhip`,
   `BP_GA_PalAttack_FireBreath`, `BP_GA_PalAttack_ChainLightning`,
   `BP_GA_PalAttack_Bite`.
3. Override `Cost GE` (e.g. `GE_Cost_PalAttack`) + `Cooldown GE`
   (e.g. `GE_Cooldown_PalAttack_Charge`). Cooldown rec:
   - Charge: 4.0s (heavy)
   - VineWhip: 2.5s (ranged kite)
   - FireBreath: 6.0s (AoE big)
   - ChainLightning: 8.0s (ult-tier)
   - Bite: 0.4s (companion spam)
4. Trên Stoneclad / Vinewraith / Boltmane BP — add ability vào
   `CombatSlot->DefaultPalAbilities` array (W18-19 grant path).

**Bước 3 — DataAsset cho companion (Foxparks / Tombat — W27-28 path)**:
1. Content Browser → Pal/Definitions/ → New DA → `UPaldarkPalDefinition`.
2. Save as `DA_PalDef_Foxparks`, `DA_PalDef_Tombat`.
3. Set:
   - `SpeciesTag = Paldark.Pal.Species.Foxparks` (hoặc Tombat).
   - `PalClass = BP_Pal_Companion_Default` (W3-4 default companion BP,
     KHÔNG cần subclass mới).
   - `SkeletalMesh = SK_Foxparks` (hoặc Tombat).
   - `AnimInstanceClass = ABP_Paldark_Pal_Foxparks` (hoặc Tombat — W27
     ALS port subclass).
   - `GrantedAbilities = [BP_GA_PalAttack_FireBreath]` (Foxparks) hoặc
     `[BP_GA_PalAttack_Bite]` (Tombat).
   - Eager stats: `MaxHealth`, `MoveSpeedBase`.

**Bước 4 — Wire experience pre-warm (W27-28 path)**:
- Open `PX_RaidSandbox` → `PreWarmPalDefinitions = [DA_PalDef_Direhound,
  DA_PalDef_Razorbird, DA_PalDef_Foxparks, DA_PalDef_Tombat]`.
- Stoneclad / Vinewraith / Boltmane KHÔNG cần pre-warm (subclass thẳng,
  hostile pack spawner load class direct).

**Bước 5 — Hostile pack spawner setup**:
1. Open `Raid_Sandbox.umap` → Place Actor → `APaldarkHostilePalSpawner`.
2. Per spawner: set `PalClassToSpawn = BP_Pal_Stoneclad_Default`,
   `SpawnCount = 4`, `WaypointActors = [TargetPoint x3]`,
   `PackTag = Paldark.Pal.Pack.Stoneclad`.
3. Repeat cho Vinewraith pack.
4. Boltmane: dùng spawner riêng với `SpawnCount = 1` + KHÔNG set PackTag
   (boss solo).

### Test loop (designer, sau bước 1-5)

1. Compile project → open `Raid_Sandbox` PIE.
2. Console:
   - `Paldark.Pal.SpawnDirehoundPack` (sanity check W20-21 vẫn xanh).
   - `Paldark.Pal.SpawnFromDefinition DA_PalDef_Foxparks` (Foxparks
     companion spawn cách player 4m, đúng mesh + ability).
   - Walk Stoneclad spawn pack → engage Stoneclad → kiểm tra
     `LogPaldarkPal`: BasePalDamage 28 + AttackInterval 2.0s + knockback
     impulse trên player khi GA_PalAttack_Charge activate.
3. Vinewraith pack — kiểm tra Pal kite (combat range 400-900cm) và
   GA_PalAttack_VineWhip damage 14 không melee miss.
4. **Boltmane boss test** (cốt lõi W29-30):
   - Spawn Boltmane → engage.
   - Kiểm tra `LogPaldarkPal`: activity selected = `BossPhase` (P50)
     thay vì `Combat` (P40).
   - Bắn Boltmane xuống HP 49% → kiểm tra log:
     "OnBossPhaseChanged: Default → Enraged", MoveSpeed bump 500 → 650,
     AttackInterval drop 2.5s → 1.875s.
   - Bắn xuống HP 24% → "Default... → Telegraph", AttackInterval drop
     thêm xuống 1.25s. Designer thấy delegate fire qua HUD widget binding
     (sẽ author Q3 polish).
5. (Optional) Record 30-60s PIE video làm artefact:
   `Documents/Devlog/Q3-W29-Roster-Demo.mp4`.

### Out of scope (defer)

- **AnimBP per loài** — designer task trong UE Editor (W27 ALS scaffold
  + per-species `ABP_Paldark_Pal_<X>` subclass). PR ship C++ scaffold
  only.
- **Real damage GE** — `GE_Damage_PalAttack` với
  `DamageExecutionCalculation` per loài (designer task, đã có
  `GE_Damage_Standard` từ W9-10 làm template).
- **`DA_PalDef_*` asset** cho companion — designer authors trong UE
  Editor sau khi PR compile xanh.
- **Boss montage / VFX cue** — designer author Niagara + montage notify
  hook off `OnBossPhaseChanged` delegate.
- **Per-species mesh / skin** — asset team task.
- **Tame minigame** cho companion Foxparks/Tombat — W35-36 roadmap.
- **AI Director** stress test với 200+ hostile entities — W31 (Mass AI).
- **Loot table per species** — Q4 content.

### Anti-patterns đã tránh (W29-30)

- Hostile-Pal `CandidateActivities.Add(...)` thay vì assignment direct →
  inherit FollowSquadCommand từ base. Fix: assign `= { ... }` direct;
  L-13 + W29-30 validator enforce.
- Boltmane đăng ký pack subsystem khi solo boss → broadcast threat về
  chính nó + corrupt PackBroadcastRadius logic. Fix: BeginPlay không
  call `RegisterPalToPack`.
- Boss FSM dùng `Paldark.Pal.Activity.Combat` tag → conflict với Combat
  activity P40 (cùng tag, khác priority → designer confuse khi debug
  activity FSM log). Fix: dedicated tag `Paldark.Pal.Activity.BossPhase`
  + dedicated class.
- FireBreath gọi `Super::ActivateAbility` rồi tự query overlap → parent
  end ability trước → second damage path apply trên `ASC_Avatar = null`
  → crash. Fix: KHÔNG gọi parent — tự handle damage + end ability thủ
  công.
- ChainLightning bounce target dedupe miss → bounce lại primary target
  → double damage on first hit. Fix: track `VisitedTargets` set, skip
  nếu actor đã hit trong chain hiện tại.
- Ability subclass `AbilityTags.Add(species_tag)` mà không reset → tag
  chain double-fire khi tag-driven `TryActivateAbilitiesByTag(PalAttack)`
  match cả Charge / Bite / FireBreath cùng lúc. Fix: `AbilityTags.Reset()`
  trước khi add base + species tag → mỗi subclass chỉ activate đúng
  species request.
- Boss phase enum dùng `uint8` mà không khai `UENUM(BlueprintType)` →
  Blueprint không bind được delegate signature. Fix: `UENUM(BlueprintType)`
  + tất cả phase value < 256.
- Boss phase modifier không restore trong `ExitActivity` → boss respawn
  lần 2 có buff chồng. Fix: `ExitActivity` reset MoveSpeed +
  AttackInterval về snapshot ghi trong `EnterActivity`.

## W31-32 — Map 2 "Rừng Hỏng" blockout (POI + spawn point + map definition)

W31-32 ship the **C++ scaffold** backing roadmap Q3 § Tuần 31-32 "Map 2 —
Rừng Hỏng". Roadmap outcome: "Map 2 playable" with POI placement + per-Pal
spawn point. This PR ships the *runtime + tooling* so a designer can author
`Map_RungHong.umap` next without writing any extra C++.

### Cấu trúc C++ (W31-32)

| Lớp                          | Vai trò                                                                   |
|-------------------------------|----------------------------------------------------------------------------|
| `UPaldarkMapDefinition`      | UPrimaryDataAsset — MapTag + DisplayName + TSoftObjectPtr<UWorld> + recommended POI/spawn counts + IntrinsicTagsToAssert |
| `APaldarkPointOfInterest`    | Designer-placeable AActor — POITag + DangerTier + AllowedSpeciesTags + RegistrationRadius. BeginPlay registers with the POI subsystem |
| `UPaldarkPOISubsystem`       | UWorldSubsystem (server-only) — registry + query (`GetAllPOIs`, `GetPOIsByTag`, `GetPOIsByDangerTier`, `GetNearestPOI`, `DumpToLog`). Initialize sweeps via `TActorIterator` to close the BeginPlay race |
| `APaldarkPalSpawnPoint`      | Designer-placeable AActor — SpeciesTag + PalDefinitionId + FallbackPalClass + AssociatedPOITag + MaxConcurrentInstances. Resolves spawn through W27-28 definition path (`UPaldarkPalSpawnSubsystem::SpawnPalAsync`) or W20-21 class path (`SpawnHostilePackAt` count=1) |

Plus integration glue:

- `UPaldarkExperienceDefinition` now carries
  `TSoftObjectPtr<UPaldarkMapDefinition> MapDefinition` so an experience
  can declare which map it ships with.
- `Config/DefaultGame.ini` registers `PaldarkMapDefinition` as a primary
  asset type pointing at `/Game/Paldark/Maps`.
- 12 new gameplay tags: 6 POI type (`Paldark.POI.Type.Outpost` /
  `.Cache` / `.RuinedTower` / `.MiniBoss` / `.ExtractionPad` /
  `.SafeZone`) + 3 danger tier (`Paldark.POI.DangerTier.Low/Medium/High`)
  + 2 map id (`Paldark.Map.RaidSandbox` + `Paldark.Map.RungHong`) + 1
  spawn-point listener activity tag (`Paldark.Pal.Activity.SpawnPoint`).
- 3 new console commands (server / standalone):
  - `Paldark.POI.Dump` — dump every registered POI to `LogPaldarkLab`.
  - `Paldark.POI.HighlightTier <Low|Medium|High>` — debug-draw spheres
    around POIs at the given tier for 5 s.
  - `Paldark.Map.LoadDefinition <RowName>` — async-load a
    `UPaldarkMapDefinition` by primary asset row name and dump resolved
    fields (MapTag / DisplayName / counts / IntrinsicTags).
- `check_w31_map_blockout_shape` validator enforces the lot.

### Designer authoring order (W31-32)

**Designer task — does NOT block CI.** All 4 steps run in UE Editor on a
separate branch after this PR's C++ compiles green.

**Bước 1 — `DA_MapDef_RungHong`**:
1. Content Browser → `/Game/Paldark/Maps/` → right-click → DataAsset →
   `PaldarkMapDefinition` → `DA_MapDef_RungHong`.
2. Set `MapTag = Paldark.Map.RungHong`.
3. Set `DisplayName = Rừng Hỏng (Map 2)`.
4. Leave `MapAsset` empty for now — fill in after Bước 2.
5. Set `RecommendedPOICount = 8`, `RecommendedSpawnPointCount = 16`,
   `RecommendedPlayerCount = 4`.
6. `IntrinsicTagsToAssert` — add every POI type tag you plan to drop +
   every species tag you plan to spawn (Direhound / Razorbird /
   Stoneclad / Vinewraith / Boltmane / Foxparks / Tombat).
7. Save the asset.
8. Verify in console: `Paldark.Map.LoadDefinition DA_MapDef_RungHong`
   should log `Tag=Paldark.Map.RungHong, DisplayName='Rừng Hỏng (Map 2)',
   Players=4, POIs=8, SpawnPoints=16, IntrinsicTags=...`.

**Bước 2 — `Map_RungHong.umap` blockout**:
1. File → New Level → Basic.
2. Save As `/Game/Paldark/Maps/Map_RungHong.umap`.
3. Block out 1 × 1 km playable area with BSP / static meshes (placeholder
   art is fine for W31-32).
4. Drop a `BP_PaldarkPlayerStart` at the spawn zone.
5. Drop `APaldarkPointOfInterest` actors (target: 8 total):
   - 3 × `SafeZone` (DangerTier=Low) at corners + center.
   - 1 × `ExtractionPad` (DangerTier=Low) near spawn.
   - 2 × `Cache` (DangerTier=Medium) mid-map.
   - 1 × `RuinedTower` (DangerTier=High).
   - 1 × `MiniBoss` (DangerTier=High).
6. For each POI: set POITag, DisplayName, DangerTier,
   `RegistrationRadius` (cm — typical 1500–3000), `AllowedSpeciesTags`.
7. Save the level.
8. Open `DA_MapDef_RungHong` → set `MapAsset` → `Map_RungHong`.

**Bước 3 — Spawn points**:
1. Drop `APaldarkPalSpawnPoint` actors (target: 16 total — 10 hostile +
   6 companion test).
2. Per spawn point: set `SpeciesTag` (e.g.
   `Paldark.Pal.Species.Direhound`), optional `AssociatedPOITag`
   (matches a nearby POI's POITag).
3. For hostile C++ subclasses (Direhound / Razorbird / Stoneclad /
   Vinewraith / Boltmane) — set `FallbackPalClass =
   APaldarkPalCharacter_<Species>` so the class path fires until a
   `DA_PalDef_<Species>` ships.
4. For companion species with a `DA_PalDef_<Species>` (Foxparks /
   Tombat) — leave `FallbackPalClass` empty; BeginPlay will derive
   `PalDefinitionId = DA_PalDef_<Leaf>` from `SpeciesTag`.
5. Save the level.

**Bước 4 — Wire experience**:
1. Open `DA_PaldarkExp_RaidSandbox` (or fork → `DA_PaldarkExp_RungHong`).
2. Set `MapDefinition = DA_MapDef_RungHong`.
3. For prewarm — list every species tag you expect to spawn on the map
   under `PreWarmPalDefinitions` so the W27-28 spawn subsystem pulls
   their Spawn bundles before raid start.
4. Open `DefaultEngine.ini` (or the experience-specific override) and
   set the default map to `Map_RungHong`.
5. PIE — server log should show `[POISubsystem] Registered POI ...` for
   each POI, `[SpawnPoint <Name>] Dispatched ...` for each spawn point.

### Test loop trong UE Editor (W31-32)

Sau khi compile + author 4 bước trên:

1. PIE → `Paldark.Map.LoadDefinition DA_MapDef_RungHong` → must resolve
   MapTag + DisplayName + counts.
2. PIE → `Paldark.POI.Dump` → must list 8 POIs grouped by tier with
   actor location.
3. PIE → `Paldark.POI.HighlightTier High` → must draw red spheres
   around MiniBoss + RuinedTower for 5 s.
4. PIE → walk past a spawn point — server log must show
   `[SpawnPoint <Name>] Definition-path spawn succeeded` (companion
   species) or `Class-path spawn succeeded` (hostile species). The Pal
   should actually appear at the spawn-point transform.
5. (Optional) Record 30-60 s PIE video as W31-32 milestone artefact.

### Out-of-scope (W31-32)

- Actual `Map_RungHong.umap` content — designer task, separate branch
  after compile validates green.
- POI mesh art — asset team, not blocking.
- AI Director density logic (Q4) — currently every spawn point fires on
  BeginPlay regardless of player position. Activity Director gating
  lands later.
- Significance Manager culling (W48-49) — POIs / spawn points stay
  always-active until then.
- Procedural map gen (W34-35) — Map 2 is hand-authored; procedural
  layer ships separately.
- POI ownership / claim system — hub-town W40-41.
- Spawn point respawn timer — lands W35-36 with capture system.

### Anti-patterns đã tránh (W31-32)

- POI BeginPlay register *before* subsystem Initialize → registry miss.
  Fix: subsystem Initialize sweeps `TActorIterator<APaldarkPointOfInterest>`
  + OnWorldBeginPlay sweep as belt-and-suspenders.
- Spawn point fires `RequestSpawnPalAsync` before W27-28 spawn subsystem
  initialised → null deref. Fix: spawn point checks
  `UPaldarkPalSpawnSubsystem::Get(World)`; falls back to class path if
  null.
- POI subsystem on clients duplicates registry → diverges from server
  POIs (replication isn't wired). Fix: `ShouldCreateSubsystem` rejects
  client net modes — registry is server-authoritative only.
- Spawn point with neither `PalDefinitionId` nor `FallbackPalClass`
  silently no-ops on BeginPlay. Fix: BeginPlay logs Warning listing all
  three knobs so designers see the misconfiguration in `LogPaldarkLab`.
- Hardcoding map identity in `DefaultEngine.ini` only → experience
  pipeline and map travel can drift. Fix: `UPaldarkMapDefinition`
  carries the canonical map identity tag + soft .umap ref; experience
  references the data asset. (Pre-load on experience init lands W34-35;
  for W31-32 the field is informational and surfaced via
  `Paldark.Map.LoadDefinition`.)
- Single spawn-point semantics piggy-backed on
  `APaldarkHostilePalSpawner` (W20-21) — would conflate "drop N Pals
  per spawner" with "one Pal at exact location" mental models. Fix:
  dedicated `APaldarkPalSpawnPoint` actor with `MaxConcurrentInstances
  = 1` default; W20-21 spawner stays as-is for pack drops on
  Raid_Sandbox.

## Pillar covered

| Pillar | Lesson trong scaffold này |
|--------|---------------------------|
| P01 L1 Setup | `.uproject`, target files |
| P01 L3 Module + Build.cs | 3 Build.cs files |
| P01 L4 Log category | `PaldarkLogCategories.h` |
| P01 L6 Server/Client/Editor target | 3 target files |
| P02 L7 Subsystem | `PaldarkLabWorldSubsystem` |
| P03 L1 Composition seed | Actor with component composition |
| P02 L1 Class framework | `PaldarkGameInstance` / `GameModeBase` / `GameStateBase` |
| P03 L2 Component slots | 12 player slots + 8 Pal slots on `APaldarkCharacter` / `APaldarkPalCharacter` |
| P04 L1 Enhanced Input wiring | `Config/DefaultInput.ini` + `APaldarkPlayerController::ApplyMappingContexts` |
| P04 L2 Tag-keyed input config | `UPaldarkInputConfig` + `Paldark.InputTag.*` native tags |
| P04 L3 Stackable mapping contexts | `FPaldarkMappingContextAndPriority` on `UPaldarkPawnData` |
| P09 L1 Activity FSM (RoN-style) | `UPaldarkBaseActivity` + `UPaldarkPalActivityComponent` + Idle/Follow/Investigate (W5-6) + Combat (W18-19) |
| P09 L3 Utility AI lite | `FPaldarkConsideration` + `ComputeUtilityScore` (W18-19) |
| P09 L5 Custom perception | `UPaldarkPalPerceptionComponent` scan + hysteresis + grace (W18-19) |
| P11 L1 Item Fragment composition | `UPaldarkItemFragment` + `UPaldarkItemDefinition` + 3 concrete fragments (W11-12) |
| P11 L2 Replicated Inventory | `UPaldarkPlayerInventoryComponent` + `FPaldarkInventoryEntry` + DOREPLIFETIME + OnRep multicast (W11-12) |
| P12 L1 Data-driven | `PaldarkExperienceDefinition` + `PaldarkPawnData` + `PaldarkInputConfig` + `PaldarkItemDefinition` (UPrimaryDataAsset) |
| P14 L1 AssetManager | `UPaldarkAssetManager` + primary asset types in ini (Experience / PawnData / ActionSet / InputConfig / Item) |
| P17 L1 Experience pattern | Vendor-neutral mirror of Lyra Experience |
| P17 L2 PawnData / InputConfig | `UPaldarkPawnData` + `UPaldarkInputConfig` mirror Lyra's PawnData wiring |

## Sau scaffold này

- W1 day 5: ✅ GitHub Actions CI (PR #8, merged).
- W1 day 6–7: ✅ Paldark Experience hello-world (PR #9, merged).
- W1 day 8–10: ✅ Custom Experience cho "Raid_Sandbox" + action sets + player framework (PR #10, merged).
- W1 day 11–14: ✅ Enhanced Input + tag-keyed Input Config wired qua PawnData (PR #11, merged).
- W3–4: ✅ Player & Pal pawn skeleton + 12 / 8 component slots + follow locomotion (PR #12, merged).
- W5–6: ✅ Activity FSM (Idle / Follow / Investigate) + ping console cmd (PR #13, merged).
- W7–8: ✅ GAS basic + AttributeSet skeleton + GA_Sprint (PR #14, merged).
- W9–10: ✅ Damage chain (GE_Damage_Standard + DamageExecutionCalculation + GA_HitscanFire + DummyTarget) (PR #15, merged).
- W11–12: ✅ Inventory fragment system (Item Fragment + ItemDefinition + replicated InventoryComponent + 4 console cmds) (PR #16, merged).
- W13: ✅ Q1 milestone build (devlog + playtest checklist + bug bash audit) (PR #17, merged).
- W14–15: ✅ 4-player dedicated server (net subsystem + Pal replication tuning) (PR #18, merged).
- W16–17: ✅ Lag compensation server-side rewind (frame buffer + rewind + time sync handshake) (PR #19, merged).
- W18–19: ✅ Pal Combat Activity (threat detection + utility AI lite + Combat activity + Pal attack ability + team tags) (PR #20, merged).
- W20–21: ✅ Hostile-Pal AI subclasses (Direhound pack + Razorbird aerial + UWorldSubsystem pack registry + spawner actor + Patrol/Stalk activities) (PR #21, merged).
- W22–23: ✅ Squad system (UWorldSubsystem trio + replicated ping marker + radial command set + Pal P22 listener activity + 3 console commands + 13 new tags) (PR #22, merged).
- W24–25: ✅ Extraction flow (match subsystem + extraction beacon + game mode/state/player state extensions + death hook wiring + 8 new tags + 3 console commands + check_match_shape validator) (PR #23, merged). Xem `Documents/PALDARK/03-Roadmap_1_Year.md` § Tuần 24–25 + `Documents/Devlog/W24-25-Build.md`.
- W26: ✅ Q2 Milestone Build (devlog + playtest checklist + Q2 bug-bash) + L-01 validator extend (extraction GameMode wiring) + L-13/L-18 validator cleanup (PR #24, #25, #26 — merged).
- W27 (Path A): ✅ AnimBP Lyra ALS port — C++ scaffold (`UPaldarkAnimInstance` + `UPaldarkPalAnimInstance` + `IPaldarkAnimLayerInterface` + PawnData `DefaultAnimInstanceClass` wiring + `check_anim_instance_shape` validator) (PR #27, merged). Đóng code-side cho K-01 (Q1 Bug-Bash) + L-06 (Q2 Bug-Bash). Designer authors `ABP_Paldark_Player` / `ABP_Paldark_Pal` / hostile-Pal AnimBPs trong UE Editor (Path A 2 days designer side). Chi tiết § "AnimBP Lyra ALS port (W27)".
- W27–28: ✅ AssetManager + Async load + Game Feature plugin (`UPaldarkPalDefinition` UPrimaryDataAsset + `UPaldarkPalSpawnSubsystem` UWorldSubsystem + `OnExperienceAssetReady` pre-warm fan-out + `PaldarkRaidContent` game-feature plugin scaffold + `check_pal_definition_shape` / `check_pal_spawn_subsystem` / `check_game_feature_plugin` validators + 2 console commands) (PR #28, merged). Đóng roadmap Q3 § Tuần 27-28. Outcome: map load không hitch, Pal async pre-warm. Designer authors `DA_PalDef_*` + (optional) `GameFeatureData_RaidContent.uasset` trong UE Editor. Chi tiết § "AssetManager + Async load + Game Feature plugin (W27-28)".
- W29–30: ✅ 5 Pal loài còn lại (Stoneclad + Vinewraith + Foxparks + Boltmane + Tombat) — 5 ability subclass (`Charge` / `VineWhip` / `FireBreath` / `ChainLightning` / `Bite`) derive `UPaldarkGameplayAbility_PalAttack` + 3 hostile-Pal subclass (Stoneclad / Vinewraith / Boltmane) reuse W20-21 pattern + boss FSM activity `UPaldarkActivity_BossPhase` (P50, phase ladder Default → Enraged @ HP<50% → Telegraph @ HP<25% + `OnBossPhaseChanged` delegate) + 13 new gameplay tags (5 species + 2 pack + 1 activity + 5 ability) + `check_w29_pal_roster_shape` validator (PR #29, merged). Đóng roadmap Q3 § Tuần 29-30. Outcome: 10 loài Pal trong roster (Direhound + Razorbird + companion default từ W20-21 + 5 mới + 2 companion placeholder). Designer authors `BP_Pal_<Species>_Default` + `BP_GA_PalAttack_<X>` + `DA_PalDef_Foxparks` / `DA_PalDef_Tombat` + AnimBP per loài + boss montage trong UE Editor. Chi tiết § "W29-30 — 5 Pal loài còn lại (Stoneclad + Vinewraith + Foxparks + Boltmane + Tombat)".
- W31–32: ✅ Map 2 "Rừng Hỏng" blockout — C++ scaffold (`UPaldarkMapDefinition` UPrimaryDataAsset + `APaldarkPointOfInterest` + `UPaldarkPOISubsystem` UWorldSubsystem registry + `APaldarkPalSpawnPoint` definition/class-path resolver + `UPaldarkExperienceDefinition.MapDefinition` soft ref + 12 new gameplay tags (6 POI type + 3 danger tier + 2 map id + 1 spawn-point activity) + 3 console commands (`Paldark.POI.Dump` / `Paldark.POI.HighlightTier` / `Paldark.Map.LoadDefinition`) + `check_w31_map_blockout_shape` validator + `Content/Paldark/Maps/` placeholder (PR #30, merged). Đóng C++-side cho roadmap Q3 § Tuần 31-32. Outcome: designer can author Map 2 + drop POI / spawn-point actors trong UE Editor without writing new C++. Designer authors `DA_MapDef_RungHong` + `Map_RungHong.umap` blockout + POI actors + spawn-point actors trong UE Editor (separate branch after compile validates green). Chi tiết § "W31-32 — Map 2 \"Rừng Hỏng\" blockout (POI + spawn point + map definition)".
- W33–34: ✅ Map 3 "Cơ Xưởng PalCorp" + tech-themed loot drop — C++ scaffold (`UPaldarkLootTable` UPrimaryDataAsset + `UPaldarkLootDropComponent` server-only listener on `OnHealthZeroed` + `APaldarkLootBag` replicated pickup actor + 10th Pal component slot `LootDropSlot` + `UPaldarkMapDefinition.bIsIndoor` designer hint + 9 new gameplay tags (1 map id + 1 POI type + 5 tech items + 2 loot table ids) + 3 console commands (`Paldark.Loot.DumpTable` / `Paldark.Loot.ForceDrop` / `Paldark.Loot.PickupNearest`) + `check_w33_loot_drop_shape` validator + `Content/Paldark/Loot/` placeholder. This PR. Đóng C++-side cho roadmap Q3 § Tuần 33-34. Outcome: hostile-Pal kills drop replicated tech-themed loot bags + designer can author Map 3 (indoor close-quarter) + 2 loot tables without writing new C++. Designer authors `DA_LootTable_HostileStandard` / `DA_LootTable_Boss` + `DA_Item_Tech_*` (5 items) + `DA_MapDef_PalCorp` + `Map_PalCorp.umap` blockout + wires `LootTableId` on hostile-Pal Blueprint subclasses trong UE Editor (separate branch after compile validates green). Chi tiết § "W33-34 — Map 3 \"Cơ Xưởng PalCorp\" + tech-themed loot drop".

## W33-34 — Map 3 "Cơ Xưởng PalCorp" + tech-themed loot drop

Outcome (roadmap): **3 map full** — Map 1 RaidSandbox + Map 2 Rừng Hỏng + Map 3 PalCorp playable. Hostile-Pal kills drop replicated tech-themed loot bags.

### Cấu trúc C++

- `Public/Loot/PaldarkLootTable.h` (+ `.cpp`) — `UPaldarkLootTable` UPrimaryDataAsset. `FPaldarkLootEntry` rows carry a soft `UPaldarkItemDefinition` + `Weight` + `MinCount` / `MaxCount` + optional `GuaranteedWhenTag` (matched against the dying Pal's `SpeciesTag` / `TeamTag` so boss kills can guarantee specific drops). `RollEntries(FRandomStream&, FGameplayTagContainer)` returns flattened `(ItemDefinition, Count)` pairs — guaranteed entries fire first at `MaxCount`, then `MaxRolls` weighted picks (clamped to `[1, 8]`). Duplicate item picks are merged. `GetPrimaryAssetId` returns `PaldarkLootTable:<RowName>` (Lyra convention).
- `Public/Loot/PaldarkLootDropComponent.h` (+ `.cpp`) — `UPaldarkLootDropComponent` UActorComponent, server-only. On `BeginPlay` (authority + non-empty `LootTableId`) subscribes to `UPaldarkAttributeSet::OnHealthZeroed`. On death: captures Pal's transform + species / team tags by value (the Pal is destroyed before the async callback fires) → `UAssetManager::LoadPrimaryAsset(LootTableId)` → callback `RollEntries` → `SpawnActor<APaldarkLootBag>` at captured transform + `InitializeContents(rolled)`. Exposes `ForceDropAt(FTransform)` for the `Paldark.Loot.ForceDrop` console cmd to bypass the OnHealthZeroed path.
- `Public/Loot/PaldarkLootBag.h` (+ `.cpp`) — `APaldarkLootBag` AActor, `bReplicates = true`. `FPaldarkLootBagEntry` rows (soft `ItemDefinition` + `Count`) replicated via `DOREPLIFETIME(APaldarkLootBag, ReplicatedContents)`. `OnRep_ReplicatedContents` broadcasts `OnBagChanged` multicast delegate. Sphere overlap (`USphereComponent`, 50 cm default) routes authoritative overlaps to `RequestPickup(APaldarkCharacter*)` which transfers entries to the player's `UPaldarkPlayerInventoryComponent` via `AddItem`, decrements counts, and self-destroys when empty. 5-minute server-side auto-cleanup timer (configurable per-instance via `AutoCleanupSeconds`; 0 disables).
- `Public/Pal/PaldarkPalCharacter.h` (modified) — 10th component slot `LootDropSlot` (`TObjectPtr<UPaldarkLootDropComponent>`) + `GetLootDropSlot()` accessor. Ctor calls `CreateDefaultSubobject<UPaldarkLootDropComponent>("LootDropSlot")` unconditionally so hostile-Pal Blueprint subclasses don't need to add the component manually — they just stamp `LootTableId` on the existing component default.
- `Public/Map/PaldarkMapDefinition.h` (modified) — `bIsIndoor` designer hint. Default `false`; Map 3 sets `true`. Pure hint for Q4 AI Director density planner + W48-49 Significance Manager LOD tuning — W33-34 runtime doesn't consume it, just surfaces via `Paldark.Map.LoadDefinition`.
- `Private/Loot/PaldarkLootConsoleCommands.cpp` — 3 `FAutoConsoleCommandWithWorldAndArgs`:
  - `Paldark.Loot.DumpTable <RowName>` — async-loads a `UPaldarkLootTable` by primary-asset row name, logs LootTableTag + MaxRolls + each entry with resolved weight percentage.
  - `Paldark.Loot.ForceDrop <PalLabelOrName>` — finds Pal by partial label/name match, calls `LootDropSlot->ForceDropAt(GetActorTransform())`. Bypasses OnHealthZeroed so testers can verify wiring without first killing the Pal.
  - `Paldark.Loot.PickupNearest` — finds nearest non-empty `APaldarkLootBag` to local player, calls `RequestPickup(LocalPlayer)`. Server / standalone only.
- `PaldarkGameplayTags.h` / `.cpp` (modified) — 9 new tags: `Paldark.Map.PalCorp`, `Paldark.POI.Type.IndoorRoom`, 5× `Paldark.Item.Tech.<Name>` (Battery/Circuit/Coolant/Polymer/Microchip), 2× `Paldark.LootTable.<Name>` (HostileStandard/Boss).
- `Config/DefaultGame.ini` (modified) — new `+PrimaryAssetTypesToScan` row for `PaldarkLootTable` pointing at `/Game/Paldark/Loot`.
- `scripts/ci/validate_paldarklab.py` (modified) — `check_w33_loot_drop_shape` verifies the 8-piece contract (table + drop component + bag + LootDropSlot wiring + bIsIndoor + ini row + 3 console cmds + Content placeholder); `EXPECTED_GAMEPLAY_TAGS` extended with the 9 new tag identifiers.
- `Content/Paldark/Loot/README.txt` — designer authoring contract (2 loot tables × N entries each + 5 tech items + Map 3 blockout + LootTableId wiring per hostile-Pal subclass).

### Lifecycle pipeline (server)

```
APaldarkPalCharacter (hostile)
  └── LootDropSlot.LootTableId = PaldarkLootTable:DA_LootTable_HostileStandard   (designer)
  └── (W7-8) UPaldarkAttributeSet::OnHealthZeroed broadcast
        └── UPaldarkLootDropComponent::HandleHealthZeroed
              ├── capture transform + species/team tags by value
              ├── UAssetManager::LoadPrimaryAsset(LootTableId, ..., callback)
              └── (async) OnLootTableLoaded
                    ├── Cast loaded asset to UPaldarkLootTable
                    ├── FRandomStream seeded from (cycles ^ table-name hash)
                    ├── Table->RollEntries(Rng, ContextTags)
                    ├── SpawnActor<APaldarkLootBag>(SpawnClass, CapturedTransform)
                    └── Bag->InitializeContents(BagEntries)
                          ├── ReplicatedContents = BagEntries
                          ├── DOREPLIFETIME → replicates to clients
                          └── OnBagChanged.Broadcast(this) (server-side immediate)
APaldarkCharacter overlap
  └── APaldarkLootBag::OnPickupSphereOverlap (authority only)
        └── RequestPickup(Character)
              ├── FindComponentByClass<UPaldarkPlayerInventoryComponent>()
              ├── for each entry: Inventory->AddItem(ItemDefinition, Count)
              ├── decrement / remove drained entries
              ├── OnBagChanged.Broadcast(this) → OnRep on clients
              └── if (ReplicatedContents.Num() == 0) Destroy()
```

### Authoring order (designer, sau khi PR merge + Generate Project Files + compile)

1. Author 5 tech-themed item DataAssets under `/Game/Paldark/Items/` (W11-12 convention):
   ```
   DA_Item_Tech_Battery       — ItemTag = Paldark.Item.Tech.Battery   + Stack/Weight fragments
   DA_Item_Tech_Circuit       — ItemTag = Paldark.Item.Tech.Circuit   + Stack/Weight fragments
   DA_Item_Tech_Coolant       — ItemTag = Paldark.Item.Tech.Coolant   + Stack/Weight fragments
   DA_Item_Tech_Polymer       — ItemTag = Paldark.Item.Tech.Polymer   + Stack/Weight fragments
   DA_Item_Tech_Microchip     — ItemTag = Paldark.Item.Tech.Microchip + Stack/Weight fragments
   ```
2. Author 2 loot tables under `/Game/Paldark/Loot/`:
   - `DA_LootTable_HostileStandard`: `LootTableTag = Paldark.LootTable.HostileStandard`, `MaxRolls = 1`, Entries = 5 tech items (Battery weight 60 baseline, Circuit/Coolant/Polymer 20-30, Microchip 5 rare).
   - `DA_LootTable_Boss`: `LootTableTag = Paldark.LootTable.Boss`, `MaxRolls = 3`, Entries = 5 tech items (counts bumped), Microchip row has `GuaranteedWhenTag = Paldark.Pal.Species.Boltmane` so Boltmane kills always drop ≥ 1 Microchip.
3. Wire `LootTableId` on hostile-Pal Blueprint subclasses (LootDropSlot default property):
   - `BP_Pal_Direhound` / `BP_Pal_Razorbird` / `BP_Pal_Stoneclad` / `BP_Pal_Vinewraith` → `PaldarkLootTable:DA_LootTable_HostileStandard`.
   - `BP_Pal_Boltmane` → `PaldarkLootTable:DA_LootTable_Boss`.
4. Block out `Map_PalCorp.umap` under `/Game/Paldark/Maps/` — indoor close-quarter (~600 × 600 m, ceiling enclosure, corridor topology so AI Director can use tighter cull distances later). Drop ~6 POI actors: 4× `IndoorRoom`, 1× `SafeZone`, 1× `ExtractionPad`. Drop ~12 `APaldarkPalSpawnPoint` actors (mix of Direhound clusters in corridor chokepoints + Boltmane in the mini-boss room).
5. Author `DA_MapDef_PalCorp` (`UPaldarkMapDefinition`) under `/Game/Paldark/Maps/`:
   - `MapTag = Paldark.Map.PalCorp`.
   - `DisplayName = "Cơ Xưởng PalCorp — Indoor Facility"`.
   - `MapAsset = /Game/Paldark/Maps/Map_PalCorp`.
   - **`bIsIndoor = true`**.
   - `RecommendedPlayerCount = 4`, `RecommendedPOICount = 6`, `RecommendedSpawnPointCount = 12`.
6. (Optional) author a new experience `PX_PalCorpRaid` referencing `DA_MapDef_PalCorp` so the game mode loads the right map without leaning on `DefaultEngine.ini` map defaults.

### Test loop (designer, sau bước 1-6)

1. PIE on `Raid_Sandbox` (or Map_PalCorp once blocked out).
2. `Paldark.Loot.DumpTable DA_LootTable_HostileStandard` — should print 5 entries with resolved weight percentages.
3. `Paldark.Pal.SpawnTestCompanion 0` (or any hostile-Pal spawn) so a hostile Pal exists in the world.
4. `Paldark.Loot.ForceDrop Direhound_0` — should spawn an `APaldarkLootBag` at the Direhound's location with the rolled entries; check `LogPaldarkInventory` for `LootDropComponent — LootTable '...' dropped N entries`.
5. Walk over the bag — server overlap fires `RequestPickup`; check `Paldark.Inventory.List` shows the transferred entries; the bag self-destroys.
6. Kill a Boltmane via standard damage (`Paldark.Gas.Damage` + iterate) — drop bag must include at least one Microchip thanks to `GuaranteedWhenTag`.
7. `Paldark.Loot.PickupNearest` — alternative pickup path that skips overlap (server / standalone only).
8. Wait 5 minutes without overlapping a bag — should auto-destroy on the cleanup timer.

### Out of scope (defer)

- Loot rarity tiers (Common/Rare/Epic) — **W37-38** inventory polish gets a `FPaldarkItemRarity` enum + per-entry rarity tag.
- UMG pickup prompt + ping marker integration — **W50** UI polish.
- Per-killer attribution (only the player who landed the killing blow gets first-overlap priority) — **W37-38** economy week.
- Indoor AI navmesh tuning (tighter cull distances, corridor pathfind heuristics) — **Q4** polish. `bIsIndoor` flag is a designer hint surfaced now so the planner has the data ready when it lands.
- Loot bag mesh / VFX / SFX — asset team task. `BagMesh` ships as an empty `UStaticMeshComponent` placeholder.
- Multiple loot bags per Pal (rare drop bonus pool) — single bag per kill is the W33-34 contract.

### Anti-patterns avoided (W33-34)

- Synchronous `LoadObject` of the loot table on death → blocks the kill pipeline frame. Fix: `UAssetManager::LoadPrimaryAsset` async with closure capturing transform by value.
- Capturing `AActor* Owner` raw pointer in the async callback → owner destroyed before callback fires. Fix: capture `FTransform` + `FGameplayTagContainer` by value; component itself is weak-referenced via `CreateUObject`.
- Spawning the loot bag inside the owning Pal's collision capsule → bag immediately overlaps the bag's PickupSphere with the dying Pal corpse. Fix: `SpawnOffset = (0,0,20)` lifts above the kill location; `ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn` resolves any residual penetration.
- Replicating the entire `UPaldarkLootTable*` to every client → giant payload + soft-ref churn. Fix: only `APaldarkLootBag::ReplicatedContents` (TArray of `FPaldarkLootBagEntry`, soft `UPaldarkItemDefinition` ref) replicates; the table stays server-only.
- Multiple subscribers on `OnHealthZeroed` clobbering each other — Pal already has the death hook from W18-19. Fix: `FOnPaldarkHealthZeroed` is a `DECLARE_MULTICAST_DELEGATE_OneParam` (W7-8 design choice); the loot component just adds another subscriber, no conflict.
- LootDropSlot only created on hostile-Pal subclasses → designers forget to add it manually. Fix: created unconditionally on the base class with empty `LootTableId`; player-side Pals incur ~80 bytes of component overhead but skip the GAS subscribe (no-op `BeginPlay` early-returns).

## W35-36 — Pal Bond + Capture (Tame minigame)

Outcome (roadmap): **Tame được Pal trong raid** — players throw a `APaldarkPalSphere` at a stunned + low-HP hostile Pal; the server rolls a capture-probability formula; on success the Pal is destroyed and an entry appends to the player's tamed-Pal roster, on fail 30% of MaxStun is drained as a penalty. Bond level (0..20) is now a real per-Pal state with an XP curve.

### Cấu trúc C++

- `Public/Gas/PaldarkAttributeSet.h` (modified) — 2 new `FGameplayAttributeData` slots `Stun` + `MaxStun` (both replicated `RepNotify`, both clamped `[0, MaxStun]` in `PreAttributeChange`). `ATTRIBUTE_ACCESSORS` macro expands per attribute; `DOREPLIFETIME_CONDITION_NOTIFY` registers replication in `GetLifetimeReplicatedProps`. Player-side companions keep Stun at 0; only hostile Pals expose the tame component that reads `Stun / MaxStun`.
- `Public/Pal/Components/PaldarkPalBondComponent.h` (+ `.cpp`, rewritten from W3-4 stub) — `BondLevel` (int32, 0..20) + `BondXP` (float accumulator), both replicated with `OnRep_BondLevel` / `OnRep_BondXP`. `AddBondXP(float, FGameplayTag)` server API bumps `BondXP`; when it crosses `BondXPCurve.GetFloatValue(NextLevel)` (fallback linear 100 XP/level if curve empty), `BondLevel` increments and `FOnPaldarkBondLevelChanged` multicast delegate fires per new level. `SetBondLevel` directly clamps to `[0, MaxBondLevel]`. `bTracksBond` toggle gates the XP grant — hostile Pal subclasses keep it false.
- `Public/Pal/Components/PaldarkPalTameComponent.h` (+ `.cpp`, 11th Pal slot) — server-only. `bIsTameable` defaults false; hostile-Pal subclasses flip true in their ctor. `BeginTameAttempt(APaldarkCharacter*, FGameplayTag PalSphereTierTag)` evaluates the capture formula:

    ```
    P_raw = BaseCaptureProbability
            * (1 - HpPct)
            * (1 + StunPct)
            * SphereTierMultiplier
            / TameDifficulty
    P     = clamp(P_raw, MinCaptureProbability, MaxCaptureProbability)
    ```

    `FMath::FRand()` rolls a sample; if sample < P → success path `RegisterAndDestroy` → roster `TamePal` + `OwningPal->Destroy()`. Fail → 30% MaxStun drain via `SetNumericAttributeBase` so back-to-back throws don't compound stun. `ForceTame` bypasses the formula for the `Paldark.Tame.Force` console cmd. `OnTameAttemptResolved` multicast delegate fires for both paths so HUD / debug overlay can show a "32% — Failed" banner. `HydrateFromPalDefinition` (called on BeginPlay if owner has a Pal data component) copies `TameDifficulty` + `BaseCaptureProbability` from `UPaldarkPalDefinition` into the component's UPROPERTYs so the formula doesn't re-resolve the definition on every hit. `bTameAttemptInProgress` lock guards re-entrancy.
- `Public/Pal/PaldarkPalDefinition.h` (modified) — 2 new UPROPERTYs `TameDifficulty` (0.5..3.0, default 1.0; species divisor in the formula) + `BaseCaptureProbability` (0..1, default 0.35; species baseline). Per-species tuning lives on the data asset, not in C++ — designer flips them on `DA_PalDef_Boltmane` to 3.0 / 0.10 to make the boss sub-1% capture rate.
- `Public/Pal/PaldarkPalSphere.h` (+ `.cpp`) — `APaldarkPalSphere : public AActor`, replicated. `USphereComponent` collision + `UProjectileMovementComponent` (default `InitialSpeed = 1500`, `MaxSpeed = 1500`, `bShouldBounce = true`, `Bounciness = 0.2`). `PalSphereTierTag` carried by-value (`Paldark.Item.PalSphere.T1` / T2 / T3). On `OnComponentHit` (authority-only): casts other actor → `APaldarkPalCharacter`, fetches `GetTameSlot()`, calls `BeginTameAttempt(InstigatorChar, PalSphereTierTag)`. Auto-destroys after 10 s if no Pal hit.
- `Public/Gas/Abilities/PaldarkGameplayAbility_UsePalSphere.h` (+ `.cpp`) — `UPaldarkGameplayAbility_UsePalSphere : public UPaldarkGameplayAbility`. Server-only (`NetExecutionPolicy::ServerInitiated`, `InstancingPolicy::InstancedPerActor`). `PalSphereTierTag` + `PalSphereClass` (`TSubclassOf<APaldarkPalSphere>`) UPROPERTYs are designer-configurable. On activate: (1) finds the player's `UPaldarkPlayerInventoryComponent`, (2) consumes 1 of `Paldark.Item.PalSphere.<Tier>` (`RemoveItem`), (3) `SpawnActorDeferred<APaldarkPalSphere>` at player's camera forward + 100 cm, sets `PalSphereTierTag` + `Instigator`, `FinishSpawning`, (4) `EndAbility(false, false, false, true)` so the cooldown GE applies. Designer authors `GA_UsePalSphere_T1/T2/T3` BP subclasses and grants them via `UPaldarkAbilitySet_Player`.
- `Public/Pal/PaldarkPlayerPalRosterComponent.h` (+ `.cpp`, 13th player slot) — `FPaldarkRosterEntry` USTRUCT (PalDefinitionId + SpeciesTag + BondLevel + HealthAtTame + MaxHealthAtTame + Nickname). `TArray<FPaldarkRosterEntry> Entries` replicated `COND_OwnerOnly` (other players can't see your roster). `TamePal(APaldarkPalCharacter*, float)` snapshots species / health / bond at tame-time and appends. `DumpToLog` iterates entries and logs to `LogPaldarkPal`. `FOnPaldarkRosterChanged` multicast delegate fires on `OnRep_Entries`.
- `Public/Pal/PaldarkPalCharacter.h` (modified) — 11th component slot `TameSlot` (`TObjectPtr<UPaldarkPalTameComponent>`) + `GetTameSlot()` accessor. Ctor creates the component unconditionally; base class keeps `bIsTameable = false`, hostile subclasses flip it true in their ctor.
- 5 hostile-Pal subclass `.cpp` files (modified) — Direhound / Razorbird / Stoneclad / Vinewraith / Boltmane all flip `TameSlot->bIsTameable = true` in their ctor. Boltmane keeps `bIsTameable = true` on the class default but `DA_PalDef_Boltmane` is authored with `TameDifficulty = 3.0` + `BaseCaptureProbability = 0.10` so realistic capture rate stays sub-1% even at full stun + low HP + T3.
- `Public/Player/PaldarkCharacter.h` (modified) — 13th component slot `RosterSlot` (`TObjectPtr<UPaldarkPlayerPalRosterComponent>`) + `GetRosterSlot()` accessor.
- `Private/Pal/PaldarkTameConsoleCommands.cpp` — 4 `FAutoConsoleCommandWithWorldAndArgs`:
  - `Paldark.Tame.Force <PalLabelOrName>` — finds Pal by partial label/name match, calls `TameSlot->ForceTame(LocalPlayer)`. Bypasses the formula.
  - `Paldark.Tame.DumpRoster` — calls `RosterSlot->DumpToLog()` on the local player. Server / standalone only.
  - `Paldark.Tame.SetStun <PalLabelOrName> <0..1>` — sets `Stun` to `MaxStun * fraction` via `ASC->SetNumericAttributeBase(GetStunAttribute(), ...)`. Designer can drive the formula's StunPct input without authoring `GE_StunOnHit`.
  - `Paldark.Pal.Bond.AddXP <PalLabelOrName> <Amount> [ReasonTag]` — calls `BondSlot->AddBondXP(Amount, ReasonTag)`. ReasonTag is optional, defaults to `Paldark.Bond.Event.Tame`; unknown tags fall back with a warning.
- `PaldarkGameplayTags.h` / `.cpp` (modified) — 10 new tags: 1 ability (`Paldark.Ability.UsePalSphere`) + 3 PalSphere items (`Paldark.Item.PalSphere.T1/T2/T3`) + 3 Tame results (`Paldark.Tame.Result.Success`, `Paldark.Tame.Result.Fail.HpTooHigh`, `Paldark.Tame.Result.Fail.RngMiss`) + 3 Bond events (`Paldark.Bond.Event.Damage`, `Paldark.Bond.Event.Tame`, `Paldark.Bond.Event.ShareFood`).
- `scripts/ci/validate_paldarklab.py` (modified) — `check_w35_tame_minigame_shape` verifies the 11-piece contract (Stun + MaxStun attrs + bond component real + tame component shape + PalDefinition fields + projectile actor + GA + roster component + Pal char wiring + player char wiring + 5 hostile subclasses flip + 4 console cmds + Content placeholder); `EXPECTED_GAMEPLAY_TAGS` extended with the 10 new tag identifiers.
- `Content/Paldark/Tame/README.txt` — designer authoring contract (3 Pal Sphere items + 1 GA BP + per-species TameDifficulty/BaseCaptureProbability on `DA_PalDef_*` + optional BondXPCurve on companion BPs + 2 GE assets).

### Lifecycle pipeline (server)

```
APaldarkCharacter input "Throw Pal Sphere"
  └── (W7-8 GAS) ActivateAbility(GA_UsePalSphere_T1)
        ├── consume 1× Paldark.Item.PalSphere.T1 from UPaldarkPlayerInventoryComponent
        └── SpawnActorDeferred<APaldarkPalSphere>(PalSphereClass, ThrowTransform)
              ├── PalSphereTierTag = Paldark.Item.PalSphere.T1
              ├── Instigator       = PaldarkCharacter
              └── FinishSpawning   → ProjectileMovement carries it forward
APaldarkPalSphere::OnComponentHit (authority only)
  └── Cast OtherActor → APaldarkPalCharacter
        └── PalChar->GetTameSlot()->BeginTameAttempt(Instigator, PalSphereTierTag)
              ├── HydrateFromPalDefinition (if not already hydrated)
              ├── HpPct      = Health / MaxHealth          (AttributeSet read)
              ├── StunPct    = Stun   / MaxStun            (AttributeSet read)
              ├── TierMult   = T1 1.0 / T2 1.5 / T3 2.0    (GetPalSphereTierMultiplier)
              ├── P_raw      = BaseCaptureProbability * (1-HpPct) * (1+StunPct) * TierMult / TameDifficulty
              ├── P          = clamp(P_raw, MinCaptureProbability, MaxCaptureProbability)
              ├── Rolled     = FMath::FRand()
              ├── if (Rolled < P) → Success
              │     ├── OnTameAttemptResolved.Broadcast(Instigator, Success, P)
              │     └── RegisterAndDestroy(Instigator, P)
              │           ├── Instigator->GetRosterSlot()->TamePal(OwningPal, P)
              │           │     ├── append FPaldarkRosterEntry to Entries (COND_OwnerOnly)
              │           │     └── OnRosterChanged.Broadcast(this) → OnRep on owner
              │           └── OwningPal->Destroy()
              └── else → Fail.RngMiss (or Fail.HpTooHigh if HpPct > 0.7)
                    ├── OnTameAttemptResolved.Broadcast(Instigator, Fail*, P)
                    └── ASC->SetNumericAttributeBase(GetStunAttribute(), Stun - FailStunDrainFraction * MaxStun)
```

### Authoring order (designer, sau khi PR merge + Generate Project Files + compile)

1. Author 3 Pal Sphere item DataAssets under `/Game/Paldark/Items/` (W11-12 convention):
   ```
   DA_Item_PalSphere_T1   — ItemTag = Paldark.Item.PalSphere.T1   + Stack fragment (max 8) + Weight fragment
   DA_Item_PalSphere_T2   — ItemTag = Paldark.Item.PalSphere.T2   + Stack fragment (max 6) + Weight fragment
   DA_Item_PalSphere_T3   — ItemTag = Paldark.Item.PalSphere.T3   + Stack fragment (max 3) + Weight fragment
   ```
2. Author `GA_UsePalSphere.uasset` Blueprint subclass of `UPaldarkGameplayAbility_UsePalSphere` (optionally three subclasses T1/T2/T3):
   - `PalSphereTierTag = Paldark.Item.PalSphere.T1` (or T2 / T3).
   - `PalSphereClass = APaldarkPalSphere` (or a designer BP subclass with sphere mesh + throw montage).
   - `CooldownGameplayEffectClass = GE_Cooldown_UsePalSphere` (designer authors a 1-second cooldown GE).
   - Grant via `UPaldarkAbilitySet_Player` so the player auto-receives the ability on possess.
3. Wire `TameDifficulty` + `BaseCaptureProbability` on per-species `DA_PalDef_*` data asset under `/Game/Paldark/Pals/`:
   - `DA_PalDef_Direhound` → `TameDifficulty = 1.0`, `BaseCaptureProbability = 0.35`.
   - `DA_PalDef_Razorbird` → `1.2`, `0.30`.
   - `DA_PalDef_Stoneclad` → `1.8`, `0.20`.
   - `DA_PalDef_Vinewraith` → `1.4`, `0.25`.
   - `DA_PalDef_Boltmane`  → `3.0`, `0.10` (boss sub-1% capture rate even at full stun + low HP + T3).
4. Author `GE_StunOnHit` + `GE_StunDecay` under `/Game/Paldark/Tame/` and wire via `UPaldarkPalCombatComponent`. `GE_StunOnHit` is instant on attack hit, magnitude = `Paldark.SetByCaller.PalDamage * 0.5`. `GE_StunDecay` is periodic (`Period = 0.25 s`, magnitude `-5/sec`) so the stun bar drains naturally.
5. (Optional) author `BondXPCurve` on player-side companion Blueprint defaults (`BP_Pal_Foxparks`, `BP_Pal_Tombat`) — keys at (1, 100), (5, 600), (10, 2000), (20, 10000) for slower high-tier bond grind. Leave empty for the linear 100 XP/level fallback.

### Test loop (designer, sau bước 1-5)

1. PIE on `Raid_Sandbox`.
2. Spawn a Direhound: `Paldark.Pal.SpawnTestCompanion 0` (or hand-place a `BP_Pal_Direhound`).
3. Drive the formula inputs:
   - `Paldark.Tame.SetStun Direhound_0 0.5` — stun bar at 50%.
   - `Paldark.Gas.Damage Direhound_0 70` (or repeat `Paldark.Gas.Damage` until HP is ~30%).
4. Throw a Pal Sphere via the GA (input pressed in `IA_UsePalSphere` from W1 day 11-14). Expect a `BeginTameAttempt … rolled=X.XX P=Y.YY result=Success/Fail` log line per throw.
5. On Success: Pal destroys, `Paldark.Tame.DumpRoster` prints the new entry (SpeciesTag, BondLevel, HealthAtTame).
6. On Fail (RngMiss): stun drains 30% MaxStun, Pal stays. Re-stun + retry.
7. Force-tame for fast iteration: `Paldark.Tame.Force Direhound_0` — skips the formula, appends to roster directly.
8. Grant bond XP manually: `Paldark.Pal.Bond.AddXP Direhound_0 100` (bumps level by 1 with the default 100 XP/level fallback curve).

### Out of scope (defer)

- Save-game persistence of the roster across raids — **W47** save-game pass. W35-36 roster is intentionally in-memory only and resets on match-end.
- Pal Sphere mesh / throw montage / VFX / SFX — asset team task; `APaldarkPalSphere` ships with an empty `UStaticMeshComponent` placeholder.
- Client-predict throw — **W38** ability polish. W35-36 throw is server-initiated; client sees the projectile spawn via standard replication.
- Bond level → ability cooldown / damage modifiers — **W37-38** ability polish week.
- Stun bar UI widget — **W50** HUD polish.
- Multi-player tame contention (two players' Pal Spheres both hit the same Pal in the same frame) — `bTameAttemptInProgress` lock makes first-hit win; race resolution polish (e.g. "split bond XP between contributors") deferred to W37-38 if needed.
- AI Director tamed-Pal-count-based difficulty scaling — **Q4** AI Director polish.

### Anti-patterns avoided (W35-36)

- Capture formula hard-coded into the tame component → designers can't tune per-species without recompile. Fix: `TameDifficulty` + `BaseCaptureProbability` live on the per-species `UPaldarkPalDefinition`; component hydrates them on BeginPlay so the formula doesn't re-resolve the asset every frame.
- Tame attempt on the client → de-sync between client roster and server destroy. Fix: `BeginTameAttempt` is server-only (the Pal Sphere's `OnComponentHit` early-returns on non-authority), success path destroys on the authority, replication carries the Destroy + roster append to clients.
- Successful tame leaves the Pal alive (so two players can both tame the same Pal) → roster double-append. Fix: `RegisterAndDestroy` calls `OwningPal->Destroy()` synchronously after `TamePal`; further hits on the destroyed actor are no-op.
- Failed tame leaves Stun at 100% forever → spammed T1 throws can permanently freeze a Pal. Fix: 30% MaxStun drain on every fail (`FailStunDrainFraction = 0.3`).
- Stun attribute on every Pal but Player-side companion never reaches the formula → wasted bandwidth. Fix: AttributeSet is shared (`Stun` defaults to 0 on companions), but only hostile Pals expose the tame component that reads it; the cost is just 2 floats per AttributeSet on the player side.
- Roster `TArray<FPaldarkRosterEntry>` replicated to all clients → other players see your roster. Fix: `DOREPLIFETIME_CONDITION(..., Entries, COND_OwnerOnly)` so only the owning client receives the rep.
- Bond level driven via a GAS attribute → integer state forced through a float pipeline. Fix: `BondLevel` is a replicated `int32` UPROPERTY on `UPaldarkPalBondComponent` (not the AttributeSet); `BondXP` is a float because the curve eval needs it, but the level breakpoint is discrete.
- `AddBondXP` runs on every client → quadruple XP grants. Fix: server-only entry point (early-returns if `!OwnerHasAuthority()`); replication propagates the result.
- Pal Sphere actor replicates `UPaldarkPalDefinition*` so clients can show "you're throwing at a Direhound" → giant soft-ref churn. Fix: actor replicates only `PalSphereTierTag` (FGameplayTag, 8 bytes); clients read species from the impacted Pal's data component as needed.

## W37-38 — Inventory full (30 items + backpack tiers + composite container + drop-on-death)

Outcome (roadmap § Tuần 37–38): **Loot economy hoạt động** — the W11-12 inventory scaffold (5 items + flat list + fixed 30 kg cap) grows into a real loot economy with a 30-item catalogue spanning Weapon / Ammo / Consumable / Throwable / Resource, three backpack tiers that dynamically bump the carry cap (`T1 +10 kg, T2 +25 kg, T3 +45 kg`), composite containers carrying nested inventory rows one level deep, and a drop-on-death path that flattens the live inventory into a W33-34 `APaldarkLootBag` at the player's death transform.

### Cấu trúc C++

- `Public/Inventory/Fragments/PaldarkItemFragment_Backpack.h` (+ `.cpp`) — `UPaldarkItemFragment_Backpack : public UPaldarkItemFragment`. Three UPROPERTYs: `BackpackTierTag` (FGameplayTag, picks `Paldark.Item.Backpack.T1/T2/T3`), `MaxWeightBonusKg` (float, default 0), `ExtraSlots` (int32, default 0; UI hint only — W37-38 doesn't enforce slot count). `GetMaxWeightBonusKg() / GetExtraSlots()` accessors are what `UPaldarkPlayerInventoryComponent::GetMaxWeightKg` reads after `SetEquippedBackpack` resolves the soft ptr. Designer authors `DA_Item_Backpack_T1/T2/T3` with this fragment + the existing W11-12 Equipable fragment (`EquipSlot = Paldark.Equip.Slot.Backpack`).
- `Public/Inventory/Fragments/PaldarkItemFragment_Container.h` (+ `.cpp`) — `UPaldarkItemFragment_Container : public UPaldarkItemFragment`. Two UPROPERTYs: `InnerMaxWeightKg` (float, default 5.0) + `InnerMaxSlots` (int32, default 4, **runtime-clamped to [0, 16]** so a designer accidentally setting `InnerMaxSlots=1024` doesn't blow the DOREPLIFETIME blob past MTU). Marks an item row as carrying a nested inner inventory in its host `FPaldarkInventoryEntry::InnerEntries` array. FastArraySerializer migration is deferred to W47 (save-game pass).
- `Public/Player/Components/PaldarkPlayerInventoryComponent.h` (modified) — three structural changes from W11-12:
  - `FPaldarkInventoryEntry` gains `UPROPERTY() TArray<FPaldarkInventoryEntry> InnerEntries` so a single replicated row can carry a one-level-deep nested inventory (containers + crates + loot boxes).
  - `MaxWeightKg` renamed to `BaseMaxWeightKg` (the per-player intrinsic cap, default 30); the effective cap is exposed via `GetMaxWeightKg() = BaseMaxWeightKg + sum(equipped-backpack MaxWeightBonusKg)`. Old W11-12 code calling `GetMaxWeightKg()` keeps working — the method is overridden, not removed.
  - New `EquippedBackpack` UPROPERTY (`TSoftObjectPtr<UPaldarkItemDefinition>`, replicated with `OnRep_EquippedBackpack`) holds the currently-equipped backpack item definition. `SetEquippedBackpack(TSoftObjectPtr<UPaldarkItemDefinition>)` server API swaps it (`LoadSynchronous` on the soft ptr if pending), broadcasts `OnInventoryChanged`, returns true if the slot changed. The W37-38 plan accepts the swap even if new total weight > new cap (just logs a warning) — the encumbrance penalty is W38 polish.
  - New helpers: `GetExtraSlotsFromBackpack()` (int32, queries the backpack fragment), `DumpCompositeToLog()` (iterates `Entries` + nested `InnerEntries` and logs both tiers).
  - `GetCurrentWeightKg()` sums every row's own weight AND every nested `InnerEntries` row's weight so containers count correctly.
- `Private/Player/Components/PaldarkPlayerInventoryComponent.cpp` (modified) — `DOREPLIFETIME(UPaldarkPlayerInventoryComponent, BaseMaxWeightKg)` + `DOREPLIFETIME(UPaldarkPlayerInventoryComponent, EquippedBackpack)` register the new replicated fields. `GetMaxWeightKg` resolves the soft ptr (`LoadSynchronous` if not in memory yet — backpacks are tiny so the sync cost is sub-millisecond), iterates the backpack item's `Fragments`, finds the `UPaldarkItemFragment_Backpack`, returns base + bonus. `SetEquippedBackpack` clears the slot when given a null soft ptr (`Paldark.Inventory.EquipBackpack clear`).
- `Public/Player/PaldarkCharacter.h` (modified) — `TSubclassOf<APaldarkLootBag> PlayerDeathBagClass` UPROPERTY (designer can override per-Blueprint; PvE bot pawns null it to suppress drop). `SpawnDeathLootBagFromInventory()` declaration.
- `Private/Player/PaldarkCharacter.cpp` (modified) — ctor defaults `PlayerDeathBagClass = APaldarkLootBag::StaticClass()` so designers don't have to wire it. The W24-25 `OnHealthZeroed` lambda invokes `SpawnDeathLootBagFromInventory()` **before** `MatchSub->RecordDeath` — that ordering is asserted by the validator because the match subsystem may destroy the pawn (which would invalidate the inventory). `SpawnDeathLootBagFromInventory` translates each `FPaldarkInventoryEntry` (+ its nested `InnerEntries`) into a flat `TArray<FPaldarkLootBagEntry>`, spawns `APaldarkLootBag` at the death transform with `SpawnCollisionHandlingOverride = AlwaysSpawn`, calls `Bag->InitializeContents(Contents)`, then `Inv->DropAllItems()` to clear the source inventory (the bag is now the canonical copy).
- `Source/PaldarkLab/PaldarkLab.cpp` (modified) — 4 new `FAutoConsoleCommandWithWorldAndArgs`:
  - `Paldark.Inventory.GiveAll` — server-adds 1 of every `PaldarkItem` asset registered with the AssetManager (resolves the `PrimaryAssetType` list, loads each via `Path.TryLoad`, calls `Inv->AddItem(ItemDef, 1)`). Logs `[Paldark.Inventory.GiveAll] registered=N granted=K (weight A/B kg)`.
  - `Paldark.Inventory.EquipBackpack <T1|T2|T3|clear>` — looks up the matching backpack ItemDef by `Paldark.Item.Backpack.<tier>`, calls `Inv->SetEquippedBackpack(ItemDef)`. `clear` empties the slot. Logs the resulting cap so designer can verify the bonus applied.
  - `Paldark.Inventory.TestDeathDrop` — invokes `PaldarkPawn->SpawnDeathLootBagFromInventory()` without applying damage; lets designer test the bag-spawn path without dying first.
  - `Paldark.Inventory.DumpComposite` — calls `Inv->DumpCompositeToLog()` (logs `Entries` + every nested row inside `InnerEntries`).
- `Source/PaldarkLabCore/Public/PaldarkGameplayTags.h` (+ `.cpp`, modified) — 21 new tag declarations / definitions:
  - 17 new item tags: `Paldark.Item.Weapon.Rifle / Shotgun / SMG / Melee.Baton`, `Paldark.Item.Ammo.Rifle / Shotgun / SMG`, `Paldark.Item.Consumable.Medkit / Stimpack / PalFood`, `Paldark.Item.Throwable.Frag / Smoke / Flash`, `Paldark.Item.Resource.Wood / Stone / PalCore`, `Paldark.Item.Resource.Currency.Credits`.
  - 3 backpack tier tags: `Paldark.Item.Backpack.T1 / T2 / T3`.
  - 1 equip slot tag: `Paldark.Equip.Slot.Backpack` (paired with W11-12's `Paldark.Equip.Slot.Primary / Consumable / Throwable`).
- `scripts/ci/validate_paldarklab.py` (modified) — new `check_w37_inventory_full_shape` enforces the 7-piece contract (Backpack fragment files + Container fragment files + InnerEntries field + component shape + drop-on-death wiring + 4 console commands + Content placeholders). `EXPECTED_GAMEPLAY_TAGS` extended with the 21 new tag identifiers. The W11-12 inventory check is updated to look for `BaseMaxWeightKg` (renamed) + `GetMaxWeightKg` so backwards-compat is asserted.
- `Content/Paldark/Inventory/Backpacks/README.txt` + `Content/Paldark/Inventory/Containers/README.txt` — designer authoring contract for the three backpack DAs + crate / container DAs (fragment composition, tier multipliers, runtime clamp on `InnerMaxSlots`).

### Lifecycle pipeline (server)

```
Player picks up DA_Item_Backpack_T2 → UPaldarkPlayerInventoryComponent::AddItem
  └── (designer wires) UPaldarkAbility_EquipFromInventory → Inv->SetEquippedBackpack(BackpackDef)
        ├── OldEquipped     = EquippedBackpack
        ├── EquippedBackpack = NewBackpackDef    (REPNOTIFY → OnRep_EquippedBackpack on clients)
        ├── recompute current weight, log warning if > new cap
        └── OnInventoryChanged.Broadcast()
GetMaxWeightKg()
  ├── Effective = BaseMaxWeightKg             (default 30)
  ├── if (EquippedBackpack.Get())
  │     ├── UPaldarkItemFragment_Backpack* Frag = ItemDef->FindFragment<…>()
  │     └── Effective += Frag->GetMaxWeightBonusKg()
  └── return Effective                         (used by IsOverEncumbered + HUD)
APaldarkCharacter::OnHealthZeroed (W24-25 lambda, authority)
  ├── SpawnDeathLootBagFromInventory()         ← W37-38, BEFORE RecordDeath
  │     ├── flatten Entries + InnerEntries → TArray<FPaldarkLootBagEntry>
  │     ├── SpawnActor<APaldarkLootBag>(PlayerDeathBagClass, GetActorTransform())
  │     ├── Bag->InitializeContents(Contents)
  │     └── Inv->DropAllItems()                 (source inventory becomes empty)
  └── MatchSub->RecordDeath(this, ...)         (W24-25 match flow continues)
```

### Authoring order (designer, sau khi PR merge + Generate Project Files + compile)

1. Author **30 item DataAssets** under `/Game/Paldark/Items/` (W11-12 convention) — 13 existing W11-12 + W33-34 + W35-36 items, plus 17 new ones:
   ```
   /Game/Paldark/Items/Weapons/
     DA_Item_Weapon_Rifle           — ItemTag = Paldark.Item.Weapon.Rifle    + Weight(4.5) + Equipable(Primary)
     DA_Item_Weapon_Shotgun         —          Paldark.Item.Weapon.Shotgun   + Weight(3.5) + Equipable(Primary)
     DA_Item_Weapon_SMG             —          Paldark.Item.Weapon.SMG       + Weight(2.5) + Equipable(Primary)
     DA_Item_Weapon_Melee_Baton     —          Paldark.Item.Weapon.Melee.Baton + Weight(1.5) + Equipable(Primary)
   /Game/Paldark/Items/Ammo/
     DA_Item_Ammo_Rifle             —          Paldark.Item.Ammo.Rifle       + Stack(60)  + Weight(0.025)
     DA_Item_Ammo_Shotgun           —          Paldark.Item.Ammo.Shotgun     + Stack(20)  + Weight(0.05)
     DA_Item_Ammo_SMG               —          Paldark.Item.Ammo.SMG         + Stack(90)  + Weight(0.015)
   /Game/Paldark/Items/Consumables/
     DA_Item_Consumable_Medkit      —          Paldark.Item.Consumable.Medkit    + Stack(3) + Weight(0.6)
     DA_Item_Consumable_Stimpack    —          Paldark.Item.Consumable.Stimpack  + Stack(5) + Weight(0.2)
     DA_Item_Consumable_PalFood     —          Paldark.Item.Consumable.PalFood   + Stack(8) + Weight(0.3)
   /Game/Paldark/Items/Throwables/
     DA_Item_Throwable_Frag         —          Paldark.Item.Throwable.Frag    + Stack(4) + Weight(0.5) + Equipable(Throwable)
     DA_Item_Throwable_Smoke        —          Paldark.Item.Throwable.Smoke   + Stack(4) + Weight(0.4) + Equipable(Throwable)
     DA_Item_Throwable_Flash        —          Paldark.Item.Throwable.Flash   + Stack(4) + Weight(0.4) + Equipable(Throwable)
   /Game/Paldark/Items/Resources/
     DA_Item_Resource_Wood          —          Paldark.Item.Resource.Wood       + Stack(50) + Weight(0.5)
     DA_Item_Resource_Stone         —          Paldark.Item.Resource.Stone      + Stack(50) + Weight(1.0)
     DA_Item_Resource_PalCore       —          Paldark.Item.Resource.PalCore    + Stack(8)  + Weight(0.6)
     DA_Item_Resource_Credits       —          Paldark.Item.Resource.Currency.Credits + Stack(9999) + Weight(0.0)
   ```
2. Author **3 backpack DAs** under `/Game/Paldark/Inventory/Backpacks/`:
   ```
   DA_Item_Backpack_T1 — ItemTag=Paldark.Item.Backpack.T1 + Stack(1) + Weight(2.0) + Equipable(Backpack) + Backpack(T1, MaxWeightBonusKg=10, ExtraSlots=2)
   DA_Item_Backpack_T2 —          Paldark.Item.Backpack.T2 + Stack(1) + Weight(3.5) + Equipable(Backpack) + Backpack(T2, MaxWeightBonusKg=25, ExtraSlots=5)
   DA_Item_Backpack_T3 —          Paldark.Item.Backpack.T3 + Stack(1) + Weight(5.0) + Equipable(Backpack) + Backpack(T3, MaxWeightBonusKg=45, ExtraSlots=10)
   ```
3. (Optional W37-38) Author **1 container DA** under `/Game/Paldark/Inventory/Containers/` (e.g. `DA_Item_LootCrate`) with `UPaldarkItemFragment_Container(InnerMaxWeightKg=5, InnerMaxSlots=4)` so designer can iterate the composite-inventory path before W47.
4. Register every new DA via the PaldarkAssetManager `PaldarkItem` PrimaryAssetType — already covered by W11-12; just place them under `/Game/Paldark/Items` or `/Game/Paldark/Inventory`.

### Test loop (designer, sau bước 1-4)

1. PIE on `Raid_Sandbox`.
2. `Paldark.Inventory.GiveAll` — should grant `granted=N` matching the AssetManager-registered DA count. Verify weight bar pops up (already encumbered if a few weapons + ammo lands).
3. `Paldark.Inventory.EquipBackpack T1` — log shows cap 30 → 40 kg, no longer over-encumbered.
4. `Paldark.Inventory.EquipBackpack T2` — cap 30 → 55 kg.
5. `Paldark.Inventory.EquipBackpack T3` — cap 30 → 75 kg.
6. `Paldark.Inventory.EquipBackpack clear` — cap reverts to 30 kg, log warns "over-encumbered (weight X / 30.0 kg)" if `GiveAll` was used.
7. `Paldark.Inventory.DumpComposite` — top-level entries + any nested `InnerEntries` (empty unless `DA_Item_LootCrate` was placed in `Entries` and seeded).
8. `Paldark.Inventory.TestDeathDrop` — should spawn an `APaldarkLootBag` at the player's feet, source inventory empties, bag carries the flattened payload. Walk close, interact, items return.
9. Damage the player to 0 HP (`Paldark.Gas.Damage <SelfLabel> 9999` if a self-damage path is wired, or hand-shoot at a `BP_PaldarkDummyTarget` with reflect mode) — `SpawnDeathLootBagFromInventory()` fires automatically before `MatchSub->RecordDeath`, bag is on the ground, the W24-25 match flow proceeds.

### Out of scope (defer)

- Save-game persistence of `Entries` / `EquippedBackpack` / nested `InnerEntries` across raids — **W47** save-game pass. W37-38 inventory state is in-memory only and resets on match-end.
- Backpack mesh attach + IK + visible carry — asset team task; the Equipable fragment has an `AttachSocketTag` slot designers can wire.
- Encumbrance → walk-speed penalty curve — **W38** ability polish; W37-38 just exposes `IsOverEncumbered()` boolean.
- Inventory UMG widget (grid layout, drag-drop, container expand/collapse) — **W50** HUD polish.
- Multi-player inventory contention on a single dropped loot bag (two players grab the same bag in the same frame) — `APaldarkLootBag` is first-touch-wins; race resolution polish deferred.
- FastArraySerializer migration for `Entries` + `InnerEntries` — **W47** save-game polish. The W37-38 `InnerMaxSlots ≤ 16` clamp keeps the DOREPLIFETIME blob under MTU in the meantime.
- Container nested >1 deep (a crate inside a crate inside a crate) — intentionally NOT supported. `InnerEntries` is one level deep.

### Anti-patterns avoided (W37-38)

- `GetMaxWeightKg` is a getter (not a constant) → designers can't see the bonus accumulate. Fix: getter computes on the fly from `BaseMaxWeightKg` + resolved `EquippedBackpack` fragment; W11-12 callers see no change.
- `EquippedBackpack` is a hard `UPROPERTY()` `UPaldarkItemDefinition*` ref → giant DataAsset gets loaded on every client connect. Fix: `TSoftObjectPtr<UPaldarkItemDefinition>` + `LoadSynchronous` on access (backpacks are tiny so the sync cost is sub-millisecond, no async needed).
- `InnerEntries` defaults to a huge slot count → DOREPLIFETIME blob overflows MTU. Fix: `UPaldarkItemFragment_Container::InnerMaxSlots` runtime-clamped to `[0, 16]`, header advertises the ceiling.
- Drop-on-death path runs `SpawnDeathLootBagFromInventory()` AFTER `RecordDeath` → the match subsystem destroys the pawn before the bag spawns. Fix: lambda invokes the spawn helper FIRST (validator enforces this ordering).
- `SpawnDeathLootBagFromInventory` runs on both server + client → double bags. Fix: `if (!HasAuthority()) return;` early-out.
- `SpawnDeathLootBagFromInventory` runs even if `Entries` is empty → cosmetic bag with no contents. Fix: early-out on `Entries.Num() == 0` and on `Contents.Num() == 0` (after filtering invalid rows).
- Drop-on-death keeps the original `InnerEntries` tree inside the bag → bag's `FPaldarkLootBagEntry` has no container concept. Fix: flatten nested rows into the same bag (one level deep); pickup reconstructs the rows as flat top-level entries. W47 save-game preserves the original tree.
- `Paldark.Inventory.GiveAll` resolves DAs via `AssetManager.GetPrimaryAssetIdList` but skips items whose soft path is null → silent gap. Fix: `Path.TryLoad()` early-returns on null + logs `registered=N granted=K` so designer notices.
- `Paldark.Inventory.EquipBackpack T2` lower-case parsing → tag mismatch. Fix: `TierArg.ToUpper()` normalises before building `Paldark.Item.Backpack.T2`.
- New tags forgotten in `EXPECTED_GAMEPLAY_TAGS` set → designer can't request them at runtime. Fix: validator's set holds all 21 W37-38 tags; CI fails on omission.

## W39 — L-21 closure (RequiredGameFeatures assertion)

Outcome (Q3 bug-bash): close the only **code-side P0** from `Documents/Devlog/Q3-Bug-Bash.md` — designers can no longer silently boot an experience that depends on a Game Feature plugin which isn't enabled in `PaldarkLab.uproject` Plugins[].

Q3 introduced the `PaldarkRaidContent` Game Feature plugin (W27-28) and made it the home of the 10-Pal roster + Map 2/3 content. But the runtime never asserted whether the plugin was actually **Active** when an experience that depends on it loaded. A designer who forgot to flip Enabled in Editor → Plugins (or imported a fresh repo before reading the W27-28 setup) would hit a silent failure: `Paldark.Pal.WarmAll` would log `warming 0 assets`, `SpawnPalAsync` would 404 on every species, and the bug surfaces several minutes later as "no Pals are spawning" rather than at PIE start.

W39 wires a three-layer defense mirrored on the W26 L-01 (RequiredGameModeClass) pattern, but adapted for plugins (which CAN be toggled at runtime, unlike GameMode class).

### Three-layer contract

1. **Experience field** — `UPaldarkExperienceDefinition::RequiredGameFeatures` (`TArray<FString>`, `EditDefaultsOnly`). Plain `FString` because `UGameFeaturesSubsystem` keys plugins by name through `GetPluginURLByName` (not by `FName` or soft ptr). Designer authors per-experience: e.g. `PX_RaidSandbox.RequiredGameFeatures = ["PaldarkRaidContent"]`. Leave empty for HelloWorld / debug experiences.
2. **Runtime assertion** — `APaldarkGameModeBase::OnExperienceAssetReady` iterates the array and per-entry:
   - `IPluginManager::Get().FindPlugin(Name)` — if null, plugin is not even registered (most common designer mistake: forgot to add to `.uproject` Plugins[]).
   - `UGameFeaturesSubsystem::Get().GetPluginURLByName(Name, /*out*/ URL)` — resolves the subsystem URL; fails if `.uplugin` doesn't set `"IsGameFeaturePlugin": true` (regular content plugin, not a feature plugin).
   - `UGameFeaturesSubsystem::Get().GetPluginState(URL) == EGameFeaturePluginState::Active` — confirms the engine drove the plugin all the way to Active (Installed → Registered → Loaded → Active).
   - Any miss: `UE_LOG(LogPaldark, Error, …)` + `AddOnScreenDebugMessage(FColor::Red, 15s)` with the plugin name + state + suggested designer fix (toggle in Editor → Plugins, restart).
3. **Build dependency** — `PaldarkLab.Build.cs` adds `GameFeatures` (`UGameFeaturesSubsystem`) + `Projects` (`IPluginManager`) to `PublicDependencyModuleNames`. Without these the assertion won't compile.

### Designer authoring

After this PR + Generate Project Files + compile:

1. Open `PX_RaidSandbox.uasset` (W1 day 8-10 experience).
2. Locate the **Paldark|GameFeatures** category in Details panel.
3. Add `"PaldarkRaidContent"` to `Required Game Features`.
4. Save the asset, commit on a designer branch (separate from this code PR).

(Optional) For any future experience that depends on a different feature plugin (e.g. W40+ hub town's `PaldarkHubContent`), list it here. Multiple entries OK — they're checked in order.

### Test loop (after step 4)

- **Positive case:** Plugin enabled in `.uproject`, designer wired field, PIE → log shows `[Experience] RequiredGameFeatures check — 1 required, 0 missing.` No red banner.
- **Negative case (plugin disabled):** Designer toggles `PaldarkRaidContent` off in Editor → Plugins, restart Editor, PIE → log shows `LogPaldark: Error: …requires Game Feature plugin PaldarkRaidContent to be Active, but its state is N (expected M)…` + red banner on-screen.
- **Negative case (typo'd field):** Designer types `"PaldarkRadeContent"` (typo), PIE → log shows `LogPaldark: Error: …IPluginManager could not find it…` + red banner suggesting the fix.

### Recovery path

Unlike W26 L-01 (where the GameMode class is locked at `InitGame` and the only fix is a re-launch with `?game=Extraction` URL flag), Game Feature plugins **can** be activated at runtime — but the engine only auto-activates plugins listed as Enabled in `.uproject` on Editor / standalone startup. So the recovery path is editor-only:

1. Editor → Plugins → search the missing plugin → check Enabled → restart Editor.
2. (Or) edit `PaldarkLab.uproject` Plugins[] directly + restart.
3. PIE again — red banner gone.

The runtime assertion does NOT attempt to call `UGameFeaturesSubsystem::LoadAndActivateGameFeaturePlugin` automatically. Auto-activate would mask the configuration bug across sessions; the explicit failure forces the designer to commit the .uproject change.

### Validator (`check_required_game_features_wired`)

Asserts in source (one error per missing piece):

- Header: `UPaldarkExperienceDefinition` declares `TArray<FString> RequiredGameFeatures;` as `EditDefaultsOnly` UPROPERTY.
- Cpp: `PaldarkGameModeBase.cpp` includes `GameFeaturesSubsystem.h` + `Interfaces/IPluginManager.h` and the assertion symbols (`UGameFeaturesSubsystem`, `GetPluginURLByName`, `IPluginManager`, `FindPlugin`, `EGameFeaturePluginState::Active`, `AddOnScreenDebugMessage`, `FColor::Red`) are present.
- Cpp: At least one `UE_LOG(LogPaldark, Error, …)` line mentions `requires Game Feature plugin` (or `RequiredGameFeatures` directly) — verbosity must be Error so PIE doesn't swallow the message.
- Build.cs: `GameFeatures` + `Projects` are listed in `PublicDependencyModuleNames` (their absence would be a runtime symbol-resolution mystery on compile).

The validator does NOT inspect `.uasset` content (binary), so it cannot verify that `PX_RaidSandbox.uasset` actually has `["PaldarkRaidContent"]` in the field — that wiring lives in editor land. The runtime red banner is the last-mile catch-all if the validator passes but the asset is empty.

### Anti-patterns avoided (W39 L-21)

- `UE_LOG(LogPaldark, Warning, …)` instead of `Error` → swallowed during PIE's noisy startup log. Fix: validator enforces `Error` verbosity for the assertion line.
- Calling `UGameFeaturesSubsystem::Get().LoadAndActivateGameFeaturePlugin(URL)` from the runtime check to auto-fix the missing plugin → would mask the configuration bug for the next session (and across designers cloning the repo fresh). Fix: don't auto-activate; force the designer to commit the `.uproject` change.
- Resolving the plugin URL once and caching it in the experience → URL is `IsGameFeaturePlugin`-flag dependent and changes if the .uplugin manifest is edited mid-session. Fix: resolve every PIE start via `GetPluginURLByName`.
- Putting `RequiredGameFeatures` on the GameMode class (not the experience) → would require subclassing GameMode per content combo, defeating the experience pattern. Fix: field lives on `UPaldarkExperienceDefinition`; multiple experiences can share GameMode + each requires its own plugin set.
- Soft pointer + `LoadPrimaryAsset` on the plugin name → Game Features subsystem doesn't use the AssetManager for plugin discovery (it uses `IPluginManager` directly). Fix: plain `FString`, no async load needed.
- Forgetting `GameFeatures` / `Projects` in `Build.cs` → cryptic LNK2019 on `UGameFeaturesSubsystem::Get()` / `IPluginManager::Get()`. Fix: validator enforces both deps.
- Authoring `RequiredGameFeatures = ["PaldarkRaidContent"]` on every experience even when the plugin isn't required → false positive red banners on debug / HelloWorld experiences. Fix: README documents the field as opt-in per-experience.

### Out of scope (defer)

- **Auto-fix path** (call `LoadAndActivateGameFeaturePlugin` from the runtime check) — intentionally NOT shipped; see anti-patterns. Q4 may revisit if the runtime check is too noisy in practice.
- **Validator inspects `.uasset` content** for the field wiring — defers to Q4 with the rest of the L-22 BP-grep work (uasset binary parser is a big lift).
- **Per-experience plugin dependency on Dedicated Server** — the W14-15 dedicated path enables the Game Features engine plugin via `PaldarkLab.uproject` already (confirmed by `check_game_feature_plugin` for `GameFeatures` + `ModularGameplay` engine deps). Dedicated-server **feature plugin auto-activation** is closed by the W39 follow-up — see § *W39 — L-34 closure (dedicated server feature plugin auto-activate)* below.
- **Plugin state telemetry** (export the active/inactive feature plugin set as a HUD overlay or metric) — Q4 polish, requires UMG.

## W39 — L-34 closure (dedicated server feature plugin auto-activate)

Outcome (Q3 bug-bash, P2): close the last code-side dedicated-server hole from `Documents/Devlog/Q3-Bug-Bash.md` — `PaldarkLabServer` boot drives `PaldarkRaidContent` (and any future feature plugin) to **Active** state by the time `UPaldarkGameInstance::Init` returns, even though Unreal's dedicated server target does not run the `BuiltInInitialFeatureState` driver path that Editor / standalone / listen-server boot uses.

### Why dedicated server needs an explicit nudge

When a designer launches `PaldarkLabServer.exe ?game=Extraction Raid_Sandbox` on a dedicated build, the engine boot path:

1. Reads `PaldarkLab.uproject` Plugins[] → `PaldarkRaidContent.Enabled=true` → plugin DLL is loaded.
2. Reads `PaldarkRaidContent.uplugin` → `IsGameFeaturePlugin=true` → plugin is registered with `UGameFeaturesSubsystem`.
3. ❌ **Does NOT necessarily drive the plugin to `EGameFeaturePluginState::Active`** — depending on `?game=` URL flags and the order in which `UGameInstance::Init` runs vs. the subsystem's pump, the plugin can sit at `Loaded` or `Registered` when the first call into raid content happens.

Editor / standalone / listen-server side-step this because the standalone game-feature driver (which reads `BuiltInInitialFeatureState: Active` from the .uplugin) runs early during engine init. The dedicated server target intentionally does NOT include the same driver hooks — that's the upstream UE5 behavior, not a Paldark bug — so a plugin that the host process expected to be Active for raid content can ship inert.

Symptom on the client side: clients connect, see the lobby, drop into `Raid_Sandbox`, and immediately every `SpawnPalAsync` 404s, every `DA_PalDef_*` resolves to `nullptr`, raid abilities don't grant, and the L-21 red banner fires per-experience — but only after several minutes of "the dedicated server is broken" troubleshooting.

### Three-layer fix

1. **`.uplugin` descriptor** — `PaldarkRaidContent.uplugin` sets `"EnabledByDefault": true` + `"BuiltInInitialFeatureState": "Active"`. The `EnabledByDefault` flag pulls the plugin into the project's build target on `PaldarkLabServer` cooks; `BuiltInInitialFeatureState` is the standard signal to the Game Features Subsystem that Editor / standalone / listen-server boot should drive the plugin to Active without explicit code. `bIsServerOnly` is left absent (defaults false) so the same .uplugin cooks the plugin on listen-server host (which serves both server + client roles).

2. **`PaldarkLab.uproject`** — `PaldarkRaidContent.Enabled=true` (was `false`). Without this, the .uplugin's `EnabledByDefault` has no effect — the project-level entry takes precedence and the plugin is excluded from the build.

3. **`UPaldarkGameInstance::Init` dedicated-server boot hook** — after `Super::Init()` returns, if `IsDedicatedServerInstance()` is true (i.e. the process is running under the `PaldarkLabServer` target, not a client / listen / standalone / Editor PIE), `EnsureFeaturePluginsActiveForDedicatedServer()` iterates every plugin returned by `IPluginManager::Get().GetDiscoveredPlugins()`, filters to `Enabled && bIsGameFeaturePlugin`, resolves the plugin URL via `UGameFeaturesSubsystem::Get().GetPluginURLByName(...)`, and:
   - If `GetPluginState(URL) == EGameFeaturePluginState::Active` → log a single `[L-34] … already Active` line and continue.
   - Otherwise → log a `[L-34] … below Active` warning explaining the state delta, then call `UGameFeaturesSubsystem::Get().LoadAndActivateGameFeaturePlugin(URL, FGameFeaturePluginLoadComplete::CreateLambda(...))`. The completion lambda logs the success/failure result; failure mode tells the designer exactly which .uproject + .uplugin field to verify.
   - A final summary line logs `Inspected=N AlreadyActive=N Forced=N Failed=N` so log scrapes can grep `[L-34]` for the boot-time activity.

The boot hook is **server-only by design**: clients don't need to load server-only raid content (and the L-21 red banner already catches client-side mis-wiring at experience load); the early-out keeps Editor PIE warm-up time unchanged. The recovery is fire-and-forget — we don't block `Init()` waiting for the activation to complete because some Game Feature actions register subsystems that depend on engine init having finished, which would deadlock.

### Designer / DevOps test loop

After this PR + Generate Project Files + compile a `PaldarkLabServer` build:

- **Positive (server already at Active):** Launch the dedicated server with the .uplugin's `BuiltInInitialFeatureState: Active` already driving the plugin. Server log shows `[L-34] Dedicated server boot — feature plugin PaldarkRaidContent already Active (state=N); no recovery needed.` and `[L-34] Dedicated server feature plugin sweep — Inspected=1 AlreadyActive=1 Forced=0 Failed=0.` Client connects → raid content loads cleanly.
- **Negative (regression test — flip `BuiltInInitialFeatureState` to `Installed` in the .uplugin manually, do NOT commit):** Re-launch dedicated server. Server log shows `[L-34] … below Active (state=2, expected 9). Forcing LoadAndActivateGameFeaturePlugin.` then `[L-34] Feature plugin PaldarkRaidContent activation complete.` once the async recovery finishes. Client connects → raid content loads cleanly anyway (recovery succeeded).
- **Catastrophic (regression test — set `Enabled: false` in .uproject):** Re-launch dedicated server. Server log shows `Inspected=0` (plugin never registered with IPluginManager because it was excluded from the build). Client connects → L-21 red banner fires per the W39 L-21 contract. This is intentional: the dedicated-server hook recovers from runtime state drift, but it can NOT compensate for `.uproject` mis-config — that's still the designer's commit.

### Validator (`check_game_feature_plugin` extension)

The W27-28 validator function (`check_game_feature_plugin`) now also enforces the L-34 contract — five new assertions, each with a precise failure message naming the field + the fix:

- `.uplugin` has `"EnabledByDefault": true`.
- `.uplugin` has `"BuiltInInitialFeatureState": "Active"` (catches the most common designer mistake — adding the plugin without setting this field, defaulting to `Installed`).
- `.uplugin` does NOT have `"bIsServerOnly": true` (would break listen-server / standalone host cook).
- `.uproject` `PaldarkRaidContent.Enabled` is `true`.
- `PaldarkGameInstance.h` declares `EnsureFeaturePluginsActiveForDedicatedServer`.
- `PaldarkGameInstance.cpp` references the contract symbols: `IsDedicatedServerInstance`, `GameFeaturesSubsystem.h`, `IPluginManager`, `EnsureFeaturePluginsActiveForDedicatedServer`, `LoadAndActivateGameFeaturePlugin`, `GetPluginState`, `bIsGameFeaturePlugin`, and the `[L-34]` log tag.

### Anti-patterns avoided (W39 L-34)

- **Hard-coding `PaldarkRaidContent` in the boot hook** → future feature plugins (e.g. W40+ `PaldarkHubContent`) would silently miss the recovery sweep. Fix: iterate `GetDiscoveredPlugins()` + filter by `bIsGameFeaturePlugin`; the helper is plugin-agnostic.
- **Blocking on the `LoadAndActivateGameFeaturePlugin` callback inside `Init()`** → would deadlock if any action registered by the activating plugin tried to call into `UGameInstance` during its construction (well-known UE issue with subsystem-during-init flows). Fix: completion lambda is logged but not awaited; the L-21 red banner is the last-mile catch if recovery doesn't finish before the experience asks for the plugin.
- **Running the activation sweep on listen-server / standalone / client** → `BuiltInInitialFeatureState: Active` already drives the plugin to Active on those targets; running it again is a no-op but adds boot-time log noise and risks loading server-only content on clients (e.g. AI brains that don't need to exist client-side). Fix: gate the helper on `IsDedicatedServerInstance()` (validator enforces).
- **Defaulting `bIsServerOnly: true` thinking it makes the plugin "server-only safer"** → cooks the plugin out of the client build, breaks listen-server host (which cooks both roles into one process). Fix: validator fails if `bIsServerOnly: true`.
- **Forgetting the `[L-34]` log tag** → dedicated-server log scrapes can't filter the auto-activate noise from the rest of the engine init churn. Fix: all 6 `UE_LOG` lines in the boot hook are tagged `[L-34]` (validator enforces).
- **Calling `IPluginManager::Get().FindPlugin("PaldarkRaidContent")` directly instead of iterating** → couples the helper to a single plugin name + would break the validator's "future feature plugin" forward-compatibility assertion. Fix: iterate + filter.
- **Setting `EnabledByDefault: true` without also setting `BuiltInInitialFeatureState: Active`** → plugin loads but stalls at `Installed`; the dedicated server boot hook still recovers it, but Editor PIE warm-up shows the plugin as inactive in `Plugins → Game Features` UI until the first PIE start. Fix: set both fields together (validator enforces both).
- **Setting `BuiltInInitialFeatureState: Active` but leaving `.uproject Enabled: false`** → the .uplugin signal is ignored because the project-level `Enabled: false` excludes the plugin from the build entirely. Fix: validator asserts both .uplugin + .uproject sides.

### Out of scope (defer)

- **Per-experience `BlueprintAssignable` delegate "OnFeaturePluginActivated"** — Q4 polish, useful for UMG lobby UI to show "Loading raid content…" toast. Not blocking dedicated-server fix.
- **Telemetry export** of the boot hook's `Inspected/AlreadyActive/Forced/Failed` counts to a backend metric — Q4 polish, defers to the W42-43 backend integration PR.
- **L-21 + L-34 unified red banner** — both contracts surface the same designer fix; consolidating the two on-screen messages is a cosmetic polish for Q4.
- **Cook-time validation** that `EnabledByDefault + BuiltInInitialFeatureState + .uproject Enabled` are coherent — currently lives in `check_game_feature_plugin`; if we ever need cook-time enforcement (designer ignores the validator in pre-merge), we can hoist the same checks into a `UAssetValidator` subclass.

## W39 — L-28 closure (Boltmane phase HP thresholds per-species)

Outcome (Q3 bug-bash, P2): close the last hard-coded Boltmane phase tuning hole from `Documents/Devlog/Q3-Bug-Bash.md` — `UPaldarkActivity_BossPhase::EnragedThresholdPct` / `TelegraphThresholdPct` are now per-species tunable via `UPaldarkPalDefinition::PhaseHealthThresholds[]` instead of forcing the designer to subclass the activity per boss.

### Why class-CDO defaults aren't enough

W29-30 shipped `UPaldarkActivity_BossPhase` with `EnragedThresholdPct = 0.5f` + `TelegraphThresholdPct = 0.25f` as `EditDefaultsOnly` UPROPERTYs on the activity class. Designers *can* override these — but only by **subclassing the activity** per boss (e.g. `BP_Activity_BossPhase_Boltmane` with retuned thresholds), then wiring the subclass into `CandidateActivities` on each boss `APaldarkPalCharacter` subclass. That works for one boss but doesn't scale:

- The Boltmane fight gets one set of thresholds (e.g. 60% / 30%).
- A future "Stoneclad Elder" boss would need a different set (e.g. 75% / 40%).
- Both bosses use the SAME `APaldarkPalCharacter_<Species>` ladder pattern, so each needs its own activity subclass + own BP wiring just to retune two floats.

W29-30 README explicitly flagged this as an **anti-pattern** ("per-boss activity subclasses just to retune scalars"). L-28 closure fixes it by reading the thresholds from the species' `UPaldarkPalDefinition` data asset (same place `TameDifficulty` + `BaseCaptureProbability` live since W35-36).

### Two-layer fix

1. **`UPaldarkPalDefinition::PhaseHealthThresholds`** (new field) — `TArray<float>` (`EditDefaultsOnly`, ClampMin=0.05, ClampMax=0.95). Descending order, `[0..1]` HP fractions. Each entry maps to one monotonic phase transition:
    - `[0]` = `Normal → Enraged` threshold (e.g. `0.6` = 60% HP).
    - `[1]` = `Enraged → Telegraph` threshold (e.g. `0.3` = 30% HP).
    - Empty array = "use activity CDO defaults" (back-compat — designers who haven't re-saved `DA_PalDef_Boltmane` get identical behaviour to W29-30).

2. **`UPaldarkActivity_BossPhase::HydrateThresholdsFromPalDefinition`** (new helper) — server-only, called from `EnterActivity_Implementation` after the activity captures its OriginalMaxWalkSpeed / OriginalAttackInterval snapshots. Lookup pattern mirrors `UPaldarkPalTameComponent::HydrateFromPalDefinition` (W35-36):
    - Read `APaldarkPalCharacter::GetSpeciesTag()` (e.g. `Paldark.Pal.Species.Boltmane`).
    - Split the leaf segment (`Boltmane`).
    - Construct `FPrimaryAssetId("PaldarkPalDefinition", "Boltmane")` and sync-read via `UAssetManager::GetIfValid()->GetPrimaryAssetObject`.
    - Cast to `UPaldarkPalDefinition` and copy `PhaseHealthThresholds[0..1]` into the activity's `EnragedThresholdPct` / `TelegraphThresholdPct` UPROPERTYs.
    - Silently fall back to CDO defaults on any missing / malformed input (no DA authored yet, empty array, `< 2` entries, non-descending order, value outside `[0, 1]`). Designer sees a `LogPaldarkPal Warning` line in PIE so they can fix the DA without re-running the encounter.

The helper is invoked **every** `EnterActivity_Implementation` so a hot-reload of the DA during designer iteration (DA edit → PIE restart) takes effect on the next activity re-entry.

### Designer test loop

Once `DA_PalDef_Boltmane` (L-25, designer task) lands, the test loop is:

1. Author `DA_PalDef_Boltmane` with `PhaseHealthThresholds = [0.6, 0.3]`.
2. Save the DA, open PIE on `Raid_Sandbox`.
3. Spawn Boltmane via `Paldark.Pal.SpawnTestHostile Boltmane 1500`.
4. Engage the boss — log should show:
   ```
   [L-28] UPaldarkActivity_BossPhase::HydrateThresholdsFromPalDefinition — pal=BP_Boltmane_C_0 species=Paldark.Pal.Species.Boltmane overrode thresholds (enraged 0.50 -> 0.60, telegraph 0.25 -> 0.30) from DA_PalDef_Boltmane.PhaseHealthThresholds
   ```
5. Damage Boltmane to 60% HP → phase transitions to Enraged (W29-30 banner delegate fires).
6. Damage to 30% HP → phase transitions to Telegraph.

Regression cases:

- **Empty `PhaseHealthThresholds[]`** on the DA → `LogPaldarkPal Verbose` line + boss uses W29-30 defaults (0.5 / 0.25). Encounter still playable.
- **`PhaseHealthThresholds = [0.3, 0.6]`** (reversed) → `LogPaldarkPal Warning` "must be strictly descending" + boss uses defaults.
- **`PhaseHealthThresholds = [60.0, 30.0]`** (designer typo: percentages instead of fractions) → `LogPaldarkPal Warning` "out of range [0,1]" + boss uses defaults.
- **No DA authored yet** (L-25 still open) → `LogPaldarkPal Verbose` + boss uses defaults. Same behaviour as W29-30; no break.

### Validator extension

`scripts/ci/validate_paldarklab.py`:

- `check_pal_definition_shape` step (7): `UPaldarkPalDefinition.h` declares `TArray<float> PhaseHealthThresholds` and the surrounding UPROPERTY uses `EditDefaultsOnly`.
- `check_w29_pal_roster_shape` (activity scaffold check, step 8): `PaldarkActivity_BossPhase.h` declares `HydrateThresholdsFromPalDefinition`; `PaldarkActivity_BossPhase.cpp` includes `Pal/PaldarkPalDefinition.h` + `Engine/AssetManager.h`, implements the helper body, calls the helper from `EnterActivity_Implementation`, references the `PhaseHealthThresholds` field, and tags log lines `[L-28]` (matches the L-34 closure precedent for log-grep attribution).

Run locally: `python3 scripts/ci/validate_paldarklab.py` → expect `OK`.

### Anti-patterns avoided (L-28)

- **Activity subclass per boss just to retune scalars** — L-28's whole point is to remove this. A future "Stoneclad Elder" boss reuses the same `UPaldarkActivity_BossPhase` class, only `DA_PalDef_StonecladElder` changes.
- **Reading the PalDef on `TickActivity`** — Would hit AssetManager once per tick (60Hz). Helper runs in `EnterActivity`, once per fight-engage; thresholds are stable for the duration.
- **Failing the spawn on bad DA data** — `Paldark.Pal.SpawnTestHostile` would silently no-op + designer can't repro mid-iteration. Warn + fall back to CDO defaults so the encounter is always playable.
- **Hard-asserting on missing DA** — L-25 (designer authoring) is independent; if Boltmane spawns before the DA is authored, the boss should still fight (the L-21 red banner already handles the "missing critical content" surface).
- **Server-side broadcast on threshold change** — Thresholds are static for the fight; broadcasting on `EnterActivity` would spam the OnBossPhaseChanged delegate (which is reserved for actual phase transitions, not config reads).
- **Per-instance EditAnywhere** — Would let a placed instance drift from the DA values. `EditDefaultsOnly` on the field means thresholds are pinned to the DA CDO; runtime instances always read from the DA.
- **Caching the resolved PalDef on the activity** — Would prevent the hot-reload path (designer edits DA → next fight uses new values). Sync-read every EnterActivity instead.

### Out of scope (defer)

- **Per-phase ability override** (boss gets a different `AttackAbilityClass` in Telegraph phase). W29-30 README already documents the workaround (designer subscribes to `OnBossPhaseChanged` and swaps `CombatSlot.AttackAbilityClass`). Marketplace polish task.
- **Threshold runtime mutation** (heal effect bumps thresholds mid-fight) — explicitly an anti-pattern; phase transitions stay monotonic.
- **Validator check that `DA_PalDef_Boltmane` actually populates `PhaseHealthThresholds`** — That's a designer-side authoring requirement, same category as L-25 / L-27. Could add a uasset binary diff check Q4 (same blocker as the L-22 BP grep).
- **`PhaseHealthThresholds` array length > 2** (e.g. 4-phase boss with extra Awoken state) — Activity FSM is hardcoded to 3 phases (Normal / Enraged / Telegraph). Expanding requires adding enum values + transition handlers; defer until a designer requests a 4-phase boss in W42+.

## W39 — L-32 closure (Inventory FastArraySerializer scaffold)

Outcome (Q3 bug-bash, P2): land the migration *scaffold* so the W47 save-game polish PR can flip `FPaldarkInventoryEntry` to a `FFastArraySerializerItem` with a single-line storage swap, instead of a 30-callsite hunt. **No replication semantics change in W39** — the W37-38 plain `DOREPLIFETIME(TArray<FPaldarkInventoryEntry>)` blob is still the wire format. The scaffold only threads through the *callers* so the W47 migration is mechanical.

### Why scaffold ahead of W47

W37-38 README and the Q3 bug-bash log both call out FastArraySerializer as W47 work. The risk if W47 starts cold:

- Every `OnInventoryChanged.Broadcast(this)` callsite (currently 4 — `AddItem`, `RemoveItemByTag`, `DropAllItems`, `SetEquippedBackpack`, `OnRep_Entries`, `OnRep_EquippedBackpack`) becomes a `MarkItemDirty` / `MarkArrayDirty` call. Each needs to know which entry index changed — but the existing call sites just shout "something changed".
- FastArraySerializer wants a stable per-row id so it can ship deltas rather than the whole array. Today rows are identified by their *array position*, which shuffles when a stack empties (`Entries.RemoveAt(i)` collapses indices).
- The migration PR would touch every inventory-mutating function AND the loot-drop translation path AND the bag — review surface explodes.

The scaffold collapses that into three structural changes that W39 lands today and that W47 then re-points to FastArraySerializer:

1. **Per-row stable id** — `FPaldarkInventoryEntry::MigrationReplicationKey` (int32, `UPROPERTY(NotReplicated, Transient)`).
2. **Atomic counter** — `UPaldarkPlayerInventoryComponent::BuildNextReplicationKey()` static, `FPlatformAtomics::InterlockedIncrement` on a process-wide volatile int32.
3. **Single broadcast funnel** — `UPaldarkPlayerInventoryComponent::MarkInventoryDirty(int32 EntryIndex = INDEX_NONE)`. Every previous broadcast callsite now routes through this. W47 swaps the body for per-row `MarkItemDirty` / `MarkArrayDirty` without touching any caller.

### Why `NotReplicated, Transient`

`MigrationReplicationKey` is a server-side correlation key — clients don't need it (the outer `TArray<FPaldarkInventoryEntry>` is still replicated as a single blob in W39, so the key is meaningless on the wire) and save-game doesn't either (the key is regenerated on dedicated-server restart). Marking the field `NotReplicated` keeps the W37-38 DOREPLIFETIME blob the same byte-size per row; `Transient` keeps W47's save-game pass from accidentally restoring a stale key from disk.

### W47 four-step migration plan

When W47 swaps the storage:

1. **Header — change inheritance**:
   ```cpp
   USTRUCT(BlueprintType)
   struct PALDARKLAB_API FPaldarkInventoryEntry : public FFastArraySerializerItem
   ```
   The base class brings its own auto-managed `int32 ReplicationKey`. Remove `MigrationReplicationKey` (the name divergence was intentional — base class wins now). Update every read of `Row.MigrationReplicationKey` to `Row.ReplicationKey`.

2. **Header — wrap `Entries`** with a `FFastArraySerializer`-backed struct:
   ```cpp
   USTRUCT()
   struct FPaldarkInventoryEntryArray : public FFastArraySerializer
   {
       UPROPERTY()
       TArray<FPaldarkInventoryEntry> Items;
       bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms) { ... }
   };
   template<> struct TStructOpsTypeTraits<FPaldarkInventoryEntryArray> : TStructOpsTypeTraitsBase2<FPaldarkInventoryEntryArray>
   { enum { WithNetDeltaSerializer = true }; };
   ```
   Replace `TArray<FPaldarkInventoryEntry> Entries` UPROPERTY with `FPaldarkInventoryEntryArray Entries` UPROPERTY (Replicated). `GetEntries()` now returns `Entries.Items` (`const TArray<FPaldarkInventoryEntry>&`).

3. **.cpp — rewrite `MarkInventoryDirty` body**:
   ```cpp
   void UPaldarkPlayerInventoryComponent::MarkInventoryDirty(int32 EntryIndex)
   {
       if (EntryIndex == INDEX_NONE || EntryIndex < 0 || EntryIndex >= Entries.Items.Num())
       {
           Entries.MarkArrayDirty();
       }
       else
       {
           Entries.MarkItemDirty(Entries.Items[EntryIndex]);
       }
       OnInventoryChanged.Broadcast(this);
   }
   ```
   The 4 existing callsites (`AddItem` / `RemoveItemByTag` / `DropAllItems` / `SetEquippedBackpack` / `OnRep_*`) keep working unchanged — they already pass the right `EntryIndex` (or `INDEX_NONE`) through the funnel.

4. **.cpp — delete the scaffold counter**. `BuildNextReplicationKey` + `PaldarkInventoryMigration::GReplicationKeyCounter` + the `FPlatformAtomics::InterlockedIncrement` line can go — `FFastArraySerializer::MarkItemDirty` increments `FFastArraySerializerItem::ReplicationKey` for us. `OnRep_Entries` can also delete (FastArraySerializer dispatches per-row `PreReplicatedRemove` / `PostReplicatedAdd` / `PostReplicatedChange` callbacks, each of which calls `MarkInventoryDirty(idx)` with the actual touched row). W47 PR is then ~50 lines net — most of them deletions.

### Validator extension

`scripts/ci/validate_paldarklab.py` → `check_w37_inventory_full_shape`, new sections (8) + (9):

- **Header (8)** — `FPaldarkInventoryEntry::MigrationReplicationKey` field declared; UPROPERTY block carries `NotReplicated` + `Transient`; `BuildNextReplicationKey()` static method declared; `MarkInventoryDirty(int32 EntryIndex = INDEX_NONE)` funnel declared with the exact `INDEX_NONE` default (so callers can omit it when the mutation isn't row-local).
- **.cpp (9)** — `PaldarkInventoryMigration::GReplicationKeyCounter` namespace + global; `BuildNextReplicationKey` definition uses `FPlatformAtomics::InterlockedIncrement`; `AddItem`'s Phase 2 stamps `NewRow.MigrationReplicationKey = BuildNextReplicationKey()`; `MarkInventoryDirty` definition exists; `[L-32][Inventory.MarkDirty]` log tag (mirrors the `[L-28]` / `[L-34]` log-grep precedent); exactly **one** residual `OnInventoryChanged.Broadcast(this)` call (the one inside `MarkInventoryDirty`'s body — every other previous callsite must have been replaced with `MarkInventoryDirty(...)`).

The comment-stripping pass `re.sub(r"//[^\n]*", "", text)` is applied before counting residual broadcasts so the W47 migration plan example (which appears in a `//` block inside `MarkInventoryDirty`) doesn't trip the count.

Run locally: `python3 scripts/ci/validate_paldarklab.py` → expect `OK`.

### Anti-patterns avoided (L-32)

- **Adding `ReplicationKey` (without the `Migration` prefix) today** — Collides with `FFastArraySerializerItem::ReplicationKey` in W47. The name divergence is intentional so the W47 PR is a *remove*-then-*inherit*, not a merge conflict.
- **`UPROPERTY(Replicated)` on `MigrationReplicationKey`** — Would inflate the W37-38 DOREPLIFETIME blob by 4 bytes per row (and per-nested row, recursively). `NotReplicated` keeps the wire format identical to W37-38.
- **Stamping the key inside `RemoveItemByTag`** — `RemoveItemByTag` mutates existing rows (decrement stack) and erases emptied rows; neither should reissue the key. Only freshly-appended rows get a new key. This matches FastArraySerializer's contract — `MarkItemDirty` doesn't change `ReplicationKey`.
- **Per-tick `MarkInventoryDirty`** — The funnel is invoked once per mutation, not per frame. W47's `MarkArrayDirty` is cheap (a uint32 bump) but `MarkItemDirty` triggers per-row delta computation; spamming it per tick negates the whole point of FastArraySerializer.
- **Inlining `OnInventoryChanged.Broadcast(this)` at any new callsite** — Validator's residual-broadcast counter caps at 1. Any new code path that mutates the inventory MUST route through `MarkInventoryDirty(...)` so W47 doesn't need another callsite-hunt round.
- **Persisting the per-process counter** — Would re-issue stale keys after a dedicated-server restart (the counter resets to 0 on process boot). `Transient` keeps save-game from snapshotting it; clients re-receive the entire `Entries` array on PostNetInit so there's no continuity to preserve.
- **Adding the `MigrationReplicationKey` to `FPaldarkLootBagEntry`** — Loot bag entries are a *flattened* projection of inventory rows (composite containers spill their `InnerEntries` into top-level rows in the bag); the per-row id doesn't survive that translation. W47 can revisit if the bag UI needs delta replication too; not required for the inventory migration itself.

### Out of scope (defer to W47)

- **Actually swapping `Entries` to `FFastArraySerializerItem` storage** — That's W47. This PR is the scaffold only; W37-38 wire format unchanged.
- **`InnerEntries` FastArraySerializer migration** — Nested containers reuse `FPaldarkInventoryEntry` recursively; W47 makes them per-row delta too. Today they inherit the outer array's DOREPLIFETIME (one blob covers the whole tree).
- **Per-row `MarkItemDirty` on `RemoveItemByTag`** — Today it routes to `INDEX_NONE` because the function touches multiple rows. W47 may revisit to issue one dirty-mark per touched row if the inventory grows past ~50 entries.
- **Removing `OnRep_Entries`** — W47's per-row `PostReplicatedAdd` / `PostReplicatedChange` / `PreReplicatedRemove` callbacks replace the OnRep_Entries broadcast. Today we keep `OnRep_Entries` calling `MarkInventoryDirty(INDEX_NONE)` for the same end-user effect.

## W40-41 — Hub Town (Q4 feature week #1)

Outcome (roadmap Q4 § Tuần 40-41): the central social/lobby map between raids. **8-player shard** (vs the 4-player raid shard) where players visit a **Pal Stable** to swap/heal/inspect their roster, a **Marketplace** kiosk to buy/sell items, and a **Briefing Room** to pick the next raid map and hand the entire shard off into the raid via a ServerTravel. This week lands the C++ scaffold only — the actual `Map_HubTown.umap`, the per-building DataAssets, the UMG widgets for stable/marketplace/briefing UI, and the AWS fleet picker that decides which dedicated server runs the hub vs the raid all stay out of scope.

### What ships (C++ side, this PR)

| File | Role |
|------|------|
| `Public/Hub/PaldarkHubBuildingDefinition.h/.cpp` | `UPrimaryDataAsset` carrying per-instance designer config: `BuildingTag` (`Paldark.Hub.Building.*`), `DisplayName`, `InteractRadius`, `InteractInputTag` (`Paldark.InputTag.Interact.*`), `RosterSlotCount` (stable only), `MarketplaceCatalog` soft array (kiosk only), `AllowedRaidMaps` soft array (briefing only). Mirrors `UPaldarkPalDefinition` / `UPaldarkMapDefinition` shape. |
| `Public/Hub/PaldarkHubBuilding.h/.cpp` | Base actor for the three hub buildings. Owns the shared scaffold: soft ref to the definition, a root `USphereComponent` sized from `InteractRadius` on BeginPlay, server-side BeginPlay/EndPlay register/unregister with `UPaldarkHubSubsystem`. `bReplicates = false` — buildings are level-placed and never destroyed mid-game. |
| `Public/Hub/PaldarkPalStable.h/.cpp` | Pal Stable subclass. Stamps `BuildingTag = Paldark.Hub.Building.PalStable` in ctor. `OnInteract` reads `RosterSlotCount` from the definition and logs a placeholder (W44+ wires the real swap/heal/inspect flow). |
| `Public/Hub/PaldarkMarketplaceKiosk.h/.cpp` | Marketplace kiosk subclass. Stamps `BuildingTag = Paldark.Hub.Building.Marketplace` in ctor. `OnInteract` reads `MarketplaceCatalog.Num()` and logs a placeholder (W45+ wires buy/sell + price table). |
| `Public/Hub/PaldarkBriefingRoom.h/.cpp` | Briefing Room subclass — **only one with real behaviour this week**. `OnInteract` reads `PendingRaidMapTag` from `UPaldarkHubSubsystem`, validates the tag is in `AllowedRaidMaps`, resolves the matching `UPaldarkMapDefinition.MapName`, then schedules a deferred `UPaldarkNetSubsystem::HostHubServer(MapName, ExperienceId)` via `FTimerManager::SetTimerForNextTick` so the ServerTravel doesn't recurse into the interact frame. |
| `Public/Hub/PaldarkHubSubsystem.h/.cpp` | `UWorldSubsystem` (server-only). Registry of registered hub buildings. Mirrors the W31-32 `UPaldarkPOISubsystem` shape: `Initialize` sweeps the world for already-placed buildings, `OnWorldBeginPlay` re-sweeps (race guard for buildings whose `BeginPlay` fires before our `Initialize`). Registers 3 console commands: `Paldark.Hub.List` / `Paldark.Hub.QueueRaid <MapTag>` / `Paldark.Hub.Status`. |
| `Public/Framework/PaldarkGameMode_Hub.h/.cpp` | `APaldarkGameModeBase` subclass for hub shards. Ctor sets `MaxPlayersPerSession = 8` so a PIE quick-test (no experience) already honours the 8-player intent. `BeginPlay` logs a status line so designers can spot the hub subsystem + building count on the first frame. |
| `Public/Experience/PaldarkExperienceDefinition.h` | Adds `int32 MaxPlayers` UPROPERTY (default 4, ClampMin 1, ClampMax 64). Hub experience (`PX_HubTown`) overrides to 8. |
| `Private/Framework/PaldarkGameModeBase.cpp` | `OnExperienceAssetReady` seeds `MaxPlayersPerSession` from `Experience->MaxPlayers` and propagates to `AGameSession::MaxPlayers`. Logs `[Experience] MaxPlayersPerSession bumped X → Y from experience <id>.` so the bump is greppable. |
| `Public/Net/PaldarkNetSubsystem.h`<br>`Private/Net/PaldarkNetSubsystem.cpp` | Adds `HostHubServer(MapName, ExperienceId)` wrapper. Logs with `[W40-41][Hub]` prefix, then delegates to `HostListenServer`. Distinct entry point so W42-43 can branch into the AWS fleet picker without touching the briefing-room callsite. |
| `Source/PaldarkLabCore/Public/PaldarkGameplayTags.h`<br>`Source/PaldarkLabCore/Private/PaldarkGameplayTags.cpp` | 10 new native tags: `Paldark.Map.HubTown`, `Paldark.Experience.HubTown`, `Paldark.Hub.Building.{PalStable,Marketplace,BriefingRoom}`, `Paldark.InputTag.Interact.{Stable,Marketplace,Briefing}`, `Paldark.Mode.{Hub,Raid}`. |
| `Config/DefaultGame.ini` | New `+PrimaryAssetTypesToScan` row for `PaldarkHubBuilding` rooted at `/Game/Paldark/Hub` so AssetManager picks up `DA_HubBuilding_<Type>` data assets. |

### Why split the building definition from the actor

Same pattern as W11-12 `UPaldarkItemDefinition` vs the spawned item pickup actor: the **definition** is a `UPrimaryDataAsset` the designer drags into a `TSoftObjectPtr<>` slot on the placed **actor**. Three reasons:

1. **Per-type payload** (stable vs marketplace vs briefing) lives in one DA struct, not three actor classes. `RosterSlotCount` / `MarketplaceCatalog` / `AllowedRaidMaps` only matter for one of the three subclasses each, but a single DA can carry all three fields without forcing the actor to know which subclass it's for. The subclass stamps its `BuildingTag` in ctor; the rest reads from the DA.
2. **DA swap without re-placing the actor**: designer changes the marketplace catalog mid-iteration → re-saves `DA_HubBuilding_Marketplace_Main.uasset`. The placed `BP_HubBuilding_Marketplace` actor in `Map_HubTown` doesn't need a re-bake.
3. **AssetManager async-load alignment with W27-28**: `MarketplaceCatalog` is `TArray<TSoftObjectPtr<UPaldarkItemDefinition>>` — the catalog *referenced* items are pre-warmed by `UPaldarkPalSpawnSubsystem::RequestPreWarmAsync` when the hub experience boots, not loaded synchronously on first interact. (Per-item pre-warm itself is a W45+ task; in W40-41 the definition just carries the soft refs as a data shape.)

### Why `MaxPlayers` on the experience definition (not the GameMode CDO)

The roadmap line "8-player shard for hub vs 4-player for raid" makes it sound like a GameMode property. But:

- A single dedicated server boots a *single* GameMode class via `InitGame` — switching experiences mid-process (e.g. `RestartGame` after a match) keeps the class but loads a different DA. If `MaxPlayers` lived on the GameMode CDO, that switch would silently keep the old cap.
- The experience asset is also the seat for `RequiredGameModeClass` (W26 L-01) and `RequiredGameFeatures` (W39 L-21) — `MaxPlayers` joins them as another per-experience contract.
- Hub-vs-raid maps via *URL flag* (`open Map_HubTown?game=Hub?Experience=Hub`) still works: the experience load fires in `InitGame`, the base `APaldarkGameModeBase::OnExperienceAssetReady` callback bumps `MaxPlayersPerSession` to the experience's value, and `AGameSession::MaxPlayers` gets the new cap before the first `PreLogin`.
- The Hub GameMode ctor *also* sets `MaxPlayersPerSession = 8` as a safety net so a PIE quick-test that launches the hub class without specifying an experience still rejects the 9th client. The two values converge once the experience loads.

### Why `HostHubServer` wrapper (not just call `HostListenServer` directly from the briefing room)

This is a **W42-43 seam, not new behaviour today**. Today `HostHubServer` is a 4-line method that logs `[W40-41][Hub]` then calls `HostListenServer`. The reason it exists at all:

- W42-43 (AWS fleet integration) needs to branch on "is this hub→raid handoff or raid→raid restart?" to pick the right Fleet Spec (hub fleets are long-lived, raid fleets are spun up per-match and torn down).
- Putting that branch inside `HostListenServer` itself would force *every* caller to thread a "kind" parameter through. Splitting into two named entry points keeps the call sites self-documenting and lets W42-43 swap the body of one of them without touching the other.
- The `[W40-41][Hub]` log prefix is greppable for designers debugging "why didn't the raid load?" — they can see whether the briefing room actually issued the travel, distinct from a generic listen-server boot.

### Subsystem race-guard pattern (mirrors W31-32 POI)

The hub subsystem and the hub building actors both expect each other to be alive when `BeginPlay` fires, but UE's `UWorldSubsystem::Initialize` can run *after* level actors' `BeginPlay`. Two-pronged guard:

1. **Building side**: `APaldarkHubBuilding::BeginPlay` tries `UPaldarkHubSubsystem::Get(GetWorld())`; if null, the building skips the register call (it'll be picked up by the subsystem sweep). If non-null, it calls `RegisterHubBuilding(this)`.
2. **Subsystem side**: both `Initialize` and `OnWorldBeginPlay` call `SweepWorldForBuildings()`, which iterates `TActorIterator<APaldarkHubBuilding>` and adds any unregistered buildings. `RegisterHubBuilding` is idempotent — re-adding a known building is a no-op. The two-sweep belt-and-suspenders covers the building-streamed-via-sublevel edge case too.

### Designer test loop (in UE Editor, after this PR lands)

1. Pull this branch, run **Generate Project Files**, build PaldarkLab + Editor in your IDE (the Devin VM has no UE compiler; this is a UE-side step).
2. Create three building DataAssets under `Content/Paldark/Hub/`:
   - `DA_HubBuilding_PalStable_Main.uasset` — `BuildingTag = Paldark.Hub.Building.PalStable`, `DisplayName = "Pal Stable"`, `InteractRadius = 400`, `InteractInputTag = Paldark.InputTag.Interact.Stable`, `RosterSlotCount = 12`.
   - `DA_HubBuilding_Marketplace_Main.uasset` — `BuildingTag = Paldark.Hub.Building.Marketplace`, `MarketplaceCatalog = [DA_Item_PalSphere_T1, DA_Item_Consumable_Medkit, ...]`.
   - `DA_HubBuilding_BriefingRoom_Main.uasset` — `BuildingTag = Paldark.Hub.Building.BriefingRoom`, `AllowedRaidMaps = [DA_MapDef_RaidSandbox, DA_MapDef_RungHong, DA_MapDef_PalCorp]`.
3. Create `DA_ExperienceDef_Hub` with `MaxPlayers = 8` and `RequiredGameModeClass = APaldarkGameMode_Hub`, and `DA_MapDef_HubTown` pointing at `Map_HubTown.umap`.
4. Author `Map_HubTown.umap` — block out the layout, drop three `BP_HubBuilding_*` actors (Blueprint subclasses of the C++ classes), wire each one's `BuildingDefinition` soft ref to the matching DA. Set the map's World Override → GameMode to `APaldarkGameMode_Hub`.
5. Launch via console: `open Map_HubTown?Experience=Paldark.Experience.HubTown` (the URL alias is in `DefaultEngine.ini` once W42-43 lands; for W40-41 use the Map World Override path).
6. Verify on the server log:
   - `[HubSubsystem] SweepWorldForBuildings — picked up 3 building(s) (total=3).`
   - `[GameMode.Hub] BeginPlay — MaxPlayers=8 HubSubsystem=present buildings=3.`
   - `[Experience] MaxPlayersPerSession bumped 8 → 8 from experience PaldarkExperience.PX_HubTown.` (the bump is a no-op here because the hub GameMode ctor already set 8, but the log line confirms the experience override path is wired).
7. Connect 8 clients (one host + 7 client `open <ip>`); confirm the 9th rejects with `Server full`.
8. Approach the briefing room — exec `Paldark.Hub.QueueRaid Paldark.Map.RaidSandbox`, then interact with the briefing-room actor. Expect a `[W40-41][Hub] HostHubServer: Map=Raid_Sandbox Experience=...` log + the whole shard ServerTravels into the raid map.

### Validator extension

`scripts/ci/validate_paldarklab.py` → new `check_w40_hub_shape` (~330 lines), wired into `main()` after `check_w37_inventory_full_shape`. Covers:

- All 14 expected files exist (`Public/Hub/*.h` + `Private/Hub/*.cpp` for the 6 hub classes + `Framework/PaldarkGameMode_Hub.{h,cpp}`).
- Each header declares the expected `class PALDARKLAB_API <Name> : public <Base>` line.
- `UPaldarkHubBuildingDefinition` has the 7 required UPROPERTY fields with the right types.
- `APaldarkHubBuilding` carries the soft ref + sphere + register/unregister hooks; the `.cpp` invokes `RegisterHubBuilding(this)` / `UnregisterHubBuilding(this)` and guards on `HasAuthority()`.
- Each subclass stamps its `BuildingTag` from the right `PaldarkGameplayTags::TAG_Paldark_Hub_Building_*`.
- `APaldarkBriefingRoom::OnInteract` calls `GetPendingRaidMapTag` + `HostHubServer` + `SetTimerForNextTick` (deferred travel).
- `UPaldarkHubSubsystem` gates on `NM_DedicatedServer / NM_ListenServer / NM_Standalone`, sweeps via `TActorIterator<APaldarkHubBuilding>`, registers the 3 console commands.
- `APaldarkGameMode_Hub` ctor sets `MaxPlayersPerSession = 8`.
- `UPaldarkExperienceDefinition` declares `int32 MaxPlayers`; `APaldarkGameModeBase::OnExperienceAssetReady` seeds `MaxPlayersPerSession = Experience->MaxPlayers`.
- `UPaldarkNetSubsystem::HostHubServer` declared with the right signature + defined with the `[W40-41][Hub]` log prefix.
- `DefaultGame.ini` includes the `PaldarkHubBuilding` AssetManager scan rule.
- 10 new gameplay tags (`EXPECTED_GAMEPLAY_TAGS` block) — enforced by the existing `check_gameplay_tags` pass.

Run locally: `python3 scripts/ci/validate_paldarklab.py` → expect `OK`.

### Anti-patterns avoided (W40-41)

- **Hardcoding `MaxPlayers = 8` in `UPaldarkNetSubsystem`** — Would couple the cap to the topology rather than the experience. Future Q4 weeks may want different shard sizes per hub variant (e.g. a 16-player social plaza vs an 8-player raid prep room); per-experience UPROPERTY scales cleanly.
- **Replicating the hub subsystem state** — `RegisteredBuildings` is server-only; clients see hub buildings as ordinary level-placed actors and route interact via their own pawn → server RPC chain. Replicating the registry would duplicate level data that clients already have, and force `bReplicates = true` on every building (which currently has no per-frame replicated state at all).
- **Calling `ServerTravel` synchronously from `OnInteract`** — Recursing the world tick into `UEngine::TickFlush` while still mid-interact triggers asserts in dedicated builds. The briefing room uses `SetTimerForNextTick` so the travel runs at the next tick boundary, free of any actor's tick window.
- **One actor class per building type (no shared base)** — Would force duplicating the soft-ref + sphere + register pattern three times. The base + subclass split keeps the per-type code to ~25 lines each (`BuildingTag` stamp in ctor + a placeholder `OnInteract` log).
- **Inlining the `Paldark.Hub.*` console commands in `PaldarkLab.cpp` startup** — The subsystem owns the commands so they auto-unregister on world teardown (designer changes map → old console handlers don't leak into a fresh subsystem). Mirrors the W31-32 POI console pattern.
- **Skipping the `bReplicates = false` on hub buildings** — Default `bReplicates = true` on `AActor` would create per-building replication slots on the wire for actors that have no replicated state. `bReplicates = false` keeps the wire format minimal; clients still see the buildings because they're placed in the level (replicated at level-load time, not per-frame).
- **Letting hub subsystem run on clients** — `ShouldCreateSubsystem` early-outs on `NM_Client` so the registry only exists on the server. Clients that try `World->GetSubsystem<UPaldarkHubSubsystem>()` get null, and any client-side caller has to RPC to the server to interrogate state. Matches W31-32 / W24-25 server-only subsystem pattern.

### Out of scope (defer to later Q4 weeks / designer)

- **The actual `Map_HubTown.umap`** — Designer authors in UE Editor; the Devin VM has no UE compiler. Placeholder DA + map work is roughed out in the test-loop steps above.
- **UMG widgets for stable / marketplace / briefing UI** — UI team task. The hub building actors emit log lines on interact; W44+ adds the widget classes.
- **Mod io / marketplace transaction backend** — W45+ (the marketplace catalog is just a soft-ref array in the DA today; no buy/sell server RPC).
- **AWS fleet picker for hub-vs-raid dedicated server selection** — W42-43; `HostHubServer` is the seam this PR ships to make that branch cheap.
- **Per-experience GameMode alias entries in `DefaultEngine.ini`** — A `+GameModeClassAliases=(Name="Hub", GameMode=...)` row is the cleanest way to launch via URL flag, but the L-01 contract already covers `RequiredGameModeClass` on the experience. Adding the alias is a 1-line change deferred to W42-43 where it lives next to the AWS fleet wiring.
- **Briefing-room map *thumbnail* / preview UI** — Designer authors thumbnails on `UPaldarkMapDefinition` directly; this PR's briefing room reads `MapName` only.
- **Hub town save/restore (player position, inventory state on re-entry)** — Save-game is the W47 milestone; the hub today is a stateless room.

## W42-43 — AWS backend integration (Q4 feature week #2)

Roadmap line item: "Cognito login + fleet allocation Lambda + DynamoDB session table → client knows which dedicated server to ClientTravel into." Ships the **client-side scaffold + mocked fallback**; the actual Lambda deployment, Cognito User Pool provisioning, DynamoDB table, and IAM policy live in a separate SRE-owned Terraform repo and land at W46+. The C++ side in this PR is the wire format + the request/response/refresh state machine that talks to that future backend.

### What this PR ships

| Layer | File | Role |
|------|------|------|
| Types | `Source/PaldarkLab/Public/Backend/PaldarkBackendTypes.h` | 3 `USTRUCT(BlueprintType)` — `FPaldarkSessionToken`, `FPaldarkFleetSpec`, `FPaldarkBackendError`. All `BlueprintReadOnly` so UMG binds directly. |
| Settings | `Source/PaldarkLab/Public/Backend/PaldarkBackendSettings.h` + `.cpp` | `UDeveloperSettings` exposing 9 knobs (`bUseAWSBackend`, `AWSRegion`, `LambdaApiBaseUrl`, `CognitoUserPoolId`, `CognitoAppClientId`, `RequestTimeoutSeconds`, `MaxRetries`, `RetryBackoffSeconds`, `TokenRefreshLeadSeconds`). Surfaces in Project Settings → Plugins → Paldark Backend. |
| Subsystem | `Source/PaldarkLab/Public/Backend/PaldarkBackendSubsystem.h` + `.cpp` | `UGameInstanceSubsystem` — same lifetime as `UPaldarkNetSubsystem`. 6 API methods (`Login`, `Logout`, `RequestHubFleet`, `RequestRaidFleet`, `RefreshToken`, `GetStatusReport`), 4 `BlueprintAssignable` delegates (`OnLoginCompleted`, `OnFleetRequestCompleted`, `OnTokenRefreshed`, `OnBackendError`). HTTP via `FHttpModule`, JSON via `FJsonObjectConverter`. |
| Net hook | `Source/PaldarkLab/Public/Net/PaldarkNetSubsystem.h` (modified) | New `HostRaidServer` + `JoinFleetByFleetSpec` + private `RequestFleetAndTravel`. `HostHubServer` now branches via `RequestFleetAndTravel(Paldark.Backend.Fleet.Hub, ...)`. |
| Console | `Source/PaldarkLab/Private/Backend/PaldarkBackendSubsystem.cpp` + `Net/PaldarkNetSubsystem.cpp` | 6 commands — `Paldark.Backend.{Login, RequestHubFleet, RequestRaidFleet, Status}` + `Paldark.Net.{HostHub, HostRaid}`. Owned by the relevant subsystem; auto-unregister on `Deinitialize`. |
| Tags | `Source/PaldarkLabCore/Public/PaldarkGameplayTags.h` + `.cpp` (modified) | 12 new tags — 4 `State` (`LoggedOut`, `Authenticating`, `LoggedIn`, `Refreshing`), 5 `Result` (`Success`, `Fail.Network`, `Fail.Auth`, `Fail.Timeout`, `Fail.NoFleet`), 3 `Fleet` (`Hub`, `Raid`, `Reserved`). |
| Log | `Source/PaldarkLabCore/Public/PaldarkLogCategories.h` + `Private/PaldarkLabCore.cpp` (modified) | New `LogPaldarkBackend` category — kept distinct from `LogPaldarkNet` so the noisy HTTP request/response lines do not drown out the W14-15 listen-server / W16-17 lag-comp logs that designers grep. |
| Config | `Config/DefaultGame.ini` + `DefaultEngine.ini` (modified) | `[/Script/PaldarkLab.PaldarkBackendSettings]` block with 9 default keys + `LogPaldarkBackend=Verbose` under `[Core.Log]`. Shipped with `bUseAWSBackend=False`. |
| Build | `Source/PaldarkLab/PaldarkLab.Build.cs` (modified) | New `PrivateDependencyModuleNames`: `HTTP`, `Json`, `JsonUtilities`, `DeveloperSettings`. |
| Validator | `scripts/ci/validate_paldarklab.py` (modified) | New `check_w42_backend_shape` (~320 lines) + 12 tags added to `EXPECTED_GAMEPLAY_TAGS` + `LogPaldarkBackend` added to `EXPECTED_LOG_CATEGORIES`. |

### Subsystem lifetime + state machine

`UPaldarkBackendSubsystem` is a `UGameInstanceSubsystem` because the session token must survive map travel (`UWorldSubsystem` would die on every map switch and force a re-login between hub and raid). Lifetime matches `UPaldarkNetSubsystem` so the two cooperate without ordering risk.

State tag transitions (all logged under `LogPaldarkBackend` with `[State]` prefix):

```
LoggedOut ──Login()──> Authenticating ──HTTP 200──> LoggedIn ──┐
   ▲                          │                                │
   │                          └──HTTP 4xx──> LoggedOut         │
   │                                                           │
   └──Logout() / Refresh fail──────────────────────────────────┤
                                                               │
                       Refreshing <──refresh timer / manual────┘
                            │
                            └──HTTP 200──> LoggedIn
                            └──HTTP 4xx──> LoggedOut
```

The 5-minute-before-expiry refresh timer is set inside `Login` via `FTimerManager::SetTimer`. Cleared on `Logout` + on `Deinitialize` (so a map travel mid-refresh does not stall a stale timer onto the next world).

### Mocked fallback (default — `bUseAWSBackend=False`)

Every API method has a parallel "mock" path that fires on the next tick via `FTimerManager::SetTimerForNextTick`. This keeps the W40-41 quick-test loop bit-identical: a designer with no AWS provisioned can still `Paldark.Net.HostHub`, see the briefing room hand off to a raid map, and exercise the whole UMG flow that binds to `OnFleetRequestCompleted`.

- `Login("foo@bar.com", "...")` → next tick → `OnLoginCompleted(synthetic token { AccessToken="mock-jwt", UserId="mock-user", ExpirySeconds=now+3600 }, { Success, 200 })`.
- `RequestHubFleet("PX_HubTown")` → next tick → `OnFleetRequestCompleted(FPaldarkFleetSpec::MakeLoopback(Hub, "PX_HubTown"), { Success, 200 })`.
- `RequestRaidFleet("PX_RaidSandbox")` → next tick → `OnFleetRequestCompleted(FPaldarkFleetSpec::MakeLoopback(Raid, "PX_RaidSandbox"), { Success, 200 })`.
- `RefreshToken()` → next tick → `OnTokenRefreshed(synthetic token + 3600s expiry)`.

The loopback `FleetSpec` resolves to `127.0.0.1:7777` so `JoinFleetByFleetSpec` → `JoinServer("127.0.0.1:7777")` matches the local quick-test path. If the local dedicated is not actually running, the client falls back to `HostListenServer` automatically (see `RequestFleetAndTravel` listener body).

### HTTP path (when `bUseAWSBackend=True`)

`IssueHttpRequest(Verb, Path, JsonBody, bAuthenticated, Callback)` is the single chokepoint. It:

1. Reads `LambdaApiBaseUrl` from the settings CDO. If empty, logs Warning + falls back to mocked path (so flipping the toggle without configuring the URL is recoverable).
2. Builds the URL: `<LambdaApiBaseUrl>/<Path>` (slash-normalised). `Path` is e.g. `sessions/login`, `sessions/hub`, `sessions/raid`, `sessions/refresh`.
3. Sets headers: `Content-Type: application/json`, `Accept: application/json`, `X-Paldark-Pool: <CognitoUserPoolId>`, `X-Paldark-Region: <AWSRegion>`, and if `bAuthenticated` and `SessionToken.IsValid()`, `Authorization: Bearer <AccessToken>`.
4. Sets `Timeout(RequestTimeoutSeconds)` so a stuck Lambda cold-start does not block forever (default 15s — chosen to absorb the typical p99 ~5s cold start while still surfacing real outages quickly).
5. Issues the request, stores the `IHttpRequest` shared ref in `InFlightRequests` so `Deinitialize` can cancel it on map travel.
6. On response, the callback dispatches based on HTTP status to a `FPaldarkBackendError` envelope:

   | HTTP status | ResultTag |
   |-------------|-----------|
   | 200..299    | `Paldark.Backend.Result.Success` |
   | 401 / 403   | `Paldark.Backend.Result.Fail.Auth` |
   | 404 / 503   | `Paldark.Backend.Result.Fail.NoFleet` (server has no spare capacity) |
   | 408 / 504   | `Paldark.Backend.Result.Fail.Timeout` |
   | 5xx (else)  | retry up to `MaxRetries` with `N * RetryBackoffSeconds` linear backoff, then `Fail.Network` |
   | Network err | `Paldark.Backend.Result.Fail.Network` |

   `Login` + `Logout` skip the retry loop (security — re-sending credentials on a 5xx is a credential-stuffing vector for a misconfigured Lambda). Only `RequestHubFleet` / `RequestRaidFleet` retry.

7. On success, deserialises the JSON body via `FJsonObjectConverter::JsonObjectStringToUStruct` into the appropriate result struct + fires the matching delegate.

### Wire format (request / response)

All bodies are flat JSON; field names match the `UPROPERTY` names exactly so `FJsonObjectConverter` does not need a custom mapper. Documented for SRE handoff:

```
POST /sessions/login
  Body:   { "Email": "...", "Password": "...", "AppClientId": "<CognitoAppClientId>" }
  Resp:   { "AccessToken": "...", "RefreshToken": "...", "UserId": "...", "ExpirySeconds": 1700000000 }

POST /sessions/hub        ──┐
POST /sessions/raid        ─┴─ Body:   { "ExperienceId": "PX_...", "ShardKey": "<empty for raid>" }
                              Resp:   { "FleetId": "fleet-...", "IP": "10.0.0.42", "Port": 33430,
                                        "FleetTag": "Paldark.Backend.Fleet.{Hub|Raid}",
                                        "ExperienceId": "PX_...", "ShardKey": "<sticky for hub>" }

POST /sessions/refresh
  Body:   { "RefreshToken": "..." }
  Resp:   { "AccessToken": "...", "ExpirySeconds": 1700003600 }

POST /sessions/logout       (best-effort; no body)
```

### `UPaldarkNetSubsystem` integration

`HostHubServer` and `HostRaidServer` are now thin wrappers around `RequestFleetAndTravel(FleetTag, MapName, ExperienceId)`:

1. Look up `UPaldarkBackendSubsystem` from the GameInstance. If null (defensive — shouldn't happen because the subsystem auto-initialises), fall back to `HostListenServer` so the dev loop never breaks.
2. Add a one-shot lambda listener to `OnFleetRequestCompleted`. The lambda captures the requested `FleetTag` and ignores responses whose tag does not match — this defends against the overlapping-request case (two `Paldark.Net.HostHub` invocations queue, second one's response wakes up the first one's listener).
3. Inside the lambda, the listener removes itself, then either:
   - On success: calls `JoinFleetByFleetSpec(Spec)` → builds `IP:Port?Experience=<id>` and issues `JoinServer`.
   - On failure: logs Warning, falls back to `HostListenServer(MapName, ExperienceId)` so the user gets *some* session even if the backend is down.
4. Issues the request: `Backend->RequestHubFleet(ExperienceId)` or `Backend->RequestRaidFleet(ExperienceId)`.

The W40-41 briefing-room handoff still calls `HostHubServer` — the W42-43 wire is invisible to that callsite. Designer authoring is unchanged; the only difference is "with the toggle on, the travel goes to a real GameLift fleet."

### Console commands (designer reference)

| Command | Usage | Effect |
|---------|-------|--------|
| `Paldark.Backend.Login` | `Paldark.Backend.Login <email> <password>` | Issues login. Logs result + state transition. |
| `Paldark.Backend.RequestHubFleet` | `Paldark.Backend.RequestHubFleet [ExperienceId]` | Issues hub fleet request. Logs returned `FleetSpec` + ServerTravel target. |
| `Paldark.Backend.RequestRaidFleet` | `Paldark.Backend.RequestRaidFleet [ExperienceId]` | Same as above, raid pool. |
| `Paldark.Backend.Status` | `Paldark.Backend.Status` | Multi-line dump: state tag, cached token validity, in-flight request count, settings snapshot. |
| `Paldark.Net.HostHub` | `Paldark.Net.HostHub [MapName] [ExperienceId]` | Convenience — goes through the full Backend → fleet → travel path. Default `Map_HubTown` / `PX_HubTown`. |
| `Paldark.Net.HostRaid` | `Paldark.Net.HostRaid [MapName] [ExperienceId]` | Same, raid pool. Default `Raid_Sandbox` / `PX_RaidSandbox`. |

### Designer test loop (in UE Editor, after this PR lands)

1. Pull this branch, **Generate Project Files**, build PaldarkLab + Editor.
2. **Mocked path** (no AWS needed) — leave `Project Settings → Plugins → Paldark Backend → Use AWS backend = false`.
   - Open PIE on `Raid_Sandbox.umap`.
   - Console: `Paldark.Backend.Status` → expect `State=Paldark.Backend.State.LoggedOut, Token=invalid, InFlight=0, bUseAWSBackend=false`.
   - Console: `Paldark.Backend.Login dev@paldark.local dev` → expect `OnLoginCompleted` log line + state → `LoggedIn` + mock token cached.
   - Console: `Paldark.Backend.RequestHubFleet PX_HubTown` → expect `OnFleetRequestCompleted` with `IP=127.0.0.1 Port=7777 Tag=Paldark.Backend.Fleet.Hub`.
   - Console: `Paldark.Net.HostHub Map_HubTown PX_HubTown` → expect full chain: log `[W40-41/W42-43][Hub]` → backend request → loopback FleetSpec → `JoinServer("127.0.0.1:7777")`. With no real listen server, the client falls back to `HostListenServer` automatically.
   - Console: `Paldark.Backend.Status` → expect `State=LoggedIn, Token=valid, InFlight=0`.
3. **Live path** (requires SRE-provisioned Lambda + Cognito) — flip `bUseAWSBackend=true`, set `LambdaApiBaseUrl=https://abc123.execute-api.ap-northeast-1.amazonaws.com/prod`, `CognitoAppClientId=...`, `CognitoUserPoolId=...`.
   - Re-run step 2; expect identical logs but with real `FleetSpec.IP` from GameLift + real Cognito-issued tokens.
   - Verify token refresh: set `TokenRefreshLeadSeconds=3500` (almost the full hour) — expect a `Refreshing` state transition ~60s after login.
4. **Failure path stress test** — set `LambdaApiBaseUrl` to a deliberately bad URL (`http://127.0.0.1:1`) → expect `OnBackendError` with `ResultTag=Paldark.Backend.Result.Fail.Network` after `MaxRetries` retries; `Paldark.Net.HostHub` falls back to `HostListenServer`.

### Risk + mitigation

| Risk | Mitigation |
|------|------------|
| JWT expires mid-session | 5-minute-before-expiry refresh timer set on login. `RefreshToken` is also exposed manually for designer testing. |
| Lambda cold start (5s+) blocks `HostHubServer` | `RequestTimeoutSeconds=15` covers p99 cold start. Future SRE wire: scheduled CloudWatch warm-up ping every 4min. |
| Replay attack on cached `AccessToken` | Token is process-memory only (no `Saved/`, no `Config/`). `Logout` zeroes the cache. SessionToken not Replicated — server never sees the client's token. |
| Player loses connection mid-fleet-request | `Deinitialize` cancels every `InFlightRequest`. `FTimerHandle` for refresh also cleared on shutdown. |
| Designer flips `bUseAWSBackend=true` without filling `LambdaApiBaseUrl` | Empty URL is treated as a config error: log Warning + fall back to mocked path. The Status console command prints both flags so they can diagnose. |
| Stale `FDelegateHandle` after subsystem teardown | `TWeakObjectPtr<UPaldarkNetSubsystem>` capture + `if (!WeakThis.IsValid()) return;` guard in the one-shot listener body. |
| Overlapping `HostHub` + `HostRaid` requests interleave | Listener inspects `FleetSpec.FleetTag` and ignores responses for the other request kind. |

### Validator extension

`check_w42_backend_shape` (~320 lines) covers:

- All 4 new files exist (`Public/Backend/PaldarkBackendTypes.h`, `Settings.h`, `Settings.cpp`, `Subsystem.h`, `Subsystem.cpp`).
- `PaldarkBackendTypes.h` declares all 3 structs with the right BlueprintType wiring + field types + `MakeLoopback` / `IsValid` accessors.
- `PaldarkBackendSettings.h` declares the right UCLASS specifier (`Config=Game, DefaultConfig`) + 9 UPROPERTY knobs.
- `PaldarkBackendSubsystem.h` declares all 6 API methods + 4 BlueprintAssignable delegates + Initialize/Deinitialize overrides.
- `PaldarkBackendSubsystem.cpp` registers all 4 console commands + uses `LogPaldarkBackend` + has the `MakeLoopback` fallback + `TransitionState` helper.
- `PaldarkNetSubsystem.h` declares `HostRaidServer`, `JoinFleetByFleetSpec`, the `UPaldarkBackendSubsystem` forward decl + the `Backend/PaldarkBackendTypes.h` include.
- `PaldarkNetSubsystem.cpp` includes `Backend/PaldarkBackendSubsystem.h` + defines `HostRaidServer` / `JoinFleetByFleetSpec` / `RequestFleetAndTravel` + registers `Paldark.Net.HostHub` / `Paldark.Net.HostRaid` + invokes `Backend->RequestHubFleet` + `Backend->RequestRaidFleet`.
- `PaldarkLab.Build.cs` lists `HTTP`, `Json`, `JsonUtilities`, `DeveloperSettings`.
- `PaldarkLogCategories.h` declares `LogPaldarkBackend`; `PaldarkLabCore.cpp` defines it.
- `DefaultGame.ini` has the `[/Script/PaldarkLab.PaldarkBackendSettings]` block with all 9 defaults; `bUseAWSBackend=True` is rejected (cold-clone safety — `bUseAWSBackend=False` is the only valid default).
- 12 new gameplay tags enforced via `check_gameplay_tags`.
- `LogPaldarkBackend` enforced via `check_log_categories`.

Run locally: `python3 scripts/ci/validate_paldarklab.py` → expect `OK`.

### Anti-patterns avoided (W42-43)

- **Using `UEngineSubsystem` instead of `UGameInstanceSubsystem`** — Engine subsystems survive even editor PIE teardown, but they also persist across user-switch / multi-PIE → leak tokens between sessions. GameInstance scope matches the natural "one logged-in user per running instance" mental model.
- **Blocking inside `HostHubServer` until the HTTP roundtrips** — A synchronous wait would freeze the briefing-room interact thread + cause `[Movement] Server Move` resimulation. Async delegate-driven path keeps the game thread free.
- **Caching `LambdaApiBaseUrl` at `Initialize` instead of reading the CDO every call** — Designers want to flip the URL in PIE without restarting the editor. CDO read every call is ~5ns; the overhead is rounding error vs the HTTP roundtrip.
- **Logging `AccessToken` in plain text** — Even at Verbose, the token leaks into Saved logs that get attached to bug reports. We log `Token=<empty|valid|invalid>` only; never the raw string.
- **Retrying `Login` on 5xx** — Repeating credentials onto a Lambda that's misbehaving is a credential-stuffing vector. Only fleet requests retry; login propagates 5xx immediately as `Fail.Network`.
- **Replicating `FPaldarkSessionToken`** — Token is `UPROPERTY(Transient)` and the subsystem itself is client-only state; server never sees the player's bearer token (the dedicated server validates against Cognito JWKS itself in W46+). Replication would leak tokens across clients in PIE.
- **Storing the token in `Saved/` to survive crashes** — A token in plain text on disk is a much bigger attack surface than re-logging in. Crashes are rare; re-login is cheap.
- **Wiring console commands in `FPaldarkLabModule::StartupModule`** — Would force the console handlers to look up the subsystem every call (and re-register on every PIE start). Subsystem-owned commands auto-scope to the GameInstance lifetime — mirrors the W31-32 POI + W40-41 Hub Subsystem pattern.
- **Coupling refresh timer to `World->GetTimerManager()`** — World-bound timers die on map travel, which is exactly the case we need to survive. The subsystem stores `FTimerHandle` on itself and uses `GetGameInstance()->GetTimerManager()` (or a manual `FDateTime::UtcNow()` polling check on `Tick`, if we ever need to remove the world-level timer dependency).
- **Hardcoding the fleet allocation policy (e.g. "always pick the lowest-latency region")** — The backend owns the picker; the client just consumes whatever IP+Port the Lambda returns. This keeps Erlang-style server-side rollout (canary fleets, region failover) invisible to the client.
- **Letting `bUseAWSBackend=True` ship in `DefaultGame.ini`** — Cold-clone CI would burn on every PR trying to hit a non-existent Lambda. Validator rejects `bUseAWSBackend=True` as a default; SRE flips it per-config via `DefaultGame_Shipping.ini` or env-driven `INI` override.

### Out of scope (defer to later Q4 weeks / SRE)

- **Lambda implementation** — Owned by SRE. Functions: `sessions-login`, `sessions-hub`, `sessions-raid`, `sessions-refresh`, `sessions-logout`. Behind API Gateway HTTP API (cheaper than REST API). Deploy via Terraform; URL flows into `LambdaApiBaseUrl` env var.
- **Cognito hosted login UI (deep-link callback)** — W46+. Today's path is plain-text email+password which is dev-only; logs WARN every call. The hosted UI flow swaps `Login` for `LoginWithCognitoCode(authCode)` once OAuth is wired.
- **DynamoDB session table** — Owned by SRE. Single-table design: `PK=user#<UserId>, SK=session#<SessionId>` for login state + `PK=fleet#<FleetId>, SK=alloc#<Slot>` for capacity tracking. The client never reads/writes Dynamo directly.
- **GameLift Fleet provisioning** — Owned by SRE. Two fleets (hub: 8-player cap, on-demand; raid: 4-player cap, spot). Auto-scaling on `PercentCapacityActive > 70`. The Lambda picks an active server slot and returns IP+Port.
- **Server-side dedicated `fleet register` ping** — When the dedicated server boots it'll POST `/fleet/register` to tell the Lambda "I'm online, here's my slot." Separate Q4 week (server-side code, mirrors this PR's shape).
- **mod.io / Steam Workshop UI** — W44-45 (marketplace UI feature week).
- **EOS / Steam Auth fallback** — Q4 stretch goal. The plan is to keep Cognito as the source of truth and bridge other identity providers as additional Cognito IdPs (Cognito supports both as federated IdPs natively).
- **Telemetry to CloudWatch / Sentry** — Future. Today every backend action logs to `LogPaldarkBackend`; piping that to CloudWatch is a build-time `UE_LOG` interceptor in the shipping build.
- **Per-region URL pinning** — `AWSRegion` knob is purely informational today (logs only). When SRE deploys multi-region, a follow-up PR adds a region resolver that maps `AWSRegion → LambdaApiBaseUrl_<Region>`.

## W44-45 — Stable & Marketplace UI (Q4 feature week #3)

Roadmap line item: "Pal Stable (deposit/withdraw/heal) + Marketplace kiosk (buy/sell items for credits) — interactive hub-building UI." Ships the **C++ scaffold + console-driven test loop** so designers can author the UMG widget layouts as Blueprint subclasses of the C++ stubs in the next sprint. The two hub buildings stamped in W40-41 (`APaldarkPalStable`, `APaldarkMarketplaceKiosk`) now have working `OnInteract` paths that route into per-player controller components and end-to-end server RPC handlers.

### What this PR ships

| Layer | File | Role |
|------|------|------|
| Item fragment | `Source/PaldarkLab/Public/Inventory/Fragments/PaldarkItemFragment_MarketValue.h` + `.cpp` | `UPaldarkItemFragment` subclass carrying 4 fields: `BuyPriceCredits`, `SellPriceCredits`, `bAllowBuy`, `bAllowSell`. Designer attaches this to any `UPaldarkItemDefinition` they want marketable; the marketplace controller reads it via `FindFragmentByClass<>`. |
| Deposit storage | `Source/PaldarkLab/Public/Pal/PaldarkPalDepositComponent.h` + `.cpp` | `UActorComponent` on `APaldarkPlayerState`. Holds `TArray<FPaldarkRosterEntry> DepositedEntries` (replicated `COND_OwnerOnly`). Survives pawn death + respawn (the active roster lives on the Character). `MaxDepositSlots` designer cap (default 24). 3 server APIs (`DepositEntry`, `WithdrawEntry`, `DumpToLog`). |
| Stable controller | `Source/PaldarkLab/Public/Hub/PaldarkStableController.h` + `.cpp` | `UActorComponent` on `APaldarkPlayerController`. 4 client APIs (`RequestOpenStable`, `RequestDeposit`, `RequestWithdraw`, `RequestHeal`), 3 Server RPCs (`Server_RequestDeposit/Withdraw/Heal`), 1 Client RPC (`Client_ReceiveActionResult`), 3 `BlueprintAssignable` delegates (`OnStableOpened`, `OnStableClosed`, `OnStableActionResult`). `HealPriceCredits` designer knob (default 50). `bIsTransactionInFlight` guard rejects overlapping RPCs. |
| Marketplace controller | `Source/PaldarkLab/Public/Hub/PaldarkMarketplaceController.h` + `.cpp` | `UActorComponent` on `APaldarkPlayerController`. 4 client APIs (`RequestOpenMarketplace`, `RequestCloseMarketplace`, `RequestBuy`, `RequestSell`), 2 Server RPCs (`Server_RequestBuy/Sell`), 1 Client RPC (`Client_ReceiveTransactionResult`), 3 `BlueprintAssignable` delegates (`OnMarketplaceOpened`, `OnMarketplaceClosed`, `OnMarketplaceActionResult`). Async catalog load via `FStreamableManager::RequestAsyncLoad` so designer-authored soft refs do not block the briefing thread. |
| Widget stubs | `Source/PaldarkLab/Public/Hub/PaldarkStableWidget.h` + `.cpp`, `PaldarkMarketplaceWidget.h` + `.cpp` | `UUserWidget` C++ bases. Each declares `BlueprintImplementableEvent` hooks (`K2_OnStableOpened`, `K2_OnStableActionResult`, `K2_OnStableClosed` + marketplace equivalents) plus `BlueprintPure` accessor for the matching controller. Designer authors `WBP_PaldarkStableWidget` / `WBP_PaldarkMarketplaceWidget` as Blueprint subclasses. |
| Hub building wiring | `Source/PaldarkLab/Private/Hub/PaldarkPalStable.cpp` + `PaldarkMarketplaceKiosk.cpp` (modified) | `OnInteract` now resolves `Interactor->GetStableController()` / `GetMarketplaceController()` and calls `RequestOpenStable(this)` / `RequestOpenMarketplace(this)`. W40-41 was a logging no-op; this PR wires it end-to-end. |
| PlayerController | `Source/PaldarkLab/Public/Player/PaldarkPlayerController.h` + `.cpp` (modified) | Default-subobject creation of both controllers in ctor + `GetStableController` / `GetMarketplaceController` `BlueprintPure` accessors. |
| PlayerState | `Source/PaldarkLab/Public/Player/PaldarkPlayerState.h` + `.cpp` (modified) | Default-subobject creation of `UPaldarkPalDepositComponent` in ctor + `GetPalDepositComponent` `BlueprintPure` accessor. |
| Roster helpers | `Source/PaldarkLab/Public/Pal/PaldarkPlayerPalRosterComponent.h` + `.cpp` (modified) | 3 new authority APIs: `RemoveEntryByIndex`, `AppendEntry`, `HealEntryToFull`. Used by `UPaldarkStableController::Server_RequestDeposit/Withdraw/Heal` to move entries between the active roster + the deposit list. |
| Tags | `Source/PaldarkLabCore/Public/PaldarkGameplayTags.h` + `.cpp` (modified) | 14 new tags — 4 UI state (`Paldark.Hub.UI.{Closed, OpeningStable, OpeningMarket, Open}`), 3 stable actions (`Paldark.Hub.Stable.Action.{Deposit, Withdraw, Heal}`), 2 market actions (`Paldark.Hub.Market.Action.{Buy, Sell}`), 5 shared results (`Paldark.Hub.Result.{Success, Fail.Empty, Fail.Full, Fail.Insufficient, Fail.NotAllowed}`). |
| Log | `Source/PaldarkLabCore/Public/PaldarkLogCategories.h` + `Private/PaldarkLabCore.cpp` (modified) | New `LogPaldarkUI` category — kept separate so the noisy "click → server RPC → client callback → widget refresh" trace does not bleed into `LogPaldark` when designers are debugging marketplace pricing or stable swaps. |
| Build | `Source/PaldarkLab/PaldarkLab.Build.cs` (modified) | New `"UMG"` private dependency for `UUserWidget` base class of the widget stubs. |
| Console | `Source/PaldarkLab/PaldarkLab.cpp` (modified) | 8 new commands — `Paldark.Hub.Stable.{List, Deposit, Withdraw, Heal}` + `Paldark.Hub.Market.{Catalog, Buy, Sell, Balance}`. Owned by the module; auto-unregister on `ShutdownModule`. |
| Validator | `scripts/ci/validate_paldarklab.py` (modified) | New `check_w44_stable_marketplace_shape` (~450 lines) + 14 tags added to `EXPECTED_GAMEPLAY_TAGS` + `LogPaldarkUI` added to `EXPECTED_LOG_CATEGORIES`. |

### Component ownership map

```
APaldarkPlayerController
├── UPaldarkStableController        (ctor default-subobject; per-player UI state)
└── UPaldarkMarketplaceController   (ctor default-subobject; per-player UI state)

APaldarkPlayerState
├── UPaldarkAbilitySystemComponent  (W7-8)
├── UPaldarkAttributeSet            (W7-8)
└── UPaldarkPalDepositComponent     (ctor default-subobject; W44-45)

APaldarkCharacter (the Pawn)
├── UPaldarkPlayerInventoryComponent  (W11-12 → W37-38)
└── UPaldarkPlayerPalRosterComponent  (W35-36)
```

Why this split:

- **Active roster on the Pawn** — Lives on `APaldarkCharacter` because it follows the player's combat loadout; if the Pal dies during a raid the roster entry reflects post-combat health.
- **Deposit list on the PlayerState** — Lives on `APaldarkPlayerState` so the deposit list survives the player respawning between raids without re-fetching from a save game. W47 wires the cross-match persistence on top of this.
- **UI state on the PlayerController** — Lives on `APaldarkPlayerController` (not the Pawn) so the UI state survives respawn. The controller is also the natural RPC target — Server RPCs from a UMG widget reach the player's PC, not the Pawn.

### End-to-end flow — `Paldark.Hub.Stable.Deposit 0`

```
1. Designer presses E next to APaldarkPalStable (W40-41 interact)
   └─ APaldarkPalStable::OnInteract(PC)
       └─ PC->GetStableController()->RequestOpenStable(this)
           ├─ UIStateTag = Paldark.Hub.UI.OpeningStable
           ├─ ActiveStable = this
           └─ OnStableOpened.Broadcast(this)
               └─ WBP_PaldarkStableWidget::K2_OnStableOpened(this)
                   └─ Designer-authored UMG mount

2. Player clicks "Deposit slot 0" → C++ widget calls:
   StableController->RequestDeposit(0)
       └─ Server_RequestDeposit_Validate / _Implementation(0)
           ├─ bIsTransactionInFlight = true
           ├─ Roster->RemoveEntryByIndex(0) → FPaldarkRosterEntry
           ├─ Deposit->DepositEntry(entry) → NewDepositIdx
           ├─ Client_ReceiveActionResult(Action.Deposit, Result.Success, entry, NewIdx)
           └─ bIsTransactionInFlight = false
               └─ OnStableActionResult.Broadcast(...)
                   └─ WBP_PaldarkStableWidget::K2_OnStableActionResult(...)
                       └─ Designer refreshes UI list

3. Marketplace flow is symmetric — RequestBuy / Sell on the controller,
   server validates against the active kiosk's MarketplaceCatalog +
   UPaldarkItemFragment_MarketValue pricing, debits/credits the player's
   credits via the inventory currency tag, and round-trips the result
   through Client_ReceiveTransactionResult.
```

### Replication + race-condition contract

| Risk | Mitigation |
|------|------------|
| Two `RequestDeposit` RPCs fire from the same client before the first resolves (double-click) | `bIsTransactionInFlight` server-side flag rejects the second with `Result.Fail.NotAllowed`; client-side, UMG disables the button on `RequestDeposit` and re-enables on `OnStableActionResult`. |
| `Withdraw` arrives but the active roster is already at the per-experience cap | `Roster->AppendEntry` checks `RosterSlotCount` from the experience-resolved `UPaldarkPawnData`; returns false. Server fires `Result.Fail.Full` and **does not drop the Pal** — it stays in the deposit list. Same pattern as W37-38 inventory weight cap. |
| `Buy` arrives but inventory weight cap blocks `AddItem` | Server reserves credits first via `ServerDebitCredits`, calls `Inv->AddItem(SoftDef, Count)`, refunds the difference on partial settle. `Result.Fail.Full` on zero settle; `Result.Success` on partial+. |
| `Sell` arrives but the player no longer has the item | `Inv->RemoveItemByTag(ItemTag, Count)` returns 0 → `Result.Fail.Insufficient`, no credits change. |
| Other players see my deposit list | `DepositedEntries` uses `DOREPLIFETIME_CONDITION(..., COND_OwnerOnly)`; matches the existing `UPaldarkPlayerPalRosterComponent::Entries` privacy gate. |
| Player closes the marketplace widget while the catalog async-load is in flight | `OnCatalogLoadCompleted` checks `ActiveKiosk` — if null, drops the snapshot silently. `CatalogLoadHandle.Reset()` releases the streamable handle. |
| Marketplace catalog soft refs not loaded yet on a fresh hub entry | `RequestOpenMarketplace` issues `Streamable.RequestAsyncLoad(...)`; `OnMarketplaceOpened` fires only after `OnCatalogLoadCompleted`. Loopback to "empty catalog" handled cleanly. |

### Console commands (designer reference)

| Command | Usage | Effect |
|---------|-------|--------|
| `Paldark.Hub.Stable.List` | `Paldark.Hub.Stable.List` | Dump active roster + deposit list to `LogPaldarkUI`. |
| `Paldark.Hub.Stable.Deposit` | `Paldark.Hub.Stable.Deposit <RosterIndex>` | Server RPC: move active roster entry [RosterIndex] into the deposit list. |
| `Paldark.Hub.Stable.Withdraw` | `Paldark.Hub.Stable.Withdraw <DepositIndex>` | Server RPC: move deposit entry [DepositIndex] back into active roster. |
| `Paldark.Hub.Stable.Heal` | `Paldark.Hub.Stable.Heal <RosterIndex>` | Server RPC: debit `HealPriceCredits` credits + refill `HealthAtTame` on roster entry [RosterIndex]. |
| `Paldark.Hub.Market.Catalog` | `Paldark.Hub.Market.Catalog [KioskFilter]` | Dump the nearest marketplace kiosk's catalog + per-item buy/sell prices. Optional substring filter on actor name. |
| `Paldark.Hub.Market.Buy` | `Paldark.Hub.Market.Buy <Paldark.Item.X> [Count=1]` | Server RPC: buy `Count` of item tag from the active kiosk. |
| `Paldark.Hub.Market.Sell` | `Paldark.Hub.Market.Sell <Paldark.Item.X> [Count=1]` | Server RPC: sell `Count` of item tag to the active kiosk. |
| `Paldark.Hub.Market.Balance` | `Paldark.Hub.Market.Balance` | Log the local player's current `Paldark.Item.Resource.Currency.Credits` balance. |

### Designer test loop (in UE Editor, after this PR lands)

1. Pull this branch, **Generate Project Files**, build PaldarkLab + Editor.
2. Open PIE on `Map_HubTown.umap` (designer-authored placeholder — see W40-41 § "Designer follow-ups").
3. Tame a couple of Pals on `Raid_Sandbox`, then `Paldark.Hub.Stable.List` → expect 2 active, 0 deposited.
4. Walk up to the stable kiosk (`APaldarkPalStable` instance), press E → expect log line `[Hub.PalStable ...] OnInteract by PC... — RosterSlotCount=N` followed by `OnStableOpened` broadcast.
5. Console: `Paldark.Hub.Stable.Deposit 0` → expect `OnStableActionResult` log with `Action=Paldark.Hub.Stable.Action.Deposit, Result=Paldark.Hub.Result.Success`. `Paldark.Hub.Stable.List` confirms entry moved.
6. Console: `Paldark.Hub.Stable.Withdraw 0` → round-trips back to active roster. `Paldark.Hub.Stable.List` confirms.
7. Walk up to marketplace kiosk, press E → expect async-catalog-load log line, then `OnMarketplaceOpened`.
8. Console: `Paldark.Hub.Market.Catalog` → list of items + buy/sell prices.
9. Console: `Paldark.Hub.Market.Balance` → current credits.
10. `Paldark.Hub.Market.Buy Paldark.Item.Bandage 3` → if credits sufficient, expect `Result.Success`; if not, expect `Result.Fail.Insufficient`. Confirm balance delta.
11. `Paldark.Hub.Market.Sell Paldark.Item.Bandage 1` → confirm credits go back up.
12. Spam-double-click test: open the marketplace, send two `Buy` commands back-to-back fast → expect one to `Result.Success` and the other to `Result.Fail.NotAllowed` due to `bIsTransactionInFlight`.

### Validator extension

`check_w44_stable_marketplace_shape` (~450 lines) covers:

- All 10 new files exist with the right UCLASS bases (`UPaldarkItemFragment`, `UActorComponent`, `UUserWidget`).
- `PaldarkItemFragment_MarketValue.h` has `BuyPriceCredits` / `SellPriceCredits` / `bAllowBuy` / `bAllowSell` + `ClampMin="0"` UI clamp.
- `PaldarkPalDepositComponent.h` declares the 3-API surface + `MaxDepositSlots` knob + `DepositedEntries` is `ReplicatedUsing=OnRep_DepositedEntries`; `.cpp` uses `DOREPLIFETIME_CONDITION(..., COND_OwnerOnly)`.
- `PaldarkStableController.h` declares all 4 client APIs + 3 Server RPCs + 1 Client RPC + 3 `BlueprintAssignable` delegates + `HealPriceCredits`; `.cpp` uses `bIsTransactionInFlight` + all 3 action tags + 1 result tag + `LogPaldarkUI`.
- `PaldarkMarketplaceController.h` declares all 4 client APIs + 2 Server RPCs + 1 Client RPC + 3 delegates + `CachedCatalog` + `CatalogLoadHandle`; `.cpp` uses `RequestAsyncLoad(` + `FindFragmentByClass<UPaldarkItemFragment_MarketValue>` + all 2 action tags + 2 result tags + `LogPaldarkUI`.
- Both widget stubs declare the `BlueprintImplementableEvent` hooks + `BlueprintPure` controller accessor.
- `PaldarkPlayerController.h` / `.cpp` instantiate both controllers as default subobjects.
- `PaldarkPlayerState.h` / `.cpp` instantiate the deposit component as default subobject.
- `PaldarkPalStable.cpp` / `PaldarkMarketplaceKiosk.cpp` route `OnInteract` through the per-player controller.
- `PaldarkLab.Build.cs` lists `"UMG"`.
- `LogPaldarkUI` declared + defined.
- All 8 console commands registered by string match.
- 14 new gameplay tags enforced via `check_gameplay_tags`.
- `LogPaldarkUI` enforced via `check_log_categories`.

Run locally: `python3 scripts/ci/validate_paldarklab.py` → expect `OK`.

### Anti-patterns avoided (W44-45)

- **Active roster + deposit list on the same actor** — Would tie the deposit list to pawn death/respawn. Split: active on Character, deposit on PlayerState (matches GAS ASC + AttributeSet pattern from W7-8).
- **Using `OnRep_DepositedEntries` to fire UI refresh without a delegate hop** — Tightly couples replication to UMG. Instead the OnRep just calls `OnDepositChanged.Broadcast(this)`; widget binds to the multicast delegate; designer can refresh from any other code path (e.g. a deposit-on-extract gameplay tag handler) without touching the OnRep body.
- **Wiring marketplace pricing as a `UDataTable` row** — Would require designers to manage the data table separately from the item def asset. Putting the price in a fragment on the item def lets designers author price-as-data on the item itself, matches the existing W11-12 / W37-38 fragment shape.
- **Letting `Server_RequestBuy` race against itself** — Without the `bIsTransactionInFlight` flag, a player could spam Buy to debit credits while AddItem is still in flight. Flag locks the whole RPC body — simple, correct, and the UI button disable on the client side hides the latency.
- **Async-loading the catalog every interact** — Would waste bandwidth + IO if a player opens the marketplace 10 times in a row. `OnCatalogLoadCompleted` caches into `CachedCatalog`; the next interact short-circuits if `ActiveKiosk == this && CachedCatalog.Num() > 0`. (W47 may add an explicit cache invalidation hook for marketplace stock changes.)
- **Replicating `CachedCatalog`** — The catalog is the same on every client (designer-authored on the kiosk's `BuildingDefinition`); each client loads it locally. Replication would burn ~10x more bandwidth than just having each client async-load on demand.
- **Putting `HealPriceCredits` in `DefaultGame.ini`** — Would force a config edit + editor restart. Designer knob on the controller's CDO is editable in Blueprint subclass defaults + `EditDefaultsOnly`.
- **Logging credit-balance deltas at `Verbose`** — Designers want these visible at default log level so they can debug pricing bugs without flipping a category to `Verbose`. Default category level for `LogPaldarkUI` is `Log` (which `[Hub.Market.Result]` lines use).
- **Wiring the kiosk → controller dispatch through a `UWorldSubsystem` indirection** — Adds latency + indirection for no benefit. The PlayerController is already the natural per-player UI host (Lyra `ULyraHeroComponent` pattern); the kiosk just resolves `Interactor->GetStableController()` once.

### Out of scope (defer to later Q4 / Q1-next)

- **Actual UMG widget layout** — Designer task. C++ provides `BlueprintImplementableEvent` hooks (`K2_OnStableOpened`, `K2_OnStableActionResult`, marketplace equivalents); the designer authors `WBP_PaldarkStableWidget` / `WBP_PaldarkMarketplaceWidget` as Blueprint subclasses + binds them in the W40-41 `UPaldarkHubBuildingDefinition::WidgetClass` field.
- **Persistent roster across matches** — W47 save-game (USaveGame slot keyed on PlayerState). W44-45 deposit list resets to empty on every match boundary.
- **Pal deploy-from-roster flow** — W45+ polish week. Today the active roster is the canonical "deployable" set; W47 may add a separate deploy queue subset.
- **Cross-player marketplace trade** — Separate Q4 stretch. Today's marketplace is NPC-backed (player buys/sells against the kiosk's catalog); P2P trade UI is a different week.
- **mod.io / Steam Workshop integration** — Separate Q4 week (referenced from the roadmap as "marketplace UI" but distinct from this NPC-backed kiosk).
- **Persistent stock simulation** — The kiosk's catalog has infinite stock today. W4Q stretch may add per-kiosk `StockTracker` that depletes on Buy + restocks on Sell + decays linearly over real time.
- **Hub building tier progression (stable T1 → T2 → T3)** — `MaxDepositSlots` is currently flat at 24. W47 progression may tier this off a building level.

## W46 — Briefing room widget + multi-player vote/ready handoff (Q4 hub QA pass)

Roadmap line item: "Hub QA pass — briefing-room widget + multi-player vote/ready handoff into the raid." Builds on W40-41 (`APaldarkBriefingRoom`, `UPaldarkHubSubsystem`) and W44-45 (per-player controller component pattern) to widen the single-player console-driven briefing into a **lobby-style flow** where every player in the hub shard votes for a raid map, signals ready, and the server runs a shared countdown before issuing a single `ServerTravel` for the entire shard. Ships the **C++ scaffold + console-driven test loop**; designer authors `WBP_PaldarkBriefingWidget` as a Blueprint subclass of the C++ stub in the next sprint.

### What this PR ships

| Layer | File | Role |
|------|------|------|
| Briefing controller | `Source/PaldarkLab/Public/Hub/PaldarkBriefingController.h` + `.cpp` | `UActorComponent` on `APaldarkPlayerController`. 6 client APIs (`RequestOpenBriefing`, `RequestCloseBriefing`, `RequestVote`, `RequestUnvote`, `RequestReady`, `RequestUnready`), 4 Server RPCs (`Server_RequestVote/Unvote/Ready/Unready` with validation), 1 Client RPC (`Client_ReceiveActionResult`), 3 `BlueprintAssignable` delegates (`OnBriefingOpened`, `OnBriefingClosed`, `OnBriefingActionResult`). `bIsTransactionInFlight` guard rejects overlapping RPCs per-player. |
| Session component | `Source/PaldarkLab/Public/Hub/PaldarkBriefingSessionComponent.h` + `.cpp` | `UActorComponent` on `APaldarkGameStateBase`. Owns shard-wide replicated state: `PhaseTag` (Idle/Countdown/Travelling), `VoterStates` (per-player vote tag + ready flag), `CountdownRemaining`. 5 authority APIs (`ServerCastVote`, `ServerClearVote`, `ServerSetReady`, `ServerRemovePlayer`, `ServerForceTravel`) + 6 read-only accessors. Server-authoritative 1Hz countdown timer; transitions phase to Travelling on completion and issues `HostHubServer(MapName, ExperienceId)`. `MinReadyPlayers` (default 1) + `CountdownSeconds` (default 10) designer knobs. |
| Widget stub | `Source/PaldarkLab/Public/Hub/PaldarkBriefingWidget.h` + `.cpp` | `UUserWidget` C++ base. Declares 4 `BlueprintImplementableEvent` hooks (`K2_OnBriefingOpened`, `K2_OnBriefingClosed`, `K2_OnBriefingActionResult`, `K2_OnSessionChanged`) plus `BlueprintPure` accessors for the controller + session component. `NativeOnInitialized` resolves and binds the 4 delegates; `NativeDestruct` unbinds. Designer authors `WBP_PaldarkBriefingWidget` as a Blueprint subclass. |
| Hub building wiring | `Source/PaldarkLab/Private/Hub/PaldarkBriefingRoom.cpp` (modified) | `OnInteract` now resolves `Interactor->GetBriefingController()` and calls `RequestOpenBriefing(this)` instead of the W40-41 immediate `HostHubServer`. The W40-41 console-driven path remains as a documented backward-compat fallback for solo-test scenarios (when `BriefingController == nullptr`). |
| PlayerController | `Source/PaldarkLab/Public/Player/PaldarkPlayerController.h` + `.cpp` (modified) | Default-subobject creation of `UPaldarkBriefingController` in ctor + `GetBriefingController` `BlueprintPure` accessor. Lives alongside the W44-45 `UPaldarkStableController` / `UPaldarkMarketplaceController`. |
| GameStateBase | `Source/PaldarkLab/Public/Framework/PaldarkGameStateBase.h` + `.cpp` (modified) | Default-subobject creation of `UPaldarkBriefingSessionComponent` in ctor + `GetBriefingSessionComponent` `BlueprintPure` accessor. Component replicates by virtue of being attached to the always-replicated GameState. |
| Tags | `Source/PaldarkLabCore/Public/PaldarkGameplayTags.h` + `.cpp` (modified) | 10 new tags — 3 UI state (`Paldark.Hub.Brief.UI.{Closed, Opening, Open}`), 4 action (`Paldark.Hub.Brief.Action.{Vote, Unvote, Ready, Unready}`), 3 phase (`Paldark.Hub.Brief.Phase.{Idle, Countdown, Travelling}`). Re-uses W44-45 `Paldark.Hub.Result.*` for action results. |
| Console | `Source/PaldarkLab/PaldarkLab.cpp` + `PaldarkLab.h` (modified) | 6 new commands — `Paldark.Hub.Brief.{Dump, Vote, Unvote, Ready, Unready}` (per-player flow) + `Paldark.Hub.QA.ForceTravel <Paldark.Map.X> [MapName]` (authority-only bypass that jumps the shard straight to Travelling). Owned by the module; auto-unregister on `ShutdownModule`. |
| Validator | `scripts/ci/validate_paldarklab.py` (modified) | New `check_w46_hub_qa_shape` (~330 lines) + 10 tags added to `EXPECTED_GAMEPLAY_TAGS`. |

### Component ownership map (W46 delta over W44-45)

```
APaldarkPlayerController
├── UPaldarkStableController          (W44-45; per-player UI state)
├── UPaldarkMarketplaceController     (W44-45; per-player UI state)
└── UPaldarkBriefingController        (W46;    per-player UI state)

APaldarkGameStateBase
└── UPaldarkBriefingSessionComponent  (W46; shard-wide replicated state)
```

Why this split mirrors W44-45 + adds a GameState component:

- **UI state on the PlayerController** — Same reason as the Stable/Marketplace controllers: survives respawn, natural RPC target.
- **Session state on the GameState** — Vote tallies + ready bitset + phase + countdown must be visible to every client. GameState replicates by default, so widgets bind straight to the local mirror without an extra RPC hop. (See the header docs for the longer rationale — GameMode would be server-only, a UWorldSubsystem would also be server-only on dedicated.)
- **No save-game involvement** — Briefing state is per-hub-session; resets naturally when the GameState is destroyed on ServerTravel.

### End-to-end flow — Vote → Ready → Countdown → ServerTravel

```
1. Player walks up to APaldarkBriefingRoom (W40-41 building) and presses E:
   APaldarkBriefingRoom::OnInteract(Interactor)
   └─ Interactor->GetBriefingController()->RequestOpenBriefing(this)
       ├─ UIStateTag = Paldark.Hub.Brief.UI.Open
       ├─ ActiveBriefingRoom = this
       └─ OnBriefingOpened.Broadcast(this)
           └─ WBP_PaldarkBriefingWidget::K2_OnBriefingOpened(this)

2. Player clicks "Vote for Rừng Hỏng" → widget calls:
   BriefingController->RequestVote(TAG_Paldark_Map_RungHong)
   └─ Server_RequestVote_Validate / _Implementation(MapTag)
       ├─ Session->ServerCastVote(this, MapTag)
       │   ├─ FindOrAddVoterRow(PC); VotedMapTag = MapTag
       │   ├─ Replicate VoterStates → every client OnRep_VoterStates fires
       │   │   └─ Broadcast OnSessionChanged → every widget refreshes tally bars
       │   └─ ServerEvaluateThreshold() → still Idle (ready count < threshold)
       └─ Client_ReceiveActionResult(Action.Vote, Result.Success, MapTag, 0)
           └─ OnBriefingActionResult.Broadcast(...) → originating widget

3. Player clicks "Ready" → widget calls:
   BriefingController->RequestReady()
   └─ Server_RequestReady_Validate / _Implementation()
       ├─ Session->ServerSetReady(this, true)
       │   ├─ Voter row bIsReady = true
       │   ├─ Replicate VoterStates
       │   └─ ServerEvaluateThreshold()
       │       → leading map has >= MinReadyPlayers ready
       │       → PhaseTag = Paldark.Hub.Brief.Phase.Countdown
       │       → CountdownRemaining = CountdownSeconds (default 10)
       │       → SetTimer(CountdownTimerHandle, ServerTickCountdown, 1.0, true)
       └─ Client_ReceiveActionResult(Action.Ready, Result.Success, <invalid>, CountdownRemaining)

4. Countdown ticks down server-side; every tick replicates CountdownRemaining
   → widgets render bar / numeric on every client.

5. CountdownRemaining hits 0:
   ServerTickCountdown() invariant met
   └─ PhaseTag = Paldark.Hub.Brief.Phase.Travelling
   └─ ServerIssueTravelForMap(LeadingMapTag, <auto-resolve from W40-41 BuildingDefinition>)
       └─ HostHubServer(ResolvedMapName, ExperienceId) → ServerTravel
           → entire shard transports together; GameState destroyed → briefing
             session resets naturally on the next hub return.
```

### Phase state machine (server-authoritative)

```
                +---------+
   start ─────► |  Idle   | ◄────────── (Unready drops below threshold)
                +----┬----+
                     │ vote majority + ready >= MinReadyPlayers
                     ▼
                +---------+
                |Countdown|
                +----┬----+
                     │ CountdownRemaining tick → 0
                     ▼
                +---------+
                |Travel-  |
                |ling     |    (terminal; ServerTravel destroys GameState)
                +---------+
```

Transitions are **server-only**; clients observe via `OnRep_PhaseTag`. Idempotent in both directions (Idle ↔ Countdown):
- **Idle → Countdown**: only when leading map has `>= MinReadyPlayers` ready voters AND we are currently in `Idle`.
- **Countdown → Idle**: when a player unreadies and the threshold drops; cancels `CountdownTimerHandle` and resets `CountdownRemaining` to 0.
- **Countdown → Travelling**: terminal; the countdown timer's final tick.
- **Force-travel bypass**: `ServerForceTravel(MapTag, MapName)` jumps straight to Travelling, bypassing every gate. Used by `Paldark.Hub.QA.ForceTravel` for QA / smoke tests.

### Replication + race-condition contract

| Risk | Mitigation |
|------|------------|
| Two `RequestVote` RPCs fire from the same client before the first resolves | `bIsTransactionInFlight` server-side flag rejects the second with `Result.Fail.NotAllowed`; client-side, UMG disables the vote button on `RequestVote` and re-enables on `OnBriefingActionResult`. Per-player guard so other players' votes are independent. |
| Player disconnects mid-vote | `APaldarkGameModeBase::Logout` (or equivalent hook in subclasses) should call `Session->ServerRemovePlayer(PC)` so their voter row + ready bit are dropped. `ServerEvaluateThreshold` re-runs after removal, may cancel an in-flight countdown if their ready bit was load-bearing. (Wire-up of `Logout` hook left to the experience-specific GameMode subclass — see "Out of scope" below.) |
| Countdown completes but ServerTravel fails (bad map) | `ServerIssueTravelForMap` logs the failure and resets phase back to Idle. Players see `OnSessionChanged` fire and `PhaseTag` rep back to Idle; widgets revert to the vote view. |
| Vote tally ties (two maps with equal counts) | `GetLeadingMapTag` breaks ties alphabetically on the tag string for deterministic resolution. Designer can override this in the widget by inspecting `GetVoteTallies` directly. |
| Other players see my exact vote (intent leak) | `VoterStates` replicates to everyone; this is intentional (vote transparency is a design goal — players should see who voted for what so they can debate). If "secret ballot" is ever needed, swap `DOREPLIFETIME` → `DOREPLIFETIME_CONDITION(..., COND_OwnerOnly)` and add a separate `VoteTallies` replicated summary. |
| Player closes the widget while in Countdown phase | Closing the widget does NOT clear their vote or ready bit — those persist until the player explicitly Unvotes / Unreadies. Matches the W44-45 marketplace pattern (closing the widget does NOT cancel an in-flight Buy). |
| Server-side countdown timer keeps ticking after GameState destruction | `EndPlay` clears `CountdownTimerHandle` via `GetWorld()->GetTimerManager().ClearTimer`. ServerTravel happens before EndPlay on the GameState, so the final tick has already issued the travel before cleanup. |

### Console commands (designer reference)

| Command | Usage | Authority | Effect |
|---------|-------|-----------|--------|
| `Paldark.Hub.Brief.Dump` | `Paldark.Hub.Brief.Dump` | Any | Dump session phase + vote tallies + voter list to `LogPaldarkUI`. |
| `Paldark.Hub.Brief.Vote` | `Paldark.Hub.Brief.Vote <Paldark.Map.X\|leaf>` | Any | Controller server RPC: cast vote for the supplied map tag. Accepts full `Paldark.Map.X` or short leaf form (`RungHong` → `Paldark.Map.RungHong`). |
| `Paldark.Hub.Brief.Unvote` | `Paldark.Hub.Brief.Unvote` | Any | Controller server RPC: clear the local player's vote. |
| `Paldark.Hub.Brief.Ready` | `Paldark.Hub.Brief.Ready` | Any | Controller server RPC: signal that the local player is ready to depart. |
| `Paldark.Hub.Brief.Unready` | `Paldark.Hub.Brief.Unready` | Any | Controller server RPC: clear the local player's ready bit. Cancels countdown if threshold drops. |
| `Paldark.Hub.QA.ForceTravel` | `Paldark.Hub.QA.ForceTravel <Paldark.Map.X> [MapName]` | **Server only** | Bypass — force the shard to Travelling phase and issue `HostHubServer` with the supplied map. If `MapName` omitted, the session resolves from the W40-41 building def's `AllowedRaidMaps`. **Designer / QA only**, never wire to gameplay. |

### Designer test loop (in UE Editor, after this PR lands)

1. Pull this branch, **Generate Project Files**, build PaldarkLab + Editor.
2. Open PIE on `Map_HubTown.umap` (designer-authored placeholder — see W40-41 § "Designer follow-ups") with 2 players for a meaningful vote/ready scenario.
3. Player 1: walk up to `APaldarkBriefingRoom`, press E → expect log line `[Hub.BriefingRoom] OnInteract by PC ... — routing through BriefingController` followed by `OnBriefingOpened` broadcast.
4. Player 1 console: `Paldark.Hub.Brief.Vote RungHong` → expect `OnBriefingActionResult` log with `Action=Paldark.Hub.Brief.Action.Vote, Result=Paldark.Hub.Result.Success, MapTag=Paldark.Map.RungHong`. `Paldark.Hub.Brief.Dump` shows 1 vote for RungHong.
5. Player 2 console: `Paldark.Hub.Brief.Vote RaidSandbox` → `Dump` shows split 1-1; alphabetical tie-break picks `Paldark.Map.RaidSandbox`.
6. Player 1 console: `Paldark.Hub.Brief.Ready` → `Result.Success`. `Dump` shows ready count = 1; phase = Countdown if `MinReadyPlayers == 1` (default), else still Idle.
7. Player 2 console: `Paldark.Hub.Brief.Vote RungHong` (switching vote) → now both players voted RungHong, leading map updated, replication fires on both clients.
8. Player 2 console: `Paldark.Hub.Brief.Ready` → both players ready, phase transitions to Countdown, CountdownRemaining = 10. Each client widget should animate the countdown bar via `OnRep_CountdownRemaining` → `K2_OnSessionChanged`.
9. Wait 10s → `ServerTravel` to `Map_RungHong` issued, entire shard transports together.
10. Mid-countdown cancel test: re-enter the scenario, get to Countdown, then have one player `Paldark.Hub.Brief.Unready` mid-countdown → phase reverts to Idle, CountdownRemaining → 0. Designer widget should show vote view again, not countdown.
11. QA bypass: from listen-server console, `Paldark.Hub.QA.ForceTravel RungHong` → entire shard jumps to Travelling phase immediately, ignoring vote/ready state. Verify the warning log line on client-side attempts ("authority-only command").

### Validator extension

`check_w46_hub_qa_shape` (~330 lines) covers:

- All 6 new files exist with the right UCLASS bases (`UActorComponent`, `UUserWidget`).
- `PaldarkBriefingController.h` declares all 6 client APIs + 4 Server RPCs + 1 Client RPC + 3 `BlueprintAssignable` delegates + `bIsTransactionInFlight` guard.
- `PaldarkBriefingController.cpp` uses 4 action/UI tags (`UI.Open`, `UI.Closed`, `Action.Vote`, `Action.Ready`) + `Result.Success` + `LogPaldarkUI`.
- `PaldarkBriefingSessionComponent.h` declares all 5 authority APIs + 6 accessors + 2 designer knobs (`MinReadyPlayers`, `CountdownSeconds`) + 3 replicated properties (`ReplicatedUsing=OnRep_*`) + 3 OnRep UFUNCTIONs + `GetLifetimeReplicatedProps` override + `OnSessionChanged` delegate + `DumpToLog` + both vote/voter structs.
- `PaldarkBriefingSessionComponent.cpp` uses `DOREPLIFETIME` for all 3 replicated properties + 3 phase tags + `LogPaldarkUI`.
- Widget stub declares the 4 `BlueprintImplementableEvent` hooks + `BlueprintPure` controller + session accessors.
- `PaldarkPlayerController.h` / `.cpp` instantiate `UPaldarkBriefingController` as a default subobject (alongside the W44-45 controllers).
- `PaldarkGameStateBase.h` / `.cpp` instantiate `UPaldarkBriefingSessionComponent` as a default subobject.
- `PaldarkBriefingRoom.cpp` includes the controller header + routes `OnInteract` through `RequestOpenBriefing(this)`.
- All 6 console commands registered by string match.
- 10 new gameplay tags enforced via `check_gameplay_tags`.

Run locally: `python3 scripts/ci/validate_paldarklab.py` → expect `OK`.

### Anti-patterns avoided (W46)

- **Session state on the GameMode (server-only)** — Clients would need an extra RPC hop to read vote tallies. Putting it on the GameState gives free replication. Matches Lyra's `ALyraGameState::Components` pattern.
- **Per-player vote RPC fan-out** — A naive impl might have each client poll `Server_GetVoteTallies` periodically. Instead, the session component's `VoterStates` replicates via `OnRep`, fires `OnSessionChanged`, widgets refresh — zero polling.
- **Client-side countdown timer prediction** — A client running its own timer drifts from the server's. The countdown is server-authoritative; clients only render the replicated `CountdownRemaining`. Drift is bounded by the replication frequency (default 100ms).
- **Letting `Server_RequestVote` race against itself** — Same `bIsTransactionInFlight` guard as W44-45. Without it, a player could spam Vote to flip the leading map mid-countdown.
- **Per-player ClientTravel** — Would split the shard across multiple sessions, defeating the "8-player raid" goal. The session component issues ONE ServerTravel for the whole shard once the countdown completes.
- **Storing voter state in a `TMap<APaldarkPlayerController*, FPaldarkBriefingVoterState>`** — `TMap` keyed on `UObject*` doesn't replicate cleanly. Instead, voter rows live in a `TArray` replicated as a whole; `FindVoterRow(PC)` does an O(N) scan (N <= 8 for an 8-player shard, fine).
- **Non-deterministic tie-break on equal vote counts** — `GetLeadingMapTag` breaks ties alphabetically on the tag string. Without this, the leading map could flicker on every replication.
- **Trying to handle Logout in the session component itself** — That would require the component to subscribe to GameMode events from inside a GameState component — awkward coupling. Instead, the experience-specific GameMode subclass owns the `Logout` override and calls `Session->ServerRemovePlayer(PC)`. (Wire-up left for a follow-up — see "Out of scope".)
- **Skipping the W40-41 console fallback in `OnInteract`** — Would break solo-test scenarios where the briefing controller is not yet wired (e.g. designer running on a custom PC subclass). The fallback path runs only if `Controller == nullptr`, keeping W40-41 backward-compat alive.

### Out of scope (defer to later Q4 / Q1-next)

- **Actual UMG widget layout** — Designer task. C++ provides 4 `BlueprintImplementableEvent` hooks + `BlueprintPure` accessors; designer authors `WBP_PaldarkBriefingWidget` as a Blueprint subclass + binds it via `UPaldarkHubBuildingDefinition::WidgetClass` (or a new `BriefingWidgetClass` field) in the next pass.
- **Logout hook in GameMode to clean up disconnecting players** — The session component exposes `ServerRemovePlayer(PC)`; the experience-specific GameMode subclass needs to override `Logout` and route through it. Left as a follow-up because the right place to wire this is in the W40-41 `APaldarkGameMode_HubTown` subclass, not in the base GameModeBase.
- **Per-experience designer override of `MinReadyPlayers` / `CountdownSeconds`** — Today these are CDO knobs on the session component. A future PR can move them to `UPaldarkExperienceDefinition` so different experiences (hub vs. raid lobby vs. tournament) can use different thresholds.
- **Vote weighting** — Today every player's vote counts equally (1 vote per player). A future PR can add `FPaldarkBriefingVoterState::VoteWeight` so e.g. squad leaders count for 2 votes.
- **In-widget chat / voice indicator** — Pure UMG concern; the C++ scaffold provides nothing here. Designer can layer this onto the Blueprint subclass independently.
- **Cross-shard matchmaking** — Today the briefing session is shard-local (8 players who happened to land on this hub). True matchmaking (lobby across shards) is a much bigger Q4 stretch tied to W42-43 backend integration.
- **Persistent vote history / audit log** — Today the session resets on every ServerTravel. W47 save-game could persist "last map voted for" as a designer convenience but it's not in scope here.

## W47 — Save game system (USaveGame for player progression)

W47 đóng L-47 carry từ Q3 ("USaveGame for player progression: roster, inventory snapshot, credits, condition tags"). Một async save/load subsystem console-driven cho phép QA bake snapshot của Active Roster + Pal Deposit + Inventory + Progress vào disk và replay lại nguyên trạng. UMG slot picker + auto-save hook deferred sang W48 polish.

### Sáu mảnh ghép

| # | File | Vai trò |
|---|------|---------|
| 1 | `Source/PaldarkLab/Public/Save/PaldarkSaveTypes.h` (+ rỗng `.cpp`) | 5 USTRUCT serializable blobs (`FPaldarkRosterEntrySaved`, `FPaldarkRosterSnapshot`, `FPaldarkInventoryEntrySaved`, `FPaldarkInventorySnapshot`, `FPaldarkProgressSnapshot`). Dùng `FGameplayTag SpeciesTag` / `FGameplayTag ItemTag` thay cho soft pointer để blob sống sót asset rename. |
| 2 | `Source/PaldarkLab/Public/Save/PaldarkPlayerSaveGame.{h,cpp}` | `UPaldarkPlayerSaveGame : USaveGame` với `kCurrentSchema = 1`, `SchemaVersion` / `SaveTime` / `PlayerName` + 4 nested snapshot (`ActiveRoster`, `DepositRoster`, `Inventory`, `Progress`). `DescribeForLog()` helper cho console dump path. |
| 3 | `Source/PaldarkLab/Public/Save/PaldarkSaveSubsystem.{h,cpp}` | `UPaldarkSaveSubsystem : UGameInstanceSubsystem` orchestrate async save/load. APIs: `RequestSaveSlot`, `RequestLoadSlot`, `RequestDumpSlot`, `ClearSlot`, `DoesSlotExist`, `ListSlotsToLog`, `WipeAllSlots`. 2 BlueprintAssignable delegate (`OnSaveCompleted`, `OnLoadCompleted`) cho UMG (W48) lắng nghe. |
| 4 | `CaptureSnapshot` / `ApplySnapshot` trên 3 live-state component | `UPaldarkPlayerPalRosterComponent` (player ActiveRoster), `UPaldarkPalDepositComponent` (PlayerState Deposit), `UPaldarkPlayerInventoryComponent` (player Inventory). Authority-only write, broadcast change delegate sau khi apply. Inventory flatten nested `InnerEntries` và re-equip backpack trước khi add lại item. |
| 5 | 7 gameplay tags mới trong `Source/PaldarkLabCore/Public/PaldarkGameplayTags.{h,cpp}` | 4 result (`Paldark.Save.Result.{Success,Fail.IOError,Fail.SchemaMismatch,Fail.NoSlot}`) + 3 trigger (`Paldark.Save.Trigger.{HubReturn,Logout,Manual}`). Result tag được stamp lên delegate payload; trigger tag được caller pass vào `RequestSaveSlot` để listener branch theo "why did we save?". |
| 6 | 6 console commands trong `Source/PaldarkLab/PaldarkLab.{h,cpp}` | `Paldark.Save.{Save, Load, Dump, ClearSlot, ListSlots}` + `Paldark.QA.WipeAllSlots`. Đủ cho QA test save/load round-trip mà không cần UMG. |

### Tag → delegate payload map

```
RequestSaveSlot(PC, "Slot01", Paldark.Save.Trigger.HubReturn)
  ├─ AsyncSaveGameToSlot       ─→ HandleSaveComplete
  │                                ├─ OnSaveCompleted.Broadcast(
  │                                │     SlotName  = "Slot01",
  │                                │     ResultTag = Paldark.Save.Result.Success | Paldark.Save.Result.Fail.IOError,
  │                                │     TriggerTag= Paldark.Save.Trigger.HubReturn  ← echoed back from caller
  │                                │   )

RequestLoadSlot(PC, "Slot01")
  ├─ DoesSaveGameExist? = false ─→ OnLoadCompleted.Broadcast(SlotName, Paldark.Save.Result.Fail.NoSlot)
  ├─ AsyncLoadGameFromSlot      ─→ HandleLoadComplete
  │                                ├─ Cast fail              → Fail.IOError
  │                                ├─ SchemaVersion mismatch → Fail.SchemaMismatch
  │                                ├─ PC gone before apply   → Fail.IOError
  │                                └─ ApplySnapshotToPC      → Success
```

### Console test loop (designer-facing)

```
# Bake a snapshot of the local player into a named slot.
> Paldark.Save.Save Slot01 HubReturn
[Save] async write begin slot=Slot01 trigger=Paldark.Save.Trigger.HubReturn schema=1 active=4 deposit=2 inventory=17
[Save] async write complete slot=Slot01 user=0 result=Paldark.Save.Result.Success trigger=Paldark.Save.Trigger.HubReturn

# Dump the slot to log without applying it (sanity-check the blob).
> Paldark.Save.Dump Slot01
[Save] DUMP slot=Slot01 Schema=1 SaveTime=2026-05-16T19:35Z Player='thanhtin1997' Active=4 Deposit=2 Inv=17 Map='Map_HubTown'

# Mutate the live PC (kill a Pal, drop some items, etc.), then…

# Replay the snapshot. Active roster + deposit + inventory all wipe and re-populate from disk.
> Paldark.Save.Load Slot01
[Save] async read begin slot=Slot01 for PC=PaldarkPlayerController_0
[Save] ApplySnapshot complete: Schema=1 ... Active=4 Deposit=2 Inv=17 ...

# Housekeeping.
> Paldark.Save.ListSlots
[Save] ListSlots dir=…/PaldarkLab/Saved/SaveGames count=3
[Save]   Slot01.sav      4_096 bytes  mtime=2026-05-16T19:36:00Z
> Paldark.Save.ClearSlot Slot01
[Save] ClearSlot slot=Slot01 result=ok
> Paldark.QA.WipeAllSlots
[Save] WipeAllSlots deleted=3 of 3 candidate files
```

### Schema versioning

`UPaldarkPlayerSaveGame::kCurrentSchema` (hiện = `1`) là single source of truth. `HandleLoadComplete` so sánh blob `SchemaVersion` với `kCurrentSchema` và stamp `Paldark.Save.Result.Fail.SchemaMismatch` nếu lệch — payload không apply lên PC. Bump `kCurrentSchema` mỗi lần struct shape thay đổi (thêm/đổi/xoá field) là cách CỘT MỐC kỷ luật để pre-launch slot khỏi nuốt sai data sau Q4 patch.

### Tại sao tag thay cho soft-ptr

Snapshot dùng `FGameplayTag SpeciesTag` / `FGameplayTag ItemTag` thay vì `TSoftObjectPtr<UPaldarkPalDefinition>` / `TSoftObjectPtr<UPaldarkItemDefinition>` vì:

1. **Asset rename survival** — Designer rename `DA_PalDef_Direhound` thành `DA_PalDef_Direhound_v2`, soft-ptr blob trên disk thành `None`; tag thì giữ nguyên `Paldark.Pal.Species.Direhound` xuyên rename.
2. **Cross-build compatibility** — Tag string là contract của runtime; ai chuyển asset path từ `/Game/Pals/` sang `/Game/Content/Pals/` cũng không phá save.
3. **`UAssetManager` resolution rẻ** — `ApplySnapshot` gọi `UAssetManager::GetPrimaryAssetIdList()` để resolve tag → DataAsset là one-time-per-load, không phải mỗi tick.

### Slot naming convention

`UPaldarkSaveSubsystem::GetDefaultSlotName(PC)` resolve theo thứ tự ưu tiên:

1. `PlayerState::GetUniqueId()` netId string (online mode) → `Paldark_<NetID>`
2. `PlayerState::GetPlayerName()` (PIE / standalone) → `Paldark_<DisplayName>`
3. Fallback → `DefaultPlayer`

QA có thể override slot name qua arg đầu của `Paldark.Save.Save`/`Load`/`Dump` (ví dụ `Paldark.Save.Save QASlot_Bug` để bake repro state).

### Test loop trong UE Editor (designer)

1. Start PIE với `Map_HubTown` (W40-41 hub) hoặc bất kỳ map có companion Pal authored.
2. Companion roster + 1-2 item trong inventory.
3. `Paldark.Save.Save TestSlot Manual` → log line `[Save] async write complete ... result=Paldark.Save.Result.Success`.
4. Kill 1 Pal trong roster + drop 1 item.
5. `Paldark.Save.Load TestSlot` → log line `[Save] ApplySnapshot complete:` + roster snap-back, inventory snap-back.
6. `Paldark.Save.Dump TestSlot` → in JSON-style summary của blob.
7. `Paldark.Save.ClearSlot TestSlot` → confirm slot xoá khỏi disk.
8. `Paldark.QA.WipeAllSlots` → wipe toàn bộ slot (chỉ designer/QA).

### Out of scope (defer to W48 polish hoặc Q1-next)

- **Actual UMG save slot picker** — Designer task. C++ provide `OnSaveCompleted` / `OnLoadCompleted` BlueprintAssignable delegate; designer subclass `WBP_PaldarkSaveSlotPicker : UUserWidget`, bind 2 delegate đó, route lên `Paldark.Hub.Result.*` UI state.
- **Auto-save hook** — W48 polish. Caller có thể trigger `RequestSaveSlot(PC, NAME_None, Paldark.Save.Trigger.HubReturn)` từ `OnPlayerExtracted` (W24-25) hoặc `OnPlayerLogout` (W14-15) khi vào W48.
- **Cloud save / Steam Workshop / cross-device sync** — Q4 stretch.
- **Per-shard save (raid persistent state)** — Q1-next. Hiện W47 chỉ save per-player progression; world state (POI explored, marketplace stock, etc.) là vấn đề khác.
- **Save versioning với automatic migration** — Hiện schema mismatch stamp `Fail.SchemaMismatch` rồi bỏ qua. Tương lai có thể author `UPaldarkSaveMigration_V1ToV2` data asset chain để designer định nghĩa migration step.
- **Per-experience save filter** — Hiện snapshot bake mọi component tìm được. Tương lai có thể author `UPaldarkExperienceDefinition::bSaveOnTravel` để hub-only save (raid không bake giữa chừng raid).
- **Slot quota / on-disk size cap** — Hiện `WipeAllSlots` xoá tất cả; không có giới hạn số slot. UMG picker (W48) có thể enforce cap khi tạo slot mới.

## W48 — Polish (auto-save hooks + UMG slot picker stub + hub→raid handoff smoke)

W48 đóng nốt phần "polish" của save loop: ba auto-save trigger đã wire vào engine event (match outcome + player logout + hub→raid travel smoke), một C++ UMG base class cho designer subclass slot picker, và một QA console command để verify hub→raid handoff path mà không cần ServerTravel thật.

### Bốn mảnh ghép

| # | File / Symbol | Vai trò |
|---|---------------|---------|
| 1 | `UPaldarkSaveSubsystem` (W47 subsystem, extend) | Bind 3 engine delegate trong `Initialize()`: `FCoreUObjectDelegates::PostLoadMapWithWorld` → re-bind world-scope match subsystem hook mỗi map load; `UPaldarkMatchSubsystem::OnPlayerOutcomeChanged` → auto-save với `Paldark.Save.Trigger.HubReturn` khi player flip Extracted; `UPaldarkNetSubsystem::OnPlayerLogout` → auto-save với `Paldark.Save.Trigger.Logout` trước khi PC destroy. Hai UPROPERTY toggle (`bAutoSaveOnExtract`, `bAutoSaveOnLogout`) cho QA bật/tắt runtime. |
| 2 | `RequestHubToRaidHandoffSmoke(PC, SlotName)` public API | Manual smoke entry — wrap `RequestSaveSlot(..., Paldark.Save.Trigger.Travel)` cho hub→raid handoff path. Không invoke ServerTravel; chỉ stamp Travel tag lên delegate payload để QA / designer verify save fires đúng tag trước khi wire vào real `UPaldarkBriefingSessionComponent::HostHubServer`. |
| 3 | `UPaldarkSaveSlotPickerWidget` (`Source/PaldarkLab/Public/Save/PaldarkSaveSlotPickerWidget.{h,cpp}`) | C++ UMG base class. 2 `BlueprintPure` accessor (GetSaveSubsystem, GetOwningPaldarkPC), 5 `BlueprintCallable` wrapper (RequestSave / RequestLoad / RequestDump / RequestClear / RequestList), 2 `BlueprintImplementableEvent` (K2_OnSaveCompleted, K2_OnLoadCompleted). `NativeOnInitialized` bind `OnSave/LoadCompleted` dynamic delegate; `NativeDestruct` unbind. Designer authors `WBP_PaldarkSaveSlotPicker : UPaldarkSaveSlotPickerWidget` để vẽ layout — picker plumbing handled C++. |
| 4 | 1 new gameplay tag + 1 new console command | `Paldark.Save.Trigger.Travel` (trigger #4) trong `PaldarkGameplayTags.{h,cpp}`. `Paldark.QA.HubToRaidHandoff [SlotName]` trong `PaldarkLab.{h,cpp}` — wrap `RequestHubToRaidHandoffSmoke` cho QA shell. |

### Auto-save trigger map

```
[Map load (any)]            FCoreUObjectDelegates::PostLoadMapWithWorld
   ↓                         ↓
   HandlePostLoadMapWithWorld(World)
   ↓                         ↓
   ├─ unbind prior world's UPaldarkMatchSubsystem::OnPlayerOutcomeChanged
   ├─ bind new world's UPaldarkMatchSubsystem::OnPlayerOutcomeChanged   ─→ HandlePlayerOutcomeChanged
   └─ once-per-GameInstance: bind UPaldarkNetSubsystem::OnPlayerLogout  ─→ HandlePlayerLogoutAutoSave

[Player extracted (raid)]   UPaldarkMatchSubsystem::OnPlayerOutcomeChanged
   ↓                         filter: PS != nullptr, Outcome == Extracted
   ↓                         filter: resolved PC is local controller
   HandlePlayerOutcomeChanged(PS, EPaldarkPlayerOutcome::Extracted)
   ↓                         ↓
   RequestSaveSlot(PC, GetDefaultSlotName(PC), Paldark.Save.Trigger.HubReturn)
   ↓                         ↓
   OnSaveCompleted.Broadcast(Slot, Result, Trigger=HubReturn)

[Player logout (any)]       UPaldarkNetSubsystem::OnPlayerLogout (BlueprintAssignable, server-side)
   ↓                         filter: Controller IsA APaldarkPlayerController
   ↓                         filter: PC->IsLocalController() (listen-host / standalone)
   HandlePlayerLogoutAutoSave(Controller)
   ↓                         ↓
   RequestSaveSlot(PC, GetDefaultSlotName(PC), Paldark.Save.Trigger.Logout)
   ↓                         ↓
   OnSaveCompleted.Broadcast(Slot, Result, Trigger=Logout)

[Hub→raid handoff smoke]    Paldark.QA.HubToRaidHandoff [SlotName]   (designer / QA console only)
   ↓                         ↓
   RequestHubToRaidHandoffSmoke(LocalPC, SlotName)
   ↓                         ↓
   RequestSaveSlot(PC, SlotName, Paldark.Save.Trigger.Travel)
   ↓                         ↓
   OnSaveCompleted.Broadcast(Slot, Result, Trigger=Travel)
```

### Delegate binding lifecycle

- `PostLoadMapWithWorld` — bound **once** trong `UPaldarkSaveSubsystem::Initialize()`, unbound trong `Deinitialize()`. Game-instance-scope, persists across map loads.
- `UPaldarkMatchSubsystem::OnPlayerOutcomeChanged` — bound **per-world** trong `HandlePostLoadMapWithWorld`. Mỗi map load unbind prior world (qua `TWeakObjectPtr<UPaldarkMatchSubsystem> BoundMatchSubsystem` + `FDelegateHandle MatchOutcomeHandle`) rồi bind world mới. Tránh double-bind nếu PostLoadMapWithWorld fire 2 lần (seamless travel).
- `UPaldarkNetSubsystem::OnPlayerLogout` — bound **once-per-GameInstance** trong `HandlePostLoadMapWithWorld` (lần đầu cả 2 subsystem cùng tồn tại). Guard bởi `bool bBoundNetLogout`. Game-instance-scope (subsystem persist across map loads), không cần per-world rebind.

### UMG slot picker shape (designer hand-off)

```
UPaldarkSaveSlotPickerWidget : UUserWidget   ← C++ base (W48)
├─ Abstract, Blueprintable                      designer subclass WBP_*
├─ NativeOnInitialized → bind OnSaveCompleted + OnLoadCompleted dynamic
├─ NativeDestruct       → unbind both delegates
│
├─ UFUNCTION BlueprintPure
│  ├─ UPaldarkSaveSubsystem*       GetSaveSubsystem()        // null on early startup
│  └─ APaldarkPlayerController*    GetOwningPaldarkPC()      // null on spectator/replay
│
├─ UFUNCTION BlueprintCallable
│  ├─ bool   RequestSave(FName Slot)          // → RequestSaveSlot(PC, Slot, Trigger.Manual)
│  ├─ bool   RequestLoad(FName Slot)          // → RequestLoadSlot(PC, Slot)
│  ├─ bool   RequestDump(FName Slot)          // → RequestDumpSlot(Slot)
│  ├─ bool   RequestClear(FName Slot)         // → ClearSlot(Slot)
│  └─ int32  RequestList()                    // → ListSlotsToLog()
│
└─ UFUNCTION BlueprintImplementableEvent
   ├─ K2_OnSaveCompleted(Slot, ResultTag, TriggerTag)   ← BP authors toast / SFX / refresh list
   └─ K2_OnLoadCompleted(Slot, ResultTag)               ← BP authors HUD refresh
```

Designer flow (W48 → Q1 polish):
1. BP designer mở `WBP_PaldarkSaveSlotPicker` (subclass `UPaldarkSaveSlotPickerWidget`).
2. Author layout: list view bind sang `GetSaveSubsystem->ListSlotsToLog` (future iteration: thêm structured `ListSlots()` returns `TArray<FName>`).
3. Button `Save`/`Load`/`Delete` → bind sang `RequestSave/Load/Clear`.
4. Override `K2_OnSaveCompleted` → check `ResultTag == Paldark.Save.Result.Success` → flash toast xanh + refresh list; else flash toast đỏ với ResultTag string.
5. Override `K2_OnLoadCompleted` → refresh HUD (hotbar, mini-map roster).

### Console test loop (W48 smoke)

```
# Verify hub→raid handoff fires Travel tag.
> Paldark.QA.HubToRaidHandoff QASmokeSlot
[Paldark.QA.HubToRaidHandoff] slot=QASmokeSlot trigger=Paldark.Save.Trigger.Travel queued=ok
[Save][W48] hub→raid smoke save slot=QASmokeSlot
[Save] async write begin slot=QASmokeSlot trigger=Paldark.Save.Trigger.Travel schema=1 ...
[Save] async write complete slot=QASmokeSlot user=0 result=Paldark.Save.Result.Success trigger=Paldark.Save.Trigger.Travel

# Verify auto-save on extract (raid map). Force-extract local player...
> Paldark.Match.Extract        # W24-25 console
[Match] PC=PaldarkPlayerController_0 outcome=Extracted (auto-save triggered)
[Save][W48] auto-save on extract slot=Paldark_<NetID> trigger=Paldark.Save.Trigger.HubReturn
[Save] async write begin ... trigger=Paldark.Save.Trigger.HubReturn ...
[Save] async write complete ... result=Paldark.Save.Result.Success trigger=Paldark.Save.Trigger.HubReturn

# Verify auto-save on logout (server-side, listen-host).
> disconnect                    # local PC quits
[Net] OnPlayerLogout broadcast for PaldarkPlayerController_0
[Save][W48] auto-save on logout slot=Paldark_<NetID> trigger=Paldark.Save.Trigger.Logout
[Save] async write begin ... trigger=Paldark.Save.Trigger.Logout ...
```

### Designer test loop (in UE Editor, after this PR lands)

1. Start PIE với `Map_HubTown` (W40-41 hub).
2. Companion roster + items (giống W47 test loop).
3. **Smoke test Travel trigger**: console `Paldark.QA.HubToRaidHandoff` → log dump `result=Success trigger=Travel`. Slot xuất hiện qua `Paldark.Save.ListSlots`.
4. **Auto-save on extract**: ServerTravel sang raid map (e.g. `Paldark.Hub.QA.ForceTravel Paldark.Map.Raid_Sandbox`). Console `Paldark.Match.Extract` → expect log `[Save][W48] auto-save on extract`.
5. **Auto-save on logout**: standalone client disconnect → expect log `[Save][W48] auto-save on logout` trên listen-host trước khi PC destroy.
6. **Toggle off**: PIE `Paldark.Save.Toggle Extract 0` (future iteration nếu thêm console toggle) hoặc edit `UPaldarkSaveSubsystem::bAutoSaveOnExtract = false` trong DefaultGame.ini. Re-run step 4 → expect no auto-save log.

### Validator extension

`scripts/ci/validate_paldarklab.py::check_w48_polish_shape` enforce 4 nhóm contract:

1. **Subsystem auto-save plumbing** — `bAutoSaveOnExtract` + `bAutoSaveOnLogout` UPROPERTY, `RequestHubToRaidHandoffSmoke` API, 3 handler (`HandlePostLoadMapWithWorld`, `HandlePlayerOutcomeChanged`, `HandlePlayerLogoutAutoSave`), member state (`BoundMatchSubsystem` weak ref, `MatchOutcomeHandle`, `bBoundNetLogout`). `.cpp` phải có delegate `AddUObject` / `AddDynamic`, 3 trigger tag (HubReturn/Logout/Travel), filter `EPaldarkPlayerOutcome::Extracted`.
2. **UMG picker stub** — header có UCLASS Abstract Blueprintable, 2 BP-pure + 5 BP-callable + 2 BP-implementable-event. `.cpp` có NativeOnInitialized/Destruct override + dynamic delegate bind/unbind + Trigger.Manual cho direct save.
3. **Travel tag definition** — `UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Save_Trigger_Travel, "Paldark.Save.Trigger.Travel")` trong `PaldarkGameplayTags.cpp` (header check đã trong global `EXPECTED_GAMEPLAY_TAGS`).
4. **Console command registration** — `Paldark.QA.HubToRaidHandoff` string trong `PaldarkLab.cpp`, handler `HandleQAHubToRaidHandoffCommand` symbol, `QAHubToRaidHandoffCommand` IConsoleObject member trong `.h`.

### Anti-patterns avoided (W48)

- **Bind auto-save delegate trong constructor / không unbind**: dùng `Initialize()` + `Deinitialize()` lifecycle. PostLoadMapWithWorld bound once + unbound trong `Deinitialize`. Match outcome delegate per-world unbind trong `HandlePostLoadMapWithWorld` trước khi rebind world mới.
- **Pin subsystem reference với strong UPROPERTY → world teardown leak**: `BoundMatchSubsystem` là `TWeakObjectPtr<UPaldarkMatchSubsystem>` để tránh ghim world subsystem qua map travel boundary.
- **Auto-save trigger không filter local controller**: dedicated server fire `OnPlayerOutcomeChanged` cho mọi player; nếu không filter `IsLocalController()` thì dedicated server save mọi player vào cùng slot. Filter ngăn cross-write; per-player dedicated save defer Q1-next.
- **Double-bind net logout trên seamless travel**: PostLoadMapWithWorld fire 2 lần với seamless travel; nếu bind `OnPlayerLogout` mỗi lần thì auto-save fire N+1 lần. Guard `bBoundNetLogout` đảm bảo once-per-GameInstance.
- **UMG widget hold strong ref đến subsystem qua UPROPERTY → GameInstance teardown leak**: `BoundSubsystem` trong picker là `TObjectPtr<UPaldarkSaveSubsystem>` (transient) nhưng `NativeDestruct` clear ref trước super::Destruct.
- **Console command đụng ServerTravel trong smoke path**: `Paldark.QA.HubToRaidHandoff` chỉ stamp Travel tag, không invoke `UPaldarkBriefingSessionComponent::HostHubServer`. Smoke path tách rời travel plumbing — designer / QA test save-on-travel mà không cần authority hoặc fully-loaded session.

### Out of scope (defer to Q1-next)

- **Real ServerTravel hook** — hiện chỉ smoke via console command. Future: wire `UPaldarkBriefingSessionComponent::HostHubServer` (W46) gọi `RequestHubToRaidHandoffSmoke` trước `World->ServerTravel()` (lúc đó rename `Smoke` → `RequestTravelBoundarySave`).
- **Dedicated-server per-player save**: hiện auto-save filter `IsLocalController()` nên dedicated server không fire. Future: dedicated server save mỗi player vào slot riêng per `NetID` qua extension hook trong `HandlePlayerOutcomeChanged`.
- **Auto-save throttle / debounce**: nếu match flap Extracted ↔ Spawned 5 lần/giây thì auto-save fire 5 lần. Future: 5-second debounce trong subsystem.
- **UMG slot picker layout authoring**: designer task. C++ base sẵn sàng; `WBP_PaldarkSaveSlotPicker` BP subclass + list view + button row author trên top.
- **Auto-save UI feedback**: hiện chỉ log; future iteration UMG toast widget bind sang `OnSaveCompleted` để show "Saving…" / "Saved!" trên HUD.
- **Save migration when SchemaVersion bumps**: hiện schema mismatch stamp `Fail.SchemaMismatch` rồi bỏ qua. Q1-next: migration data asset chain.

## W49-50 — Q4 Milestone Build (devlog + playtest checklist + bug bash)

W49-50 đóng Q4 quarter (W40-48). Không có feature week mới — chỉ tổng hợp Q4 ledger, đo coverage thực tế của pillar, gap-analysis vs alpha-demo claim, và lock Q1-next sprint plan. Cộng dồn Q1 (W13) + Q2 (W26) + Q3 (W39) + Q4 (W48), repo đạt:

- **34 PR Devin merged** (Q1=10, Q2=6, Q3=7, Q4=11)
- **~258 C++ source file** (Q3=180 → Q4=258, +78)
- **185 native gameplay tag** (Q3=~90 → Q4=185, +95)
- **~60 console command** (Q3=45 → Q4=60, +15)
- **12 subsystem** (Q3=7 → Q4=12, +5: HubSubsystem + BackendSubsystem + SaveSubsystem + Briefing + Wallet)
- **15 primary asset type** (Q3=11 → Q4=15, +4)
- **5 UMG widget base** (Q3=0 → Q4=5: Briefing + Stable + Marketplace + SaveSlotPicker + W43 placeholder)
- **7,962 validator dòng** (Q3=5,400 → Q4=7,962, +2,562)
- **~80% pillar coverage** (target 84%; P17 audio -35 chậm vì FMOD bị reassign Q1-next)

### Q4 PR ledger pointer

| PR | Week | Title |
|----|------|-------|
| #35 | Q3 follow-up | L-21 RequiredGameFeatures[] auto-activate |
| #36 | Q3 follow-up | L-20 NetSubsystem BlueprintAssignable login/logout |
| #37 | Q3 follow-up | L-34 Dedicated server feature plugin auto-activate |
| #38 | Q3 follow-up | L-28 Boltmane phase HP thresholds per-DA |
| #39 | Q3 follow-up | L-32 Inventory FastArraySerializer scaffold |
| #40 | W40-41 | Hub Town scaffold |
| #41 | W42-43 | AWS backend integration (Cognito + Lambda + DynamoDB) |
| #42 | W44-45 | Stable & Marketplace UI |
| #43 | W46 | Hub QA pass — Briefing room |
| #44 | W47 | Save game system |
| #45 | W48 | Polish — auto-save hooks + UMG slot picker + handoff smoke |

### Designer hand-off (5-6 ngày authoring critical path cho alpha demo)

| ID | Task | Estimate |
|----|------|----------|
| L-46 | `WBP_PaldarkBriefingWidget` BP subclass (vote panel + ready toggle + countdown HUD) | ~1 ngày |
| L-47 | `WBP_PaldarkStableWidget` + `WBP_PaldarkMarketplaceWidget` BP subclass | ~2 ngày |
| L-49 | `Map_HubTown.umap` + `DA_PaldarkExperience_HubTown` + 3 building placement | ~2 ngày |
| L-50 | `DA_MarketplaceCatalog_Default` (~30 entry buy/sell price tuning) | ~½ ngày |
| L-48 | `WBP_PaldarkSaveSlotPicker` BP subclass (P2 nice-to-have) | ~½ ngày |

### Q1-next sprint plan (post-alpha)

1. **Animation back-fill** (2 week dedicated, đóng P05 -15 carry từ Q3): `ABP_PaldarkPlayer` + `ABP_PaldarkPal` + 5 hostile-Pal AnimBP subclass.
2. **FMOD integration** (2 week, đóng P17 -35 deferred): UE FMOD plugin install + initial event sound bank + ambient music wire.
3. **Real AWS deploy** (~5 ngày SRE): Cognito user pool + identity pool + DynamoDB `Paldark-Sessions` table + 4 Lambda function + API Gateway + IAM role. Flip `bUseAWSBackend=True`.
4. **Validator polish followup**: L-04 + L-05 + L-09 + L-17 (4 P2 từ Q1/Q2 carry).
5. **Save migration data asset chain** (L-43): cho schema bump > 1.

### Docs landed

- [`Documents/Devlog/Q4-Milestone-Build.md`](../Documents/Devlog/Q4-Milestone-Build.md) — Q4 PR ledger + pillar coverage (11 pillar tracked) + gap analysis vs roadmap + 20-issue known-issues list + cumulative Q1+Q2+Q3+Q4 stats.
- [`Documents/Devlog/Q4-Playtest-Checklist.md`](../Documents/Devlog/Q4-Playtest-Checklist.md) — runnable 4-client PIE walkthrough (12 step + §0 setup delta cho Q4).
- [`Documents/Devlog/Q4-Bug-Bash.md`](../Documents/Devlog/Q4-Bug-Bash.md) — 20-issue priority list cho 1-day bug bash + 4 morning/afternoon slot plan + closure criteria.

### Validator coverage Q4

CI structural validator (`scripts/ci/validate_paldarklab.py`, ~7,962 dòng) check 30+ shape (Q3 + Q4 cumulative):
- W40-41 Hub Town shape (experience + map + buildings + 8-player shard)
- W42-43 Backend shape (subsystem + Lambda type + session + command set)
- W44-45 Stable + Marketplace shape (deposit + 2 controller + wallet + catalog + 2 widget base)
- W46 Hub QA shape (briefing component + terminal + widget + FSM phases + replicated vote/ready arrays)
- W47 Save game shape (subsystem + types + player save + snapshot helpers + 7 tags + 6 commands)
- W48 Polish shape (auto-save hooks + UMG slot picker base + Travel tag + HubToRaidHandoff command)

### Out of scope (Q4 close-out)

W49-50 không author code. Tất cả còn lại là:
1. Designer asset authoring (5-6 ngày critical path, xem L-46/L-47/L-49/L-50)
2. QA 12-step playtest checklist run (designer task)
3. 90-second alpha demo video record + embed
4. Internal alpha demo review meeting + Q1-next scope freeze

## Anti-patterns đã tránh

- Đặt log category trong game module → editor không log được. Fix: đặt trong `PaldarkLabCore` (PreDefault).
- Game module phụ thuộc Editor module → Game build fail. Fix: Editor module ở `Source/PaldarkLabEditor/` + Target Editor only.
- Console cmd register trong constructor → race condition. Fix: register trong `StartupModule`.
- Subsystem dùng raw `UObject*` không UPROPERTY → GC eat. Fix: `UPROPERTY()` trên `TrackedActors`.
- Experience load synchronous trong InitGame → block server startup. Fix: `UAssetManager::LoadPrimaryAsset` async + `OnExperienceAssetReady` callback.
- Replicate full `UPaldarkExperienceDefinition*` → giant payload mỗi client connect. Fix: replicate `FPrimaryAssetId`, client resolve soft pointer locally trên RepNotify.
- Đặt experience config trong `DefaultEngine.ini` `[Core.Log]` thay vì class wiring section — fix: dùng đúng `[/Script/Engine.Engine]` + `[/Script/EngineSettings.GameMapsSettings]`.
- Activity FSM đặt `AddMovementInput` vào `EnterActivity` thay vì `TickActivity` — movement input phải bơm mỗi frame. Fix: chỉ dùng Enter cho state setup (flag flips, montage cues), đã kiểm tra trong Investigate.
- FSM eval không có hysteresis → Pal flicker Idle↔Follow quanh distance threshold. Fix: tách `CanRun()` (khởi tạo) và `ShouldContinue()` (giữ nguyên) với 2 ngưỡng khác nhau (EnterDistance > ExitDistance).
- FSM chạy cả client → race condition + de-sync. Fix: `BeginPlay`/`TickComponent` early-return nếu `!GetOwner()->HasAuthority()`; client thấy kết quả qua `Character->SetReplicateMovement(true)`.
