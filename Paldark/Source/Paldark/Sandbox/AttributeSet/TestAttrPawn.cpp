#include "TestAttrPawn.h"

#include "SandboxAttrLog.h"
#include "TestPaldarkAttributeSet.h"

#include "AbilitySystemComponent.h"

ATestAttrPawn::ATestAttrPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	// Default subobjects: ASC discovers child UAttributeSet subobjects via
	// reflection during init, so no manual AddAttributeSetSubobject call needed.
	ASC     = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
	AttrSet = CreateDefaultSubobject<UTestPaldarkAttributeSet>(TEXT("AttrSet"));
}

void ATestAttrPawn::BeginPlay()
{
	Super::BeginPlay();

	// InitAbilityActorInfo(OwnerActor, AvatarActor). For sandbox: same actor.
	if (ASC)
	{
		ASC->InitAbilityActorInfo(this, this);
		UE_LOG(LogSandboxAttr, Display, TEXT("ATestAttrPawn::BeginPlay -> ASC InitAbilityActorInfo done"));
	}
}
