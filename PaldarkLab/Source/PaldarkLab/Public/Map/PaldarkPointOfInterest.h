// PALDARK W31-32 — Designer-placeable Point of Interest actor.
//
// Drop an `APaldarkPointOfInterest` into a level, set its `POITag` (one of the
// `Paldark.POI.Type.*` tags), `DangerTier` enum, optional `AllowedSpeciesTags`,
// and `DisplayName`. On `BeginPlay` (server only) the actor registers itself
// with `UPaldarkPOISubsystem`; on `EndPlay` it unregisters.
//
// Roadmap Q3 § Tuần 31-32 deliverable — "POI placement" line item. The actor
// itself stays metadata-only: no AI logic, no spawn fan-out, no replication.
// Spawning Pals near a POI is the spawn-point actor's job
// (`APaldarkPalSpawnPoint`); querying which POIs exist is the subsystem's job
// (`UPaldarkPOISubsystem::GetPOIsByTag` / `GetNearestPOI`).
//
// Editor-only debug:
//   - The root `USphereComponent` (radius = `RegistrationRadius`) is set to
//     `bHiddenInGame = true` so designers see the registration radius gizmo
//     in-editor without it leaking into shipping builds.
//
// What this is NOT (deferred):
//   - No "POI ownership" — multiple players don't claim a POI competitively.
//     Hub-town claim system lands W40-41.
//   - No "POI events" multicast delegate. The subsystem provides query-only
//     access; consumers poll on enter/leave via overlap if needed. Event
//     broadcast (PlayerEnteredPOI / Cleared) lands Q4 polish.
//   - No replication. POI placement is server-authored level data; clients
//     have no need for the registry (HUD breadcrumb tooling reads from the
//     server-side replicated player state instead).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "PaldarkPointOfInterest.generated.h"

class USphereComponent;

// Coarse danger bin for an encounter. Three tiers so the AI Director (Q4)
// can plan spawn budget without overfitting to per-POI numbers.
UENUM(BlueprintType)
enum class EPaldarkPOIDangerTier : uint8
{
	Low      UMETA(DisplayName = "Low"),
	Medium   UMETA(DisplayName = "Medium"),
	High     UMETA(DisplayName = "High"),
};

UCLASS(ClassGroup = (Paldark), meta = (DisplayName = "Paldark Point of Interest"))
class PALDARKLAB_API APaldarkPointOfInterest : public AActor
{
	GENERATED_BODY()

public:
	APaldarkPointOfInterest();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Designer knobs ----------------------------------------------------

	// POI type. Must be one of `Paldark.POI.Type.*` (Outpost / Cache /
	// RuinedTower / MiniBoss / ExtractionPad / SafeZone). Leaving this
	// invalid means BeginPlay logs a Warning and skips registration —
	// the actor still ticks normally but the subsystem ignores it.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|POI",
		meta = (Categories = "Paldark.POI.Type"))
	FGameplayTag POITag;

	// Human-readable label shown in `Paldark.POI.Dump` + future hub-town
	// breadcrumb UI. Falls back to actor label when empty.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|POI")
	FText DisplayName;

	// Danger tier (Low / Medium / High). Used by the AI Director (Q4) to
	// budget spawns. `Paldark.POI.HighlightTier <Tier>` console command
	// also uses this to filter the debug-draw set.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|POI")
	EPaldarkPOIDangerTier DangerTier = EPaldarkPOIDangerTier::Low;

	// Pal species expected to spawn near this POI. Designer hint only —
	// the spawn-point actor reads this from its `AssociatedPOITag` lookup
	// to verify "the Razorbird spawn point I dropped at the RuinedTower
	// matches what the POI says it allows". Empty = no restriction.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|POI",
		meta = (Categories = "Paldark.Pal.Species"))
	FGameplayTagContainer AllowedSpeciesTags;

	// Radius (cm) of the editor debug sphere shown in-editor. Also used
	// by `GetNearestPOI` queries as the POI's "characteristic size" when
	// designers don't supply a radius to the query.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|POI",
		meta = (ClampMin = "0.0"))
	float RegistrationRadius = 1500.f;

	// Read-only accessors ----------------------------------------------

	// Resolve the tag that represents this POI's danger tier
	// (`Paldark.POI.DangerTier.*`). Used by the subsystem for tier-keyed
	// queries + the highlight console command. Centralised here so the
	// enum → tag mapping lives in one place.
	FGameplayTag GetDangerTierTag() const;

	// Debug label used by the console dump command. Returns `DisplayName`
	// when set, otherwise the POI tag's leaf name, otherwise the actor's
	// label / name.
	FString GetDebugLabel() const;

protected:
	// Editor-only debug sphere. Hidden in game.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|POI",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> DebugSphere;
};
