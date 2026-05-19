// PALDARK W29-30 — Stoneclad (hostile tank) heavy charge ability.
//
// Melee + knockback. Stoneclad is the W29-30 tank — slow approach, big
// per-hit damage, and a knockback impulse so players are pushed off the
// arena's edge or into other hostiles. The damage GE itself is authored
// by the designer; this subclass adds the knockback-impulse-on-hit
// physics nudge on top of the inherited single-target damage path.
//
// W29-30 scaffold scope:
//   - Override `ActivateAbility` to chain the parent damage logic and then
//     impulse the target's CharacterMovement. The impulse direction is
//     `(Target - Avatar).GetSafeNormal2D()` scaled by `KnockbackImpulse`.
//     A small upward component lifts the target slightly so the impulse
//     reads visually.
//   - No new GE — re-uses the per-species damage GE wired on the parent.
//
// What this is NOT:
//   - No charge wind-up animation (designer authors the montage).
//   - No charge dash trajectory — Stoneclad's `UPaldarkActivity_Stalk`
//     handles approach; this ability fires only once the target is in
//     MaxEngageRange. Charge "feel" comes from the post-hit impulse.
//   - No damage falloff over distance — single-target on-hit.

#pragma once

#include "CoreMinimal.h"
#include "Gas/Abilities/PaldarkGameplayAbility_PalAttack.h"
#include "PaldarkGameplayAbility_PalAttack_Charge.generated.h"

UCLASS()
class PALDARKLAB_API UPaldarkGameplayAbility_PalAttack_Charge : public UPaldarkGameplayAbility_PalAttack
{
	GENERATED_BODY()

public:
	UPaldarkGameplayAbility_PalAttack_Charge();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// Horizontal impulse magnitude (cm/s^2 * mass roughly) applied to the
	// target's CharacterMovement on hit. 1200 reads as a clear "shoved
	// back" without launching the player off the map.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Ability|PalAttack|Charge", meta = (ClampMin = "0.0"))
	float KnockbackImpulse = 1200.f;

	// Small vertical kicker so the impulse reads (without lift, the
	// CharacterMovement friction kills the horizontal slide too fast).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Ability|PalAttack|Charge", meta = (ClampMin = "0.0"))
	float VerticalLiftRatio = 0.25f;
};
