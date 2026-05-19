// PALDARK W20-21 — Pal Stalk activity.
//
// Priority-25 activity that fires when perception sees a threat but the Pal
// is still outside `Combat.MinEngageRange`. Sits between Investigate (30 — only
// for player-side Pals reacting to pings) and Combat (40). For hostile Pals
// the FSM is [Idle 10, Patrol 15, Stalk 25, Combat 40].
//
// Behaviour:
//   - CanRun: perception has a valid threat AND distance > Combat.MinEngageRange.
//     Below MinEngageRange Combat (P40) preempts naturally via its higher priority.
//   - TickActivity: steer the Pal toward the threat's location at reduced
//     speed (`StalkSpeedScale` * default MaxWalkSpeed). The reduced scale
//     reads as "creeping in" which is the W20-21 deliverable.
//   - ExitActivity: restore the saved MaxWalkSpeed so a subsequent flee /
//     patrol activity uses the species default.
//
// Designer knobs:
//   - StalkSpeedScale (0.6) — multiplier applied to MaxWalkSpeed on enter.
//   - LoseSightDistance (2500 cm) — if the threat ever exceeds this, Stalk
//     stops continuing (perception's grace timer handles the actual handoff,
//     this is the safety net).
//
// What this is NOT (deferred):
//   - No LOS check — perception already gates threats by radius; LOS lands
//     W48-49 with the perception subsystem rewrite.
//   - No "circle the target" behaviour — direct approach only. Flock /
//     formation cohesion lands W30+ in the Razorbird aerial polish week.
//   - No replication — server-only behaviour. ACharacter replicates the
//     resulting transform.

#pragma once

#include "CoreMinimal.h"
#include "Pal/Activities/PaldarkBaseActivity.h"
#include "PaldarkActivity_Stalk.generated.h"

class UPaldarkPalCombatComponent;
class UPaldarkPalPerceptionComponent;

UCLASS()
class PALDARKLAB_API UPaldarkActivity_Stalk : public UPaldarkBaseActivity
{
	GENERATED_BODY()

public:
	UPaldarkActivity_Stalk();

	virtual bool InitActivity(UPaldarkPalActivityComponent* InComponent) override;

	virtual bool CanRun_Implementation() const override;
	virtual bool ShouldContinue_Implementation() const override;
	virtual void EnterActivity_Implementation() override;
	virtual void TickActivity_Implementation(float DeltaSeconds) override;
	virtual void ExitActivity_Implementation() override;

	// Multiplier applied to MaxWalkSpeed while stalking. Default 0.6 — reads
	// as "stalking" rather than "charging" at the target.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Activity|Stalk", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float StalkSpeedScale = 0.6f;

	// Safety: if the threat exceeds this distance while Stalk is active, the
	// activity stops continuing even before perception's grace timer fires.
	// Default 2500 cm — slightly larger than perception's default
	// `ThreatRadius` (1500 cm) so the activity follows the perception's
	// hand-off, not the other way around.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Activity|Stalk", meta = (ClampMin = "0.0"))
	float LoseSightDistance = 2500.f;

protected:
	UPROPERTY(Transient)
	TWeakObjectPtr<UPaldarkPalCombatComponent> CombatRef;

	UPROPERTY(Transient)
	TWeakObjectPtr<UPaldarkPalPerceptionComponent> PerceptionRef;

	// Restored on ExitActivity so subsequent activities (Combat, Patrol) use
	// the species default speed instead of the stalk-reduced one.
	UPROPERTY(Transient)
	float SavedMaxWalkSpeed = 0.f;
};
