# P11 — Inventory & Equipment

> Tự soạn từ: [09] Inventory, [11] Aura, [13] Crunch, [15] Lyra RPG, [03] PUBG.
>
> Inventory = extraction game CORE. PALDARK = extraction → mọi loot decision quan trọng → inventory chuẩn.

---

## Pillar Identity

**Cái gì:** Lưu/load item, stack, drag-drop, equipment slot, persistence, weight, item fragment.

**Scope:**
- ✅ Item Definition (PrimaryDataAsset).
- ✅ Item Fragment pattern (composition).
- ✅ Inventory Component (per-pawn).
- ✅ Equipment Component (slot dict).
- ✅ Stack count + max stack.
- ✅ Pickup actor → inventory add.
- ✅ Drop item from inventory.
- ✅ Drag-drop UMG (P10 L2).
- ✅ Composite container (bag inside bag — careful, default OFF).
- ✅ Weight + carry capacity.

**Out of scope:**
- ❌ UI rendering (P10).
- ❌ Save/load persistence (P13).

---

## PALDARK cần gì từ pillar này?

| Need | Solution |
|------|----------|
| Player carry weapon + ammo + loot | Inventory Component + Equipment Component |
| Pal carry small loot (30% player) | Pal Inventory Component (limited capacity) |
| Loot has different rarity (Common/Rare/Epic/Legendary) | Item Definition Tag + Rarity enum |
| Loot dropped on death (extraction game) | OnDeath → spawn pickup actor with inventory dump |
| Stack ammo 999 + stack consumable 10 | MaxStackSize per item |
| Hotbar 4 slot (weapon 1/2, consumable 3, Pal item 4) | Equipment Slot Tag |
| Weight system (over-encumbered = slow) | Stamina-Weight interaction |

---

## Sources Matrix

| Source | Coverage | Lesson lấy từ đó |
|--------|----------|------------------|
| [09] Inventory | 🟢 | L1 Item Definition + Fragment, L2 Inventory Component, L3 Equipment Component, L4 Stack, L5 Composite container |
| [11] Aura | 🟡 | L6 Equipment menu (basic) |
| [13] Crunch | 🟠 | L6 Shop + buy item |
| [15] Lyra RPG | 🟢 | L7 Quick Bar pattern |
| [03] PUBG | 🟡 | L8 BR-style inventory (weight + slot) |

---

## Prerequisite

- P03 (Component composition).
- P10 (UI drag-drop).
- P12 (Data-driven Item Definition).

---

## Lessons

### L1 — Item Definition + Fragment pattern (Inventory course CORE) 🧪

**Goal:** Định nghĩa Item type qua DataAsset + Fragment composition.

**Concept:**
- `UItemDefinition` PrimaryDataAsset — id, name, icon, mesh, max stack, rarity, tag.
- `UItemFragment` base — modular feature.
- Fragments: `UItemFragment_Equippable`, `UItemFragment_Consumable`, `UItemFragment_Ammo`, `UItemFragment_Weight`.
- Each fragment adds capability — composition over inheritance.

**Source learning path:**
- [09] Inventory § Item Definition chapter + Fragment chapter.
- Cross-ref: `Documents/GameDesign/09-Inventory_GDD.md` § Phase 5 ADR-1.

**API / Class chính:**
- `UPrimaryDataAsset` base.
- `UItemFragment` abstract.
- `UItemDefinition::FindFragmentByClass<T>()`.

**Practice exercise (4 hour):**
1. `UPaldarkLabItemDefinition` PrimaryDataAsset.
2. 4 fragment subclass: Equippable, Consumable, Ammo, Weight.
3. Create 5 item asset: `ID_HealthPotion` (Consumable + Weight), `ID_Pistol` (Equippable + Weight), `ID_Ammo_9mm` (Ammo + stack 60).
4. PIE: query `Item->FindFragmentByClass<UItemFragment_Weight>()->GetWeight()` → 0.5kg.

**PALDARK take-away:**
- 🟢 **CORE** — mọi item PALDARK dùng Item Definition + Fragment.
- 🟢 Fragment list cho PALDARK: Equippable, Consumable, Ammo, Weight, PalFood, Currency, Rarity, BondMaterial.

