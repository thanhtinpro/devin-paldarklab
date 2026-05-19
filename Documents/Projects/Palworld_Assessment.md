# Palworld (KYWorld/PalworldProject) — Đánh giá theo UE5 Core Pillars

> Source: `02.Palworld/Source/Palworld_Base/` (70 file .h/.cpp).
> Spec: 67-page Korean PDF, bản dịch markdown track ở PR #1 (`Documents/PalWorld_Development_Specifications.md`, chưa merge vào `main`).
> Kế hoạch refactor track ở PR #2 (`Documents/Refactor_Plan.md`, chưa merge vào `main`).
>
> Đánh giá apply [Donchitos framework](../Donchitos_GameStudios_Framework.md) (game pillars + systems-index + ADR) và [UE5 Core Pillars](../UE5_Core_Pillars.md).

---

## 1. Concept (Donchitos `/brainstorm` output dự kiến)

| Khía cạnh | Spec PDF | Hiện trạng code |
|-----------|----------|-----------------|
| **Game type** | Open-world creature-collection + survival (Palworld clone) | Khung trống — chỉ có Character/Pal/Player + GAS skeleton |
| **Target platform** | PC (Steam) | UE5, dedicated server chưa setup |
| **Multiplayer scope** | Co-op 4–32 | Mọi class đều chưa có replication logic, chỉ `bReplicates` cờ |
| **Spec progress** | 90% (PDF có đủ widget/UI/level flow/inventory/customization) | ~5–10% (code skeleton, hầu hết class rỗng) |

### Game Pillars (đề xuất — chưa có trong spec)

1. **Capture + Work** — Pal vừa chiến đấu vừa lao động sản xuất.
2. **Survival cycle** — Đói/khát/lạnh, base management, sleep.
3. **Co-op friendly** — Mọi cơ chế chơi được multiplayer 4–8 người (target dev), 32 (target prod).

> Cần `/architecture-decision` để chốt pillar. Hiện không có doc nào ghi explicit.

---

## 2. Systems Map (Donchitos `/map-systems`)

| Hệ thống | Layer | Status | Priority | Bằng chứng code |
|----------|-------|--------|----------|-----------------|
| Core Framework (GM/GS/PC/PS/Pawn) | Core | 🟡 Drafted | P0 | `BaseGameMode`, `BasePlayerController`, `BaseCharacter` — class rỗng |
| Enhanced Input | Core | 🟡 Drafted | P0 | `BaseInputComponent`, `DataAsset_InputConfig` |
| GAS (ASC, AS) | Core | 🟡 Drafted | P0 | `BaseAbilitySystemComponent`, `BaseAttributeSet` — rỗng |
| Character (Player/Pal) | Entity | 🟡 Drafted | P0 | `PlayerCharacter`, `PalCharacterBase` — rỗng |
| Animation | Entity | 🟡 Drafted | P1 | `BaseAnimInstance`, `PlayerAnimInstance` |
| Items (Equipment/Resource) | Entity | 🟡 Drafted | P1 | `ItemBase`, `WeaponBase`, `ArmorBase`, `ConsumeItemBase`, `MaterialItemBase`, `ResourceItemBase` |
| Inventory Component | System | 🔴 Empty | P0 | `InventoryComponentBase.cpp` rỗng |
| Pawn Extension / Equipment Comp | System | 🟡 Drafted | P0 | `PawnExtensionComponent`, `PawnEquipmentComponent` |
| AI (Pal behavior) | System | 🔴 Missing | P1 | `BaseAIController` rỗng, không có BT/EQS/StateTree |
| Save / Load | System | 🔴 Missing | P1 | Không có lớp SaveGame |
| UI / HUD (Inventory, MiniMap, HP) | Presentation | 🟡 Drafted | P0 | `WidgetBase`, `MiniMapWidget` chỉ có khung |
| Survival Cycle (Hunger/Thirst/Sleep) | System | 🔴 Missing | P0 | Không có attribute hay tick logic |
| Base Building | System | 🔴 Missing | P1 | Spec có nhưng code không có |
| Pal Capture (Sphere) | System | 🔴 Missing | P0 | Spec có Pal sphere; code không có actor |
| Customization | System | 🔴 Missing | P2 | Spec có; code không có |
| Multiplayer / Dedicated Server | Infra | 🔴 Missing | P0 | Chỉ có `bReplicates`, không có session/EOS/Steam |

