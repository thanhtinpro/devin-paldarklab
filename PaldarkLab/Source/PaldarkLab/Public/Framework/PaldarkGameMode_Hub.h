// PALDARK W40-41 — Hub town game mode.
//
// `APaldarkGameModeBase` subclass for hub town shards (the lobby map between
// raids). Owns the 8-player shard contract:
//   - Default `MaxPlayersPerSession = 8` (vs 4 for the W14-15 raid default).
//   - Pulls `MaxPlayers` from the experience definition the same way the base
//     class does — hub experiences (PX_HubTown) set MaxPlayers = 8 in their
//     PrimaryDataAsset so the cap survives a designer-side rewire.
//
// Does NOT subclass `APaldarkGameMode_Extraction` — hubs have no match
// lifecycle, no extraction beacons, no scoreboard. The briefing-room actor
// handles handoff via `UPaldarkNetSubsystem::HostHubServer` (ServerTravel
// the entire hub shard into the raid map).
//
// Class registration (mirrors W26 L-01 path):
//   1. Map World Override → GameMode = APaldarkGameMode_Hub.
//   2. URL flag: `open Map_HubTown?game=Hub?Experience=...` (uses the
//      `+GameModeClassAliases=(Name="Hub",...)` entry in DefaultEngine.ini).
//   3. Authoring the hub experience: set
//      `UPaldarkExperienceDefinition::RequiredGameModeClass =
//      APaldarkGameMode_Hub`. Base GameMode asserts at experience-ready
//      time and logs Error + red on-screen warning if the spawned GameMode
//      is not a subclass.

#pragma once

#include "CoreMinimal.h"
#include "Framework/PaldarkGameModeBase.h"
#include "PaldarkGameMode_Hub.generated.h"

UCLASS(Config = Game)
class PALDARKLAB_API APaldarkGameMode_Hub : public APaldarkGameModeBase
{
	GENERATED_BODY()

public:
	APaldarkGameMode_Hub();

protected:
	virtual void BeginPlay() override;
};
