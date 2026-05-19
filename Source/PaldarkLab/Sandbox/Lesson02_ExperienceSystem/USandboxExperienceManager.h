// USandboxExperienceManager.h
// SANDBOX: Gộp logic từ APaldarkGameModeBase + UPaldarkAssetManager + APaldarkGameStateBase
// vào 1 class duy nhất để tối giản. Trong thực tế:
//   - APaldarkGameModeBase::RequestExperienceLoad() parse URL + kick async
//   - UPaldarkAssetManager::LoadPrimaryAsset() async load
//   - APaldarkGameModeBase::OnExperienceAssetReady() apply overrides + action sets
//   - APaldarkGameStateBase::SetCurrentExperience() replicate to clients

#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "USandboxExperienceManager.generated.h"

class USandboxExperienceDefinition;

// SANDBOX: Trong thực tế = APaldarkGameStateBase::OnExperienceLoaded delegate
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSandboxExperienceLoaded, const USandboxExperienceDefinition*);

UCLASS()
class USandboxExperienceManager : public UObject
{
    GENERATED_BODY()
public:
    // Registry: add experience definitions (SANDBOX: thay thế AssetManager scan)
    void RegisterExperience(const FString& ExperienceKey, USandboxExperienceDefinition* Definition);

    // Core flow entry — mirrors APaldarkGameModeBase::RequestExperienceLoad
    // Parse URL → resolve → "async load" → apply → broadcast
    void RequestExperienceLoad(const FString& URLOptions);

    // Current state (SANDBOX: thay thế APaldarkGameStateBase::CurrentExperienceId)
    FString GetResolvedExperienceKey() const;
    const USandboxExperienceDefinition* GetLoadedExperience() const;
    FGameplayTagContainer GetActiveExperienceTags() const;

    // Overridden class names (SANDBOX: thay thế GameMode's PlayerControllerClass, etc.)
    FString GetPlayerControllerClassName() const;
    FString GetPlayerStateClassName() const;
    FString GetPawnClassName() const;

    // Delegate
    FOnSandboxExperienceLoaded OnExperienceLoaded;

private:
    // SANDBOX: Thay thế UAssetManager registry
    TMap<FString, USandboxExperienceDefinition*> ExperienceRegistry;

    // SANDBOX: Thay thế APaldarkGameModeBase::DefaultExperience
    FString DefaultExperienceKey;

    // Resolved state
    FString ResolvedKey;
    const USandboxExperienceDefinition* LoadedExperience = nullptr;
    FGameplayTagContainer ActiveTags;
    FString OverriddenPCClass;
    FString OverriddenPSClass;
    FString OverriddenPawnClass;
    bool bExperienceLoaded = false;

    // Internal steps — each mirrors a real function
    void OnExperienceAssetReady(USandboxExperienceDefinition* Experience);
    void ApplyClassOverrides(const USandboxExperienceDefinition& Exp);
    void ApplyActionSets(const USandboxExperienceDefinition& Exp);
};
