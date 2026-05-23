// ATestPawnDataInputActor.cpp
#include "ATestPawnDataInputActor.h"
#include "USandboxPawnData.h"
#include "USandboxInputConfig.h"
#include "USandboxCharacterSim.h"
#include "GameplayTagContainer.h"

// Register sandbox tags needed for this lesson
#include "NativeGameplayTags.h"
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SB_Move, "Sandbox.InputTag.Move");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SB_Look, "Sandbox.InputTag.Look");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SB_Jump, "Sandbox.InputTag.Jump");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SB_Sprint, "Sandbox.InputTag.Sprint");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SB_Fire, "Sandbox.InputTag.Fire");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SB_Interact, "Sandbox.InputTag.Interact");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SB_Trade, "Sandbox.InputTag.Trade");

DEFINE_LOG_CATEGORY_STATIC(LogSandboxPDTest, Log, All);

void ATestPawnDataInputActor::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogSandboxPDTest, Log, TEXT("=== LESSON 03: PawnData + Input Configuration ==="));

    Test01_TagLookup_FindsCorrectAction();
    Test02_TagLookup_MissingTag_ReturnsEmpty();
    Test03_PawnData_OnPossess_BindsNativeActions();
    Test04_PawnData_OnPossess_BindsAbilityActions();
    Test05_PawnData_MappingContexts_Applied();
    Test06_DifferentPawnData_DifferentBindings();
    Test07_SimulateInputPress_NativeAction();
    Test08_SimulateInputPress_AbilityAction();
    Test09_SimulateInputRelease_CancelAbility();
    Test10_NoPawnData_GracefulFail();
    Test11_NoInputConfig_GracefulFail();

    UE_LOG(LogSandboxPDTest, Log, TEXT("=== END LESSON 03 ==="));
}

USandboxInputConfig* ATestPawnDataInputActor::CreateRaidInputConfig()
{
    USandboxInputConfig* Config = NewObject<USandboxInputConfig>(this);
    // Native actions — mirrors PaldarkInputConfig's NativeInputActions
    Config->NativeInputActions.Add({ TEXT("IA_Move"), TAG_SB_Move });
    Config->NativeInputActions.Add({ TEXT("IA_Look"), TAG_SB_Look });
    Config->NativeInputActions.Add({ TEXT("IA_Jump"), TAG_SB_Jump });
    // Ability actions — mirrors AbilityInputActions
    Config->AbilityInputActions.Add({ TEXT("IA_Sprint"), TAG_SB_Sprint });
    Config->AbilityInputActions.Add({ TEXT("IA_Fire"), TAG_SB_Fire });
    return Config;
}

USandboxInputConfig* ATestPawnDataInputActor::CreateHubInputConfig()
{
    USandboxInputConfig* Config = NewObject<USandboxInputConfig>(this);
    // Hub has Move/Look but NO Jump, NO Fire — just trading + interact
    Config->NativeInputActions.Add({ TEXT("IA_Move"), TAG_SB_Move });
    Config->NativeInputActions.Add({ TEXT("IA_Look"), TAG_SB_Look });
    // No Jump! Hub town doesn't need jumping
    // Ability actions — trading, not combat
    Config->AbilityInputActions.Add({ TEXT("IA_Interact"), TAG_SB_Interact });
    Config->AbilityInputActions.Add({ TEXT("IA_Trade"), TAG_SB_Trade });
    // No Sprint, no Fire in hub
    return Config;
}

USandboxPawnData* ATestPawnDataInputActor::CreateRaidPawnData()
{
    USandboxPawnData* PD = NewObject<USandboxPawnData>(this);
    PD->PawnClassName = TEXT("PaldarkCharacter");
    PD->InputConfig = CreateRaidInputConfig();
    PD->MappingContexts.Add({ TEXT("IMC_OnFoot"), 0 });
    PD->MappingContexts.Add({ TEXT("IMC_Combat"), 1 });
    PD->GrantedAbilityNames = { TEXT("GA_Sprint"), TEXT("GA_HitscanFire") };
    return PD;
}

USandboxPawnData* ATestPawnDataInputActor::CreateHubPawnData()
{
    USandboxPawnData* PD = NewObject<USandboxPawnData>(this);
    PD->PawnClassName = TEXT("HubCharacter");
    PD->InputConfig = CreateHubInputConfig();
    PD->MappingContexts.Add({ TEXT("IMC_HubWalk"), 0 });
    PD->MappingContexts.Add({ TEXT("IMC_Trading"), 2 });
    PD->GrantedAbilityNames = { TEXT("GA_Trade"), TEXT("GA_Interact") };
    return PD;
}

