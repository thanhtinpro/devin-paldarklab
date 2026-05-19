# P03 — Composition over Inheritance

> Tự soạn từ: [01] ActionRoguelike, [03] PUBG, [04] ReadyOrNot, [09] Inventory, [14] Lyra, [15] Lyra RPG.
>
> Pillar quyết "code logic đặt ở Actor hay Component hay Subsystem". Vi phạm → god-character 368KB như RoN.

---

## Pillar Identity

**Cái gì:** Phân chia logic vào Actor/Component/Subsystem đúng nguyên tắc — composition over inheritance.

**Scope:**
- ✅ Khi nào tạo Component vs khi nào để logic trong Actor.
- ✅ Subsystem cho cross-actor logic.
- ✅ Pattern: `Pawn + N component`, mỗi component own responsibility.
- ✅ Modular Gameplay Plugin (AddComponents/AddAbilities/AddSpawners) cho injection runtime.

**Out of scope:**
- ❌ Inventory Component cụ thể (P11).
- ❌ ASC component (P08).
- ❌ Replication theory (P06).

---

## PALDARK cần gì từ pillar này?

| Component | Đặt ở | Trách nhiệm |
|-----------|-------|-------------|
| `UPaldarkPalCompanionComponent` | Player Pawn | Tracking bonded Pal + command issue |
| `UPaldarkExtractionComponent` | Player Pawn | Track extract eligibility (alive, inventory, distance) |
| `UPaldarkSanityComponent` | Player Pawn | Sanity stat + decay logic |
| `UPaldarkScentTrailComponent` | Player Pawn + Pal | Emit scent for AI sense |
| `UPaldarkPalActivityComponent` | Pal | Activity FSM (Idle/Follow/Combat/Forage) |
| `UPaldarkPalAttunementComponent` | Pal | Owner relationship + trust level |
| `UPaldarkPalAbilityComponent` | Pal | Wraps ASC for ability cast |

**Mục tiêu:** Pawn class ≤ 1KB code (chỉ wiring), 90% logic ở component.

---

## Sources Matrix

| Source | Coverage | Lesson lấy từ đó |
|--------|----------|------------------|
| [01] ActionRoguelike | 🟢 | L1 AttributeComponent + ActionComponent (separate concern) |
| [03] PUBG | 🟢 | L2 6-Component Pawn (gold standard mid-size) |
| [04] RoN | 🔴 | L3 Anti-pattern god-character 368KB |
| [09] Inventory | 🟢 | L4 Inventory + Equipment component (per-actor) |
| [14] Lyra | 🟢 | L5 Modular Gameplay AddComponents/AddAbilities |
| [15] Lyra RPG | 🟡 | L6 Quick Bar component application |

---

## Prerequisite

- P01 (UCLASS, UPROPERTY).
- P02 (biết Actor lifetime).

---

## Lessons

### L1 — AttributeComponent + ActionComponent pattern (Tom Looman) 🧪

**Goal:** Tách stat (Health/Stamina) khỏi action (Sprint/Dash/Cast) — 2 component riêng.

**Concept:**
- `UAttributeComponent` — sở hữu Health, MaxHealth, OnHealthChanged delegate.
- `UActionComponent` — array `UAction*`, có `StartAction(Name)` / `StopAction(Name)`.
- Pawn chỉ wire: `AttributeComp = CreateDefaultSubobject<UAttributeComponent>(...)`.
- Pawn không biết Health số bao nhiêu — hỏi `AttributeComp->GetHealth()`.

**Source learning path:**
- [01] ActionRoguelike — `SAttributeComponent.h/cpp`, `SActionComponent.h/cpp`.
- Cross-ref: `Documents/GameDesign/01-ActionRoguelike_GDD.md` § Phase 5 ADR-1.

**API / Class chính:**
- `CreateDefaultSubobject<T>(FName)` — trong Pawn constructor.
- Delegate: `DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams`.

**Practice exercise (3 hour):**
1. Tạo `UPaldarkLabAttributeComponent` với Health/Stamina/OnDeath delegate.
2. Tạo `APaldarkLabPawn` add component qua `CreateDefaultSubobject`.
3. Override `BeginPlay` không biết Health — chỉ subscribe `OnHealthChanged`.

**PALDARK take-away:**
- 🟢 Pattern này dùng cho **mọi stat** trong PALDARK (Pal có HealthComp, SanityComp, AttunementComp).
- 🟡 Adapt: dùng GAS `UAttributeSet` thay AttributeComponent (P08).

**Apply ở week:** Roadmap week 3.

---

### L2 — 6-Component Pawn pattern (PUBG gold standard) 🧪

