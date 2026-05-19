// PALDARK W46 — Briefing session replicated state.
//
// `UActorComponent` that lives on `APaldarkGameStateBase` and owns the
// shard-wide briefing session state — vote tallies per map, ready bitset
// per player, current phase (Idle / Countdown / Travelling), and the
// shared countdown timer. Replicated to every connected client so the
// briefing widget can show identical state on every screen.
//
// Why a component on GameState (and not on GameMode or a subsystem):
//
//   - GameMode is server-only — clients cannot read MaxPlayers or vote
//     tallies without an extra replication hop. GameState replicates by
//     default, so the briefing widget binds straight to the component
//     on the local GameState mirror.
//   - A `UWorldSubsystem` would also be server-only on dedicated and
//     would force every per-tally read into an RPC. GameState component
//     is the idiomatic UE shape (mirrors `ALyraGameState::Components`
//     pattern + `AGameState::PlayerArray` ergonomics).
//   - GameState is recreated on every map travel, so the briefing state
//     resets naturally when the hub-to-raid handoff completes. No
//     manual cleanup on ServerTravel.
//
// Authority surface (server-only):
//
//   * `ServerCastVote(PC, MapTag)`     — record one player's vote.
//   * `ServerClearVote(PC)`            — remove one player's vote.
//   * `ServerSetReady(PC, bReady)`     — flip one player's ready bit.
//   * `ServerRemovePlayer(PC)`         — clean up a disconnecting player.
//   * `ServerEvaluateThreshold()`      — recompute majority + ready count,
//                                        transition phase if reached.
//   * `ServerForceTravel(MapTag, ...)` — designer / QA bypass that skips
//                                        the vote/ready gate and issues
//                                        the ServerTravel immediately
//                                        (used by `Paldark.Hub.QA.ForceTravel`).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "PaldarkBriefingSessionComponent.generated.h"

class APaldarkPlayerController;
class APaldarkPlayerState;

// One row in the replicated vote tally. MapTag = a Paldark.Map.* tag from
// the briefing room's AllowedRaidMaps; VoteCount = how many players have
// cast that vote (computed server-side from VoterStates, replicated as a
// compact array so widgets can render bars without iterating the player
// list).
USTRUCT(BlueprintType)
struct FPaldarkBriefingVoteTally
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Hub|Briefing")
	FGameplayTag MapTag;

	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Hub|Briefing")
	int32 VoteCount = 0;
};

// One player's per-session state. Replicated as part of `VoterStates`.
// PlayerState soft-ref so the widget can resolve display names without
// the GameState component holding a hard pointer (clients see a TWeakObjectPtr
// reslove-to-null for disconnecting players, server cleans up on logout).
USTRUCT(BlueprintType)
struct FPaldarkBriefingVoterState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Hub|Briefing")
	TWeakObjectPtr<APaldarkPlayerState> PlayerState = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Hub|Briefing")
	FGameplayTag VotedMapTag;

	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Hub|Briefing")
	bool bIsReady = false;
};

