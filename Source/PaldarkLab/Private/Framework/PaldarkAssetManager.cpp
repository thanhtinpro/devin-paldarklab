#include "Framework/PaldarkAssetManager.h"

#include "PaldarkLogCategories.h"
#include "Experience/PaldarkExperienceDefinition.h"

UPaldarkAssetManager::UPaldarkAssetManager() = default;

UPaldarkAssetManager& UPaldarkAssetManager::Get()
{
	UAssetManager& Manager = UAssetManager::Get();
	UPaldarkAssetManager* Cast = ::Cast<UPaldarkAssetManager>(&Manager);
	checkf(
		Cast != nullptr,
		TEXT("AssetManagerClassName in DefaultEngine.ini must be set to /Script/PaldarkLab.PaldarkAssetManager; "
		     "got %s instead."),
		*Manager.GetClass()->GetPathName());
	return *Cast;
}

const UPaldarkExperienceDefinition* UPaldarkAssetManager::GetExperienceSync(const FPrimaryAssetId& AssetId) const
{
	if (!AssetId.IsValid())
	{
		UE_LOG(LogPaldark, Warning, TEXT("GetExperienceSync called with invalid asset id."));
		return nullptr;
	}

	// Synchronous load is acceptable for console-driven hello-world; production
	// path uses async via FStreamableManager (see PaldarkGameModeBase).
	const FSoftObjectPath Path = GetPrimaryAssetPath(AssetId);
	if (Path.IsNull())
	{
		UE_LOG(LogPaldark, Warning, TEXT("No registered path for asset id %s."), *AssetId.ToString());
		return nullptr;
	}

	UObject* Loaded = Path.TryLoad();
	const UPaldarkExperienceDefinition* Experience = ::Cast<UPaldarkExperienceDefinition>(Loaded);
	if (Experience == nullptr)
	{
		UE_LOG(
			LogPaldark,
			Warning,
			TEXT("Asset id %s resolved to %s, which is not a UPaldarkExperienceDefinition."),
			*AssetId.ToString(),
			Loaded != nullptr ? *Loaded->GetClass()->GetPathName() : TEXT("nullptr"));
	}
	return Experience;
}

void UPaldarkAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

	// Primary asset types must match those declared in
	// DefaultGame.ini -> [/Script/Engine.AssetManagerSettings].PrimaryAssetTypesToScan
	// for the editor's Asset Manager UI to align with runtime registration.
	// W27-28: PaldarkPalDefinition added to the manifest so designers can author
	// /Game/Paldark/Pals/DA_PalDef_<Name> and pre-warm them via the experience's
	// PreWarmPalDefinitions field (loaded by UPaldarkPalSpawnSubsystem).
	UE_LOG(LogPaldark, Log, TEXT("UPaldarkAssetManager::StartInitialLoading — primary asset types registered via ini (incl. PaldarkPalDefinition)."));
}
