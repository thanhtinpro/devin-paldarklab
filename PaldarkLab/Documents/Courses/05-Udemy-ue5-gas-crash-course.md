# 05. Unreal Engine 5 Gameplay Ability System (GAS) Crash Course

| | |
|---|---|
| **Tier** | T3 — Combat Systems / GAS Core |
| **Provider** | Stephen Ulibarri (DruidMech) — Udemy |
| **Link** | https://www.udemy.com/course/ue5-gas-crash-course/ |
| **Source** | https://github.com/DruidMech/GASCrashCourse |
| **Trong repo** | `05.Udemy-ue5-gas-crash-course/` (chỉ Source, không có Documents) |

---

## 1. Mục tiêu khóa

Crash course đúng nghĩa: từ 0 đến demo GAS chạy được. Đủ kiến thức để đọc/viết được:
- **AbilitySystemComponent** (ASC)
- **AttributeSet**
- **GameplayAbility + AbilityTask**
- **GameplayEffect**
- **GameplayTag**
- **GameplayCue**
- **Player/Enemy** dùng GAS, có **Melee + Hit React + Search target** ability

---

## 2. Lộ trình suy ra từ source

Khóa không có Documents folder. Curriculum suy từ thư mục `Source/CrashCourse/`:

```
CrashCourse/
├── Public/Private — mirrored layout
├── AbilitySystem/
│   ├── CC_AbilitySystemComponent.h/cpp   ← ASC kế thừa
│   ├── CC_AttributeSet.h/cpp             ← AttributeSet (Health, Mana...)
│   ├── Abilities/
│   │   ├── CC_GameplayAbility.h/cpp      ← base ability
│   │   ├── Player/CC_Primary.h/cpp       ← ability primary attack player
│   │   └── Enemy/
│   │       ├── CC_HitReact.h/cpp
│   │       └── CC_SearchForTarget.h/cpp
│   └── AbilityTasks/
│       └── CC_WaitGameplayEvent.h/cpp    ← custom AbilityTask
├── Characters/
│   ├── CC_BaseCharacter.h/cpp            ← base có ASC + AttributeSet
│   ├── CC_PlayerCharacter.h/cpp
│   └── CC_EnemyCharacter.h/cpp
├── GameObjects/
│   └── CC_Projectile.h/cpp
├── GameplayTags/
│   └── CCTags.h/cpp                      ← tag native registration
├── Notifies/
│   └── CC_MeleeAttack.h/cpp              ← AnimNotify trigger ability
├── Player/
│   ├── CC_PlayerController.h/cpp
│   └── CC_PlayerState.h/cpp              ← ASC giữ ở PlayerState
├── Tasks/
│   └── CC_AttributeChangeTask.h/cpp      ← async task observer attribute
├── UI/
│   ├── CC_AttributeWidget.h/cpp
│   └── CC_WidgetComponent.h/cpp
└── Utils/
    └── CC_BlueprintLibrary.h/cpp         ← static helper
```

**Pedagogical pathway** (chuẩn của DruidMech): 

1. Project creation (Third-person template).
2. Thêm **AbilitySystemComponent** vào PlayerState (player) và Character (enemy) — giảng giải vì sao tách.
3. **AttributeSet** — Health, MaxHealth, Attack... + Pre/PostAttributeChange.
4. **GameplayEffect** — Instant (damage), Duration, Infinite. Modifier types.
5. **GameplayAbility** — Activate, Commit, End, Cancel. Cost + Cooldown.
6. **AbilityTask** — `WaitTargetData`, `WaitGameplayEvent`, custom task `CC_WaitGameplayEvent`.
7. **GameplayTag** — native registration trong `CCTags.h/cpp`.
8. **GameplayCue** — visual/audio response to gameplay event.
9. **AnimNotify** trigger ability — `CC_MeleeAttack` notify gửi GameplayEvent → AbilityTask `WaitGameplayEvent` activate damage logic.
10. **AI enemy** — `CC_HitReact` + `CC_SearchForTarget` ability cho AI.
11. **UI** — `CC_AttributeWidget` bind vào AttributeSet change delegate.

---

## 3. Cốt lõi (Pillars)

| Pillar | Mức | Code |
|--------|-----|------|
| P2 Core Framework | 🟢 | PlayerState giữ ASC |
| P8 GAS | 🟡 (full đủ Crash) | `AbilitySystem/` |
| P10 UI | 🟢 | `CC_AttributeWidget` |
| P9 AI | 🟢 | Ability-based AI |
| P5 Animation | 🟢 | `CC_MeleeAttack` notify |

---

## 4. Approach sư phạm

- **Bottom-up:** ASC + AttributeSet → Effect → Ability → Task → Tag → Cue. Mỗi bước có demo.
- **Short:** so với [11] Aura RPG (33 chương), khóa này crash trong vài giờ.
- **Tập trung pattern, không quan tâm game design:** không có cốt truyện, chỉ player vs enemy.
- **PlayerState ASC vs Character ASC:** khóa dạy *replication mode* khác nhau (Minimal, Mixed, Full). Đây là cốt lõi để pick đúng cho game.

---

## 5. Đầu ra

- Có demo player melee + ranged + enemy HitReact + AttributeWidget UI.
- Hiểu khi nào dùng:
  - Instant effect vs Duration effect.
  - Single-modifier vs MMC vs Execution Calculation.
  - Replication Mode: **Minimal** (single-player only), **Mixed** (multiplayer with players seeing own data), **Full** (FPS-style).

---

## 6. Lưu ý

- Khóa **không cover damage chain phức tạp** (block, dodge, resistance, vulnerability). Phải lên [11] Aura RPG.
- Không cover **multiplayer GAS** sâu — chỉ giới thiệu replication mode. Lên [13] Crunch để thấy GAS multiplayer thật.
- Không có **Save** cho GAS attribute — lên [11] Aura RPG.

---

## 7. Liên kết khóa khác

| Trước | Sau |
|-------|-----|
| [01] Tom Action System | [11] Aura GAS RPG, [13] Crunch, [14] Lyra GAS |
