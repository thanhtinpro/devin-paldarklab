#include "Pal/Components/PaldarkPalPatrolComponent.h"

#include "GameFramework/Actor.h"

#include "PaldarkLogCategories.h"

UPaldarkPalPatrolComponent::UPaldarkPalPatrolComponent()
{
	// Patrol state is read by the activity every tick but the component
	// itself doesn't tick — activity drives all the logic.
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UPaldarkPalPatrolComponent::BeginPlay()
{
	Super::BeginPlay();

	const AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr || !OwnerActor->HasAuthority())
	{
		return;
	}

	if (bRandomizeStartIndex && Waypoints.Num() > 0)
	{
		CurrentWaypointIndex = FMath::RandRange(0, Waypoints.Num() - 1);
		UE_LOG(LogPaldarkPal, Verbose,
			TEXT("UPaldarkPalPatrolComponent::BeginPlay — pal=%s randomised start index to %d (of %d waypoints)"),
			*GetNameSafe(OwnerActor),
			CurrentWaypointIndex,
			Waypoints.Num());
	}
}

FVector UPaldarkPalPatrolComponent::GetCurrentWaypoint() const
{
	if (Waypoints.Num() == 0)
	{
		return FVector::ZeroVector;
	}
	const int32 Index = FMath::Clamp(CurrentWaypointIndex, 0, Waypoints.Num() - 1);
	return Waypoints[Index];
}

void UPaldarkPalPatrolComponent::AdvanceToNextWaypoint()
{
	if (Waypoints.Num() == 0)
	{
		return;
	}
	CurrentWaypointIndex = (CurrentWaypointIndex + 1) % Waypoints.Num();
	UE_LOG(LogPaldarkPal, Verbose,
		TEXT("UPaldarkPalPatrolComponent::AdvanceToNextWaypoint — pal=%s next=%d location=%s"),
		*GetNameSafe(GetOwner()),
		CurrentWaypointIndex,
		*GetCurrentWaypoint().ToCompactString());
}

void UPaldarkPalPatrolComponent::SetWaypointsFromLocations(const TArray<FVector>& InWaypoints)
{
	Waypoints = InWaypoints;
	CurrentWaypointIndex = 0;
	if (bRandomizeStartIndex && Waypoints.Num() > 0)
	{
		CurrentWaypointIndex = FMath::RandRange(0, Waypoints.Num() - 1);
	}
	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkPalPatrolComponent::SetWaypointsFromLocations — pal=%s waypoints=%d start_index=%d"),
		*GetNameSafe(GetOwner()),
		Waypoints.Num(),
		CurrentWaypointIndex);
}

void UPaldarkPalPatrolComponent::SetWaypointsFromActors(const TArray<AActor*>& InWaypointActors)
{
	TArray<FVector> Locations;
	Locations.Reserve(InWaypointActors.Num());
	for (const AActor* Actor : InWaypointActors)
	{
		if (Actor != nullptr)
		{
			Locations.Add(Actor->GetActorLocation());
		}
	}
	SetWaypointsFromLocations(Locations);
}

void UPaldarkPalPatrolComponent::DumpToLog() const
{
	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkPalPatrolComponent[%s] waypoints=%d current_index=%d current_location=%s arrival_radius=%.1f"),
		*GetNameSafe(GetOwner()),
		Waypoints.Num(),
		CurrentWaypointIndex,
		*GetCurrentWaypoint().ToCompactString(),
		ArrivalRadius);
}
