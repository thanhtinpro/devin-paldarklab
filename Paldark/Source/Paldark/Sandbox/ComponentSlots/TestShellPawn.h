// Sandbox Lesson 04 — the SHELL pawn.
//
// Holds N components as named slots, brokers inter-component wires in BeginPlay,
// and otherwise stays empty. The pawn class never grows when you add a feature —
// you add a new component class instead.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TestShellPawn.generated.h"

class UTestHealthComponent;
class UTestInventoryComponent;
class UTestAbilityComponent;

UCLASS()
class PALDARK_API ATestShellPawn : public APawn
{
	GENERATED_BODY()

public:
	ATestShellPawn();

	virtual void BeginPlay() override;

	// Public so the driver can poke them in test cases. In production these
	// would expose interfaces and be queried via FindComponentByClass.
	UPROPERTY(VisibleAnywhere)
	UTestHealthComponent* HealthComp = nullptr;

	UPROPERTY(VisibleAnywhere)
	UTestInventoryComponent* InventoryComp = nullptr;

	UPROPERTY(VisibleAnywhere)
	UTestAbilityComponent* AbilityComp = nullptr;
};
