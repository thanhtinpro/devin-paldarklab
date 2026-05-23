# P08 — Gameplay Ability System (GAS)

> Tự soạn từ: [05] GAS Crash, [01] ActionRoguelike, [11] Aura, [13] Crunch, [14] Lyra, [15] Lyra RPG.
>
> **Pillar phức tạp nhất, but PALDARK build trên GAS.** Mọi ability (Player fire, Pal cast, buff, debuff) đều qua GAS.

---

## Pillar Identity

**Cái gì:** Dùng GAS cho ability + buff/debuff + damage + attribute.

**Scope:**
- ✅ UAbilitySystemComponent (ASC) — mount point.
- ✅ UAttributeSet (AS) — stat storage.
- ✅ UGameplayAbility (GA) — executable action.
- ✅ UGameplayEffect (GE) — modify attribute.
- ✅ FGameplayTag — universal label.
- ✅ GameplayCue — VFX/SFX trigger.
- ✅ AbilityTask — async flow in GA.
- ✅ Execution Calculation — custom damage formula.
- ✅ Modifier Magnitude Calculation (MMC) — dynamic modifier.
- ✅ Replication Mode (Full/Mixed/Minimal).

**Out of scope:**
- ❌ Lyra GAS wrapper detail (P17).
- ❌ Lag compensation (P06).

---

## Sources Matrix

| Source | Coverage | Lesson lấy từ đó |
|--------|----------|------------------|
| [05] GAS Crash | 🟡 | L1 Vocabulary (ASC/AS/GA/GE/Tag/Cue) |
| [01] ActionRoguelike | 🟠 | L2 Custom Action System (pre-GAS — compare) |
| [11] Aura | 🟢 | L3 AS structure, L4 Damage Execution, L5 Spell tree, L6 Passive |
| [13] Crunch | 🟢 | L7 GAS optimize + cooldown + combo |
| [14] Lyra | 🟠 | L8 Lyra GAS wrapper |
| [15] Lyra RPG | 🟠 | L8 Lyra GAS RPG extension |

---

## Prerequisite

- P01 (C++ macro).
- P02 (PlayerState own ASC).
- P03 (Component composition).
- P04 (Input → ability trigger).

---

## Lessons

### L1 — GAS Vocabulary: 6 bricks 📖🧪

**Goal:** Nhớ ASC + AS + GA + GE + Tag + Cue.

**Concept:**
- **ASC** — component trên Actor (hoặc PlayerState). Là "brain" quản lý GA/GE/Tag.
- **AS** — `UPROPERTY` float set (Health, Mana, ...). Có `PreAttributeChange`, `PostGameplayEffectExecute`.
- **GA** — `ActivateAbility()` / `EndAbility()`. Có Cost (GE) + Cooldown (GE) + Tag requirement.
- **GE** — modify AS. Type: Instant (damage), Duration (buff 5s), Infinite (passive).
- **Tag** — hierarchical label: `Ability.Fire.Bolt`, `State.Dead`, `Debuff.Poison`.
- **Cue** — VFX/SFX triggered by GE tag match `GameplayCue.*`.

**Source learning path:**
- [05] GAS Crash § Full course (9 chapters).
- Cross-ref: `Documents/GameDesign/05-GAS_CrashCourse_GDD.md`.

**API / Class chính:**
- `UAbilitySystemComponent::GiveAbility(FGameplayAbilitySpec)`
- `UAbilitySystemComponent::TryActivateAbilityByClass(TSubclassOf<UGameplayAbility>)`
- `UAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData&)`

**Practice exercise (3 hour):**
1. Tạo `UPaldarkLabASC`, `UPaldarkLabAS` (Health, Mana).
2. Tạo `GA_LabFireBolt` — cost 10 Mana, cooldown 2s.
3. `GE_LabFireDamage` instant -20 Health.
4. PIE: press Q → fire bolt → enemy Health -20 → cue particle trigger.

**PALDARK take-away:**
- 🟢 Toàn bộ Player + Pal ability qua GAS.
- 🟢 Pal ability: GA_PalBite, GA_PalRoar, GA_PalHeal.
- 🟢 Player ability: GA_Fire, GA_Reload, GA_Sprint, GA_Interact.

