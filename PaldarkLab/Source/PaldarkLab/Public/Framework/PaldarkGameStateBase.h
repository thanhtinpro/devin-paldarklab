// PALDARK W1 day 6-7 — Game state base.
//
// Holds the currently-loaded experience id and replicates it to clients via
// RepNotify. Clients use `OnExperienceLoaded` to react (e.g. apply IMC, swap
// HUD, register input). This is the same shape Lyra uses in
// `ULyraExperienceManagerComponent` but flattened onto the game state for W1.
// Once we add component-based managers (W3+), this responsibility moves to a
// `UGameStateComponent`.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Match/PaldarkMatchTypes.h"
#include "PaldarkGameStateBase.generated.h"

class UPaldarkBriefingSessionComponent;
class UPaldarkExperienceDefinition;

DECLARE_MULTICAST_DELEGATE_OneParam(FPaldarkOnExperienceLoaded, const UPaldarkExperienceDefinition* /*Experience*/);

// W24-25 — Multicast delegate fired on every machine when the replicated
// MatchPhase changes. HUD widgets bind to swap the active screen; AI
// activities bind to gate behaviour (e.g. hostile-Pal patrol disables
// during Ended).
DECLARE_MULTICAST_DELEGATE_OneParam(
	FPaldarkOnMatchPhaseReplicated,
	EPaldarkMatchPhase /* NewPhase */);

UCLASS()
class PALDARKLAB_API APaldarkGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	APaldarkGameStateBase();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	// Server-only. Called by the GameMode once the experience finished loading.
	void SetCurrentExperience(const FPrimaryAssetId& AssetId, const UPaldarkExperienceDefinition* Experience);

	const FPrimaryAssetId& GetCurrentExperienceId() const { return CurrentExperienceId; }
	const UPaldarkExperienceDefinition* GetCurrentExperience() const { return CurrentExperience; }

	// Fires on both server and client whenever a new experience finishes loading.
	// On server: invoked synchronously from SetCurrentExperience.
	// On client: invoked from OnRep_CurrentExperience after the soft pointer resolves.
	FPaldarkOnExperienceLoaded OnExperienceLoaded;

	// W24-25 — Read-only accessors for the replicated match phase fields.
	// The server-side `UPaldarkMatchSubsystem` is the source of truth;
	// clients read these mirrors.
	UFUNCTION(BlueprintPure, Category = "Paldark|Match")
	EPaldarkMatchPhase GetMatchPhase() const { return MatchPhase; }

	UFUNCTION(BlueprintPure, Category = "Paldark|Match")
	EPaldarkMatchEndReason GetMatchEndReason() const { return MatchEndReason; }

	// Server time (seconds, GetWorld()->GetTimeSeconds) at which the current
	// phase's timer expires. Zero when no timer is in flight.
	UFUNCTION(BlueprintPure, Category = "Paldark|Match")
	float GetPhaseEndServerTime() const { return PhaseEndServerTime; }

	// Server-only. Called by `UPaldarkMatchSubsystem::MirrorToGameState`
	// every time phase / end-reason / phase-end-time change. Sets the
	// replicated props + fires OnRep manually so observers see the same
	// ordering as remote clients.
	void SetMatchPhaseFromSubsystem(
		EPaldarkMatchPhase NewPhase,
		EPaldarkMatchEndReason NewReason,
		float NewPhaseEndServerTime);

	// Fires on server (after SetMatchPhaseFromSubsystem) and on clients
	// (from OnRep_MatchPhase). HUD + designer widgets bind.
	FPaldarkOnMatchPhaseReplicated OnMatchPhaseReplicated;

	// W46 — Briefing room session state (vote tallies + ready bits +
	// countdown phase). Replicated to every client so the briefing widget
	// can render identical state across the shard. Lives on GameState
	// because GameMode is server-only.
	UFUNCTION(BlueprintPure, Category = "Paldark|Hub|Briefing")
	UPaldarkBriefingSessionComponent* GetBriefingSessionComponent() const { return BriefingSessionComponent; }

protected:
	UFUNCTION()
	void OnRep_CurrentExperienceId();

	UFUNCTION()
	void OnRep_MatchPhase();

	UFUNCTION()
	void OnRep_MatchEndReason();

	UFUNCTION()
	void OnRep_PhaseEndServerTime();

	// Replicated id; clients resolve the soft pointer themselves.
	UPROPERTY(ReplicatedUsing = OnRep_CurrentExperienceId, Transient)
	FPrimaryAssetId CurrentExperienceId;

	// Server-side cached pointer. Not replicated — clients reload from id.
	UPROPERTY(Transient)
	TObjectPtr<const UPaldarkExperienceDefinition> CurrentExperience = nullptr;

	// W24-25 — Replicated match phase mirror of the server-side
	// `UPaldarkMatchSubsystem`. Always Warmup before the first
	// `SetMatchPhaseFromSubsystem` lands.
	UPROPERTY(ReplicatedUsing = OnRep_MatchPhase, Transient)
	EPaldarkMatchPhase MatchPhase = EPaldarkMatchPhase::Warmup;

	UPROPERTY(ReplicatedUsing = OnRep_MatchEndReason, Transient)
	EPaldarkMatchEndReason MatchEndReason = EPaldarkMatchEndReason::None;

	// Server time (seconds) at which the current phase's timer expires.
	// Replicated so the HUD countdown bar matches the server. Zero when
	// no timer is in flight.
	UPROPERTY(ReplicatedUsing = OnRep_PhaseEndServerTime, Transient)
	float PhaseEndServerTime = 0.f;

	// W46 — Briefing session component default subobject. Replicated by
	// AActor::bReplicates inheritance.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Hub|Briefing", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkBriefingSessionComponent> BriefingSessionComponent;
};
