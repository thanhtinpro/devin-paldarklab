#include "ATestExperienceActor.h"
#include "USandboxExperienceManager.h"
#include "USandboxExperienceDefinition.h"
#include "SandboxExpTags.h"
#include "GameplayTagContainer.h"

DEFINE_LOG_CATEGORY_STATIC(LogSandboxExpTest, Log, All);

void ATestExperienceActor::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogSandboxExpTest, Log, TEXT("=== LESSON 02: Experience System ==="));

    Test01_URLOption_SelectsRaid();
    Test02_NoURLOption_FallbackToDefault();
    Test03_InvalidExperience_GracefulFallback();
    Test04_ClassOverrides_PCandPS();
    Test05_ActionSets_MergeTags();
    Test06_SameMap_DifferentRules();
    Test07_OnExperienceLoadedDelegate();
    Test08_MaxPlayers_DifferentPerExperience();

    UE_LOG(LogSandboxExpTest, Log, TEXT("=== END LESSON 02 ==="));
}

USandboxExperienceDefinition* ATestExperienceActor::CreateRaidExperience()
{
    USandboxExperienceDefinition* Exp = NewObject<USandboxExperienceDefinition>(this);
    Exp->DisplayName = TEXT("Raid Sandbox");
    Exp->PawnClassName = TEXT("PaldarkCharacter");
    Exp->PlayerControllerClassName = TEXT("RaidPlayerController");
    Exp->PlayerStateClassName = TEXT("RaidPlayerState");
    Exp->MaxPlayers = 4;
    Exp->HelloWorldMessage = TEXT("Welcome to Raid!");
    // IntrinsicTags: request "Sandbox.Mode.Raid" tag
    Exp->IntrinsicTags.AddTag(FGameplayTag(SandboxExpTags::TAG_Sandbox_Mode_Raid));

    // ActionSet 1: Combat
    FSandboxActionSet CombatSet;
    CombatSet.DebugNote = TEXT("CombatLoop ActionSet");
    CombatSet.GrantedTags.AddTag(FGameplayTag(SandboxExpTags::TAG_Sandbox_Feature_Combat));
    CombatSet.GrantedTags.AddTag(FGameplayTag(SandboxExpTags::TAG_Sandbox_Feature_LagComp));
    Exp->ActionSets.Add(CombatSet);

    return Exp;
}

USandboxExperienceDefinition* ATestExperienceActor::CreateHubExperience()
{
    USandboxExperienceDefinition* Exp = NewObject<USandboxExperienceDefinition>(this);
    Exp->DisplayName = TEXT("Hub Town");
    Exp->PawnClassName = TEXT("HubCharacter");
    Exp->PlayerControllerClassName = TEXT("HubPlayerController");
    Exp->PlayerStateClassName = TEXT("HubPlayerState");
    Exp->MaxPlayers = 8;
    Exp->HelloWorldMessage = TEXT("Welcome to Hub Town!");
    Exp->IntrinsicTags.AddTag(FGameplayTag(SandboxExpTags::TAG_Sandbox_Mode_Hub));

    // ActionSet: Social features, no combat
    FSandboxActionSet SocialSet;
    SocialSet.DebugNote = TEXT("Social ActionSet");
    SocialSet.GrantedTags.AddTag(FGameplayTag(SandboxExpTags::TAG_Sandbox_Feature_Trading));
    SocialSet.GrantedTags.AddTag(FGameplayTag(SandboxExpTags::TAG_Sandbox_Feature_Stable));
    Exp->ActionSets.Add(SocialSet);

    return Exp;
}

USandboxExperienceManager* ATestExperienceActor::CreateManager()
{
    USandboxExperienceManager* Mgr = NewObject<USandboxExperienceManager>(this);
    Mgr->RegisterExperience(TEXT("RaidSandbox"), CreateRaidExperience());
    Mgr->RegisterExperience(TEXT("HubTown"), CreateHubExperience());
    return Mgr;
}

void ATestExperienceActor::Test01_URLOption_SelectsRaid()
{
    // FLOW: URL contains ?Experience=RaidSandbox → manager resolves Raid experience
    // WHY: Same map, different URL = different game rules
    USandboxExperienceManager* Mgr = CreateManager();
    Mgr->RequestExperienceLoad(TEXT("MapName?Experience=RaidSandbox"));
    bool bPass = Mgr->GetResolvedExperienceKey() == TEXT("RaidSandbox");
    LogResult(TEXT("Test01_URLOption_SelectsRaid"), bPass,
        FString::Printf(TEXT("ResolvedKey='%s'"), *Mgr->GetResolvedExperienceKey()));
}