**Apply ở week:** Roadmap week 6-7.

---

### L2 — Custom Action System vs GAS (Tom Looman compare) 📖

**Goal:** Hiểu tại sao Tom viết custom Action System, và tại sao PALDARK chọn GAS.

**Concept:**
- Tom's Action System: `UActionComponent` + `UAction` subclass.
- Ưu: Simple, lightweight, dễ hiểu.
- Nhược: Không replicate native, không tag system, không prediction, không execution calc.
- GAS ưu: Built-in replication, prediction, tag, execution calc, community support.
- GAS nhược: Learning curve cao, boilerplate nhiều.

**Source learning path:**
- [01] ActionRoguelike `SActionComponent`, `SAction` class.
- Cross-ref: `Documents/GameDesign/01-ActionRoguelike_GDD.md` ADR-2.

**PALDARK take-away:**
- 🟢 PALDARK GAS hết — MP game cần prediction + replication native.
- 🔴 KHÔNG viết custom ability system.

**Apply ở week:** Decision đã made ở ADR-002 PALDARK.

---

### L3 — AS structure: Primary / Secondary / Vital (Aura) 🧪

**Goal:** Thiết kế AttributeSet multi-layer.

**Concept (Aura pattern):**
- **Primary:** Strength, Intelligence, Resilience, Vigor (user allocate point).
- **Secondary:** Armor (derived from Resilience via MMC), CritChance (from Intelligence), etc.
- **Vital:** Health, MaxHealth, Mana, MaxMana (current value).

**Source learning path:**
- [11] Aura § AS chapter.
- Cross-ref: `Documents/GameDesign/11-Aura_RPG_GDD.md` ADR-2.

**PALDARK adaptation:**
- Primary: Vitality, Agility, PalBond.
- Secondary: MaxHealth (Vitality × 10), MovementSpeed (Agility × 1.5), PalCastPower (PalBond × 2).
- Vital: Health, Stamina, Sanity.

**Practice exercise:**
1. `UPaldarkLabAS` với Primary/Secondary/Vital.
2. MMC `CalcMagnitude` cho MaxHealth = Vitality × 10.
3. PIE: allocate Vitality → MaxHealth auto increase.

**PALDARK take-away:**
- 🟢 Copy Aura AS structure.
- 🟡 Adapt: thêm `Sanity` (unique PALDARK stat) + `PalBond`.

**Apply ở week:** Roadmap week 7.

---

### L4 — Damage Execution Calculation (Aura CORE) 🧪

**Goal:** Viết custom damage chain: Base → Armor → Block → Crit → Final.

**Concept:**
- `UGameplayEffectExecutionCalculation` subclass.
- `Execute_Implementation(const FGameplayEffectCustomExecutionParameters&, FGameplayEffectCustomExecutionOutput&)`.
- Capture attacker attributes + defender attributes.
- Step-by-step: BaseDamage → reduce by Armor → roll BlockChance → roll CritChance → multiply CritDamage → apply final.

**Source learning path:**
- [11] Aura § Damage Execution chapter (best in all 15 courses).
- Cross-ref: `Documents/GameDesign/11-Aura_RPG_GDD.md` ADR-3.

**API / Class chính:**
- `FGameplayEffectAttributeCaptureDefinition`
- `UGameplayEffectExecutionCalculation::Execute_Implementation`
- `FGameplayEffectCustomExecutionOutput::AddOutputModifier`

**Practice exercise (4 hour):**
1. `UExecCalc_PaldarkDamage` capture: Attacker.BaseDamage, Defender.Armor, Defender.BlockChance.
2. Chain: Base - (Armor × 0.5) → if rand < BlockChance then ×0.5 → if rand < CritChance then ×CritMultiplier → final.
3. PIE: attack enemy → log each step → final damage applied.

**PALDARK take-away:**
- 🟢 `UExecCalc_PaldarkDamage` — single class cho mọi damage (Player, Pal, NPC).
- 🟢 Transparent chain — debug-able mỗi step.

**Apply ở week:** Roadmap week 13-14.

---

### L5 — Spell tree + Cooldown via GE (Aura + Crunch) 🧪

**Goal:** Grant multiple GA, manage cooldown, spell unlock tree.

