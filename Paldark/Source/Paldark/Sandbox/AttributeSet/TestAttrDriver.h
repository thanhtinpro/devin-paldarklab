// Sandbox Lesson 06 — driver: spawns ATestAttrPawn, applies a series of test
// GameplayEffects to probe PreAttributeChange clamping + PostExecute meta
// translation + OnHealthZeroed broadcast.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayEffectTypes.h"
#include "TestAttrDriver.generated.h"

class ATestAttrPawn;
class UGameplayEffect;
struct FGameplayAttribute;

UCLASS()
class PALDARK_API UTestAttrDriver : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

protected:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
	void OnWorldBeginPlay();
	void RunAllTestCases();

	// Helper: build a transient instant GE that overrides `Attr` with `Value`
	// and apply it to the pawn's ASC. Returns true on success.
	bool ApplyInstantOverride(const FGameplayAttribute& Attr, float Value);

	UPROPERTY()
	ATestAttrPawn* Pawn = nullptr;

	int32 HealthZeroedCallCount = 0;
	FDelegateHandle WorldBeginPlayHandle;
};
