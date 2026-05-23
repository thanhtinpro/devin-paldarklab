# 03 — PUBG-KI (Donchitos Reverse GDD)

> Source: [github.com/PUBG-KI/PUBG-KI](https://github.com/PUBG-KI/PUBG-KI) — Korean fan-made PUBG replica/prototype
>
> Coverage 18 pillar: ~53%. Prototype mid-size có gameplay loop chính hoạt động (lobby + plane + drop + circle + dedicated server).

---

## Identity Card

| Field | Value |
|-------|-------|
| 📦 Type | Prototype (gameplay loop end-to-end, missing polish) |
| Genre | Battle Royale 100-player |
| Engine | UE5 |
| Đóng góp PALDARK | Manager pattern + Lobby/Plane pipeline + Zone Manager + 6-Component Pawn (clean mid-size composition) |

---

## Phase 1 — Brainstorm (reverse)

### Implicit pitch
> *"100 người, 1 đảo, 1 vòng tròn co lại, 1 winner. Không respawn, ai làm ngu chết."*

### Cảm xúc cốt lõi
- **Hồi hộp** từ second 1 (drop).
- **Tham lam** (gear cao hơn = an toàn hơn).
- **Cô đơn** (giai đoạn early-game silent stalk).
- **Adrenaline** (final circle).

### Vì sao thành công
- BR = 1 game = 100 stories. Replay vô hạn.
- Random loot = mỗi run khác.
- Final circle = climax forced.

---

## Phase 2 — Decompose

### Verb cốt lõi
- Wait in lobby
- Drop from plane
- Parachute
- Loot
- Move
- Shoot
- Survive zone
- Win or die

### System hierarchy

```
┌────────────────────────────┐
│ APUBG_GameMode             │ Dedicated server
│ APUBG_GameState            │ Match phase state
└──────────────┬─────────────┘
               │
   ┌───────────┼──────────┬────────────┬───────────┬──────────┐
   ▼           ▼          ▼            ▼           ▼          ▼
PlayerChar  AirplaneA   ZoneManagr  LobbyManagr  Map      Spawn
 ↓           ↓           ↓            ↓           POI       Manager
6 Comp      Pal-fall    Shrink       Wait flow   loot       loot
            anim        timer                    spawn
            spawn                                
PlayerCtrl  drop logic
 7 Manager  
 (AActor)
```

### 7 Manager (AActor singleton — anti-pattern, see below)
- `APUBG_LobbyManager`
- `APUBG_PlaneManager`
- `APUBG_ZoneManager`
- `APUBG_LootManager`
- `APUBG_PlayerManager`
- `APUBG_StatManager`
- `APUBG_MatchManager`

---

## Phase 3 — Define (GDD reverse)

### Game Loop
1. Lobby — 100 player join + wait.
2. Plane spawn → players parachute drop.
3. Land → loot 30 seconds.
4. Zone start shrinking.
5. Combat encounters.
6. Continue loot + reposition.
7. Final zone → 1 winner.

### Player Character
- TPS / FPS toggle.
- 6 Component:
  - `UEquippedItemComponent`
  - `UInputComponent` (custom)
  - `UInventoryComponent`
  - `UItemDataComponent`
  - `UMovementComponent` (custom)
  - `UNearAreaComponent` (proximity check loot)

### Combat
- Hitscan rifle + projectile sniper.
- No lag compensation (gap pillar).
- Damage simple direct.

### Inventory
- Backpack tier + slot.
- Stack count.
- Pickup proximity (NearArea component).

### Zone System
- Concentric circle shrink.
- Damage outside zone.
- Timer per phase (waiting → shrinking → final).

### Airplane
- Plane spawn at random angle.
- Players drop on key.
- Parachute physics.

### Dedicated Server
- Setup partial — not on AWS. Local dedicated.

---

## Phase 4 — 3 Game Pillars (inferred)

### Pillar 1 — Stillness Wins
> *"Người di chuyển chết. Người đứng yên thấy người di chuyển."*
- Final zone yêu cầu camp.
- Audio cue = enemy footstep.

### Pillar 2 — Last Man Surviving
> *"Không ai win bằng kỹ năng riêng. Win = sống sót, không phải kill."*
- Kill cao không đảm bảo win.
- 1 lần chết = out — emotional impact.

### Pillar 3 — Loot Defines Strategy
> *"Súng quyết định bạn aggressive hay defensive."*
- Loot tier → playstyle.
- Random spawn = random meta.

---

## Phase 5 — Architecture Decisions (implicit)

### ADR-1: Manager pattern (AActor singleton)
- **Quyết định:** Mỗi system là `AActor` placed trong level.
- **Lý do:** Convenient — drag-drop trong editor.
- **Hệ quả:** Singleton manage qua `BeginPlay` lookup. Có debt: lifecycle tricky, không cross-level.
- **PALDARK adopt:** 🔴 **Anti-pattern** — PALDARK dùng `UWorldSubsystem` thay vì AActor singleton.

### ADR-2: 6-Component Player Pawn (clean composition)
- **Quyết định:** Player Pawn = 6 component, mỗi cái logic riêng.
- **Lý do:** Reusable, testable.
- **Hệ quả:** Mid-size game đẹp — gold standard composition.
- **PALDARK adopt:** ✅ — extend 6 thành 12, giữ pattern "mỗi component own logic".

### ADR-3: Dedicated server local (chưa AWS)
- **Quyết định:** Setup dedicated nhưng host local.
- **Lý do:** Course/prototype scope.
- **Hệ quả:** Không production-ready.
- **PALDARK adopt:** 🟡 Tham chiếu khái niệm dedicated, deploy AWS riêng từ [08].

### ADR-4: Không lag compensation
- **Quyết định:** Hitscan trực tiếp, không server rewind.
- **Hệ quả:** Cheat-bait, ping-bias.
- **PALDARK adopt:** 🔴 Avoid — PALDARK bắt buộc Lag Comp ([10]).

### ADR-5: ZoneManager đơn giản
- **Quyết định:** Vẽ concentric circle, damage outside.
- **PALDARK adopt:** ✅ — Pattern Extract Zone (timed) học từ đây.

---

## Phase 6 — System Map

| System | Class | File | Status |
|--------|-------|------|--------|
| GameMode | `APUBG_GameMode` | `PUBG_GameMode.h/cpp` | 🟢 OK |
| GameState | `APUBG_GameState` | `PUBG_GameState.h/cpp` | 🟢 OK |
| Player | `APUBG_PlayerCharacter` | `PUBG_PlayerCharacter.h/cpp` | 🟢 OK (6 component) |
| Manager Pawn | 7 `A*Manager` | `Managers/*` | 🟠 debt (AActor singleton) |
| Inventory | `UInventoryComponent` | `InventoryComponent.h/cpp` | 🟢 OK |
| Airplane | `AAirplane` | `Airplane.h/cpp` | 🟢 OK |
| Zone | `AZoneManager` | `ZoneManager.h/cpp` | 🟢 OK (logic) / 🟠 (kiến trúc) |
| UI | 8 sub-folder widget | `Widgets/` | 🟢 OK (mid-size gold) |

---

## Phase 7 — Stories taught (course-style)

Vì là prototype không có course đi kèm, US infer từ commit log + feature list:

| US | Mô tả |
|----|-------|
| US-1 | Là dev, tôi setup dedicated server target + listen → dedicated travel |
| US-2 | Là dev, tôi tạo Lobby UI cho 100 player join |
| US-3 | Là dev, tôi viết Plane spawn + parachute drop physics |
| US-4 | Là dev, tôi setup Zone shrinking + damage outside |
| US-5 | Là dev, tôi viết Inventory với weight + slot tier |
| US-6 | Là dev, tôi viết Loot spawn (placed + dynamic) |
| US-7 | Là dev, tôi setup 6-component Pawn (clean composition) |

---

## Take-aways cho PALDARK

### 🟢 Copy nguyên
- **6-Component Pawn pattern** (mid-size gold standard).
- **Zone Manager pattern** → adapt thành Extract Beacon Manager.
- **Lobby flow** (Wait → Plane → Drop) → PALDARK adapt thành (Briefing → Helicopter → Drop).

### 🟡 Adapt
- **Manager pattern** — chuyển AActor → UWorldSubsystem.

### 🔴 Avoid
- **AActor singleton manager** (debt).
- **No lag compensation** — PALDARK bắt buộc có.
- **Stat tracking đơn giản** — PALDARK cần DynamoDB persistent.

---

## Pillar Mapping — đóng góp của source này

| Pillar | Đóng góp |
|--------|----------|
| P1 | 🟢 Module setup mid-size |
| P2 | 🟢 GameMode + GameState |
| P3 | 🟢 **6-Component composition** |
| P6 | 🟠 Replication (chưa lag comp) |
| P7 | 🟡 Dedicated local |
| P10 | 🟢 UI mid-size organized |
| P11 | 🟢 Inventory mid-size |
| P15 | 🔴 Chưa Significance (cần với 100-player) |

---

## Tham chiếu

- [Courses/03-PUBG.md](../Courses/03-PUBG.md)
- [Projects/PUBG_Assessment.md](../Projects/PUBG_Assessment.md) — 53% coverage.
- [PALDARK/02-Pillar_Coverage.md](../PALDARK/02-Pillar_Coverage.md) — pattern adoption.
