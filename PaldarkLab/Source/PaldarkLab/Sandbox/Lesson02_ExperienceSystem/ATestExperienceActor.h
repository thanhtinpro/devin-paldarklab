// ATestExperienceActor.h
#pragma once
#include "GameFramework/Actor.h"
#include "ATestExperienceActor.generated.h"

class USandboxExperienceManager;
class USandboxExperienceDefinition;

UCLASS()
class ATestExperienceActor : public AActor
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;

private:
    // Helpers
    USandboxExperienceDefinition* CreateRaidExperience();
    USandboxExperienceDefinition* CreateHubExperience();
    USandboxExperienceManager* CreateManager();

    void Test01_URLOption_SelectsRaid();
    void Test02_NoURLOption_FallbackToDefault();
    void Test03_InvalidExperience_GracefulFallback();
    void Test04_ClassOverrides_PCandPS();
    void Test05_ActionSets_MergeTags();
    void Test06_SameMap_DifferentRules();
    void Test07_OnExperienceLoadedDelegate();
    void Test08_MaxPlayers_DifferentPerExperience();

    void LogResult(const FString& TestName, bool bPassed, const FString& Detail);
};
