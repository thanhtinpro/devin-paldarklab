// PALDARK W40-41 — Hub town subsystem.
//
// `UWorldSubsystem` that owns the server-side registry of hub buildings
// (Pal Stable, Marketplace, Briefing Room) placed in the hub map. Mirrors
// the W31-32 `UPaldarkPOISubsystem` shape — server-only, sweep-on-init,
// race-guard via TActorIterator + OnWorldBeginPlay retry.
//
// Three console commands registered at Initialize / unregistered at
// Deinitialize:
//   - `Paldark.Hub.List` — dump every registered hub building.
//   - `Paldark.Hub.QueueRaid <MapTag>` — set the pending raid map tag; the
//     briefing room reads this on OnInteract to issue the ServerTravel.
//   - `Paldark.Hub.Status` — dump subsystem state (registered count,
//     pending raid tag, current GameMode MaxPlayers cap).
//
// `PendingRaidMapTag` is replicated implicitly via the briefing room's
// per-interact branching — the subsystem itself stays server-only, no
// REPNOTIFY. Future UMG widgets pull state through a server RPC + replicated
// player state in W42-43 backend work.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "PaldarkHubSubsystem.generated.h"

class APaldarkHubBuilding;
class IConsoleObject;

UCLASS()
class PALDARKLAB_API UPaldarkHubSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPaldarkHubSubsystem();

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	// Convenience accessor — equivalent to
	// `World->GetSubsystem<UPaldarkHubSubsystem>()` but with a null-guard so
	// callers don't have to repeat the `World != nullptr` check.
	static UPaldarkHubSubsystem* Get(const UWorld* World);

	// Idempotent register / unregister. Called by `APaldarkHubBuilding::BeginPlay`
	// / `EndPlay` on authority; the Initialize sweep + OnWorldBeginPlay retry
	// sweep cover the subsystem-creation race window (building BeginPlay
	// fires before Initialize).
	void RegisterHubBuilding(APaldarkHubBuilding* InBuilding);
	void UnregisterHubBuilding(APaldarkHubBuilding* InBuilding);

	// Query helpers. Sorted-by-name for deterministic console output.
	TArray<APaldarkHubBuilding*> GetAllBuildings() const;
	TArray<APaldarkHubBuilding*> GetBuildingsByTag(const FGameplayTag& InTag) const;
	APaldarkHubBuilding* GetNearestBuilding(const FVector& InLocation, float MaxRadius = 0.0f) const;

	// Pending raid map tag — set by `Paldark.Hub.QueueRaid <MapTag>` console
	// command, read by `APaldarkBriefingRoom::OnInteract` to decide which map
	// to ServerTravel into. Invalid tag = "no raid queued".
	const FGameplayTag& GetPendingRaidMapTag() const { return PendingRaidMapTag; }
	void SetPendingRaidMapTag(const FGameplayTag& InTag);

	// Dump every registered building to LogPaldarkLab + the screen for
	// designer debugging via the `Paldark.Hub.List` console command.
	void DumpToLog() const;

private:
	// Sweep the world for any APaldarkHubBuilding placed in the level that
	// hasn't registered itself yet (race-guard for buildings whose BeginPlay
	// fired before subsystem Initialize). Idempotent — re-adding a known
	// building is a no-op.
	void SweepWorldForBuildings();

	// Console handlers — registered in Initialize, unregistered in
	// Deinitialize. Names mirror the W31-32 / W14-15 / W22-23 cmd shape.
	void HandleConsoleList(const TArray<FString>& Args);
	void HandleConsoleQueueRaid(const TArray<FString>& Args);
	void HandleConsoleStatus(const TArray<FString>& Args);

	// Set of registered building actors. TObjectPtr so we can iterate
	// without WeakPtr unwrapping; nulled entries are skipped in queries.
	UPROPERTY()
	TSet<TObjectPtr<APaldarkHubBuilding>> RegisteredBuildings;

	// Pending raid map tag (server-only). Invalid until `Paldark.Hub.QueueRaid`
	// fires. Briefing-room interact validates the tag against its
	// `AllowedRaidMaps` allow-list before ServerTravelling.
	FGameplayTag PendingRaidMapTag;

	// Console command handles — owned by IConsoleManager but unregistered in
	// Deinitialize so reloading the subsystem doesn't double-register.
	IConsoleObject* CmdList = nullptr;
	IConsoleObject* CmdQueueRaid = nullptr;
	IConsoleObject* CmdStatus = nullptr;
};
