# P17 — Lyra Framework

> Tự soạn từ: [14] Exploring Lyra, [15] Lyra RPG.
>
> Lyra = Epic-shipped reference framework, **PALDARK adopts Lyra ở week 33-34** sau khi đã có foundation từ Phase 1-2.

---

## Pillar Identity

**Cái gì:** Đọc/extend Lyra framework — Experience, Game Features, Modular Gameplay, CommonUI Stack, Indicator Manager.

**Scope:**
- ✅ ULyraExperienceDefinition (mode = DataAsset).
- ✅ Game Features Plugin (runtime content load).
- ✅ Modular Gameplay (AddComponents/AddAbilities/AddSpawners).
- ✅ ULyraPawnData (data-driven Pawn).
- ✅ ULyraInputConfig (Tag-based input).
- ✅ LyraCommonUI (CommonUI extended).
- ✅ ULyraEquipmentManagerComponent.
- ✅ ULyraQuickBarComponent.
- ✅ ULyraInventoryItemDefinition.
- ✅ State Tree integration.

**Out of scope:**
- ❌ GAS detail (P08).
- ❌ Inventory deep (P11).

---

## PALDARK adopt strategy

**Decision tree (week 33):**
- ✅ Adopt: Experience Definition, Game Features, Modular Gameplay, CommonUI, Indicator Manager, QuickBar.
- 🟡 Adapt: PawnData (extend), InventoryItemDef (compose with Item Fragment P11).
- 🔴 Skip: Lyra-specific things không phù hợp PALDARK (e.g., Lyra default Health hardcoded).

**Why adopt at W33, not W1:**
- W1-32: Build foundation manual (control flow, avoid magic).
- W33: Replace manual scaffolding with Lyra equivalent (now understand what Lyra does).
- Net result: 2-3 week extra upfront but save 5+ weeks on plugin/feature architecture later.

---

## Sources Matrix

| Source | Coverage | Lesson lấy từ đó |
|--------|----------|------------------|
| [14] Exploring Lyra | 🟢 | L1-5 (full deep dive) |
| [15] Lyra RPG | 🟢 | L6 Lyra applied to RPG genre |

---

## Prerequisite

- All prior pillar P01-P16 covered at basic level (PALDARK done W32+ before Lyra adoption).
- P03 (Component composition).
- P08 (GAS).
- P14 (Asset Manager).

---

## Lessons

### L1 — Experience Definition (Lyra mode) 🧪

**Goal:** Define game mode as DataAsset, not C++ GameMode subclass.

**Concept:**
- `ULyraExperienceDefinition` PrimaryDataAsset.
- Contains:
  - `TArray<TSoftObjectPtr<UGameFeatureData>> GameFeaturesToEnable` — plugins to load.
  - `TArray<TObjectPtr<ULyraExperienceActionSet>> ActionSets` — addAbility, addComponent, addSpawn.
  - `TObjectPtr<ULyraPawnData> DefaultPawnData`.
- `ULyraGameMode` queries `URL` → load Experience → activate.

**Source learning path:**
- [14] Exploring Lyra § Experience chapter (best deep dive).
- Cross-ref: `Documents/GameDesign/14-Exploring_Lyra_GDD.md` ADR-1.

**API / Class chính:**
- `ULyraExperienceDefinition`.
- `ULyraExperienceManagerComponent::ServerSetCurrentExperience(FPrimaryAssetId)`.
- `OnLyraExperienceLoaded` delegate.

**Practice exercise (3 hour):**
1. Adopt LyraStarter project.
2. Create `EX_PaldarkLab_Raid` ExperienceDefinition.
3. Map default GameMode = `ALyraGameMode`.
4. PIE → confirm Experience loaded log.