**Goal:** Cấu trúc Pawn với 6 component, mỗi component 1 trách nhiệm.

**Concept:**
PUBG-KI `APUBGCharacter`:
- `UEquippedComponent` — weapon equipped.
- `UInputComponent` (custom wrap) — input bindings.
- `UInventoryComponent` — backpack.
- `UItemDataComponent` — item cache.
- `UMovementComponent` (extend) — sprint/crouch.
- `UNearAreaComponent` — pickup query trace.

**Source learning path:**
- [03] PUBG — `APUBGCharacter` constructor.
- Cross-ref: `Documents/GameDesign/03-PUBG_GDD.md` § Phase 2.

**Practice exercise:**
1. Tạo `APaldarkLabCharacter` thêm 6 component empty.
2. Constructor wire bằng `CreateDefaultSubobject`.
3. Log mỗi `BeginPlay` component xác nhận tồn tại.

**PALDARK take-away:**
- 🟢 Player Pawn PALDARK = `Character + 7 component` (xem bảng "PALDARK cần gì").
- 🟢 Pal Pawn PALDARK = `Character + 5 component` (ActivityComp, AttunementComp, AbilityComp, ScentComp, AttributeComp).

**Apply ở week:** Roadmap week 5 (Pal Pawn) + week 7 (Player Pawn).

---

### L3 — Anti-pattern: God-Character (ReadyOrNot) 🔴

**Goal:** BIẾT để TRÁNH — RoN dùng 66 component nhưng Character vẫn 368KB.

**Concept:**
- 66 Component đúng nguyên tắc, nhưng:
  - `APlayerCharacter` 368KB chứa logic không nên ở Character.
  - Logic về AI threat detection nằm trong Character thay vì ThreatAwarenessComp.
  - Logic về inventory swap nằm trong Character thay vì InventoryComp.
- Component "giả tạo" — tạo ra nhưng logic vẫn ở Character.

**Source learning path:**
- [04] RoN GDD § Phase 5 ADR-7 (god-character anti-pattern).

**Cách tránh:**
- Quy tắc: nếu Character class > 50KB code → review, có khả năng cao là logic sai chỗ.
- Mỗi method trong Character chỉ nên là wiring (forward call đến component).
- Logic thật phải ở component/subsystem.

**PALDARK take-away:**
- 🟢 `APaldarkCharacter` mục tiêu < 50KB code (target 30KB).
- 🟢 `APaldarkPalCharacter` mục tiêu < 30KB code.
- 🔴 Nếu thấy method `APaldarkCharacter::FireWeapon(...)` chứa 200 LOC — sai. Phải gọi `WeaponComp->Fire()`.

**Apply ở week:** Mỗi review (week 4, 8, 12, ...).

---

### L4 — Inventory + Equipment component (per-actor scope) 🧪

**Goal:** Inventory thuộc về actor cụ thể, không phải subsystem global.

**Concept:**
- `UInventoryComponent` trên Pawn → mỗi pawn có inventory riêng.
- `UEquipmentComponent` trên Pawn → mỗi pawn có slot equip riêng.
- KHÔNG dùng `UInventorySubsystem` global — vì inventory thuộc về owner.

**Source learning path:**
- [09] Inventory § Chapter 3-9.
- Cross-ref: `Documents/GameDesign/09-Inventory_GDD.md` § Phase 5 ADR-2.

**Practice exercise:**
1. Tạo `UPaldarkLabInventoryComponent` với array `TArray<FItemSlot>`.
2. Tạo `APaldarkLabPickup` actor — OnOverlap → forward `Player->InventoryComp->Add(...)`.
3. PIE: nhặt 5 item → check inventory.

**PALDARK take-away:**
- 🟢 Player + Pal đều có `UPaldarkInventoryComponent`.
- 🟢 Pal carry capacity = 30% Player (force trade-off).

**Apply ở week:** Roadmap week 19 + 31.

---

### L5 — Modular Gameplay Plugin (Lyra runtime injection) 📖🧪

**Goal:** Inject Component/Ability runtime qua plugin, không hardcode ở constructor.

**Concept:**
- `UGameFeatureAction_AddComponents` — runtime add component nếu actor có tag.
- `UGameFeatureAction_AddAbilities` — runtime grant GA nếu actor có ASC.
- `UGameFeatureAction_AddInputBinding` — runtime wire input.
- Plugin activate → trigger AddComponent → tất cả actor matching nhận component mới.

**Source learning path:**
- [14] Exploring Lyra § Chapter Modular Gameplay.
- Cross-ref: `Documents/GameDesign/14-Exploring_Lyra_GDD.md` § Phase 5 ADR-3.

