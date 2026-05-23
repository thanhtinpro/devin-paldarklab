# P09 — AI (BT / StateTree / Activity FSM / Utility)

> Tự soạn từ: [01] ActionRoguelike, [04] ReadyOrNot, [11] Aura, [12] Pro UE, [15] Lyra RPG.
>
> **Pal AI là differentiator** — nếu Pal hành xử như bot thì game thất bại. Activity FSM + custom AISense = PALDARK CORE.

---

## Pillar Identity

**Cái gì:** Build AI có decision-making khả thi production.

**Scope:**
- ✅ Behavior Tree (BT) + Blackboard.
- ✅ EQS (Environment Query System).
- ✅ StateTree (UE5.1+).
- ✅ UAIPerceptionComponent + AISense.
- ✅ Activity FSM pattern (ReadyOrNot — production-grade).
- ✅ Utility AI (ReadyOrNot — scoring decision).
- ✅ Custom AISense (Scent/Threat/Sound).

**Out of scope:**
- ❌ Animation driven by AI state (P05).
- ❌ Navigation mesh tuning (P16).

---

## PALDARK cần gì từ pillar này?

| AI Agent | Pattern | Vì sao |
|----------|---------|--------|
| Pal Companion | Activity FSM (Follow/Combat/Forage/Idle/Wait) | Nhiều activity rõ ràng, transition dựa trên Utility |
| Pal Combat | BT (Attack/Retreat/Heal) | Standard combat loop |
| Dark Zone NPC Enemy | BT + EQS | Patrol/Search/Attack pattern |
| Civilian | State Machine (Idle/Fearful/Fleeing/Dead) | Simple FSM, react to threat |
| Hub NPC (shop/quest) | StateTree | Dialogue + simple behavior |

---

## Sources Matrix

| Source | Coverage | Lesson lấy từ đó |
|--------|----------|------------------|
| [01] ActionRoguelike | 🟡 | L1 BT + Blackboard + EQS basics |
| [11] Aura | 🟡 | L2 AI role-based (Melee/Ranged/Spellcaster) |
| [04] RoN | 🟢 | L3 Activity FSM (CORE), L4 Custom AISense, L5 Utility scoring |
| [12] Pro UE | 🟠 | L6 BT intro |
| [15] Lyra RPG | 🟡 | L7 StateTree |

---

## Prerequisite

- P02 (AIController, Pawn).
- P03 (Component — AIPerception is component).
- P08 (GAS — Pal ability trigger from AI).

---

## Lessons

### L1 — BT + Blackboard + EQS basics (Tom Looman) 🧪

**Goal:** AI bot patrol + detect player + chase + attack.

**Concept:**
- **BT:** tree of Selector/Sequence + Task/Decorator/Service nodes.
  - Selector = try children left → right, stop at first success.
  - Sequence = run children left → right, stop at first failure.
- **Blackboard:** key-value store (TargetActor, PatrolLocation, HealthFloat).
- **EQS:** query environment (find cover, find nearest player, find random point).
- **AIController:** run BT, `RunBehaviorTree(BT_Asset)`.

**Source learning path:**
- [01] ActionRoguelike § AI chapters.

**API / Class chính:**
- `UBehaviorTree`
- `UBlackboardData`
- `AAIController::RunBehaviorTree(UBehaviorTree*)`
- `UEnvQueryManager::RunEQSQuery(...)`

**Practice exercise (4 hour):**
1. `BT_LabPatrolBot` — Patrol (EQS random point) → Detect Player (Perception) → Chase → Attack.
2. Blackboard: `TargetActor`, `PatrolLocation`.
3. Service `BTService_CheckPerception` update TargetActor.
4. PIE: bot patrol → see player → chase → melee.

**PALDARK take-away:**
- 🟢 NPC enemy Dark Zone dùng BT pattern này.
- 🟢 EQS cho "find cover" + "find patrol route".

**Apply ở week:** Roadmap week 20-21.

---

### L2 — AI role-based: Melee / Ranged / Spellcaster (Aura) 🧪

**Goal:** BT khác nhau cho role khác nhau — enemy type diversity.

**Concept:**
- Melee BT: approach → attack range check → melee strike → retreat.
- Ranged BT: keep distance → LOS check → fire projectile → retreat if close.
- Spellcaster BT: keep distance → cast spell → cooldown wait → cast next.
- AI Character granted GA per role → BT task `BTTask_ActivateAbility`.