**Apply ở week:** Roadmap week 18-19.

---

### L2 — Inventory Component (per-pawn) 🧪

**Goal:** Component on Pawn, array of item entry, add/remove API.

**Concept:**
- `UInventoryComponent` on Pawn.
- `TArray<FInventoryEntry>` — entry = ItemDef + StackCount + UniqueInstanceData.
- Methods: `AddItem(Def, Count)`, `RemoveItem(Index, Count)`, `FindItem(Def)`.
- Replicate: `UPROPERTY(Replicated) TArray<FInventoryEntry> Items;` + DOREPLIFETIME.

**Source learning path:**
- [09] Inventory § Inventory Component chapter.

**Practice exercise (3 hour):**
1. `UPaldarkLabInventoryComp` with array entry.
2. `Server_AddItem(ItemDef, Count)` RPC.
3. `Server_RemoveItem(Index, Count)` RPC.
4. RepNotify `OnRep_Items` → broadcast UI update.
5. PIE: pickup → AddItem → UI auto-show.

**PALDARK take-away:**
- 🟢 Player + Pal đều có Inventory Component.
- 🟢 Pal capacity = 30% Player (force trade-off).

**Apply ở week:** Roadmap week 19 + 31.

---

### L3 — Equipment Component (slot dictionary) 🧪

**Goal:** Slot-based equipment (Weapon1, Weapon2, Armor, Helmet, etc.).

**Concept:**
- `UEquipmentComponent` on Pawn.
- `TMap<FGameplayTag, FEquipmentSlot> Slots;` (Tag = `Equipment.Slot.Weapon.Primary`).
- `EquipItem(Tag, ItemEntry)` swap with currently equipped.
- Replicate slots.
- Spawn equipped actor (e.g., `AWeapon_Pistol`) attached to socket.

**Source learning path:**
- [09] Inventory § Equipment Component chapter.

**Practice exercise (3 hour):**
1. `UPaldarkLabEquipmentComp` with 4 slot: PrimaryWeapon, SecondaryWeapon, Backpack, Helmet.
2. `EquipItem(SlotTag, ItemEntry)` → spawn actor + attach socket.
3. PIE: equip rifle → spawn `AWeapon_Rifle` actor at hand socket.

**PALDARK take-away:**
- 🟢 PALDARK 4 equipment slot + 4 quick bar slot.
- 🟢 Pal "equipment" = Saddle / Accessory (later expansion).

**Apply ở week:** Roadmap week 19.

---

### L4 — Stack + Max Stack Size 🧪

**Goal:** Item stack up to max, overflow create new entry.

**Concept:**
- When AddItem: find existing entry with same Def + spaceLeft → add.
- If no space → new entry.
- If new entry exceeds inventory size → drop pickup on ground.

**Source learning path:**
- [09] Inventory § Stack chapter.

**Practice exercise:**
1. Add 50 Ammo_9mm (maxstack 60) → 1 entry [50].
2. Add 20 more → 1 entry [60] + new entry [10].

**PALDARK take-away:**
- 🟢 Ammo max stack 60-300 (weapon-dependent).
- 🟢 Consumable max stack 5-10.
- 🟢 Pal Food max stack 20.

**Apply ở week:** Roadmap week 19.

---

### L5 — Composite Container (bag-in-bag) — careful 📖🟡

**Goal:** Hiểu pattern Composite — KHÔNG dùng cho PALDARK beta.

**Concept:**
- Composite: Item is also Container (e.g., loot crate inside backpack).
- Recursion: Container.Items = `TArray<UItem>` includes another Container.
- Pros: rich loot semantic (bag stuff inside bag).
- Cons: complexity ↑, UI nightmare, save fragile.

**Source learning path:**
- [09] Inventory § Composite chapter.

**PALDARK decision:**
- 🟡 Defer to post-beta. Beta = flat inventory.
- 🟡 Pal "carries" loot via Pal's own Inventory Component (not composite of player's).

**Apply ở week:** Beta scope — KHÔNG dùng.

---

### L6 — Equipment menu + Shop UI (Aura + Crunch) 🧪

