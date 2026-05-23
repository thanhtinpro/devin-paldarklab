// PALDARK W31-32 — Playable-map definition.

#include "Map/PaldarkMapDefinition.h"

UPaldarkMapDefinition::UPaldarkMapDefinition() = default;

FPrimaryAssetId UPaldarkMapDefinition::GetPrimaryAssetId() const
{
	// Lyra convention — type matches the row name in DefaultGame.ini's
	// AssetManagerSettings scan rules so editor + runtime discovery stay
	// aligned without an extra registration step.
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("PaldarkMapDefinition")), GetFName());
}

FString UPaldarkMapDefinition::GetDebugLabel() const
{
	if (!DisplayName.IsEmpty())
	{
		return DisplayName.ToString();
	}
	if (MapTag.IsValid())
	{
		return MapTag.ToString();
	}
	return GetName();
}
