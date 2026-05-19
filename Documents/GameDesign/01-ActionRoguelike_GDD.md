# 01 — Tom Looman ActionRoguelike (Donchitos Reverse GDD)

> Source: [github.com/tomlooman/ActionRoguelike](https://github.com/tomlooman/ActionRoguelike) + [courses.tomlooman.com](https://courses.tomlooman.com/p/unrealengine-cpp)
>
> Coi course này là **một game** (thay vì coi là tutorial) — đây là một single-player top-down action RPG đầy đủ vòng lặp.

---

## Identity Card

| Field | Value |
|-------|-------|
| 📦 Type | Course (sample game đầy đủ) |
| Genre | Single-player top-down action RPG roguelike-lite |
| Engine | UE5 |
| Lines of code (approx) | ~10K C++ + Blueprint |
| Đóng góp PALDARK | Pattern Library #1 — composition + DataAsset + SaveGame + Async + Aggregate Tick |

---

## Phase 1 — Brainstorm (reverse-engineer)

### Implicit pitch
> *"Bạn là pháp sư bị rơi vào một thế giới hostile. Mỗi run là một dungeon thủ tục với bot AI, item nhặt, save checkpoint. Sống sót, level up, mạnh dần."*

### Cảm xúc cốt lõi
- **Tự khám phá** — không có hand-holding.
- **Tiến bộ thấy được** — mỗi run mạnh hơn run trước.
- **Tử vong là bài học** — chết → respawn từ checkpoint, không reset hoàn toàn.

### Problem nó giải
- Là **sample game vừa đủ phức tạp** để dạy mọi pillar nền (P1–P5, P9, P11, P12, P13, P14, P15) trong khoảng 30–40 giờ học.
- Tránh được scope MMO (không multiplayer) — giữ scope dạy học.

---

## Phase 2 — Decompose

### Verb cốt lõi
- Move (third-person)
- Attack (primary projectile + dash + ultimate)
- Pickup (health, coin, powerup)
- Pickup item (treasure chest)
- Save/Load (sleep at checkpoint)
- Spawn enemy (AI director)

### System hierarchy

```
┌──────────────────┐
│ SGameModeBase    │ Server (single-player nhưng dùng GM)
│   AI Director    │ Spawn bot mỗi N giây qua DataAsset
│   SaveGame coord │ Trigger save on checkpoint
└────────┬─────────┘
         │
   ┌─────┴─────┬─────────┬──────────┬───────────┐
   ▼           ▼         ▼          ▼           ▼
 SCharacter  SAI_Bot  STargetD.  SPickup    SCheckpoint
   ↓           ↓         ↓          ↓           ↓
 ActionComp  BT/EQS  AttributeC  Trigger    SaveTrigger
 AttributeC  Action C ActionComp Health/Coin
```

---

## Phase 3 — Define (GDD reverse)

### Game Loop
1. Spawn at last checkpoint.
2. Explore map (dungeon procedurally arranged).
3. Encounter bot AI (1–5 cùng lúc).
4. Combat: primary fire + dash + ultimate (3 ability).
5. Pick up: health, coin, powerup.
6. Reach checkpoint → save → respawn point updated.
7. Boss area / lock end-game.

### Player Character (`ASCharacter`)
- Third-person camera.
- Ability set: PrimaryAttack, Dash, BlackHole (ultimate).
- Attribute: Health, Rage (ultimate resource), MaxHealth.

### Combat
- Projectile-based primary (custom action).
- Action System (Tom's custom GAS-lite — predecessor lý thuyết của GAS).
- Damage broadcast qua AttributeComponent → UI/Health bar.

### AI Bot (`ASAICharacter`)
- BehaviorTree + EQS.
- Sense: sight via AIPerceptionComponent.
- Action set: Move to player, Range attack.

### Inventory / Pickup
- Treasure chest spawn coin → coin attribute increase.
- Health pickup heal.
- Powerup buff temp.

### Save / Load
- USaveGame subclass.
- FMemoryWriter + FObjectAndNameAsStringProxyArchive.
- Persist: transform, attribute, picked-up state.

### AI Director
- DataAsset table cho bot type.
- AssetManager async load bot mesh + icon.

### Endgame
- Course-specific: dạy đến boss + cleanup. Không có endgame ship.

---

## Phase 4 — 3 Game Pillars (inferred)

### Pillar 1 — Iteration is Mastery
> *"Mỗi run là cơ hội học một mẹo combat mới."*
- AI Director cảm xúc theo độ khó player.
- Item nhặt thay đổi hành vi (powerup).

### Pillar 2 — Power Growth Through Items
> *"Bạn mạnh hơn không phải vì level — vì bạn tìm được."*
- Coin = currency dùng cho powerup tạm thời.
- Treasure mở khi pay credit.

### Pillar 3 — Death is Teacher
> *"Chết không xoá hết. Chết dạy."*
- Checkpoint giữ attribute progress.
- Respawn nhanh, không cutscene dài.

---

## Phase 5 — Architecture Decisions (implicit)

### ADR-1: Tách AttributeComponent ra khỏi Character
- **Quyết định:** Health/Damage logic ở component, không trong Character.
- **Lý do:** Reuseable cho AI, breakable, prop.
- **Hệ quả:** Damage broadcast 1 nơi, UI lắng nghe 1 nơi.
- **PALDARK adopt:** ✅ — Player + Pal + Enemy đều dùng AttributeComponent pattern. Sau này thay GAS AttributeSet (cao cấp hơn).

### ADR-2: Tự custom Action System thay vì dùng GAS
- **Quyết định:** Build "Action System" gọn (UAction + UActionEffect) thay vì học GAS.
- **Lý do:** Course beginner-friendly; GAS quá nặng.
- **Hệ quả:** Học sinh hiểu **pattern** GA-like trước khi học GAS.
- **PALDARK adopt:** 🟡 Skip — PALDARK dùng GAS thật từ đầu (vì đã có Aura course).

### ADR-3: DataAsset + AssetManager cho bot
- **Quyết định:** Bot type là PrimaryDataAsset, load async via FStreamableManager.
- **Lý do:** Tránh hard reference làm dài cooked size.
- **Hệ quả:** Bot mesh + icon async load.
- **PALDARK adopt:** ✅ — Pal definition PrimaryDataAsset + async load.

### ADR-4: SaveGame archive + Transform serialize
- **Quyết định:** Save = full archive (FObjectAndNameAsStringProxyArchive).
- **PALDARK adopt:** 🟠 Tham chiếu — PALDARK chủ yếu server-persist (DynamoDB), local SaveGame chỉ settings.

### ADR-5: Aggregate Tick + Object Pool cho projectile
- **Quyết định:** Tích hợp tick management để giảm hitch.
- **PALDARK adopt:** ✅ — bullet pool + Pal AI aggregate tick.

---

## Phase 6 — System Map (class + file reference)

| System | Class | File |
|--------|-------|------|
| GameMode | `ASGameModeBase` | `Source/.../SGameModeBase.h/cpp` |
| Character | `ASCharacter` | `SCharacter.h/cpp` |
| AI Bot | `ASAICharacter` | `SAICharacter.h/cpp` |
| Action System | `USActionComponent` + `USAction` | `SActionComponent.h/cpp` |
| AttributeSystem | `USAttributeComponent` | `SAttributeComponent.h/cpp` |
| Pickup | `ASItemChest` + `ASPowerup_*` | `SItemChest.h/cpp` |
| Save | `USSaveGame` | `SSaveGame.h/cpp` |
| Director | DataAsset + AssetManager | `SMonsterData.h` |
| UI | `USWorldUserWidget` | `SWorldUserWidget.h/cpp` |

---

## Phase 7 — Stories / Lessons (course chapters as US)

| Chapter | US-style |
|---------|----------|
| 1 | Là dev, tôi setup được module C++ + module dependency |
| 2 | Là dev, tôi tạo Character với input + camera + animation |
| 3 | Là dev, tôi viết Attribute Component reusable cho mọi Actor |
| 4 | Là dev, tôi viết Action System (project's custom GAS-lite) |
| 5 | Là dev, tôi setup BehaviorTree + EQS cho AI |
| 6 | Là dev, tôi tạo PrimaryDataAsset cho bot và async load |
| 7 | Là dev, tôi viết SaveGame archive + checkpoint |
| 8 | Là dev, tôi setup Aggregate Tick + Object Pooling |
| ... | ... (full ở [Courses/01-Tomlooman](../Courses/01-Tomlooman-ActionRoguelike.md)) |

---

## Take-aways cho PALDARK

### 🟢 Copy nguyên
- **`USActionComponent` pattern** → port thành `UPaldarkPalActivityComponent` (vì Activity ≈ Action).
- **`USAttributeComponent` pattern** → tham chiếu khi quyết định ASC location (PALDARK chọn GAS, nhưng Tom's pattern là precursor logic).
- **DataAsset + AssetManager** → `UPaldarkPalDefinition` async load.
- **Aggregate Tick + Object Pool** → bullet + loot pool, Pal AI tick batch.

### 🟡 Adapt
- **SaveGame archive** → chỉ dùng cho settings local. Pal/inventory persist server-side.

### 🔴 Avoid (cho PALDARK)
- **Custom Action System thay GAS** → KHÔNG làm. PALDARK dùng GAS thật.

---

## Pillar Mapping (15 source × 18 pillar) — đóng góp của source này

| Pillar | Đóng góp |
|--------|----------|
| P1 | 🟡 Module setup |
| P2 | 🟡 GameMode/GameInstance basics |
| P3 | 🟢 **AttributeComponent + ActionComponent pattern** |
| P4 | 🟠 Enhanced Input basic |
| P5 | 🟡 AnimNotify pattern |
| P9 | 🟠 BT + EQS basic |
| P10 | 🟡 UMG basic |
| P11 | 🟠 Pickup pattern |
| P12 | 🟢 **DataAsset + AssetManager** |
| P13 | 🟢 **SaveGame archive** |
| P14 | 🟢 **Async load** |
| P15 | 🟢 **Aggregate Tick + Object Pool** |

---

## Tham chiếu

- [Courses/01-Tomlooman-ActionRoguelike.md](../Courses/01-Tomlooman-ActionRoguelike.md) — bài giảng chi tiết.
- [PALDARK/04-Resource_Map.md](../PALDARK/04-Resource_Map.md) — pillar nào học từ đây.
- [Donchitos_GameStudios_Framework.md](../Donchitos_GameStudios_Framework.md) — framework gốc.