**Goal:** UI swap equipment + buy from shop.

**Concept:**
- Equipment Menu = grid of slot UI + drag-drop to equip.
- Shop = NPC, current Gold display, item list + buy button.
- Buy: validate Gold ≥ Cost → deduct → grant item.

**Source learning path:**
- [11] Aura § Equipment menu chapter.
- [13] Crunch § Shop chapter.

**PALDARK take-away:**
- 🟢 Hub Vendor: Pal Food + Ammo + Medkit + Bond Material.
- 🟡 Adapt: Vendor uses Coordinator API (P18), persistent inventory.

**Apply ở week:** Roadmap week 35-37 (Hub vendor).

---

### L7 — Quick Bar pattern (Lyra RPG) 🧪

**Goal:** Hotbar 4-6 slot, 1-6 key activate.

**Concept:**
- `ULyraQuickBarComponent` 6 slot.
- Each slot binds InventoryItem.
- Press number key → equip or consume slot item.

**Source learning path:**
- [15] Lyra RPG § Quick Bar chapter.

**PALDARK take-away:**
- 🟢 Hotbar 4 slot: weapon1, weapon2, consumable, pal-item.
- 🟢 Press 1/2/3/4 → equip or use.

**Apply ở week:** Roadmap week 33.

---

### L8 — BR-style inventory (PUBG) 🟡

**Goal:** Weight-based + slot-based hybrid.

**Concept:**
- PUBG: backpack tiers (small/medium/large) define max weight.
- Each item has weight + size.
- Over-encumbered → slow movement.
- Slot-based weapon (primary/secondary).

**Source learning path:**
- [03] PUBG `UInventoryComponent`.
- Cross-ref: `Documents/GameDesign/03-PUBG_GDD.md`.

**PALDARK take-away:**
- 🟡 Adapt: PALDARK player có Backpack tier (basic 30kg, Trader 50kg, Elite 80kg).
- 🟡 Weight system soft (over → slow 20%, no hard cap).

**Apply ở week:** Roadmap week 23.

---

## ⚡ Capstone Exercise

**Goal:** Full inventory + equipment for PaldarkLab.

**Yêu cầu:**
1. 10 ItemDef asset (Pistol, Rifle, Ammo, Medkit, PalFood, Bandage, etc.).
2. 4 Fragment type used.
3. Player Inventory + Equipment component (replicated).
4. Pickup actor → Add to inventory on overlap.
5. Drop key → remove from inventory + spawn pickup.
6. Equip rifle → spawn actor at hand socket.
7. Quick Bar 4 slot, 1-4 key activate.
8. Weight system: > 30kg → MoveSpeed × 0.8.

**Acceptance:** Pickup → inventory; equip → world actor; weight slows.

**Effort:** 1.5 tuần.

---

## Khi apply vào PALDARK roadmap

| Week | Pillar P11 usage |
|------|------------------|
| W18 | Item Definition + Fragment design |
| W19 | Inventory + Equipment Component |
| W23 | Weight system + Backpack tier |
| W31 | Pal Inventory Component |
| W33 | Lyra Quick Bar adopt |
| W35-37 | Hub Vendor + persistent inventory |

---

## Anti-patterns (cảnh báo)

- 🔴 Composite container default ON → UI nightmare.
- 🔴 Item logic in Pawn class — should be Item Fragment.
- 🔴 Inventory = global Subsystem — should be per-pawn Component.
- 🔴 Sync save inventory on every Add → frame freeze. Batch save.
- 🔴 No max stack → exploit hoard.
- 🔴 Client-authoritative item add → cheat.

---

## Tham chiếu

- [`Documents/UE5_Core_Pillars.md`](../UE5_Core_Pillars.md) § P11.
- [`Documents/GameDesign/09-Inventory_GDD.md`](../GameDesign/09-Inventory_GDD.md) — main source.
- [`Documents/GameDesign/03-PUBG_GDD.md`](../GameDesign/03-PUBG_GDD.md) — BR weight system.
- [`Documents/GameDesign/15-Lyra_RPG_GDD.md`](../GameDesign/15-Lyra_RPG_GDD.md) — Quick Bar.
