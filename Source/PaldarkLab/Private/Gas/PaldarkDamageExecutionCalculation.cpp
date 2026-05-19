#include "Gas/PaldarkDamageExecutionCalculation.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"

#include "Gas/PaldarkAttributeSet.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"

// Aura-style capture struct: one declaration per attribute we want to capture
// at execute time. Wrapped in a function-local static so the
// FGameplayEffectAttributeCaptureDefinition is constructed exactly once.
struct PaldarkDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(Armor);

	PaldarkDamageStatics()
	{
		// Target capture — we want the *defender's* armor, snapshotted at
		// execute time (false → not at spec-instantiation time, matching
		// Aura's "real-time" capture).
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPaldarkAttributeSet, Armor, Target, false);
	}
};

static const PaldarkDamageStatics& DamageStatics()
{
	static PaldarkDamageStatics Statics;
	return Statics;
}

UPaldarkDamageExecutionCalculation::UPaldarkDamageExecutionCalculation()
{
	// The engine asks the execution calc which captures it needs ahead of
	// time so the spec can snapshot them. Missing a capture here means
	// AttemptCalculateCapturedAttributeMagnitude returns 0 silently.
	RelevantAttributesToCapture.Add(DamageStatics().ArmorDef);
}

void UPaldarkDamageExecutionCalculation::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	// Aggregator parameters bound from the source / target tag containers so
	// `AttemptCalculateCapturedAttributeMagnitude` can apply tag-conditional
	// modifiers (e.g. armor multipliers gated by Paldark.State.IsDead).
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = SourceTags;
	EvalParams.TargetTags = TargetTags;

	// Read SetByCaller magnitudes. The player Fire ability fills BaseDamage
	// (see GA_HitscanFire.cpp); the Pal attack ability (W18-19) fills
	// PalDamage on its own key so designers can tune Pal damage without
	// touching the weapon damage curve. Both feed the same damage GE so
	// the execution sums them and treats either as the source.
	const float WeaponDamage = Spec.GetSetByCallerMagnitude(
		PaldarkGameplayTags::TAG_Paldark_SetByCaller_BaseDamage,
		/*WarnIfNotFound=*/false,
		/*DefaultIfNotFound=*/0.0f);

	const float PalDamage = Spec.GetSetByCallerMagnitude(
		PaldarkGameplayTags::TAG_Paldark_SetByCaller_PalDamage,
		/*WarnIfNotFound=*/false,
		/*DefaultIfNotFound=*/0.0f);

	const float BaseDamage = WeaponDamage + PalDamage;

	const float HeadshotMultiplier = Spec.GetSetByCallerMagnitude(
		PaldarkGameplayTags::TAG_Paldark_SetByCaller_HeadshotMultiplier,
		/*WarnIfNotFound=*/false,
		/*DefaultIfNotFound=*/1.0f);

	if (BaseDamage <= 0.0f)
	{
		UE_LOG(LogPaldarkGAS, Verbose,
			TEXT("UPaldarkDamageExecutionCalculation — BaseDamage<=0 (weapon=%.2f pal=%.2f), skipping (spec=%s)."),
			WeaponDamage, PalDamage, *Spec.ToSimpleString());
		return;
	}

	// Capture target armor. Floor at 0 so a debuff-driven negative armor can't
	// flip the formula sign.
	float TargetArmor = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		DamageStatics().ArmorDef, EvalParams, TargetArmor);
	TargetArmor = FMath::Max(TargetArmor, 0.0f);

	// Damage formula — base × headshot × armor mitigation. The 100/(100+Armor)
	// curve is the canonical "diminishing returns" shape used by every Aura /
	// Crunch course in the repo. Designers tune armor via GE_InitAttributes;
	// headshot multiplier defaults to 1.0 (no bonus) until the hitscan ability
	// adds it on a head-bone hit.
	const float ArmorMitigation = 100.0f / (100.0f + TargetArmor);
	const float FinalDamage     = BaseDamage * HeadshotMultiplier * ArmorMitigation;

	UE_LOG(LogPaldarkGAS, Log,
		TEXT("UPaldarkDamageExecutionCalculation — weapon=%.2f pal=%.2f base=%.2f headshot=%.2f armor=%.2f mitigation=%.3f final=%.2f"),
		WeaponDamage, PalDamage, BaseDamage, HeadshotMultiplier, TargetArmor, ArmorMitigation, FinalDamage);

	// Output: write the result into IncomingDamage. The AttributeSet's
	// PostGameplayEffectExecute hook drains this onto Health on the same
	// frame and broadcasts OnHealthZeroed when Health crosses zero.
	const FGameplayModifierEvaluatedData EvaluatedData(
		UPaldarkAttributeSet::GetIncomingDamageAttribute(),
		EGameplayModOp::Additive,
		FinalDamage);

	OutExecutionOutput.AddOutputModifier(EvaluatedData);
}
