#include "Pal/Activities/PaldarkActivity_Follow.h"

#include "GameFramework/Pawn.h"

#include "PaldarkGameplayTags.h"
#include "Pal/Components/PaldarkPalLocomotionComponent.h"

UPaldarkActivity_Follow::UPaldarkActivity_Follow()
{
	ActivityTag = PaldarkGameplayTags::TAG_Paldark_Pal_Activity_Follow;
	Priority    = 20;
}

bool UPaldarkActivity_Follow::CanRun_Implementation() const
{
	const APawn* Leader = GetFollowedPawn();
	if (Leader == nullptr)
	{
		return false;
	}
	return GetPlanarDistanceToFollowedPawn() > EnterDistance;
}

bool UPaldarkActivity_Follow::ShouldContinue_Implementation() const
{
	const APawn* Leader = GetFollowedPawn();
	if (Leader == nullptr)
	{
		return false;
	}
	return GetPlanarDistanceToFollowedPawn() > ExitDistance;
}

void UPaldarkActivity_Follow::EnterActivity_Implementation()
{
	Super::EnterActivity_Implementation();
	if (UPaldarkPalLocomotionComponent* Loco = GetLocomotionSlot())
	{
		// Idle disabled the leash; re-enable so locomotion steers toward the leader.
		Loco->SetFollowEnabled(true);
	}
}
