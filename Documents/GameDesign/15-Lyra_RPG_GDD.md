# 15 — Build an RPG using Lyra Framework (Donchitos Reverse GDD)

> Source: [Udemy — UE5.6 Build an Action RPG using Lyra Framework](https://www.udemy.com/course/unreal-engine-56-build-an-rpg-using-lyra-framework/).
>
> Sample game: action RPG xây trên Lyra — locomotion, inventory, indicator, attribute, FrontEnd menu.

---

## Identity Card

| Field | Value |
|-------|-------|
| 📦 Type | Course (Lyra application) |
| Genre | Third-person action RPG |
| Engine | UE5.6 |
| Đóng góp PALDARK | **Áp dụng Lyra cho RPG** — chứng minh Lyra không chỉ shooter |

---

## Phase 1 — Brainstorm (reverse)

### Implicit pitch
> *"Lyra không chỉ shooter. Có thể build RPG ổn với Locomotion + Inventory + Indicator + Attribute + FrontEnd."*

### Cảm xúc cốt lõi
- **Lyra unblocked** — không phải Lyra chỉ shooter.
- **RPG vibe** — narrative-friendly framework.

---

## Phase 2 — Decompose

### Verb cốt lõi
- Move (Lyra Locomotion)
- Fight (melee/ranged)
- Open inventory (Lyra Quick Bar + UI)
- Look at indicator (Lyra Indicator)
- Open menu (Lyra FrontEnd)
- Talk NPC
- Level up

### System hierarchy

```
┌──────────────────────────────────────┐
│ Lyra Experience: RPGCampaign         │
└──────────────┬───────────────────────┘
               │
   ┌───────────┼────────────┬───────────┐
   ▼           ▼            ▼           ▼
LyraChar    LyraASC      Lyra UI    Lyra Indicator
+ RPG Pawn   + RPG AS    + RPG Menu  + Quest waypoint
+ Locomotion + RPG GA
Distance     Spells
Matching
```

---

## Phase 3 — Define (GDD reverse)

### Game Loop
1. FrontEnd Menu → New Game.
2. Spawn into world map.
3. Move (Lyra Locomotion = distance matching, smooth turn).
4. Combat (RPG ability via Lyra GAS).
5. Pickup item (Lyra Quick Bar).
6. Open inventory + equip.
7. Quest waypoint via Lyra Indicator.
8. Level up.

### Player Character
- Lyra Pawn Data với RPG mesh + RPG Ability Set + Input Config.
- AttributeSet RPG (Health, Stamina, Mana, XP, Level).

### Locomotion
- Lyra distance matching → step root motion matches velocity.
- Smooth turn-in-place.

### Inventory Quick Bar
- Lyra QuickBar component on Pawn.
- Slot index (1–6 hotkey).

### Indicator
- Quest waypoint world-space marker.
- Distance overlap shown.

### FrontEnd
- Activatable Widget Stack.
- Main Menu → Char Select → New Game.

---

## Phase 4 — 3 Game Pillars (inferred)

### Pillar 1 — Locomotion is Character
> *"Cách char di chuyển = identity. Distance matching = smooth = AAA feel."*

### Pillar 2 — UI Tells Health
> *"Quick Bar + Indicator + HUD = 90% gameplay clarity."*

### Pillar 3 — Indicator Guides
> *"Player không lost. Marker tell where to go."*

---

## Phase 5 — Architecture Decisions (implicit)

### ADR-1: Lyra adopt full (không build from scratch)
- **Quyết định:** Inherit Lyra base, extend cho RPG.
- **Lý do:** Tiết kiệm 6–12 tháng work.
- **PALDARK adopt:** 🟢 **CORE** — ADR-001 của PALDARK.

### ADR-2: Distance Matching cho locomotion
- **Quyết định:** Step length match velocity → no foot sliding.
- **PALDARK adopt:** 🟢 — Player + Pal locomotion.

### ADR-3: Quick Bar component
- **Quyết định:** Lyra Quick Bar inventory.
- **PALDARK adopt:** 🟢.

### ADR-4: Indicator Manager
- **Quyết định:** Subsystem manage all marker.
- **PALDARK adopt:** 🟢.

### ADR-5: FrontEnd menu Activatable Stack
- **PALDARK adopt:** 🟢.

### ADR-6: State Tree cho NPC AI
- **PALDARK adopt:** 🟡 Adapt — chỉ NPC Hub.

---

## Phase 6 — System Map

| System | Class | File |
|--------|-------|------|
| Experience | `URPG_ExperienceDefinition` | `RPG/...` |
| Pawn Data | `URPG_PawnData` | `RPG/...` |
| Char | `ARPG_LyraCharacter` | `RPG/.../Character/...` |
| AS | `URPG_AttributeSet` | `RPG/.../GAS/...` |
| Quick Bar | `ULyraQuickBarComponent` (inherit) | Lyra |
| Indicator | `ULyraIndicatorManagerComponent` (inherit) | Lyra |
| Locomotion | Lyra Locomotion Distance Matching | Lyra |
| FrontEnd | `ULyraFrontEndStateComponent` (inherit) | Lyra |

---

## Phase 7 — Stories taught

| Phần | US |
|------|----|
| 1 | Là dev, tôi clone Lyra base + tạo RPG plugin |
| 2 | Là dev, tôi tạo RPG Experience |
| 3 | Là dev, tôi tạo RPG Pawn Data |
| 4 | Là dev, tôi tích hợp RPG mesh + animation |
| 5 | Là dev, tôi setup Distance Matching Locomotion |
| 6 | Là dev, tôi viết RPG AttributeSet |
| 7 | Là dev, tôi viết RPG Ability (melee/spell) |
| 8 | Là dev, tôi tích hợp Quick Bar inventory |
| 9 | Là dev, tôi setup Indicator quest waypoint |
| 10 | Là dev, tôi viết RPG FrontEnd menu |
| 11 | Là dev, tôi setup Level Up flow |
| 12 | Là dev, tôi setup NPC State Tree |

---

## Take-aways cho PALDARK

### 🟢 Copy nguyên (Application playbook)
- **Lyra adopt pattern** (inherit + extend).
- **Distance Matching Locomotion**.
- **Quick Bar** cho inventory.
- **Indicator Manager** cho extract beacon, civilian marker, Pal location.
- **FrontEnd Activatable Stack**.

### 🟡 Adapt
- **State Tree NPC** — PALDARK chỉ NPC Hub town.

---

## Pillar Mapping

| Pillar | Đóng góp |
|--------|----------|
| P17 | 🟢 **Lyra application playbook** |
| P5 | 🟢 Distance Matching |
| P10 | 🟢 FrontEnd + Indicator + Quick Bar |
| P11 | 🟠 Quick Bar inventory |
| P8 | 🟠 GAS Lyra style |

---

## Tham chiếu

- [Courses/15-Udemy-ue5-build-an-rpg-using-lyra-framework.md](../Courses/15-Udemy-ue5-build-an-rpg-using-lyra-framework.md)
- [14-Exploring_Lyra_GDD.md](14-Exploring_Lyra_GDD.md) — framework basics.
- [PALDARK/01-GameDesign.md](../PALDARK/01-GameDesign.md) — Lyra adopt decisions.
