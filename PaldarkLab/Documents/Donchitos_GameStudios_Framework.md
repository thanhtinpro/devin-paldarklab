# Donchitos — Claude-Code Game Studios Framework (tóm tắt)

> Nguồn: https://github.com/Donchitos/Claude-Code-Game-Studios
> Cập nhật khi đọc: 14k stars, 49 agents, 73 skills, 12 hooks, 11 rules, 41 templates.
> Đây là **khung phương pháp luận** áp dụng *trước* khi viết bất kỳ dòng code nào của một dự án game khi làm cùng AI. File này tóm tắt các phần dùng được trong context của repo Palworld / PUBG / ReadyOrNot.

---

## 1. Triết lý cốt lõi

> "Building a game solo with AI is powerful — but a single chat session has no structure."

- **Studio mô phỏng:** 1 Claude Code session chia thành nhiều "agent" có vai trò cố định (director, lead, specialist).
- **Người dùng vẫn quyết:** mọi agent đi theo protocol `Ask → Options → Decision → Draft → Approval`. AI không tự ý ghi file.
- **Quality gates:** mỗi domain có hook/rule chặn commit khi vi phạm convention.
- **Domain boundary:** specialist không được sửa file ngoài domain của mình nếu không có ủy quyền explicit.
- **Workflow phase:** mọi feature đi qua chuỗi *Design → Architecture → Story → Dev → Review → QA → Release*. Không có nhánh tắt.

---

## 2. Cấu trúc Studio (apply cho dự án solo + AI)

```
Tier 1 — Directors (định hướng, không code)
  creative-director       (vision, pillar)
  technical-director      (architecture, tech-debt)
  producer                (scope, milestone)

Tier 2 — Department Leads (chủ domain)
  game-designer           (GDD, balance)
  lead-programmer         (code style, review)
  art-director            (visual)
  audio-director          (audio)
  narrative-director      (story)
  qa-lead                 (test plan)
  release-manager
  localization-lead

Tier 3 — Specialists (làm việc hand-on)
  gameplay-programmer  engine-programmer  ai-programmer
  network-programmer   tools-programmer   ui-programmer
  systems-designer     level-designer     economy-designer
  technical-artist     sound-designer     writer
  world-builder        ux-designer        prototyper
  performance-analyst  devops-engineer    analytics-engineer
  security-engineer    qa-tester          accessibility-specialist
  live-ops-designer    community-manager
```

**Engine specialist riêng:** với UE5 dùng `unreal-specialist` + sub-specialists: GAS, Blueprints, Replication, UMG/CommonUI.

> Trong repo này (Palworld / PUBG / ReadyOrNot — đều là UE5) ta luôn dùng `unreal-specialist`.

---

## 3. Workflow 7 phase (lấy từ `workflow-catalog.yaml`)

| Phase | Skill chính | Output |
|-------|-------------|--------|
| **1. Discover / Concept** | `/brainstorm` `/start` | `design/gdd/game-concept.md` |
| **2. Decompose Systems** | `/map-systems` | `design/gdd/systems-index.md` (ưu tiên, dependency, phase) |
| **3. Design System GDD** | `/design-system <name>` | `design/gdd/<system>.md` đầy đủ section |
| **4. Architecture** | `/create-architecture` `/architecture-decision` `/create-control-manifest` | ADR + tech stack |
| **5. Stories / Sprint** | `/create-epics` `/create-stories` `/sprint-plan` `/dev-story` | Epic → Story → Code |
| **6. Review / QA** | `/design-review` `/code-review` `/balance-check` `/perf-profile` `/tech-debt` `/qa-plan` `/smoke-check` `/soak-test` | Review report |
| **7. Release** | `/release-checklist` `/launch-checklist` `/changelog` `/patch-notes` `/hotfix` | Build |

**Team orchestration** (gộp nhiều specialist): `/team-combat`, `/team-narrative`, `/team-ui`, `/team-release`, `/team-polish`, `/team-audio`, `/team-level`, `/team-live-ops`, `/team-qa`.

---

## 4. Cấu trúc thư mục chuẩn (Donchitos đề xuất)

```
CLAUDE.md                           # Master config (engine, rules)
.claude/
  agents/                           # 49 agent định nghĩa (md + YAML)
  skills/                           # 73 slash command
  hooks/                            # 12 hook script
  rules/                            # 11 coding rule path-scoped
  docs/templates/                   # 41 template (GDD, UX, ADR, sprint…)
src/                                # Code game
assets/                             # Art / audio / VFX / shader / data
design/                             # GDD, narrative, level
docs/                               # Tech doc + ADR
tests/                              # Unit / integration / perf / playtest
tools/                              # Build & pipeline
prototypes/                         # Throwaway (cô lập khỏi src/)
production/                         # Sprint, milestone, release tracking
```

**Mapping cho repo hiện tại:**

