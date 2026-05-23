#include "Pal/PaldarkHostilePalSpawner.h"

#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/KismetMathLibrary.h"

#include "Pal/Components/PaldarkPalPatrolComponent.h"
#include "Pal/PaldarkPalCharacter.h"
#include "PaldarkLogCategories.h"

APaldarkHostilePalSpawner::APaldarkHostilePalSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	// Server-only — clients don't run BeginPlay's spawn flow because the
	// spawned Pals replicate from the authoritative server. Replicate the
	// spawner actor itself so designers placing it in a level-streamed
	// sub-level can see it on client too.
	bReplicates = false;
}

void APaldarkHostilePalSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	if (PalClassToSpawn == nullptr)
	{
		UE_LOG(LogPaldarkPal, Warning,
			TEXT("APaldarkHostilePalSpawner::BeginPlay — %s has no PalClassToSpawn; nothing spawned."),
			*GetName());
		return;
	}

	// Snapshot waypoint locations once — avoids one Cast/GetLocation per
	// spawned Pal at spawn time.
	TArray<FVector> WaypointLocations;
	WaypointLocations.Reserve(WaypointActors.Num());
	for (const TObjectPtr<AActor>& WaypointActor : WaypointActors)
	{
		if (AActor* Actor = WaypointActor.Get())
		{
			WaypointLocations.Add(Actor->GetActorLocation());
		}
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	int32 Spawned = 0;
	for (int32 Index = 0; Index < SpawnCount; ++Index)
	{
		const FTransform SpawnXform = ResolveSpawnTransformForIndex(Index);

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner       = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		APaldarkPalCharacter* Pal = World->SpawnActor<APaldarkPalCharacter>(
			PalClassToSpawn, SpawnXform, SpawnParams);
		if (Pal == nullptr)
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("APaldarkHostilePalSpawner::BeginPlay — %s failed to spawn Pal at index %d."),
				*GetName(), Index);
			continue;
		}

		// Stamp the spawner's pack tag onto the Pal so the pack subsystem
		// groups them. Designer can leave PackTag empty on the spawner to
		// inherit the species default (Direhound's BeginPlay assigns
		// Paldark.Pal.Pack.Direhound when empty).
		if (PackTag.IsValid())
		{
			Pal->SetPackTag(PackTag);
		}

		// Push waypoints onto the Pal's patrol component if the spawner has any.
		if (WaypointLocations.Num() > 0)
		{
			if (UPaldarkPalPatrolComponent* Patrol = Pal->GetPatrolSlot())
			{
				Patrol->SetWaypointsFromLocations(WaypointLocations);
			}
		}
		++Spawned;
	}

	UE_LOG(LogPaldarkPal, Log,
		TEXT("APaldarkHostilePalSpawner::BeginPlay — %s spawned %d/%d Pals (class=%s, pack=%s, waypoints=%d)"),
		*GetName(),
		Spawned,
		SpawnCount,
		PalClassToSpawn != nullptr ? *PalClassToSpawn->GetName() : TEXT("<null>"),
		*PackTag.ToString(),
		WaypointLocations.Num());
}

FTransform APaldarkHostilePalSpawner::ResolveSpawnTransformForIndex(int32 InIndex) const
{
	const FVector BaseLocation = GetActorLocation();

	// Even ring distribution around the spawner so 4 Pals don't overlap.
	const float Angle = (SpawnCount > 0)
		? (2.f * PI * static_cast<float>(InIndex) / static_cast<float>(SpawnCount))
		: 0.f;
	const FVector RingOffset(
		FMath::Cos(Angle) * SpawnSpreadRadius,
		FMath::Sin(Angle) * SpawnSpreadRadius,
		0.f);

	FVector Candidate = BaseLocation + RingOffset;

	// Ground snap — line-trace down so the Pal spawns on the floor instead
	// of mid-air at the spawner's Z (which is often the designer's eye
	// height for visibility).
	const UWorld* World = GetWorld();
	if (World != nullptr && GroundTraceDown > 0.f)
	{
		const FVector TraceStart(Candidate.X, Candidate.Y, BaseLocation.Z + GroundTraceUp);
		const FVector TraceEnd  (Candidate.X, Candidate.Y, BaseLocation.Z - GroundTraceDown);

		FCollisionQueryParams Params(SCENE_QUERY_STAT(PaldarkHostilePalSpawnerSnap), /*bTraceComplex=*/false, this);
		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, GroundTraceChannel.GetValue(), Params))
		{
			Candidate.Z = Hit.Location.Z + 50.f; // small lift so capsule doesn't clip floor.
		}
		else
		{
			Candidate.Z = BaseLocation.Z;
		}
	}

	return FTransform(GetActorRotation(), Candidate);
}

int32 APaldarkHostilePalSpawner::SpawnHostilePackAt(
	UWorld*                          InWorld,
	TSubclassOf<APaldarkPalCharacter> InPalClass,
	const FGameplayTag&              InPackTag,
	const FVector&                   InOrigin,
	int32                            InCount,
	float                            InSpreadRadius,
	const TArray<FVector>&           InWaypoints)
{
	if (InWorld == nullptr || InPalClass == nullptr || InCount <= 0)
	{
		return 0;
	}

	int32 Spawned = 0;
	for (int32 Index = 0; Index < InCount; ++Index)
	{
		const float Angle = (InCount > 0)
			? (2.f * PI * static_cast<float>(Index) / static_cast<float>(InCount))
			: 0.f;
		const FVector RingOffset(
			FMath::Cos(Angle) * InSpreadRadius,
			FMath::Sin(Angle) * InSpreadRadius,
			0.f);
		const FTransform SpawnXform(FRotator::ZeroRotator, InOrigin + RingOffset);

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		APaldarkPalCharacter* Pal = InWorld->SpawnActor<APaldarkPalCharacter>(
			InPalClass, SpawnXform, SpawnParams);
		if (Pal == nullptr)
		{
			continue;
		}

		if (InPackTag.IsValid())
		{
			Pal->SetPackTag(InPackTag);
		}
		if (InWaypoints.Num() > 0)
		{
			if (UPaldarkPalPatrolComponent* Patrol = Pal->GetPatrolSlot())
			{
				Patrol->SetWaypointsFromLocations(InWaypoints);
			}
		}
		++Spawned;
	}

	UE_LOG(LogPaldarkPal, Log,
		TEXT("APaldarkHostilePalSpawner::SpawnHostilePackAt — spawned %d/%d at %s (class=%s pack=%s waypoints=%d)"),
		Spawned, InCount,
		*InOrigin.ToCompactString(),
		*InPalClass->GetName(),
		*InPackTag.ToString(),
		InWaypoints.Num());

	return Spawned;
}
