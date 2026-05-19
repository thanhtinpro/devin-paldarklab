#include "Pal/Activities/PaldarkActivity_Idle.h"

#include "PaldarkGameplayTags.h"
#include "Pal/Components/PaldarkPalLocomotionComponent.h"

UPaldarkActivity_Idle::UPaldarkActivity_Idle()
{
	ActivityTag = PaldarkGameplayTags::TAG_Paldark_Pal_Activity_Idle;
	Priority    = 10;
}

void UPaldarkActivity_Idle::EnterActivity_Implementation()
{
	Super::EnterActivity_Implementation();
	if (UPaldarkPalLocomotionComponent* Loco = GetLocomotionSlot())
	{
		Loco->SetFollowEnabled(false);
	}
}

void UPaldarkActivity_Idle::ExitActivity_Implementation()
{
	if (UPaldarkPalLocomotionComponent* Loco = GetLocomotionSlot())
	{
		Loco->SetFollowEnabled(true);
	}
	Super::ExitActivity_Implementation();
}
