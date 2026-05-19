# UE5 Unified Course — 18 Pillars × Lessons

> Khoá học tự soạn, **lấy 18 UE5 Core Pillars làm chuẩn**, rút resource từ **15 source** (12 Udemy course + 3 real project).
>
> Khác với học từng course rồi tổng hợp pillar — ở đây ta đi **pillar-first**: mỗi pillar = 1 file, mỗi pillar có 4–6 lesson, mỗi lesson cite source nào dạy chỗ đó.
>
> Mục tiêu duy nhất: **mỗi lesson áp ngay vào PALDARK trong vòng 1 năm** — không học theory rời rạc.

---

## Cách dùng course này

### Khi học pillar mới
1. Đọc file `P_XX_*.md`.
2. Mỗi lesson liên kết đến source gốc + GDD reverse (`Documents/GameDesign/`).
3. Làm capstone exercise của pillar (nhỏ — 1–3 ngày).
4. Áp dụng vào PALDARK milestone tương ứng (xem `Documents/PALDARK/03-Roadmap_1_Year.md`).

### Khi viết code PALDARK
1. Tra `Documents/PALDARK/04-Resource_Map.md` xem week này dùng pillar nào.
2. Mở file pillar → xem lesson tương ứng.
3. Copy/adapt theo notation 🟢🟡🔴.

### Khi review architecture
1. Mở pillar liên quan → ADR section.
2. Đối chiếu với `Documents/PALDARK/01-GameDesign.md` ADR list.

---

## 18 Pillars — Bird-Eye View

| # | Pillar | Focus | Effort (week) | Top source |
|---|--------|-------|---------------|------------|
| P01 | C++ & Build Foundation | Module, UCLASS, GC, smart ptr | 2 | [12] Pro UE |
| P02 | Core Class Framework | GameMode/State/PC/PS/Pawn/HUD/GI/Subsystem | 2 | [07] CPP MP |
| P03 | Composition (Actor/Component/Subsystem) | Avoid god-object | 2 | [01] Tom + [14] Lyra |
| P04 | Enhanced Input | IA/IMC/Trigger/Modifier | 1 | [11] Aura |
| P05 | Animation | AnimBP, Montage, RootMotion, Distance Match | 3 | [15] Lyra RPG |
| P06 | Multiplayer / Replication | Authority, RPC, RepNotify, Lag Comp | 4 | [07] + [10] Blaster |
| P07 | Dedicated Server + Sessions | GameLift, Container, Coordinator | 3 | [08] + [13] Crunch |
| P08 | Gameplay Ability System | ASC, AS, GA, GE, Tag, Cue, Exec Calc | 4 | [11] Aura + [13] Crunch |
| P09 | AI | BT, EQS, State Tree, Activity FSM, Utility, Custom Sense | 4 | [04] RoN + [01] Tom |
| P10 | UI (UMG / CommonUI) | Widget Controller MVC, Activatable Stack, Indicator | 3 | [15] Lyra RPG + [11] Aura |
| P11 | Inventory & Equipment | Fragment, Composite, Quick Bar | 2 | [09] Inventory |
| P12 | Data-Driven Design | DataTable, DataAsset, PrimaryDataAsset, AssetManager | 2 | [01] Tom + [04] RoN |
| P13 | Save / Load | SaveGame archive, Checkpoint, Server persistence | 2 | [01] Tom + [11] Aura |
| P14 | Asset Manager + Async | FStreamableManager, Game Feature Plugin | 2 | [14] Lyra |
| P15 | Performance & Production | Significance, Pool, Aggregate Tick, Insights | 3 | [04] RoN + [14] Lyra |
| P16 | Math / Physics / Audio | Vec/Quat/Lerp, sub-step, FMOD | 2 | [12] Pro UE + [04] RoN |
| P17 | Lyra Framework | Experience, Game Feature, Modular Gameplay | 3 | [14] + [15] |
| P18 | Backend / Live Ops | Cognito, Lambda, DynamoDB, EOS | 2 | [08] + [13] |

**Tổng effort:** ~46 tuần (≈ song song với roadmap 52 tuần ship beta).

---

## Pre-requisite Graph