// Multicast broadcast on every OnRep — widgets bind to this rather than
// to the OnRep_ function so designer code can refresh from gameplay
// callbacks (e.g. `BP_ForceVoteRefresh` on a debug actor) without
// touching the OnRep body.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPaldarkBriefingSessionChanged);

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent))
class PALDARKLAB_API UPaldarkBriefingSessionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkBriefingSessionComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Convenience accessor — equivalent to
	// `World->GetGameState<>()->FindComponentByClass<UPaldarkBriefingSessionComponent>()`
	// with a null-guard so callers don't have to repeat the world-state
	// resolution.
	static UPaldarkBriefingSessionComponent* Get(const UWorld* World);

	// --- Authority API -----------------------------------------------------

	// Cast a vote for the given map. Replaces any previous vote by this
	// player. Returns true on success, false if the controller is null,
	// the map tag is invalid, or we're in Travelling phase (votes locked).
	bool ServerCastVote(APaldarkPlayerController* PC, const FGameplayTag& MapTag);

	// Clear this player's vote (no-op if they hadn't voted). Returns true
	// if a vote was actually cleared.
	bool ServerClearVote(APaldarkPlayerController* PC);

	// Flip this player's ready bit to bReady. Returns true on state change.
	bool ServerSetReady(APaldarkPlayerController* PC, bool bReady);

	// Remove a player entirely from the session (used on logout / shard
	// teardown). Re-evaluates the threshold afterwards.
	bool ServerRemovePlayer(APaldarkPlayerController* PC);

	// Force the session into Travelling phase + issue HostHubServer
	// directly with the given map. Bypasses every gate. Designer / QA
	// only — used by the `Paldark.Hub.QA.ForceTravel` console command.
	void ServerForceTravel(const FGameplayTag& MapTag, const FString& ResolvedMapName);

	// --- Read-only state ---------------------------------------------------

	// Current phase tag. Replicated. Defaults to `Paldark.Hub.Brief.Phase.Idle`.
	UFUNCTION(BlueprintPure, Category = "Paldark|Hub|Briefing")
	FGameplayTag GetPhaseTag() const { return PhaseTag; }

	// Snapshot of the replicated vote tallies (sorted descending by count).
	UFUNCTION(BlueprintPure, Category = "Paldark|Hub|Briefing")
	TArray<FPaldarkBriefingVoteTally> GetVoteTallies() const;

	// Snapshot of the replicated voter states (sorted by player name for
	// deterministic UI rendering). Note: dead/disconnected entries are
	// skipped client-side via `PlayerState.IsValid()`.
	UFUNCTION(BlueprintPure, Category = "Paldark|Hub|Briefing")
	TArray<FPaldarkBriefingVoterState> GetVoterStates() const { return VoterStates; }

	// Computed: how many of the present voters have bIsReady=true.
	UFUNCTION(BlueprintPure, Category = "Paldark|Hub|Briefing")
	int32 GetReadyCount() const;

	// Computed: how many of the present voters have a valid VotedMapTag.
	UFUNCTION(BlueprintPure, Category = "Paldark|Hub|Briefing")
	int32 GetVoteCount() const;

	// Computed: the map tag that currently has the most votes (or invalid
	// tag if no votes have been cast yet). Ties are broken alphabetically
	// on the tag string for deterministic tie-handling.
	UFUNCTION(BlueprintPure, Category = "Paldark|Hub|Briefing")
	FGameplayTag GetLeadingMapTag() const;

	// Replicated countdown seconds remaining. 0 unless PhaseTag is
	// `Paldark.Hub.Brief.Phase.Countdown`.
	UFUNCTION(BlueprintPure, Category = "Paldark|Hub|Briefing")
	int32 GetCountdownRemaining() const { return CountdownRemaining; }

	// --- Designer knobs ----------------------------------------------------

	// Minimum number of ready players required for the countdown to start.
	// Default 1 so a designer running solo can advance the flow; production
	// shard policy will bump this to e.g. ceil(0.6 * MaxPlayers).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Hub|Briefing",
		meta = (ClampMin = "1", UIMin = "1"))
	int32 MinReadyPlayers = 1;

	// How many seconds to count down before issuing the ServerTravel.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Hub|Briefing",
		meta = (ClampMin = "0", UIMin = "0"))
	int32 CountdownSeconds = 10;

	// --- Delegates ---------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "Paldark|Hub|Briefing")
	FPaldarkBriefingSessionChanged OnSessionChanged;

	// Dump the entire session state (phase, tallies, voters) to LogPaldarkUI.
	// Used by the `Paldark.Hub.Brief.Dump` console command.
	void DumpToLog() const;

protected:
	// Server-only: re-evaluate "leading map + min ready" threshold. Called
	// after every state mutation (vote, unvote, ready, unready, remove).
	// Transitions phase between Idle / Countdown as appropriate.
	void ServerEvaluateThreshold();

	// Server-only countdown tick. Decrements `CountdownRemaining`; when
	// it hits 0, transitions phase to Travelling + issues the ServerTravel.
	void ServerTickCountdown();

	// Server-only — issue the actual ServerTravel for the resolved leading
	// map. Called from the countdown tick OR `ServerForceTravel`.
	void ServerIssueTravelForMap(const FGameplayTag& MapTag, const FString& OverrideMapName);

	UFUNCTION()
	void OnRep_PhaseTag();

	UFUNCTION()
	void OnRep_VoterStates();

	UFUNCTION()
	void OnRep_CountdownRemaining();

private:
	// Replicated phase tag. Defaults to `Paldark.Hub.Brief.Phase.Idle` in ctor.
	UPROPERTY(ReplicatedUsing = OnRep_PhaseTag, Transient)
	FGameplayTag PhaseTag;

	// Replicated per-voter state. Server appends on first vote/ready, removes
	// on logout. Clients render directly from this array.
	UPROPERTY(ReplicatedUsing = OnRep_VoterStates, Transient)
	TArray<FPaldarkBriefingVoterState> VoterStates;

	// Replicated countdown seconds remaining. Server starts at
	// CountdownSeconds when entering Countdown phase, decrements via timer,
	// flips to Travelling at 0. Clients render a bar / numeric.
	UPROPERTY(ReplicatedUsing = OnRep_CountdownRemaining, Transient)
	int32 CountdownRemaining = 0;

	// Server-only timer handle for the 1Hz countdown tick.
	FTimerHandle CountdownTimerHandle;

	// Find / create the voter row for this player. Server-only.
	FPaldarkBriefingVoterState* FindOrAddVoterRow(APaldarkPlayerController* PC);

	// Find the existing voter row (or nullptr). Server-only.
	FPaldarkBriefingVoterState* FindVoterRow(APaldarkPlayerController* PC);
};
