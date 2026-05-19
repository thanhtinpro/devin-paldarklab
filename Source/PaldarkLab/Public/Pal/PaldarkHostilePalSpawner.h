// PALDARK W20-21 — Hostile-Pal spawner ("Pal Den").
//
// Designer-placeable actor that spawns N hostile Pals on BeginPlay and
// distributes the spawner's `Waypoints` across each spawned Pal's
// `UPaldarkPalPatrolComponent`. This is the "spawn manager cơ bản" line item
// from Documents/PALDARK/03-Roadmap_1_Year.md § Tuần 20–21.
//
// Authoring flow (designer side):
//   1. Drop an APaldarkHostilePalSpawner into Raid_Sandbox.
//   2. Set `PalClassToSpawn` to `BP_Direhound_Default` (or Razorbird BP).
//   3. Set `SpawnCount` (default 4 — matches the roadmap deliverable).
//   4. Optionally author a `PackTag` (default = pack tag of the species —
//      the spawner stamps each spawned Pal with this tag so the pack
//      subsystem groups them).
//   5. Drop 3-4 child actor / target points / static-mesh markers into the
//      level near the spawner and reference them in `WaypointActors[]`.
//      Spawned Pals copy the waypoint locations into their patrol component.
//
// What this is NOT (deferred):
//   - No respawn loop — Pals despawn permanently when killed (W22+ adds the
//     respawn timer when squad system / save-game integration lands).
//   - No level-streaming awareness — spawner fires on BeginPlay regardless
//     of distance to player (W48-49 adds significance manager culling).
//   - No "wave" / "encounter" definitions — every spawned Pal is identical.
//     Encounter authoring lands W25+ in the activity director week.
//
// Server-only spawn — clients see the spawned Pals through normal actor
// replication once the server has authored them.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "PaldarkHostilePalSpawner.generated.h"

class APaldarkPalCharacter;
class AActor;

UCLASS(ClassGroup = (Paldark), meta = (DisplayName = "Paldark Hostile Pal Spawner"))
class PALDARKLAB_API APaldarkHostilePalSpawner : public AActor
{
	GENERATED_BODY()

public:
	APaldarkHostilePalSpawner();

	virtual void BeginPlay() override;

	// Designer knobs ----------------------------------------------------

	// Hostile-Pal C++/BP class to spawn. Required — leaving this empty
	// means BeginPlay logs a warning and spawns nothing.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Spawner")
	TSubclassOf<APaldarkPalCharacter> PalClassToSpawn;

	// How many Pals to spawn. Default 4 — matches the W20-21 roadmap
	// deliverable ("4 Direhound patrol").
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Spawner", meta = (ClampMin = "1", UIMin = "1"))
	int32 SpawnCount = 4;

	// Pack tag stamped on every spawned Pal. Empty = leave the species
	// default (Direhound auto-registers under `Paldark.Pal.Pack.Direhound`;
	// Razorbird stays lone).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Spawner")
	FGameplayTag PackTag;

	// Designer-placed waypoint actors (TargetPoint, child actor, etc.).
	// Their `GetActorLocation()` is read once on BeginPlay and copied into
	// every spawned Pal's `UPaldarkPalPatrolComponent::Waypoints`.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Spawner")
	TArray<TObjectPtr<AActor>> WaypointActors;

	// Radius (cm) around the spawner location to scatter Pal spawn points.
	// Each Pal spawns at `SpawnerLocation + RandPointInCircle(SpawnSpreadRadius)`
	// so 4 spawned at the same actor don't overlap.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Spawner", meta = (ClampMin = "0.0"))
	float SpawnSpreadRadius = 300.f;

	// Trace channel used for the ground snap. ECC_Visibility by default;
	// designers can swap to a custom channel if the level uses one.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Spawner")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	// Distance (cm) above/below the spawner Z to start/end the ground snap
	// trace. Defaults cover most outdoor terrain.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Spawner", meta = (ClampMin = "0.0"))
	float GroundTraceUp = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Spawner", meta = (ClampMin = "0.0"))
	float GroundTraceDown = 2000.f;

	// Console-driven spawn helper used by `Paldark.Pal.SpawnDirehoundPack`.
	// Creates a transient spawner-equivalent at the given world location +
	// rotation and immediately spawns + configures `InCount` Pals around it.
	// Returns the number actually spawned. Server-only.
	static int32 SpawnHostilePackAt(
		UWorld*                          InWorld,
		TSubclassOf<APaldarkPalCharacter> InPalClass,
		const FGameplayTag&              InPackTag,
		const FVector&                   InOrigin,
		int32                            InCount,
		float                            InSpreadRadius,
		const TArray<FVector>&           InWaypoints);

protected:
	// Resolves the actual spawn transform for the Nth Pal — scatter in a
	// circle around the spawner + ground snap via line trace. Falls back
	// to the spawner location when the trace misses.
	FTransform ResolveSpawnTransformForIndex(int32 InIndex) const;
};
