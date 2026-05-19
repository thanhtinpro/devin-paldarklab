// PALDARK W40-41 — Briefing room hub building.
//
// The only hub building with real behaviour in W40-41 scope. `OnInteract`:
//   1. Reads `PendingRaidMapTag` from the hub subsystem (set by designer
//      via `Paldark.Hub.QueueRaid <MapTag>` console command).
//   2. Validates the tag is one of `BuildingDefinition->AllowedRaidMaps` (or
//      passes through if AllowedRaidMaps is empty — designer test maps).
//   3. Issues `UPaldarkNetSubsystem::HostHubServer(<Map>, <Experience>)` to
//      ServerTravel the entire hub shard into the raid map. Deferred to the
//      next tick via FTimerManager so the interact processing finishes
//      before the world tears down.
//
// Stamps `BuildingTag = Paldark.Hub.Building.BriefingRoom` in its ctor so the
// hub subsystem's GetBuildingsByTag query can find it without UClass.

#pragma once

#include "CoreMinimal.h"
#include "Hub/PaldarkHubBuilding.h"
#include "PaldarkBriefingRoom.generated.h"

UCLASS()
class PALDARKLAB_API APaldarkBriefingRoom : public APaldarkHubBuilding
{
	GENERATED_BODY()

public:
	APaldarkBriefingRoom();

	virtual void OnInteract(APaldarkPlayerController* Interactor) override;

protected:
	// Deferred ServerTravel — `HostHubServer` mid-frame during interact
	// processing is risky (the world tears down while UMG / GameMode is
	// mid-call). The interact path schedules this on the next tick via
	// `FTimerManager::SetTimerForNextTick`.
	void IssueDeferredTravel(FString MapName, FString ExperienceId);
};
