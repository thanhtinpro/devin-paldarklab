#include "Pal/PaldarkPalDefinition.h"

UPaldarkPalDefinition::UPaldarkPalDefinition() = default;

FPrimaryAssetId UPaldarkPalDefinition::GetPrimaryAssetId() const
{
	// Lyra convention: <TypeName>:<AssetName>. The asset name (`GetFName()`)
	// matches the row name editors use, so console commands can spawn by name
	// (e.g. `Paldark.Pal.SpawnFromDefinition DA_PalDef_Direhound`).
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("PaldarkPalDefinition")), GetFName());
}

FString UPaldarkPalDefinition::GetDebugLabel() const
{
	if (!DisplayName.IsEmpty())
	{
		return DisplayName.ToString();
	}
	if (SpeciesTag.IsValid())
	{
		return SpeciesTag.ToString();
	}
	return GetName();
}
