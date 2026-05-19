// PALDARK W46 — Briefing room UI controller.
//
// ActorComponent that lives on every `APaldarkPlayerController`. Owns the
// per-player client/server plumbing the W40-41 `APaldarkBriefingRoom` building
// delegates to in its `OnInteract` — it's the bridge between the UMG widget
// (designer-authored, `UPaldarkBriefingWidget` subclass) and the shard-wide
// session state living on:
//
//   * `UPaldarkBriefingSessionComponent` (W46) — replicated vote tally,
//     ready count, current phase + countdown remaining. Lives on the
//     GameState because it must be visible to every connected client.
//   * `UPaldarkHubSubsystem` (W40-41)         — read-only allow-list /
//     pending raid tag fallback for solo-test scenarios.
//
// W40-41 wired the briefing room straight to `HostHubServer` on a single-
// player console flow (`Paldark.Hub.QueueRaid` + press E). W46 widens that
// to the actual lobby-style flow:
//
//   1. BriefingRoom::OnInteract        → controller->RequestOpenBriefing(this).
//   2. UI state tag flips to `Paldark.Hub.Brief.UI.Opening` → `Open`.
//   3. UMG widget binds `OnBriefingOpened` and reads vote tallies + ready
//      count + phase via the replicated session component (no separate
//      snapshot RPC — phase + tallies are already `COND_None`).
//   4. Player clicks "Vote for Rung Hong" → controller fires
//      `Server_RequestVote(MapTag)`; server stamps the player's vote in
//      the session component; OnRep_VoteTallies fires on every client,
//      widgets refresh tally bars.
//   5. Player clicks "Ready" → `Server_RequestReady`; server flips the
//      player's ready bit, recomputes "majority + threshold" — once the
//      majority-voted map has >= MinReadyPlayers ready, server transitions
//      session phase to `Paldark.Hub.Brief.Phase.Countdown` and starts the
//      shared countdown timer.
//   6. Countdown completes → server transitions phase to `Travelling` and
//      issues `HostHubServer(ResolvedMapName, ExperienceId)` via the
//      W40-41 deferred travel helper (single ServerTravel for the whole
//      shard, not a per-player ClientTravel).
//   7. Action results (vote/unvote/ready/unready/cancel) round-trip
//      through `Client_ReceiveActionResult` so the originating player's
//      widget can show success/fail (e.g. votes for non-allowed maps).
//
// Design notes:
//   - All blueprint-facing delegates are `DECLARE_DYNAMIC_MULTICAST_DELEGATE_*`
//     so UMG can wire them without C++.
//   - `bIsTransactionInFlight` guards against double-click vote spam; the
//     server flag is per-controller (per-player), so other players' votes
//     are not gated by my in-flight RPC.
//   - The countdown timer + ServerTravel issue are owned by the session
//     component on GameState — the controller is purely a client+server
//     RPC bridge.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "PaldarkBriefingController.generated.h"

class APaldarkBriefingRoom;
class APaldarkPlayerController;
class UPaldarkBriefingSessionComponent;
class UPaldarkBriefingWidget;

