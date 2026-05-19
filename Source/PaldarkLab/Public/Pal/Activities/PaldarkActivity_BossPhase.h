// PALDARK W29-30 — Boltmane (boss) phased combat activity.
//
// Replaces the W18-19 `UPaldarkActivity_Combat` (Priority 40) for the
// Boltmane subclass at Priority 50 — once the boss enters this state it
// never reverts (`ShouldContinue` always true). The activity is a thin
// wrapper around `UPaldarkPalCombatComponent::TryFireAttack` PLUS Health %
// threshold tracking:
//   - Phase Normal      — HP > EnragedThresholdPct.        Default cadence.
//   - Phase Enraged     — HP <= EnragedThresholdPct.       Speed +30%, attack interval -25%.
//   - Phase Telegraph   — HP <= TelegraphThresholdPct.     Broadcasts FOnBossPhaseChanged
//                                                         so designers / HUD can flash
//                                                         a phase banner + the AnimBP
//                                                         can play a wind-up montage.
//
// The phase transitions are monotonic (Normal → Enraged → Telegraph); once
// the boss enters Telegraph it stays there even if a heal effect bumps HP
// back above the threshold.
//
// W29-30 scaffold scope:
//   - Single C++ class + 1 multicast delegate.
//   - Phase math runs in `TickActivity` (server-only — Boltmane only
//     spawns on authority).
//   - Hands off the actual attack rhythm to the Pal's combat component
//     (same pattern as UPaldarkActivity_Combat).
//
// What this is NOT:
//   - No second ability slot for Telegraph phase (designer hooks the
//     delegate to swap CombatSlot.AttackAbilityClass at runtime if they
//     need a different ability per phase).
//   - No multi-target phase logic (chain-lightning ability handles
//     bounces itself).
//   - No HP recovery scripting — boss doesn't heal.
//   - No phase banner UMG (designer authors HUD widget).

#pragma once

#include "CoreMinimal.h"
#include "Pal/Activities/PaldarkBaseActivity.h"
#include "PaldarkActivity_BossPhase.generated.h"

class UPaldarkPalCombatComponent;
class UPaldarkPalDefinition;
class UPaldarkPalPerceptionComponent;

UENUM(BlueprintType)
enum class EPaldarkBossPhase : uint8
{
	Normal      UMETA(DisplayName = "Normal"),
	Enraged     UMETA(DisplayName = "Enraged"),
	Telegraph   UMETA(DisplayName = "Telegraph"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnBossPhaseChanged,
	EPaldarkBossPhase, OldPhase,
	EPaldarkBossPhase, NewPhase);

UCLASS()
class PALDARKLAB_API UPaldarkActivity_BossPhase : public UPaldarkBaseActivity
{
	GENERATED_BODY()

public:
	UPaldarkActivity_BossPhase();

	virtual bool InitActivity(UPaldarkPalActivityComponent* InComponent) override;

	virtual bool CanRun_Implementation() const override;
	virtual bool ShouldContinue_Implementation() const override;
	virtual void EnterActivity_Implementation() override;
	virtual void TickActivity_Implementation(float DeltaSeconds) override;
	virtual void ExitActivity_Implementation() override;

	// HP fractions (0..1) at which the boss steps into the next phase. The
	// designer authors these on the BP subclass of this activity per boss
	// encounter. Defaults are tuned for Boltmane (50% → Enraged, 25% →
	// Telegraph).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Activity|BossPhase", meta = (ClampMin = "0.05", ClampMax = "0.95"))
	float EnragedThresholdPct = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Activity|BossPhase", meta = (ClampMin = "0.05", ClampMax = "0.95"))
	float TelegraphThresholdPct = 0.25f;

	// Per-phase MoveSpeed multipliers — applied to `MaxWalkSpeed` on
	// CharacterMovement at the phase transition.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Activity|BossPhase", meta = (ClampMin = "0.1"))
	float EnragedMoveSpeedMultiplier = 1.3f;