**Source learning path:**
- [11] Aura § AI chapters (3 BT variant).

**Practice exercise:**
1. 3 BT: `BT_LabMelee`, `BT_LabRanged`, `BT_LabSpellcaster`.
2. `BTTask_ActivateAbilityByTag(Tag)` generic task.
3. Spawn 3 enemy types → PIE: observe different behavior.

**PALDARK take-away:**
- 🟡 Pattern dùng cho Dark Zone NPC (Melee/Ranged).
- 🟡 Pal AI hỗn hợp (không pure role) — dùng Activity FSM thay vì role BT.

**Apply ở week:** Roadmap week 21.

---

### L3 — Activity FSM pattern (ReadyOrNot CORE) 🧪

**Goal:** AI agent có nhiều "activity" — mỗi activity là FSM state với transition condition.

**Concept:**
RoN pattern:
- **Activity** = FSM state: Patrol, Search, Guard, Breach, Clear, Arrest, Flee, Combat.
- Mỗi activity có `Enter()` / `Tick()` / `Exit()` + transition rules.
- **Transition** dựa trên Utility score (L5).
- Ưu so với BT: BT hard-coded priority; Activity FSM flexible per-situation scoring.

**Source learning path:**
- [04] RoN GDD § Phase 5 ADR-1 (Activity FSM).
- Cross-ref: `Documents/GameDesign/04-ReadyOrNot_GDD.md`.

**PALDARK Activity FSM cho Pal:**

| Activity | Enter condition | Tick | Exit condition |
|----------|----------------|------|----------------|
| Follow | Default (no threat, owner walking) | Follow owner at distance 3m | Threat detected or command |
| Combat | Threat detected or Attack command | Attack target BT subtree | Target dead or owner recall |
| Forage | Forage command + resource nearby | Approach resource + harvest timer | Resource depleted or threat |
| Idle | At Hub + no command | Play idle anim + look around | Command received |
| Wait | Wait command | Stay position + guard | Timeout 30s or command |
| BondRitual | Owner hold E + Pal trust ≥ threshold | Play bond anim + trust ++ | Anim complete or interrupt |

**Practice exercise (1 ngày):**
1. `UPaldarkLabActivityComponent` với state enum.
2. `FPaldarkActivity` base struct: `Enter/Tick/Exit`.
3. `FPaldarkActivity_Follow`, `FPaldarkActivity_Combat`, etc.
4. Transition: `PickNextActivity()` → loop activities, pick highest utility.
5. PIE: Pal follow → threat → combat → threat gone → follow.

**PALDARK take-away:**
- 🟢 **CORE** — `UPaldarkPalActivityComponent` với 6 activity.
- 🟢 Transition via Utility scoring (L5).

**Apply ở week:** Roadmap week 8 (Pal follow prototype) + W20-21 (full Activity FSM).

---

### L4 — Custom AISense: Scent + Sound + Threat (RoN) 🧪

**Goal:** AI sense phong phú hơn default Sight + Hearing.

**Concept:**
- `UAISense` base → subclass:
  - `UAISense_Sight` (built-in) — LOS + angle + distance.
  - `UAISense_Hearing` (built-in) — MakeNoise.
  - `UAISense_Scent` (custom) — scent trail decay over time.
  - `UAISense_Threat` (custom) — threat octree spatial query.

**Source learning path:**
- [04] RoN GDD § Phase 5 ADR-2 (custom AISense).

**API / Class chính:**
- `UAISense` / `UAISenseConfig`
- `UAIPerceptionComponent::OnTargetPerceptionUpdated` delegate.
- Custom `UAISense_Scent` — register scent source, decay timer.

**Practice exercise (4 hour):**
1. `UAISense_LabScent` custom sense.
2. Player emits scent every 5m walked (FootstepAnimNotify → register scent).
3. NPC `UAIPerceptionComponent` has Scent config → detect scent age < 30s.
4. PIE: NPC patrol → detect scent trail → follow → find player.

**PALDARK take-away:**
- 🟢 `UAISense_Scent` cho Pal + NPC.
- 🟢 `UAISense_Threat` cho Dark Zone fear propagation.
- 🟢 Pal sniff → detect loot/enemy via scent.

**Apply ở week:** Roadmap week 22-23.

---

### L5 — Utility AI scoring (RoN decision-making) 📖🧪

