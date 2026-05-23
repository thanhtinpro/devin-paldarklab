#include "Pal/Activities/PaldarkActivity_Patrol.h"

#include "GameFramework/Character.h"

#include "Pal/Components/PaldarkPalLocomotionComponent.h"
#include "Pal/Components/PaldarkPalPatrolComponent.h"
#include "Pal/Components/PaldarkPalPerceptionComponent.h"
#include "Pal/PaldarkPalCharacter.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"

UPaldarkActivity_Patrol::UPaldarkActivity_Patrol()
{
	ActivityTag = PaldarkGameplayTags::TAG_Paldark_Pal_Activity_Patrol;
	// Priority 15 — above Idle (10), below Follow (20) / Investigate (30) /
	// Stalk (25) / Combat (40). Hostile Pals don't have Follow; Patrol fills
	// the "I'm doing something while waiting" slot.
	Priority    = 15;
}

bool UPaldarkActivity_Patrol::InitActivity(UPaldarkPalActivityComponent* InComponent)
{
	if (!Super::InitActivity(InComponent))
	{
		return false;
	}

	if (APaldarkPalCharacter* Pal = GetPalOwner())
	{
		PatrolRef = Pal->FindComponentByClass<UPaldarkPalPatrolComponent>();
	}

	if (!PatrolRef.IsValid())
	{
		UE_LOG(LogPaldarkPal, Verbose,
			TEXT("UPaldarkActivity_Patrol::InitActivity — pal=%s has no UPaldarkPalPatrolComponent; activity will never run."),
			*GetNameSafe(GetPalOwner()));
	}
	return true;
}

bool UPaldarkActivity_Patrol::CanRun_Implementation() const
{
	const UPaldarkPalPatrolComponent* Patrol = PatrolRef.Get();
	if (Patrol == nullptr || !Patrol->HasWaypoints())
	{
		return false;
	}

	// Defensive gate — Patrol must NOT preempt Combat/Stalk when there's a
	// visible threat. Priority handles preempt naturally, but the explicit
	// check keeps DumpThreat output readable.
	const APaldarkPalCharacter* Pal = GetPalOwner();
	if (Pal != nullptr)
	{
		if (const UPaldarkPalPerceptionComponent* Perception = Pal->FindComponentByClass<UPaldarkPalPerceptionComponent>())
		{
			if (Perception->HasThreat())
			{
				return false;
			}
		}
	}
	return true;
}

bool UPaldarkActivity_Patrol::ShouldContinue_Implementation() const
{
	return CanRun();
}

void UPaldarkActivity_Patrol::EnterActivity_Implementation()
{
	Super::EnterActivity_Implementation();
	if (UPaldarkPalLocomotionComponent* Loco = GetLocomotionSlot())
	{
		// Defensive — hostile Pals don't follow but the locomotion comp
		// still ticks the follow loop if `bFollowEnabled` stays true. Pause
		// here to be explicit.
		Loco->SetFollowEnabled(false);
	}
	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkActivity_Patrol::EnterActivity — pal=%s starting patrol (%d waypoints)"),
		*GetNameSafe(GetPalOwner()),
		PatrolRef.IsValid() ? PatrolRef->Waypoints.Num() : 0);
}

void UPaldarkActivity_Patrol::TickActivity_Implementation(float /*DeltaSeconds*/)
{
	UPaldarkPalPatrolComponent* Patrol = PatrolRef.Get();
	APaldarkPalCharacter*       Pal    = GetPalOwner();
	if (Patrol == nullptr || Pal == nullptr || !Patrol->HasWaypoints())
	{
		return;
	}

	const FVector Target = Patrol->GetCurrentWaypoint();
	FVector Delta = Target - Pal->GetActorLocation();
	Delta.Z = 0.f; // Planar steering — Z handled by the movement comp / nav.
	const float Distance = Delta.Size();

	if (Distance <= Patrol->ArrivalRadius)
	{
		Patrol->AdvanceToNextWaypoint();
		return;
	}

	Pal->AddMovementInput(Delta.GetSafeNormal(), 1.f);
}

void UPaldarkActivity_Patrol::ExitActivity_Implementation()
{
	if (UPaldarkPalLocomotionComponent* Loco = GetLocomotionSlot())
	{
		Loco->SetFollowEnabled(true);
	}
	Super::ExitActivity_Implementation();
}