**PALDARK take-away:**
- 🟢 2 Experience: `EX_Paldark_Hub` + `EX_Paldark_Raid` + future `EX_Paldark_Tutorial`.
- 🟢 Each Experience load different GameFeaturePlugin (Hub doesn't load combat plugin).

**Apply ở week:** Roadmap week 33.

---

### L2 — Game Feature Plugin (runtime modular) 🧪

**Goal:** Content as plugin, load runtime, decouple core.

**Concept:**
- Plugin folder `Plugins/PaldarkPalCombat/`.
- `.uplugin` declare Type=GameFeature.
- Plugin owns: DataAsset, GA, Animation, Mesh, BP.
- `UGameFeatureData` plugin definition asset.
- Activate via `UGameFeaturesSubsystem::LoadGameFeaturePlugin(URL, Options, OnComplete)`.

**Source learning path:**
- [14] Exploring Lyra § Game Feature chapter.
- Cross-ref: `Documents/GameDesign/14-Exploring_Lyra_GDD.md` ADR-2.

**Practice exercise (1 ngày):**
1. Create plugin `PaldarkLabCombat` (Game Feature Plugin).
2. Add GA_Fire + GA_Reload in plugin.
3. ExperienceDef reference plugin → Raid mode loads plugin.
4. Hub mode → plugin NOT loaded → combat GA not granted.

**PALDARK take-away:**
- 🟢 4 Game Feature Plugin:
  - `PaldarkCombat` — weapon + ammo + damage GA.
  - `PaldarkPalCore` — Pal taming, Activity FSM, ability.
  - `PaldarkPalPack1` — first DLC Pal pack (5 Pal).
  - `PaldarkHub` — Hub-specific actor + UI.

**Apply ở week:** Roadmap week 33-34.

---

### L3 — Modular Gameplay: AddComponents / AddAbilities / AddSpawners 🧪

**Goal:** Inject component/ability/spawner runtime, not constructor.

**Concept:**
- `UGameFeatureAction_AddComponents` — runtime add component to actor matching tag.
- `UGameFeatureAction_AddAbilities` — grant GA to ASC.
- `UGameFeatureAction_AddSpawners` — register spawner with WorldSubsystem.
- Workflow: Plugin activate → action execute → actors get component/ability.

**Source learning path:**
- [14] Exploring Lyra § Modular Gameplay chapter.
- Cross-ref: `Documents/GameDesign/14-Exploring_Lyra_GDD.md` ADR-3.

**Practice exercise (4 hour):**
1. ExperienceDef include `UGameFeatureAction_AddComponents` targeting Pawn with tag `Pawn.Hero`.
2. Action adds `UPaldarkPalCompanionComponent`.
3. Player Pawn has tag → action adds component.

**PALDARK take-away:**
- 🟢 Player Pawn skeleton bare — components injected via ExperienceDef.
- 🟢 Pal Pack DLC → AddComponents `UPalSpecies_FireLizardComp` runtime.

**Apply ở week:** Roadmap week 33.

---

### L4 — PawnData + InputConfig (Tag-based) 🧪

**Goal:** Data-driven Pawn — mesh, anim, ability, input all in DataAsset.

**Concept:**
- `ULyraPawnData` PrimaryDataAsset.
- Contains: PawnClass, AbilitySet, InputConfig, CameraMode.
- `ULyraInputConfig` map IA → GameplayTag.
- ASC grant ability with InputTag → input system route input via tag.

**Source learning path:**
- [14] Exploring Lyra § PawnData chapter.
- [15] Lyra RPG § Input chapter.

**PALDARK take-away:**
- 🟢 `PD_PaldarkPlayer` — mesh, anim, input config (16 IA).
- 🟢 `PD_PaldarkPal_Fire` — Fire Pal pawn data with specific abilities.
- 🟢 `PD_PaldarkPal_Water`, `PD_PaldarkPal_Grass`, etc.

**Apply ở week:** Roadmap week 33.

---

### L5 — CommonUI + Indicator Manager 🧪

**Goal:** Lyra UI stack + 3D world indicator.

**Concept:**
- `ULyraActivatableWidget` extends CommonUI.
- `ULyraIndicatorManagerComponent` register descriptor.
- `ULyraIndicatorLayer` UMG layer project 3D → 2D.

**Source learning path:**
- [14] Exploring Lyra § UI chapter.
- [15] Lyra RPG § FrontEnd chapter.

**PALDARK take-away:**
- 🟢 FrontEnd Menu Activatable Stack.
- 🟢 Indicator: Extract Beacon + Pal team + Enemy ping + Loot.

**Apply ở week:** Roadmap week 33.

---

### L6 — Lyra Equipment + QuickBar + Inventory (Lyra RPG) 🧪

**Goal:** Lyra inventory ecosystem.

**Concept:**
- `ULyraInventoryItemDefinition` — Lyra item.
- `ULyraEquipmentManagerComponent` — equip/unequip.
- `ULyraQuickBarComponent` — hotbar slot.

**Source learning path:**
- [15] Lyra RPG § Inventory chapter.

**Trade-off vs P11:**
- Lyra Inventory dùng `InventoryItemInstance` (Owner-stamped).
- PALDARK Phase 1-2 dùng Item Fragment pattern (course 09).
- → W33 adopt: Wrap Item Fragment into Lyra InventoryItemDefinition.

**PALDARK take-away:**
- 🟢 Adopt Lyra QuickBar (4 slot hotkey).
- 🟢 Adapt Item Fragment thành Lyra InventoryItemDefinition.

**Apply ở week:** Roadmap week 33-34.

---

## ⚡ Capstone Exercise

**Goal:** Adopt Lyra in PaldarkLab — replace manual scaffolding.

**Yêu cầu:**
1. Adopt Lyra Starter Project base.
2. Create 2 Experience: `EX_Lab_Hub` + `EX_Lab_Raid`.
3. 2 Game Feature Plugin: `LabCore` + `LabCombat`.
4. Modular Gameplay: Player Pawn empty → ExperienceDef inject Components.
5. PD_LabPlayer + PD_LabPal_Default.
6. CommonUI FrontEnd menu (Play/Settings/Quit).
7. IndicatorManager: 3 extract beacon in world.
8. QuickBar 4 slot, hotkey 1-4.

**Acceptance:** PIE → switch Experience Hub vs Raid → different content/ability.

**Effort:** 2 tuần (W33 + W34).

---

## Khi apply vào PALDARK roadmap

| Week | Pillar P17 usage |
|------|------------------|
| W33 | Lyra Starter adopt + Experience + Modular |
| W34 | PawnData + InputConfig + GameFeature Plugin |
| W34 | CommonUI + Indicator + QuickBar |
| W43+ | Pal Pack DLC via Game Feature Plugin |

---

## Anti-patterns (cảnh báo)

- 🔴 Adopt Lyra at W1 → too many concepts, can't separate Lyra from UE core.
- 🔴 Use Lyra as black box without understanding underlying GAS/Asset Manager.
- 🔴 Modify Lyra source directly → upgrade Lyra later painful. Inherit + extend.
- 🔴 Stick to Lyra defaults blindly when conflict with PALDARK design (e.g., Lyra Health AS doesn't have Sanity).
- 🔴 ExperienceDef chứa nhiều plugin không cần → load time bloat.

---

## Tham chiếu

- [`Documents/UE5_Core_Pillars.md`](../UE5_Core_Pillars.md) § P17.
- [`Documents/GameDesign/14-Exploring_Lyra_GDD.md`](../GameDesign/14-Exploring_Lyra_GDD.md) — Exploring Lyra deep dive.
- [`Documents/GameDesign/15-Lyra_RPG_GDD.md`](../GameDesign/15-Lyra_RPG_GDD.md) — Lyra RPG adapted.
- [`Documents/PALDARK/03-Roadmap_1_Year.md`](../PALDARK/03-Roadmap_1_Year.md) — W33-34 Lyra adoption.
