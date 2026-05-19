// PALDARK W40-41 — Pal Stable hub building.
//
// W40-41 scope: scaffold only. `OnInteract` logs a "TODO W44+ swap/heal/inspect"
// placeholder. W44+ wires real mechanics on top of the existing
// `UPaldarkPalRosterComponent` (W35-36) — swap captured Pal, heal injured Pal,
// inspect bond / capture / stun attributes per slot.
//
// Stamps `BuildingTag = Paldark.Hub.Building.PalStable` in its ctor so the hub
// subsystem's GetBuildingsByTag query can find it without relying on UClass.

#pragma once

#include "CoreMinimal.h"
#include "Hub/PaldarkHubBuilding.h"
#include "PaldarkPalStable.generated.h"

UCLASS()
class PALDARKLAB_API APaldarkPalStable : public APaldarkHubBuilding
{
	GENERATED_BODY()

public:
	APaldarkPalStable();

	virtual void OnInteract(APaldarkPlayerController* Interactor) override;
};
