// Sandbox Lesson 03 — InputBinder simulates what a Character/PC does on
// possession: iterate PawnData.InputConfig and bind each tag to a callback.
// Then we can "simulate" input arrival and verify only the right callback runs.
//
// Auto-runs the full test suite on UWorld::OnWorldBeginPlay so no level setup is needed.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "TestPawnData.h"
#include "TestInputBinder.generated.h"

DECLARE_DELEGATE_OneParam(FOnSandboxInputFired, const UTestInputAction* /*FiredAction*/);

UCLASS()
class PALDARK_API UTestInputBinder : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Wipes any prior bindings and re-binds from PawnData.InputConfig.
	// Mirrors what a Character does in SetupPlayerInputComponent after possession.
	void BindToPawnData(const UTestPawnData* PawnData);

	// Simulate an input event arriving with a known intent tag. The binder
	// looks up its tag->callback map and invokes the matching delegate.
	void SimulateInput(const FGameplayTag& InputTag);

protected:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
	void OnWorldBeginPlay();
	void SeedTwoPawnDataVariants();
	void RunAllTestCases();

	// One callback slot per intent tag. Populated by BindToPawnData.
	TMap<FGameplayTag, FOnSandboxInputFired> TagCallbacks;

	// Per-test scoreboard so assertions can verify "only X fired, not Y".
	TMap<FName, int32> FiredActionCounts;

	void ResetScoreboard() { FiredActionCounts.Reset(); }
	int32 CountFor(FName ActionId) const
	{
		const int32* P = FiredActionCounts.Find(ActionId);
		return P ? *P : 0;
	}

	// Two hand-built PawnData variants sharing "Pawn class" but with
	// different InputConfigs — proves data-driven retarget.
	UPROPERTY() UTestPawnData* PawnData_KBM     = nullptr;
	UPROPERTY() UTestPawnData* PawnData_Gamepad = nullptr;

	FDelegateHandle WorldBeginPlayHandle;
};
