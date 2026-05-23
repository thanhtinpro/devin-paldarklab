# 11. Unreal Engine 5 — Gameplay Ability System — Top Down RPG (Aura)

| | |
|---|---|
| **Tier** | T4 — Specialized Systems / GAS sâu nhất |
| **Provider** | Stephen Ulibarri (DruidMech) — Udemy |
| **Link** | https://www.udemy.com/course/unreal-engine-5-gas-top-down-rpg/ |
| **Source** | https://github.com/DruidMech/GameplayAbilitySystem_Aura |
| **Trong repo** | `11.Udemy-ue5-gas-top-down-rpg/` (Documents + Source) |

---

## 1. Mục tiêu khóa

Build **action-RPG top-down Diablo-style "Aura"** dùng GAS chính chủ + Damage chain + Spells + Save. **Đây là khóa GAS dài nhất (33 chương).**

---

## 2. Lộ trình giảng dạy (33 chương)

| Chương | Cốt lõi |
|--------|---------|
| 01 - Introduction | Aura RPG overview, project structure |
| 02 - Project Creation | Top-down template, version control optional |
| 03 - **Intro to GAS** | The Gameplay Ability System (overview), Main Parts of GAS, PlayerState pattern, ASC + AttributeSet, GAS in Multiplayer, Constructing ASC/AS, Replication Mode |
| 04 - Attributes | Vital (Health/Mana) vs Primary (Strength/Intelligence) vs Secondary (Armor/Crit), Init via GE |
| 05 - RPG Game UI | Frame, Health/Mana globe, OverlayWidgetController pattern (MVC for GAS) |
| 06 - Gameplay Effects | Instant / Duration / Infinite, Period, Stacking |
| 07 - Gameplay Tags | Native tags, Tag query, Activation tag, Block tag |
| 08 - RPG Attributes | Resistance (Fire/Light/Arcane/Physical), Damage type, Vulnerability |
| 09 - Attribute Menu | Spend attribute points, Confirmation popup |
| 10 - **Gameplay Abilities** | GA lifecycle, Granting Abilities, Input Config DataAsset, **Aura Input Component**, callbacks, activate |
| 11 - Ability Tasks | `WaitTargetData`, `PlayMontageAndWait`, custom AbilityTask |
| 12 - RPG Character Classes | Warrior / Ranger / Elementalist data-driven setup |
| 13 - Damage | Damage Calculator (MMC, ExecutionCalculation), Source/Target |
| 14 - **Advanced Damage Techniques** | Block chance, Crit, Resistance, Armor, Debuff infliction |
| 15 - Enemy AI | Behavior Tree base setup, Aggro logic |
| 16 - Enemy Melee Attacks | Melee ability, Hit React |
| 17 - Enemy Ranged Attacks | Projectile, Lead target |
| 18 - Enemy Spell Attacks | Spell + cooldown |
| 19 - Enemy Finishing Touches | Death, ragdoll, drop XP/item |
| 20 - Level Tweaks | Level design balance |
| 21 - Cost and Cooldown | GA Cost / Cooldown GE |
| 22 - **Experience and Leveling Up** | XP curve, Level-up event, Stat scaling |
| 23 - Attribute Points | Spend on PrimaryAttribute, recompute Secondary |
| 24 - Spell Menu | Drag-drop assign spell to hotbar |
| 25 - Combat Tricks | Block, dodge, parry edge case |
| 26 - What a Shock | Lightning chain ability — beam, branch |
| 27 - **Passive Spells** | Always-on aura effect (Halo of Protection) |
| 28 - Arcane Shards | Multi-projectile + target select |
| 29 - Fire Blast | AoE ability + radial damage |
| 30 - Saving Progress | SaveGame integration with GAS attributes |
| 31 - Checkpoints | Map checkpoint actor, autosave |
| 32 - Map Entrance | Persistent world state, transition |
| 33 - Course Conclusion | Wrap, future work |

---

## 3. Cốt lõi (Pillars)

| Pillar | Mức |
|--------|-----|
| P8 GAS | 🔴 (deepest single course on GAS) |
| P2 Core Framework | 🟠 (PlayerState + GameMode + GameState pipeline) |
| P9 AI | 🟡 (BT + perception) |
| P10 UI | 🟠 (OverlayWidgetController pattern = MVC cho GAS) |
| P11 Inventory | 🟡 (basic inventory + equipment) |
| P12 Data-driven | 🟠 (CharacterClassInfo DataAsset, CurveTable XP) |
| P13 Save/Load | 🟠 (Save Progress + Checkpoints) |

**Pattern key:**
- **Widget Controller** (MVC): widget không poll attribute. Controller subscribe vào ASC attribute change delegate và broadcast lên widget.
- **MMC vs Execution Calc:** MMC dùng cho damage simple (multiplier), Execution dùng cho damage có nhiều source (block, crit, resistance).
- **Damage chain (chương 13–14):** event → ExecCalc → block/dodge → crit → resistance → final damage → apply effect.

---

## 4. Approach sư phạm

- **GAS đi từ ASC tới spell lưới phức:** spiral curriculum — chương 3 nhắc lại nhiều lần để học viên thấm.
- **Discord-Community.url + Git-Commit.url** trong mỗi lecture — học viên có thể `git checkout <commit>` để vào trạng thái cuối lecture đó.
- **Replication Mode** dạy sớm (chương 3 lecture 7) vì ảnh hưởng pattern multiplayer về sau.
- **Theory + practice mix:** chương Damage (13–14) có lecture lý thuyết + chương Spells (26–29) là 100% practice.

---

## 5. Đầu ra học viên

- Có **game action-RPG hoàn chỉnh** với 5+ spell, 3+ enemy type, level up, save, checkpoint.
- Hiểu được Damage chain production-grade.
- Đọc được Lyra GAS code (vì Lyra dùng GAS chính chủ).

---

## 6. Lưu ý

- **Không cover multiplayer GAS** sâu — phải lên [13].
- **Không cover dedicated server** — phải lên [08]/[13].
- **Không cover State Tree** — phải lên [15] Lyra RPG.
- **Inventory đơn giản** so với [09].

---

## 7. Liên kết khóa khác

| Trước | Sau |
|-------|-----|
| [05] GAS Crash, [01] Tom Action | [13] Crunch, [14] Lyra |
