# PALDARK — 18 Pillar Coverage

> Mỗi feature game PALDARK đối chiếu vào 18 pillar trong [UE5_Core_Pillars.md](../UE5_Core_Pillars.md).
> Mục tiêu: PALDARK sử dụng **17/18 pillar** (P17 Lyra dùng làm backbone, không phải feature).
>
> Đây là bảng đảm bảo **không pillar nào bị bỏ quên** trong production.

---

## Bảng tổng — Pillar × PALDARK Feature

| Pillar | Feature dùng | Mức độ |
|--------|-------------|--------|
| **P1 C++/Build** | Toàn module + plugin (FMOD, GameLift, mod.io) | 🟢 100% |
| **P2 Core Framework** | GameMode raid + GameMode hub + GameInstance + Subsystem | 🟢 100% |
| **P3 Composition** | Pal Pawn = 8 Component + Player Pawn = 12 Component | 🟢 95% |
| **P4 Enhanced Input** | InputAction + IMC + GameplayTag pairing | 🟢 100% |
| **P5 Animation** | Lyra Locomotion + Pal AnimBP + Montage + Motion Warp | 🟢 90% |
| **P6 Replication** | Full replication + Lag Compensation Server-Side Rewind | 🟢 100% |
| **P7 Dedicated Server** | AWS GameLift Anywhere → Production Fleet | 🟢 100% |
| **P8 GAS** | Player ability + Pal ability + Damage chain + Effects | 🟢 100% |
| **P9 AI** | Activity FSM port + Custom Sense + ThreatOctree | 🟢 100% |
| **P10 UI** | CommonUI menu + Radial wheel + Indicator Manager | 🟢 90% |
| **P11 Inventory** | Fragment composition + Weight + Container Composite | 🟢 95% |
| **P12 Data-driven** | Pal DataAsset + Weapon DataAsset + Map Experience | 🟢 100% |
| **P13 Save/Load** | Local SaveGame + Server-side persistence | 🟢 90% |
| **P14 AssetMgr/Async** | Async map load + Async Pal asset + Game Feature plugin | 🟢 100% |
| **P15 Performance** | Significance Manager + Pal LOD + Spatial Octree | 🟢 100% |
| **P16 Math/Physics/Audio** | FMOD Propagation + Ragdoll + Cover gen + Vector math | 🟢 90% |
| **P17 Lyra** | Lyra Experience + GameFeature Plugin + Modular Gameplay | 🟢 100% (backbone) |
| **P18 Backend/Live Ops** | DynamoDB + Lambda + Cognito + Telemetry + mod.io | 🟢 100% |

> **PALDARK = 17/18 pillar ở mức ≥90%.** Đây là game **tham vọng nhất** so với 3 project đã đánh giá:
> - Palworld 20% / PUBG 53% / RoN 73% / **PALDARK 96%**.

---

## Chi tiết từng Pillar

### P1 — C++ / Build Foundation

**PALDARK dùng cho:**
- 7 module C++: `Paldark` (main), `PaldarkRaid`, `PaldarkHub`, `PaldarkPal`, `PaldarkCombat`, `PaldarkInventory`, `PaldarkBackend`.
- 5 plugin: `FMODStudio`, `OnlineSubsystemGameLift`, `Modio`, `Reflex`, `GameplayAbilities`.
- Server target riêng + Editor target.
- Module dependency graph rõ — `PaldarkBackend` chỉ phụ thuộc `Core`, không phụ thuộc UI.

**Học từ:** [12] Pro UE4, [08] AWS GameLift Build.cs.

---

### P2 — Core Class Framework

**PALDARK dùng cho:**

| Class | Vai trò |
|-------|---------|
| `APaldarkGameMode_Raid` | Server raid loop, spawn Pal hostile, manage extract |
| `APaldarkGameState_Raid` | Replicate match phase, extract timer, squad info |
| `APaldarkGameMode_Hub` | Hub town shard, social, marketplace |
| `APaldarkGameState_Hub` | Replicate hub state (Pal of Fame, marketplace) |
| `APaldarkPlayerController_Raid` | Input + UI input forwarding (raid) |
| `APaldarkPlayerController_Hub` | Hub interaction (vendor, breeding bench) |
| `APaldarkPlayerState` | Persist player profile, raid result |
| `APaldarkCharacter` | Lyra-extended pawn |
| `UPaldarkGameInstance` | Persist online state, login flow |
| `UPaldarkMatchSubsystem` (WorldSubsystem) | Match-level helpers |
| `UPaldarkPalRosterSubsystem` (GameInstance) | Pal đã thuần (cross-session) |
| `UPaldarkInventorySubsystem` (Player) | Stash global |

