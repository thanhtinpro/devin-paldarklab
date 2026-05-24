// Sandbox Lesson 07 — driver: reuse ATestAttrPawn (Lesson 06) and feed it a
// matrix of damage scenarios (varying BaseDamage / Armor / DamageType) via
// the UTestDamageExecution pipeline. Validates the formula end-to-end.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TestDamageDriver.generated.h"

class ATestAttrPawn;

UCLASS()
class PALDARK_API UTestDamageDriver : public UWorldSubsystem
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

	// Set Armor directly via Override (uses the Lesson 06 clamp path).
	void SetArmor(float Value);
	// Restore Health to 100 between scenarios via direct Override.
	void RestoreHealth();
	// Apply a damage GE with SetByCaller magnitude + optional true-damage tag.
	void ApplyDamage(float BaseDamage, bool bTrueDamage);

	UPROPERTY()
	ATestAttrPawn* Pawn = nullptr;

	FDelegateHandle WorldBeginPlayHandle;
};
