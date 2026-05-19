// PALDARK W31-32 — Point-of-Interest subsystem (UWorldSubsystem, server-only).
//
// Registry + query layer for `APaldarkPointOfInterest` actors. Designers drop
// POI actors into a level (W31-32 § Map 2 "Rừng Hỏng" blockout); each actor
// calls `RegisterPOI` on BeginPlay and `UnregisterPOI` on EndPlay. The
// subsystem maintains:
//
//   - `AllPOIs` — flat list of every currently-registered POI.
//   - `POIsByTag` — `Paldark.POI.Type.*` → array of registered POIs.
//   - `POIsByDangerTier` — `EPaldarkPOIDangerTier` → array of POIs at that
//     danger tier.
//
// Query API:
//   - `GetAllPOIs()` — full list (sorted by actor label for deterministic
//     console dump output).
//   - `GetPOIsByTag(Tag)` — exact-tag match (no parent-tag matching).
//   - `GetPOIsByDangerTier(Tier)`.
//   - `GetNearestPOI(Location, MaxRadius)` — O(N) sweep with optional radius
//     filter. Returns nullptr if no POI is within MaxRadius.
//
// Registry race handling (per W31-32 plan):
//   - `Initialize` sweeps `TActorIterator<APaldarkPointOfInterest>` to pick
//     up POIs that already finished BeginPlay before the subsystem started.
//   - POI BeginPlay tries to register immediately; if the subsystem is not
//     yet initialized (`GetSubsystem` returns null), the POI swallows the
//     register call — the subsystem's Initialize sweep will pick it up.
//   - The subsystem also subscribes to `FWorldDelegates::OnWorldBeginPlay`
//     and runs a second sweep there as a belt-and-suspenders guard.
//
// Server-only. `ShouldCreateSubsystem` returns false on dedicated clients +
// PIE clients so the registry stays a single source of truth on the server.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "PaldarkPOISubsystem.generated.h"

class APaldarkPointOfInterest;
enum class EPaldarkPOIDangerTier : uint8;

UCLASS()
class PALDARKLAB_API UPaldarkPOISubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPaldarkPOISubsystem();

	// UWorldSubsystem — only initialise on hosts. POI registry is
	// server-authoritative.
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	// Convenience accessor — get-or-null for the subsystem on `World`.
	static UPaldarkPOISubsystem* Get(const UWorld* World);

	// Register/unregister --------------------------------------------------

	// Called from `APaldarkPointOfInterest::BeginPlay`. Idempotent — a
	// double-register collapses to one entry. Safe to call before
	// Initialize finishes (the call is dropped; Initialize's sweep will
	// pick the POI up).
	void RegisterPOI(APaldarkPointOfInterest* InPOI);

	// Called from `APaldarkPointOfInterest::EndPlay`. Safe to call on a
	// POI that was never successfully registered (no-op in that case).
	void UnregisterPOI(APaldarkPointOfInterest* InPOI);

	// Query API ------------------------------------------------------------

	// Full list of currently-registered POIs. Order is by actor label for
	// deterministic console output. Filters out stale weak refs.
	TArray<APaldarkPointOfInterest*> GetAllPOIs() const;

	// Returns POIs whose `POITag` exactly matches `InTag`. No parent-tag
	// matching — designers either know the exact type or use
	// `GetAllPOIs()` and filter manually.
	TArray<APaldarkPointOfInterest*> GetPOIsByTag(const FGameplayTag& InTag) const;

	// Returns POIs at the given danger tier.
	TArray<APaldarkPointOfInterest*> GetPOIsByDangerTier(EPaldarkPOIDangerTier InTier) const;

	// O(N) linear search for the nearest registered POI within `MaxRadius`
	// (cm) of `InLocation`. Pass `MaxRadius <= 0` to disable the radius
	// filter. Returns nullptr if no POI matches.
	APaldarkPointOfInterest* GetNearestPOI(const FVector& InLocation, float MaxRadius = 0.f) const;

	// Dump full registry to LogPaldarkLab. Used by `Paldark.POI.Dump`.
	void DumpToLog() const;

private:
	// Strong refs so a POI being destroyed mid-frame doesn't fault on
	// access; UnregisterPOI removes the entry on EndPlay.
	UPROPERTY(Transient)
	TArray<TObjectPtr<APaldarkPointOfInterest>> RegisteredPOIs;

	// Initialize sweep — iterates `TActorIterator<APaldarkPointOfInterest>`
	// and registers any POI not already tracked. Idempotent.
	void SweepWorldForPOIs();
};