void ATestPawnDataInputActor::Test01_TagLookup_FindsCorrectAction()
{
    // FLOW: InputConfig has (TAG_SB_Move, "IA_Move") → FindNativeActionForTag returns "IA_Move"
    // WHY: C++ asks "what action is bound to Move?" — data answers, not hardcode
    USandboxInputConfig* Config = CreateRaidInputConfig();
    FString Action = Config->FindNativeActionForTag(TAG_SB_Move);
    bool bPass = (Action == TEXT("IA_Move"));
    LogResult(TEXT("Test01_TagLookup_FindsCorrectAction"), bPass,
        FString::Printf(TEXT("FindNativeActionForTag(Move) = '%s'"), *Action));
}

void ATestPawnDataInputActor::Test02_TagLookup_MissingTag_ReturnsEmpty()
{
    // FLOW: Raid InputConfig has no "Sandbox.InputTag.Trade" → returns empty
    // WHY: Optional probing — character gracefully skips missing bindings
    USandboxInputConfig* Config = CreateRaidInputConfig();
    FString Action = Config->FindNativeActionForTag(TAG_SB_Trade, false);
    bool bPass = Action.IsEmpty();
    LogResult(TEXT("Test02_TagLookup_MissingTag"), bPass,
        FString::Printf(TEXT("FindNativeActionForTag(Trade) = '%s' (expected empty)"), *Action));
}

void ATestPawnDataInputActor::Test03_PawnData_OnPossess_BindsNativeActions()
{
    // FLOW: OnPossess → SetPawnData → BindNativeInputActions → 3 native bindings (Move/Look/Jump)
    // WHY: PawnData drives what inputs the character responds to
    USandboxCharacterSim* Char = NewObject<USandboxCharacterSim>(this);
    Char->SimulateOnPossess(CreateRaidPawnData());
    bool bPass = (Char->GetNativeBindCount() == 3);
    LogResult(TEXT("Test03_BindsNativeActions"), bPass,
        FString::Printf(TEXT("NativeBindCount=%d (expected 3)"), Char->GetNativeBindCount()));
}

void ATestPawnDataInputActor::Test04_PawnData_OnPossess_BindsAbilityActions()
{
    // FLOW: OnPossess also binds ability actions (Sprint/Fire = 2)
    USandboxCharacterSim* Char = NewObject<USandboxCharacterSim>(this);
    Char->SimulateOnPossess(CreateRaidPawnData());
    bool bPass = (Char->GetAbilityBindCount() == 2);
    LogResult(TEXT("Test04_BindsAbilityActions"), bPass,
        FString::Printf(TEXT("AbilityBindCount=%d (expected 2)"), Char->GetAbilityBindCount()));
}

void ATestPawnDataInputActor::Test05_PawnData_MappingContexts_Applied()
{
    // FLOW: Raid PawnData has 2 IMCs (OnFoot + Combat) → both applied
    // WHY: MappingContexts are additive — base locomotion + combat overlay
    USandboxCharacterSim* Char = NewObject<USandboxCharacterSim>(this);
    Char->SimulateOnPossess(CreateRaidPawnData());
    TArray<FString> MCs = Char->GetActiveMappingContexts();
    bool bPass = (MCs.Num() == 2) && MCs.Contains(TEXT("IMC_OnFoot")) && MCs.Contains(TEXT("IMC_Combat"));
    LogResult(TEXT("Test05_MappingContexts"), bPass,
        FString::Printf(TEXT("MCs=%d contains OnFoot=%s Combat=%s"),
            MCs.Num(),
            MCs.Contains(TEXT("IMC_OnFoot")) ? TEXT("Y") : TEXT("N"),
            MCs.Contains(TEXT("IMC_Combat")) ? TEXT("Y") : TEXT("N")));
}

void ATestPawnDataInputActor::Test06_DifferentPawnData_DifferentBindings()
{
    // FLOW: Raid has Jump+Sprint+Fire. Hub has NO Jump, NO Sprint, NO Fire — has Trade+Interact instead.
    // WHY: THIS IS THE CORE INSIGHT — swap PawnData = swap entire input scheme, zero C++ changes
    USandboxCharacterSim* RaidChar = NewObject<USandboxCharacterSim>(this);
    RaidChar->SimulateOnPossess(CreateRaidPawnData());

    USandboxCharacterSim* HubChar = NewObject<USandboxCharacterSim>(this);
    HubChar->SimulateOnPossess(CreateHubPawnData());

    // Raid has Jump, Hub doesn't
    bool bRaidHasJump = !RaidChar->GetBoundHandlerForTag(TAG_SB_Jump).IsEmpty();
    bool bHubNoJump = HubChar->GetBoundHandlerForTag(TAG_SB_Jump).IsEmpty();

    // Raid has Fire, Hub doesn't
    bool bRaidHasFire = !RaidChar->GetBoundHandlerForTag(TAG_SB_Fire).IsEmpty();
    bool bHubNoFire = HubChar->GetBoundHandlerForTag(TAG_SB_Fire).IsEmpty();

    // Hub has Trade, Raid doesn't
    bool bHubHasTrade = !HubChar->GetBoundHandlerForTag(TAG_SB_Trade).IsEmpty();
    bool bRaidNoTrade = RaidChar->GetBoundHandlerForTag(TAG_SB_Trade).IsEmpty();

    // Different pawn class
    bool bDiffPawn = RaidChar->GetPawnData()->PawnClassName != HubChar->GetPawnData()->PawnClassName;

    bool bPass = bRaidHasJump && bHubNoJump && bRaidHasFire && bHubNoFire &&
                 bHubHasTrade && bRaidNoTrade && bDiffPawn;
    LogResult(TEXT("Test06_DifferentPawnData_DifferentBindings"), bPass,
        FString::Printf(TEXT("Raid:Jump=%s Hub:Jump=%s Raid:Fire=%s Hub:Fire=%s Hub:Trade=%s Raid:Trade=%s Pawn:Raid=%s Hub=%s"),
            bRaidHasJump ? TEXT("Y") : TEXT("N"),
            bHubNoJump ? TEXT("N") : TEXT("Y"),
            bRaidHasFire ? TEXT("Y") : TEXT("N"),
            bHubNoFire ? TEXT("N") : TEXT("Y"),
            bHubHasTrade ? TEXT("Y") : TEXT("N"),
            bRaidNoTrade ? TEXT("N") : TEXT("Y"),
            *RaidChar->GetPawnData()->PawnClassName,
            *HubChar->GetPawnData()->PawnClassName));
}