// 1-arg multicast — autonomous-client widget binds to refresh on every
// transaction reply. ActionTag = Paldark.Hub.Brief.Action.*,
// ResultTag = Paldark.Hub.Result.*, MapTag = the map vote target
// (invalid for Ready/Unready), CountdownRemaining = if the server has
// transitioned phase to Countdown, the remaining seconds (0 otherwise).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FPaldarkBriefingActionResult,
	FGameplayTag, ActionTag,
	FGameplayTag, ResultTag,
	FGameplayTag, MapTag,
	int32, CountdownRemaining);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FPaldarkBriefingOpened, AActor*, BriefingRoomActor);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPaldarkBriefingClosed);

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent))
class PALDARKLAB_API UPaldarkBriefingController : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkBriefingController();

	// --- Client API --------------------------------------------------------

	// Called by `APaldarkBriefingRoom::OnInteract` on the autonomous client
	// (or listen-server host). Flips the UI state tag to
	// `Paldark.Hub.Brief.UI.Open` and fires `OnBriefingOpened` so the
	// widget mounts. Caches the briefing-room actor for the widget to
	// read room-specific config (AllowedRaidMaps from the building def).
	UFUNCTION(BlueprintCallable, Category = "Paldark|Hub|Briefing")
	void RequestOpenBriefing(APaldarkBriefingRoom* BriefingRoom);

	// Called by widget on close button / Escape. Restores the UI state tag
	// to `Paldark.Hub.Brief.UI.Closed` and fires `OnBriefingClosed`. The
	// player's vote + ready state on the server is NOT cleared — closing
	// the widget just hides the UI; the player is still committed until
	// they explicitly Unvote / Unready.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Hub|Briefing")
	void RequestCloseBriefing();

	// Cast a vote for the supplied map tag. MapTag must be one of
	// `BuildingDefinition->AllowedRaidMaps` on the active briefing room
	// (or pass-through if AllowedRaidMaps is empty — designer test maps).
	// Replaces any previous vote by this player.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Hub|Briefing")
	void RequestVote(FGameplayTag MapTag);

	// Clear my vote (no-op if I had not voted).
	UFUNCTION(BlueprintCallable, Category = "Paldark|Hub|Briefing")
	void RequestUnvote();

	// Signal that I'm ready to depart. Server flips my ready bit + recomputes
	// the "majority-voted map + MinReadyPlayers" trigger; if reached, server
	// transitions session phase to Countdown.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Hub|Briefing")
	void RequestReady();

	// Clear my ready bit. If we were in Countdown phase and dropping below
	// the MinReadyPlayers threshold, server cancels the countdown back to
	// Idle phase.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Hub|Briefing")
	void RequestUnready();

	// --- Read-only state ---------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Paldark|Hub|Briefing")
	FGameplayTag GetUIStateTag() const { return UIStateTag; }

	// Always non-null. Cached on BeginPlay from the outer actor.
	UFUNCTION(BlueprintPure, Category = "Paldark|Hub|Briefing")
	APaldarkPlayerController* GetOwningPaldarkController() const { return OwnerController; }

	// Resolves the session component on the current GameState. Replicated
	// to every client; safe to read for UMG bind. Returns null in solo /
	// no-GameState contexts.
	UFUNCTION(BlueprintPure, Category = "Paldark|Hub|Briefing")
	UPaldarkBriefingSessionComponent* ResolveSessionComponent() const;

	// --- Delegates ---------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "Paldark|Hub|Briefing")
	FPaldarkBriefingOpened OnBriefingOpened;

	UPROPERTY(BlueprintAssignable, Category = "Paldark|Hub|Briefing")
	FPaldarkBriefingClosed OnBriefingClosed;

	UPROPERTY(BlueprintAssignable, Category = "Paldark|Hub|Briefing")
	FPaldarkBriefingActionResult OnBriefingActionResult;

protected:
	virtual void BeginPlay() override;

	// --- Server RPCs -------------------------------------------------------

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestVote(FGameplayTag MapTag);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestUnvote();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestReady();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestUnready();

	// --- Client RPC --------------------------------------------------------

	UFUNCTION(Client, Reliable)
	void Client_ReceiveActionResult(
		FGameplayTag ActionTag,
		FGameplayTag ResultTag,
		FGameplayTag MapTag,
		int32 CountdownRemaining);

private:
	// Cached on BeginPlay so server RPCs don't keep re-casting.
	UPROPERTY(Transient)
	TObjectPtr<APaldarkPlayerController> OwnerController;

	// Client-side: cached briefing-room reference for the "currently open"
	// briefing room. Nulled on `RequestCloseBriefing`. Server doesn't track
	// this — the vote/ready RPCs are room-agnostic and route through the
	// shard-wide session component on GameState.
	UPROPERTY(Transient)
	TObjectPtr<APaldarkBriefingRoom> ActiveBriefingRoom;

	// Client-side UI state. Flipped by RequestOpenBriefing /
	// RequestCloseBriefing and read by the UMG widget via `GetUIStateTag`.
	// Never replicated — each client has its own UI state.
	FGameplayTag UIStateTag;

	// Server-side flag — guards against a malicious client spamming
	// Server_RequestX while the previous one is still processing. Per-player
	// so other players' votes are independent.
	bool bIsTransactionInFlight = false;
};