Layer key: **Core → Entity → System → Infra → Presentation**.

---

## 3. Đánh giá theo Pillars (UE5_Core_Pillars)

Quy tắc tính %: `(weight_of_actual_implementation / weight_of_target_for_genre) × 100%`. Target = mức **production-ready cho creature-collection survival co-op**.

| Pillar | % | Trạng thái | Bằng chứng | Còn thiếu |
|--------|---|-----------|------------|-----------|
| **P1 C++/Build** | 60% | 🟡 | `Palworld_Base.Build.cs` có module GAS + EnhancedInput, namespace tổ chức tốt | Thiếu module Online Subsystem, AI, GameplayTask, ModelViewViewModel |
| **P2 Core Framework** | 25% | 🔴 | `BaseGameMode`, `BasePlayerController`, `BaseCharacter` tồn tại | Toàn bộ rỗng. Thiếu `BaseGameState`, `BasePlayerState`, `BaseGameInstance`, `BaseHUD` |
| **P3 Composition** | 30% | 🟡 | Có `PawnExtensionComponent`, `PawnEquipmentComponent`, `InventoryComponentBase` | Component rỗng. Thiếu HealthComponent, HungerComponent, SurvivalComponent |
| **P4 Enhanced Input** | 40% | 🟡 | `BaseInputComponent` + `DataAsset_InputConfig` có | Không có IA_/IMC_ actual, không có Lyra-style input action bind via GameplayTag |
| **P5 Animation** | 20% | 🔴 | `BaseAnimInstance`, `PlayerAnimInstance` tồn tại | Rỗng, không có ABP graph, không có motion warping, không có montage cho ability |
| **P6 Replication** | 10% | 🔴 | Một số class có `bReplicates` | Không có `DOREPLIFETIME`, không RPC, không RepNotify |
| **P7 Dedicated Server** | 5% | 🔴 | Module có | Chưa setup Server target, chưa OnlineSubsystem |
| **P8 GAS** | 30% | 🟡 | `BaseAbilitySystemComponent`, `BaseAttributeSet`, `BaseGameplayAbility`, `PalGameplayAbility`, `PlayerGameplayAbility`, `BaseGameplayTag` | Tất cả rỗng. Thiếu GE classes, AbilityTask, native tag registration |
| **P9 AI** | 5% | 🔴 | `BaseAIController` tồn tại | Rỗng. Không BT/Blackboard/EQS/StateTree. Spec yêu cầu Pal AI phức tạp (work + combat + idle) |
| **P10 UI** | 20% | 🔴 | `WidgetBase`, `MiniMapWidget` tồn tại | Khung trống. Spec có 10+ widget (Inventory, Pal manage, Crafting, Base, Quest…) |
| **P11 Inventory** | 25% | 🟡 | `ItemBase`, sub-class items, `InventoryComponentBase` | Component rỗng, không có Item Fragment, không có Pickup actor, không có drop/move/stack |
| **P12 Data-driven** | 50% | 🟡 | `DataAsset_StartupPlayer/Base/Pal`, `DataAsset_InputConfig`, `BaseStructType` | Thiếu DataTable item/Pal stat, thiếu CurveTable level, thiếu PrimaryAssetId setup |
| **P13 Save/Load** | 0% | 🔴 | Không có | Cần SaveGame archive cho world state + player + Pal |
| **P14 AssetMgr/Async** | 10% | 🔴 | `DataAsset_*` có | Không có AssetManager subclass, không async load |
| **P15 Performance** | 5% | 🔴 | — | Cần Significance Manager (open world), pooling cho Pal, network culling |
| **P16 Math/Physics** | 30% | 🟡 | UE math built-in | Vehicle (mount Pal), swimming, climbing chưa có |
| **P17 Lyra** | 0% | 🔴 | Không dùng Lyra | (không bắt buộc) |
| **P18 Backend / Live Ops** | 0% | 🔴 | — | Cần EOS/Steam, session, dedicated server, save persist |