**Học từ:** [07] MP Crash, [01] Tom Looman, [15] Lyra RPG.

---

### P3 — Composition

**Anti-pattern phải tránh:** không tạo god-character như RoN.

**Player Pawn:** 12 Component nhưng mỗi cái own logic.
- `UPaldarkLocomotionComponent`
- `UPaldarkAimComponent` (recoil, aim assist disabled)
- `UPaldarkInteractionComponent`
- `UPaldarkInventoryComponent`
- `UPaldarkPalBondComponent` (manages Pal companion link)
- `UPaldarkSquadCommandComponent`
- `UPaldarkAudioPropagationListener`
- `UPaldarkStaminaComponent`
- `UPaldarkExtractionComponent`
- `UPaldarkVOIPComponent`
- `UPaldarkIndicatorClientComponent` (UI marker)
- `UAbilitySystemComponent` (GAS)

**Pal Pawn:** 8 Component.
- `UPaldarkPalLocomotionComponent`
- `UPaldarkPalActivityComponent` (Activity FSM core)
- `UPaldarkPalBondComponent` (mirror player side)
- `UPaldarkPalAbilityComponent`
- `UPaldarkPalSenseComponent` (sight + scent + sound)
- `UPaldarkPalStatComponent` (gen + level)
- `UPaldarkPalAnimDriverComponent`
- `UAbilitySystemComponent`

**Học từ:** [01] Tom Looman ActionComponent, [14] Lyra Modular Gameplay, [15] Lyra RPG component split.

---

### P4 — Enhanced Input

**InputConfig DataAsset** (theo [11] Aura):
- `IA_Move`, `IA_Look`, `IA_Crouch`, `IA_Lean_Left`, `IA_Lean_Right`, `IA_Sprint`, `IA_Aim`, `IA_Fire`, `IA_Reload`, `IA_Throw_Grenade`, `IA_Open_Inventory`, `IA_Open_Map`, `IA_Open_PalMenu`, `IA_Ping`, `IA_PalCommand`, `IA_Squadcommand`, `IA_Interact`, `IA_Tame`, `IA_RevivePal`.

**IMC riêng cho:** Raid, Hub, Briefing, Vehicle (nếu có sau).

**Pairing GameplayTag:** mỗi IA gắn `InputTag.*` → trigger GA tương ứng (theo [15]).

**Học từ:** [15] Lyra Input Config, [11] Aura InputConfigDataAsset.

---

### P5 — Animation

**PALDARK dùng:**
- Lyra Locomotion (Distance Matching + Orientation Warping).
- Pal AnimBP có Activity Driver — output `bIsAlert`, `bIsCowering`, `CurrentActivity` từ Activity FSM vào AnimBP.
- Combat Montage: shoot, reload, melee.
- Motion Warping cho door breach + extract climb.

**Học từ:** [15] Lyra RPG Locomotion, [10] MP Shooter recoil, [01] Tom Looman AnimNotify.

---

### P6 — Replication

**Replicate:**
- Player movement (Lyra default + custom Lean replicate).
- Pal Pawn (full replicate, attribute + activity).
- Combat — **server-side rewind** cho hitscan (từ [10]).
- Inventory state.
- Squad command broadcasted (RPC).
- VOIP (Vivox plugin).

**Authority model:**
- Server authoritative cho mọi thứ damage + loot.
- Client predict cho movement + UI feedback.

**Học từ:** [10] MP Shooter (lag comp), [07] MP Crash (lý thuyết), [13] Crunch (GAS replicate + dedicated).

---

### P7 — Dedicated Server

**PALDARK dùng:**
- AWS GameLift Anywhere Fleet (dev) → Production Fleet (beta).
- Server build target separate.
- Coordinator pattern: Lambda + DynamoDB queue + assign player → fleet.
- Hub town shard chạy trên server riêng (32 max, beta giới hạn 8).

**Học từ:** [08] AWS GameLift, [13] Crunch Coordinator pattern.

---

### P8 — GAS