void ATestExperienceActor::Test02_NoURLOption_FallbackToDefault()
{
    // FLOW: No ?Experience= in URL → fallback to first registered (default)
    // WHY: Server must always have SOME experience, even without URL params
    USandboxExperienceManager* Mgr = CreateManager();
    Mgr->RequestExperienceLoad(TEXT("MapName")); // No ?Experience=
    bool bPass = Mgr->GetResolvedExperienceKey() == TEXT("RaidSandbox"); // first registered
    LogResult(TEXT("Test02_NoURLOption_FallbackToDefault"), bPass,
        FString::Printf(TEXT("ResolvedKey='%s' (expected 'RaidSandbox' as default)"), *Mgr->GetResolvedExperienceKey()));
}

void ATestExperienceActor::Test03_InvalidExperience_GracefulFallback()
{
    // FLOW: ?Experience=NonExistent → warning log + bExperienceLoaded=true (no crash)
    // WHY: Graceful degradation — server doesn't crash if designer typos experience name
    USandboxExperienceManager* Mgr = CreateManager();
    Mgr->RequestExperienceLoad(TEXT("MapName?Experience=NonExistent"));
    bool bPass = Mgr->GetLoadedExperience() == nullptr;
    LogResult(TEXT("Test03_InvalidExperience_GracefulFallback"), bPass,
        TEXT("LoadedExperience is nullptr (graceful fallback, check Warning log above)"));
}

void ATestExperienceActor::Test04_ClassOverrides_PCandPS()
{
    // FLOW: Raid experience overrides PC to "RaidPlayerController", PS to "RaidPlayerState"
    // WHY: Different experiences need different player behavior (Raid has combat input, Hub has trading UI)
    USandboxExperienceManager* Mgr = CreateManager();
    Mgr->RequestExperienceLoad(TEXT("MapName?Experience=RaidSandbox"));
    bool bPC = Mgr->GetPlayerControllerClassName() == TEXT("RaidPlayerController");
    bool bPS = Mgr->GetPlayerStateClassName() == TEXT("RaidPlayerState");
    LogResult(TEXT("Test04_ClassOverrides_PCandPS"), bPC && bPS,
        FString::Printf(TEXT("PC=%s PS=%s"), *Mgr->GetPlayerControllerClassName(), *Mgr->GetPlayerStateClassName()));
}

void ATestExperienceActor::Test05_ActionSets_MergeTags()
{
    // FLOW: Raid has IntrinsicTags (Sandbox.Mode.Raid) + ActionSet (Combat, LagComp)
    // ActiveTags = union of both
    // WHY: Composition — same base experience + mix-and-match action sets
    USandboxExperienceManager* Mgr = CreateManager();
    Mgr->RequestExperienceLoad(TEXT("MapName?Experience=RaidSandbox"));
    FGameplayTagContainer ActiveTags = Mgr->GetActiveExperienceTags();
    bool bHasRaid = ActiveTags.HasTag(FGameplayTag(SandboxExpTags::TAG_Sandbox_Mode_Raid));
    bool bHasCombat = ActiveTags.HasTag(FGameplayTag(SandboxExpTags::TAG_Sandbox_Feature_Combat));
    bool bHasLagComp = ActiveTags.HasTag(FGameplayTag(SandboxExpTags::TAG_Sandbox_Feature_LagComp));
    bool bNoTrading = !ActiveTags.HasTag(FGameplayTag(SandboxExpTags::TAG_Sandbox_Feature_Trading));
    bool bPass = bHasRaid && bHasCombat && bHasLagComp && bNoTrading;
    LogResult(TEXT("Test05_ActionSets_MergeTags"), bPass,
        FString::Printf(TEXT("Raid=%s Combat=%s LagComp=%s NoTrading=%s"),
            bHasRaid ? TEXT("Y") : TEXT("N"),
            bHasCombat ? TEXT("Y") : TEXT("N"),
            bHasLagComp ? TEXT("Y") : TEXT("N"),
            bNoTrading ? TEXT("Y") : TEXT("N")));
}

