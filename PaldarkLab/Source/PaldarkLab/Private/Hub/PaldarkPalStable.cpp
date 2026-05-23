// PALDARK W40-41 / W44-45 — Pal Stable hub building.

#include "Hub/PaldarkPalStable.h"

#include "Hub/PaldarkHubBuildingDefinition.h"
#include "Hub/PaldarkStableController.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"
#include "Player/PaldarkPlayerController.h"

APaldarkPalStable::APaldarkPalStable()
{
	BuildingTag = PaldarkGameplayTags::TAG_Paldark_Hub_Building_PalStable;
}

void APaldarkPalStable::OnInteract(APaldarkPlayerController* Interactor)
{
	int32 SlotCount = 0;
	if (const UPaldarkHubBuildingDefinition* Def = BuildingDefinition.Get())
	{
		SlotCount = Def->RosterSlotCount;
	}

	UE_LOG(LogPaldarkLab, Log,
		TEXT("[Hub.PalStable %s] OnInteract by %s — RosterSlotCount=%d."),
		*GetDebugLabel(),
		Interactor != nullptr ? *Interactor->GetName() : TEXT("<null>"),
		SlotCount);

	// W44-45 — Delegate to the per-player stable controller component on
	// the PlayerController. The controller flips the UI state tag, fires
	// `OnStableOpened`, and the designer-authored UMG widget mounts.
	if (Interactor)
	{
		if (UPaldarkStableController* Controller = Interactor->GetStableController())
		{
			Controller->RequestOpenStable(this);
		}
		else
		{
			UE_LOG(LogPaldarkUI, Warning,
				TEXT("[Hub.PalStable %s] interactor %s has no StableController — UI not mounted."),
				*GetDebugLabel(), *Interactor->GetName());
		}
	}
}
