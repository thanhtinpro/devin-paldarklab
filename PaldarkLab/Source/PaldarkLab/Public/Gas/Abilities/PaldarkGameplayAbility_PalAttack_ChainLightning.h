// PALDARK W29-30 — Boltmane (boss) chain-lightning ability.
//
// Multi-target jump attack. The lightning starts at the locked combat
// target and bounces up to `MaxBounces` times to the nearest unbounced
// hostile within `BounceRangeCM` of the previous link. Each bounce deals
// damage scaled by `BounceFalloff` (0.7 = each bounce 70% of the prior).
//
// W29-30 scaffold scope:
//   - Override `ActivateAbility`: walk a bounce chain, apply the per-species
//     damage GE on each link with diminishing magnitude.
//   - Re-uses the per-species damage GE wired on the parent.
//
// What this is NOT:
//   - No paragraph-long VFX. The lightning visual is designer-authored.
//   - No stun GE. Boss multi-phase scaffold (`UPaldarkActivity_BossPhase`)
//     gates the activation rate, not this ability.
//   - No friendly-fire safety — Boltmane is hostile, every hit pawn is
//     intended damage. (For a friendly chain-lightning later, copy this
//     ability into a Foxparks variant and filter by team tag.)

#pragma once

#include "CoreMinimal.h"
#include "Gas/Abilities/PaldarkGameplayAbility_PalAttack.h"
#include "PaldarkGameplayAbility_PalAttack_ChainLightning.generated.h"

UCLASS()
class PALDARKLAB_API UPaldarkGameplayAbility_PalAttack_ChainLightning : public UPaldarkGameplayAbility_PalAttack
{
	GENERATED_BODY()

public:
	UPaldarkGameplayAbility_PalAttack_ChainLightning();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// Maximum number of additional jumps after the initial target hit.
	// 2 keeps the visual + balance manageable in a 4-player raid.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Ability|PalAttack|ChainLightning", meta = (ClampMin = "0", ClampMax = "8"))
	int32 MaxBounces = 2;

	// Maximum bounce search radius (cm) from the previous chain link.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Ability|PalAttack|ChainLightning", meta = (ClampMin = "100.0"))
	float BounceRangeCM = 600.f;

	// Damage multiplier between successive bounces. 0.7 = 100% / 70% / 49%.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Ability|PalAttack|ChainLightning", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float BounceFalloff = 0.7f;
};
