// PALDARK W18-19 — Pal perception component (W3-4 stub → real shape).
//
// Server-only periodic threat scan. Promoted from the W3-4 stub into the
// "is there an enemy near my Pal" oracle that the combat activity reads.
//
// Detection model (intentionally minimal for W18-19):
//   - Every `ScanInterval` (default 0.25 s) on authority, iterate actors
//     within `ThreatRadius` (default 1500 cm) of the owning Pal.
//   - An actor counts as a threat when its `Paldark.Team.*` TeamTag matches
//     `HostileTeamTag` (default `Paldark.Team.Hostile`) AND it isn't already
//     dead (no `Paldark.State.IsDead` ASC tag).
//   - Pick the closest valid threat as `CurrentThreat`. Aggro hysteresis: a
//     newly-spotted threat must be at least `AggroSwitchHysteresisCm` (200 cm)
//     closer than the current pick before the Pal switches focus — prevents
//     thrash around equidistant hostiles.
//   - When perception loses sight, hold `CurrentThreat` for
//     `ThreatGracePeriodSeconds` (default 3 s) so a momentary occlusion or
//     LOS break doesn't drop the Pal back to Follow.
//
// What this is NOT (deferred):
//   - No `UAIPerceptionComponent` wrap. The W18-19 sandbox has 1 Pal + 1
//     hostile, and `TActorIterator<AActor>` filtered by radius is faster
//     than the AIPerception stimulus pipeline for that scale.
//   - No octree / spatial index — every Pal scans every actor each tick.
//     Replace with `UPaldarkThreatRegistrySubsystem` in W48-49 when the
//     significance manager work lands.
//   - No line-of-sight raycast. Threats are detected through walls in
//     W18-19; LOS gating lands in W20-21 when hostile-Pal AI ships.
//   - No client-side prediction. The component only ticks on authority;
//     clients only see the replicated `bHasThreat` for HUD purposes.
//
// Roadmap reference: Documents/PALDARK/03-Roadmap_1_Year.md § Tuần 18–19.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Delegates/DelegateCombinations.h"
#include "GameplayTagContainer.h"
#include "PaldarkPalPerceptionComponent.generated.h"

class AActor;
class APaldarkPalCharacter;

// Broadcast (authority only) when the perception's CurrentThreat changes
// identity, including loss of threat (NewThreat == nullptr). The combat
// component binds this to immediately re-aim instead of polling.
DECLARE_MULTICAST_DELEGATE_TwoParams(
	FOnPaldarkThreatChanged,
	AActor* /*OldThreat*/,
	AActor* /*NewThreat*/);

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent), Abstract = false)
class PALDARKLAB_API UPaldarkPalPerceptionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkPalPerceptionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Returns the currently tracked threat actor (server authority — clients
	// will always see nullptr because the component is server-only). nullptr
	// when no threat is in range OR the grace timer is active but the actor
	// has despawned.
	UFUNCTION(BlueprintPure, Category = "Paldark|Pal|Perception")
	AActor* GetCurrentThreat() const { return CurrentThreat.Get(); }

	UFUNCTION(BlueprintPure, Category = "Paldark|Pal|Perception")
	float GetCurrentThreatDistance() const { return CurrentThreatDistance; }

	UFUNCTION(BlueprintPure, Category = "Paldark|Pal|Perception")
	bool HasThreat() const { return CurrentThreat.IsValid() && !CurrentThreat->IsActorBeingDestroyed(); }

	// Debug: force the Pal to consider `InThreat` as its current threat
	// regardless of distance / team filter. Used by
	// `Paldark.Pal.ForceCombat`. Authority only.
	void ForceThreat(AActor* InThreat);

	// Dump the perception state to `LogPaldarkPal`. Used by `Paldark.Pal.DumpThreat`.
	void DumpToLog() const;

	// Server-side multicast — combat component binds this to reset its
	// last-attack-time so the new target can be fired on immediately.
	FOnPaldarkThreatChanged OnThreatChanged;

	// Designer knobs ----------------------------------------------------

	// Scan period in seconds. Default 0.25 s — fast enough to catch a
	// running player within ~1m of overshoot at 4 m/s, slow enough that the
	// O(N) iterator cost stays negligible for the W18-19 sandbox scale.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Perception", meta = (ClampMin = "0.0"))
	float ScanInterval = 0.25f;

	// Detection radius around the Pal in centimeters. Default 1500 cm = 15 m,
	// matching the W18-19 sandbox spawn distances. Designers tune up per
	// species (e.g. a flying Pal might scan 30 m).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Perception", meta = (ClampMin = "0.0"))
	float ThreatRadius = 1500.f;

	// Aggro-switch hysteresis in centimeters. A new candidate threat must be
	// at least this much closer than `CurrentThreat` before the Pal
	// switches. Prevents flicker between two equidistant hostiles.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Perception", meta = (ClampMin = "0.0"))
	float AggroSwitchHysteresisCm = 200.f;

	// Seconds to stick on a lost threat before clearing. Stops Combat→Follow
	// thrash when a hostile briefly steps behind cover or the actor pointer
	// goes stale for a frame.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Perception", meta = (ClampMin = "0.0"))
	float ThreatGracePeriodSeconds = 3.0f;

	// Team tag the perception treats as the Pal's own team. Default
	// `Paldark.Team.Player` so a Pal travelling with the player ignores the
	// player as a threat.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Perception")
	FGameplayTag FriendlyTeamTag;

	// Team tag the perception treats as hostile. Anything with this tag is
	// a candidate threat. Default `Paldark.Team.Hostile`.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Perception")
	FGameplayTag HostileTeamTag;

protected:
	// Per-tick threat scan. Picks the closest hostile within radius, applies
	// hysteresis, updates `CurrentThreat` / `CurrentThreatDistance`, broadcasts
	// `OnThreatChanged` when identity changes, and arms / disarms the grace
	// timer when sight is lost.
	void RunScan();

	// Reads the team tag from an actor. Casts to the small handful of
	// classes that expose `GetTeamTag()` (Pal, player character, dummy).
	static FGameplayTag GetActorTeamTag(const AActor* InActor);

	// Resolves whether an actor counts as a threat to this Pal (team tag
	// match + not dead + still valid). Pure function so it can be reused
	// by `ForceThreat` validation.
	bool IsActorThreatening(const AActor* Candidate) const;

private:
	// Resolved on BeginPlay so the scan loop avoids the GetOwner cast every tick.
	UPROPERTY(Transient)
	TWeakObjectPtr<APaldarkPalCharacter> PalOwner;

	// Active threat. Server-only; clients always read null (replication would
	// add no value for the W18-19 sandbox).
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentThreat;

	float CurrentThreatDistance = 0.f;

	// Accumulator counts down between scans so tick rate doesn't drive scan rate.
	float TimeUntilNextScan = 0.f;

	// >0 when we have lost line of sight but are still inside the grace period.
	float ThreatGraceRemaining = 0.f;
};
