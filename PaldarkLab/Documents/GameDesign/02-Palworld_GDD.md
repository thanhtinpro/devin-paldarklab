# 02 — Palworld (Donchitos Reverse GDD)

> Source: [github.com/KYWorld/PalworldProject](https://github.com/KYWorld/PalworldProject) (Korean fan/learning project)
>
> Repo này là **skeleton recreation** — class hierarchy đúng vibe Palworld nhưng phần lớn **rỗng**. Coverage ~21%.

---

## Identity Card

| Field | Value |
|-------|-------|
| 📦 Type | Skeleton (correct intent, empty implementation) |
| Genre | Open-world creature-collect survival co-op |
| Engine | UE5.4 |
| Đóng góp PALDARK | Base codebase + intent — cần fill in |

---

## Phase 1 — Brainstorm (reverse)

### Implicit pitch
> *"Pokémon + Ark + Stardew Valley. Bắt creature, train chúng, xây base, sinh tồn co-op."*

### Cảm xúc cốt lõi
- **Phấn khích** khi gặp Pal lạ.
- **Sở hữu** khi tame thành công.
- **Mưu mô** khi xây base + chuỗi sản xuất.
- **Cool factor** khi Pal mạnh.

### Vì sao thành công
- Combine 4 mechanic chưa từng combine → tạo "weird" buzz.
- Memes (Pal lao động, súng, ăn Pal) tạo viral.
- Co-op survival dễ-vào.

---

## Phase 2 — Decompose

### Verb cốt lõi
- Roam (open-world third-person)
- Capture (Pal Sphere → tame)
- Combat (gun + Pal ability)
- Build (base building)
- Craft (recipe-driven)
- Harvest (Pal labor)
- Sleep (save + heal)

### System hierarchy

```
┌────────────────────────────┐
│ APalworld_BaseGameMode     │ World rule
│ APalworld_BasePlayerState  │ Per-player persist
└──────────────┬─────────────┘
               │
   ┌───────────┼───────────────┬─────────────────┬──────────────┐
   ▼           ▼               ▼                 ▼              ▼
PlayerChar  PalChar       BaseBuild       Inventory       SurvivalSys
 ↓           ↓                 ↓               ↓              ↓
 Pawn       Pawn          Mesh+attach     Component         Stats
 Ext.Comp   Activity                       (skeleton)      (skeleton)
 Equip.Comp Component
 Inventory  Inventory
 Comp       Comp
```

---

## Phase 3 — Define (GDD reverse — từ skeleton)

### Game Loop (intended)
1. Spawn world.
2. Roam + explore POI.
3. Encounter Pal → battle.
4. Throw Pal Sphere → capture attempt.
5. Tame → Pal join party.
6. Return base → Pal labor.
7. Sleep → save → progress next day.
8. Co-op: friend join, share base.

### Player Character (`APalworld_BasePlayerCharacter`)
- Third-person.
- Slot: 3 weapon + 5 Pal.
- 4 stat: Health, Stamina, Hunger, Sanity.

### Pal Character (`APalworld_BasePalCharacter`)
- Pawn extension component (Lyra-ish style).
- Activity component (skeleton — chưa implement).
- Equipment component (item, accessory).
- Inventory component.

### Capture
- `UInventoryComponentBase` (rỗng).
- Pal Sphere throw mechanic.
- Tame success = Pal vào party.

### Base building
- (skeleton — chưa có mesh attach logic).

### Combat
- Skeleton GAS: `UBaseAttributeSet`, `UBaseAbilitySystemComponent`, `UBaseGameplayAbility`.
- Damage chain chưa có.

### UI
- `UWidgetBase` (rỗng).
- HUD skeleton.

---

## Phase 4 — 3 Game Pillars (inferred từ Palworld thực tế)

### Pillar 1 — Capture Transcends Combat
> *"Trận đấu không nhằm tiêu diệt — nhằm thu thập."*
- Damage attribute lowering → tăng chance capture.
- Mỗi Pal = unique gameplay (mỗi loài 3–5 ability).

### Pillar 2 — Friendship Has Utility
> *"Pal không phải pet. Pal là công nhân, là chiến hữu, là người bay."*
- Pal labor base.
- Pal mount (cưỡi).
- Pal combat partner.

### Pillar 3 — World Invites Curiosity
> *"Ai cũng muốn biết Pal tiếp theo trông như nào."*
- 100+ loài Pal trong full game.
- POI signal khám phá.

---

## Phase 5 — Architecture Decisions (implicit, qua skeleton)

### ADR-1: Component skeleton trước — fill in sau
- **Quyết định:** Tạo class rỗng (PawnExtension, Equipment, Inventory) với UPROPERTY khai báo, nhưng chưa code logic.
- **Lý do:** Tài liệu hoá intent cho team / cộng đồng.
- **Hệ quả:** Người mới đọc hiểu hướng nhưng không có gameplay.
- **PALDARK adopt:** 🟢 Cách layout component (Pawn + Pal đều có suite Component) → copy.

### ADR-2: GAS từ đầu (skeleton)
- **Quyết định:** `UBaseAbilitySystemComponent` + `UBaseAttributeSet` ở base class.
- **Lý do:** Game design có ability theo loài Pal → GAS phù hợp.
- **Hệ quả:** Đúng hướng nhưng chưa fill.
- **PALDARK adopt:** ✅ — GAS từ đầu, ASC trên Pawn.

### ADR-3: Open-world map không streamed
- **Quyết định:** Map mở (như Palworld) — yêu cầu World Partition.
- **Hệ quả:** Performance nặng nếu thiếu Significance.
- **PALDARK adopt:** 🔴 Skip — PALDARK Dark Zone là 1km×1km closed map, KHÔNG open-world beta.

### ADR-4: Inventory base class skeleton
- **Quyết định:** `UInventoryComponentBase` rỗng.
- **Hệ quả:** Cần fill (xem course [09] làm reference).
- **PALDARK adopt:** 🟡 Adapt — dùng pattern Fragment ([09]) cho inventory PALDARK.

---

## Phase 6 — System Map

| System | Class | File | Status |
|--------|-------|------|--------|
| GameMode | `APalworld_BaseGameMode` | `BaseGameMode.h/cpp` | 🔴 rỗng |
| Character | `APalworld_BasePlayerCharacter` | `BasePlayerCharacter.h/cpp` | 🟡 partial |
| Pal | `APalworld_BasePalCharacter` | `BasePalCharacter.h/cpp` | 🟡 partial |
| Inventory | `UInventoryComponentBase` | `InventoryComponentBase.h` | 🔴 rỗng |
| GAS | `UBaseAbilitySystemComponent` + `UBaseAttributeSet` | `BaseAbilitySystemComponent.h/cpp` | 🔴 rỗng |
| UI | `UWidgetBase` | `WidgetBase.h/cpp` | 🔴 rỗng |
| GameInstance | `UPalworld_GameInstance` | (chưa có) | 🔴 missing |

(Xem `Documents/Refactor_Plan.md` từ PR #2 — kế hoạch fill skeleton từng phase.)

---

## Phase 7 — Stories taught (course-style)

Vì là skeleton, **không có lesson tự nhiên** — nhưng có thể chuyển hoá thành lesson tự build:

| US | Mô tả | Ghi chú |
|----|-------|---------|
| US-1 | Là dev, tôi fill `UInventoryComponentBase` với add/remove/move item | Học từ [09] Inventory |
| US-2 | Là dev, tôi fill `UBaseAttributeSet` với Health/Stamina/Hunger/Sanity | Học từ [11] Aura |
| US-3 | Là dev, tôi viết Pal capture flow (sphere throw + chance calc) | Game-specific |
| US-4 | Là dev, tôi setup base building (mesh attach + grid) | Cần thư viện riêng |
| US-5 | Là dev, tôi setup co-op replicate | Học từ [07] MP Crash + [10] |
| US-6 | Là dev, tôi setup AI Activity FSM cho Pal labor | Học từ [04] RoN |

> **Trở thành PALDARK** không phải bằng fill toàn bộ Palworld feature, mà **đổi genre** sang extraction tactical.

---

## Take-aways cho PALDARK

### 🟢 Copy nguyên
- **Component slot layout:** Player + Pal đều có suite (Extension, Equipment, Inventory, AbilitySystem) → PALDARK extend thành 12+8 component.
- **GAS từ đầu:** ASC ở Pawn level.
- **Lyra-style modular intent** dù chưa implement.

### 🟡 Adapt
- **Inventory skeleton** → fill bằng Fragment pattern từ [09].
- **GAS skeleton** → fill bằng pattern từ [11] Aura.

### 🔴 Avoid
- **Open-world full streamed:** PALDARK closed 1km map → đơn giản hoá performance.
- **Empty class with no logic:** PALDARK mỗi component code đầy đủ trước commit.

---

## Pillar Mapping — đóng góp của source này

| Pillar | Đóng góp |
|--------|----------|
| P1 | 🟢 Build config base |
| P2 | 🟡 GameMode skeleton (cần extend) |
| P3 | 🟢 **Component layout intent** |
| P4 | 🟠 Input partial |
| P5 | 🟠 Animation skeleton |
| P8 | 🟡 GAS skeleton intent |
| P11 | 🔴 Inventory skeleton rỗng |
| Others | 🔴 Empty |

---

## Liên kết với PALDARK

PALDARK = **fork** của Palworld skeleton nhưng:
- **Drop:** open-world streaming, base building feature creep, mount, harvest.
- **Keep:** Component layout intent, GAS-from-start, Pal-companion concept.
- **Add:** Extraction loop, dedicated server, Activity AI, lag comp, FMOD propagation, AWS backend, Lyra backbone.

---

## Tham chiếu

- [Courses/02-Palworld.md](../Courses/02-Palworld.md) — assessment chi tiết.
- [Projects/Palworld_Assessment.md](../Projects/Palworld_Assessment.md) — 21% coverage breakdown.
- `Documents/Refactor_Plan.md` (track ở PR #2, chưa merge vào `main`) — 10-phase plan fill skeleton.
- [PALDARK/00-VISION.md](../PALDARK/00-VISION.md) — fork direction.
