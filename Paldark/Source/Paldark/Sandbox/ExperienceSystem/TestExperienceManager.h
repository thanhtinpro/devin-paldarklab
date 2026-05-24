// Sandbox Lesson 02 — Experience Manager.
//
// Responsibilities (mirrors LyraExperienceManagerComponent):
//   1. Hold a small registry of ExperienceDefinitions (sandbox: seeded in code).
//   2. Parse "?Experience=Name" from a URL string.
//   3. Resolve URL value -> definition; graceful fallback to default if invalid.
//   4. "Async load" the definition (sandbox: SetTimer next-tick), then apply.
//   5. Broadcast OnExperienceLoaded so listeners (PC, HUD, ...) can react.
//
// Auto-runs test scenarios on UWorld::OnWorldBeginPlay so the lesson works
// without dropping any actor into a level.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TestExperienceDefinition.h"
#include "TestExperienceManager.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSandboxExperienceLoaded, const UTestExperienceDefinition* /*LoadedDefinition*/);

UCLASS()
class PALDARK_API UTestExperienceManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Sync path: parse URL, look up registry, return definition (default fallback if invalid).
	const UTestExperienceDefinition* ResolveExperience(const FString& URLOptions) const;

	// Async path: schedule a next-tick apply + delegate broadcast. Returns immediately.
	void LoadAndApplyExperience(const FString& URLOptions);

	const UTestExperienceDefinition* GetCurrentExperience() const { return CurrentExperience; }

	FOnSandboxExperienceLoaded OnExperienceLoaded;

protected:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
	void SeedRegistry();
	void OnWorldBeginPlay();
	void RunAllTestCases();
	void ApplyExperience(const UTestExperienceDefinition* Def);

	UPROPERTY()
	TMap<FName, UTestExperienceDefinition*> ExperienceRegistry;

	UPROPERTY()
	const UTestExperienceDefinition* CurrentExperience = nullptr;

	// Name used when URL omits the option or names something unregistered.
	FName DefaultExperienceName;

	FDelegateHandle WorldBeginPlayHandle;
};
