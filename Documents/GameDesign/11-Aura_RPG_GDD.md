# 11 — GAS Top-Down RPG (Aura) (Donchitos Reverse GDD)

> Source: [github.com/DruidMech/GameplayAbilitySystem_Aura](https://github.com/DruidMech/GameplayAbilitySystem_Aura) — Stephen Ulibarri, [Udemy](https://www.udemy.com/course/unreal-engine-5-gas-top-down-rpg/).
>
> Sample game: **Aura** — Diablo-like top-down ARPG với GAS sâu (damage chain, spells, attribute menu, checkpoint).

---

## Identity Card

| Field | Value |
|-------|-------|
| 📦 Type | Course (deep GAS application) |
| Genre | Top-down ARPG (Diablo-style) |
| Engine | UE5 |
| Đóng góp PALDARK | **GAS production — Damage Execution + Spells + Attribute Menu + Widget Controller MVC** |

---

## Phase 1 — Brainstorm (reverse)

### Implicit pitch
> *"Bạn là pháp sư trên grid. Bắn spell, level up, level điểm, spell tree. Đánh damage qua armor → block chance → crit → damage. Mỗi click là 1 quyết định."*

### Cảm xúc cốt lõi
- **Power fantasy** — level cao = strong feel.
- **Build identity** — passive tree + spell choice.
- **Loot satisfaction**.

---

## Phase 2 — Decompose

### Verb cốt lõi
- Click move
- Click spell
- Level up (XP)
- Attribute point (manual allocate)
- Spell point (unlock spell)
- Pickup item
- Use checkpoint

### System hierarchy

```
┌─────────────────────────────────┐
│ AAuraCharacter + ASC + AttribSet │
└──────────────┬──────────────────┘
               │
   ┌───────────┼────────────┬───────────┐
   ▼           ▼            ▼           ▼
GAS Stack    UI MVC      Damage Chain  Save/Load
(50+ GA)     Widget       Execution    Checkpoint
             Controllers  Calculation   spawn
                          MMC
                          
AI:                       Inventory:
- Melee BT                - Slim
- Ranged BT
- Spell BT
```

---

## Phase 3 — Define (GDD reverse)

### Game Loop
1. Spawn at checkpoint.
2. Click move + click enemy.
3. Cast spell (RMB / Q / E / R).
4. Kill enemy → XP.
5. Level up → allocate attribute point + spell point.
6. Reach checkpoint → save + map travel.
7. Boss area.

### Player Character
- Top-down.
- ASC on PlayerState (persistent across respawn).
- AttributeSet: Strength, Intelligence, Resilience, Vigor, +derived (Health, Mana, Armor, Crit, etc.).

### Damage Chain (Execution Calculation)
- Custom `UExecutionCalculation_Damage`:
  - Read attacker BaseDamage.
  - Read target Armor → reduce.
  - Roll BlockChance → if block, half damage.
  - Roll CritChance → if crit, multiply CritDamage.
  - Apply final damage.
- Each step is **transparent** — debug-able.

### Spells (GA subclass)
- Fire Bolt, Electrocute, Arcane Shards, ...
- Spell Tree (DataAsset).
- Spell Menu UI to unlock.

### UI MVC pattern
- **Widget Controller** (UPawnSomething_WidgetController) holds data + delegate.
- **Widget** subscribes to delegate.
- **Model** = AttributeSet + DataAssets.

### Save/Load Checkpoint
- USaveGame subclass per slot.
- Save: player attribute, level, gold, position, map ID.
- Load on Boot.

### AI
- Multiple BT subclass: Melee, Ranged, Spellcaster.
- Granted ability per AI type.

---

## Phase 4 — 3 Game Pillars (inferred)

### Pillar 1 — Ability Tells Story
> *"Mỗi ability có name, icon, description. Cast = identity."*

### Pillar 2 — Damage is a Conversation
> *"Damage không 1 con số — là 5 bước: base → armor → block → crit → final."*

### Pillar 3 — Level is Permanent
> *"Bạn level lên không reset. Tiến bộ thật."*

---

## Phase 5 — Architecture Decisions (implicit)

### ADR-1: ASC trên PlayerState (không Pawn)
- **Quyết định:** PlayerState ownASC → persistent across respawn.
- **PALDARK adopt:** 🟢 — Player ASC on PlayerState.

### ADR-2: AS hybrid (Primary + Secondary + Vital)
- **Quyết định:** Primary = stat user allocate. Secondary = derived from primary (MMC). Vital = current health/mana.
- **PALDARK adopt:** 🟡 Adapt — PALDARK simplify (Primary: Strength/Vitality/Stamina; Secondary: derived; Vital: Health/Stamina/Sanity).

### ADR-3: Custom Execution Calculation cho damage
- **Quyết định:** Damage chain explicit step.
- **PALDARK adopt:** 🟢 — `UExecutionCalculation_PaldarkDamage`.

### ADR-4: Widget Controller MVC
- **Quyết định:** UI logic ở separate UObject, không Widget.
- **Lý do:** Avoid god-widget (RoN anti-pattern).
- **PALDARK adopt:** 🟢 **CORE** — radial wheel + attribute menu dùng MVC.

### ADR-5: Curve Table cho level → stat
- **Quyết định:** XP → Level via Curve. Level → bonus stat via Curve.
- **PALDARK adopt:** 🟢 — Pal level curve.

### ADR-6: SaveGame + Checkpoint actor
- **Quyết định:** Checkpoint actor in level, on overlap → save.
- **PALDARK adopt:** 🟡 — PALDARK Hub đại diện checkpoint (server-side save trigger).

---

## Phase 6 — System Map

| System | Class | File |
|--------|-------|------|
| Char | `AAuraCharacter` | `Character/AuraCharacter.h/cpp` |
| ASC | `UAuraAbilitySystemComponent` | `AbilitySystem/AuraAbilitySystemComponent.h/cpp` |
| AS | `UAuraAttributeSet` | `AbilitySystem/AuraAttributeSet.h/cpp` |
| Damage Exec | `UExecCalc_Damage` | `AbilitySystem/ExecCalc/...` |
| Widget Ctrl | `UAuraWidgetController` | `UI/WidgetController/...` |
| Save | `UAuraSaveGame` | `Game/AuraSaveGame.h/cpp` |
| AI | `AAuraAICharacter` + BT | `AI/...` |
| Spell DataAsset | `UAbilityInfo` | `AbilitySystem/Data/...` |

---

## Phase 7 — Stories taught (5 phần lớn × ~40 lesson)

| Phần | US tổng |
|------|---------|
| 1 | Setup project + Top-down camera + Click move |
| 2 | ASC + AS Primary/Secondary/Vital |
| 3 | Granted ability + Spell tree + Spell Menu |
| 4 | Damage Execution + Block + Crit |
| 5 | UI Widget Controller MVC + Attribute Menu |
| 6 | Inventory + Pickup |
| 7 | Save / Load + Checkpoint |
| 8 | AI Melee / Ranged / Spellcaster |
| 9 | Boss + Multi-stage encounter |

---

## Take-aways cho PALDARK

### 🟢 Copy nguyên (GAS Playbook)
- **ASC on PlayerState** (persistent).
- **AS Primary/Secondary/Vital structure**.
- **Damage Execution Calculation** (transparent chain).
- **Widget Controller MVC** (avoid god-widget).
- **Curve Table cho progression**.
- **GA Spell pattern** (apply cho Pal ability).

### 🟡 Adapt
- **Spell tree** → PALDARK Pal Ability tree (mỗi loài 3 ability).
- **Click-move** → PALDARK TPS, không click-move.

---

## Pillar Mapping

| Pillar | Đóng góp |
|--------|----------|
| P8 | 🟢 **GAS production — best in course** |
| P10 | 🟢 **MVC Widget Controller** |
| P11 | 🟠 Inventory mid |
| P12 | 🟢 **Curve Table + DataAsset spell** |
| P13 | 🟢 **SaveGame + Checkpoint** |
| P9 | 🟠 AI BT subclass per role |

---

## Tham chiếu

- [Courses/11-Udemy-ue5-gas-top-down-rpg.md](../Courses/11-Udemy-ue5-gas-top-down-rpg.md)
- [05-GAS_CrashCourse_GDD.md](05-GAS_CrashCourse_GDD.md) — basics trước.
- [PALDARK/03-Roadmap_1_Year.md](../PALDARK/03-Roadmap_1_Year.md) — tuần 7, 36 áp pattern.
