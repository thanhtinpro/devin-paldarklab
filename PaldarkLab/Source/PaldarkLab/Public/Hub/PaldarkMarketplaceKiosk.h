// PALDARK W40-41 — Marketplace kiosk hub building.
//
// W40-41 scope: scaffold only. `OnInteract` logs the loaded catalog size
// (count of `UPaldarkItemDefinition` soft refs the kiosk advertises) and
// returns. W45+ wires real buy/sell mechanics on top of the existing
// `UPaldarkPlayerInventoryComponent` (W11-12 / W37-38) — read
// `Paldark.Item.Resource.Currency.Credits` from the player's inventory,
// debit on purchase, credit on sale.
//
// Stamps `BuildingTag = Paldark.Hub.Building.Marketplace` in its ctor so the
// hub subsystem's GetBuildingsByTag query can find it without UClass.

#pragma once

#include "CoreMinimal.h"
#include "Hub/PaldarkHubBuilding.h"
#include "PaldarkMarketplaceKiosk.generated.h"

UCLASS()
class PALDARKLAB_API APaldarkMarketplaceKiosk : public APaldarkHubBuilding
{
	GENERATED_BODY()

public:
	APaldarkMarketplaceKiosk();

	virtual void OnInteract(APaldarkPlayerController* Interactor) override;
};
