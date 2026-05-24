#include "TestHealthComponent.h"

#include "SandboxShellLog.h"

UTestHealthComponent::UTestHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTestHealthComponent::ApplyDamage(int32 Amount)
{
	if (Health <= 0)
	{
		return; // already dead — idempotent
	}

	const int32 Before = Health;
	Health = FMath::Max(0, Health - Amount);
	UE_LOG(LogSandboxShell, Display, TEXT("Health: %d -> %d (damage %d)"), Before, Health, Amount);

	if (Health == 0)
	{
		UE_LOG(LogSandboxShell, Display, TEXT("Health: zeroed -> broadcasting OnHealthZeroed"));
		OnHealthZeroed.Broadcast();
	}
}
