// PALDARK W1 day 8-10 / 11-14 + W7-8 — Player controller.
//
// Owns the slot the experience system points at, plus the Enhanced Input
// mapping-context plumbing (W1 day 11-14). On possess the controller:
//   1. Resolves the experience's PawnData (via the active game state),
//   2. Hands the PawnData to the character so SetupPlayerInputComponent can
//      pick up the tag-keyed UPaldarkInputConfig,
//   3. Pushes every IMC in `DefaultMappingContexts` into the local player's
//      UEnhancedInputLocalPlayerSubsystem with the configured priority,
//   4. W7-8: on the server, grants every `UPaldarkGameplayAbility` listed in
//      `PawnData.GrantedAbilities` and applies every GE in
//      `PawnData.StartupEffects` to the PlayerState's ASC.
//
// Future homes (do NOT implement here in W1):
//   - W18+ Backend: telemetry events on login / logout.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "PaldarkPlayerController.generated.h"

class UPaldarkBriefingController;
class UPaldarkMarketplaceController;
class UPaldarkPawnData;
class UPaldarkStableController;

UCLASS()
class PALDARKLAB_API APaldarkPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	APaldarkPlayerController();

	// Returns the PawnData this controller resolved from the active experience,
	// or null before possession completes. Read-only for debug / console.
	const UPaldarkPawnData* GetActivePawnData() const { return ActivePawnData; }

	// W44-45 — Per-player UI controllers for the hub buildings. The
	// `APaldarkPalStable` / `APaldarkMarketplaceKiosk` actors call into
	// these components in their `OnInteract` paths to drive the UMG flow
	// (server RPCs + client delegates + UI state tag).
	UFUNCTION(BlueprintPure, Category = "Paldark|Hub|Stable")
	UPaldarkStableController* GetStableController() const { return StableController; }

	UFUNCTION(BlueprintPure, Category = "Paldark|Hub|Marketplace")
	UPaldarkMarketplaceController* GetMarketplaceController() const { return MarketplaceController; }

	// W46 — Briefing room UI controller. Same default-subobject pattern
	// as Stable/Marketplace so designers don't need to wire it by hand.
	UFUNCTION(BlueprintPure, Category = "Paldark|Hub|Briefing")
	UPaldarkBriefingController* GetBriefingController() const { return BriefingController; }

	// W16-17 — Returns the current server-time estimate on the client (or the
	// canonical world time on the server). Used by the lag compensation flow:
	// the predicted shooter samples this at the instant of fire, the value
	// travels through the ServerScoreRequest RPC, and the server rewinds its
	// FrameHistory to the matching time.
	//
	// Server: returns GetWorld()->GetTimeSeconds() directly.
	// Client: returns GetWorld()->GetTimeSeconds() + ClientServerDelta, where
	// ClientServerDelta is computed off the ServerRequest/ClientReport ping
	// handshake (see ServerRequestServerTime / ClientReportServerTime below).
	UFUNCTION(BlueprintCallable, Category = "Paldark|Network")
	float GetServerTime() const;

	// W16-17 — Most recent single-trip RTT/2 estimate in seconds. The predicted
	// shooter subtracts this from GetServerTime() to back-date its HitTime so
	// the server rewinds to the moment the trigger was pulled, not the
	// moment the score request arrived.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Network")
	float GetSingleTripTime() const { return SingleTripTime; }

	// W22-23 — Server-authoritative ping request. Called by the autonomous
	// client (T key → "drop ping at where I am looking") OR by the
	// `Paldark.Squad.Ping` console command on a non-authority client. The
	// server resolves to a `UPaldarkPingSubsystem::SpawnPing` which
	// creates a replicated `APaldarkPingMarker`.
	UFUNCTION(Server, Reliable)
	void Server_RequestPing(FVector WorldLocation, FGameplayTag PingType);

	// W22-23 — Mark-enemy variant. Client-side line trace from the camera
	// (up to `MarkEnemyTraceDistance` cm) finds the actor under the
	// crosshair; the server re-traces (anti-cheat) and stamps the marker
	// with the marked actor. If the trace misses or hits a non-hostile
	// actor, the server falls back to a Spot ping at the impact point.
	UFUNCTION(Server, Reliable)
	void Server_RequestMarkUnderCrosshair();

	// Designer knob — trace distance for the mark-enemy variant. Default
	// 5000 cm (~50 m) covers the W22-23 sandbox scale; raised in W30+ when
	// the levels grow.
	UPROPERTY(EditDefaultsOnly, Category = "Paldark|Squad", meta = (ClampMin = "100.0"))
	float MarkEnemyTraceDistance = 5000.f;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void ReceivedPlayer() override;
	virtual void PlayerTick(float DeltaTime) override;

	// W16-17 — Time-sync handshake. Mirrors the [10] Udemy MP Shooter
	// (Blaster) pattern: client sends ClientTime via ServerRequestServerTime,
	// server stamps its receive time and ships ClientReportServerTime back,
	// client updates ClientServerDelta. Repeated on a slow cadence
	// (TimeSyncFrequency) so clock drift over a long match stays bounded.
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	// W16-17 — Re-issue ServerRequestServerTime if at least TimeSyncFrequency
	// seconds elapsed since the last sync. Called once a frame from
	// PlayerTick on the autonomous client.
	void CheckTimeSync(float DeltaTime);

