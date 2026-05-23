#include "Gas/Abilities/PaldarkGameplayAbility_Sprint.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"

#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"

UPaldarkGameplayAbility_Sprint::UPaldarkGameplayAbility_Sprint()
{
	// Tag-keyed input lookup — Enhanced Input binds this same tag in the
	// character (see APaldarkCharacter::BindAbilityInputActions).
	ActivationInputTag = PaldarkGameplayTags::TAG_Paldark_InputTag_Sprint;
	DebugName          = TEXT("Sprint");

	// Tag the ability so cancel-by-tag works. Granted abilities receive these
	// tags into their AbilityTags container by default.
	AbilityTags.AddTag(PaldarkGameplayTags::TAG_Paldark_Ability_Sprint);

	// State tag applied to the owner while sprinting (HUD / AnimBP read it).
	SprintingStateTag = PaldarkGameplayTags::TAG_Paldark_State_Sprinting;

	// Don't auto-cancel us on input release — we cancel via the explicit
	// CancelAbilityByActivationTag path from the character so the cancel
	// fires symmetrically on server + autonomous client.
}

void UPaldarkGameplayAbility_Sprint::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo != nullptr ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (ASC == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	// Apply periodic stamina-cost GE (designer-authored; null in tests is OK).
	if (SprintCostEffect != nullptr)
	{
		const FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(SprintCostEffect, GetAbilityLevel());
		if (Spec.IsValid())
		{
			CostEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}

	// Apply infinite MoveSpeed buff (additive in the designer-authored GE).
	if (SprintMoveSpeedEffect != nullptr)
	{
		const FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(SprintMoveSpeedEffect, GetAbilityLevel());
		if (Spec.IsValid())
		{
			MoveSpeedEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}

	if (SprintingStateTag.IsValid())
	{
		ASC->AddLooseGameplayTag(SprintingStateTag);
	}

	UE_LOG(LogPaldarkGAS, Log,
		TEXT("UPaldarkGameplayAbility_Sprint::ActivateAbility — owner=%s cost=%s movespeed=%s"),
		*GetNameSafe(ActorInfo->OwnerActor.Get()),
		*GetNameSafe(SprintCostEffect),
		*GetNameSafe(SprintMoveSpeedEffect));
}

void UPaldarkGameplayAbility_Sprint::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	UAbilitySystemComponent* ASC = ActorInfo != nullptr ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (ASC != nullptr)
	{
		if (CostEffectHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(CostEffectHandle);
		}
		if (MoveSpeedEffectHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(MoveSpeedEffectHandle);
		}
		if (SprintingStateTag.IsValid())
		{
			ASC->RemoveLooseGameplayTag(SprintingStateTag);
		}
	}

	CostEffectHandle      = FActiveGameplayEffectHandle();
	MoveSpeedEffectHandle = FActiveGameplayEffectHandle();

	UE_LOG(LogPaldarkGAS, Log,
		TEXT("UPaldarkGameplayAbility_Sprint::EndAbility — owner=%s cancelled=%d"),
		ActorInfo != nullptr ? *GetNameSafe(ActorInfo->OwnerActor.Get()) : TEXT("<null>"),
		bWasCancelled ? 1 : 0);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
