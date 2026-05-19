// PALDARK W29-30 — Foxparks (companion) fire-breath cone ability.
//
// Cone AoE — Foxparks is a player-side companion that funnels damage into
// a forward cone. The friendly-fire problem is solved by filtering out
// any actor whose `TeamTag` matches the Pal's own `TeamTag` (so player +
// other player-team companions are skipped).
//
// W29-30 scaffold scope:
//   - Override `ActivateAbility`: instead of inheriting the parent's
//     single-target damage, query overlapping pawns within
//     `ConeRangeCM` + `ConeHalfAngleDeg`, and apply the per-species
//     damage GE to each hostile target found.
//   - Re-uses the per-species damage GE wired on the parent
//     (`DamageEffectClass`).
//
// What this is NOT:
//   - No DoT / burn GE. Designer can author a separate GE_Burn and
//     apply it inside a BP_GA override of ActivateAbility.
//   - No particle / VFX trigger. Designer authors via montage.
//   - No falloff over distance. Every target inside the cone takes the
//     same damage value (combat component's BasePalDamage).

#pragma once

#include "CoreMinimal.h"
#include "Gas/Abilities/PaldarkGameplayAbility_PalAttack.h"
#include "PaldarkGameplayAbility_PalAttack_FireBreath.generated.h"

UCLASS()
class PALDARKLAB_API UPaldarkGameplayAbility_PalAttack_FireBreath : public UPaldarkGameplayAbility_PalAttack
{
	GENERATED_BODY()

public:
	UPaldarkGameplayAbility_PalAttack_FireBreath();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// Forward reach of the cone (cm). 800 is a comfortable mid-range AoE.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Ability|PalAttack|FireBreath", meta = (ClampMin = "100.0"))
	float ConeRangeCM = 800.f;

	// Half-angle of the cone (deg). 30 = 60-degree total spread.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Ability|PalAttack|FireBreath", meta = (ClampMin = "1.0", ClampMax = "89.0"))
	float ConeHalfAngleDeg = 30.f;

	// Hard cap to prevent pathological "everyone gets damaged" scenarios
	// during W29-30 testing. Designer can bump per-BP if needed.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Ability|PalAttack|FireBreath", meta = (ClampMin = "1"))
	int32 MaxConeTargets = 6;
};
