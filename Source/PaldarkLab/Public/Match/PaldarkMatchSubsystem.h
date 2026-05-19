// PALDARK W24-25 — Match subsystem.
//
// Server-authoritative `UWorldSubsystem` that drives the extraction flow:
//
//   1. Players register on PostLogin (`RegisterPlayer`).
//   2. GameMode flips phase Warmup -> Active via `StartMatch` once warmup
//      grace expires (or all expected players joined).
//   3. Extraction beacons call `RecordExtraction(PlayerState)` when their
//      ramp completes; subsystem flips that player's outcome to Extracted.
//   4. The AttributeSet death wiring calls `RecordDeath(PlayerState)` when
//      Health reaches 0; subsystem flips that player's outcome to KIA.
//   5. GameMode's Logout calls `RecordDisconnect(PlayerState)`.
//   6. After every state change, `EvaluateEndCondition` checks whether the
//      match should end (all players terminal, or a hard timeout). When
//      true, subsystem flips phase to Ended and broadcasts `OnMatchEnded`.
//
// Mirrors the W20-21 `UPaldarkHostilePackSubsystem` / W22-23
// `UPaldarkSquadSubsystem` pattern: server-only, broadcast delegates,
// per-key registry. The match subsystem differs in that it owns the FSM
// (not just a registry) — but ShouldCreateSubsystem still gates clients
// out so clients read the mirror replicated on `APaldarkGameStateBase`.
//
// What this is NOT (deferred):
//   - No actual lobby travel (`RestartGame`). GameMode currently soft-
//     restarts; W40+ hub town adds menu-level travel.
//   - No persistent scoreboard. End-of-match results live in the
//     subsystem for the rest of the world's lifetime and are wiped on
//     world teardown. W42-43 AWS persists to DynamoDB.
//   - No reconnect handling. Disconnected players cannot rejoin in the
//     same match; W14-15 follow-up adds reconnect.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Match/PaldarkMatchTypes.h"
#include "Subsystems/WorldSubsystem.h"

#include "PaldarkMatchSubsystem.generated.h"

class APaldarkExtractionBeacon;
class APaldarkGameStateBase;
class APaldarkPlayerState;
class APlayerState;

// Multicast delegate fired on the server every time the phase changes.
// `APaldarkGameStateBase` listens so it can replicate the new phase tag +
// `PhaseEndServerTime` to clients without a separate path.
DECLARE_MULTICAST_DELEGATE_OneParam(
	FPaldarkOnMatchPhaseChanged,
	EPaldarkMatchPhase /* NewPhase */);

// Multicast delegate fired on the server when the match transitions to
// Ended. Reason explains why; per-player outcome is on each PlayerState.
DECLARE_MULTICAST_DELEGATE_OneParam(
	FPaldarkOnMatchEnded,
	EPaldarkMatchEndReason /* Reason */);

// Multicast delegate fired on the server every time a player's outcome
// changes (Alive -> Extracted/KIA/Disconnected). `APaldarkGameStateBase`
// + HUD widgets bind to this to refresh per-player UI.
DECLARE_MULTICAST_DELEGATE_TwoParams(
	FPaldarkOnPlayerOutcomeChanged,
	APlayerState* /* PlayerState */,
	EPaldarkPlayerOutcome /* NewOutcome */);

