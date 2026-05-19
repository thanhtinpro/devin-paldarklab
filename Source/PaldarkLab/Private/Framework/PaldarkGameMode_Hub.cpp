// PALDARK W40-41 — Hub town game mode implementation.

#include "Framework/PaldarkGameMode_Hub.h"

#include "Hub/PaldarkHubSubsystem.h"
#include "PaldarkLogCategories.h"

APaldarkGameMode_Hub::APaldarkGameMode_Hub()
{
	// W40-41 — 8-player shard default (roadmap Q4 § Tuần 40-41). The base
	// CDO ships 4 (the W14-15 raid default); the hub experience
	// (PX_HubTown) bumps this to 8 via the MaxPlayers field on
	// UPaldarkExperienceDefinition. Setting the CDO here means a hub-class
	// GameMode launched without an experience (e.g. PIE quick-test) still
	// honours the 8-player intent.
	MaxPlayersPerSession = 8;
}

void APaldarkGameMode_Hub::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	UPaldarkHubSubsystem* HubSubsystem = UPaldarkHubSubsystem::Get(World);
	const int32 BuildingCount = HubSubsystem != nullptr ? HubSubsystem->GetAllBuildings().Num() : -1;
	UE_LOG(LogPaldarkLab, Log,
		TEXT("[GameMode.Hub] BeginPlay — MaxPlayers=%d HubSubsystem=%s buildings=%d."),
		MaxPlayersPerSession,
		HubSubsystem != nullptr ? TEXT("present") : TEXT("missing"),
		BuildingCount);
}
