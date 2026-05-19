# 10 — C++ Multiplayer Shooter (Blaster) (Donchitos Reverse GDD)

> Source: [github.com/DruidMech/MultiplayerCourseBlasterGame](https://github.com/DruidMech/MultiplayerCourseBlasterGame) — Stephen Ulibarri, [Udemy](https://www.udemy.com/course/unreal-engine-5-cpp-multiplayer-shooter/).
>
> Sample game: **Blaster** — Lyra-style PvP shooter với lag compensation server-side rewind.

---

## Identity Card

| Field | Value |
|-------|-------|
| 📦 Type | Course (lớn nhất series) |
| Genre | PvP TPS shooter |
| Engine | UE5 |
| Đóng góp PALDARK | **Server-Side Rewind / Lag Compensation pattern** + weapon class hierarchy |

---

## Phase 1 — Brainstorm (reverse)

### Implicit pitch
> *"Bạn vào trận PvP TPS. Nhặt vũ khí, recoil, headshot. Lag không lừa được bạn — server rewind theo timestamp client thấy."*

### Cảm xúc cốt lõi
- **Trải nghiệm AAA** ở scale course.
- **Skill matters** — không phải ping wars.

---

## Phase 2 — Decompose

### Verb cốt lõi
- Spawn
- Move (sprint/crouch)
- Aim down sight
- Fire (hitscan + projectile + grenade)
- Reload
- Pickup weapon
- Take cover
- Respawn

### System hierarchy

```
┌──────────────────────────────────────┐
│ ABlasterCharacter (god ~OK at course)│
└──────────────┬───────────────────────┘
               │
   ┌───────────┼────────────┬─────────┬──────────┐
   ▼           ▼            ▼         ▼          ▼
CombatComp   LagCompComp  BuffComp  WeaponA  WeaponHistory
(weapon hold) (server-     (heal/    (base)   (frame buffer
              side rewind)  shield)             timestamp)
                                    │
                              ┌─────┼──────┐
                              ▼     ▼      ▼
                            Rifle  Pistol  Sniper
                            Hitscan Hitscan Projectile
                            +Grenade
```

---

## Phase 3 — Define (GDD reverse)

### Game Loop (sample)
1. Spawn at base.
2. Pick weapon.
3. Engage opponent.
4. Die → respawn after N seconds.
5. Match end at score threshold.

### Player Character
- TPS với CombatComponent.
- Stats: Health, Shield.

### Weapons
- Base class + Rifle/Pistol/Sniper/Grenade subclass.
- Recoil pattern data-driven.
- Reload animation montage.

### Combat Component
- Holds equipped weapon.
- Fires + handles reload.

### Lag Compensation
- **Frame Buffer** — client-side và server-side keep ring buffer (300ms history) of hitboxes per character.
- Client fires → packs `FireTimestamp`.
- Server receives → rewind all character hitboxes to that timestamp → check hit.
- If hit → confirm damage.

### Buffs (heal pickup, shield, jump boost)
- BuffComponent — apply temp stat mod.

---

## Phase 4 — 3 Game Pillars (inferred)

### Pillar 1 — Hit Registers Truth
> *"Bạn bắn trúng vì server thấy bạn bắn trúng — không phải lag."*

### Pillar 2 — Latency Lies, Server Tells
> *"Client experience là illusion. Truth ở server."*

### Pillar 3 — Cosmetic ≠ Power
> *"Skin không buff — kỹ năng quyết."*

---

## Phase 5 — Architecture Decisions (implicit)

### ADR-1: Server-Side Rewind / Lag Comp
- **Quyết định:** Component `ULagCompensationComponent` server-only.
- **Lý do:** Bắn buộc fair cho mọi ping.
- **Hệ quả:** Server CPU/mem cost cho buffer.
- **PALDARK adopt:** 🟢 **CORE** — copy nguyên `UPaldarkLagCompComp`.

### ADR-2: Weapon class hierarchy thay vì data-driven
- **Quyết định:** Subclass per-weapon thay vì DataAsset config.
- **Hệ quả:** OK cho course; full game nên data-driven.
- **PALDARK adopt:** 🟡 Adapt — PALDARK weapon = DataAsset + 1 base class behavior.

### ADR-3: ASC không dùng (custom Combat)
- **Quyết định:** Combat custom thay vì GAS.
- **PALDARK adopt:** 🔴 Skip — PALDARK GAS hết.

### ADR-4: Frame buffer 300ms
- **Quyết định:** Buffer kích thước fixed.
- **PALDARK adopt:** 🟢.

### ADR-5: HUD widget per-weapon
- **Quyết định:** Ammo/crosshair per-weapon class.
- **PALDARK adopt:** 🟡 — PALDARK shared HUD, weapon-specific overlay.

---

## Phase 6 — System Map

| System | Class | File |
|--------|-------|------|
| Char | `ABlasterCharacter` | `Character/BlasterCharacter.h/cpp` |
| Combat | `UCombatComponent` | `Components/CombatComponent.h/cpp` |
| Lag Comp | `ULagCompensationComponent` | `Components/LagCompensationComponent.h/cpp` |
| Weapon Base | `AWeapon` | `Weapon/Weapon.h/cpp` |
| Buff | `UBuffComponent` | `Components/BuffComponent.h/cpp` |
| HUD | `UBlasterHUD` | `HUD/BlasterHUD.h/cpp` |
| GameMode | `ABlasterGameMode` | `GameMode/BlasterGameMode.h/cpp` |

---

## Phase 7 — Stories taught

| Chapter | US |
|---------|----|
| 1 | Là dev, tôi setup Character + Camera + Combat Comp |
| 2 | Là dev, tôi viết Weapon base + equip flow |
| 3 | Là dev, tôi setup ADS (Aim Down Sight) + crosshair |
| 4 | Là dev, tôi replicate weapon equip + ammo |
| 5 | Là dev, tôi viết Reload với Animation Montage |
| 6 | Là dev, tôi setup hitscan fire + bullet decal |
| 7 | Là dev, tôi tạo Projectile + Grenade weapons |
| 8 | Là dev, tôi setup frame buffer (LagCompComp) |
| 9 | Là dev, tôi viết server rewind hitbox check |
| 10 | Là dev, tôi tích hợp Buff Comp (heal/shield) |
| 11 | Là dev, tôi viết Match flow + Score + Respawn |
| 12 | Là dev, tôi setup HUD + Death message |

---

## Take-aways cho PALDARK

### 🟢 Copy nguyên (CORE)
- **LagCompensationComponent + Frame Buffer** — PALDARK bắt buộc có (TTK ngắn, fairness critical).
- **CombatComponent pattern**.
- **Weapon Reload + ADS + Crosshair**.
- **Match flow (Spawn → Combat → Score → End)**.

### 🟡 Adapt
- **Weapon class hierarchy** → DataAsset-based.
- **Custom Combat** → GAS.

---

## Pillar Mapping

| Pillar | Đóng góp |
|--------|----------|
| P6 | 🟢 **Lag Compensation / Server Rewind** |
| P5 | 🟢 ADS + Recoil + Reload montage |
| P10 | 🟢 Crosshair + HUD pattern |
| P2 | 🟢 Match flow + GameMode |

---

## Tham chiếu

- [Courses/10-Udemy-ue5-cpp-multiplayer-shooter.md](../Courses/10-Udemy-ue5-cpp-multiplayer-shooter.md)
- [07-CPP_MP_Crash_GDD.md](07-CPP_MP_Crash_GDD.md) — theory base.
- [PALDARK/03-Roadmap_1_Year.md](../PALDARK/03-Roadmap_1_Year.md) — tuần 17-18 Lag Comp.
