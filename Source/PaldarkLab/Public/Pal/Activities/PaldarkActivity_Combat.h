// PALDARK W18-19 — Pal Combat activity.
//
// Top-priority preempt path for the Pal Activity FSM. When the Pal's
// `UPaldarkPalPerceptionComponent` locks onto a hostile, this activity's
// utility score crosses the threshold and the FSM switches in. Combat
// preempts Idle (P10), Follow (P20), Investigate (P30) and runs at Priority
// 40 (set in the C++ constructor).
//
// Behaviour:
//   - CanRun: combat component has a valid target AND the utility score
//     (weighted average of ThreatDistance + PalHealth + AttackReady) >= MinUtilityScore.
//   - ShouldContinue: target still valid AND distance < `DisengageDistance` so
//     the activity sticks even when the target briefly walks out of the
//     perception radius (handled by the perception component's grace timer,
//     this is a cheap second-line guard).
//   - EnterActivity: pause the locomotion follow loop so the Pal stops
//     chasing the player and faces the threat. Logs the entry tag.
//   - TickActivity: re-orient the Pal toward the target (server-side
//     `SetActorRotation` — locomotion's bOrientRotationToMovement is off by
//     default on the Pal pawn, see `APaldarkPalCharacter::APaldarkPalCharacter`)
//     and call `UPaldarkPalCombatComponent::TryFireAttack()` every tick. The
//     combat component handles the cooldown gate internally.
//   - ExitActivity: re-enable the follow loop. The threat went away (perception
//     cleared the target or the dummy died), so the FSM will fall back to
//     Follow/Idle on the next selection tick.
//
// Designer knobs:
//   - MinUtilityScore (0.45) — score threshold to enter; above the threshold
//     the activity wants to run.
//   - DisengageDistance (2000cm) — beyond this the activity stops continuing
//     even if a stale target pointer is still set.
//   - ThreatDistanceWeight / PalHealthWeight / AttackReadyWeight — designer
//     tuning. Sum doesn't need to be 1.0; the helper normalises by total weight.
//
// Roadmap reference: Documents/PALDARK/03-Roadmap_1_Year.md § Tuần 18–19.

#pragma once

#include "CoreMinimal.h"
#include "Pal/Activities/PaldarkBaseActivity.h"
#include "Pal/Combat/PaldarkPalConsideration.h"
#include "PaldarkActivity_Combat.generated.h"

class UPaldarkPalCombatComponent;
class UPaldarkPalPerceptionComponent;

UCLASS()
class PALDARKLAB_API UPaldarkActivity_Combat : public UPaldarkBaseActivity
{
	GENERATED_BODY()

public:
	UPaldarkActivity_Combat();

	virtual bool InitActivity(UPaldarkPalActivityComponent* InComponent) override;

	virtual bool CanRun_Implementation() const override;
	virtual bool ShouldContinue_Implementation() const override;
	virtual void EnterActivity_Implementation() override;
	virtual void TickActivity_Implementation(float DeltaSeconds) override;
	virtual void ExitActivity_Implementation() override;

	// Utility threshold. The activity runs when the weighted-average score
	// across all considerations is >= this value. 0.45 keeps Combat from
	// triggering on low-health Pals chasing far-away threats.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Activity|Combat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinUtilityScore = 0.45f;

	// Beyond this distance (cm) the activity stops continuing even if the
	// target pointer is still set. Default = 2000 cm so the Pal disengages
	// when the player + Pal escape together but the dummy keeps spawning
	// out of range.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Activity|Combat", meta = (ClampMin = "0.0"))
	float DisengageDistance = 2000.f;

	// Considerations weights — designer-tunable per Pal subclass.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Activity|Combat", meta = (ClampMin = "0.0"))
	float ThreatDistanceWeight = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Activity|Combat", meta = (ClampMin = "0.0"))
	float PalHealthWeight = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Activity|Combat", meta = (ClampMin = "0.0"))
	float AttackReadyWeight = 0.4f;

	// Yaw rotation rate (deg/sec) used when re-orienting the Pal toward the
	// target. 360 = instant; lower values give a softer "track" feel that
	// reads better when animation montages land in W22+.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Activity|Combat", meta = (ClampMin = "0.0"))
	float FaceTargetRateDegPerSec = 360.f;

	// Read-only accessor used by `Paldark.Pal.DumpThreat` to surface the
	// most recent utility score in the console.
	UFUNCTION(BlueprintPure, Category = "Paldark|Pal|Activity|Combat")
	float GetLastUtilityScore() const { return LastUtilityScore; }

protected:
	// Cached on InitActivity so CanRun doesn't re-walk the component graph
	// every selection tick.
	UPROPERTY(Transient)
	TWeakObjectPtr<UPaldarkPalCombatComponent> CombatRef;

	UPROPERTY(Transient)
	TWeakObjectPtr<UPaldarkPalPerceptionComponent> PerceptionRef;

	// Build the consideration table from the current Pal state. Reads the
	// combat component (cooldown), perception component (distance), and Pal
	// AttributeSet (health %). Returns false when any dependency is missing.
	bool BuildConsiderations(TArray<FPaldarkConsideration>& OutConsiderations) const;

private:
	// Updated in CanRun / TickActivity so the debug command can surface the
	// last computed score without re-running the considerations.
	mutable float LastUtilityScore = 0.f;
};
