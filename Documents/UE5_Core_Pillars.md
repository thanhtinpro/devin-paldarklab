# UE5 Core Pillars — Tổng hợp trụ cột kỹ thuật

> Tổng hợp từ 12 khóa Udemy + 1 khóa Tom Looman trong repo. Mỗi pillar là một **năng lực kỹ thuật** mà bạn cần master để làm 1 game UE5 production. Dưới mỗi pillar liệt kê các khóa dạy nó + mức độ sâu.

**Quy ước mức độ:**
- 🟢 **Surface** — chỉ giới thiệu / lướt.
- 🟡 **Working** — đủ làm 1 system thực tế.
- 🟠 **Production** — có pattern tối ưu, lỗi corner case.
- 🔴 **Mastery** — kiến trúc / scale / debug nâng cao.

---

## P1. C++ & Build Foundation
**Năng lực:** viết được Unreal C++ idiomatic, biết module/build system, debug, version control.

| Khóa | Mức | Nội dung |
|------|-----|----------|
| [12] Pro UE4 Game Coding | 🟠 | Editor, Visual Studio, source build, Git, coding standard |
| [01] ActionRoguelike | 🟡 | Build.cs, module include, log category |
| [08][13] Dedicated Servers | 🟠 | Server target, packaging, command-line |

**Concepts:** `UCLASS / USTRUCT / UENUM`, `UPROPERTY / UFUNCTION`, module dependency, Unreal Header Tool, GarbageCollection, smart pointers (`TWeakObjectPtr`, `TSharedPtr`), `LogCategory`.

---

## P2. Core Class Framework
**Năng lực:** biết khi nào dùng `GameMode` vs `GameState` vs `PlayerController` vs `PlayerState` vs `HUD` vs `GameInstance` vs `WorldSubsystem`.

| Khóa | Mức | Ghi chú |
|------|-----|--------|
| [07] MP Crash Course Section 5 | 🟠 | Bảng đối chiếu Authority cho từng class |
| [01] ActionRoguelike | 🟡 | Áp dụng vào single-player |
| [10][11][13] | 🟠 | Áp dụng đa dạng game type |

**Concepts:**
- **GameMode** server-only, định luật chơi, spawn.
- **GameState** replicated, trạng thái match shared.
- **PlayerController** mỗi client 1 cái, owner-only.
- **PlayerState** mỗi player 1 cái, replicated.
- **Pawn / Character** vật thể trong world.
- **HUD** owning client.
- **GameInstance** xuyên level, xuyên travel.
- **Subsystem** (GameInstance / World / LocalPlayer / Engine) — singleton có lifetime.

---

## P3. Actor / Component / Subsystem composition
**Năng lực:** chọn đúng giữa kế thừa và composition; biết khi nào tạo Subsystem vs Manager Actor.

| Khóa | Mức | Ghi chú |
|------|-----|--------|
| [01] ActionRoguelike | 🟠 | `AttributeComponent`, `ActionComponent` → pattern composition |
| [09] Inventory | 🟠 | `EquipmentComponent`, Item fragments — composite pattern |
| [14] Lyra | 🔴 | Modular Gameplay Plugin, Game Feature Plugin, Pawn Extension |

**Anti-pattern:** monolithic `Character` 368KB của ReadyOrNot — vi phạm Single Responsibility.

---

## P4. Enhanced Input
**Năng lực:** binding action/axis bằng `InputAction` + `InputMappingContext`, không xài input legacy.

| Khóa | Mức |
|------|-----|
| [01] ActionRoguelike | 🟠 |
| [11] GAS RPG | 🟠 (`InputConfigDataAsset` + custom `EnhancedInputComponent`) |
| [15] Lyra RPG | 🟠 (Lyra Input Config) |

**Concepts:** `IA_*`, `IMC_*`, `Trigger`, `Modifier`, `PressedTriggerEvent`. Pairing với `GameplayTag` để abstract input → ability.

---

## P5. Animation
**Năng lực:** Animation Blueprint, State Machine, AnimNotify, Animation Montage, RootMotion, IK.

| Khóa | Mức |
|------|-----|
| [01] ActionRoguelike | 🟡 |
| [10] MP Shooter | 🟠 (recoil, aim offsets) |
| [13] Crunch | 🟠 (combo system) |
| [15] Lyra RPG | 🟠 (Lyra Locomotion System, distance matching) |

**Concepts:** `UAnimInstance`, `UAnimMontage`, `AnimNotify_*`, `RootMotionSource`, `MotionWarping`.

