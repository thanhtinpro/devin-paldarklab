# Lesson 07 — Damage Pipeline (SetByCaller + ExecutionCalculation + diminishing-returns Armor)

## Câu hỏi cốt lõi
**Vì sao damage = `Base × 100/(100+Armor)` (diminishing returns) thay vì `Base − Armor` (linear)?**

## WHY — Bản chất

### Linear `Damage - Armor` phá ở extremes
| Armor | Damage=30 (linear) | Damage=30 (DR) |
|-------|--------------------|----------------|
| 0     | 30                 | 30.00          |
| 50    | 0 (cliff!)         | 20.00          |
| 100   | 0                  | 15.00          |
| 200   | 0                  | 10.00          |
| 400   | 0                  | 6.00           |

Linear biến armor thành **binary cliff**: dưới ngưỡng = full damage, vượt ngưỡng = miễn nhiễm. Designer phải tune từng vũ khí với từng tier armor. Bullet vs heavy armor = không thể chỉnh.

### Diminishing returns `100/(100+Armor)` luôn cho phép damage qua
- Armor=0 → mitigation 1.0
- Armor=100 → 0.5 (50% giảm) ← "100 armor = double effective HP"
- Armor=200 → 0.333
- Armor=400 → 0.2
- Armor=900 → 0.1

Property quan trọng: **effective HP scale tuyến tính với armor**. Cứ thêm 100 armor = thêm 100% effective HP (tương đối base 100). Predictable cho cả designer và player. Không bao giờ miễn nhiễm tuyệt đối (`Mitigation > 0` luôn).

Đây là pattern Dota/LoL/Lyra — không phải Paldark phát minh, mà chọn vì property này là **invariant universal** cho mọi tier vũ khí/armor.

### WHY ExecutionCalculation (không phải ModifierMagnitude)
`FAttributeBasedFloat` magnitude chỉ đọc **một** attribute. Damage formula cần:
1. BaseDamage (từ caller, không phải attribute) — qua **SetByCaller**.
2. Target.Armor — qua **captured attribute**.
3. DamageType tag (true/physical) — qua **DynamicGrantedTags trên spec**.

Multi-input → cần `UGameplayEffectExecutionCalculation`. ExecutionCalc cho phép gộp tất cả nguồn dữ liệu này, áp formula tuỳ ý, output modifier sang attribute đích.

### WHY SetByCaller (không phải hardcode magnitude trong GE)
Nếu mỗi vũ khí cần một `UGE_DamageBullet50`, `UGE_DamageSniper200`, `UGE_DamageRPG500`... **N vũ khí = N GE asset**. SetByCaller cho phép **1 GE_Damage class** nhận damage value từ caller qua spec:

```cpp
FGameplayEffectSpec Spec(GE, Ctx, 1.f);
Spec.SetSetByCallerMagnitude(TAG_SetByCaller_Damage, WeaponDamage);
ASC->ApplyGameplayEffectSpecToSelf(Spec);
```

Designer chỉ edit `WeaponDamage` row trong DataAsset, không tạo GE mới.

### WHY meta-attribute IncomingDamage (gắn vào PR-06)
Execution **không** ghi thẳng Health. Nó deposit `IncomingDamage` (additive). PR-06 `PostGameplayEffectExecute` mới drain `IncomingDamage` → `Health -= ...` qua `PreAttributeChange` clamp + `OnHealthZeroed` broadcast.

**Tại sao một bước nữa?** Vì Health invariant (clamp [0, MaxHealth]) phải ở **một chỗ**. Nếu Execution ghi thẳng Health, phải duplicate clamp logic. Meta-attribute giữ single chokepoint.

## Flow

