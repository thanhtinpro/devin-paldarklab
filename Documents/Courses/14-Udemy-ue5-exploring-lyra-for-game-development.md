# 14. Unreal Engine 5.2 — Exploring Lyra for Game Development

| | |
|---|---|
| **Tier** | T5 — Pro Production (Lyra deep dive) |
| **Provider** | Udemy |
| **Link** | https://www.udemy.com/course/exploring-lyra-for-game-development-in-unreal-engine-52/ |
| **Trong repo** | `14.Udemy-ue5-exploring-lyra-for-game-development/Documents/` (đầy đủ chương, không có Source) |

---

## 1. Mục tiêu khóa

Hiểu **Lyra framework của Epic** — cách Epic thiết kế UE5 sample chuẩn. Khóa **đọc + extend** Lyra, không build game từ đầu.

---

## 2. Lộ trình giảng dạy (12 section)

| Section | Cốt lõi |
|---------|---------|
| 01 - Introduction | Phạm vi khóa, Lyra là gì, **Different Aspects of Game Development**, intro Procedural game dev, Course Structure |
| 02 - Lyra and GAS Concepts | Lyra GAS architecture, AS, GA, GE trong Lyra style |
| 03 - Developing a Combat Game | Combat scaffold trên Lyra |
| 04 - **Performance Optimization** | Insights, Stat command, Niagara cull, Significance Manager |
| 05 - **Lyra Deep Dive** | Experience, GameFeature Plugin, Modular Gameplay, ULyraExperienceDefinition, ULyraExperienceActionSet |
| 06 - **Extending Lyra — Adding new Character Abilities** | Custom ability tích hợp vào Lyra Input Config |
| 07 - Multiplayer Gaming | Lyra multiplayer pipeline |
| 08 - How-To | Recipe: thêm character, thêm map, thêm ability |
| 09 - Mini Project 1 (Custom Animated Characters with Modified Gameplay) | Demo project |
| 10 - References | Resources |
| 11 - Section 11 Unreal Engine 5.3 Directions | UE 5.3 changes |
| 12 - **Lyra from Scratch in Unreal Engine 5.3** | Rebuild minimal Lyra |

---

## 3. Cốt lõi (Pillars)

| Pillar | Mức |
|--------|-----|
| P17 Lyra Framework | 🔴 (sâu nhất 12 khóa) |
| P3 Composition | 🔴 (Modular Gameplay, GameFeature) |
| P14 Asset Manager / Async | 🔴 (GameFeature = async load) |
| P15 Performance | 🟠 (Significance, Niagara cull) |
| P8 GAS | 🟠 (Lyra-flavored GAS) |
| P10 UI | 🟠 (CommonUI in Lyra) |

**Key Lyra concept:**
- **Experience:** một experience = 1 game mode "package" gồm DefaultPawnData + Components + Actions. Đổi experience = đổi gameplay không reload level.
- **ExperienceActionSet:** group action có thể bật/tắt — modular.
- **GameFeature Plugin:** package gameplay riêng (1 character, 1 map, 1 mode) chunked, async load. Hoán đổi runtime.
- **Modular Gameplay:** `UGameFrameworkComponentManager` — Add Components / Add Abilities theo experience, không cần spawn cố định trên Pawn.

---

## 4. Approach sư phạm

- **Đọc-trước-extend:** không build từ đầu (trừ section 12). Mỗi section bóc 1 layer của Lyra.
- **Theory-heavy:** "Different Aspects of Game Development" + "Procedural game Development" làm framework hiểu vì sao Lyra thiết kế thế.
- **Mini project ở section 9:** áp dụng để thấy mình hiểu.
- **Section 12 "Lyra from Scratch":** rebuild minimal Lyra để chứng minh hiểu thật.

---

## 5. Đầu ra học viên

- Đọc được code Lyra mà không lạc.
- Tạo được GameFeature Plugin cho game riêng.
- Hiểu khi nào dùng Lyra (game có nhiều experience/mode) vs không dùng Lyra (game nhỏ, single experience).

---

## 6. Lưu ý

- **Lyra over-engineered cho game nhỏ.** Đừng force Lyra cho mọi project.
- **Phụ thuộc engine version:** Lyra ở UE5.2 vs UE5.6 có thay đổi. Khóa này UE5.2/5.3, cần update khi áp dụng cho UE5.6.
- **Không cover dedicated server cho Lyra** — phải lên [13] hoặc tự research.

---

## 7. Liên kết khóa khác

| Trước | Sau |
|-------|-----|
| [05] GAS Crash, [11] Aura | [15] Lyra RPG |