---

## P6. Multiplayer / Replication
**Năng lực:** replicate biến và RPC đúng, hiểu authority, prediction, lag compensation.

| Khóa | Mức | Coverage |
|------|-----|---------|
| [07] MP Crash Course | 🟠 | Full lý thuyết: ActorReplication, RepNotify, RPC, NetRole, Ownership |
| [10] MP Shooter | 🔴 | **Lag Compensation / Server-Side Rewind** — quan trọng nhất |
| [13] Crunch | 🟠 | GAS replicate, dedicated server containerization |

**Concepts:** `DOREPLIFETIME`, `Replicated_ReadOnly`, `RepNotify`, `Server/Client/Multicast`, `IsLocallyControlled / HasAuthority`, `Net Update Frequency`, `Significance Manager`, **Server-Side Rewind** (rewind hitbox theo timestamp).

---

## P7. Dedicated Server + Sessions
**Năng lực:** package Server target, host listen vs dedicated, session management, matchmaking.

| Khóa | Mức |
|------|-----|
| [07] MP Crash | 🟡 (Travel, Steam listen) |
| [08] AWS GameLift | 🔴 (Fleet, Anywhere Fleet, Lambda Auth, DynamoDB) |
| [13] Crunch | 🔴 (Coordinator + AWS Deploy + Containerize) |

**Concepts:** GameLift Fleet/AnywhereFleet, ServerSDK, OnlineSubsystem, EOS/Steam, Matchmaker, Lobby + Coordinator pattern, container/Docker, ECS task definitions.

---

## P8. Gameplay Ability System (GAS)
**Năng lực:** dùng GAS chính chủ cho ability + buff + damage; biết khi nào *không* nên dùng GAS.

| Khóa | Mức |
|------|-----|
| [05] GAS Crash | 🟡 (ASC, AS, GA, GE, Tags, Cues, Tasks) |
| [01] Tom Looman | 🟢 (Action System custom — học pattern trước GAS) |
| [11] Aura RPG | 🔴 (Damage chain, Spells, Passive, Save) |
| [13] Crunch | 🔴 (GAS optimization, headshot, level-up) |
| [14][15] Lyra | 🔴 (Lyra Attribute Set, Lyra Game Phase) |

**Concepts:** `UAbilitySystemComponent`, `UAttributeSet`, `UGameplayAbility`, `UGameplayEffect`, `FGameplayTag`, `GameplayCue`, `AbilityTask`, **Modifier Magnitude Calculation (MMC)**, **Execution Calculation**, **Gameplay Effect Context**.

---

## P9. AI — Behavior Tree / State Tree / Utility
**Năng lực:** dựng AI có decision-making khả thi cho production.

| Khóa | Mức |
|------|-----|
| [01] ActionRoguelike | 🟠 (BT + EQS + Blackboard) |
| [11] Aura RPG | 🟡 (AI ranged/melee/spell) |
| [15] Lyra RPG | 🟠 (**State Tree**) |
| [12] UE4 Pro | 🟢 (AI giới thiệu) |
| [04] ReadyOrNot study | 🔴 (Activity FSM + Utility AI) |

**Concepts:** `UBehaviorTree`, `UBlackboardData`, `EQS`, `StateTreeComponent`, `UAIPerceptionComponent`, custom `AIAction` + `AIActionConsideration` (Utility AI), **Activity-based FSM** (như ReadyOrNot).

---

## P10. UI — UMG / CommonUI
**Năng lực:** widget binding, responsive layout, focus management, controller support.

| Khóa | Mức |
|------|-----|
| [01] Tom Looman | 🟡 |
| [11] Aura RPG | 🟠 (Attribute Menu, Spell Menu, full HUD) |
| [10] MP Shooter | 🟠 |
| [15] Lyra RPG | 🟠 (FrontEnd Menu, Indicator Manager, Quick Bar) |
| [14] Lyra | 🟠 (CommonUI, Activatable Widget) |

**Concepts:** `UUserWidget`, `UCommonActivatableWidget`, `WidgetSwitcher`, `RichTextBlock`, `Async UI loading`, `IndicatorManager`.

---

## P11. Inventory & Equipment
**Năng lực:** lưu/load item, stack, drag-drop, equipment slot, persistence.