```mermaid
flowchart TD
  C[Caller: Weapon.OnHit, BaseDamage=50] --> S[Build FGameplayEffectSpec]
  S --> Sb[Spec.SetSetByCallerMagnitude&#40;TAG_SetByCaller_Damage, 50&#41;]
  Sb --> St[Optional: Spec.DynamicGrantedTags.Add&#40;TAG_DamageType_True&#41;]
  St --> Apply[ASC.ApplyGameplayEffectSpecToSelf]
  Apply --> Exec[UTestDamageExecution.Execute]
  Exec --> RA[Read SetByCaller -- 50]
  Exec --> RB[Capture Target.Armor -- e.g. 100]
  Exec --> RC[Check Spec.DynamicGrantedTags has True?]
  RA --> M[Mitigation: bTrue ? 1 : 100/(100+Armor)]
  RB --> M
  RC --> M
  M --> O[Final = Base × Mitigation]
  O --> Out[AddOutputModifier IncomingDamage Additive Final]
  Out --> PostExec[PR-06 PostGameplayEffectExecute drains IncomingDamage -> Health]
  PostExec --> Death{Health <= 0?}
  Death -- yes --> Z[OnHealthZeroed.Broadcast]
```

## Test plan

Mở Editor → Play (PIE). `UTestDamageDriver` (UWorldSubsystem) spawn `ATestAttrPawn` (PR-06), defer 1 tick, chạy 7 TCs. Filter Output Log: `LogSandboxDamage` + `LogSandboxAttr` (để thấy PR-06 drain log).

| # | Bước reproduce                                              | Assertion observable                                                                       | PASS criteria               |
|---|-------------------------------------------------------------|--------------------------------------------------------------------------------------------|-----------------------------|
| 1 | `SetArmor(0); ApplyDamage(50, false)`                       | Mitigation=1.0 → Final=50 → Health 100→50                                                  | `[TC1] ... PASS`            |
| 2 | `SetArmor(50); ApplyDamage(50, false)`                      | Mitigation=100/150 → Final≈33.33 → Health 100→66.67                                        | `[TC2] ... PASS`            |
| 3 | `SetArmor(100); ApplyDamage(50, false)`                     | Mitigation=0.5 → Final=25 → Health 100→75                                                  | `[TC3] ... PASS`            |
| 4 | `Armor=100; ApplyDamage(100); RestoreHealth; ApplyDamage(200)` | Final 50 và 100 → Health 50 và 0 (linearity in BaseDamage)                              | `[TC4] ... PASS`            |
| 5 | (math-only) Compute Mit(0)/Mit(100)/Mit(200)/Mit(400)        | 1.0, 0.5, 0.333, 0.2 — monotonic decreasing, never 0                                       | `[TC5] ... PASS`            |
| 6 | `SetArmor(100); ApplyDamage(50, bTrue=true)`                | Mitigation forced 1.0 → Health 100→50 (bypass armor)                                       | `[TC6] ... PASS`            |
| 7 | `SetArmor(100); ApplyDamage(99999, false)`                  | Health clamps to 0; `OnHealthZeroed` từ PR-06 fire đúng 1 lần (pipeline chain intact)      | `[TC7] ... PASS`            |

## Expected output (đoạn quan trọng)

```
LogSandboxDamage: === Lesson07 DamagePipeline :: RUN ALL TESTS ===
LogSandboxDamage: Execution: Base=50.00 Armor=0.00 bTrue=0 -> Mitigation=1.0000 Final=50.00
LogSandboxAttr: PostGameplayEffectExecute: IncomingDamage=50.00 translated -> Health 100.00 -> 50.00
LogSandboxDamage: [TC1] Armor=0 Base=50 -> Health=50.00 (expect 50.00): PASS

LogSandboxDamage: Execution: Base=50.00 Armor=50.00 bTrue=0 -> Mitigation=0.6667 Final=33.33
LogSandboxAttr: PostGameplayEffectExecute: IncomingDamage=33.33 translated -> Health 100.00 -> 66.67
LogSandboxDamage: [TC2] Armor=50 Base=50 -> Health=66.67 (expect ~66.67): PASS

... (TC3-7 tương tự)

LogSandboxDamage: Execution: Base=50.00 Armor=100.00 bTrue=1 -> Mitigation=1.0000 Final=50.00
LogSandboxDamage: [TC6] True damage: Armor=100 Base=50 bTrue -> Health=50.00 (expect 50.00, bypass armor): PASS

LogSandboxDamage: Execution: Base=99999.00 Armor=100.00 bTrue=0 -> Mitigation=0.5000 Final=49999.50
LogSandboxAttr: PostGameplayEffectExecute: Health hit zero -> broadcasting OnHealthZeroed
LogSandboxDamage: [TC7] Lethal: Health=0.00, OnHealthZeroed count=1 (delta 1): PASS
```

