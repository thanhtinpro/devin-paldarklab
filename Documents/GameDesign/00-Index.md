# GameDesign Index — 15 Source × Donchitos Reverse GDD

> Áp dụng [Donchitos 7-phase framework](../Methodology/01-Donchitos_Applied.md) **ngược** cho 15 source đã có.
>
> User yêu cầu: coi cả 15 source là "game" (12 course + 3 shipped/skeleton). Mỗi source = 1 game với mục tiêu khác nhau. Áp Donchitos để rút phương pháp luận đã/chưa được dùng → take-away cho PALDARK.
>
> **Đây là phần phân tích "trước khi viết code"** — mỗi source được "design lại" để hiểu tác giả ra quyết định gì.

---

## Notation

- 📦 **Type:** Course = giảng dạy / Shipped = đã release / Skeleton = empty template / Prototype = demo
- 🎯 **Coverage UE5 Pillar:** % bao phủ 18 pillar (số đã tính ở [Projects_Comparison.md](../Projects_Comparison.md))
- 🟢🟡🟠🔴 **Mức kiểm chứng cho PALDARK:** 🟢=copy nguyên / 🟡=adapt / 🟠=tham chiếu / 🔴=anti-pattern

---

## Bảng 15 Source × Game Identity

| # | Source | 📦 Type | Genre | 🎯 Coverage | Mức PALDARK |
|---|--------|---------|-------|-------------|-------------|
| 01 | [Tom Looman ActionRoguelike](01-ActionRoguelike_GDD.md) | Course | Single-player top-down action RPG | ~45% | 🟢 Pattern lib |
| 02 | [Palworld Project](02-Palworld_GDD.md) | Skeleton | Open-world creature-collect survival | ~21% | 🟡 Base extend |
| 03 | [PUBG-KI](03-PUBG_GDD.md) | Prototype | Battle royale 100-player | ~53% | 🟡 Manager + Lobby |
| 04 | [Ready or Not](04-ReadyOrNot_GDD.md) | Shipped (AAA) | Tactical FPS co-op SWAT | ~73% | 🟢 Activity AI + Significance |
| 05 | [GAS Crash Course](05-GAS_CrashCourse_GDD.md) | Course | Tech demo (Projectile + Effect) | GAS-only | 🟢 GAS basics |
| 06 | [Blueprint MP Crash](06-Blueprint_MP_Crash_GDD.md) | Course | Party arena multiplayer | MP-only | 🟠 BP perspective |
| 07 | [C++ MP Crash](07-CPP_MP_Crash_GDD.md) | Course | Multiplayer foundational | MP-only | 🟢 Theory base |
| 08 | [AWS GameLift](08-AWS_GameLift_GDD.md) | Course | PvP arena + backend stack | Backend | 🟢 AWS playbook |
| 09 | [Inventory Systems](09-Inventory_GDD.md) | Course | RPG inventory showcase | Inventory | 🟢 Fragment + Composite |
| 10 | [MP Shooter Blaster](10-MP_Shooter_GDD.md) | Course | Lyra-style PvP shooter | MP + Shooter | 🟢 Lag Comp pattern |
| 11 | [Aura GAS Top-Down](11-Aura_RPG_GDD.md) | Course | Diablo-like ARPG | GAS depth | 🟢 Damage chain + Spells |
| 12 | [Pro UE4 Game Coding](12-Pro_UE_GDD.md) | Course | Sandbox compendium | Math/Physics | 🟡 Reference textbook |
| 13 | [Crunch](13-Crunch_GDD.md) | Course | 3v3 MOBA arena | GAS+AWS | 🟢 Coordinator pattern |
| 14 | [Exploring Lyra](14-Exploring_Lyra_GDD.md) | Course | Lyra framework tour | Lyra | 🟢 Lyra backbone |
| 15 | [Lyra RPG](15-Lyra_RPG_GDD.md) | Course | Lyra-powered action RPG | Lyra+RPG | 🟢 Application example |

---

## Game Pillar Cluster — 15 Source

Mỗi source rút ra **3 game design pillar** (cảm xúc, không phải kỹ thuật). Bảng tổng:

| # | Source | Pillar 1 | Pillar 2 | Pillar 3 |
|---|--------|----------|----------|----------|
| 01 | Tom Roguelike | Iteration is mastery | Power growth through items | Death is teacher |
| 02 | Palworld | Capture transcends combat | Friendship has utility | World invites curiosity |
| 03 | PUBG | Stillness wins | Last man surviving | Loot defines strategy |
| 04 | RoN | Tactical patience | Every door is a decision | Civilians have weight |
| 05 | GAS Crash | Decouple cause and effect | Tag the world | Data drives behavior |
| 06 | BP MP Crash | Connection is gameplay | Listen vs Dedicated matters | UI is the lobby |
| 07 | C++ MP Crash | Authority is sacred | Replication is choice | RPC is intent |
| 08 | AWS GameLift | Identity precedes play | Match before mayhem | Persistence is product |
| 09 | Inventory | Items are characters | Stack grows world | Equipment shapes player |
| 10 | MP Shooter | Hit registers truth | Latency lies, server tells | Cosmetic ≠ power |
| 11 | Aura | Ability tells story | Damage is a conversation | Level is permanent |
| 12 | Pro UE | Math is mechanic | Physics has personality | Audio is feedback |
| 13 | Crunch | Combo is identity | Server is referee | Match starts before match |
| 14 | Exploring Lyra | Configuration over code | Experience composes game | Modular gameplay scales |
| 15 | Lyra RPG | Locomotion is character | UI tells health | Indicator guides |

