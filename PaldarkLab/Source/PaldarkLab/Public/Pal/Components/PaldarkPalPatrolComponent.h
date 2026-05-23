// PALDARK W20-21 — Pal patrol waypoint component.
//
// Holds the designer-authored waypoint list + the current index a hostile
// Pal is travelling toward. The `UPaldarkActivity_Patrol` activity reads
// `GetCurrentWaypoint()` to know where to steer, and calls
// `AdvanceToNextWaypoint()` when the Pal arrives.
//
// Two authoring paths:
//   1. Static FVector list — designer fills `Waypoints` directly on the
//      Blueprint subclass of the Pal (or on the spawned C++ subclass).
//      Coordinates are world-space.
//   2. Spawner-driven — `APaldarkHostilePalSpawner` populates the list at
//      runtime by reading its own `WaypointActors[]` array and calling
//      `SetWaypointsFromActors`. This is the path used by the W20-21
//      `Paldark.Pal.SpawnDirehoundPack` console flow.
//
// Behaviour:
//   - `HasWaypoints` returns false when the list is empty — Patrol activity
//     refuses to run, FSM falls back to Idle.
//   - `AdvanceToNextWaypoint` wraps around (modulo) so a Pal with 4 waypoints
//     loops 0 → 1 → 2 → 3 → 0 forever.
//   - `bRandomizeStartIndex` shuffles the initial index per Pal so a pack
//     of 4 Direhounds doesn't all march to the same waypoint on spawn.
//
// What this is NOT (deferred):
//   - No nav query — Patrol activity uses raw `AddMovementInput` toward the
//     waypoint. Designers must place waypoints reachable by walking.
//     Pathfinding lands in W30+ when designer levels grow past sandbox scale.
//   - No per-waypoint dwell timer — Patrol activity advances immediately on
//     arrival. Dwell / "stand and look around" behaviour lands W22+.
//   - No replication — the Pal's transform replicates via ACharacter; this
//     component only feeds Patrol activity server-side.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PaldarkPalPatrolComponent.generated.h"

class AActor;

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent), Abstract = false)
class PALDARKLAB_API UPaldarkPalPatrolComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkPalPatrolComponent();

	virtual void BeginPlay() override;

	// Designer knobs ----------------------------------------------------

	// Designer-authored waypoint list. World-space. Empty = no patrol.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Pal|Patrol")
	TArray<FVector> Waypoints;

	// Arrival tolerance in centimeters. Once the Pal is within this distance
	// of the current waypoint, the activity advances to the next one.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Patrol", meta = (ClampMin = "0.0"))
	float ArrivalRadius = 200.f;

	// When true, the starting index is randomised on BeginPlay so a pack
	// of N Pals spawned at the same spawner don't all march to waypoint 0
	// in lock-step. Server-only randomisation.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Patrol")
	bool bRandomizeStartIndex = true;

	// Accessors --------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Paldark|Pal|Patrol")
	bool HasWaypoints() const { return Waypoints.Num() > 0; }

	UFUNCTION(BlueprintPure, Category = "Paldark|Pal|Patrol")
	int32 GetCurrentWaypointIndex() const { return CurrentWaypointIndex; }

	// Returns the world-space location of the current waypoint. Returns
	// FVector::ZeroVector when no waypoints exist (callers must
	// `HasWaypoints` first).
	UFUNCTION(BlueprintPure, Category = "Paldark|Pal|Patrol")
	FVector GetCurrentWaypoint() const;

	// Advance to the next waypoint, wrapping modulo Waypoints.Num().
	UFUNCTION(BlueprintCallable, Category = "Paldark|Pal|Patrol")
	void AdvanceToNextWaypoint();

	// Replace the waypoint list with a copy of the input. Used by
	// `APaldarkHostilePalSpawner::BeginPlay` to push spawner-owned
	// waypoints onto each spawned Pal.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Pal|Patrol")
	void SetWaypointsFromLocations(const TArray<FVector>& InWaypoints);

	// Convenience — reads `GetActorLocation()` from each non-null actor and
	// forwards to `SetWaypointsFromLocations`. Skips null entries.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Pal|Patrol")
	void SetWaypointsFromActors(const TArray<AActor*>& InWaypointActors);

	// Dump the patrol state to LogPaldarkPal. Used by `Paldark.Pal.DumpPackState`.
	void DumpToLog() const;

private:
	// Updated by AdvanceToNextWaypoint. Server-only state.
	UPROPERTY(Transient)
	int32 CurrentWaypointIndex = 0;
};