void ATestExperienceActor::Test06_SameMap_DifferentRules()
{
    // FLOW: Load Raid → check tags → Load Hub on new manager → check different tags
    // WHY: THIS IS THE CORE INSIGHT — same map, different rules, just by changing URL
    USandboxExperienceManager* RaidMgr = CreateManager();
    RaidMgr->RequestExperienceLoad(TEXT("TestMap?Experience=RaidSandbox"));

    USandboxExperienceManager* HubMgr = CreateManager();
    HubMgr->RequestExperienceLoad(TEXT("TestMap?Experience=HubTown"));

    // Raid has combat, Hub has trading
    bool bRaidHasCombat = RaidMgr->GetActiveExperienceTags().HasTag(
        FGameplayTag(SandboxExpTags::TAG_Sandbox_Feature_Combat));
    bool bHubHasTrading = HubMgr->GetActiveExperienceTags().HasTag(
        FGameplayTag(SandboxExpTags::TAG_Sandbox_Feature_Trading));
    bool bRaidNoCombatInHub = !HubMgr->GetActiveExperienceTags().HasTag(
        FGameplayTag(SandboxExpTags::TAG_Sandbox_Feature_Combat));

    // Different pawn class
    bool bDiffPawn = RaidMgr->GetPawnClassName() != HubMgr->GetPawnClassName();

    bool bPass = bRaidHasCombat && bHubHasTrading && bRaidNoCombatInHub && bDiffPawn;
    LogResult(TEXT("Test06_SameMap_DifferentRules"), bPass,
        FString::Printf(TEXT("Raid:Combat=%s Hub:Trading=%s Hub:NoCombat=%s DiffPawn=%s (Raid=%s, Hub=%s)"),
            bRaidHasCombat ? TEXT("Y") : TEXT("N"),
            bHubHasTrading ? TEXT("Y") : TEXT("N"),
            bRaidNoCombatInHub ? TEXT("Y") : TEXT("N"),
            bDiffPawn ? TEXT("Y") : TEXT("N"),
            *RaidMgr->GetPawnClassName(),
            *HubMgr->GetPawnClassName()));
}

void ATestExperienceActor::Test07_OnExperienceLoadedDelegate()
{
    // FLOW: Subscribe to OnExperienceLoaded → load experience → delegate fires
    // WHY: Clients need to know WHEN experience loaded to setup HUD/Input
    // In production: APaldarkGameStateBase::OnExperienceLoaded delegate
    USandboxExperienceManager* Mgr = CreateManager();
    bool bDelegateFired = false;
    const USandboxExperienceDefinition* ReceivedExp = nullptr;

    Mgr->OnExperienceLoaded.AddLambda([&bDelegateFired, &ReceivedExp](const USandboxExperienceDefinition* Exp)
    {
        bDelegateFired = true;
        ReceivedExp = Exp;
    });

    Mgr->RequestExperienceLoad(TEXT("MapName?Experience=HubTown"));
    bool bPass = bDelegateFired && ReceivedExp != nullptr && ReceivedExp->DisplayName == TEXT("Hub Town");
    LogResult(TEXT("Test07_OnExperienceLoadedDelegate"), bPass,
        FString::Printf(TEXT("DelegateFired=%s ExpName='%s'"),
            bDelegateFired ? TEXT("Y") : TEXT("N"),
            ReceivedExp != nullptr ? *ReceivedExp->DisplayName : TEXT("nullptr")));
}

void ATestExperienceActor::Test08_MaxPlayers_DifferentPerExperience()
{
    // FLOW: Raid MaxPlayers=4, Hub MaxPlayers=8
    // WHY: Hub needs more players than Raid. Experience data drives session config
    USandboxExperienceManager* RaidMgr = CreateManager();
    RaidMgr->RequestExperienceLoad(TEXT("MapName?Experience=RaidSandbox"));

    USandboxExperienceManager* HubMgr = CreateManager();
    HubMgr->RequestExperienceLoad(TEXT("MapName?Experience=HubTown"));

    int32 RaidMax = RaidMgr->GetLoadedExperience()->MaxPlayers;
    int32 HubMax = HubMgr->GetLoadedExperience()->MaxPlayers;
    bool bPass = (RaidMax == 4) && (HubMax == 8);
    LogResult(TEXT("Test08_MaxPlayers"), bPass,
        FString::Printf(TEXT("Raid=%d Hub=%d"), RaidMax, HubMax));
}

void ATestExperienceActor::LogResult(const FString& TestName, bool bPassed, const FString& Detail)
{
    if (bPassed)
    {
        UE_LOG(LogSandboxExpTest, Log, TEXT("[PASS] %s — %s"), *TestName, *Detail);
    }
    else
    {
        UE_LOG(LogSandboxExpTest, Error, TEXT("[FAIL] %s — %s"), *TestName, *Detail);
    }
}
