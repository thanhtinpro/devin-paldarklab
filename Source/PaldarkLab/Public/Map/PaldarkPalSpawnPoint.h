// PALDARK W31-32 — Designer-placeable Pal spawn point actor.
//
// Drop an `APaldarkPalSpawnPoint` into a level (Map 2 "Rừng Hỏng" blockout),
// set its `SpeciesTag` (one of `Paldark.Pal.Species.*`), optionally an
// `AssociatedPOITag`, and BeginPlay (server only) resolves the spawn through
// one of two paths:
//
//   1. **Definition path (W27-28)** — preferred when `DefinitionId` is set
//      (or auto-derived from `SpeciesTag`). Calls
//      `UPaldarkPalSpawnSubsystem::SpawnPalAsync(DefId, Transform)` so the
//      species' Spawn asset bundle (mesh + anim + abilities) is async-loaded
//      first, then the Pal spawns at the spawn point's transform.
//   2. **Class path (W20-21)** — fallback when designers want a hard
//      `TSubclassOf<APaldarkPalCharacter>` reference (e.g. the C++
//      Direhound / Razorbird / Stoneclad / Vinewraith / Boltmane
//      subclasses that don't yet have a `DA_PalDef_*` shipped). Calls
//      `APaldarkHostilePalSpawner::SpawnHostilePackAt(...)` with count = 1
//      so the W20-21 console-spawn helper reuses the same ground-snap
//      logic.
//
// Path choice is deterministic on actor BeginPlay:
//   - Definition path if `PalDefinitionId.IsValid()` OR `SpeciesTag` resolves
//     to a known primary asset row via `UPaldarkAssetManager::ResolveSpeciesAsset`.
//   - Else class path if `FallbackPalClass` is set.
//   - Else log Warning + skip spawn.
//
// Why a separate actor (not just `APaldarkHostilePalSpawner` from W20-21):
//   - The W20-21 spawner is "spawn N Pals around me + waypoints" — designed
//     for pack drops. Map 2 blockout needs *single-spawn* points scattered
//     across the level (designer drops 16 of them, not 4 spawner actors of
//     4 Pals each).
//   - Spawn point semantically owns the *location* — a designer tweaks the
//     map by moving spawn points individually. Spawner from W20-21 owns the
//     *pack* — a different mental model.
//   - The W20-21 spawner stays as-is for pack-style encounters (the
//     existing Raid_Sandbox map continues to work).
//
// Concurrent-instance budget:
//   - `MaxConcurrentInstances` (default 1) limits how many spawn fan-outs
//     are allowed at once from this point. Killing the spawned Pal frees
//     the slot — the spawn point doesn't respawn automatically (W31-32
//     scope; respawn loop lands W35-36 with capture system).
//
// What this is NOT (deferred):
//   - No respawn timer. Killed Pals stay dead — Q3 polish.
//   - No replication of the spawn point itself; clients see the spawned
//     Pal through normal actor replication.
//   - No spawn condition (player nearby, day/night, etc.). Conditions land
//     W34-35 with procedural map gen.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "UObject/PrimaryAssetId.h"
#include "PaldarkPalSpawnPoint.generated.h"

class APaldarkPalCharacter;
class UBillboardComponent;
class UPaldarkPalSpawnSubsystem;

UCLASS(ClassGroup = (Paldark), meta = (DisplayName = "Paldark Pal Spawn Point"))
class PALDARKLAB_API APaldarkPalSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	APaldarkPalSpawnPoint();

	virtual void BeginPlay() override;

	// Designer knobs ----------------------------------------------------

	// Species tag for the Pal to spawn. Must be one of
	// `Paldark.Pal.Species.*`. Used for AssetManager species → definition
	// lookup (when `PalDefinitionId` is left empty) + validator hint
	// (`AllowedSpeciesTags` on the linked POI should contain this tag).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|SpawnPoint",
		meta = (Categories = "Paldark.Pal.Species"))
	FGameplayTag SpeciesTag;

	// Optional explicit `UPaldarkPalDefinition` primary asset id. When
	// set, overrides the species → definition lookup. Leave invalid to
	// let BeginPlay derive the id from `SpeciesTag` via the AssetManager.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|SpawnPoint")
	FPrimaryAssetId PalDefinitionId;

	// Fallback Pal class used when neither `PalDefinitionId` nor a
	// species → definition lookup resolves. Required for the hostile
	// C++ subclasses (Direhound / Razorbird / Stoneclad / Vinewraith /
	// Boltmane) until a `DA_PalDef_*` ships for them. Optional for
	// companion species (Foxparks / Tombat) that ship as data assets.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|SpawnPoint")
	TSubclassOf<APaldarkPalCharacter> FallbackPalClass;

	// Should this spawn point fire automatically on BeginPlay? Default
	// true. Set false for "designer-paused" spawn points (test maps,
	// future Activity Director gated spawn).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|SpawnPoint")
	bool bSpawnOnBeginPlay = true;

	// Maximum number of concurrently-alive Pals this spawn point will
	// have outstanding. Default 1 — Map 2 blockout uses one point per
	// hostile so designers can scatter exact spawn locations. Future
	// "Pal field" points might bump this to 3+ for swarm spawns.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|SpawnPoint",
		meta = (ClampMin = "1", UIMin = "1"))
	int32 MaxConcurrentInstances = 1;

	// Optional reference back to the POI this spawn point sits inside.
	// The validator + future AI Director use this to check
	// "spawn point's SpeciesTag must appear in the POI's
	// AllowedSpeciesTags container".
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|SpawnPoint",
		meta = (Categories = "Paldark.POI.Type"))
	FGameplayTag AssociatedPOITag;

	// Read-only accessors ----------------------------------------------

	// True once `BeginPlay` actually issued a spawn (either path).
	// `Paldark.POI.Dump` console output flags spawn points that never
	// fired so designers can spot mis-wired ones.
	bool HasFiredSpawn() const { return bHasFiredSpawn; }

	// How many spawned instances are still alive (slots used).
	int32 GetActiveInstanceCount() const { return ActiveInstances.Num(); }

protected:
	// Editor billboard so the spawn point shows up in the level editor
	// without a static-mesh component (selects via icon click).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|SpawnPoint",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBillboardComponent> Billboard;

	// Definition-path spawn — resolves DefId → PalDefinition → SpawnPalAsync.
	// Returns true if the async request was issued. False = caller should
	// try the class path.
	bool TryDefinitionPathSpawn(UPaldarkPalSpawnSubsystem* Subsystem);

	// Class-path spawn — uses the W20-21 SpawnHostilePackAt helper with
	// count = 1. Returns true on success.
	bool TryClassPathSpawn();

	// Bookkeeping for the concurrent-instance budget.
	void RegisterSpawnedInstance(APaldarkPalCharacter* InSpawned);
	void OnSpawnedInstanceEndPlay(AActor* Actor, EEndPlayReason::Type Reason);

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<APaldarkPalCharacter>> ActiveInstances;

	bool bHasFiredSpawn = false;
};