UCLASS()
class PALDARKLAB_API UPaldarkMatchSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPaldarkMatchSubsystem();

	// UWorldSubsystem — server-only. Clients fall back to reading mirrored
	// state on APaldarkGameStateBase.
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Register/unregister hooks called by the GameMode (PostLogin / Logout).
	// `RegisterPlayer` is idempotent so PostLogin re-entry (rare) collapses.
	void RegisterPlayer(APlayerState* PlayerState);
	void UnregisterPlayer(APlayerState* PlayerState);

	// Flip phase Warmup -> Active. Called by the GameMode once warmup grace
	// expires or all expected players joined. Idempotent if already Active.
	void StartMatch();

	// Record per-player events. Each one re-evaluates the end condition.
	void RecordExtraction(APlayerState* PlayerState);
	void RecordDeath(APlayerState* PlayerState);
	void RecordDisconnect(APlayerState* PlayerState);

	// Force the match to end with the given reason. Used by the
	// `Paldark.Match.ForceEnd` console command and by the hard-timeout
	// watchdog (`MaxMatchDuration`).
	void EndMatch(EPaldarkMatchEndReason Reason);

	// Read-only accessors.
	EPaldarkMatchPhase GetPhase() const { return CurrentPhase; }
	EPaldarkMatchEndReason GetEndReason() const { return EndReason; }
	float GetPhaseEndServerTime() const { return PhaseEndServerTime; }
	int32 GetAlivePlayerCount() const;
	int32 GetExtractedPlayerCount() const;
	int32 GetKIAPlayerCount() const;
	int32 GetDisconnectedPlayerCount() const;
	int32 GetRegisteredPlayerCount() const { return PlayerRows.Num(); }

	// Read a player's outcome. Returns Alive if not registered, so callers
	// don't have to null-check.
	EPaldarkPlayerOutcome GetPlayerOutcome(APlayerState* PlayerState) const;

	// Dump phase + per-player table to LogPaldarkPlayer. Backend for
	// `Paldark.Match.Dump`.
	void DumpToLog() const;

	// Designer knobs.
	//
	// MaxMatchDuration — hard timer (seconds). If still in Active /
	// Extracting when this elapses since StartMatch was called, match ends
	// with Timeout. Set to <= 0 to disable.
	UPROPERTY(EditDefaultsOnly, Config, Category = "Paldark|Match")
	float MaxMatchDuration = 600.f;

	// WarmupDuration — seconds Warmup phase auto-advances after the first
	// player joins. Designer can override via the Extraction GameMode.
	UPROPERTY(EditDefaultsOnly, Config, Category = "Paldark|Match")
	float WarmupDuration = 30.f;

	// Delegates.
	FPaldarkOnMatchPhaseChanged    OnMatchPhaseChanged;
	FPaldarkOnMatchEnded           OnMatchEnded;
	FPaldarkOnPlayerOutcomeChanged OnPlayerOutcomeChanged;

private:
	// Single-source-of-truth phase setter. Updates timer, logs, fires
	// delegate. Idempotent.
	void SetPhase(EPaldarkMatchPhase NewPhase);

	// Re-check whether the match should end and call EndMatch if so. Reads
	// PlayerRows to count alive / extracted / kia / disconnected. Called
	// after every RecordExtraction / RecordDeath / RecordDisconnect.
	void EvaluateEndCondition();

	// Internal helper: mutate a player's outcome row + broadcast. Returns
	// true if the row actually flipped (false on no-op / unknown player).
	bool SetPlayerOutcomeInternal(APlayerState* PlayerState, EPaldarkPlayerOutcome NewOutcome);

	// Mirror phase + reason onto APaldarkGameStateBase so clients see them.
	void MirrorToGameState();

	// World-timer callbacks.
	void OnWarmupTimerExpired();
	void OnMatchHardTimeoutExpired();

	UPROPERTY(Transient)
	EPaldarkMatchPhase CurrentPhase = EPaldarkMatchPhase::Warmup;

	UPROPERTY(Transient)
	EPaldarkMatchEndReason EndReason = EPaldarkMatchEndReason::None;

	// Seconds (server time) at which the current phase's timer expires.
	// 0.f when no timer is in flight. Mirrored to GameState for clients.
	UPROPERTY(Transient)
	float PhaseEndServerTime = 0.f;

	// Server time when StartMatch flipped phase to Active. Used by the
	// hard-timeout check.
	UPROPERTY(Transient)
	float MatchStartedAtServerTime = 0.f;

	// Per-player rows. Keyed by raw PlayerState pointer because TWeakObjectPtr
	// isn't TMap-key friendly; the per-row TWeakObjectPtr keeps the safe ref.
	// All access goes through SetPlayerOutcomeInternal / iteration helpers
	// that filter stale rows.
	UPROPERTY(Transient)
	TMap<TObjectPtr<APlayerState>, FPaldarkMatchPlayerRow> PlayerRows;

	// World-timer handles so Deinitialize can clear them.
	FTimerHandle WarmupTimerHandle;
	FTimerHandle HardTimeoutTimerHandle;
};
