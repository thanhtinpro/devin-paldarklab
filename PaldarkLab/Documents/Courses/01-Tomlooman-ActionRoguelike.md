# 01. Tom Looman — Professional Game Development in C++ and UE5 (ActionRoguelike "Project Orion")

| | |
|---|---|
| **Tier** | T1 — Single-player C++ Game |
| **Provider** | Tom Looman (https://courses.tomlooman.com/p/unrealengine-cpp) |
| **Source** | https://github.com/tomlooman/ActionRoguelike |
| **Engine** | UE5.6 (main branch) — có branch `Lecture29-FinishedProject` (UE4.25) cho khóa gốc và `UE5.6-CourseProject` cho version mới |
| **Trong repo** | `01.Tomlooman-ActionRoguelike/` (chỉ có README + Source) |

---

## 1. Mục tiêu khóa (đọc từ README)

Xây "Project Orion" — game **co-op Action Roguelike** trên UE5 C++ từ đầu, đủ feature production:

- Third-person character + Enhanced Input
- **Action System** (thiết kế tương đương GAS nhưng custom)
- `AttributeComponent` (health)
- **SaveGame** persist character + world
- Event-driven gameplay (UI react)
- C++ + Blueprint kết hợp
- **GameplayTags** trên Actor/Buff/Action
- **Multiplayer** cho mọi feature
- GameMode logic: EQS spawn locations, **bot spawning** dựa trên credit, DataTable + DataAsset config
- **AssetManager** async load
- AI: BehaviorTree (Roam/See/Chase/Attack/Flee/Heal) + custom C++ BT Node + EQS attack/cover
- Powerups (pickup heal/credit/action)
- UMG main menu + projected widget cho powerup/enemy health, **C++ Localized Text**
- *Experimental:* Aggregate Ticking, Actor Pooling cho projectile, **Data-Oriented projectiles** (không dùng Actor)
- Async line trace
- PSO Precaching, Bundled PSOs Setup DX12

---

## 2. Lộ trình giảng dạy

Khóa có 29 lecture chính (theo branch `Lecture29-FinishedProject`). Không có folder Documents/ với .srt trong repo (chỉ source). Lộ trình rút từ source layout `Source/ActionRoguelike/`:

```
ActionRoguelike/
├── Core/                    → AttributeComponent, GameMode, GameInstance
├── Player/                  → Character, Controller, Camera
├── ActionSystem/            → RogueAction, RogueActionComponent, RogueAttributeSet
│                              RogueActionEffect, _StunFromDamage, _Thorns
├── Projectiles/             → ProjectileBase, Magic / Dash / Blackhole
├── Pickups/                 → PickupBase, HealthPotion, CoinPickup
├── AI/                      → AICharacter, AIController, custom BT nodes
├── StateTrees/              → StateTree (UE5.x experimental)
├── SaveSystem/              → SaveGame subsystem
├── Subsystems/              → AssetManager-derived, GameInstance subsystems
├── UI/                      → HUD widget, popup, menu
├── Animation/               → AnimInstance
├── World/                   → Powerup volumes, spawn manager
├── Performance/             → ActorPooling, Aggregate Tick
├── Development/             → Cheat manager, debug
├── SharedGameplayTags.h/cpp → Gameplay Tag centralization
```

**Pedagogical pathway** (suy ra từ Tom's bài viết và cấu trúc source):

1. **Setup + Character + Enhanced Input** — biết move, run, jump.
2. **AttributeComponent** — health/damage/event delegate.
3. **ActionSystem** — clone đơn giản GAS (Action = Ability, ActionEffect = GameplayEffect, ActionComponent = ASC). **Mục đích là dạy pattern trước khi chạm GAS thật.**
4. **Projectiles** — magic attack, **Dash** (projectile teleport), **Blackhole**.
5. **AI** — Behavior Tree, EQS attack/cover, custom BT C++ node.
6. **Pickups + GameMode** — bot spawn theo credit cycle, EQS pick spawn points.
7. **UMG + UI Events** — health bar, projected widget cho enemy.
8. **GameplayTags** — đánh dấu effect (burning, thorns), input gating.
9. **SaveGame** — persist Actor transform + AttributeComponent + custom data.
10. **Multiplayer pass** — replicate AttributeComponent, ActionComponent, projectile.
11. **AssetManager + Async load** — primary data asset cho enemy, async load icon.
12. **Performance** — Aggregate Tick, ActorPool, PSO Precaching.

---

## 3. Cốt lõi (Pillars rút ra)

| Pillar (xem [UE5_Core_Pillars](../UE5_Core_Pillars.md)) | Mức trong khóa | Code chứng |
|---|---|---|
| P1 C++/Build | 🟡 | `ActionRoguelike.Build.cs` |
| P2 Core Framework | 🟡 | `RogueGameMode`, `RogueCharacter` |
| P3 Composition | 🟠 | `RogueAttributeComponent`, `RogueActionComponent` |
| P4 Enhanced Input | 🟠 | `Player/RoguePlayerController.cpp` |
| P5 Animation | 🟡 | `Animation/` |
| P6 Replication | 🟡 | Mọi component đều `Replicated` |
| P8 GAS-like pattern | 🟢 (Action System) | `ActionSystem/RogueAction.h` |
| P9 AI BT + EQS | 🟠 | `AI/`, custom BT C++ node |
| P10 UI | 🟡 | `UI/` widget projected |
| P12 Data-driven | 🟠 | DataTable bot, DataAsset enemy |
| P13 Save | 🟠 | `SaveSystem/` |
| P14 AssetMgr Async | 🟠 | Subsystem-based AssetManager |
| P15 Performance | 🟠 | `Performance/` aggregate tick + pool |

**Action System của Tom (thay GAS):**

```
RogueAction              ← ~ GameplayAbility
RogueActionEffect        ← ~ GameplayEffect (e.g. Burning, Thorns, Stun)
RogueActionComponent     ← ~ AbilitySystemComponent
RogueAttributeSet        ← ~ AttributeSet
RogueAttributeComponent  ← persistent health/attribute container
GameplayTags             ← thẻ thật của UE, để gate ability
```

**Lý do dùng Action System thay GAS:** GAS overhead cao + curve học dốc. Action System ~ 500 LOC nhưng dạy đúng **pattern** (composability, event-driven, tag gating). Sau khóa Tom, học [05] GAS Crash Course sẽ hiểu nhanh hơn.

---

## 4. Approach sư phạm

- **"Show, don't tell":** Tom không lecture lý thuyết. Mỗi feature có git commit + lecture số.
- **Iterative refactor:** bắt đầu monolithic, refactor dần ra component.
- **Cover trade-off:** mỗi feature đều giải thích why **không dùng** approach khác (vd: tại sao không GAS, tại sao không dùng FloatingPawnMovement).
- **Multiplayer là first-class:** thêm replication ngay sau khi feature single-player chạy. Không "đợi đến cuối khóa".
- **Save System dạy serialization sâu:** không dùng `SaveGame` thuần — Tom viết archive proxy để save bất kỳ Actor nào có Tag `Saveable`.

---

## 5. Đầu ra học viên

- Có **1 game co-op chơi được** đầy đủ feature.
- Hiểu được pattern composition của UE5 (Component-driven thay vì inheritance-driven).
- Đọc được code GAS thật ở khóa sau vì đã có mental model từ Action System.
- Có template để tiếp tục build game riêng.

---

## 6. Bài tập tự đặt sau khóa

1. Replace Action System bằng GAS thật → so sánh LOC + perf.
2. Thay Behavior Tree bằng State Tree (UE5.4+) → so sánh debuggability.
3. Thay Actor Pool bằng Data-Oriented projectile (đã có branch experimental).
4. Thêm dedicated server packaging (link sang khóa [08]).

---

## 7. Liên kết khóa khác

| Trước | Sau |
|-------|-----|
| [12] Pro UE4 Coding | [05] GAS Crash, [07] MP Crash, [09] Inventory |
