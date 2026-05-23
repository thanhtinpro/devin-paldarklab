// ATestPawnDataInputActor.h
#pragma once
#include "GameFramework/Actor.h"
#include "ATestPawnDataInputActor.generated.h"

class USandboxPawnData;
class USandboxInputConfig;

UCLASS()
class ATestPawnDataInputActor : public AActor
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;

private:
    // Factory helpers
    USandboxInputConfig* CreateRaidInputConfig();
    USandboxInputConfig* CreateHubInputConfig();
    USandboxPawnData* CreateRaidPawnData();
    USandboxPawnData* CreateHubPawnData();

    // Test cases
    void Test01_TagLookup_FindsCorrectAction();
    void Test02_TagLookup_MissingTag_ReturnsEmpty();
    void Test03_PawnData_OnPossess_BindsNativeActions();
    void Test04_PawnData_OnPossess_BindsAbilityActions();
    void Test05_PawnData_MappingContexts_Applied();
    void Test06_DifferentPawnData_DifferentBindings();
    void Test07_SimulateInputPress_NativeAction();
    void Test08_SimulateInputPress_AbilityAction();
    void Test09_SimulateInputRelease_CancelAbility();
    void Test10_NoPawnData_GracefulFail();
    void Test11_NoInputConfig_GracefulFail();

    void LogResult(const FString& TestName, bool bPassed, const FString& Detail);
};
