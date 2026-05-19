// PALDARK W27-28 — Pal spawn subsystem (UWorldSubsystem, server-only).
//
// Lyra-style asynchronous spawn pipeline backed by `FStreamableManager`. Three
// public surfaces:
//
//   1. `RequestPreWarmAsync(DefId, OnDone)` — kick off async load of the
//      definition's "Spawn" asset bundle (mesh + anim instance class + ability
//      classes). Idempotent: if the same DefId is already in flight, the new
//      callback is queued onto the existing handle instead of issuing a second
//      load.
//
//   2. `SpawnPalAsync(DefId, Transform, OnSpawned)` — pre-warms if not warm,
//      then spawns `DefId.PalClass` at `Transform`, overrides the spawned
//      Pal's mesh + anim instance + grants its abilities from the
//      definition's soft refs (all resolved via the warm cache).
//
//   3. `IsPreWarmed(DefId)` — cheap predicate for HUD / debug console
//      (`Paldark.Pal.DumpDefinitionRegistry`).
//
// Server-only. `OnWorldBeginPlay` returns early on non-authoritative worlds
// (PIE clients, listen-server clients) so the subsystem only ticks on the
// authoritative game instance. Clients see spawned Pals through normal actor
// replication.
//
// Lifetime: the subsystem keeps `TArray<TSharedPtr<FStreamableHandle>>` for
// each in-flight pre-warm AND for each completed pre-warm. Completed handles
// stay alive so the cooked assets aren't garbage-collected behind the
// subsystem's back. `ClearWarmCache()` releases everything.
//
// Future work (defer Q4):
//   - "Cold spawn budget" pacing — limit how many async loads can be in
//     flight at once.
//   - Pre-warm of the *next* expected wave of Pals based on Activity Director
//     state.
//   - Persistent warm set across map travel via `UPaldarkAssetManager`.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/PrimaryAssetId.h"
#include "PaldarkPalSpawnSubsystem.generated.h"

class APaldarkPalCharacter;
class UPaldarkPalDefinition;

// Fired on the game thread when an async pre-warm completes. `bSuccess` is
// false if the definition asset itself failed to resolve or if `PalClass`
// is null. Always called exactly once per RequestPreWarmAsync call.
DECLARE_DELEGATE_TwoParams(FOnPalPreWarmComplete,
	FPrimaryAssetId /*DefId*/,
	bool /*bSuccess*/);

// Fired on the game thread after SpawnPalAsync finishes. `SpawnedPal` is
// null on failure. Always called exactly once per SpawnPalAsync call.
DECLARE_DELEGATE_TwoParams(FOnPalSpawnedAsync,
	FPrimaryAssetId /*DefId*/,
	APaldarkPalCharacter* /*SpawnedPal*/);

USTRUCT()
struct FPaldarkPalPreWarmState
{
	GENERATED_BODY()

	// Live streamable handle. Kept alive (cached after completion) so the
	// loaded assets stay resident in memory until `ClearWarmCache` runs.
	TSharedPtr<FStreamableHandle> Handle;

	// Resolved definition pointer (valid only when `bResolved` is true).
	UPROPERTY()
	TObjectPtr<const UPaldarkPalDefinition> Definition = nullptr;

	// True once the async load completes AND `Definition` resolves.
	bool bResolved = false;

	// Coalesced callbacks: callers piling on the same DefId queue their
	// callbacks here so the second/Nth caller doesn't issue a duplicate
	// FStreamableManager request.
	TArray<FOnPalPreWarmComplete> PendingCallbacks;
};

UCLASS()
class PALDARKLAB_API UPaldarkPalSpawnSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// USubsystem overrides ----------------------------------------------
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	// Convenience: get-or-null for the subsystem on `World`. Returns null
	// if the world has no game instance (early editor commandlet, etc.) or
	// if the subsystem skipped creation because of `ShouldCreateSubsystem`.
	static UPaldarkPalSpawnSubsystem* Get(const UWorld* World);

	// Async API ---------------------------------------------------------

	// Kicks off async load of the definition's "Spawn" bundle. Idempotent:
	// callers re-requesting the same DefId queue their callbacks onto the
	// existing handle. Server-authoritative only — calling on a non-server
	// world invokes `OnDone(DefId, false)` synchronously on the next tick.
	void RequestPreWarmAsync(
		const FPrimaryAssetId& DefId,
		FOnPalPreWarmComplete OnDone = FOnPalPreWarmComplete());

	// Pre-warms (if not warm) then spawns the Pal at `Transform`. `OnSpawned`
	// fires exactly once. Server-authoritative only.
	void SpawnPalAsync(
		const FPrimaryAssetId& DefId,
		const FTransform& Transform,
		FOnPalSpawnedAsync OnSpawned = FOnPalSpawnedAsync());

	// Cheap predicate. Returns true once `RequestPreWarmAsync` has resolved
	// to a non-null UPaldarkPalDefinition. False while in-flight or on
	// failed loads.
	bool IsPreWarmed(const FPrimaryAssetId& DefId) const;

	// Returns the resolved definition for `DefId` once the async load has
	// completed, else nullptr. Used by console commands + tests.
	const UPaldarkPalDefinition* FindWarmedDefinition(const FPrimaryAssetId& DefId) const;

	// All definitions currently tracked (in-flight + resolved). Used by
	// `Paldark.Pal.DumpDefinitionRegistry`.
	TArray<FPrimaryAssetId> GetTrackedDefinitionIds() const;

	// Releases every handle. Cooked assets become candidates for GC after
	// this call returns. Used by world teardown + test loops.
	void ClearWarmCache();

private:
	// Streamable handle storage keyed by primary asset id.
	UPROPERTY()
	TMap<FPrimaryAssetId, FPaldarkPalPreWarmState> WarmStates;

	// Internal: actually issues the FStreamableManager load for a fresh
	// DefId. Assumes WarmStates.Contains(DefId) is false (caller must
	// check).
	void StartLoadInternal(const FPrimaryAssetId& DefId);

	// Internal: completion routed back from FStreamableManager. Resolves
	// the definition, marks the state, fires every pending callback once.
	void HandleLoadComplete(FPrimaryAssetId DefId);

	// Internal: invoked once the definition is warm. Spawns + configures
	// the Pal and fires `OnSpawned`. Returns nullptr if PalClass is null
	// or the spawn fails (logs LogPaldarkPal Warning).
	APaldarkPalCharacter* SpawnFromWarmedDefinition(
		const UPaldarkPalDefinition& Definition,
		const FTransform& Transform);
};
