# PALDARK — Resource Map (Pillar × 15 Nguồn)

> Mỗi pillar trong PALDARK cần học từ nguồn cụ thể nào trong 15 nguồn đã có.
> Đây là **bản đồ phụ thuộc tri thức**: trước khi viết code pillar X, mở 2–3 nguồn này đọc.
>
> Áp dụng kết hợp với [03-Roadmap_1_Year.md](03-Roadmap_1_Year.md) — Roadmap nói **khi nào**, Resource Map nói **đọc cái gì**.

---

## Notation

- 🥇 = Nguồn chính (must-read trước khi code).
- 🥈 = Nguồn phụ (cross-reference).
- 🥉 = Tham chiếu phụ (đọc nếu có thời gian).
- ⚙️ = Pattern code có thể copy thẳng vào PALDARK (xem ghi chú).

---

## Pillar × Resource Matrix

| Pillar | 🥇 Nguồn chính | 🥈 Phụ | 🥉 Phụ |
|--------|---------------|--------|--------|
| **P1 C++/Build** | [12] Pro UE4 Ch1-4 | [08] Build.cs | [01] Tom module setup |
| **P2 Core Framework** | [07] MP Crash S5 | [01] ActionRoguelike | [15] Lyra RPG class layout |
| **P3 Composition** | [01] Tom Action+Attribute Comp | [14] Lyra Modular Gameplay | [09] Item Fragment |
| **P4 Enhanced Input** | [15] Lyra RPG Input Config | [11] Aura InputConfigDataAsset | [01] Tom basic |
| **P5 Animation** | [15] Lyra Locomotion | [10] MP Shooter recoil | [01] Tom AnimNotify |
| **P6 Replication** | [10] MP Shooter (Lag Comp) | [07] MP Crash (basics) | [13] Crunch GAS replicate |
| **P7 Dedicated Server** | [08] AWS GameLift full | [13] Crunch Coordinator | [07] Travel + Steam |
| **P8 GAS** | [11] Aura RPG (depth) | [05] GAS Crash (basics) | [13] Crunch (optimization) |
| **P9 AI** | **[04] ReadyOrNot study** (Activity FSM + Utility) | [01] Tom BT+EQS | [15] Lyra StateTree |
| **P10 UI** | [11] Aura HUD MVVM | [14] Lyra CommonUI | [15] Lyra Indicator Mgr |
| **P11 Inventory** | [09] Inventory full | [15] Lyra Quick Bar | [11] Aura inv |
| **P12 Data-driven** | [01] Tom DataAsset+AssetMgr | [11] Aura Curve Table | [15] Lyra Experience |
| **P13 Save/Load** | [01] Tom SaveGame | [11] Aura checkpoint | — |
| **P14 AssetMgr/Async** | [01] Tom async | [14] Lyra GameFeature | — |
| **P15 Performance** | **[04] ReadyOrNot study** (Significance) | [01] Tom Aggregate Tick | [14] Lyra Perf |
| **P16 Math/Physics/Audio** | [12] Pro UE4 math + audio | [04] ReadyOrNot FMOD | — |
| **P17 Lyra** | [14] Exploring Lyra full | [15] Lyra RPG application | — |
| **P18 Backend** | [08] AWS GameLift Lambda/Cognito/DynamoDB | [13] Crunch Coordinator + Container | [04] ReadyOrNot mod.io/Steam |

---

## Phân Tích Per-Pillar — Trang chi tiết

### P1 — C++ / Build Foundation

**Đọc trước khi code Tuần 1–2:**

| Source | File | Chương | Note |
|--------|------|--------|------|
| 🥇 [12] Pro UE4 | Chapter 1–4 | Editor, VS, Source Build, Git | Foundation tuyệt đối |
| 🥈 [08] DedicatedServers | `.Build.cs` files | — | Cách setup server target |
| 🥉 [01] Tom ActionRoguelike | `ActionRoguelike.Build.cs` | — | Module config gọn |

**Pattern copy vào PALDARK ⚙️:**
- `ActionRoguelike.Build.cs` setup → `Paldark.Build.cs`.
- Module split: pattern của [04] ReadyOrNot 14 module → PALDARK 7 module.

---

### P2 — Core Class Framework

**Đọc trước khi code Tuần 3–4:**

| Source | Vị trí | Note |
|--------|--------|------|
| 🥇 [07] MP Crash | Section 5 (Class Framework) | Bảng authority |
| 🥈 [01] Tom | `SGameModeBase.h/cpp` | GameMode minimal |
| 🥉 [15] Lyra RPG | `ALyraRPGCharacter.h` | Lyra class layout |