**Concept:**
- `GA_Spell_*` subclass per spell.
- Cooldown via `GE_Cooldown_*` Duration type.
- `GA::GetCooldownGameplayEffect()` → return GE_Cooldown.
- Spell tree = DataAsset list of GA class + required level.
- `ASC->GiveAbility(FGameplayAbilitySpec(GA, Level))`.

**Source learning path:**
- [11] Aura § Spell Tree chapter.
- [13] Crunch § Combo + Cooldown chapter.

**Practice exercise:**
1. 3 Pal ability: GA_PalBite (CD 3s), GA_PalRoar (CD 8s), GA_PalHeal (CD 15s).
2. Pal level up → unlock GA_PalHeal at level 3.
3. `UPalAbilityDataAsset` — list {GA class, required level, description}.

**PALDARK take-away:**
- 🟢 Mỗi Pal species có `UPalAbilityDataAsset` (3 ability).
- 🟢 Cooldown via GE Duration.
- 🟢 Unlock ability via level → auto grant.

**Apply ở week:** Roadmap week 15.

---

### L6 — Passive ability + Infinite GE (Aura) 📖🧪

**Goal:** Passive ability = GE Infinite applied on spawn.

**Concept:**
- `GA_Passive_*` — ActivateAbility → ApplyGameplayEffectToSelf(GE_Passive_Armor) → EndAbility.
- `GE_Passive_Armor` — Infinite duration, modify Armor +10.
- Applied once at BeginPlay.

**Source learning path:**
- [11] Aura § Passive chapter.

**PALDARK take-away:**
- 🟢 Pal species passive: `GE_PalPassive_FireResist` (fire Pal immune to fire).
- 🟢 Equipment passive: equip armor → apply `GE_ArmorBonus`.

**Apply ở week:** Roadmap week 15.

---

## ⚡ Capstone Exercise

**Goal:** GAS combat sandbox: Player + Pal + Enemy.

**Yêu cầu:**
1. Player ASC (on PlayerState) + AS Primary/Secondary/Vital.
2. Pal ASC (on Pawn) + AS.
3. 3 Player GA (Fire, Sprint, Reload) + 3 Pal GA (Bite, Roar, Heal).
4. Damage Execution Calculation chain.
5. Cooldown GE.
6. Passive GE.
7. Replication mode Mixed.
8. PIE 2 player: Player shoot enemy, Pal cast, cooldown visible.

**Acceptance:** Damage chain log 5-step; cooldown prevent spam; passive auto-apply; replicate correct.

**Effort:** 1.5 tuần.

---

## Khi apply vào PALDARK roadmap

| Week | Pillar P08 usage |
|------|------------------|
| W6 | ASC mount on PlayerState |
| W7 | AS Primary/Secondary/Vital |
| W13-14 | Damage Execution Calculation |
| W15 | Pal ability tree + passive |
| W17 | Player weapon GA (Fire/Reload/ADS) |
| W36 | GAS polish + optimization |

---

## Anti-patterns (cảnh báo)

- 🔴 ASC trên Pawn cho Player → mất buff khi respawn.
- 🔴 Custom ability system thay GAS cho MP game → reinvent wheel.
- 🔴 Damage logic ngoài GAS (raw `Health -= Damage`) → bypass buff/debuff.
- 🔴 Cooldown manual timer thay GE Duration → không integrate tag system.
- 🔴 God-GA 500 LOC → split thành AbilityTask + Execution Calc.
- 🔴 Replicate mode Full cho mọi ASC → bandwidth waste.

---

## Tham chiếu

- [`Documents/UE5_Core_Pillars.md`](../UE5_Core_Pillars.md) § P8.
- [`Documents/GameDesign/05-GAS_CrashCourse_GDD.md`](../GameDesign/05-GAS_CrashCourse_GDD.md).
- [`Documents/GameDesign/11-Aura_RPG_GDD.md`](../GameDesign/11-Aura_RPG_GDD.md).
- [`Documents/GameDesign/13-Crunch_GDD.md`](../GameDesign/13-Crunch_GDD.md).
- [`Documents/PALDARK/01-GameDesign.md`](../PALDARK/01-GameDesign.md) ADR-002.
