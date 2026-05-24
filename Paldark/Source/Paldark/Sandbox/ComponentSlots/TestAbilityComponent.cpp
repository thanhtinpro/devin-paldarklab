#include "TestAbilityComponent.h"

#include "SandboxShellLog.h"

UTestAbilityComponent::UTestAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTestAbilityComponent::ActivateAbility(const FGameplayTag& AbilityTag)
{
	ActiveAbilities.AddTag(AbilityTag);
	UE_LOG(LogSandboxShell, Display, TEXT("Ability: +%s (active count %d)"),
		*AbilityTag.ToString(), ActiveAbilities.Num());
}

void UTestAbilityComponent::CancelAll()
{
	const int32 N = ActiveAbilities.Num();
	ActiveAbilities.Reset();
	UE_LOG(LogSandboxShell, Display, TEXT("Ability: CancelAll -> cancelled %d abilities"), N);
}
