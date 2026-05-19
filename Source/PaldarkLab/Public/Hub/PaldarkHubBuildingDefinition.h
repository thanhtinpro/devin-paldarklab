// PALDARK W40-41 — Hub building definition data asset.
//
// Per-instance designer config for a hub building actor. One authored
// `DA_HubBuilding_<Stable|Marketplace|Briefing>` under /Game/Paldark/Hub/ holds
// the display name, interact radius, interact input tag, plus the per-type
// payload (marketplace catalog, stable slot count, briefing-room allowed
// raid maps).
//
// Lives in /Hub/ so the experience / map data assets can stay focused on their
// own concerns. The actor (`APaldarkHubBuilding`) reads this on BeginPlay
// (after async load) and stamps its `BuildingTag` / interact-radius sphere /
// debug label from the loaded asset.
//
// W42-43 backend AWS will read these definitions server-side to bill
// marketplace transactions, persist Pal Stable roster slots, and enforce
// briefing-room raid-map allow-lists. Keep the contract stable.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PaldarkHubBuildingDefinition.generated.h"

class UPaldarkItemDefinition;
class UPaldarkMapDefinition;

UCLASS(BlueprintType, Const, meta = (DisplayName = "Paldark Hub Building"))
class PALDARKLAB_API UPaldarkHubBuildingDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Building type tag — must be one of Paldark.Hub.Building.{PalStable,
	// Marketplace,BriefingRoom}. The hub subsystem's GetBuildingsByTag query
	// keys on this so future UMG / interact code looks up a stable contract
	// instead of the UClass of the actor subclass.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Hub",
		meta = (Categories = "Paldark.Hub.Building"))
	FGameplayTag BuildingTag;

	// Human-readable label, shown in logs / on-screen debug / UI.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Hub")
	FText DisplayName;

	// Interact radius (cm). Sphere component on the actor uses this both as
	// the editor debug gizmo size and as the overlap probe range. 400 cm
	// (= 4 m) keeps the prompt tight to the prop without forcing a precise
	// camera-pixel-aim.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Hub",
		meta = (ClampMin = "10.0", UIMin = "10.0", UIMax = "2000.0"))
	float InteractRadius = 400.0f;

	// Input tag the player presses while inside InteractRadius to trigger
	// the building's OnInteract path. Must be one of
	// Paldark.InputTag.Interact.{Stable,Marketplace,Briefing}.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Hub",
		meta = (Categories = "Paldark.InputTag.Interact"))
	FGameplayTag InteractInputTag;

	// Pal Stable — number of roster slots the stable exposes. Read by
	// `APaldarkPalStable::OnInteract` to size the swap UI. Ignored by
	// Marketplace / Briefing room definitions. 0 means "use the player's
	// existing UPaldarkPalRosterComponent capacity unmodified".
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Hub|PalStable",
		meta = (ClampMin = "0", UIMin = "0", UIMax = "32"))
	int32 RosterSlotCount = 0;

	// Marketplace — soft refs to the item definitions on sale at this
	// marketplace kiosk. `APaldarkMarketplaceKiosk::OnInteract` async-loads
	// them on first interaction so designers don't pay the streaming cost
	// at hub map load. Empty for non-marketplace definitions.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Hub|Marketplace")
	TArray<TSoftObjectPtr<UPaldarkItemDefinition>> MarketplaceCatalog;

	// Briefing room — soft refs to the map definitions players are allowed
	// to queue from this briefing room. `APaldarkBriefingRoom::OnInteract`
	// validates `PendingRaidMapTag` against this allow-list before issuing
	// the `HostHubServer → HostListenServer` ServerTravel. Empty means
	// "any registered map" — only acceptable for designer test maps.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Hub|BriefingRoom")
	TArray<TSoftObjectPtr<UPaldarkMapDefinition>> AllowedRaidMaps;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
