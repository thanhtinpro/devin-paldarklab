# 04 — Ready or Not (Donchitos Reverse GDD)

> Source: VOID Interactive shipped AAA tactical FPS. Repo có code C++ và project assets — coverage 18 pillar **~73%** (cao nhất trong 3 project).
>
> Đây là **playbook chính của PALDARK** cho AI Activity FSM + Significance + FMOD Propagation.

---

## Identity Card

| Field | Value |
|-------|-------|
| 📦 Type | Shipped AAA |
| Genre | Tactical FPS co-op SWAT |
| Engine | UE5 |
| Đóng góp PALDARK | Activity FSM + ThreatOctree + Significance + FMOD Propagation + Indicator Manager + 42 DataAsset pattern |

---

## Phase 1 — Brainstorm (reverse)

### Implicit pitch
> *"Bạn là leader SWAT. Vào 1 căn hộ. Quyết định mỗi cửa: breach? sneak? gas? Mỗi tic tab có thể giết bạn hoặc giết civilian."*

### Cảm xúc cốt lõi
- **Tic tac căng** — không có heart-rate cao mass mass nhưng có heart-rate kéo dài.
- **Đạo đức** — civilian innocent.
- **Đồng đội** — 4 AI hoặc co-op.
- **Tự hào kỹ năng** — không phải reflex mà là decision.

### Vì sao thành công
- Niche khan (tactical FPS shipped 2023).
- AAA polish.
- Mode mod community.

---

## Phase 2 — Decompose

### Verb cốt lõi
- Plan (pre-mission map)
- Stack (cửa)
- Breach (door)
- Clear (room)
- Command (4 AI: stack/cover/push/grenade)
- Arrest / Restrain
- Report (call out civilian)

### System hierarchy

```
┌─────────────────────────────────────┐
│ A* GameMode + GameState              │
│ Activity FSM Subsystem               │
│ Threat Awareness Subsystem (Octree)  │
│ Significance Manager (custom)        │
└──────────────┬──────────────────────┘
               │
   ┌───────────┼─────────────┬─────────────┬─────────────────┐
   ▼           ▼             ▼             ▼                 ▼
SwatChar    SuspectAI    CivilianAI    PoliceAI         DoorActor
 66 Comp    Activity      Activity      Activity         Breachable
 god-debt   FSM           FSM           FSM              Statful

UI (180 widget)        ThreatOctree         FMOD AudioProp
SwatCommand            spatial query          per-room reverb
   5K LOC                                     occlusion
   (god-widget)
```

---

## Phase 3 — Define (GDD reverse)

### Game Loop
1. Briefing — chọn map, loadout, AI loadout.
2. Drop xuống building.
3. Plan approach.
4. Stack at door.
5. Breach/sneak/peek.
6. Clear room (engage/arrest/avoid civ).
7. Move to next room until cleared.
8. Mission complete or abort.

### Player Character (`ASwatCharacter`)
- 66 Component đính kèm.
- ❗ **Logic vẫn nằm trong character** (god-character anti-pattern).
- 368KB single `.cpp` file.

### Squad command
- Radial wheel `SwatCommandWidget` (5,000 LOC, god-widget).
- 4 AI execute order via Activity FSM.

### AI (Activity FSM pattern)
- `UBaseActivity` parent class.
- Mỗi loại AI có activity hierarchy:
  - Suspect: Patrol, Stalk, Ambush, Surrender, Engage.
  - Civilian: Idle, Flee, Comply, Panic.
  - Police: Stack, Cover, Push, Wait.
- Mỗi Activity có:
  - `CanActivate()` — consideration return true/false.
  - `Tick()` — logic per-frame.
  - `OnDeactivate()`.

### Threat Awareness (Octree)
- Spatial partition cho threat (gunshot, body, danger source).
- Query: "có threat trong bán kính X từ point P không?"
- Custom AISense: scent, sound, sight.

### Audio Propagation
- FMOD Studio + Custom Component.
- Tính occlusion per-room.
- Reverb / muffle theo wall material.

### Indicator Manager
- World-space marker UI (civilian alive, suspect down, door waypoint).

