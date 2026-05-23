#include "Gas/PaldarkAbilitySystemComponent.h"

#include "AbilitySystemGlobals.h"
#include "GameplayAbilitySpec.h"
#include "GameplayTagContainer.h"

#include "PaldarkLogCategories.h"

UPaldarkAbilitySystemComponent::UPaldarkAbilitySystemComponent()
{
	// W7-8 default: replicate from server -> autonomous + simulated. The owner
	// PlayerState / Pal pawn calls SetIsReplicated(true) + SetReplicationMode
	// at construction time too, so this constructor stays minimal.
}

int32 UPaldarkAbilitySystemComponent::TryActivateAbilityByActivationTag(const FGameplayTag& InActivationTag)
{
	if (!InActivationTag.IsValid())
	{
		UE_LOG(LogPaldarkGAS, Warning, TEXT("UPaldarkAbilitySystemComponent::TryActivateAbilityByActivationTag — invalid tag, ignored."));
		return 0;
	}

	FGameplayTagContainer ActivationTags;
	ActivationTags.AddTag(InActivationTag);

	const bool bSuccess = TryActivateAbilitiesByTag(ActivationTags);
	const int32 Activated = bSuccess ? 1 : 0;

	UE_LOG(LogPaldarkGAS, Verbose,
		TEXT("UPaldarkAbilitySystemComponent::TryActivateAbilityByActivationTag — tag=%s activated=%d"),
		*InActivationTag.ToString(),
		Activated);
	return Activated;
}

void UPaldarkAbilitySystemComponent::CancelAbilityByActivationTag(const FGameplayTag& InActivationTag)
{
	if (!InActivationTag.IsValid())
	{
		return;
	}

	FGameplayTagContainer Cancel;
	Cancel.AddTag(InActivationTag);

	CancelAbilities(&Cancel);
	UE_LOG(LogPaldarkGAS, Verbose,
		TEXT("UPaldarkAbilitySystemComponent::CancelAbilityByActivationTag — tag=%s"),
		*InActivationTag.ToString());
}
