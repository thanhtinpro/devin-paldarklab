// PALDARK W22-23 — Ping subsystem.
//
// `UWorldSubsystem` that owns the server-authoritative ping spawn path.
// Mirrors the W20-21 pack subsystem topology: clients call into the
// per-pawn `UPaldarkPlayerController::Server_RequestPing*` RPCs, the
// controller forwards to the subsystem, the subsystem spawns a replicated
// `APaldarkPingMarker`, and replication delivers the marker to every
// squadmate's client.
//
// Per-player rate limit:
//   `MinSecondsBetweenPings` (default 1.0) per player. Server tracks the
//   last ping wall-clock time per player; rejects spam without dropping
//   the connection (just a Verbose log line). Matches the [10] Udemy MP
//   Shooter pattern for ServerRequest spam.
//
// What this is NOT:
//   - No client-side prediction. Pings are rare + non-game-changing, so
//     200 ms perceived latency on the issuer is fine. Predicted markers
//     land W30+ with the rest of the UI polish.
//   - No squad-isolation filter. Single squad per match in W22-23. When
//     PvP teams land in W40+, the ping subsystem will start filtering by
//     issuer squad tag (delegate fan-out via the squad subsystem instead
//     of `bAlwaysRelevant=true` on the marker).

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "PaldarkPingSubsystem.generated.h"

class APaldarkCharacter;
class APaldarkPingMarker;

UCLASS()
class PALDARKLAB_API UPaldarkPingSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPaldarkPingSubsystem();

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Server-authoritative entrypoint called by `APaldarkPlayerController::Server_RequestPing`
	// after RPC validation. Spawns a replicated `APaldarkPingMarker` at
	// `WorldLocation` (or at `MarkedActor`'s location if non-null), stamps
	// `PingType` + `OwningPlayer`, and returns the freshly-spawned marker
	// (server only).
	//
	// Returns nullptr when:
	//   - Issuer is rate-limited.
	//   - Issuer is null / has no authority.
	//   - Marker class is unset.
	APaldarkPingMarker* SpawnPing(
		APaldarkCharacter* Issuer,
		const FVector& WorldLocation,
		AActor* MarkedActor,
		const FGameplayTag& PingType);

	// Designer override — Blueprint subclass of `APaldarkPingMarker` (e.g.
	// a Blueprint that adds a billboard sprite). Defaults to the C++ class.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Paldark|Ping")
	TSubclassOf<APaldarkPingMarker> MarkerClass;

	// Designer knob — minimum seconds between pings per player. 1.0 default.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Paldark|Ping", meta = (ClampMin = "0.0"))
	float MinSecondsBetweenPings = 1.0f;

	// Designer knob — default ping lifetime in seconds. Enemy pings can
	// extend their own lifetime by re-pinging the same actor (handled in
	// the player controller's mark-enemy path).
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Paldark|Ping", meta = (ClampMin = "0.1"))
	float DefaultPingLifetime = 3.0f;

	// Debug accessor used by `Paldark.Squad.Dump`.
	int32 GetActivePingCount() const;

private:
	// Per-player wall-clock timestamp of the last accepted ping. Older
	// entries cleaned up lazily during SpawnPing.
	UPROPERTY(Transient)
	TMap<TWeakObjectPtr<APaldarkCharacter>, float> LastPingTimePerPlayer;
};