**Pattern ⚙️:**
- `APaldarkGameMode_Raid` ← lấy từ Lyra GameMode + custom raid logic.
- `UPaldarkGameInstance` ← từ [01] Tom SGameInstance pattern.

---

### P3 — Composition

**Đọc trước khi code Tuần 3 (Component setup):**

| Source | File | Note |
|--------|------|------|
| 🥇 [01] Tom | `SActionComponent.h/cpp` + `SAttributeComponent.h/cpp` | **Đọc kỹ — pattern PALDARK copy gần như nguyên** |
| 🥈 [14] Lyra | Modular Gameplay + GameFrameworkComponentManager | Dynamic add component |
| 🥉 [09] Inventory | Item Fragments | Composition cho item |

**Pattern ⚙️:**
- `UPaldarkPalActivityComponent` ← từ Tom `SActionComponent` (Action ≈ Activity).
- Modular Gameplay: thêm Component dynamic từ Lyra Experience.

---

### P4 — Enhanced Input

**Đọc trước Tuần 1 cuối:**

| Source | Vị trí | Note |
|--------|--------|------|
| 🥇 [15] Lyra RPG | `ULyraInputConfig` + IMC files | Layout PALDARK theo đây |
| 🥈 [11] Aura | `UInputConfigDataAsset` | Pattern InputTag → GA |
| 🥉 [01] Tom | `SCharacter::SetupPlayerInputComponent` | Basic Enhanced Input |

**Pattern ⚙️:**
- `UPaldarkInputConfig` ← copy struct từ [15] Lyra.
- `IA_*` UAsset list trong DataAsset.

---

### P5 — Animation

**Đọc trước Tuần 11 trở đi:**

| Source | Vị trí | Note |
|--------|--------|------|
| 🥇 [15] Lyra RPG | Locomotion System (Distance Matching) | Player + Pal đều dùng |
| 🥈 [10] MP Shooter | Section Animation + Recoil | Combat anim |
| 🥉 [01] Tom | `SAnimNotify_*` | AnimNotify pattern |

**Pattern ⚙️:**
- Lyra Locomotion AnimBP → fork cho Player + Pal (Pal có Activity Driver thêm).

---

### P6 — Replication

**Đọc trước Tuần 14–17:**

| Source | Vị trí | Note |
|--------|--------|------|
| 🥇 [10] MP Shooter | **Section "Lag Compensation"** (Server-Side Rewind) | **Quan trọng nhất — copy nguyên** |
| 🥈 [07] MP Crash | Section 2–4 (Actor Replication, Remote Functions) | Lý thuyết base |
| 🥉 [13] Crunch | GAS net update freq optimization | Pal mass replicate |

**Pattern ⚙️:**
- Frame package buffer cho hitbox → `UPaldarkLagCompComponent` copy nguyên từ [10].

---

### P7 — Dedicated Server

**Đọc trước Tuần 14–15 + Tuần 42–43:**

| Source | Vị trí | Note |
|--------|--------|------|
| 🥇 [08] AWS GameLift | **Toàn bộ 16 chương** | Production stack |
| 🥈 [13] Crunch | Server Coordinator pattern | Multi-fleet |
| 🥉 [07] MP Crash | Section 6 (Travel + Steam) | Listen → dedicated |

**Pattern ⚙️:**
- Anywhere Fleet đầu (dev), Production Fleet (beta).
- Lambda + DynamoDB coordinator từ [13].

---

### P8 — GAS

**Đọc trước Tuần 7–10 + 18–19:**

| Source | Vị trí | Note |
|--------|--------|------|
| 🥇 [11] Aura RPG | **Full 33 chương** (đặc biệt damage chain) | **Source #1 GAS** |
| 🥈 [05] GAS Crash | Toàn bộ 8 chương | Basics nhanh |
| 🥉 [13] Crunch | GAS combo + level-up | Optimize |

**Pattern ⚙️:**
- `UPaldarkAttributeSet` ← copy template từ [11] Aura `UAuraAttributeSet`.
- Damage Execution Calc từ [11] → port nguyên.
- ASC mount on Pawn pattern.

---

### P9 — AI ⭐ (Pillar quan trọng nhất cho PALDARK)

**Đọc trước Tuần 5–6 + 18–19 + 29–30:**

| Source | Vị trí | Note |
|--------|--------|------|
| 🥇 **[04] ReadyOrNot study** | `Source/.../Info/Activities/` + `AI/AIActionData.*` + `Senses/*` + `Octree/*` | **Source #1, đọc 2–3 vòng** |
| 🥈 [01] Tom | BT + EQS section | BT base nếu fallback |
| 🥉 [15] Lyra RPG | StateTree section | Modern AI alternative |

