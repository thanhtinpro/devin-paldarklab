# P14 — Asset Manager + Async Loading

> Tự soạn từ: [01] Tom Looman, [14] Exploring Lyra, [15] Lyra RPG.
>
> PALDARK = open-ish world Dark Zone + Hub + 30+ Pal species — async load bắt buộc để tránh hitch.

---

## Pillar Identity

**Cái gì:** Load asset bất đồng bộ, manage memory budget.

**Scope:**
- ✅ FPrimaryAssetId + FPrimaryAssetType.
- ✅ FStreamableManager — async load handle.
- ✅ FSoftObjectPath, TSoftObjectPtr, TSoftClassPtr.
- ✅ Asset Manager registry config.
- ✅ Game Feature Plugin (Lyra) — runtime asset loading.
- ✅ Bundle (group of asset loaded together).
- ✅ Preload + Unload + Reference count.

**Out of scope:**
- ❌ DataAsset definition (P12).
- ❌ Save soft path (P13).

---

## PALDARK cần gì từ pillar này?

| Need | Solution |
|------|----------|
| 30+ Pal mesh, anim, FX — not all loaded at once | Soft ref + async load per match |
| Item icon (50+) — load on demand | Async load on widget show |
| Map asset (Dark Zone biome 2-3 variant) | Bundle per biome |
| Pal Pack 1/2/3 DLC release | Game Feature Plugin |
| Audio bank (FMOD) | Async load on match start |

---

## Sources Matrix

| Source | Coverage | Lesson lấy từ đó |
|--------|----------|------------------|
| [01] Tom Looman | 🟢 | L1 Async DataAsset, L2 Async UI icon |
| [14] Exploring Lyra | 🟢 | L3 Game Feature Plugin, L4 Bundle |
| [15] Lyra RPG | 🟡 | L5 Pawn Data async load |

---

## Prerequisite

- P12 (PrimaryDataAsset).
- P03 (Modular Gameplay).

---

## Lessons

### L1 — Async DataAsset load 🧪

**Goal:** Load PrimaryDataAsset bất đồng bộ với callback.

**Concept:**
- `UAssetManager& AM = UAssetManager::Get();`
- `AM.GetStreamableManager().RequestAsyncLoad(SoftPath, FStreamableDelegate::CreateUObject(this, &OnLoaded))`.
- Callback runs when loaded.
- Returned `TSharedPtr<FStreamableHandle>` — keep alive to track.

**Source learning path:**
- [01] Tom Looman § Async DataAsset chapter.

**API / Class chính:**
- `UAssetManager::GetStreamableManager()`.
- `FStreamableManager::RequestAsyncLoad(Path, Callback, Priority)`.
- `FStreamableHandle::IsActive() / IsLoadingInProgress() / GetLoadedAsset()`.

**Practice exercise (2 hour):**
1. `UPaldarkLabEnemyDefinition` PrimaryDataAsset chứa `TSoftClassPtr<APaldarkLabEnemy>`.
2. GameMode `SpawnEnemy(EnemyDef)` → AsyncLoad class → callback → SpawnActor.
3. PIE: spawn → first time delay ~500ms (load), second time instant.

**PALDARK take-away:**
- 🟢 Spawn Pal/NPC/Enemy → async load class first.

**Apply ở week:** Roadmap week 22.

---

### L2 — Async UI icon load (Tom Looman) 🧪

**Goal:** Inventory open → icon lazy load không freeze frame.

**Concept:**
- Item Definition chứa `TSoftObjectPtr<UTexture2D> Icon`.
- UI Widget `OnItemShow(Item)` → `RequestAsyncLoad(Item->Icon.ToSoftObjectPath(), CB_SetIcon)`.
- Callback set icon to image widget.

**Source learning path:**
- [01] Tom Looman § Async UI chapter.

**Practice exercise:**
1. ItemDef soft ref icon.
2. Inventory widget async load 24 icon simultaneously.
3. Check no hitch (single frame > 16ms).

**PALDARK take-away:**
- 🟢 Inventory open → 24 slot icon async load.
- 🟢 Pal Roster open → 12 Pal icon async load.

**Apply ở week:** Roadmap week 19.

---

### L3 — Game Feature Plugin (Lyra CORE) 🧪

**Goal:** DLC content via plugin — load on demand, runtime.

