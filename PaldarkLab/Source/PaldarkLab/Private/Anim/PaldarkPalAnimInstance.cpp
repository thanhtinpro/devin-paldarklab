// PALDARK W27 — UPaldarkPalAnimInstance impl.

#include "Anim/PaldarkPalAnimInstance.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Pal/Components/PaldarkPalActivityComponent.h"
#include "Pal/PaldarkPalCharacter.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"

void UPaldarkPalAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (APaldarkPalCharacter* PalCharacter = Cast<APaldarkPalCharacter>(GetOwningActor()))
	{
		CachedPalCharacter = PalCharacter;
		if (UPaldarkPalActivityComponent* ActivityComponent = PalCharacter->GetActivitySlot())
		{
			CachedActivityComponent = ActivityComponent;
		}
	}

	CombatActivityTag = PaldarkGameplayTags::TAG_Paldark_Pal_Activity_Combat;
	HostileTeamTag    = PaldarkGameplayTags::TAG_Paldark_Team_Hostile;

	UE_LOG(LogPaldarkPal, Verbose,
		TEXT("UPaldarkPalAnimInstance::NativeInitializeAnimation — pal=%s activity=%s"),
		*GetNameSafe(CachedPalCharacter.Get()),
		CachedActivityComponent.IsValid() ? TEXT("yes") : TEXT("no"));
}

void UPaldarkPalAnimInstance::GatherSubclassSnapshot_GameThread(float DeltaSeconds)
{
	Super::GatherSubclassSnapshot_GameThread(DeltaSeconds);

	const APaldarkPalCharacter* PalCharacter = CachedPalCharacter.Get();
	if (PalCharacter == nullptr)
	{
		return;
	}

	Snapshot.bIsHostileTeam = HostileTeamTag.IsValid()
		&& PalCharacter->GetTeamTag() == HostileTeamTag;

	if (const UPaldarkPalActivityComponent* ActivityComponent = CachedActivityComponent.Get())
	{
		Snapshot.CurrentActivityTag = ActivityComponent->GetCurrentActivityTag();
	}
	else
	{
		Snapshot.CurrentActivityTag = FGameplayTag::EmptyTag;
	}
}

void UPaldarkPalAnimInstance::DeriveSubclassAnimProperties_ThreadSafe(float DeltaSeconds)
{
	Super::DeriveSubclassAnimProperties_ThreadSafe(DeltaSeconds);

	bIsHostile  = Snapshot.bIsHostileTeam;
	// MOVE_Flying = 4 per EMovementMode. We compare to the enum value
	// directly to avoid pulling the header on the worker thread.
	bIsFlying   = (Snapshot.MovementMode == static_cast<uint8>(EMovementMode::MOVE_Flying));
	bIsInCombat = CombatActivityTag.IsValid()
		&& Snapshot.CurrentActivityTag == CombatActivityTag;

	CurrentActivityTag = Snapshot.CurrentActivityTag;
}