**Pattern ⚙️ (port từ RoN):**
- `UBaseActivity` (RoN) → `UPaldarkBaseActivity`.
- `UActivityFiniteStateMachine` (RoN) → `UPaldarkPalActivityFSM`.
- `UAIActionData` (RoN) → `UPaldarkAIAction`.
- `UReadyOrNotAISense_Sight` → tham chiếu, viết `UPaldarkAISense_Scent`.
- `ThreatAwarenessSubsystem` → port nguyên thành `UPaldarkThreatSubsystem`.

> ⚠️ **Đây là phần khó nhất.** Dự trù 4–6 tuần thuần. Đừng vội.

---

### P10 — UI

**Đọc trước Tuần 22–25:**

| Source | Vị trí | Note |
|--------|--------|------|
| 🥇 [11] Aura | Widget Controller MVC sections | **MVVM pattern** — tránh god-widget RoN |
| 🥈 [14] Lyra | CommonUI + ActivatableWidget | Menu + FrontEnd |
| 🥉 [15] Lyra RPG | Indicator Manager + Quick Bar | World-space marker |

**Pattern ⚙️:**
- `UPaldarkWidgetController` ← copy pattern từ [11] Aura.
- Radial wheel: tham chiếu RoN `SwatCommandWidget` nhưng MVVM.

---

### P11 — Inventory

**Đọc trước Tuần 11–12 + 37–38:**

| Source | Vị trí | Note |
|--------|--------|------|
| 🥇 [09] Inventory | **Full 16 chương** | Fragment pattern |
| 🥈 [15] Lyra RPG | Quick Bar inventory | Hotbar UI |
| 🥉 [11] Aura | Inventory đơn giản | Reference |

**Pattern ⚙️:**
- `UFragment_*` ← copy struct từ [09].
- Composite container ← từ [09] Composite Pattern chapter.

---

### P12 — Data-Driven

**Đọc trước Tuần 27–28:**

| Source | Vị trí | Note |
|--------|--------|------|
| 🥇 [01] Tom | DataAsset + AssetManager chapters | Pattern PrimaryDataAsset |
| 🥈 [11] Aura | Curve Table for XP | Curve pattern |
| 🥉 [15] Lyra | Experience DataAsset | Map definition |

**Pattern ⚙️:**
- `UPaldarkPalDefinition : UPrimaryDataAsset` ← copy struct [01] Tom.
- Lyra Experience cho map.

---

### P13 — Save / Load

**Đọc trước Tuần 44–45:**

| Source | Vị trí | Note |
|--------|--------|------|
| 🥇 [01] Tom | SaveGame section | Archive pattern |
| 🥈 [11] Aura | Checkpoint section | Per-zone save |

**Pattern ⚙️:**
- `USPaldarkSaveGame` ← từ Tom.
- Note: PALDARK chủ yếu **server-side persist** (DynamoDB), local SaveGame chỉ cho settings.

---

### P14 — Asset Manager / Async

**Đọc trước Tuần 27–28:**

| Source | Vị trí | Note |
|--------|--------|------|
| 🥇 [01] Tom | Async DataAsset, Async UI Icon | Pattern async |
| 🥈 [14] Lyra | Game Features Plugin section | Modular async |

**Pattern ⚙️:**
- `FStreamableManager::RequestAsyncLoad` từ Tom.
- GameFeature Plugin từ [14].

---

### P15 — Performance

**Đọc trước Tuần 48–49 (nhưng setup pillar từ Tuần 18 trở đi):**

| Source | Vị trí | Note |
|--------|--------|------|
| 🥇 **[04] ReadyOrNot study** | `ReadyOrNotSignificanceManager.h/cpp` + ThreatOctree | Production-grade |
| 🥈 [01] Tom | Aggregate Tick + Object Pool | Pooling pattern |
| 🥉 [14] Lyra | Performance section | General tips |

**Pattern ⚙️:**
- SignificanceManager subclass → `UPaldarkSignificanceManager`.
- Object Pool pattern từ Tom cho bullet/loot.

---

### P16 — Math / Physics / Audio

**Đọc trước Tuần 46–47 (Audio) + spread Q3 (Math when needed):**

| Source | Vị trí | Note |
|--------|--------|------|
| 🥇 [12] Pro UE4 | Math + Audio chapters (Ch5-10) | Foundation |
| 🥈 [04] ReadyOrNot | `FMODAudioPropagationComponent` | FMOD integration |

