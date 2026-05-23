#include "Experience/PaldarkExperienceDefinition.h"

UPaldarkExperienceDefinition::UPaldarkExperienceDefinition() = default;

FPrimaryAssetId UPaldarkExperienceDefinition::GetPrimaryAssetId() const
{
	// "PaldarkExperience" — kept short so the URL option `?Experience=...` stays
	// readable. Must match the type registered in UPaldarkAssetManager.
	return FPrimaryAssetId(TEXT("PaldarkExperience"), GetFName());
}