**PALDARK dùng GAS cho:**
- **Player ability:** sprint, throw grenade, revive teammate, mark enemy. Note: shoot KHÔNG dùng GAS (quá tốn cho hitscan — Lyra cũng tách shoot riêng).
- **Pal ability:** mỗi loài Pal có 3 ability + 1 signature. Tất cả qua GA.
- **Pal/Player attributes:** Health, Stamina, MaxHealth, Bond (Pal-side), MoveSpeed, ArmorRating.
- **Damage chain:** GameplayEffect → Execution Calc (block, resistance, headshot mult) — port từ [11] Aura.

**ASC location:** trên Pawn (player + Pal). Owner Actor.

**Học từ:** [05] GAS Crash (basics), [11] Aura (damage chain + spells), [13] Crunch (GAS optimization net update freq).

---

### P9 — AI

**PALDARK dùng:**
- **Pal companion AI:** Activity FSM với 8 Activity (Idle/Follow/Scout/Cower/Combat/Investigate/Revive/SignatureMove).
- **Pal hostile AI:** Activity FSM với 12 Activity + Considerations (Utility).
- **Custom AISense:** `UPaldarkAISense_Scent` (smell), `UPaldarkAISense_Aura` (psychic).
- **ThreatOctree:** spatial query cho "threat trong khu vực".

**Hierarchy:**
- `UPaldarkBaseActivity` (base)
- → `UPaldarkPalFollowActivity`
- → `UPaldarkPalCombatActivity`
- → `UPaldarkPalCowerActivity`
- → ... (8 companion activities)
- → `UPaldarkEchoStalkActivity`
- → `UPaldarkEchoAmbushActivity`
- → ... (12 hostile activities)

**Học từ:** ReadyOrNot study (kien_truc 04-ReadyOrNot/Activities/), [01] Tom Looman BT base, [15] Lyra StateTree.

---

### P10 — UI

**PALDARK dùng:**
- CommonUI cho menu (Lyra-style).
- ActivatableWidget cho FrontEnd.
- Radial wheel (port từ RoN `SwatCommandWidget` — nhưng MVVM, không god-widget 5K LOC).
- Indicator Manager (Lyra RPG style) cho world-space marker.
- HUD raid tối giản.

**Radial wheel command:**
- 4 Pal command: Attack/Defend/Scout/Heel.
- 4 Squad command: Stack/Cover/Push/Retreat.

**Học từ:** [14] Lyra CommonUI, [15] Lyra RPG Indicator, RoN radial pattern, [11] Aura HUD MVVM.

---

### P11 — Inventory

**PALDARK dùng:**
- Fragment composition ([09] DruidMech):
  - `UFragment_Stackable`, `UFragment_Equipable`, `UFragment_Consumable`, `UFragment_Statful`, `UFragment_Weight`, `UFragment_PalCapture`.
- Composite pattern cho container (backpack, ammo box).
- Weight system.
- Stash unlimited (hub) — Inventory limited (raid).

**Học từ:** [09] Inventory full, [15] Lyra RPG Quick Bar.

---

### P12 — Data-driven

**PALDARK dùng PrimaryDataAsset cho:**
- `UPaldarkPalDefinition` — mỗi loài Pal (stat range, abilities, animation, sound, voice).
- `UPaldarkWeaponDefinition` — recoil pattern, damage curve, ammo type.
- `UPaldarkAbilityDefinition` — GA + tags + cost.
- `UPaldarkMapDefinition` — POI list, extract beacons, music.
- `UPaldarkLootTable` — drop weighted.
- Lyra Experience cho từng map (theo [14]+[15]).

**Học từ:** [01] Tom Looman DataAsset + AssetManager, [11] Aura Curve Table, [15] Lyra Experience.

---

### P13 — Save / Load

**PALDARK dùng:**
- Server-side persistence cho Pal roster + inventory + base layout (DynamoDB).
- Local SaveGame cho settings + key binding + last loadout.
- Raid result diff write-back to backend on extract.

**Học từ:** [01] Tom Looman SaveGame, [11] Aura checkpoint.

---

### P14 — Asset Manager + Async

**PALDARK dùng:**
- AssetManager scan PrimaryAssetType.
- Async load Pal asset khi raid load (10 loài Pal nặng).
- Game Feature Plugin cho mỗi map ([14]+[15] pattern).
- Soft reference cho UI icon, voice line.

**Học từ:** [01] Tom Looman async, [14] Lyra GameFeature.

---

### P15 — Performance