void ATestPawnDataInputActor::Test07_SimulateInputPress_NativeAction()
{
    // FLOW: Press Move tag → dispatches to Input_Move handler (native path)
    USandboxCharacterSim* Char = NewObject<USandboxCharacterSim>(this);
    Char->SimulateOnPossess(CreateRaidPawnData());
    FString Result = Char->SimulateInputPress(TAG_SB_Move);
    bool bPass = Result.Contains(TEXT("NATIVE")) && Result.Contains(TEXT("Input_Move"));
    LogResult(TEXT("Test07_SimInputPress_Native"), bPass, Result);
}

void ATestPawnDataInputActor::Test08_SimulateInputPress_AbilityAction()
{
    // FLOW: Press Sprint tag → routes through ASC->TryActivateAbilityByTag (ability path)
    // WHY: Same input system, two paths — native (Move/Look/Jump) vs ability (Sprint/Fire)
    USandboxCharacterSim* Char = NewObject<USandboxCharacterSim>(this);
    Char->SimulateOnPossess(CreateRaidPawnData());
    FString Result = Char->SimulateInputPress(TAG_SB_Sprint);
    bool bPass = Result.Contains(TEXT("ABILITY")) && Result.Contains(TEXT("TryActivateAbilityByTag"));
    LogResult(TEXT("Test08_SimInputPress_Ability"), bPass, Result);
}

void ATestPawnDataInputActor::Test09_SimulateInputRelease_CancelAbility()
{
    // FLOW: Release Sprint → ASC->CancelAbilityByTag
    // WHY: Hold-to-activate pattern needs symmetric cancel on release
    USandboxCharacterSim* Char = NewObject<USandboxCharacterSim>(this);
    Char->SimulateOnPossess(CreateRaidPawnData());
    FString Result = Char->SimulateInputRelease(TAG_SB_Sprint);
    bool bPass = Result.Contains(TEXT("CancelAbilityByTag"));
    LogResult(TEXT("Test09_SimInputRelease_Cancel"), bPass, Result);
}

void ATestPawnDataInputActor::Test10_NoPawnData_GracefulFail()
{
    // FLOW: OnPossess with nullptr PawnData → warning log, no crash
    USandboxCharacterSim* Char = NewObject<USandboxCharacterSim>(this);
    Char->SimulateOnPossess(nullptr);
    bool bPass = (Char->GetNativeBindCount() == 0) && (Char->GetPawnData() == nullptr);
    LogResult(TEXT("Test10_NoPawnData"), bPass, TEXT("No crash, 0 bindings"));
}

void ATestPawnDataInputActor::Test11_NoInputConfig_GracefulFail()
{
    // FLOW: PawnData exists but InputConfig is nullptr → warning, no bindings, no crash
    USandboxPawnData* PD = NewObject<USandboxPawnData>(this);
    PD->PawnClassName = TEXT("TestChar");
    PD->InputConfig = nullptr; // no config!
    USandboxCharacterSim* Char = NewObject<USandboxCharacterSim>(this);
    Char->SimulateOnPossess(PD);
    bool bPass = (Char->GetNativeBindCount() == 0);
    LogResult(TEXT("Test11_NoInputConfig"), bPass, TEXT("No crash, 0 bindings (check Warning log)"));
}

void ATestPawnDataInputActor::LogResult(const FString& TestName, bool bPassed, const FString& Detail)
{
    if (bPassed)
        UE_LOG(LogSandboxPDTest, Log, TEXT("[PASS] %s — %s"), *TestName, *Detail);
    else
        UE_LOG(LogSandboxPDTest, Error, TEXT("[FAIL] %s — %s"), *TestName, *Detail);
}