**Pattern ⚙️:**
- FMOD Propagation Component từ RoN study → wire vào PALDARK.

---

### P17 — Lyra

**Đọc trước Tuần 1–10 (backbone):**

| Source | Vị trí | Note |
|--------|--------|------|
| 🥇 [14] Exploring Lyra | **Full 12 sections** | Deep dive |
| 🥈 [15] Lyra RPG | Toàn bộ | Application example |

**Pattern ⚙️:**
- ExperienceDefinition + ActionSet + GameFeature → PALDARK adopt nguyên.

> ⚠️ **Lyra learning curve dốc.** Đầu tư 2–3 tuần thuần Lyra trước khi extend.

---

### P18 — Backend

**Đọc trước Tuần 42–43:**

| Source | Vị trí | Note |
|--------|--------|------|
| 🥇 [08] AWS GameLift | **Full 16 chương** | Production stack |
| 🥈 [13] Crunch | Server Coordinator | Multi-fleet |
| 🥉 [04] ReadyOrNot | mod.io / Steam / EOS integration | Plugin pattern |

**Pattern ⚙️:**
- Lambda functions từ [08] node.js code → port vào PALDARK Lambda.
- Cognito user pool setup từ [08].

---

## Reading Schedule (Học chương → Code chương)

| Tuần | Đọc chương | Code |
|------|------------|------|
| 1 | [14] Lyra Ch 1-3 | Setup Paldark module |
| 2 | [14] Lyra Ch 4-7 + [04] kien_truc | Lyra Experience custom |
| 3 | [01] Tom Action+Attr Comp | Pawn skeleton |
| 4 | [01] Tom Action chi tiết | Component empty |
| 5 | **[04] RoN Activity Chap (kien_truc Phase 5)** | Activity base class |
| 6 | [04] BaseActivity.cpp đọc cụ thể | Activity FSM Pal Follow |
| 7 | [05] GAS Crash Ch 1-3 + [11] Aura Ch 1-3 | GAS setup |
| 8 | [11] Aura damage chain Ch 6-9 | AttributeSet |
| 9 | [11] Aura GE chain Ch 10-13 | Damage |
| 10 | [10] MP Shooter Section Lag Comp | Hitscan lag comp |
| 11 | [09] Inventory Ch 1-8 | Fragment system |
| 12 | [09] Inventory Ch 9-16 | UI inventory |
| 13 | Q1 Milestone — đọc đối chiếu | Bug bash |
| 14–15 | [07] MP Crash Section 5-6 + [08] AWS Ch 1-5 | Dedicated test |
| 16–17 | [10] MP Shooter Lag Comp đọc lần 2 | Server rewind |
| 18–19 | [04] RoN Combat Activity | Pal Combat Activity |
| 20–21 | [04] RoN Hostile Activity + Custom Sense | Echo Pal |
| 22–23 | [11] Aura Widget Controller + [14] CommonUI | Radial UI |
| 24–25 | [08] AWS extract flow design | Extract |
| 26 | Q2 Milestone | Bug bash |
| 27–28 | [01] Tom AssetMgr + [14] GameFeature | Async load |
| 29–30 | [15] Lyra RPG Pal Pattern | 5 Pal more |
| 31–32 | Level design self-study | Map 2 |
| 33–34 | Level design self-study | Map 3 |
| 35–36 | [11] Aura Capture / Bond design | Bond + tame |
| 37–38 | [09] Composite re-read | Loot economy |
| 39 | Q3 Milestone | Bug bash |
| 40–41 | [15] Lyra RPG Hub-style + [04] Hub town inspiration | Hub setup |
| 42–43 | [08] AWS Lambda+Cognito+DynamoDB | Backend |
| 44–45 | [01] Tom SaveGame + [11] Aura checkpoint | Save/breed |
| 46–47 | [04] FMOD propagation | Audio |
| 48–49 | [04] Significance + [01] Tom Aggregate | Perf pass |
| 50 | Polish docs | UI polish |
| 51 | — | Closed beta build |
| 52 | — | Beta launch |

---

## Câu hỏi tự kiểm tra (mỗi tuần)

1. **Tuần này tôi đọc nguồn 🥇 chưa?**
2. **Tôi có pattern code nào copy thẳng từ nguồn ⚙️ không?**
3. **Outcome cuối tuần (build chạy được) đạt chưa?**
4. **Có chệch khỏi 3 game pillar của VISION không?**
5. **Pillar % của roadmap tuần này có hợp lệ không?**

Nếu 1 câu trả lời "không" → flag risk, có thể phải shift roadmap.