## Cách chứng minh thủ công

1. **Đổi sang linear formula:** Trong `TestDamageExecution.cpp`, thay `100.f / (100.f + Armor)` bằng `FMath::Max(0.f, 1.f - Armor / 100.f)`. Run lại:
   - TC2 (Armor=50): linear → Mit=0.5 → Final=25 → Health=75 (khác DR). Mathematically OK.
   - TC3 (Armor=100): linear → Mit=0.0 → **Final=0 → Health=100 (no damage!)**. TC3 FAIL nhưng cho thấy cliff effect.
   - TC5 (math): không còn diminishing — Mit(200)=−1.0 (clamped 0). Property "always > 0" mất.
2. **Skip SetByCaller, hardcode 50:** Trong Execution, thay `Spec.GetSetByCallerMagnitude(...)` bằng `50.f`. TC4 sẽ FAIL (Base=200 cũng chỉ ra 50). **Đây là lý do SetByCaller tồn tại — caller-controlled magnitude.**
3. **Skip true-damage check:** Comment dòng `bTrueDamage = Spec.DynamicGrantedTags...`. TC6 FAIL (true damage không bypass nữa). Tag-driven branching trong Execution là pattern chuẩn cho damage type.

## Placeholder mapping (sandbox → thực tế)

| Sandbox                                | Trong PaldarkLab thật                                                  |
|----------------------------------------|------------------------------------------------------------------------|
| `Sandbox.SetByCaller.Damage`           | `Data.Damage` hoặc per-source: `Data.Damage.Weapon`, `Data.Damage.Skill` |
| `Sandbox.DamageType.True`              | `DamageType.True`, `DamageType.Physical`, `DamageType.Fire`, ... (mỗi cái có resistance attribute riêng) |
| Single `UTestDamageExecution`          | `UPaldarkDamageExecution` + có thể `UPaldarkHealExecution`, `UPaldarkPoisonExecution` cho mỗi loại |
| Capture chỉ `Target.Armor`             | Production capture thêm: Source.AttackPower, Target.Resist.Fire, GE.Level qua FAttributeBasedFloat |
| `Spec.DynamicGrantedTags` cho damage type | Có thể là `Spec.SetByCallerTagMagnitudes` hoặc `GameplayEffectAssetTags`  |
| Runtime-built `GE_Damage`              | Cooked `UGE_Damage` Blueprint với Execution slot                       |

## Bridge đến các Lesson sau
- **Lesson 08 (Sprint Ability):** GameplayAbility kích `GE_DrainStamina` periodic (Cost effect). Cùng pattern: caller-controlled magnitude qua SetByCaller, Execution áp formula (bonus stamina drain khi sprint+jump).
- **Lesson 09 (Lag Compensation):** Lúc client hit được xác nhận, server replay hit bằng damage GE — cần đúng armor TẠI THỜI ĐIỂM HIT, không phải hiện tại. Captured attribute với `bSnapshot=false` (như sandbox) sẽ đọc fresh, nhưng để rewind cần `bSnapshot=true` hoặc query frame history. Lesson 09 sẽ giải.

## Câu hỏi mở (chuyển sang Lesson 08)
Damage pipeline hoạt động khi có GE. Nhưng "sprint" không phải single-tick damage — nó là **continuous state** drain stamina mỗi giây. Hardcode `Tick { Stamina -= 5 * dt }`? Không scale. → **UGameplayAbility với periodic GE_Cost.**