**API / Class chính:**
- `UGameFeatureAction` base.
- `UGameFrameworkComponentManager::AddReceiver(Actor, ExtensionEvent)`.
- `UPawnExtensionComponent` — receive ability set runtime.

**Practice exercise (4 hour):**
1. Tạo plugin `PaldarkPalPack1` (Game Feature Plugin).
2. Plugin có `GA_PalFire` GameplayAbility.
3. `UGameFeatureAction_AddAbilities` grant cho actor có tag `Pal.Type.Fire`.
4. Activate plugin runtime → Pal cast được fire ability.

**PALDARK take-away:**
- 🟢 Pal Pack 1/2/3 (Q3-Q4 release) = Game Feature Plugin.
- 🟢 NPC enemy variant = AddComponent + AddAbility qua plugin.

**Apply ở week:** Roadmap week 33+ (Lyra adopt).

---

### L6 — Lyra Pawn Data + Quick Bar (case study) 🧪

**Goal:** Apply Lyra extension cho RPG.

**Concept:**
- `ULyraPawnData` DataAsset — mesh + ability set + input config.
- `ULyraQuickBarComponent` — inventory hotbar slot.
- `ULyraEquipmentManagerComponent` — equipped item handle.

**Source learning path:**
- [15] Lyra RPG § Chapter Quick Bar.
- Cross-ref: `Documents/GameDesign/15-Lyra_RPG_GDD.md` § Phase 6.

**Practice exercise:**
1. Inherit `ULyraQuickBarComponent` → `UPaldarkQuickBarComponent`.
2. Add 6 slot (number key 1-6).
3. PIE: pickup item → slot 1 → press 1 to equip.

**PALDARK take-away:**
- 🟢 PALDARK adopt Quick Bar pattern cho hotkey Pal command (1=Follow, 2=Attack, 3=Forage, 4=Wait).

**Apply ở week:** Roadmap week 33+ (Lyra adopt).

---

## ⚡ Capstone Exercise

**Goal:** Build PaldarkLab Pawn với 7 component đúng nguyên tắc.

**Yêu cầu:**
1. Tạo `APaldarkLabPlayer` có 7 component:
   - `UPaldarkLabAttributeComp` (Health/Stamina/Sanity).
   - `UPaldarkLabActionComp` (cast ability).
   - `UPaldarkLabInventoryComp` (backpack).
   - `UPaldarkLabEquipmentComp` (slot).
   - `UPaldarkLabPalCompanionComp` (bonded Pal).
   - `UPaldarkLabScentComp` (emit scent).
   - `UPaldarkLabExtractionComp` (eligibility).
2. `APaldarkLabPlayer.h/cpp` ≤ 200 LOC (chỉ wiring).
3. Mỗi component có riêng `.h/cpp`, OnComponentBeginPlay log.

**Acceptance:** 7 component register; Pawn class < 5KB; log đầy đủ.

**Effort:** 3 ngày.

---

## Khi apply vào PALDARK roadmap

| Week | Pillar P03 usage |
|------|------------------|
| W3 | 9-class + Component skeleton |
| W5 | Pal Pawn với 5 component |
| W7 | Player Pawn với 7 component |
| W19 | Inventory + Equipment comp |
| W33 | Modular Gameplay adopt (Lyra) |
| W43 | Pal Pack 1 release (Game Feature Plugin) |

---

## Anti-patterns (cảnh báo)

- 🔴 God-Character > 50KB (RoN style).
- 🔴 Component "giả tạo" — tạo component nhưng logic vẫn ở Pawn.
- 🔴 Inventory ở Subsystem global thay vì Component per-pawn.
- 🔴 Hardcoded ability ở Pawn constructor thay vì runtime grant qua Pawn Data.
- 🔴 Inherit deep (5+ level) thay vì compose.

---

## Tham chiếu

- [`Documents/UE5_Core_Pillars.md`](../UE5_Core_Pillars.md) § P3.
- [`Documents/GameDesign/01-ActionRoguelike_GDD.md`](../GameDesign/01-ActionRoguelike_GDD.md) — Component pattern.
- [`Documents/GameDesign/03-PUBG_GDD.md`](../GameDesign/03-PUBG_GDD.md) — 6-Component gold standard.
- [`Documents/GameDesign/04-ReadyOrNot_GDD.md`](../GameDesign/04-ReadyOrNot_GDD.md) — Anti-pattern.
- [`Documents/GameDesign/14-Exploring_Lyra_GDD.md`](../GameDesign/14-Exploring_Lyra_GDD.md) — Modular.
- [`Documents/PALDARK/01-GameDesign.md`](../PALDARK/01-GameDesign.md) — Component plan.
