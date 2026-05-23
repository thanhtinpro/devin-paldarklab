# 09. Unreal Engine 5 C++ Inventory Systems

| | |
|---|---|
| **Tier** | T4 — Specialized Systems |
| **Provider** | Stephen Ulibarri (DruidMech) — Udemy |
| **Link** | https://www.udemy.com/course/unreal-engine-5-inventory-systems/ |
| **Source** | https://github.com/DruidMech/InventorySystems |
| **Trong repo** | `09.Udemy-ue5-inventory-system/` (Documents + Plugins + Source) |

---

## 1. Mục tiêu khóa

Xây inventory system **production-grade, plugin-able**, có pattern composition (Item Fragments) + Composite Pattern cho container items.

---

## 2. Lộ trình giảng dạy (16 chương)

| Chương | Cốt lõi |
|--------|---------|
| 01 - Introduction | Phạm vi khóa, project setup, custom character mesh, **Creating a Plugin** |
| 02 - Inventory HUD | Widget khung, slot grid, focus management |
| 03 - Inventory Data | `UDataAsset` item, `FInventoryItem` struct |
| 04 - **Item Fragments** | Composition pattern — fragment = thuộc tính độc lập (Stack, Equip, Consume, Stat) |
| 05 - Searching for a Place to Belong | Algorithm tìm slot phù hợp cho item mới |
| 06 - Adding Stacks | Stack count, merge, split |
| 07 - Pick Up | Interact + add item flow |
| 08 - Hover Item | Tooltip, preview |
| 09 - Put Down | Drop item về world |
| 10 - Pop Up Menu | Right-click menu (Use/Drop/Equip/Inspect) |
| 11 - Dropping Items | Spawn pickup actor có data |
| 12 - Consuming Items | GameplayEffect hoặc direct attribute change |
| 13 - Item Description | Rich Text + dynamic value |
| 14 - **The Composite Pattern** | Container items (bag chứa item con) |
| 15 - Equipping Items | Slot equipment, validate slot type |
| 16 - Equipment Component | `UEquipmentComponent` decouple từ inventory |

---

## 3. Cốt lõi (Pillars)

| Pillar | Mức |
|--------|-----|
| P11 Inventory | 🔴 (deepest course on inventory) |
| P3 Composition | 🟠 (Fragment + Composite) |
| P10 UI | 🟠 (full HUD + tooltip + popup) |
| P12 Data-driven | 🟠 (DataAsset cho item) |

**Pattern key:**
- **Item Fragment:** thay vì 1 class `UInventoryItem` to bự (cooldown, stack, equip, stat), chia ra:
  - `UItemFragment_Stackable`
  - `UItemFragment_Equipable`
  - `UItemFragment_Consumable`
  - `UItemFragment_Statful`
  - Một item = `UDataAsset` chứa array các fragment.
- **Composite Pattern:** `UInventory` chứa `UInventoryEntry`; mỗi entry có thể là single item *hoặc* container (chứa `UInventory` con) → bag-in-bag.

---

## 4. Approach sư phạm

- **Plugin từ chương 1:** dạy tách inventory thành plugin tái dùng → mặc nhiên Modular.
- **Compose, don't inherit:** mọi item là 1 DataAsset + array fragments, không có hierarchy sâu.
- **UI bound vào delegate:** mọi widget reactive với inventory change.
- **Equipment tách khỏi Inventory:** equipment ≠ inventory storage — học cách thiết kế interface giữa 2 thằng.

---

## 5. Đầu ra học viên

- Có **plugin Inventory** drop được vào project bất kỳ.
- Hiểu khi nào dùng composition vs inheritance cho game data.
- Có UI inventory drag-drop chuẩn.

---

## 6. Lưu ý

- **Không cover multiplayer inventory** — single-player only. Cần custom replication cho FastArraySerializer khi multiplayer.
- **Không cover save/load inventory** — phải kết hợp với [01] Tom Save System hoặc [11] Aura save.
- **Không cover crafting** — chỉ inventory + equipment.

---

## 7. Liên kết khóa khác

| Trước | Sau / parallel |
|-------|----------------|
| [01] Tom Action | [11] Aura RPG (inventory integration), [15] Lyra Inventory QuickBar |