| Khóa | Mức |
|------|-----|
| [09] Inventory | 🔴 (16 chương: HUD, Fragment, Stack, Pick Up, Hover, Composite, Equipping, Equipment Component) |
| [11] Aura RPG | 🟡 (inventory đơn giản) |
| [13] Crunch | 🟠 (shop + inventory) |
| [15] Lyra RPG | 🟠 (Lyra Inventory Quick Bar + Interaction) |

**Patterns:** **Item Fragments** (component-style item composition), **Composite Pattern** cho group items, Equipment slot data-driven.

---

## P12. Data-Driven Design
**Năng lực:** tách logic khỏi data — DataTable, DataAsset, PrimaryDataAsset, Curve.

| Khóa | Mức |
|------|-----|
| [01] Tom Looman | 🟠 (DataTable spawn bots, DataAsset enemy config, Asset Manager async) |
| [11] Aura RPG | 🟠 (Curve Table cho XP, Damage) |
| [15] Lyra RPG | 🟠 (Lyra Experience = DataAsset) |
| [04] ReadyOrNot study | 🔴 (42 DataAsset cho item/level/AI/customization) |

**Concepts:** `UDataTable + FTableRowBase`, `UDataAsset`, `UPrimaryDataAsset`, `UAssetManager`, `FPrimaryAssetId`, **Async Load**, `FStreamableManager`.

---

## P13. Save / Load System
**Năng lực:** persist player + world state qua session.

| Khóa | Mức |
|------|-----|
| [01] Tom Looman | 🟠 (SaveGame archive, transform, attribute) |
| [11] Aura RPG | 🟠 (Save Progress + Checkpoints + Map entrance) |

**Concepts:** `USaveGame`, `FMemoryWriter / FMemoryReader`, `FObjectAndNameAsStringProxyArchive`, replay-style serialization, checkpoint pattern.

---

## P14. Asset Manager + Async Loading
**Năng lực:** load asset bất đồng bộ, tránh hard-reference chặn tải level.

| Khóa | Mức |
|------|-----|
| [01] Tom Looman | 🟠 (Async DataAsset, Async UI icon) |
| [14] Lyra | 🔴 (Game Features Plugin = async load gameplay) |

**Concepts:** `FPrimaryAssetId`, `FStreamableManager`, `FSoftObjectPath`, `Game Feature Plugin`.

---

## P15. Performance & Production-grade
**Năng lực:** profile, fix hitch, async, pooling, significance.

| Khóa | Mức |
|------|-----|
| [01] Tom Looman | 🟠 (Aggregate Ticking, Actor Pooling, Data-Oriented projectiles experimental) |
| [12] UE4 Pro | 🟡 (graphics, audio) |
| [14] Lyra | 🟠 (Performance Optimization section) |
| [04] ReadyOrNot | 🔴 (Significance Manager + custom AI cull) |

**Concepts:** `Insights`, `Significance Manager`, **Object Pooling**, **Aggregate Ticking**, PSO precaching, Data-Oriented Design (DoD).

---

## P16. Math / Physics / Vehicle / Audio (foundation)
**Năng lực:** vector/quaternion math, easing/interp, FMOD/audio, vehicle dynamics, camera.

| Khóa | Mức |
|------|-----|
| [12] UE4 Pro | 🔴 (14 chương riêng — math, motion, vehicle x3, time, graphics, audio, AI, camera, pickup) |
| [04] ReadyOrNot study | 🟠 (FMOD propagation, ragdoll, cover gen) |

**Concepts:** Vectors, Dot/Cross, Quaternion, Rotator, Transform, Lerp/Slerp, Easing curves, Sub-stepping, Chaos Vehicle, FMOD Studio integration.

---

## P17. Lyra Framework
**Năng lực:** đọc/extend Lyra — Experiences, Game Features, Modular Gameplay.

| Khóa | Mức |
|------|-----|
| [14] Exploring Lyra | 🔴 (deep dive + extend abilities) |
| [15] Lyra RPG | 🔴 (build RPG on Lyra: Locomotion, AttributeSet, Inventory, Indicator) |

**Concepts:** `ULyraExperienceDefinition`, `ULyraExperienceActionSet`, **Game Feature Plugin**, **Modular Gameplay** (`AddComponents`, `AddAbilities`), `LyraCommonUI`, **State Tree** for AI.

---

## P18. Backend / Live Ops
**Năng lực:** tích hợp dịch vụ ngoài Unreal: auth, leaderboard, matchmaking, telemetry.

