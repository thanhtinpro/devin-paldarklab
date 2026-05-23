# 09 — Inventory Systems (Donchitos Reverse GDD)

> Source: [github.com/DruidMech/InventorySystems](https://github.com/DruidMech/InventorySystems) — Stephen Ulibarri, [Udemy](https://www.udemy.com/course/unreal-engine-5-inventory-systems/).
>
> Sample game: small RPG với inventory rich — fragment, composite, stack, drag-drop, equipment.

---

## Identity Card

| Field | Value |
|-------|-------|
| 📦 Type | Course (deep dive 16 chương) |
| Genre | RPG inventory showcase |
| Engine | UE5 |
| Đóng góp PALDARK | **Inventory production-grade — Fragment + Composite + Equipment slot** |

---

## Phase 1 — Brainstorm (reverse)

### Implicit pitch
> *"Inventory không phải just data — là gameplay. Pickup, hover, drag, drop, equip, stack — mỗi tương tác tạo cảm xúc 'có chứa'."*

### Cảm xúc cốt lõi
- **Có chứa** — nhặt item, có cảm giác sở hữu.
- **Tổ chức** — drag-drop satisfying.
- **Identity** — equipment slot = nhân vật của bạn.

---

## Phase 2 — Decompose

### Verb cốt lõi
- Pickup
- Hover (tooltip)
- Drag-drop
- Stack
- Equip
- Drop
- Use

### System hierarchy

```
┌──────────────────────────────────┐
│ AInventoryCharacter              │
└──────────┬───────────────────────┘
           │
   ┌───────┴────────┬────────────────┐
   ▼                ▼                ▼
InventoryComp   EquipmentComp   InteractComp
(slot grid)     (slot dict)     (hover/use)
   │                │
   └────────────────┴─── ItemFragment (composable)
                          ├─ ItemStackFragment
                          ├─ ItemEquipFragment
                          ├─ ItemConsumeFragment
                          └─ ItemContainerFragment (composite)
```

---

## Phase 3 — Define (GDD reverse)

### Game Loop (sample)
1. Char đi đến item world.
2. Hover → tooltip widget.
3. Press E → pickup → vào inventory slot.
4. Open inventory UI → drag-drop.
5. Equip vào slot.
6. Drop item → spawn back world.

### Player Character
- TPS với UI inventory toggle.

### Inventory Component
- Slot 2D grid (X × Y) hoặc list.
- Add/Remove/Move item.
- Stack same-type up to MaxStackSize.

### Item Definition (DataAsset)
- Base `UInventoryItemDefinition`.
- Fragments composable:
  - `UItemFragment_Stack` (stack count + max).
  - `UItemFragment_Equip` (slot type + stats).
  - `UItemFragment_Consume` (effect on use).
  - `UItemFragment_Container` (inventory bên trong = bag in bag).

### Equipment Component
- Dict slot → item instance.
- Stat aggregate khi equip.

### UI
- Grid widget.
- Tooltip widget on hover.
- Drag-drop operation.

---

## Phase 4 — 3 Game Pillars (inferred)

### Pillar 1 — Items are Characters
> *"Mỗi item có ID, name, icon, description. Không phải số."*

### Pillar 2 — Stack Grows World
> *"Stack = depth. Stack 99 wood = thực sự có nhiều wood."*

### Pillar 3 — Equipment Shapes Player
> *"Equip = thay đổi visual + stat. Bạn là gì bạn equip."*

---

## Phase 5 — Architecture Decisions (implicit)

### ADR-1: Item Fragment composition
- **Quyết định:** Item Definition = base class + fragments (như Lyra).
- **Lý do:** Reusable, composable, không inheritance hell.
- **PALDARK adopt:** 🟢 **CORE** — copy pattern.

### ADR-2: Inventory Component thay vì Subsystem
- **Quyết định:** Mỗi Pawn có Inventory Component.
- **Lý do:** Per-actor inventory natural.
- **PALDARK adopt:** 🟢 — Player + Pal đều có Inventory Component.

### ADR-3: PrimaryDataAsset cho Item Definition
- **Quyết định:** Item là `UPrimaryDataAsset` async loadable.
- **PALDARK adopt:** 🟢.

### ADR-4: Drag-drop với UMG operation
- **Quyết định:** UDragDropOperation subclass.
- **PALDARK adopt:** 🟢.

### ADR-5: Composite (bag-in-bag)
- **Quyết định:** Item có thể là Container chứa items khác.
- **PALDARK adopt:** 🟡 Adapt — PALDARK chỉ 1 level (backpack, không bag-in-bag).

---

## Phase 6 — System Map

| System | Class | File |
|--------|-------|------|
| Item Def | `UInventoryItemDefinition` | `Items/InventoryItemDefinition.h/cpp` |
| Fragment | `UItemFragment_*` | `Items/Fragments/` |
| Inv Comp | `UInventoryComponent` | `Inventory/InventoryComponent.h/cpp` |
| Equip Comp | `UEquipmentComponent` | `Inventory/EquipmentComponent.h/cpp` |
| Inv UI | `UInventoryWidget` | `UI/InventoryWidget.h/cpp` |
| Tooltip | `UItemTooltipWidget` | `UI/...` |

---

## Phase 7 — Stories taught (16 chương)

| Chapter | US |
|---------|----|
| 1 | Là dev, tôi setup project + module |
| 2 | Là dev, tôi tạo Item Definition base + fragments |
| 3 | Là dev, tôi viết Inventory Component (add/remove/move) |
| 4 | Là dev, tôi stack same-type item |
| 5 | Là dev, tôi spawn Pickup actor từ Item Def |
| 6 | Là dev, tôi hover tooltip widget |
| 7 | Là dev, tôi viết Inventory UI grid |
| 8 | Là dev, tôi drag-drop từ slot → slot |
| 9 | Là dev, tôi Equipment Component slot dict |
| 10 | Là dev, tôi Equip → aggregate stat |
| 11 | Là dev, tôi Composite container (bag in bag) |
| 12 | Là dev, tôi Use Consumable item |
| 13 | Là dev, tôi Drop item → spawn back world |
| 14 | Là dev, tôi setup ItemInstance vs ItemDefinition (instance has runtime data) |
| 15 | Là dev, tôi replicate inventory cho multiplayer |
| 16 | Là dev, tôi serialize inventory → SaveGame |

---

## Take-aways cho PALDARK

### 🟢 Copy nguyên (CORE — fill Palworld InventoryComponentBase skeleton)
- **Item Fragment pattern**.
- **Inventory Component per-Pawn**.
- **Equipment Component slot dict**.
- **PrimaryDataAsset Item Definition**.
- **Drag-drop UMG operation**.

### 🟡 Adapt
- **Composite container** → simplify chỉ 1 level cho PALDARK.

---

## Pillar Mapping

| Pillar | Đóng góp |
|--------|----------|
| P11 | 🟢 **Inventory production-grade** |
| P12 | 🟢 **DataAsset + Fragment** |
| P10 | 🟢 **UMG drag-drop pattern** |
| P3 | 🟠 Component composition cho inventory |

---

## Tham chiếu

- [Courses/09-Udemy-ue5-inventory-systems.md](../Courses/09-Udemy-ue5-inventory-systems.md)
- [02-Palworld_GDD.md](02-Palworld_GDD.md) — Palworld inventory skeleton sẽ fill bằng pattern này.
- [PALDARK/03-Roadmap_1_Year.md](../PALDARK/03-Roadmap_1_Year.md) — tuần 19, 30-31 áp pattern.