**Concept:**
- Plugin folder = `Plugins/PaldarkPalPack1/`.
- `.uplugin` file declares Game Feature.
- Plugin contains: GA, DataAsset, content asset.
- `UGameFeatureSubsystem::LoadAndActivateGameFeaturePlugin(PluginURL, OnComplete)`.
- Once active, `UGameFeatureAction_AddComponents` injects components, etc.

**Source learning path:**
- [14] Exploring Lyra § Game Feature Plugin chapter (best resource).
- Cross-ref: `Documents/GameDesign/14-Exploring_Lyra_GDD.md` ADR-2.

**API / Class chính:**
- `UGameFeaturesSubsystem::LoadGameFeaturePlugin(URL, Options, OnComplete)`.
- `UGameFeatureData` plugin's data asset.

**Practice exercise (1 ngày):**
1. Tạo plugin `PaldarkPalPack1` (Game Feature Plugin).
2. Add 3 Pal Def + 3 GA in plugin.
3. Runtime: `LoadAndActivate(PluginURL)` → 3 Pal có thể tame.

**PALDARK take-away:**
- 🟢 Pal Pack 1 (Q3) + Pack 2 (Q4) = Game Feature Plugin.
- 🟢 Map Pack (biome variant) = Game Feature Plugin.

**Apply ở week:** Roadmap week 43+ (DLC content).

---

### L4 — Bundle (loaded group) 📖🧪

**Goal:** Group of asset luôn load together.

**Concept:**
- Asset Manager config: PrimaryAssetType có `Rules` define bundle.
- `Bundle = {"UI", "Game"}` — UI bundle (icon) + Game bundle (mesh, anim).
- Load: `AM.LoadPrimaryAsset(AssetId, {Bundle1, Bundle2})`.

**Source learning path:**
- [14] Exploring Lyra § Bundle chapter.

**PALDARK take-away:**
- 🟢 Pal Def có 2 bundle: "UI" (icon, portrait) + "Game" (mesh, anim, montage).
- 🟢 Hub load "UI" bundle of all Pal (lightweight); Raid load "Game" bundle of team only.

**Apply ở week:** Roadmap week 25-26 (optimize load).

---

### L5 — Pawn Data async load (Lyra RPG) 📖

**Goal:** Pawn skeletal mesh + abilities load async.

**Concept:**
- `ULyraPawnData` PrimaryDataAsset.
- On PC spawn → request load Pawn Data → callback configure Pawn.

**Source learning path:**
- [15] Lyra RPG § Pawn Data chapter.

**PALDARK take-away:**
- 🟢 Different Pawn class per Pal species → async load on tame.

**Apply ở week:** Roadmap week 33.

---

## ⚡ Capstone Exercise

**Goal:** PaldarkLab full async load pipeline.

**Yêu cầu:**
1. 5 PalDef PrimaryDataAsset.
2. Asset Manager register type with 2 bundle.
3. Hub: async load "UI" bundle of all 5 Pal — display roster.
4. Raid match start: async load "Game" bundle of 3-Pal team.
5. Game Feature Plugin `LabPalPackTest` add 2 extra Pal.

**Acceptance:** No hitch > 16ms on inventory open or Pal spawn.

**Effort:** 1 tuần.

---

## Khi apply vào PALDARK roadmap

| Week | Pillar P14 usage |
|------|------------------|
| W19 | Async load inventory icon |
| W22 | Async load NPC/Pal class |
| W25-26 | Bundle config (Hub vs Raid) |
| W33 | Lyra adopt — Pawn Data async |
| W43+ | DLC via Game Feature Plugin |

---

## Anti-patterns (cảnh báo)

- 🔴 Hard ref toàn bộ Pal mesh → 30 Pal × 20MB = 600MB load at start.
- 🔴 Sync load on click → frame freeze 1-2s.
- 🔴 No `FStreamableHandle` keep alive → loaded asset get GC.
- 🔴 No bundle config → load all asset bundle on tiny query.
- 🔴 Plugin activate without `OnComplete` callback → race condition.

---

## Tham chiếu

- [`Documents/UE5_Core_Pillars.md`](../UE5_Core_Pillars.md) § P14.
- [`Documents/GameDesign/01-ActionRoguelike_GDD.md`](../GameDesign/01-ActionRoguelike_GDD.md) — Async pattern.
- [`Documents/GameDesign/14-Exploring_Lyra_GDD.md`](../GameDesign/14-Exploring_Lyra_GDD.md) — Game Feature.