### DataAsset pattern (42 cái)
- Item, Level, AI loadout, Customization, etc. all DataAsset.

### Significance Manager (custom)
- Subclass `USignificanceManager` → `UReadyOrNotSignificanceManager`.
- Cull AI ticking ngoài N mét.
- LOD logic per-entity.

---

## Phase 4 — 3 Game Pillars (inferred)

### Pillar 1 — Tactical Patience
> *"Tốc độ giết. Bình tĩnh sống."*
- Không có run-and-gun.
- Stamina nhỏ.
- Aim drift khi mệt.

### Pillar 2 — Every Door is a Decision
> *"Mỗi cửa = 5 cách. Sai 1 = ăn đạn hoặc chết civ."*
- Stack vs Breach vs Sneak vs Stun vs Gas.
- Game design forces choice.

### Pillar 3 — Civilians Have Weight
> *"Bạn không xếp họ vào target list."*
- ROE (Rules of Engagement) checking.
- Penalty score nếu hurt civ.
- Civilian AI có Activity Flee + Panic.

---

## Phase 5 — Architecture Decisions (implicit)

### ADR-1: Activity FSM thay vì BT/StateTree
- **Quyết định:** Custom Activity FSM (UBaseActivity + UActivityFiniteStateMachine).
- **Lý do:** BT/StateTree không express được "Consideration scoring" + "Mid-execution Activity switch" theo cách RoN muốn.
- **Hệ quả:** AI cá thể, không swarm. Pattern reuse-able đa loại AI.
- **PALDARK adopt:** 🟢 **CORE — copy nguyên.** Đọc kỹ `BaseActivity.cpp` rồi port `UPaldarkBaseActivity`.

### ADR-2: Custom AISense (Scent/Sound/Sight)
- **Quyết định:** Override `UAISense_*` với behavior riêng.
- **PALDARK adopt:** 🟢 — `UPaldarkAISense_Scent` cho Pal scent tracking.

### ADR-3: ThreatOctree spatial query
- **Quyết định:** Spatial partition cho "threat awareness query".
- **Lý do:** O(log n) thay vì O(n) loop AI.
- **PALDARK adopt:** 🟢 — `UPaldarkThreatSubsystem` port nguyên.

### ADR-4: SignificanceManager subclass
- **Quyết định:** Subclass UE base, custom logic per-entity.
- **PALDARK adopt:** 🟢 — `UPaldarkSignificanceManager`.

### ADR-5: FMOD Studio integration + Propagation Component
- **Quyết định:** FMOD thay vì UE native (lúc đó MetaSound chưa mature).
- **Hệ quả:** $$$ license but quality cao.
- **PALDARK adopt:** 🟡 **ADR-006 cần quyết** — FMOD hay MetaSound.

### ADR-6: GAS KHÔNG dùng
- **Quyết định:** Skip GAS, custom AttributeSystem.
- **Lý do:** GAS chưa mature lúc bắt đầu dự án.
- **Hệ quả:** Tech debt — ability/effect chain custom.
- **PALDARK adopt:** 🔴 Skip — PALDARK dùng GAS từ đầu.

### ADR-7: 66-Component Character với god-logic
- **Quyết định:** Tách component nhưng để logic trong Character.
- **Hệ quả:** **Anti-pattern.** Component như "data holder" — Character vẫn 368KB.
- **PALDARK adopt:** 🔴 **AVOID.** Component phải own logic.

### ADR-8: SwatCommandWidget 5K LOC monolith
- **Quyết định:** UI command + state + render trong 1 widget.
- **Hệ quả:** **God-widget anti-pattern.**
- **PALDARK adopt:** 🔴 **AVOID.** Radial wheel PALDARK dùng MVVM (Aura pattern).

### ADR-9: 42 DataAsset cho data-driven
- **Quyết định:** Mọi configurable thông qua DataAsset.
- **PALDARK adopt:** 🟢 — Pal/Weapon/Ability/Map đều là DataAsset.

---

## Phase 6 — System Map