private:
	// Resolves the PawnData soft pointer off the current experience and loads it
	// synchronously. Synchronous load is fine in W1 because the game mode
	// already loaded the experience before any pawn spawns — async polish is a
	// P14 follow-up.
	const UPaldarkPawnData* ResolvePawnDataForCurrentExperience() const;

	// Adds every IMC row in `DefaultMappingContexts` to the local player's
	// UEnhancedInputLocalPlayerSubsystem. Logs a warning if no subsystem is
	// found (e.g. running on a dedicated-server controller).
	void ApplyMappingContexts(const UPaldarkPawnData* PawnData);

	// Symmetric removal — runs on OnUnPossess so contexts don't pile up between
	// possess cycles (W2+ when we have remote spectator → pawn re-possession).
	void ClearMappingContexts(const UPaldarkPawnData* PawnData);

	// W7-8 — Server-only. Grants every ability + applies every startup effect
	// from `PawnData` onto the PlayerState's ASC. Idempotent because the ASC
	// tracks the grants and silently rejects duplicates.
	void GrantGasFromPawnData(APawn* InPawn, const UPaldarkPawnData* PawnData);

	// PawnData cached for the lifetime of the current possession. Cleared on
	// OnUnPossess so spectators see a fresh resolve.
	UPROPERTY(Transient)
	TObjectPtr<const UPaldarkPawnData> ActivePawnData;

	// W44-45 — Hub UI controllers. Created as default subobjects so every
	// player ships with both — designer doesn't need to attach them by
	// hand. Lives on the PlayerController (not the Pawn) so the UI state
	// survives respawn.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Hub", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkStableController> StableController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Hub", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkMarketplaceController> MarketplaceController;

	// W46 — Briefing controller default subobject.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Hub", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkBriefingController> BriefingController;

	// W16-17 — Time sync state. ClientServerDelta is added to local world
	// time to yield server time; SingleTripTime is roughly RTT/2 and is
	// subtracted by the shooter to back-date its HitTime; TimeSinceLastSync
	// counts up so PlayerTick can re-issue the request every
	// TimeSyncFrequency seconds.
	float ClientServerDelta   = 0.f;
	float SingleTripTime      = 0.f;
	float TimeSinceLastSync   = 0.f;

	// How often (in seconds) to re-sync the clock. 5 s default — frequent
	// enough that drift stays under 1 ms RTT delta in practice, infrequent
	// enough that the reliable RPC channel doesn't pay a meaningful cost.
	UPROPERTY(EditDefaultsOnly, Category = "Paldark|Network")
	float TimeSyncFrequency = 5.f;
};
