// Sandbox Lesson 06 — minimal ASC-bearing pawn.
//
// IAbilitySystemInterface lets GAS find the component from anywhere with
// just an AActor*. SANDBOX: production Paldark spawns ASC on PlayerState
// instead of Pawn (Lyra pattern) so it survives pawn respawn. Sandbox keeps
// it on Pawn for brevity — the AttributeSet lifecycle is identical either way.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AbilitySystemInterface.h"
#include "TestAttrPawn.generated.h"

class UAbilitySystemComponent;
class UTestPaldarkAttributeSet;

UCLASS()
class PALDARK_API ATestAttrPawn : public APawn, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ATestAttrPawn();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return ASC; }

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	UAbilitySystemComponent* ASC = nullptr;

	UPROPERTY(VisibleAnywhere)
	UTestPaldarkAttributeSet* AttrSet = nullptr;
};