**PALDARK dùng:**
- SignificanceManager cho Pal & enemy ngoài 50m.
- Pal LOD cycle (high detail tick gần player, low ngoài 30m).
- ThreatOctree cho spatial query.
- Lyra Async asset prefetch.
- Actor Pooling cho bullet + drop item.
- Aggregate Ticking cho Pal AI 100+ instance.

**Học từ:** ReadyOrNot SignificanceManager, [01] Tom Looman Aggregate Tick + Object Pool, [14] Lyra Performance section.

---

### P16 — Math / Physics / Audio

**PALDARK dùng:**
- FMOD Studio + Audio Propagation Component (port RoN).
- AMRagdoll-style ragdoll on death.
- DynamicCoverSystem (nếu có license) hoặc tự gen cover.
- Vector math cho aim spread + ballistic.
- Quaternion smooth cho Pal head tracking.

**Học từ:** [12] Pro UE4 math chapter, [04] ReadyOrNot FMOD setup.

---

### P17 — Lyra Framework

**PALDARK = Lyra extension.** Toàn bộ codebase build trên Lyra:
- Lyra Experience cho raid + hub.
- ExperienceActionSet cho map-specific gameplay.
- GameFeature Plugin: `GFP_RaidPort`, `GFP_HubTown`, `GFP_PalSet_Beta`, `GFP_PvE_Mode`.
- Modular Gameplay via GameFrameworkComponentManager.
- Lyra Input Config.
- Lyra Equipment Manager.

**Học từ:** [14] Exploring Lyra full, [15] Lyra RPG application.

---

### P18 — Backend / Live Ops

**PALDARK dùng:**
- AWS Lambda + API Gateway + Cognito (auth + JWT).
- DynamoDB:
  - `players` table (player_id, profile, currency)
  - `pals` table (pal_id, owner_id, gen, level, bond)
  - `inventory` table (player_id, item_id, count)
  - `match_history` table (match_id, player_ids, result, telemetry)
- GameLift fleet (raid server) + GameLift hub server (32 instance, beta 8).
- mod.io for community maps post-beta.
- Telemetry dashboard (CloudWatch + Quicksight).

**Học từ:** [08] AWS GameLift full, [13] Crunch Server Coordinator.

---

## Gap List (cái nào KHÔNG có)

**PALDARK beta sẽ KHÔNG có:**

| Feature | Lý do cắt | Đẩy tới |
|---------|-----------|---------|
| Open world 64km² | Scope creep | v1.0 |
| 100-player MMO | Tech load lớn + AWS cost | v1.0 |
| PvP raid | Cần balance dài | v0.9 |
| Vehicle | Không phục vụ 3 pillar | v1.1 |
| Crafting tree đầy đủ | Cần economy designer | v0.9 |
| Mod.io tích hợp | Wait stable backend | v1.0 |
| Localization | English-only beta | v1.0 |
| Console release | PC first | v1.0 |

---

## Tổng kết coverage

| Pillar | Palworld | PUBG | RoN | **PALDARK Beta target** |
|--------|---------|------|-----|---------------------------|
| P1 | 60% | 75% | 90% | **100%** |
| P2 | 25% | 90% | 95% | **100%** |
| P3 | 30% | 85% | 60% | **95%** |
| P4 | 40% | 80% | 75% | **100%** |
| P5 | 20% | 60% | 85% | **90%** |
| P6 | 10% | 60% | 85% | **100%** |
| P7 | 5% | 40% | 50% | **100%** |
| P8 | 30% | 70% | 0% | **100%** |
| P9 | 5% | 10% | 100% | **100%** |
| P10 | 20% | 85% | 100% | **90%** |
| P11 | 25% | 80% | 85% | **95%** |
| P12 | 50% | 70% | 85% | **100%** |
| P13 | 0% | 0% | 80% | **90%** |
| P14 | 10% | 30% | 70% | **100%** |
| P15 | 5% | 30% | 95% | **100%** |
| P16 | 30% | 60% | 90% | **90%** |
| P17 | 0% | 0% | 0% | **100%** (backbone) |
| P18 | 0% | 20% | 70% | **100%** |
| **AVG** | **20%** | **53%** | **73%** | **~97%** |

PALDARK là **target** cao nhất. Đạt được = đã đi qua hết 15 nguồn.

---

## Tiếp theo

- [03-Roadmap_1_Year.md](03-Roadmap_1_Year.md) — 52 tuần
- [04-Resource_Map.md](04-Resource_Map.md) — pillar × source