| Donchitos | Tương đương UE/Palworld_Base |
|-----------|------------------------------|
| `src/` | `Source/Palworld_Base/` |
| `assets/` | `Content/` (UE Content Browser) |
| `design/gdd/` | `Documents/` (đang dùng) |
| `docs/` | `Documents/` + comments |
| `tests/` | `Source/Palworld_BaseTests/` (chưa có) |
| `production/` | Không có — cần tạo nếu áp dụng |

---

## 5. Pattern đáng học để áp dụng

### 5.1 Game Pillars trước khi viết code
Trước khi `/map-systems`, mọi project phải có `design/gdd/game-pillars.md`. Đây là **3–5 nguyên tắc bất di bất dịch** mọi quyết định phải đối chiếu.

Ví dụ Palworld pillars (em đề xuất):
1. *Capture + work* (Pal có thể chiến đấu và lao động — không chỉ chiến đấu).
2. *Survival cycle* (đói/lạnh/SAN).
3. *Co-op friendly* (mọi cơ chế play được trong multiplayer 4–8 người).

### 5.2 `/map-systems` — phân rã hệ thống
Đầu ra là 1 bảng có:
- Tên hệ thống
- Layer (Foundation → Core → Entity → System → Presentation)
- Status (Not started / Drafted / Design done / In progress / Done)
- Priority (P0 / P1 / P2)
- Dependencies (system nào phải xong trước)

> Đây chính là cách em sẽ làm bảng "Gap Analysis" trong Palworld_Assessment.md.

### 5.3 Per-system GDD template (rút gọn)
1. Overview
2. Pillars alignment (system này phục vụ pillar nào?)
3. Functional spec (input → process → output)
4. Data model
5. Dependencies
6. Edge cases / failure modes
7. Test plan
8. Open questions

### 5.4 Architecture Decision Record (ADR)
Khi quyết định design quan trọng:
```
Title:
Status: Proposed / Accepted / Superseded
Context:
Decision:
Consequences (positive + negative):
Alternatives considered:
```

### 5.5 Hook validation tự động
- `validate-commit.sh`: chặn commit có hardcoded magic number, TODO sai format, JSON invalid, GDD thiếu section.
- `validate-push.sh`: cảnh báo push vào protected branch.
- `validate-assets.sh`: convention naming + JSON valid trong `assets/`.
- `session-start.sh`: hiện branch + commit gần.
- `detect-gaps.sh`: nhắc nhở khi project chưa có concept/GDD.

> Áp dụng tương đương vào repo UE5: dùng `.editorconfig`, `clang-format`, `commitlint`, Unreal Header Tool warnings.

---

## 6. Cách em sẽ apply framework này cho 3 dự án

| Project | Có sẵn | Cần bổ sung theo framework |
|---------|--------|----------------------------|
| **Palworld** | Code skeleton, 1 spec PDF | Game pillars, systems-index, per-system GDD, ADR cho GAS adoption, sprint plan |
| **PUBG** | Code đầy đủ Manager pattern, 1 spec PDF | Game pillars, ADR cho Manager-vs-Subsystem, GAS migration plan |
| **ReadyOrNot** | 298k LOC, kien_truc doc | Architecture review (god-object), tech-debt audit, modular split proposal |

Chi tiết trong từng file `Documents/Projects/<name>_Assessment.md`.

---

## 7. Mapping skill Donchitos → output tài liệu trong repo này

| Donchitos skill | File em đã/đang tạo |
|-----------------|--------------------|
| `/brainstorm` | `Documents/Projects/<name>_Assessment.md` §Concept |
| `/map-systems` | `Documents/Projects/<name>_Assessment.md` §Systems Map |
| `/design-system` | (tương lai) `Documents/Projects/<name>/<system>.md` |
| `/create-architecture` | `Documents/UE5_Core_Pillars.md` |
| `/architecture-review` | `Documents/Projects/<name>_Assessment.md` §Architectural Issues |
| `/tech-debt` | `Documents/Projects/<name>_Assessment.md` §Tech Debt |
| `/create-epics` + `/create-stories` | `Documents/Refactor_Plan.md` (đã merge cho Palworld) |

---

## 8. Lưu ý khi apply

1. **Đừng cài Donchitos vào repo này** (49 agent + 73 skill là cho Claude Code workflow, không phải code game). Chỉ học **phương pháp luận**.
2. **Mỗi project nên có CLAUDE.md riêng** chứa pillar + engine + coding standard, ngay trong root của project con (`02.Palworld/CLAUDE.md` thay vì gốc repo).
3. **Tài liệu là first-class citizen** — viết GDD trước, code sau. Cả 3 project hiện tại đều **không có GDD đủ chuẩn**:
   - Palworld có spec PDF tiếng Hàn 67 trang — chưa phân rã thành system GDD.
   - PUBG chỉ có 1 spec PDF.
   - ReadyOrNot có doc kiến trúc tiếng Việt, nhưng không có pillar.
4. **Áp dụng cấp độ phù hợp:** dự án solo dùng review `solo`, team nhỏ dùng `lean`, team lớn dùng `full`. Khi review chính `Documents/` này, em dùng `lean` (phù hợp 1 user + 1 AI).
