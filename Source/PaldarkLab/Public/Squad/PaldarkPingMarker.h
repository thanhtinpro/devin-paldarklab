// PALDARK W22-23 — Ping marker actor (replicated world placeholder).
//
// Tiny `AActor` spawned by the `UPaldarkPingSubsystem` when a squad member
// fires a Server_RequestPing. Default lifetime 3 seconds; auto-destroys
// itself on the server, which destroys the replicated actor on every
// client. Enemy pings additionally cache the marked actor and re-position
// every tick so the marker tracks moving targets.
//
// The actor is intentionally minimal — a `USceneComponent` root + a few
// replicated properties (PingType, Lifetime, OwningPlayer). The HUD widget
// (W22+ UMG task) reads these properties from a per-pawn Actor iterator to
// render the icon + label. No mesh / particle / sound is added here so the
// PR stays C++ only.
//
// Why server-spawn (not multicast RPC):
//   - Replicated actor gives clients automatic late-join correctness — a
//     player who joins mid-match still sees pings that are still alive,
//     without a special replay channel.
//   - Lifetime is server-authoritative; clients can't fake "ping forever".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "PaldarkPingMarker.generated.h"

class APaldarkCharacter;

UCLASS()
class PALDARKLAB_API APaldarkPingMarker : public AActor
{
	GENERATED_BODY()

public:
	APaldarkPingMarker();

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	// Set by the ping subsystem immediately after spawn (server-side). The
	// values flow to clients via replication.
	void InitMarker(
		const FGameplayTag& InPingType,
		APaldarkCharacter* InOwningPlayer,
		AActor* InMarkedActor,
		float InLifetime);

	// HUD accessors. Reading from a non-authority pawn is safe — these
	// properties replicate.
	UFUNCTION(BlueprintPure, Category = "Paldark|Ping")
	FGameplayTag GetPingType() const { return PingType; }

	UFUNCTION(BlueprintPure, Category = "Paldark|Ping")
	APaldarkCharacter* GetOwningPlayer() const { return OwningPlayer.Get(); }

	UFUNCTION(BlueprintPure, Category = "Paldark|Ping")
	AActor* GetMarkedActor() const { return MarkedActor.Get(); }

	UFUNCTION(BlueprintPure, Category = "Paldark|Ping")
	float GetRemainingLifetime() const;

protected:
	// Replicated type tag (one of `Paldark.Ping.Type.*`). Drives the HUD
	// widget's icon selection.
	UPROPERTY(Replicated)
	FGameplayTag PingType;

	// Replicated weak ref to the player who fired the ping. HUD uses this
	// to colour-code by squad role + render the player's name.
	UPROPERTY(Replicated)
	TWeakObjectPtr<APaldarkCharacter> OwningPlayer;

	// Replicated weak ref to the marked actor for Enemy pings. Null for
	// generic Spot/Loot/Danger pings. When non-null + still alive, the
	// marker re-snaps its location to the marked actor every tick.
	UPROPERTY(Replicated)
	TWeakObjectPtr<AActor> MarkedActor;

	// Server-only lifetime tracking. Replicated so HUD bars can show a
	// fade timer on the client. World time of expiration.
	UPROPERTY(Replicated)
	float ExpirationServerTime = 0.f;
};
