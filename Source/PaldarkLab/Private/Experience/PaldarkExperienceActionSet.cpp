#include "Experience/PaldarkExperienceActionSet.h"

UPaldarkExperienceActionSet::UPaldarkExperienceActionSet() = default;

FPrimaryAssetId UPaldarkExperienceActionSet::GetPrimaryAssetId() const
{
	// Matches the type registered in DefaultGame.ini under
	// [/Script/Engine.AssetManagerSettings].
	return FPrimaryAssetId(TEXT("PaldarkExperienceActionSet"), GetFName());
}
