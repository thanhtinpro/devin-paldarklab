// PALDARK W18-19 — Pal attack ability (AI-driven, server-only).
//
// The Pal's first combat ability. Granted to the Pal's ASC by
// `UPaldarkPalCombatComponent::BeginPlay` and triggered by
// `UPaldarkActivity_Combat` while the Pal is in the Combat state.
//
// Differences from W9-10 `UPaldarkGameplayAbility_HitscanFire`:
//   - No input tag (Pal abilities are AI-triggered, the combat component
//     calls `TryActivateAbility(SpecHandle)` directly).
//   - No line trace. The target pointer comes from the combat component's
//     `GetCurrentTarget` (perception locked it in). Skipping the trace is
//     intentional: Pal melee/instant abilities don't need aim, and the
//     ranged Pal abilities scheduled for W30+ will subclass and override.
//   - SetByCaller uses `Paldark.SetByCaller.PalDamage` so designers can
//     tune Pal damage independently of weapon damage.
//
// Authority model:
//   - `NetExecutionPolicy = ServerOnly` — the W18-19 sandbox doesn't need
//     client prediction for Pal abilities (clients see the dummy's Health
//     replicate down). Predicted Pal attacks ship W22+ once animation
//     montages land.
//
// Future homes (do NOT implement here in W18-19):
//   - Animation montage trigger — W22+.
//   - Per-species cooldown override (some Pals fire faster) — W18+ via
//     `UPaldarkPalCombatComponent::AttackInterval`.
//   - Multi-hit melee swing (sweep trace) — W30+ when Pal species diverge.

#pragma once

#include "CoreMinimal.h"
#include "Gas/PaldarkGameplayAbility.h"
#include "PaldarkGameplayAbility_PalAttack.generated.h"

class UGameplayEffect;

UCLASS()
class PALDARKLAB_API UPaldarkGameplayAbility_PalAttack : public UPaldarkGameplayAbility
{
	GENERATED_BODY()

public:
	UPaldarkGameplayAbility_PalAttack();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// Designer-authored damage GE (Blueprint subclass of UGameplayEffect with
	// the UPaldarkDamageExecutionCalculation execution attached). When null
	// the ability still runs (logs a warning) so designers can validate the
	// activation path before authoring the GE.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Ability|PalAttack")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// Fallback base damage when the source `UPaldarkPalCombatComponent` is
	// unreachable. Normally the combat component's `BasePalDamage` wins.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Ability|PalAttack", meta = (ClampMin = "0.0"))
	float FallbackBaseDamage = 10.f;
};
