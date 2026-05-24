// Sandbox Lesson 07 — UGameplayEffectExecutionCalculation that implements
// the canonical Paldark damage formula:
//
//   Mitigation = 100 / (100 + Armor)         (diminishing returns)
//   Final      = BaseDamage * Mitigation     (per damage instance)
//
// If the spec is tagged Sandbox.DamageType.True, the mitigation is forced
// to 1.0 — true damage bypasses armor (Lesson asks designer to think about
// what should ignore armor: poison ticks? script-only damage? bosses?).
//
// Output is written to the meta-attribute IncomingDamage (additive) so the
// PR-06 PostGameplayEffectExecute drains it into Health with clamping +
// OnHealthZeroed broadcast. Damage code touches no other attribute directly.
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "TestDamageExecution.generated.h"

UCLASS()
class PALDARK_API UTestDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UTestDamageExecution();

	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