**Tổng % (weighted average):**

| Group | % | Weight | Score |
|-------|---|--------|-------|
| Foundation (P1, P2, P3, P4) | 39% | 25 | 9.7 |
| Combat (P5, P6, P8, P9) | 16% | 25 | 4.0 |
| Content (P10, P11, P12, P13) | 24% | 20 | 4.8 |
| Production (P14, P15, P7, P18) | 5% | 20 | 1.0 |
| Specialty (P16, P17) | 15% | 10 | 1.5 |
| **TOTAL** | | 100 | **~21%** |

> **Palworld base ~21% coverage so với target.**

---

## 4. Đã làm được gì (positive)

1. **Layout C++ tốt:** namespace `Source/Palworld_Base/Public,Private/<Domain>` tách rõ (AbilitySystem, Character, Component, Controller, DataAsset, GameMode, Item, Widget, BaseLibrary).
2. **Decision dùng GAS từ đầu:** Có `BaseAbilitySystemComponent` + `BaseAttributeSet` từ ngày 1 — đúng cho game đa người chơi có ability phức tạp.
3. **Component composition đúng hướng:** `PawnExtensionComponent` + `PawnEquipmentComponent` + `InventoryComponentBase` đi đúng pattern Lyra-flavored.
4. **DataAsset cho startup:** `DataAsset_StartupPlayer/Base/Pal` cho data-driven init — tốt cho mod-ability.
5. **Item hierarchy có sẵn:** `ItemBase` → `EquipmentBase` → `WeaponBase`/`ArmorBase` và `ItemBase` → resource → `ConsumeItemBase`/`MaterialItemBase`/`ResourceItemBase`.

---

## 5. Cần phát triển thêm (Gap List — ưu tiên P0 → P2)

### P0 (Blocker — không có thì không chạy)
- Triển khai logic `BaseGameMode`, `BasePlayerController`, `BaseCharacter` (movement, replication baseline).
- Triển khai `BaseAbilitySystemComponent` + `BaseAttributeSet` (Health, Stamina, Hunger, Thirst, SAN, ColdResist, Weight).
- Triển khai `InventoryComponentBase` (FastArray, slot, stack, drop).
- Setup OnlineSubsystem Steam/EOS + session create/join.
- Survival tick logic (Hunger/Thirst/Stamina degrade theo time + activity).
- Pal capture system (Sphere actor + capture ability + Pal pool subsystem).
- Save/Load: `USaveGame` cho player + base + Pal team.

### P1 (Core gameplay)
- Pal AI (BehaviorTree hoặc StateTree): Combat / Work / Idle / Follow / Order.
- Base building system: snap point, foundation, wall, crafting station.
- HUD chính: Health/Stamina/Hunger bar, MiniMap, Pal team panel, Inventory grid.
- Crafting recipe (DataTable + UI).
- Pal ride / mount movement (special MovementComponent).
- Day/Night cycle + weather subsystem.
- Sleep system (skip time + restore stat).
- Equipment slot logic (weapon slot 1-4, armor head/body/leg, accessory).

### P2 (Polish)
- Customization UI (face, hair, body).
- Pal evolution / breeding.
- Quest / mission system.
- Achievements.
- Cosmetic store hooks.

---

## 6. Cấu trúc — đáp ứng không? Sai phần nào?

### Đáp ứng tốt

