// USandboxExperienceManager.cpp
#include "USandboxExperienceManager.h"
#include "USandboxExperienceDefinition.h"

DEFINE_LOG_CATEGORY_STATIC(LogSandboxExperience, Log, All);

void USandboxExperienceManager::RegisterExperience(const FString& Key, USandboxExperienceDefinition* Def)
{
    ExperienceRegistry.Add(Key, Def);
    UE_LOG(LogSandboxExperience, Log, TEXT("[Registry] Registered experience '%s' (%s)"), *Key, *Def->DisplayName);

    // First registered becomes default (SANDBOX: thay thế DefaultExperience CDO config)
    if (DefaultExperienceKey.IsEmpty())
    {
        DefaultExperienceKey = Key;
    }
}

void USandboxExperienceManager::RequestExperienceLoad(const FString& URLOptions)
{
    // STEP 1: Parse URL — mirrors PaldarkGameModeBase.cpp line 164
    // Trong thực tế: FString Requested = UGameplayStatics::ParseOption(Options, TEXT("Experience"));
    FString Requested;
    // Simple parse: find "?Experience=" in URL
    const FString SearchToken = TEXT("?Experience=");
    int32 Idx = URLOptions.Find(SearchToken);
    if (Idx != INDEX_NONE)
    {
        Requested = URLOptions.Mid(Idx + SearchToken.Len());
        // Trim at next ? if any
        int32 NextQ = Requested.Find(TEXT("?"));
        if (NextQ != INDEX_NONE) Requested = Requested.Left(NextQ);
    }

    // STEP 2: Fallback to default — mirrors line 165-169
    if (Requested.IsEmpty())
    {
        Requested = DefaultExperienceKey;
        UE_LOG(LogSandboxExperience, Log, TEXT("[RequestLoad] No URL option, fallback to default: '%s'"), *Requested);
    }
    else
    {
        UE_LOG(LogSandboxExperience, Log, TEXT("[RequestLoad] URL resolved: '%s'"), *Requested);
    }

    // STEP 3: Lookup in registry — mirrors UAssetManager::LoadPrimaryAsset
    USandboxExperienceDefinition** Found = ExperienceRegistry.Find(Requested);
    if (Found == nullptr || *Found == nullptr)
    {
        UE_LOG(LogSandboxExperience, Warning, TEXT("[RequestLoad] Experience '%s' not found in registry. bExperienceLoaded=true (fallback)."), *Requested);
        bExperienceLoaded = true;
        return;
    }

    ResolvedKey = Requested;
    UE_LOG(LogSandboxExperience, Log, TEXT("[RequestLoad] Async load started for '%s' (SANDBOX: synchronous)."), *Requested);

    // STEP 4: Callback — mirrors OnExperienceAssetReady
    OnExperienceAssetReady(*Found);
}

void USandboxExperienceManager::OnExperienceAssetReady(USandboxExperienceDefinition* Experience)
{
    LoadedExperience = Experience;

    // Apply overrides BEFORE resolving pawn — mirrors line 432-433
    ApplyClassOverrides(*Experience);
    ApplyActionSets(*Experience);

    // Resolve pawn class — mirrors line 441-451
    OverriddenPawnClass = Experience->PawnClassName;

    bExperienceLoaded = true;

    UE_LOG(LogSandboxExperience, Log,
        TEXT("[Ready] Experience '%s' loaded. Pawn=%s PC=%s PS=%s ActionSets=%d Tags=%d"),
        *ResolvedKey,
        *OverriddenPawnClass,
        *OverriddenPCClass,
        *OverriddenPSClass,
        Experience->ActionSets.Num(),
        ActiveTags.Num());

    // Set on GameState + broadcast — mirrors line 464-467
    // SANDBOX: Trong thực tế = APaldarkGameStateBase::SetCurrentExperience(AssetId, Experience)
    // + OnExperienceLoaded.Broadcast(Experience)
    OnExperienceLoaded.Broadcast(Experience);
}

void USandboxExperienceManager::ApplyClassOverrides(const USandboxExperienceDefinition& Exp)
{
    // Mirrors APaldarkGameModeBase::ApplyExperienceClassOverrides (line 470-504)
    if (!Exp.PlayerControllerClassName.IsEmpty())
    {
        OverriddenPCClass = Exp.PlayerControllerClassName;
        UE_LOG(LogSandboxExperience, Log, TEXT("[Override] PlayerController → %s"), *OverriddenPCClass);
    }
    else
    {
        OverriddenPCClass = TEXT("DefaultPC");
    }

    if (!Exp.PlayerStateClassName.IsEmpty())
    {
        OverriddenPSClass = Exp.PlayerStateClassName;
        UE_LOG(LogSandboxExperience, Log, TEXT("[Override] PlayerState → %s"), *OverriddenPSClass);
    }
    else
    {
        OverriddenPSClass = TEXT("DefaultPS");
    }
}

void USandboxExperienceManager::ApplyActionSets(const USandboxExperienceDefinition& Exp)
{
    // Mirrors APaldarkGameModeBase::ApplyExperienceActionSets (line 561-589)
    ActiveTags.Reset();
    ActiveTags.AppendTags(Exp.IntrinsicTags);

    for (const FSandboxActionSet& Set : Exp.ActionSets)
    {
        ActiveTags.AppendTags(Set.GrantedTags);
        if (!Set.DebugNote.IsEmpty())
        {
            UE_LOG(LogSandboxExperience, Log, TEXT("[ActionSet] %s"), *Set.DebugNote);
        }
        else
        {
            UE_LOG(LogSandboxExperience, Log, TEXT("[ActionSet] applied (%d tags)"), Set.GrantedTags.Num());
        }
    }
}

// Getters
FString USandboxExperienceManager::GetResolvedExperienceKey() const { return ResolvedKey; }
const USandboxExperienceDefinition* USandboxExperienceManager::GetLoadedExperience() const { return LoadedExperience; }
FGameplayTagContainer USandboxExperienceManager::GetActiveExperienceTags() const { return ActiveTags; }
FString USandboxExperienceManager::GetPlayerControllerClassName() const { return OverriddenPCClass; }
FString USandboxExperienceManager::GetPlayerStateClassName() const { return OverriddenPSClass; }
FString USandboxExperienceManager::GetPawnClassName() const { return OverriddenPawnClass; }
