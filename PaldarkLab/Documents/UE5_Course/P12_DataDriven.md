# P12 — Data-Driven Design

> Tự soạn từ: [01] Tom Looman, [11] Aura, [15] Lyra RPG, [04] ReadyOrNot.
>
> Data-driven = designer chỉnh balance không cần code. PALDARK 30+ Pal species + 50+ loot — bắt buộc data-driven.

---

## Pillar Identity

**Cái gì:** Tách logic khỏi data qua DataTable, DataAsset, PrimaryDataAsset, Curve.

**Scope:**
- ✅ DataTable (CSV/Excel import) + FTableRowBase.
- ✅ DataAsset (single-instance config).
- ✅ PrimaryDataAsset (asset manager indexable).
- ✅ Curve Table (continuous variable — XP, damage scaling).
- ✅ Soft Reference vs Hard Reference.
- ✅ FPrimaryAssetId + FPrimaryAssetType.
- ✅ Asset Manager registry.

**Out of scope:**
- ❌ Async load detail (P14).
- ❌ Item Fragment composition (P11).

---

## PALDARK cần gì từ pillar này?

| Data type | Asset class | Count |
|-----------|-------------|-------|
| Pal Species | `UPaldarkPalDefinition` PrimaryDataAsset | 12 (beta) → 30+ (Q4) |
| Item | `UPaldarkItemDefinition` PrimaryDataAsset | 50+ |
| Enemy NPC | `UPaldarkEnemyDefinition` PrimaryDataAsset | 8 (beta) |
| Civilian Variant | `UPaldarkCivilianDefinition` PrimaryDataAsset | 5 |
| Loot Drop Table | DataTable | 30 |
| XP Curve | Curve Table | 1 (shared) |
| Damage Scaling | Curve Table | 1 (per weapon type) |
| Pal Bond Trust Curve | Curve Table | 1 |
| Dark Zone Fog Curve | Curve Table | 1 (24-min cycle) |
| Cosmetic Customization | DataTable | 50+ |

---

## Sources Matrix

| Source | Coverage | Lesson lấy từ đó |
|--------|----------|------------------|
| [01] Tom Looman | 🟢 | L1 DataTable basics, L2 DataAsset, L3 Asset Manager |
| [11] Aura | 🟢 | L4 Curve Table cho XP/Damage |
| [04] RoN GDD | 🟢 | L5 42-DataAsset pattern |
| [15] Lyra RPG | 🟡 | L6 Lyra Experience = DataAsset |

---

## Prerequisite

- P01 (USTRUCT, UCLASS).
- P03 (Component composition).

---

## Lessons

### L1 — DataTable + FTableRowBase 🧪

**Goal:** Define struct row + import CSV/Excel.

**Concept:**
- `USTRUCT(BlueprintType) struct FPalSpawnTableRow : public FTableRowBase` — row schema.
- Editor: Right-click → Miscellaneous → DataTable → pick row struct.
- Import CSV: matching column names → row.
- Lookup: `DataTable->FindRow<FPalSpawnTableRow>(FName("PalA"), Context)`.

**Source learning path:**
- [01] Tom Looman § DataTable chapter (bot spawn).

**API / Class chính:**
- `UDataTable`
- `FTableRowBase`
- `UDataTable::FindRow<T>(RowName, Context)`

**Practice exercise (2 hour):**
1. `FLabSpawnRow` struct: ActorClass + Weight + Cooldown.
2. CSV: 3 row, import to DataTable.
3. Random pick by weight in GameMode `SpawnTimer`.

**PALDARK take-away:**
- 🟢 Loot drop table (NPC → loot pool) qua DataTable.
- 🟢 Customization cosmetic (head/body/Pal-color) qua DataTable.

**Apply ở week:** Roadmap week 20.

---

### L2 — DataAsset + PrimaryDataAsset 🧪

**Goal:** Define single-instance config asset.

**Concept:**
- `UDataAsset` simple — load with `UAssetManager::GetAssetData`.
- `UPrimaryDataAsset` — Asset Manager indexable, FPrimaryAssetId, async-loadable.
- Override `GetPrimaryAssetId()` return `FPrimaryAssetId("PalDef", GetFName())`.
- In `Project Settings → Asset Manager`, register Primary Asset Type `PalDef` with class + scan dir.

**Source learning path:**
- [01] Tom Looman § DataAsset chapter (enemy config).

**API / Class chính:**
- `UPrimaryDataAsset::GetPrimaryAssetId()`.
- `UAssetManager::GetPrimaryAssetIdList(AssetType, OutList)`.
- `UAssetManager::GetPrimaryAssetObject<T>(AssetId)`.

**Practice exercise (3 hour):**
1. `UPaldarkLabEnemyDefinition` PrimaryDataAsset (Name + Class + Health + Damage).
2. Create 3 enemy asset.
3. `Project Settings → Asset Manager` register `EnemyDef` type.
4. GameMode spawn: query `UAssetManager` → randomized pick.

**PALDARK take-away:**
- 🟢 Pal + Item + Enemy đều dùng PrimaryDataAsset.

**Apply ở week:** Roadmap week 15-19.

---

### L3 — Asset Manager + Soft Reference 🧪

**Goal:** Lazy load asset, không hard-ref toàn bộ.

**Concept:**
- `TSoftObjectPtr<UTexture2D>` — soft ref, lazy load.
- `TSoftClassPtr<APaldarkPalCharacter>` — class soft ref.
- `UAssetManager::GetStreamableManager().RequestAsyncLoad(SoftPtr.ToSoftObjectPath(), OnLoaded)`.
- Hard ref (`UTexture2D*`) → load immediately on owner load.
- Soft ref → only loaded when explicitly request.