```
                 P01 C++ & Build
                  /            \
            P02 Core           P12 Data-Driven
           Framework            (DataAsset)
            /     \                  \
   P03 Composition  P04 Input        P14 AssetMgr
        /      \         \            (async)
   P05 Anim   P06 Repl   P10 UI       \
              /  \  \       \         P17 Lyra
            P07  P08 P09    P11 Inv    (frame)
           Server GAS  AI    
            /                \         
          P18                P15 Perf
        Backend             P13 Save
                            P16 Math/Audio
```

**Đường ngắn nhất đến gameplay loop:** P01 → P02 → P03 → P04 → P05 → P06 → P07 → P10 → P11 (9 pillar = ~22 tuần).

**Pillar deferred (sau khi vertical slice xong):** P14 (week 23+), P15 (week 24+), P17 (week 33+), P18 (week 38+).

---

## Notation

| Symbol | Ý nghĩa |
|--------|---------|
| 🟢 | Copy-as-is — pattern production-grade |
| 🟡 | Adapt — refactor cho PALDARK scope |
| 🟠 | Reference — đọc để hiểu nhưng không copy |
| 🔴 | Anti-pattern — biết để tránh |
| 📖 | Concept lesson (đọc + screenshot) |
| 🧪 | Practice lesson (code/build mini-app) |
| ⚡ | Capstone — apply tất cả lesson trong pillar |

---

## Lesson Template (mỗi lesson trong P_XX_*.md)

```markdown
### L<num> — <Tên lesson> <emoji>

**Prerequisite:** L<n-1>, hoặc Pillar nào khác

**Goal:** sau lesson, anh làm được X.

**Concept (10 phút đọc):**
- Bullet 1
- Bullet 2

**Source learning path:**
- [<idx> Source name] § Chương Y → file/class cụ thể
- Cross-ref: `Documents/GameDesign/<idx>-*.md` § Phase Z

**API / Class chính:**
- `UClassName::Method()` — mô tả
- `FStructName` — mô tả

**Practice exercise (1-3 hour):**
1. Tạo Pawn class với Y component.
2. Override Z method.
3. Test trong PIE.

**PALDARK take-away:**
- 🟢 Copy pattern này cho `APaldark<class>`.
- 🟡 Adapt: thay X bằng Y.
- 🔴 Không làm cách XYZ.

**Apply ở week:** Roadmap week N.
```

---

## Quan hệ với các tài liệu khác

| File | Quan hệ |
|------|---------|
| `Documents/UE5_Core_Pillars.md` (PR #3) | **Pillar definition (one-page)** — file này là **deep-dive** |
| `Documents/Learning_Roadmap.md` (PR #3) | Beginner→Pro order. Course này song song. |
| `Documents/Courses/*.md` (PR #3) | Course content summary. Lesson cite vào đây. |
| `Documents/GameDesign/*.md` (PR #5) | Donchitos reverse GDD. Lesson cite phase 5 (ADR). |
| `Documents/PALDARK/03-Roadmap_1_Year.md` (PR #4) | Week mapping. Lesson cite "Apply ở week N". |
| `Documents/PALDARK/04-Resource_Map.md` (PR #4) | Resource lookup. Lesson cite back. |

---

## 18 Pillar Files

- [P01_CPP_Build.md](P01_CPP_Build.md)
- [P02_Core_Framework.md](P02_Core_Framework.md)
- [P03_Composition.md](P03_Composition.md)
- [P04_Enhanced_Input.md](P04_Enhanced_Input.md)
- [P05_Animation.md](P05_Animation.md)
- [P06_Replication.md](P06_Replication.md)
- [P07_Dedicated_Server.md](P07_Dedicated_Server.md)
- [P08_GAS.md](P08_GAS.md)
- [P09_AI.md](P09_AI.md)
- [P10_UI.md](P10_UI.md)
- [P11_Inventory.md](P11_Inventory.md)
- [P12_DataDriven.md](P12_DataDriven.md)
- [P13_SaveLoad.md](P13_SaveLoad.md)
- [P14_AssetMgr.md](P14_AssetMgr.md)
- [P15_Performance.md](P15_Performance.md)
- [P16_Math_Physics_Audio.md](P16_Math_Physics_Audio.md)
- [P17_Lyra.md](P17_Lyra.md)
- [P18_Backend_LiveOps.md](P18_Backend_LiveOps.md)