**Goal:** Mỗi activity có score, pick highest → flexible decision.

**Concept:**
- `IActivityConsideration` interface — compute score 0.0–1.0.
- Considerations: DistanceToThreat, HealthPercent, AmmoCount, OwnerDistance, TrustLevel.
- `Score = Consideration1 × Consideration2 × ... × Weight`.
- Highest score activity = next activity.
- Re-evaluate every 0.5s (không mỗi frame).

**Source learning path:**
- [04] RoN GDD § Phase 5 ADR-1.

**Practice exercise (3 hour):**
1. Pal Activity scoring:
   - Follow: `1.0 - (OwnerDist / 100m)` — xa owner → want follow.
   - Combat: `ThreatLevel × 0.8`.
   - Forage: `ResourceNearby × 0.5`.
2. PIE: Pal follow → enemy appear (ThreatLevel 0.9) → Combat wins → enemy die → Follow wins.

**PALDARK take-away:**
- 🟢 Utility scoring cho Pal Activity transition — linh hoạt hơn BT.
- 🟢 Tunable per-species: Fire Pal has CombatWeight × 1.5 (aggressive).

**Apply ở week:** Roadmap week 21.

---

### L6 — StateTree for Hub NPC 📖🧪

**Goal:** StateTree cho NPC đơn giản (shop, quest giver).

**Concept:**
- StateTree: state machine visual trong editor.
- Nodes: state + transition condition + task.
- Ưu: visual, designer-friendly.
- Nhược: less flexible than BT for complex behavior.

**Source learning path:**
- [15] Lyra RPG § StateTree chapter.

**Practice exercise:**
1. `ST_LabShopkeeper` — Idle → PlayerApproach → OpenShopUI → PlayerLeave → Idle.
2. `UStateTreeComponentSchema` trên NPC.

**PALDARK take-away:**
- 🟡 Hub NPC (shopkeeper, quest giver) dùng StateTree — simple enough.
- 🟡 KHÔNG dùng StateTree cho Pal (too complex for StateTree → Activity FSM).

**Apply ở week:** Roadmap week 35 (Hub NPC).

---

## ⚡ Capstone Exercise

**Goal:** Pal companion full AI: Activity FSM + Scent Sense + Utility scoring.

**Yêu cầu:**
1. `UPaldarkLabPalActivityComp` 6 activity.
2. Utility scoring 3 consideration (OwnerDist, ThreatLevel, ResourceNearby).
3. `UAISense_LabScent` custom sense.
4. BT subtree for Combat activity.
5. PIE: Pal follow → detect scent → switch Forage → enemy appear → Combat → enemy die → Follow.

**Acceptance:** Pal transitions smoothly; no stuck state; utility re-eval every 0.5s.

**Effort:** 1.5 tuần.

---

## Khi apply vào PALDARK roadmap

| Week | Pillar P09 usage |
|------|------------------|
| W8 | Pal Follow activity prototype |
| W20-21 | Full Activity FSM + Utility scoring |
| W22-23 | Custom AISense (Scent + Threat) |
| W35 | Hub NPC StateTree |

---

## Anti-patterns (cảnh báo)

- 🔴 Simple BT cho complex agent (Pal) → inflexible, hard to tune.
- 🔴 BT tightly coupled với specific enemy → not reusable.
- 🔴 AI tick mỗi frame → performance issue. Budget 0.5s re-eval.
- 🔴 Custom AISense without spatial optimization (octree) → O(n²).
- 🔴 StateTree cho mọi AI → overkill cho complex; underkill cho simple.
- 🔴 Hardcoded behavior thay vì data-driven consideration → designer không tune được.

---

## Tham chiếu

- [`Documents/UE5_Core_Pillars.md`](../UE5_Core_Pillars.md) § P9.
- [`Documents/GameDesign/04-ReadyOrNot_GDD.md`](../GameDesign/04-ReadyOrNot_GDD.md) — Activity FSM + Utility.
- [`Documents/GameDesign/01-ActionRoguelike_GDD.md`](../GameDesign/01-ActionRoguelike_GDD.md) — BT basics.
- [`Documents/GameDesign/11-Aura_RPG_GDD.md`](../GameDesign/11-Aura_RPG_GDD.md) — Role-based BT.
- [`Documents/PALDARK/01-GameDesign.md`](../PALDARK/01-GameDesign.md) — Pal Activity definition.
