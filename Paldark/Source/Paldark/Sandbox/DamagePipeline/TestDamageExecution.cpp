#include "TestDamageExecution.h"

#include "SandboxDamageLog.h"
#include "SandboxDamageTags.h"
#include "../AttributeSet/TestPaldarkAttributeSet.h"

#include "AbilitySystemComponent.h"

// ---------------------------------------------------------------------------
// Capture struct — tells GAS which attributes this Execution needs to read.
// For damage, we only need TARGET.Armor (snapshot=false: read fresh at apply).
// ---------------------------------------------------------------------------
struct FSandboxDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(Armor);

	FSandboxDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UTestPaldarkAttributeSet, Armor, Target, false);
	}
};

static const FSandboxDamageStatics& DamageStatics()
{
	static FSandboxDamageStatics S;
	return S;
}

UTestDamageExecution::UTestDamageExecution()
{
	RelevantAttributesToCapture.Add(DamageStatics().ArmorDef);
}

void UTestDamageExecution::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	// Aggregator parameters — pass source/target tags so any tagged modifiers
	// would apply (none here, but this is the canonical wiring).
	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	float Armor = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ArmorDef, EvalParams, Armor);
	Armor = FMath::Max(0.f, Armor);

	// SetByCaller: the caller deposited "how much" here. We read "how applied".
	// fail-safe default 0 keeps the path safe if someone applies the GE without
	// setting damage — better silent zero than uninitialized memory.
	const float BaseDamage = Spec.GetSetByCallerMagnitude(TAG_Sandbox_SetByCaller_Damage, /*WarnIfNotFound*/ false, /*Default*/ 0.f);

	const bool bTrueDamage = Spec.DynamicGrantedTags.HasTagExact(TAG_Sandbox_DamageType_True);

	// 100 / (100 + Armor): diminishing returns. See LESSON.md for the math intuition.
	const float Mitigation = bTrueDamage ? 1.f : (100.f / (100.f + Armor));
	const float Final      = FMath::Max(0.f, BaseDamage * Mitigation);

	UE_LOG(LogSandboxDamage, Display,
		TEXT("Execution: Base=%.2f Armor=%.2f bTrue=%d -> Mitigation=%.4f Final=%.2f"),
		BaseDamage, Armor, bTrueDamage ? 1 : 0, Mitigation, Final);

	// Additive into IncomingDamage — meta-attribute drain happens in Lesson 06
	// PostGameplayEffectExecute (Health -= IncomingDamage, mailbox cleared).
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
		UTestPaldarkAttributeSet::GetIncomingDamageAttribute(),
		EGameplayModOp::Additive,
		Final));
}
