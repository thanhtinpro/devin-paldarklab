#include "Pal/Activities/PaldarkActivity_Investigate.h"

#include "GameFramework/Character.h"

#include "PaldarkGameplayTags.h"
#include "Pal/Components/PaldarkPalActivityComponent.h"
#include "Pal/Components/PaldarkPalLocomotionComponent.h"
#include "Pal/PaldarkPalCharacter.h"
#include "PaldarkLogCategories.h"

UPaldarkActivity_Investigate::UPaldarkActivity_Investigate()
{
	ActivityTag = PaldarkGameplayTags::TAG_Paldark_Pal_Activity_Investigate;
	Priority    = 30;
}

bool UPaldarkActivity_Investigate::CanRun_Implementation() const
{
	const UPaldarkPalActivityComponent* Component = GetActivityComponent();
	return Component != nullptr && Component->HasActiveInvestigateRequest();
}

bool UPaldarkActivity_Investigate::ShouldContinue_Implementation() const
{
	// Investigate "sticks" once we have a request — the activity itself
	// clears the request when it arrives or times out.
	return CanRun();
}

void UPaldarkActivity_Investigate::EnterActivity_Implementation()
{
	Super::EnterActivity_Implementation();
	ElapsedActivityTime = 0.0f;
	if (UPaldarkPalLocomotionComponent* Loco = GetLocomotionSlot())
	{
		Loco->SetFollowEnabled(false);
	}
}

void UPaldarkActivity_Investigate::TickActivity_Implementation(float DeltaSeconds)
{
	UPaldarkPalActivityComponent* Component = GetActivityComponent();
	APaldarkPalCharacter* PalChar = GetPalOwner();
	if (Component == nullptr || PalChar == nullptr)
	{
		return;
	}

	ElapsedActivityTime += DeltaSeconds;
	if (ElapsedActivityTime >= MaxInvestigateTime)
	{
		UE_LOG(LogPaldarkPal, Log, TEXT("UPaldarkActivity_Investigate::TickActivity — pal=%s timeout after %.1fs"),
			*PalChar->GetName(),
			ElapsedActivityTime);
		Component->ClearInvestigateRequest();
		return;
	}

	const FVector Target = Component->GetInvestigateTarget();
	FVector Delta = Target - PalChar->GetActorLocation();
	Delta.Z = 0.0f;
	const float Distance = Delta.Size();

	if (Distance <= ArrivalRadius)
	{
		UE_LOG(LogPaldarkPal, Log, TEXT("UPaldarkActivity_Investigate::TickActivity — pal=%s arrived (dist=%.1f cm)"),
			*PalChar->GetName(),
			Distance);
		Component->ClearInvestigateRequest();
		return;
	}

	PalChar->AddMovementInput(Delta.GetSafeNormal(), 1.0f);
}

void UPaldarkActivity_Investigate::ExitActivity_Implementation()
{
	if (UPaldarkPalLocomotionComponent* Loco = GetLocomotionSlot())
	{
		Loco->SetFollowEnabled(true);
	}
	Super::ExitActivity_Implementation();
}