| Pattern | Đánh giá |
|---------|----------|
| Module Build.cs | ✅ Có Public/Private split |
| Namespace per domain | ✅ Tốt |
| GAS layer tách | ✅ Có `AbilitySystem/Ability/` subfolder |
| DataAsset startup | ✅ Có pattern phù hợp Lyra-flavored |

### Sai / thiếu cấu trúc

| Vấn đề | Hậu quả | Khuyến nghị |
|--------|---------|-------------|
| Không có `Source/Palworld_BaseTests/` | Không thể TDD | Thêm test module theo Tom Looman pattern |
| Không có `Plugins/` riêng cho inventory / AI | Dependency chéo | Sau khi system stable → tách thành plugin (như [09] dạy) |
| Không có `Subsystems/` | Manager nằm đâu cũng được, dễ chaos | Thêm folder cho GameInstance/World/Local subsystem |
| Không có folder cho `GameplayCue` | Cue effect sẽ tản mác | Cần `AbilitySystem/Cues/` |
| Không có folder cho `ExecutionCalculation` / `MMC` | Damage chain phức tạp khó scale | Cần `AbilitySystem/Calculations/` (xem [11] Aura) |
| `BaseGameplayTag.cpp` để native tag register — nhưng chưa register tag | Native tag là cốt lõi GAS-driven design | Phải khởi tạo tag trong `StartupModule` |
| Không có `Localization` | Game đa thị trường khó | Cần `Localization/` từ sớm |
| Không có `Performance/` subfolder | Open world không Significance sẽ lag | Thêm SignificanceManager + Actor Pool (như [01] Tom) |

---

## 7. Ánh xạ tới khóa học (học gì để fill gap)

| Gap | Course nên học |
|-----|----------------|
| GAS full implementation | [05] + [11] (Aura cho damage chain, [13] cho multiplayer GAS) |
| Inventory thật | [09] (Item Fragment + Composite) |
| Save/Load | [01] Tom (archive proxy) + [11] (checkpoint + map entrance) |
| Multiplayer baseline | [07] (Class framework + RepCondition + RPC) |
| Lag compensation (nếu PvP) | [10] |
| Dedicated server + AWS | [08] + [13] |
| AI Pal | [01] Tom (BT + EQS) + [15] (State Tree) + ReadyOrNot Activity AI pattern |
| Customization | Lyra Modular Gameplay ([14]) |
| Base building | Không có course — cần research riêng |

---

## 8. ADR cần ghi (Architecture Decision Records)

| ADR | Tình trạng |
|-----|-----------|
| ADR-01 GAS adoption | Implicit (code có) — cần document |
| ADR-02 Online subsystem (Steam vs EOS) | Chưa quyết |
| ADR-03 Save format (binary archive vs JSON) | Chưa quyết |
| ADR-04 Pal AI (BT vs State Tree) | Chưa quyết |
| ADR-05 Network model (listen vs dedicated) | Chưa quyết — spec ngầm dedicated |
| ADR-06 Base building (Lyra Game Feature vs custom) | Chưa quyết |

---

## 9. Lộ trình refactor (đã có riêng)

Xem `Documents/Refactor_Plan.md` (track ở PR #2) — Phase 0 → Phase 10, F-01 → F-10 small fixes. Plan đó ưu tiên fill các pillar P0 trước (Core, GAS, Inventory).

---

## 10. Kết luận

- **Palworld base hiện tại = 21% coverage so với target genre.** Là "skeleton + spec PDF chi tiết" — phải gấp 5x công sức để ship.
- **Cấu trúc layout đúng hướng** (Lyra-flavored composition), không cần phá đi xây lại.
- **Lỗ hổng lớn nhất:** P6 Replication, P7 Dedicated Server, P9 AI, P13 Save — vì spec là **co-op survival 4-32 người**, không có 4 pillar này thì không ship được.
- **Khuyến nghị tiếp theo:** chạy `Refactor_Plan.md` Phase 0 trên 1 PR riêng (track ở PR #2, chưa merge — em chưa đụng code).