**Source learning path:**
- [01] Tom Looman § Async UI chapter.

**Practice exercise:**
1. `UPaldarkLabEnemyDefinition` chứa `TSoftObjectPtr<UTexture2D> IconTex`.
2. Inventory UI lazy load icon when item appears.

**PALDARK take-away:**
- 🟢 Mọi icon, mesh, montage trong Item/Pal Definition là soft ref.
- 🟢 Async load via `FStreamableManager`.

**Apply ở week:** Roadmap week 19.

---

### L4 — Curve Table (Aura XP / Damage scaling) 🧪

**Goal:** Continuous variable per level.

**Concept:**
- `UCurveTable` — many curve in 1 asset.
- Each curve = X (level) → Y (value).
- Lookup: `CurveTable->FindCurve(FName("XP_To_Next_Level"), Context)->Eval(CurrentLevel)`.

**Source learning path:**
- [11] Aura § Curve Table chapter (CT_XP, CT_Damage).

**Practice exercise:**
1. `CT_LabXP` curve: level 1 = 100 XP, level 10 = 1000, level 50 = 10000 (exponential).
2. PlayerState `AddXP(int Amount)` check curve for next level threshold.

**PALDARK take-away:**
- 🟢 5 Curve Table: XP_To_Next, Damage_Scaling, Sanity_Decay, Bond_Trust_Curve, Fog_Density.

**Apply ở week:** Roadmap week 15.

---

### L5 — 42-DataAsset pattern (RoN reference) 📖

**Goal:** Học từ scale lớn — RoN dùng 42 DataAsset cho mọi gameplay config.

**Concept:**
RoN có 42 PrimaryDataAsset class:
- WeaponDef, AttachmentDef, AmmoDef
- AIBehaviorDef, AIActivityDef, AISenseDef
- LevelDef, RoomDef, DoorDef
- CivilianDef, SuspectDef
- CustomizationDef (uniform, helmet, vest, etc.)
- TutorialDef, ObjectiveDef, MissionDef
- ... etc.

**Pattern:**
- Mọi gameplay tuning → DataAsset.
- C++ chỉ wire data → behavior.
- Designer change DataAsset → game change. No compile.

**Source learning path:**
- [04] RoN GDD § Phase 5 ADR-6.

**PALDARK take-away:**
- 🟢 Target 30 DataAsset cho PALDARK beta:
  - PalDef, ItemDef, EnemyDef, CivilianDef
  - WeaponDef, AmmoDef, AttachmentDef
  - LevelDef, ExtractZoneDef, FogProfileDef
  - PalAbilityDef, PalSpeciesDef
  - InputConfig, AnimMontageSet
  - CustomizationDef (player, Pal)
  - LootTable (DataTable, 30 ref-able)
- 🟢 Designer-friendly → easy iteration.

**Apply ở week:** Mỗi week có new DataAsset.

---

### L6 — Lyra Experience as DataAsset 📖

**Goal:** Lyra mode definition = DataAsset.

**Concept:**
- `ULyraExperienceDefinition` PrimaryDataAsset.
- Contains: PawnData + GameFeaturePluginList + ActionSetList.
- Activate → load plugin + register components/abilities.

**Source learning path:**
- [14] Exploring Lyra § Experience chapter.

**PALDARK take-away:**
- 🟢 `UPaldarkExperienceDef_Raid` — define Raid match (pawn data, plugin Pal Pack 1, fog level).
- 🟢 `UPaldarkExperienceDef_Hub` — define Hub (peaceful mode).

**Apply ở week:** Roadmap week 33 (Lyra adopt).

---

## ⚡ Capstone Exercise

**Goal:** PaldarkLab full data-driven sandbox.

**Yêu cầu:**
1. 5 PrimaryDataAsset type: PalDef, ItemDef, EnemyDef, WeaponDef, LevelDef.
2. 3 DataTable: LootTable, SpawnTable, CustomizationTable.
3. 3 Curve Table: XP, Damage, Sanity.
4. Asset Manager register all types.
5. Designer change DataAsset → game change without compile.

**Acceptance:** Editor edit Pal Health = 200 → next PIE Pal has Health 200 (no compile).

**Effort:** 1 tuần.

---

## Khi apply vào PALDARK roadmap

| Week | Pillar P12 usage |
|------|------------------|
| W15 | XP + Damage Curve |
| W18 | Item Def + Frag |
| W19 | Equipment slot tag |
| W20 | Loot table |
| W21 | Enemy Def |
| W23 | Backpack tier table |
| W31 | Pal Def + Pal Ability Def |
| W33 | Lyra Experience Def |
| W43 | Pal Pack 1 DataAsset |

---

## Anti-patterns (cảnh báo)

- 🔴 Hardcode in C++ thay vì DataAsset — designer can't iterate.
- 🔴 Hard reference asset → load all on level start.
- 🔴 GameMode chứa balance number — should be in DataAsset.
- 🔴 No Asset Manager register → `GetPrimaryAssetIdList` returns empty.
- 🔴 Edit DataAsset value but no `MarkPackageDirty()` → save lost.

---

## Tham chiếu

- [`Documents/UE5_Core_Pillars.md`](../UE5_Core_Pillars.md) § P12.
- [`Documents/GameDesign/01-ActionRoguelike_GDD.md`](../GameDesign/01-ActionRoguelike_GDD.md) — DataTable + DataAsset basics.
- [`Documents/GameDesign/11-Aura_RPG_GDD.md`](../GameDesign/11-Aura_RPG_GDD.md) — Curve Table.
- [`Documents/GameDesign/04-ReadyOrNot_GDD.md`](../GameDesign/04-ReadyOrNot_GDD.md) — 42-DataAsset reference.
