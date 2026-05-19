# 05 — GAS Crash Course (Donchitos Reverse GDD)

> Source: [github.com/DruidMech/GASCrashCourse](https://github.com/DruidMech/GASCrashCourse) — Stephen Ulibarri, [Udemy](https://www.udemy.com/course/ue5-gas-crash-course/).
>
> Đây là **tech demo dạy GAS** — sample game đơn giản: char đi, bắn projectile, applies effect, attribute change.

---

## Identity Card

| Field | Value |
|-------|-------|
| 📦 Type | Course (tech demo) |
| Genre | Tech demo, không phải game đầy đủ |
| Engine | UE5 |
| Đóng góp PALDARK | GAS basic vocabulary — ASC, AS, GA, GE, Tag, Cue |

---

## Phase 1 — Brainstorm (reverse)

### Implicit pitch
> *"Đây không phải một game — đây là demo GAS có thể chạy. Bạn nhìn projectile, bạn áp effect, attribute thay đổi, UI binding."*

### Cảm xúc cốt lõi (tech demo)
- **Aha moment** khi hiểu tách rời cause-effect.
- **Modular feeling** — tag thay vì hard-code.
- **Data-driven satisfaction**.

### Vì sao có sample game này
- Sample đơn giản nhất có thể để dạy GAS không bị overshadow bởi gameplay phức tạp.

---

## Phase 2 — Decompose

### Verb cốt lõi (demo)
- Move
- Aim
- Fire projectile (Gameplay Ability)
- Hit target → Apply Gameplay Effect
- Attribute change → UI update

### System hierarchy

```
┌─────────────────────────┐
│ Character with ASC      │
│ AttributeSet            │
└──────────┬──────────────┘
           │
   ┌───────┴─────────┬────────────┬───────────┐
   ▼                 ▼            ▼           ▼
GameplayAbility   GameplayEffect  Tag      Cue
 (Projectile)     (Damage)        IDs      (Burst VFX)
```

---

## Phase 3 — Define (GDD reverse)

### Game Loop (sample)
1. Char move.
2. LMB → fire projectile (GA).
3. Hit → apply GE (instant damage).
4. Health attribute change.
5. HUD update.

### Player Character
- ASC + AS attached.
- Input Action → Granted Ability.

### Ability set
- **Projectile** — spawn projectile, on-hit apply GE.
- **Heal** — apply heal GE.
- **Buff** — temporary attribute boost.

### Attribute Set
- Health, Mana, MaxHealth, MaxMana.
- Replicated for MP context.

### Tags
- `Ability.Projectile`
- `Effect.Damage.Fire`
- `State.Stunned`

### Cue
- `GameplayCue.Hit.Burst` → spawn VFX.

---

## Phase 4 — 3 Game Pillars (inferred from course design)

### Pillar 1 — Decouple Cause and Effect
> *"Cause = Ability. Effect = GE. Không nối chặt."*

### Pillar 2 — Tag the World
> *"Tag là ngôn ngữ chung. Không dùng enum."*

### Pillar 3 — Data Drives Behavior
> *"Hành vi đổi không qua code — qua DataAsset GE."*

---

## Phase 5 — Architecture Decisions (implicit)

### ADR-1: ASC ở Pawn (không phải PlayerState)
- **Quyết định:** AbilitySystemComponent attach trực tiếp Pawn.
- **Lý do:** Course đơn giản, không cần persistent across respawn.
- **Hệ quả:** Reset on respawn.
- **PALDARK adopt:** 🟡 **PALDARK chọn PlayerState ASC cho persistence** (theo Aura, Lyra style).

### ADR-2: Native Tag enum (FGameplayTagsSingleton chưa dùng)
- **Quyết định:** Native global FGameplayTags struct cho fast access.
- **PALDARK adopt:** 🟢 — `FPaldarkGameplayTags` singleton.

### ADR-3: Granted Ability on PossessedBy
- **Quyết định:** GiveAbility() chạy server-side khi Pawn possessed.
- **PALDARK adopt:** 🟢.

---

## Phase 6 — System Map

| System | Class | File |
|--------|-------|------|
| Character | `AGASCrashCharacter` | `Character/GASCrashCharacter.h/cpp` |
| ASC | `UGCAbilitySystemComponent` | `AbilitySystem/GCAbilitySystemComponent.h/cpp` |
| Attribute | `UGCAttributeSet` | `AbilitySystem/GCAttributeSet.h/cpp` |
| Ability | `UGameplayAbility_Projectile` | `Abilities/...` |
| Effect | `UGameplayEffect_Damage` | DataAsset / BP |
| Cue | `UGameplayCueNotify_Burst` | BP |
| Tag Singleton | `FGCGameplayTags` | `GAS/GCGameplayTags.h` |

---

## Phase 7 — Stories taught (course chapters as US)

| Chapter | US |
|---------|----|
| 1 | Là dev, tôi setup module GAS + dependency |
| 2 | Là dev, tôi tạo ASC subclass + AS subclass |
| 3 | Là dev, tôi grant ability on PossessedBy |
| 4 | Là dev, tôi viết GameplayAbility kích hoạt bằng input |
| 5 | Là dev, tôi spawn projectile từ GA |
| 6 | Là dev, tôi apply GameplayEffect on-hit |
| 7 | Là dev, tôi expose Tag singleton |
| 8 | Là dev, tôi viết GameplayCue cho VFX |
| 9 | Là dev, tôi bind Attribute → UI |

---

## Take-aways cho PALDARK

### 🟢 Copy nguyên
- **GAS vocabulary** — vào PALDARK ngay từ ngày 1.
- **Native Tag singleton** → `FPaldarkGameplayTags`.
- **Ability/Effect/Cue separation**.

### 🟡 Adapt
- **ASC location** → PALDARK chọn PlayerState (Aura/Lyra style).

---

## Pillar Mapping

| Pillar | Đóng góp |
|--------|----------|
| P8 | 🟢 **GAS basic vocabulary** |
| P12 | 🟠 DataAsset effect |
| P10 | 🟠 Attribute → UI binding |

---

## Tham chiếu

- [Courses/05-Udemy-ue5-gas-crash-course.md](../Courses/05-Udemy-ue5-gas-crash-course.md)
- [11-Aura_RPG_GDD.md](11-Aura_RPG_GDD.md) — apply GAS sâu hơn.
