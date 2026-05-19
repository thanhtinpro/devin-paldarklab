# 14 — Exploring Lyra for Game Development (Donchitos Reverse GDD)

> Source: [Udemy — UE5.2 Exploring Lyra for Game Development](https://www.udemy.com/course/exploring-lyra-for-game-development-in-unreal-engine-52/).
>
> Course tour Lyra sample project — Experience system, Game Features Plugin, Modular Gameplay.

---

## Identity Card

| Field | Value |
|-------|-------|
| 📦 Type | Course (framework deep dive) |
| Genre | Framework, không phải 1 game riêng |
| Engine | UE5.2 |
| Đóng góp PALDARK | **Lyra backbone — Experience + Game Features + Modular Gameplay = production framework** |

---

## Phase 1 — Brainstorm (reverse)

### Implicit pitch
> *"Lyra không phải game. Lyra là framework. Sau khoá bạn hiểu Experience là gì, Game Feature là gì, Modular Gameplay là gì."*

### Cảm xúc cốt lõi
- **Đến nhà** — sau khi học DIY, Lyra cho thấy production framework đã có sẵn.
- **Modular** — không phải god-character.

---

## Phase 2 — Decompose

### Lyra concepts

```
ULyraExperienceDefinition (DataAsset)
 ├── GameFeatureToEnable []
 ├── ActionSets []
 │    └── ULyraExperienceActionSet
 │         └── Actions [] (GameFeatureAction subclass)
 │              ├── AddComponents
 │              ├── AddAbilities
 │              ├── AddSpawners
 │              └── AddInputBinding
 ├── DefaultPawnData
 │    └── ULyraPawnData (Ability Set + Input Config + Mesh)
 └── PlayerSpawnInfo

Game Feature Plugin
 ├── Content (BP, DataAsset, Maps)
 ├── C++ Module (optional)
 └── Activation via Experience
```

---

## Phase 3 — Define

### Experience
- DataAsset định nghĩa "game mode" — không phải C++ GameMode mà là config-driven.
- Chứa Game Features cần bật + Action Sets cần apply.

### Game Feature Plugin
- Plugin tự đóng gói (Content + Code).
- Activation/deactivation runtime.
- Async load.

### Modular Gameplay
- AddComponents → inject Component vào Actor matching tag.
- AddAbilities → grant GA matching ASC.
- AddInputBinding → wire input runtime.

### Pawn Data
- DataAsset chứa: Ability Set (default GA), Input Config (IMC + IA), Mesh.
- Selectable per-Experience.

### Lyra Common UI
- Activatable Widget Stack.
- Async load widget per-state.

### Lyra State Tree AI
- Lyra dùng State Tree thay BT.

---

## Phase 4 — 3 Game Pillars (inferred — framework-level)

### Pillar 1 — Configuration over Code
> *"Experience config quyết game mode, không cần subclass GameMode."*

### Pillar 2 — Experience Composes Game
> *"Game = experience + features. Đổi experience = đổi game."*

### Pillar 3 — Modular Gameplay Scales
> *"Thêm feature không đụng base code. Plugin = isolated."*

---

## Phase 5 — Architecture Decisions (implicit)

### ADR-1: Experience-driven game mode
- **Quyết định:** UE GameMode chỉ thin wrapper, logic ở Experience DataAsset.
- **Lý do:** Designer change game mode bằng đổi DataAsset, không code.
- **PALDARK adopt:** 🟢 — Hub Experience + Raid Experience.

### ADR-2: Game Feature Plugin
- **Quyết định:** Mỗi feature lớn (weapon set, mode, map) là plugin.
- **PALDARK adopt:** 🟢 — Pal Pack 1/2/3 là plugin (Q3 release).

### ADR-3: Modular Gameplay (AddComponents/AddAbilities)
- **Quyết định:** Inject runtime thay vì hardcode.
- **PALDARK adopt:** 🟢 — Pal-specific component/ability inject qua plugin.

### ADR-4: CommonUI Activatable Widget
- **Quyết định:** Stack-based UI thay vì hard switch.
- **PALDARK adopt:** 🟢.

### ADR-5: State Tree thay BT
- **Quyết định:** Lyra dùng State Tree.
- **PALDARK adopt:** 🟡 Pal AI dùng Activity FSM (theo RoN), không State Tree. State Tree cho NPC Hub.

### ADR-6: Indicator Manager
- **Quyết định:** World-space marker subsystem.
- **PALDARK adopt:** 🟢 — copy nguyên.

---

## Phase 6 — System Map

| System | Class | File |
|--------|-------|------|
| Experience | `ULyraExperienceDefinition` | `Lyra/.../LyraExperienceDefinition.h/cpp` |
| ActionSet | `ULyraExperienceActionSet` | `Lyra/.../LyraExperienceActionSet.h/cpp` |
| Pawn Data | `ULyraPawnData` | `Lyra/.../LyraPawnData.h/cpp` |
| Modular | `UGameFeatureAction_AddComponents` | Engine plugin |
| CommonUI | `UCommonActivatableWidget` | Engine plugin |
| State Tree | `UStateTreeComponentSchema` | Engine |
| Indicator Mgr | `ULyraIndicatorManagerComponent` | `Lyra/.../UI/Indicators/` |

---

## Phase 7 — Stories taught

| Phần | US |
|------|----|
| 1 | Là dev, tôi navigate Lyra source map |
| 2 | Là dev, tôi tạo Experience Definition mới |
| 3 | Là dev, tôi tạo Action Set + add ability/component |
| 4 | Là dev, tôi tạo Pawn Data tùy chỉnh |
| 5 | Là dev, tôi tạo Game Feature Plugin |
| 6 | Là dev, tôi viết GameFeatureAction subclass |
| 7 | Là dev, tôi activate plugin từ Experience |
| 8 | Là dev, tôi viết Activatable Widget |
| 9 | Là dev, tôi dùng Indicator Manager |
| 10 | Là dev, tôi setup State Tree cho AI |

---

## Take-aways cho PALDARK

### 🟢 Copy nguyên (FRAMEWORK CORE)
- **Experience Definition** — Hub vs Raid.
- **Game Feature Plugin** — Pal Pack 1/2/3.
- **Modular Gameplay AddComponents/AddAbilities**.
- **CommonUI Activatable Widget Stack**.
- **Indicator Manager**.

### 🟡 Adapt
- **State Tree** — chỉ cho NPC Hub. Pal AI dùng Activity FSM (RoN).

---

## Pillar Mapping

| Pillar | Đóng góp |
|--------|----------|
| P17 | 🟢 **Lyra framework — CORE** |
| P3 | 🟢 Modular Gameplay |
| P10 | 🟢 CommonUI |
| P14 | 🟢 Game Feature async load |

---

## Tham chiếu

- [Courses/14-Udemy-ue5-exploring-lyra-for-game-development.md](../Courses/14-Udemy-ue5-exploring-lyra-for-game-development.md)
- [15-Lyra_RPG_GDD.md](15-Lyra_RPG_GDD.md) — apply Lyra cho RPG.
- [PALDARK/01-GameDesign.md](../PALDARK/01-GameDesign.md) ADR-001 (Lyra adopt).
