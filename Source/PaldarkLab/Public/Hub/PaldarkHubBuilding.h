// PALDARK W40-41 — Designer-placeable hub building base actor.
//
// Base class for the three hub buildings shipping in W40-41 (Pal Stable,
// Marketplace, Briefing Room). Each subclass overrides `OnInteract` to wire
// per-type behaviour; the base owns shared scaffold:
//   - Soft ref to `UPaldarkHubBuildingDefinition` (designer-authored DA).
//   - Root `USphereComponent` sized to BuildingDefinition.InteractRadius.
//   - Server-side BeginPlay → register with `UPaldarkHubSubsystem`.
//   - Server-side EndPlay → unregister.
//
// Mirrors the W31-32 `APaldarkPointOfInterest` registration shape so the
// subsystem-race guard pattern (subsystem sweeps `TActorIterator` if the
// building's BeginPlay fired before Initialize) is identical.
//
// `bReplicates = false` — buildings are placed by designers and never
// destroyed mid-game in W40-41 scope; the hub subsystem is server-only so
// clients don't need a registry. UMG widgets reach into the building via the
// server-authoritative subsystem broadcast (W42-43 hook), not via replication.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "PaldarkHubBuilding.generated.h"

class APaldarkPlayerController;
class UPaldarkHubBuildingDefinition;
class USphereComponent;

UCLASS(Abstract, Blueprintable)
class PALDARKLAB_API APaldarkHubBuilding : public AActor
{
	GENERATED_BODY()

public:
	APaldarkHubBuilding();

	// Designer-authored data asset carrying the per-instance config
	// (BuildingTag, DisplayName, InteractRadius, per-type payload). Soft so
	// async-load doesn't pull the catalog / map allow-list at hub map load.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Hub")
	TSoftObjectPtr<UPaldarkHubBuildingDefinition> BuildingDefinition;

	// Per-subclass building type tag — stamped by the subclass ctor (e.g.
	// `Paldark.Hub.Building.PalStable` for `APaldarkPalStable`). The hub
	// subsystem keys its GetBuildingsByTag query on this so designer-renamed
	// `DA_HubBuilding_*` asset paths don't break console / future UMG code.
	// Subclasses MUST set this; the base asserts at BeginPlay if invalid.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Hub",
		meta = (Categories = "Paldark.Hub.Building"))
	FGameplayTag BuildingTag;

	// Virtual interact hook. Subclasses log a placeholder action (W44+ wires
	// real mechanics on top of the stable / marketplace; the briefing room is
	// the only one with real behaviour in W40-41 — it issues a `HostHubServer`
	// → `HostListenServer` ServerTravel after validating the pending raid map
	// against the building definition's allow-list).
	virtual void OnInteract(APaldarkPlayerController* Interactor);

	// Debug label used by `Paldark.Hub.List` console + log lines. Falls back
	// to BuildingDefinition->DisplayName / the BuildingTag leaf / actor name.
	FString GetDebugLabel() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Root sphere component, sized to BuildingDefinition->InteractRadius on
	// BeginPlay (designers can preview the radius in-editor at ctor default
	// size). Collision is set to NoCollision — the building is a designer
	// volume, not a physics body; player interact is done via a separate
	// sphere overlap probe in the player controller (W42+).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Hub")
	TObjectPtr<USphereComponent> InteractSphere = nullptr;
};
