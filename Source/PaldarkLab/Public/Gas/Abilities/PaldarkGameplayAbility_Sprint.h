// PALDARK W7-8 — Sprint ability (player stamina drain on hold).
//
// Outcome guarded by `Documents/PALDARK/03-Roadmap_1_Year.md` § Tuần 7-8:
//   "Player sprint = stamina tiêu hao via GA."
//
// Behaviour:
//   - ActivationInputTag = `Paldark.InputTag.Sprint` so the Enhanced Input →
//     ability bridge can activate / cancel it via the same tag table the
//     native (Move/Look/Jump) handlers already use.
//   - On Activate: apply two GEs (designer-authored, configured via the
//     `SprintCostEffect` + `SprintMoveSpeedEffect` properties) — a periodic
//     stamina-cost GE and an additive MoveSpeed buff. Tag the owner with
//     `Paldark.State.Sprinting` so other systems (HUD, AnimBP) can read it.
//   - On Input released / stamina hit 0: cancel the ability so EndAbility
//     fires; EndAbility removes both GEs (by handle) and the Sprinting tag.
//   - Cooldown deferred to W9-10 — for W7-8 the stamina drain is the
//     self-balancing limiter.
//
// Future homes (do NOT implement here in W7-8):
//   - Pal sprint variant — W18+ Pal Combat Activity.
//   - Dynamic sprint speed scaling vs encumbrance — W11-12 inventory weight.

#pragma once

#include "ActiveGameplayEffectHandle.h"
#include "CoreMinimal.h"
#include "Gas/PaldarkGameplayAbility.h"
#include "PaldarkGameplayAbility_Sprint.generated.h"

class UGameplayEffect;
struct FGameplayEventData;

UCLASS()
class PALDARKLAB_API UPaldarkGameplayAbility_Sprint : public UPaldarkGameplayAbility
{
	GENERATED_BODY()

public:
	UPaldarkGameplayAbility_Sprint();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	// Designer-authored periodic GE that drains Stamina (e.g. -10/sec). When
	// null, the ability still applies the move-speed buff but does NOT drain
	// stamina — useful for debug Pal sprint without survival pressure.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Ability|Sprint")
	TSubclassOf<UGameplayEffect> SprintCostEffect;

	// Designer-authored infinite GE that adds to MoveSpeed (e.g. +300 cm/s).
	// Applied on Activate, removed (by handle) on EndAbility.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Ability|Sprint")
	TSubclassOf<UGameplayEffect> SprintMoveSpeedEffect;

	// Owner tag added on Activate, removed on EndAbility. HUD / AnimBP can
	// read this via the standard `HasMatchingGameplayTag` API.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Ability|Sprint")
	FGameplayTag SprintingStateTag;

protected:
	// Handles cached so EndAbility can remove the active effects without
	// scanning the ASC again.
	FActiveGameplayEffectHandle CostEffectHandle;
	FActiveGameplayEffectHandle MoveSpeedEffectHandle;
};