| Khóa | Mức |
|------|-----|
| [08] AWS GameLift | 🔴 (Lambda, Cognito, API Gateway, Access Token, DynamoDB) |
| [13] Crunch | 🔴 (Server Coordinator + AWS deploy + Containerize) |
| [04] ReadyOrNot study | 🟠 (mod.io, Steam, EOS, PS5 Activities) |

**Concepts:** AWS Lambda, API Gateway, Cognito User Pool + Identity Pool, JWT access token, DynamoDB partition key, GameLift Fleet, EC2/ECS, Docker container, mod.io SDK, Steam workshop, EOS.

---

## 19 — Map pillar ↔ khóa (matrix)

| Pillar | 01 | 05 | 07 | 08 | 09 | 10 | 11 | 12 | 13 | 14 | 15 |
|--------|----|----|----|----|----|----|----|----|----|----|----|
| P1 C++/Build       | 🟡 |    |    | 🟠 |    |    |    | 🟠 | 🟠 |    |    |
| P2 Core Framework  | 🟡 |    | 🟠 |    |    | 🟠 | 🟠 |    | 🟠 |    |    |
| P3 Composition     | 🟠 |    |    |    | 🟠 |    |    |    |    | 🔴 |    |
| P4 Enhanced Input  | 🟠 |    |    |    |    |    | 🟠 |    |    |    | 🟠 |
| P5 Animation       | 🟡 |    |    |    |    | 🟠 |    |    | 🟠 |    | 🟠 |
| P6 Replication     |    |    | 🟠 |    |    | 🔴 |    |    | 🟠 |    |    |
| P7 Dedicated Server|    |    | 🟡 | 🔴 |    |    |    |    | 🔴 |    |    |
| P8 GAS             | 🟢 | 🟡 |    |    |    |    | 🔴 |    | 🔴 | 🔴 | 🔴 |
| P9 AI              | 🟠 |    |    |    |    |    | 🟡 | 🟢 |    |    | 🟠 |
| P10 UI             | 🟡 |    |    |    |    | 🟠 | 🟠 |    |    | 🟠 | 🟠 |
| P11 Inventory      |    |    |    |    | 🔴 |    | 🟡 |    | 🟠 |    | 🟠 |
| P12 Data-driven    | 🟠 |    |    |    |    |    | 🟠 |    |    |    | 🟠 |
| P13 Save/Load      | 🟠 |    |    |    |    |    | 🟠 |    |    |    |    |
| P14 AssetMgr/Async | 🟠 |    |    |    |    |    |    |    |    | 🔴 |    |
| P15 Performance    | 🟠 |    |    |    |    |    |    | 🟡 |    | 🟠 |    |
| P16 Math/Physics   |    |    |    |    |    |    |    | 🔴 |    |    |    |
| P17 Lyra           |    |    |    |    |    |    |    |    |    | 🔴 | 🔴 |
| P18 Backend        |    |    |    | 🔴 |    |    |    |    | 🔴 |    |    |

> Trống = không cover. 1 hàng có thể có nhiều cột.

---

## 20 — Cách dùng tài liệu này để đánh giá dự án

Mỗi project assessment (`Documents/Projects/<name>_Assessment.md`) sẽ có 1 bảng:

| Pillar | Tình trạng | % | Bằng chứng | Còn thiếu |
|--------|-----------|---|------------|----------|

Trong đó:
- **% = covered + working / target_for_genre**.
- **Bằng chứng** trỏ tới file C++ trong `Source/`.
- **Còn thiếu** liệt kê class/system cần làm (mapping về Course nào dạy nó).

---

## 21 — Pillar không có trong 12 course (cảnh báo gap kiến thức)

| Pillar khuyết | Cần học ngoài 12 khóa | Tài liệu khuyên |
|---------------|----------------------|------------------|
| Audio nâng cao (FMOD) | Không course nào dạy FMOD sâu | FMOD Studio docs + plugin ReadyOrNot |
| Networking topology (P2P / Mirror) | Tất cả khóa đều client-server | EOS / Steamworks docs |
| Anti-cheat / Security | Không cover | BattlEye, EAC docs |
| Localization | Mức nhập môn ở [14] | Unreal Localization Dashboard docs |
| Editor extension | Không course nào dạy custom EditorMode | UnrealEd docs |
| Replays / Demo recording | Không cover | Unreal Replay Subsystem docs |
| Asset cooking / Build pipeline | [08][13] chỉ chạm tới packaging | UAT scripts |

> Nếu dạy AI làm game thật, cần bổ sung tài liệu cho các pillar trên — KHÔNG được tự suy diễn.
