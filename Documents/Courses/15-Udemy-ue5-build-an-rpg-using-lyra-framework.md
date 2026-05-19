# 15. Unreal Engine 5.6 — Build an Action RPG using Lyra Framework

| | |
|---|---|
| **Tier** | T5 — Pro Production (Lyra applied) |
| **Provider** | Udemy |
| **Link** | https://www.udemy.com/course/unreal-engine-56-build-an-rpg-using-lyra-framework/ |
| **Trong repo** | `15.Udemy-ue5-build-an-rpg-using-lyra-framework/Documents/` (đầy đủ chương, không có Source) |

---

## 1. Mục tiêu khóa

Build **action-RPG hoàn chỉnh trên Lyra framework** (UE5.6). Khóa **hands-on extend Lyra**, không deep-dive như [14].

---

## 2. Lộ trình giảng dạy (16 section)

| Section | Cốt lõi |
|---------|---------|
| 01 - Introduction | Visual Studio install, **Unreal Engine Fundamentals**, course about |
| 02 - Creating a new RPG Experience | Tạo `ULyraExperienceDefinition` riêng cho RPG |
| 03 - **Lightweight Lyra Locomotion System** | Locomotion đơn giản hóa từ Lyra |
| 04 - Environment Creation | Map dungeon |
| 05 - Creating a Great Sword Weapon | Weapon class, equip flow, animation |
| 06 - **RPGStats - LyraAttributeSet** | Custom AttributeSet kế thừa ULyraAttributeSet |
| 07 - Creating a Melee Attack Ability | GA melee với Lyra input config |
| 08 - **AI and State Trees** | State Tree thay Behavior Tree cho enemy |
| 09 - Creating an Archer Ability | Ranged ability với projectile |
| 10 - Creating the Inventory Quick Bar | Quick bar pattern (slot 1-9 keyboard) |
| 11 - Creating a Magic Ability for the Boss | Boss-tier ability với phase |
| 12 - Creating a FrontEnd Menu | Main menu + setting CommonUI |
| 13 - **Lyra Indicator Manager Component** | World-space indicator UI (objective, quest) |
| 14 - **Lyra Inventory and Interaction** | Lyra inventory hệ thống + interact prompt |
| 15 - Assignments | Bài tập tự làm |
| 16 - What's Next | Wrap |

---

## 3. Cốt lõi (Pillars)

| Pillar | Mức |
|--------|-----|
| P17 Lyra | 🔴 (applied: tạo experience, extend AttributeSet, GameFeature) |
| P8 GAS | 🔴 (Lyra GAS flavor) |
| P9 AI | 🟠 (**State Tree** — quan trọng cho UE5.4+ AI) |
| P10 UI | 🟠 (CommonUI, Quick Bar, FrontEnd, Indicator Manager) |
| P11 Inventory | 🟠 (Lyra Inventory Quick Bar + Lyra Inventory module) |
| P4 Enhanced Input | 🟠 (Lyra Input Config) |
| P5 Animation | 🟠 (Locomotion từ Lyra) |
| P12 Data-driven | 🟠 (Experience definition = DataAsset) |

**Pattern key:**
- **State Tree** thay BT: State + Transition + Task gọn hơn BT cho UE5.4+. Section 8.
- **Indicator Manager:** Lyra có sẵn `IndicatorManagerComponent` — thêm UI element 3D vào widget pool. Section 13.
- **Quick Bar Inventory:** pattern slot 1–9 keyboard, không phải grid inventory. Section 10.
- **Lyra Experience DataAsset:** mỗi experience là 1 PrimaryDataAsset. Section 2.

---

## 4. Approach sư phạm

- **Pragmatic apply:** mỗi section sản phẩm chạy được. Không lý thuyết dài như [14].
- **Lyra hands-on > Lyra deep dive:** khóa này build, [14] khóa đọc.
- **State Tree introduction:** lần đầu tiên trong 15 khóa có State Tree (UE5.4+).
- **Locomotion lightweight:** Lyra mặc định nặng — section 3 dạy cách giảm.

---

## 5. Đầu ra học viên

- Có **action-RPG dungeon** trên Lyra với great sword + archer + magic + state tree AI + inventory quick bar + boss phase.
- Biết tạo Lyra Experience custom cho game riêng.
- Hiểu State Tree (next-gen AI).
- Biết tích hợp Indicator Manager cho quest UI.

---

## 6. Lưu ý

- **Yêu cầu nền cao:** xong [05] + [11] + [14] trước (hiểu GAS + Lyra structure).
- **UE5.6-only** — khác với [14] UE5.2/5.3, một số API đã thay.
- **Single-player focused** — multiplayer trên Lyra phải tham khảo [13] và Lyra docs.

---

## 7. Liên kết khóa khác

| Trước | Sau |
|-------|-----|
| [14] Exploring Lyra, [11] Aura | (đỉnh stack RPG) |