	// Per-phase AttackInterval multiplier — multiplied against the combat
	// component's AttackInterval at the transition. 0.75 = 25% faster.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Activity|BossPhase", meta = (ClampMin = "0.1"))
	float EnragedAttackIntervalMultiplier = 0.75f;

	// Disengage range — same role as `UPaldarkActivity_Combat::DisengageDistance`
	// but the boss only "disengages" by losing the target, never by exiting
	// the activity itself (ShouldContinue stays true).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Activity|BossPhase", meta = (ClampMin = "0.0"))
	float DisengageDistance = 4000.f;

	// Yaw rotation rate (deg/sec) when re-orienting toward target — same
	// role as the equivalent on `UPaldarkActivity_Combat`.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Activity|BossPhase", meta = (ClampMin = "0.0"))
	float FaceTargetRateDegPerSec = 360.f;

	// Broadcast on every phase transition (Normal → Enraged, Enraged →
	// Telegraph). Designers bind a HUD widget event to flash a banner or
	// swap the Pal's CombatSlot.AttackAbilityClass to a different GA.
	UPROPERTY(BlueprintAssignable, Category = "Paldark|Pal|Activity|BossPhase")
	FOnBossPhaseChanged OnBossPhaseChanged;

	// Read-only accessor for HUD / debug.
	UFUNCTION(BlueprintPure, Category = "Paldark|Pal|Activity|BossPhase")
	EPaldarkBossPhase GetCurrentPhase() const { return CurrentPhase; }

protected:
	// Resolve current HP fraction (0..1) from the Pal's AttributeSet.
	// Returns 1.0 if no AttributeSet is available so the boss stays in
	// Normal phase rather than spuriously triggering.
	float GetCurrentHealthFraction() const;

	void TransitionToPhase(EPaldarkBossPhase NewPhase);

	// W39 — L-28 closure. Read the owning Pal's `UPaldarkPalDefinition`
	// (lookup pattern mirrors `UPaldarkPalTameComponent::HydrateFromPalDefinition`
	// — Species leaf → `DA_PalDef_<Leaf>` primary asset id) and copy the
	// definition's `PhaseHealthThresholds[]` array into this activity's
	// `EnragedThresholdPct` / `TelegraphThresholdPct` UPROPERTYs. Called
	// once at `EnterActivity` so a re-enter (post-disengage) re-reads
	// fresh values. Silently falls back to the activity's CDO defaults
	// (W29-30 baseline 0.5 / 0.25) when:
	//   - No AssetManager / no resolved PalDef for the species (early
	//     spawn frame, missing DA, sync-load not warm yet).
	//   - PalDef declares an empty `PhaseHealthThresholds[]` array (back-
	//     compat for `DA_PalDef_*` authored before W39 — no DA changes
	//     means same behaviour as W29-30).
	//   - Array is malformed (non-descending order, fewer than 2 entries,
	//     value outside `[0, 1]`). Logs a Warning so the designer sees
	//     it in PIE and falls back to defaults.
	// Server-only by virtue of `EnterActivity` running on authority (the
	// boss only spawns on authority); no client path involved.
	void HydrateThresholdsFromPalDefinition();

	UPROPERTY(Transient)
	TWeakObjectPtr<UPaldarkPalCombatComponent> CombatRef;

	UPROPERTY(Transient)
	TWeakObjectPtr<UPaldarkPalPerceptionComponent> PerceptionRef;

	UPROPERTY(Transient)
	EPaldarkBossPhase CurrentPhase = EPaldarkBossPhase::Normal;

	// Cached on first transition so we can revert multipliers on
	// ExitActivity (e.g. if the boss dies and the activity tears down).
	UPROPERTY(Transient)
	float OriginalMaxWalkSpeed = 0.f;

	UPROPERTY(Transient)
	float OriginalAttackInterval = 0.f;
};
