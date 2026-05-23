// PALDARK W35-36 — UPaldarkPalBondComponent (Bond level + XP).
//
// W3-4 originally registered this class as an empty `UActorComponent` slot so
// the owning pawn's ctor could `CreateDefaultSubobject<UPaldarkPalBondComponent>(...)`
// and designer Blueprints could extend the component without touching C++.
//
// W35-36 fills it in:
//   - Per-Pal `BondLevel` (int32, 0..20) and `BondXP` (float, accumulator)
//     replicated to all clients so HUD widgets can read bond state.
//   - `AddBondXP(Amount, ReasonTag)` server-only API. Bumps `BondXP`; on
//     crossing the next-level XP threshold (read from `BondXPCurve`), bumps
//     `BondLevel` and broadcasts `OnBondLevelChanged`. Designers wire raids /
//     activities into this API via the 3 reason tags:
//       Paldark.Bond.Event.Damage     — Pal contributed to a hostile kill.
//       Paldark.Bond.Event.Tame       — Pal was tamed by the player (50 XP).
//       Paldark.Bond.Event.ShareFood  — Pal received food from the player.
//   - `OnBondLevelChanged` multicast delegate (`int32 NewLevel`) — bound by
//     `UPaldarkPalCombatComponent` / animation BP / UI HUD to react.
//
// Component lives on every `APaldarkPalCharacter` (W3-4 slot pattern) but
// only player-side companions tick `AddBondXP` in practice — hostile Pals
// never get bond. Designers can opt out by flipping `bTracksBond` to false
// in the Pal Blueprint default.
//
// Future homes (do NOT implement here in W35-36):
//   - Persistent bond across matches → save game (W47).
//   - Bond level → ability cooldown / damage modifiers (W37-38).
//   - Bond-based UI prompt above Pal head (W50).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Curves/CurveFloat.h"
#include "GameplayTagContainer.h"
#include "PaldarkPalBondComponent.generated.h"

// Multicast delegate fired when BondLevel transitions to a new integer level.
// Non-dynamic so any non-UFUNCTION subscriber (e.g. AnimBP graph helper) can
// hook without dragging UFUNCTION boilerplate.
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPaldarkBondLevelChanged, int32 /*NewLevel*/);

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent), Abstract = false)
class PALDARKLAB_API UPaldarkPalBondComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkPalBondComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Server-only XP grant. `Amount` is added directly to `BondXP`; if the
	// running total crosses the next-level threshold (read from
	// `BondXPCurve.GetFloatValue(BondLevel + 1)`), bumps `BondLevel` by 1 (or
	// more, for very large grants) and broadcasts `OnBondLevelChanged` for
	// every new level. `ReasonTag` is logged for telemetry; no behavioural
	// effect — designers pass `Paldark.Bond.Event.<Reason>` so dumps stay
	// readable.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Pal|Bond")
	void AddBondXP(float Amount, FGameplayTag ReasonTag);

	// Sets a level directly (clamped to [0, MaxBondLevel]). Used by console
	// commands and the tame flow when the player tames a Pal (registers it
	// at Bond Level 1 in the player roster).
	UFUNCTION(BlueprintCallable, Category = "Paldark|Pal|Bond")
	void SetBondLevel(int32 NewLevel);

	UFUNCTION(BlueprintPure, Category = "Paldark|Pal|Bond")
	int32 GetBondLevel() const { return BondLevel; }

	UFUNCTION(BlueprintPure, Category = "Paldark|Pal|Bond")
	float GetBondXP() const { return BondXP; }

	// Read the cumulative XP threshold to reach a given level. Reads
	// `BondXPCurve` at `TargetLevel`; if no curve is authored, falls back
	// to a linear 100 XP/level baseline.
	UFUNCTION(BlueprintPure, Category = "Paldark|Pal|Bond")
	float GetXPThresholdForLevel(int32 TargetLevel) const;

	// Broadcast on the authority (and on clients via OnRep_BondLevel) when
	// BondLevel transitions. Multiple subscribers OK (AnimBP + HUD + combat
	// component).
	FOnPaldarkBondLevelChanged OnBondLevelChanged;

	// W35-36 — Designer-facing knobs.

	// Max bond level (roadmap baseline is 20). Exposed so a future PR can
	// raise the ceiling without breaking saves. Read by SetBondLevel clamp.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Pal|Bond")
	int32 MaxBondLevel = 20;

	// XP curve. Designer authors the per-level thresholds as a runtime curve
	// (e.g. linear 100 XP/level, exponential to slow down high-tier bond).
	// X axis = target level, Y axis = cumulative XP threshold to reach that
	// level. GetXPThresholdForLevel(N) returns curve.Eval(N) (fallback
	// linear 100 XP/level if curve empty).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Pal|Bond")
	FRuntimeFloatCurve BondXPCurve;

	// When false, the Pal is permanently locked at BondLevel 0 (hostile Pal
	// default in subclass ctor). Pal subclasses flip to true on companions.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Pal|Bond")
	bool bTracksBond = true;

protected:
	UFUNCTION()
	void OnRep_BondLevel(int32 OldLevel);

	UFUNCTION()
	void OnRep_BondXP(float OldXP);

	// Current bond level, replicated. RepNotify forwards to OnBondLevelChanged
	// so clients see the same broadcast cascade as the server.
	UPROPERTY(ReplicatedUsing = OnRep_BondLevel, VisibleAnywhere, Category = "Paldark|Pal|Bond")
	int32 BondLevel = 0;

	// Accumulator XP, replicated. RepNotify is mostly informational — HUD
	// widget binds to OnBondLevelChanged for the integer breakpoints.
	UPROPERTY(ReplicatedUsing = OnRep_BondXP, VisibleAnywhere, Category = "Paldark|Pal|Bond")
	float BondXP = 0.0f;
};
