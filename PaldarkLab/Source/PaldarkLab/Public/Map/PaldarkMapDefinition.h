// PALDARK W31-32 — Playable-map definition (UPrimaryDataAsset).
//
// One DataAsset per playable map under `/Game/Paldark/Maps/DA_MapDef_<Name>`.
// Carries the soft reference to the actual `.umap`, a stable map identity tag
// (`Paldark.Map.*`), and the recommended POI / spawn-point counts the designer
// is expected to author. This is the Q3 § Tuần 31-32 deliverable that lets
// experiences (W1) reference *which map* they ship with — previously the
// experience leaned on `[/Script/EngineSettings.GameMapsSettings]` defaults +
// the map's own World Override which is brittle for multi-map projects.
//
// Two access paths:
//   1. `UPaldarkExperienceDefinition::MapDefinition` — soft ref. The
//      experience pre-loads it on startup and logs the resolved map id +
//      recommended POI / spawn counts so designers can diff "what map is
//      supposed to ship" vs "what level the server actually opened".
//   2. Console command `Paldark.Map.LoadDefinition <RowName>` — async load
//      a map definition by primary-asset row name + dump the resolved fields
//      to LogPaldarkLab. Useful for verifying the AssetManager scan picked
//      up the new directory after authoring a new `DA_MapDef_<X>`.
//
// Registered as primary asset type `PaldarkMapDefinition` via both
// `Config/DefaultGame.ini` and the editor scan rule. The primary asset id is
// `PaldarkMapDefinition:<RowName>` matching the Lyra convention used by every
// other Paldark data asset (Experience / PawnData / ActionSet / InputConfig /
// Item / PalDefinition / SquadCommandSet).
//
// Designer-side authoring contract (see README.md § W31-32):
//   - Drop a `DA_MapDef_RungHong` in `/Game/Paldark/Maps/`.
//   - Set `MapTag = Paldark.Map.RungHong` + `MapAsset = /Game/Paldark/Maps/Map_RungHong`.
//   - Set `RecommendedPlayerCount`, `RecommendedPOICount`, `RecommendedSpawnPointCount`.
//   - Populate `IntrinsicTagsToAssert` with the POI types + species tags
//     expected on the level.
//
// What this is NOT (deferred):
//   - No level-streaming partition list — Map 2 is 1×1km block-out, not
//     World Partition. Partition wiring lands W48-49 when significance
//     manager culling tooling exists.
//   - No per-level GameMode override on this asset. Use the experience's
//     `RequiredGameModeClass` instead. The map asset stays GameMode-agnostic
//     so two experiences can share one map (`Raid_Sandbox` + a hypothetical
//     `Tutorial_Sandbox`).
//   - No procedural fill / seed metadata. Procedural map gen lands W34-35.
//   - No `AssociatedExperience` back-reference. The relationship is intentionally
//     one-way (Experience → Map) so a single map can be reused by multiple
//     experiences without circular data-asset references.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PaldarkMapDefinition.generated.h"

class UWorld;

UCLASS(BlueprintType, Const, meta = (DisplayName = "Paldark Map Definition"))
class PALDARKLAB_API UPaldarkMapDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPaldarkMapDefinition();

	// Identity ----------------------------------------------------------

	// Stable identity tag for this map. Must match a tag under
	// `Paldark.Map.*` (e.g. `Paldark.Map.RungHong`). The experience load
	// pipeline logs this on resolve so designers can verify
	// "what experience expects" matches "what map the server opened".
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Map|Definition",
		meta = (Categories = "Paldark.Map"))
	FGameplayTag MapTag;

	// Localised display name used by the future hub-town map picker UI
	// (W40-41) + console dump output.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Map|Definition")
	FText DisplayName;

	// Soft reference to the actual `.umap` shipped under
	// `/Game/Paldark/Maps/Map_<Name>`. Soft so the data asset can sit in
	// the experience without root-loading the entire level on AssetManager
	// init. The experience pipeline pre-loads this via `LoadPrimaryAsset`
	// alongside the experience itself.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Map|Definition")
	TSoftObjectPtr<UWorld> MapAsset;

	// Designer-side hints (recommended counts). These are *not* enforced by
	// the runtime — they exist so playtest checklists + the validator can
	// flag "Map 2 should have 8 POIs but I see 3 in the live level"
	// without having to load the .umap on the CI runner.

	// Recommended number of players this map is balanced for. Default 4
	// matches the W14-15 dedicated server target.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Map|Definition|Hints",
		meta = (ClampMin = "1", UIMin = "1"))
	int32 RecommendedPlayerCount = 4;

	// Recommended number of `APaldarkPointOfInterest` actors on the map.
	// Map 2 "Rừng Hỏng" roadmap target = 8 (3 SafeZone + 1 ExtractionPad +
	// 2 Cache + 1 RuinedTower + 1 MiniBoss). Map 1 "RaidSandbox" target
	// = 4 (sandbox sized).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Map|Definition|Hints",
		meta = (ClampMin = "0", UIMin = "0"))
	int32 RecommendedPOICount = 8;

	// Recommended number of `APaldarkPalSpawnPoint` actors. Map 2 target
	// = 16 (10 hostile + 6 companion test points). Map 1 target = 0 — the
	// sandbox uses the W20-21 `APaldarkHostilePalSpawner` batch path.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Map|Definition|Hints",
		meta = (ClampMin = "0", UIMin = "0"))
	int32 RecommendedSpawnPointCount = 16;

	// Tags asserted on this map at load time. Concrete consumers
	// (HUD breadcrumb, AI Director density planner) gate behaviour off
	// the union of these tags with the experience's `IntrinsicTags`.
	// Intended set: every POI type expected (`Paldark.POI.Type.*`) +
	// every species expected on the map (`Paldark.Pal.Species.*`).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Map|Definition|Tags")
	FGameplayTagContainer IntrinsicTagsToAssert;

	// W33-34 — Indoor close-quarter hint. When true, AI Director density
	// planner (Q4) caps concurrent hostile count + biases spawn-point
	// activation toward closer player range, and Significance Manager
	// (W48-49) uses tighter LOD distances. Map 3 "Cơ Xưởng PalCorp" sets
	// this true; Map 1 "RaidSandbox" + Map 2 "Rừng Hỏng" leave it false.
	// Pure designer hint — not consumed by the W33-34 runtime, just
	// surfaced via `Paldark.Map.LoadDefinition` for now.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Map|Definition|Hints")
	bool bIsIndoor = false;

	// Primary asset id with the `PaldarkMapDefinition` type. Matches the
	// row in `[/Script/Engine.AssetManagerSettings]` (see
	// `Config/DefaultGame.ini`).
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	// Debug label used by `Paldark.Map.LoadDefinition` console output.
	// Returns `DisplayName` when set, otherwise `MapTag` string, otherwise
	// the asset name.
	FString GetDebugLabel() const;
};
