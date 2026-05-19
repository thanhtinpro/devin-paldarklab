// PALDARK W1 day 8-10 + W7-8 — Player state.
//
// Holds per-player data that needs to be visible to all clients (score, team,
// GAS ASC). For the player, the ASC + AttributeSet live on the PlayerState
// (canonical pattern: Aura RPG, Crunch, Lyra) so the ability + attribute data
// survives a pawn death + respawn without losing buffs.
//
// W7-8 additions:
//   - Owns a UPaldarkAbilitySystemComponent (replicated) + a UPaldarkAttributeSet.
//   - Implements IAbilitySystemInterface so GetAbilitySystemComponent() / the
//     AbilitySystemGlobals helpers resolve back to this object.
//   - Replication mode = Mixed (server <-> autonomous proxy + minimal to
//     simulated proxies) which matches the Aura / Crunch playbook.

#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Match/PaldarkMatchTypes.h"
#include "PaldarkPlayerState.generated.h"

class UAbilitySystemComponent;
class UPaldarkAbilitySystemComponent;
class UPaldarkAttributeSet;
class UPaldarkPalDepositComponent;

// W24-25 — Multicast delegate fired on every client (and server) when the
// per-player match outcome flips. HUD widgets bind to refresh the end
// screen; AI Pal squad logic binds to release "follow this player" links
// when the player dies or extracts.
DECLARE_MULTICAST_DELEGATE_OneParam(
	FPaldarkOnPlayerMatchOutcomeChanged,
	EPaldarkPlayerOutcome /* NewOutcome */);

UCLASS()
class PALDARKLAB_API APaldarkPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	APaldarkPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	int32 GetTeamIndex() const { return TeamIndex; }

	// Server-only. Replicated to all clients via DOREPLIFETIME.
	void SetTeamIndex(int32 NewTeamIndex);

	// IAbilitySystemInterface — returns the strongly typed ASC up-cast to the
	// engine base type. Always non-null after construction.
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// W7-8 — Strongly typed accessors for the Paldark subclass + AttributeSet.
	UPaldarkAbilitySystemComponent* GetPaldarkAbilitySystemComponent() const { return AbilitySystemComponent; }
	const UPaldarkAttributeSet*     GetPaldarkAttributeSet()          const { return AttributeSet; }

	// W44-45 — Pal deposit storage. Lives on the PlayerState so the
	// deposit list survives pawn death + respawn (same logic the ASC
	// uses for surviving respawn). The active roster lives on the
	// character — see `UPaldarkPlayerPalRosterComponent`.
	UFUNCTION(BlueprintPure, Category = "Paldark|Hub|Stable")
	UPaldarkPalDepositComponent* GetPalDepositComponent() const { return PalDepositComponent; }

	// W24-25 — Match outcome accessors. MatchOutcome is replicated; the
	// match subsystem on the server calls SetMatchOutcome which sets the
	// property + fires OnRep_MatchOutcome manually so server-side observers
	// see the same ordering as clients.
	UFUNCTION(BlueprintPure, Category = "Paldark|Match")
	EPaldarkPlayerOutcome GetMatchOutcome() const { return MatchOutcome; }

	// IsAlive is a derived flag (MatchOutcome == Alive) but exposed
	// separately because (a) it's the most common runtime query, and (b)
	// HUD widgets bind to OnMatchOutcomeChanged once and just read this.
	UFUNCTION(BlueprintPure, Category = "Paldark|Match")
	bool IsAliveInMatch() const { return MatchOutcome == EPaldarkPlayerOutcome::Alive; }

	// 0.0 ... 1.0 — most recent extraction progress reported by an
	// `APaldarkExtractionBeacon`. Replicated low-frequency (NetUpdateFrequency
	// of the PlayerState is already 100 Hz; the beacon throttles writes
	// to once per 0.1 s so the wire traffic stays bounded).
	UFUNCTION(BlueprintPure, Category = "Paldark|Match")
	float GetExtractionProgress() const { return ExtractionProgress; }

	// Server-only. Flip outcome + broadcast. Called by
	// `UPaldarkMatchSubsystem::SetPlayerOutcomeInternal`.
	void SetMatchOutcome(EPaldarkPlayerOutcome NewOutcome);

	// Server-only. Update extraction progress (0..1). Called by beacons
	// every TickFreq. Designer-tunable. Clamped on set.
	void SetExtractionProgress(float NewProgress);

	// Delegate fires on server (after SetMatchOutcome) and on clients
	// (from OnRep_MatchOutcome).
	FPaldarkOnPlayerMatchOutcomeChanged OnMatchOutcomeChanged;

protected:
	UFUNCTION()
	void OnRep_TeamIndex();

	UFUNCTION()
	void OnRep_MatchOutcome();

	UFUNCTION()
	void OnRep_ExtractionProgress();

	// -1 = unassigned. Server populates on PostLogin from experience defaults.
	UPROPERTY(ReplicatedUsing = OnRep_TeamIndex, Transient)
	int32 TeamIndex = -1;

	// W7-8 — ASC + AttributeSet. ASC is replicated automatically because the
	// PlayerState is replicated; AttributeSet is a sub-object of the ASC and
	// rides along.
	UPROPERTY(VisibleAnywhere, Category = "Paldark|GAS")
	TObjectPtr<UPaldarkAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UPaldarkAttributeSet> AttributeSet;

	// W44-45 — Pal deposit storage. Default subobject so every PlayerState
	// ships with one; replicated `COND_OwnerOnly` (other players don't
	// observe each other's deposit list).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Hub|Stable")
	TObjectPtr<UPaldarkPalDepositComponent> PalDepositComponent;

	// W24-25 — Per-player match outcome, replicated to all clients so the
	// HUD on every machine can render "Alice extracted" / "Bob died".
	UPROPERTY(ReplicatedUsing = OnRep_MatchOutcome, Transient)
	EPaldarkPlayerOutcome MatchOutcome = EPaldarkPlayerOutcome::Alive;

	// W24-25 — Most recent extraction progress (0..1). Replicated so the
	// player's own HUD shows the ramp meter without a custom RPC.
	UPROPERTY(ReplicatedUsing = OnRep_ExtractionProgress, Transient)
	float ExtractionProgress = 0.f;
};
