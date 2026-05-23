# Donchitos Framework Applied — Workflow cho Mọi Dự Án

> Áp dụng [Donchitos / Claude-Code Game Studios](https://github.com/Donchitos/Claude-Code-Game-Studios) cho phát triển game UE5.
>
> Đây là **workflow chuẩn** dùng cho **bất kỳ** dự án game — không chỉ PALDARK.
>
> Bổ sung [00-Vision_Reverse_Engineering.md](00-Vision_Reverse_Engineering.md).

---

## Tóm Tắt Donchitos

Donchitos / Claude-Code-Game-Studios cung cấp framework game dev gồm:
- **49 agent** chuyên ngành (game-designer, ai-engineer, level-designer, narrative-designer, …)
- **73 skill** (slash commands như `/brainstorm`, `/define`, `/architecture-decision`, `/create-story`, …)
- **7-phase workflow**: Brainstorm → Decompose → Define → Architecture → Map Systems → Create Story → Implement

Đầy đủ: [../Donchitos_GameStudios_Framework.md](../Donchitos_GameStudios_Framework.md).

---

## Mapping 7-Phase Donchitos → UE5

| Phase Donchitos | Skill chính | UE5 deliverable | File mẫu |
|-----------------|-------------|-----------------|----------|
| 1. Brainstorm | `/brainstorm` | High-concept 1 trang | `Documents/<Game>/00-VISION.md` |
| 2. Decompose | `/decompose` | System hierarchy | `Documents/<Game>/01-GameDesign.md` §2 |
| 3. Define | `/define` | GDD full | `Documents/<Game>/01-GameDesign.md` §3 |
| 4. Define-Pillars | `/define-pillars` | 3 Game Pillars | `01-GameDesign.md` §4 + `00-VISION.md` §III |
| 5. Architecture | `/architecture-decision` | ADR set | `Documents/<Game>/ADR/ADR-XXX.md` |
| 6. Map Systems | `/map-systems` | System docs | `Documents/<Game>/Systems/*.md` |
| 7. Create Story | `/create-story` | US tickets | `Documents/<Game>/Stories/US-XXX.md` |

---

## Workflow Chi Tiết (Áp dụng cho PALDARK + future games)

### Phase 1 — Brainstorm

**Input:** Cảm hứng (game tham khảo, mood, target audience).
**Output:** 1 trang high-concept.

**Sub-steps:**
1. Liệt kê 5 game gần nhất trong tâm tưởng.
2. Tách yếu tố thích của mỗi game.
3. Ghép yếu tố thành combo chưa ai làm.
4. Viết "Một câu Pitch 10 giây".
5. Test với 3 người không biết.

**Đầu ra mẫu:** [PALDARK/00-VISION.md §I](../PALDARK/00-VISION.md#i-mt-cu-pitch-10-giây).

### Phase 2 — Decompose

**Input:** High-concept.
**Output:** System hierarchy (text diagram).

**Sub-steps:**
1. Liệt kê tất cả "verb" trong game (drop, loot, tame, extract, breed, …).
2. Group verb thành system.
3. Map system → pillar UE5 (P1-P18).
4. Vẽ diagram phụ thuộc.

**Đầu ra mẫu:** [PALDARK/01-GameDesign.md §Phase 2](../PALDARK/01-GameDesign.md#phase-2--decompose).

### Phase 3 — Define (GDD)

**Input:** System hierarchy.
**Output:** GDD đầy đủ — Game Loop / Character / Combat / Inventory / UI / World / etc.

**Sub-steps mỗi mục:**
1. Mô tả mechanic.
2. Mô tả UI/UX.
3. Mô tả data needed.
4. Mô tả failure modes.

**Đầu ra mẫu:** [PALDARK/01-GameDesign.md §Phase 3](../PALDARK/01-GameDesign.md#phase-3--define-gdd-chi-tiết).

### Phase 4 — Define Pillars

**Input:** GDD.
**Output:** Chính xác 3 game pillar.

**Sub-steps:**
1. Đọc GDD, rút **cảm xúc** trục.
2. Viết 3 pillar (mỗi pillar = 1 câu trích + 2-3 mệnh đề).
3. Test pillar bằng cách thử cắt feature: nếu cắt được mà không hỏng game → feature đó không phục vụ pillar đủ mạnh.

**Đầu ra mẫu:** [PALDARK/01-GameDesign.md §Phase 4](../PALDARK/01-GameDesign.md#phase-4--define-pillars).

### Phase 5 — Architecture (ADR)

**Input:** GDD + pillar.
**Output:** Bộ ADR (Architectural Decision Record).

**ADR Template:**
```markdown
# ADR-XXX: [Tên quyết định]

## Bối cảnh
[Vì sao cần quyết định này]

## Lựa chọn
- A. [Option A]
- B. [Option B]
- C. [Option C]

## Quyết định
**Chọn:** [A/B/C]

## Lý do
- [Lý do 1]
- [Lý do 2]

## Hệ quả
**Positive:**
- [...]

**Negative / Risk:**
- [...]

## Date / Version
2026-MM-DD / v0.1

## Liên quan
- ADR-YYY
- Pillar [N]
```

**ADR PALDARK bắt buộc:**
- ADR-001: Lyra Framework backbone?
- ADR-002: AWS GameLift hay custom EC2?
- ADR-003: GAS scope (full hay subset)?
- ADR-004: Activity FSM port hay viết mới?
- ADR-005: Inventory Fragment hay Lyra Inventory?
- ADR-006: FMOD hay UE native audio?
- ADR-007: DynamoDB hay relational?
- ADR-008: 4-player vs 6-player squad?

### Phase 6 — Map Systems (per-system docs)

**Input:** GDD + ADR.
**Output:** 1 file `.md` per system trong `Documents/<Game>/Systems/`.

**Template mỗi system:**

```markdown
# System: [Tên]

## 1. Concept
[3-5 dòng]

## 2. Pillar Mapping
- Primary: P[X]
- Secondary: P[Y], P[Z]

## 3. Class Diagram (text)
[ASCII diagram]

## 4. Key Components
| Class | Trách nhiệm |
|-------|-------------|

## 5. Tech Decisions (ADR refs)
- ADR-XXX: [...]

## 6. Source nghiên cứu
- 🥇 [Nguồn 1]
- 🥈 [Nguồn 2]

## 7. Order of Development
1. Tuần X: [Task]
2. Tuần Y: [Task]

## 8. Test plan
- Unit: [...]
- Integration: [...]
- Manual: [...]

## 9. Open Questions
- [Q1]
- [Q2]
```

**System files PALDARK cần có (Sẽ làm trong PR sau):**
```
Documents/PALDARK/Systems/
├── 01-RaidLoop.md
├── 02-HubPersistence.md
├── 03-PalAI.md
├── 04-Combat.md
├── 05-Inventory.md
├── 06-Squad.md
├── 07-World.md
├── 08-Backend.md
```

### Phase 7 — Create Story (User Story / Ticket)

**Input:** System docs.
**Output:** US ticket trong `Documents/<Game>/Stories/`.

**US Template:**

```markdown
# US-XXX: [Title]

## As a [role]
## I want to [goal]
## So that [reason]

## Acceptance Criteria
- [ ] AC1
- [ ] AC2

## Definition of Done
- [ ] Code merged
- [ ] Test pass
- [ ] Demo screenshot/video
- [ ] Doc cập nhật

## Pillar
- Primary: P[X]
- Game Pillar: [Pillar 1/2/3]

## Estimate
[X tuần / X ngày]

## Dependency
- US-YYY (must finish first)

## Files affected (sketch)
- [path/file.h]
- [path/file.cpp]
```

**US PALDARK đầu tiên (sketch):**

```
US-001: Là Tamer, tôi drop xuống Dark Zone cùng 3 đồng đội
US-002: Pal companion phải theo tôi đúng khoảng cách
US-003: Tôi shoot dummy → dummy chết với damage chain
US-004: Tôi pick up item → item vào inventory với weight
US-005: Pal switch giữa Idle ↔ Follow ↔ Investigate dựa trên player distance
...
```

(Sẽ generate đầy đủ trong PR sau khi system docs done.)

---

## Áp dụng Workflow cho 15 Project đã có

> User yêu cầu: coi cả 15 source (12 course + 3 project) như game, áp Donchitos phân tích.

Mỗi source → áp 7-phase backward (vì game/course đã có code, ta reverse-engineer):

| Phase | Cho một course/project đã có |
|-------|-------------------------------|
| 1. Brainstorm | High-concept (đọc README + GDD nếu có) |
| 2. Decompose | Map module → verb chính |
| 3. Define | GDD reverse (extract từ code) |
| 4. Pillars | Suy ra 3 pillar (gameplay analysis) |
| 5. Architecture | ADR reverse (decisions implicit trong code) |
| 6. Systems | Per-system markdown |
| 7. Story | (skip — đã release/finish) |

**Đầu ra:** 15 file `Documents/GameDesign/<idx>-<name>_GDD.md` — coi mỗi project là game, áp 7-phase.

> **PR tiếp theo (4b):** thực hiện 15 GDD này. PR này (4a) chỉ thiết lập framework.

---

## Áp dụng Workflow cho Course Mới (UE5 Unified Course)

> User yêu cầu: tạo course mới theo 18 Core Pillar, lấy tài nguyên từ 15 nguồn.

**Course structure:**

```
Documents/UE5_Course/
├── 00-Index.md                                  ← Course outline
├── P01_CPP_Build/
│   ├── 01-Lesson_Module_Setup.md
│   ├── 02-Lesson_UCLASS_USTRUCT.md
│   ├── 03-Lesson_Smart_Pointers.md
│   └── 99-Pattern_Library.md                    ← Copy-paste patterns
├── P02_Core_Framework/
│   ├── 01-Lesson_GameMode.md
│   ├── 02-Lesson_GameState.md
│   ├── ...
├── ...
├── P18_Backend_LiveOps/
│   ├── ...
└── Capstone/
    └── 00-PALDARK_Capstone.md                   ← Capstone là làm PALDARK
```

**Mỗi lesson template:**

```markdown
# Lesson [N]: [Title]

## 1. Mục tiêu học (5 phút đọc)
[3 mục tiêu cụ thể]

## 2. Concept (10 phút đọc)
[Lý thuyết]

## 3. Code Pattern (10 phút copy)
[Code snippet cụ thể từ 15 nguồn]
- Source: [01] Tom `SActionComponent.h:23-45`

## 4. Bài tập (30 phút làm)
[Task cụ thể trong PALDARK base]

## 5. Self-check
- [ ] [Câu hỏi 1]
- [ ] [Câu hỏi 2]

## 6. Tham chiếu sâu
- Source 🥇 — (điền đường dẫn vào tài liệu / repo gốc khi instantiate template).
- Source 🥈 — (điền đường dẫn vào tài liệu / repo gốc khi instantiate template).
```

> **PR tiếp theo (4c):** thực hiện 18 pillar × 4–6 lesson = ~80–100 lesson file. PR này (4a) chỉ thiết lập framework.

---

## Cách Devin AI Hỗ Trợ Workflow Này

User hỏi: *"Devin AI có thể tự động xây dựng được dàn ý vững chắc, sau đó từng bước đắp lên tạo nên từng thớ da, thớ thịt..."*

Devin có thể:
1. **Sinh dàn ý:** đọc 15 source + áp Donchitos → generate 7-phase doc tự động.
2. **Sinh ADR:** đọc code base + extract decision implicit → ghi ADR template.
3. **Sinh System docs:** đọc system trong source ref → generate per-system markdown.
4. **Sinh US:** từ system docs → generate user story tickets.
5. **Sinh lesson:** từ pillar + source ref → generate lesson markdown.
6. **Update roadmap:** weekly check, generate devlog từ git history.
7. **Code stub:** từ US ticket → generate C++ stub class.

**Quy trình recommended với Devin:**

```
1. User: "tôi muốn làm game X, vibe Y"
2. Devin: viết 00-VISION.md (Step 1-2 Vision Reverse)
   → User review/edit Scene + Pillar
3. Devin: viết 01-GameDesign.md (Phase 1-3 Donchitos)
   → User review GDD
4. Devin: viết ADR cho 8 quyết định lớn
   → User review/decide
5. Devin: viết 02-Pillar_Coverage.md (Phase 4 + map UE5 pillar)
   → User review
6. Devin: viết 03-Roadmap_1_Year.md
   → User review
7. Devin: viết 04-Resource_Map.md
   → User review
8. Devin: viết System docs (Phase 6) per-system
9. Devin: viết US tickets (Phase 7) per-system
10. Devin: code stub theo US — user code, Devin review/fix CI
11. Weekly: Devin tự generate devlog + update roadmap status
```

**User chỉ cần:**
- Review Vision + Pillar (quyết định cảm xúc).
- Review ADR (quyết định kỹ thuật lớn).
- Code thực sự (mỗi ngày 1-2 task).
- Test + give feedback.

**Phần plan + structure 80% Devin lo. Người tập trung vào tay nghề + cảm xúc.**

---

## Sự Khác Biệt giữa Donchitos & Vision Reverse Engineering

| Donchitos | Vision Reverse |
|-----------|----------------|
| Top-down: từ high-concept → code | Bottom-up: từ Scene → System |
| 7 phase tuần tự | Quy tắc "Cảnh trước, code sau" |
| 49 agent + 73 skill | 7 step ngắn |
| Phù hợp team studio lớn | Phù hợp solo dev |
| Document đậm + ADR đậm | Document gọn + emotion-focused |

**Kết hợp:**
- Bắt đầu bằng **Vision Reverse** (Step 1-2: Scene + Triggers).
- Sau khi có System list, chuyển sang **Donchitos Phase 3-7** (GDD + ADR + Systems + Stories).
- Donchitos là **tool**, Vision Reverse là **filter**.

---

## Câu Trích Nguồn Cảm Hứng

> *"Process serves the game, the game doesn't serve the process."*  
> — Studio adage

> *"Bạn không cần Donchitos để bắt đầu. Bạn cần Vision để bắt đầu. Donchitos đến khi Vision đã rõ."*  
> — Quy tắc áp dụng

---

## Tài Liệu Liên Quan

- [../Donchitos_GameStudios_Framework.md](../Donchitos_GameStudios_Framework.md) — Framework tóm tắt.
- [00-Vision_Reverse_Engineering.md](00-Vision_Reverse_Engineering.md) — Bước 1.
- [../PALDARK/01-GameDesign.md](../PALDARK/01-GameDesign.md) — Ví dụ Donchitos áp dụng.
