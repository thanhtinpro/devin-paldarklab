# P04 — Enhanced Input

> Tự soạn từ: [01] ActionRoguelike, [11] Aura, [15] Lyra RPG.
>
> Pillar nhỏ nhưng quan trọng — input sai → mọi player action sai từ frame đầu.

---

## Pillar Identity

**Cái gì:** Dùng Enhanced Input system thay legacy Input Action / Axis.

**Scope:**
- ✅ `UInputAction` (IA) asset.
- ✅ `UInputMappingContext` (IMC) asset.
- ✅ Trigger (Down/Pressed/Tap/Hold/Chord/Released).
- ✅ Modifier (Scale, Negate, Deadzone, Swizzle).
- ✅ Bind ở C++ qua `UEnhancedInputComponent`.
- ✅ Per-character IMC swap (vehicle vs on-foot).

**Out of scope:**
- ❌ Mouse/touch UI input (P10).
- ❌ Ability binding via GAS InputAction (P08).

---

## PALDARK cần gì từ pillar này?

| IA | Mapping | Trigger |
|----|---------|---------|
| `IA_Move` | WASD + Stick | Vector2D, Tick |
| `IA_Look` | Mouse + Stick | Vector2D, Tick |
| `IA_Jump` | Space | Pressed |
| `IA_Sprint` | Shift | Hold |
| `IA_Crouch` | Ctrl | Tap toggle |
| `IA_PalCommand_Follow` | 1 | Pressed |
| `IA_PalCommand_Attack` | 2 | Pressed |
| `IA_PalCommand_Forage` | 3 | Pressed |
| `IA_PalCommand_Wait` | 4 | Pressed |
| `IA_PalBondAction` | Hold E | Hold 1.5s |
| `IA_Fire` | LMB | Down (auto fire) |
| `IA_Aim` | RMB | Hold |
| `IA_Reload` | R | Pressed |
| `IA_Inventory` | Tab | Pressed (toggle UI) |
| `IA_Interact` | F | Pressed |
| `IA_ExtractCall` | Hold V | Hold 3s |

**IMC list:** `IMC_OnFoot`, `IMC_Inventory_Active`, `IMC_Vehicle` (future Q4).

---

## Sources Matrix

| Source | Coverage | Lesson lấy từ đó |
|--------|----------|------------------|
| [01] ActionRoguelike | 🟡 | L1 Basic IA + IMC |
| [11] Aura | 🟢 | L2 InputConfigDataAsset + Custom Input Component |
| [15] Lyra RPG | 🟢 | L3 Lyra Input Config + Pawn Data integration |

---

## Prerequisite

- P01 (UCLASS).
- P02 (PlayerController owns input).

---

## Lessons

### L1 — Basic IA + IMC + Trigger 🧪

**Goal:** Tạo IA Move + IA Jump, bind ở C++, PIE chạy được.

**Concept:**
- Tạo IA asset trong Content (Right-click → Input → Input Action).
- ValueType: Bool / Float / Vector2D / Vector3D.
- Tạo IMC asset (Input Mapping Context).
- IMC chứa: IA + Key + Trigger array + Modifier array.
- Trong PC `BeginPlay`, gọi `Subsystem->AddMappingContext(IMC, Priority)`.

**Source learning path:**
- [01] ActionRoguelike `SCharacter.cpp::SetupPlayerInputComponent`.

**API / Class chính:**
- `UEnhancedInputLocalPlayerSubsystem`
- `UEnhancedInputComponent::BindAction(IA, ETriggerEvent, this, &Class::Func)`
- `FInputActionValue::Get<FVector2D>()`

**Practice exercise (2 hour):**
1. Tạo `IA_LabMove` (Vector2D), `IA_LabJump` (Bool).
2. Tạo `IMC_LabOnFoot` map WASD + Space.
3. `APaldarkLabCharacter::SetupPlayerInputComponent` bind 2 IA.
4. PIE → WASD move, Space jump.

**PALDARK take-away:**
- 🟢 Mỗi action 1 IA — không "InputAction giant".
- 🟢 IMC theo state — OnFoot vs Inventory_Active vs Vehicle.

**Apply ở week:** Roadmap week 4.

---

### L2 — InputConfigDataAsset + Custom EnhancedInputComponent (Aura) 🧪

**Goal:** Data-driven input binding qua DataAsset thay vì hardcode.

**Concept:**
- `UInputConfig` PrimaryDataAsset chứa `TArray<FInputAction>` với GameplayTag.
- `UPaldarkInputComponent` thừa kế `UEnhancedInputComponent`.
- Method `BindAbilityActions(InputConfig, Object, PressedFunc, ReleasedFunc, HeldFunc)`.
- Mỗi IA pair với GameplayTag → forward tag vào ability.

**Source learning path:**
- [11] Aura `Source/Aura/Public/Input/AuraInputComponent.h` + `AuraInputConfig.h`.
- Cross-ref: `Documents/GameDesign/11-Aura_RPG_GDD.md`.

**API / Class chính:**
- `UInputConfig::FindAbilityInputActionForTag(FGameplayTag)`
- `DECLARE_DELEGATE_OneParam(FAbilityInputDelegate, FGameplayTag)`

