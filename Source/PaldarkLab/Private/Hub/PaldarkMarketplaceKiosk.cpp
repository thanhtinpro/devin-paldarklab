// PALDARK W40-41 / W44-45 — Marketplace kiosk hub building.

#include "Hub/PaldarkMarketplaceKiosk.h"

#include "Hub/PaldarkHubBuildingDefinition.h"
#include "Hub/PaldarkMarketplaceController.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"
#include "Player/PaldarkPlayerController.h"

APaldarkMarketplaceKiosk::APaldarkMarketplaceKiosk()
{
	BuildingTag = PaldarkGameplayTags::TAG_Paldark_Hub_Building_Marketplace;
}

void APaldarkMarketplaceKiosk::OnInteract(APaldarkPlayerController* Interactor)
{
	int32 CatalogSize = 0;
	if (const UPaldarkHubBuildingDefinition* Def = BuildingDefinition.Get())
	{
		CatalogSize = Def->MarketplaceCatalog.Num();
	}

	UE_LOG(LogPaldarkLab, Log,
		TEXT("[Hub.Marketplace %s] OnInteract by %s — CatalogSize=%d."),
		*GetDebugLabel(),
		Interactor != nullptr ? *Interactor->GetName() : TEXT("<null>"),
		CatalogSize);

	// W44-45 — Delegate to the per-player marketplace controller. The
	// controller kicks off an async catalog load via FStreamableManager,
	// flips the UI state tag, fires `OnMarketplaceOpened`, and the
	// designer-authored UMG widget mounts.
	if (Interactor)
	{
		if (UPaldarkMarketplaceController* Controller = Interactor->GetMarketplaceController())
		{
			Controller->RequestOpenMarketplace(this);
		}
		else
		{
			UE_LOG(LogPaldarkUI, Warning,
				TEXT("[Hub.Marketplace %s] interactor %s has no MarketplaceController — UI not mounted."),
				*GetDebugLabel(), *Interactor->GetName());
		}
	}
}
