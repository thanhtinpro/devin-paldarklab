#include "TestShellPawn.h"

#include "SandboxShellLog.h"
#include "TestHealthComponent.h"
#include "TestInventoryComponent.h"
#include "TestAbilityComponent.h"

ATestShellPawn::ATestShellPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	// Each slot = one CreateDefaultSubobject in the constructor. Adding a new
	// concern (Stamina, Camera, Interaction, ...) means adding one new line here
	// and one new component class — no pawn-class surgery elsewhere.
	HealthComp    = CreateDefaultSubobject<UTestHealthComponent>(TEXT("HealthComp"));
	InventoryComp = CreateDefaultSubobject<UTestInventoryComponent>(TEXT("InventoryComp"));
	AbilityComp   = CreateDefaultSubobject<UTestAbilityComponent>(TEXT("AbilityComp"));
}

void ATestShellPawn::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogSandboxShell, Display, TEXT("ShellPawn::BeginPlay -> brokering inter-component delegates"));

	// Broker pattern: the SHELL knows about its slots and wires them up.
	// Inventory.cpp never includes TestHealthComponent.h, and vice-versa.
	// Adding a new listener for OnHealthZeroed = one more AddLambda here.
	if (HealthComp)
	{
		// Inventory: on death, drop everything.
		if (InventoryComp)
		{
			HealthComp->OnHealthZeroed.AddUObject(InventoryComp, &UTestInventoryComponent::DropAll);
		}
		// Ability: on death, cancel all in-flight abilities.
		if (AbilityComp)
		{
			HealthComp->OnHealthZeroed.AddUObject(AbilityComp, &UTestAbilityComponent::CancelAll);
		}
	}
}
