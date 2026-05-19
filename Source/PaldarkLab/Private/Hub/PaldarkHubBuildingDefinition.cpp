// PALDARK W40-41 — Hub building definition data asset.

#include "Hub/PaldarkHubBuildingDefinition.h"

FPrimaryAssetId UPaldarkHubBuildingDefinition::GetPrimaryAssetId() const
{
	// Mirrors UPaldarkPalDefinition / UPaldarkMapDefinition: the registered
	// `PrimaryAssetType` from DefaultGame.ini AssetTypesToScan is
	// `PaldarkHubBuilding`. AssetName uses the package's short name so the
	// designer-renamed `DA_HubBuilding_<Stable|Marketplace|Briefing>` is what
	// the asset manager keys the registry on.
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("PaldarkHubBuilding")), GetFName());
}