**Practice exercise (3 hour):**
1. Tạo `UPaldarkLabInputConfig` PrimaryDataAsset với `TArray<FPaldarkInputAction>`.
2. Mỗi entry: `UInputAction* IA + FGameplayTag InputTag`.
3. `UPaldarkLabInputComponent::BindAbilityActions(InputConfig, this, &APaldarkLabCharacter::AbilityInputPressed, ...)`.
4. PIE: nhấn 1 → log "InputTag.Pal.Command.Follow pressed".

**PALDARK take-away:**
- 🟢 **CORE pattern** — `UPaldarkInputConfig` chứa toàn bộ 16 IA + tag.
- 🟢 Forward tag → `UPaldarkPalCompanionComponent::HandleCommand(Tag)` clean abstraction.

**Apply ở week:** Roadmap week 4-5.

---

### L3 — Per-state IMC swap (Lyra style) 🧪

**Goal:** Đổi IMC khi state đổi (OnFoot ↔ Inventory_Active ↔ Vehicle).

**Concept:**
- Player có nhiều state: OnFoot, Inventory open, Vehicle riding.
- Mỗi state có IMC riêng (priority khác nhau).
- Khi vào Inventory → `AddMappingContext(IMC_Inventory, Priority=2)` đè IMC_OnFoot.
- Khi đóng Inventory → `RemoveMappingContext(IMC_Inventory)`.

**Source learning path:**
- [15] Lyra RPG — FrontEnd state component swap IMC.

**Practice exercise:**
1. Tạo `IMC_LabOnFoot` (WASD), `IMC_LabInventory` (Tab close + drag UI).
2. PC method `EnterInventoryMode()` swap IMC.
3. PIE: Tab → IMC swap → WASD không move nữa.

**PALDARK take-away:**
- 🟢 IMC_OnFoot (priority 0) + IMC_Inventory_Active (priority 5) + IMC_PalCommand_Active (priority 3).
- 🟢 Hold E để vào Pal Command mode → add IMC_PalCommand → 1/2/3/4 commands.

**Apply ở week:** Roadmap week 7 (Pal Command UI).

---

### L4 — Trigger + Modifier nâng cao 📖🧪

**Goal:** Dùng Hold Trigger, Chord Trigger, Modifier để input phong phú.

**Concept:**
- **Trigger types:**
  - Down — frame which key down.
  - Pressed — frame đầu down.
  - Released — frame buông.
  - Tap — short press (< 0.5s).
  - Hold — held ≥ X giây.
  - Chord — kết hợp với IA khác đang active.
- **Modifier:**
  - Scale — multiply value.
  - Negate — đảo dấu.
  - Deadzone — bỏ stick drift.
  - Swizzle — swap XY axis.

**Source learning path:**
- [Epic Enhanced Input doc](https://docs.unrealengine.com/5.3/en-US/enhanced-input-in-unreal-engine/).

**Practice exercise:**
1. `IA_LabBondAction` với Hold 1.5s trigger.
2. PIE: press E < 1.5s không trigger; ≥ 1.5s trigger.

**PALDARK take-away:**
- 🟢 `IA_PalBondAction` Hold 1.5s — không misclick.
- 🟢 `IA_ExtractCall` Hold 3s — irreversible decision.
- 🟢 `IA_Sprint` Hold trigger (release stop sprint).

**Apply ở week:** Roadmap week 5.

---

## ⚡ Capstone Exercise

**Goal:** PaldarkLab full input setup với 16 IA + 3 IMC.

**Yêu cầu:**
1. 16 IA asset + 3 IMC asset.
2. `UPaldarkLabInputConfig` PrimaryDataAsset chứa toàn bộ.
3. `UPaldarkLabInputComponent` custom bind.
4. Hold E 1.5s trigger BondAction log "Bonded".
5. Hold V 3s trigger ExtractCall log "Extract requested".
6. PIE: switch IMC OnFoot → Inventory → PalCommand mode.

**Acceptance:** All 16 IA tested OK, no input miss.

**Effort:** 2 ngày.

---

## Khi apply vào PALDARK roadmap

| Week | Pillar P04 usage |
|------|------------------|
| W4 | Basic IA_Move + IA_Look + IA_Jump |
| W5 | IA_Pal Command 1-4 + Hold trigger |
| W7 | IMC swap (OnFoot ↔ Inventory) |
| W17 | IA_Fire/Aim/Reload cho weapon |

---

## Anti-patterns (cảnh báo)

- 🔴 Legacy Input Action / Axis trong `DefaultInput.ini` — deprecated.
- 🔴 Bind input trực tiếp ở Pawn `SetupPlayerInputComponent` mà không qua InputConfig.
- 🔴 1 IA gấp 3 action (Move + Sprint + Crouch chung) — split ra.
- 🔴 Hardcode key trong code thay vì IMC asset — rebindable fail.

---

## Tham chiếu

- [`Documents/UE5_Core_Pillars.md`](../UE5_Core_Pillars.md) § P4.
- [`Documents/GameDesign/11-Aura_RPG_GDD.md`](../GameDesign/11-Aura_RPG_GDD.md) — InputConfig pattern.
- [`Documents/PALDARK/01-GameDesign.md`](../PALDARK/01-GameDesign.md) — 16 IA list.