---

## Take-aways tổng cho PALDARK

> Mỗi source contribute **1–3 pattern** cho PALDARK. Tổng hợp:

### 🟢 Copy nguyên (Pattern Library cho PALDARK)
- **Tom Looman:** AttributeComponent, ActionComponent, DataAsset + AssetManager, Aggregate Tick, Object Pool.
- **Ready or Not:** Activity FSM base class, ThreatOctree, SignificanceManager subclass, FMOD Audio Propagation Component.
- **PUBG-KI:** Lobby + Coordinator pattern, Zone Manager, 6-Component Pawn composition (clean version).
- **Aura RPG:** Damage Execution Calculation, Curve Table progression, Widget Controller MVC, Checkpoint save.
- **MP Shooter:** Server-Side Rewind / Lag Compensation Frame Buffer.
- **AWS GameLift:** Cognito + Lambda + DynamoDB + Anywhere Fleet → Production Fleet pipeline.
- **Inventory:** Fragment composition, Composite container, Equipment slot data-driven.
- **Lyra:** ExperienceDefinition, ExperienceActionSet, GameFeature Plugin, Modular Gameplay, CommonUI activatable, Indicator Manager.
- **Crunch:** Server Coordinator pattern, Container deploy, GAS net update optimization.

### 🟡 Adapt (giữ ý, đổi cấu trúc)
- **Palworld skeleton:** giữ Component slot, fill code (PawnExtensionComponent → ActivityComponent style của Lyra/RoN).
- **PUBG Manager pattern:** convert AActor singleton → UWorldSubsystem.

### 🔴 Anti-pattern (tránh)
- **ReadyOrNot god-character (368KB) + god-widget (5K LOC):** không build PlayerCharacter trên 50KB; widget không quá 1.5K LOC.
- **PUBG Manager-as-Actor:** không tạo AActor singleton — dùng UWorldSubsystem.
- **No-Lyra approach:** không build framework custom từ đầu — dùng Lyra backbone.

---

## Workflow áp dụng Donchitos Reverse

Mỗi file `0X-<name>_GDD.md` sẽ áp **7-phase backward**:

1. **Brainstorm** — implicit pitch của source.
2. **Decompose** — system hierarchy có trong source.
3. **Define** — GDD reverse (extract từ code/lesson).
4. **Pillars** — 3 game pillar inferred từ design decision.
5. **Architecture** — ADR implicit (decision tác giả đã làm).
6. **System Map** — class + file reference.
7. **Stories** — chương/feature như US tickets.

Cuối mỗi file: **Take-aways cho PALDARK** + **Tham chiếu chéo**.

---

## Cách dùng tài liệu này

### Khi code tuần X của roadmap
1. Mở [`PALDARK/04-Resource_Map.md`](../PALDARK/04-Resource_Map.md) → biết pillar nào tuần này → biết source nào 🥇.
2. Mở file GDD tương ứng ở folder này → **Phase 5 (ADR)** + **Phase 6 (System Map)** = pattern code cụ thể.
3. Copy pattern từ source về PALDARK module → modify cho phù hợp.

### Khi quyết định kiến trúc mới
1. Đọc 3 GDD cùng genre (ví dụ 4-RoN + 11-Aura + 14-Lyra cho RPG decision).
2. So sánh **Phase 5 ADR** giữa 3 source.
3. Quyết định cho PALDARK ADR.

### Khi rebuild course mới
- Folder này = blueprint.
- [Documents/UE5_Course/](../UE5_Course/) (PR 4c sắp tới) = lesson per pillar.

---

## Tài liệu liên quan

- [Donchitos Framework](../Donchitos_GameStudios_Framework.md)
- [Methodology — Donchitos Applied](../Methodology/01-Donchitos_Applied.md)
- [Methodology — Vision Reverse Engineering](../Methodology/00-Vision_Reverse_Engineering.md)
- [PALDARK Vision](../PALDARK/00-VISION.md)
- [Per-course lesson roadmap](../Courses/) (PR #3)
- [Project assessment](../Projects/) (PR #3)
- [Projects comparison](../Projects_Comparison.md) (PR #3)
- [UE5 18 Pillars](../UE5_Core_Pillars.md) (PR #3)
- [Learning Roadmap](../Learning_Roadmap.md) (PR #3)