| System | Class chính | File reference (đường dẫn approx) | Status |
|--------|------|-----------------------------------|--------|
| Activity Base | `UBaseActivity` | `Source/.../Info/Activities/BaseActivity.h/cpp` | 🟢 |
| Activity FSM | `UActivityFiniteStateMachine` | `Source/.../Info/Activities/` | 🟢 |
| Custom AISense | `UReadyOrNotAISense_*` | `Source/.../Senses/` | 🟢 |
| Threat Octree | `UThreatAwarenessSubsystem` | `Source/.../Octree/` | 🟢 |
| Significance | `UReadyOrNotSignificanceManager` | `Source/.../Performance/` | 🟢 |
| FMOD Propagation | `UFMODAudioPropagationComponent` | `Source/.../Audio/` | 🟢 |
| Indicator | `UIndicatorManagerComponent` | `Source/.../UI/Indicators/` | 🟢 |
| Character (god) | `ASwatCharacter` | `Source/.../Characters/SwatCharacter.h/cpp` | 🔴 god |
| Command Widget (god) | `USwatCommandWidget` | `Source/.../UI/Widgets/SwatCommandWidget.h/cpp` | 🔴 god |
| DataAsset | 42 cái | `Source/.../Data/` | 🟢 |

---

## Phase 7 — Stories taught (extract từ shipped)

| US | Mô tả |
|----|-------|
| US-1 | Là dev, tôi build Activity FSM base reusable |
| US-2 | Là dev, tôi viết Activity cho mỗi behavior (12+ per AI type) |
| US-3 | Là dev, tôi tạo Custom AISense (scent/sound/sight) |
| US-4 | Là dev, tôi viết ThreatOctree subsystem cho spatial query |
| US-5 | Là dev, tôi subclass SignificanceManager cho cull AI tick |
| US-6 | Là dev, tôi integrate FMOD Studio + Propagation Component |
| US-7 | Là dev, tôi build Indicator Manager cho world-space marker |
| US-8 | Là dev, tôi setup 42 DataAsset cho data-driven content |

---

## Take-aways cho PALDARK

### 🟢 Copy nguyên (CORE — PALDARK đứng vững nhờ những cái này)
- **Activity FSM:** `UBaseActivity` + FSM container → `UPaldarkBaseActivity` + `UPaldarkPalActivityComponent`.
- **Custom AISense:** template từ RoN.
- **ThreatOctree:** spatial partition.
- **SignificanceManager subclass:** AI cull + LOD.
- **FMOD Audio Propagation Component:** wire vào PALDARK.
- **Indicator Manager:** world-space marker.
- **42 DataAsset pattern:** áp cho Pal/Weapon/Ability/Map.

### 🔴 AVOID (anti-pattern phải tránh)
- **God-character 368KB** → PALDARK Character < 50KB, logic ở component.
- **God-widget 5K LOC** → PALDARK widget < 1.5K LOC, MVVM.
- **No GAS** → PALDARK dùng GAS từ đầu.

---

## Pillar Mapping — đóng góp của source này

| Pillar | Đóng góp |
|--------|----------|
| P1 | 🟢 Module split AAA-scale |
| P2 | 🟢 GameMode/GameState/Subsystem nâng cao |
| P3 | 🟡 Component-debt — copy intent, không copy thực thi |
| P5 | 🟢 Animation polish |
| P6 | 🟢 Replication mid-tier |
| P9 | 🟢 **Activity FSM + Octree + Custom Sense — best in class** |
| P10 | 🟡 UI scale (god-debt) |
| P12 | 🟢 **42 DataAsset** |
| P13 | 🟢 Save/Load |
| P14 | 🟢 Async loading |
| P15 | 🟢 **Significance + Octree** |
| P16 | 🟢 **FMOD Propagation** |
| P18 | 🟡 mod.io + Steam + PS5 |

---

## Tham chiếu

- [Courses/04-ReadyOrNot.md](../Courses/04-ReadyOrNot.md)
- [Projects/ReadyOrNot_Assessment.md](../Projects/ReadyOrNot_Assessment.md) — 73% coverage.
- [PALDARK/04-Resource_Map.md](../PALDARK/04-Resource_Map.md) — pillar nào học từ đây (🥇 nhất).
- [Donchitos Framework](../Donchitos_GameStudios_Framework.md).
