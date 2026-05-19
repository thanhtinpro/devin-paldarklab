// PALDARK W5-6 — Pal Investigate activity.
//
// CanRun when the activity component holds an active investigate request
// (set via `UPaldarkPalActivityComponent::RequestInvestigate(Location, Radius)`,
// driven in W5-6 by the `Paldark.Pal.Ping` console command).
//
// Behaviour:
//   - Enter: pauses the locomotion follow loop so the leash doesn't yank the
//     Pal back toward the leader.
//   - Tick: planar distance to the request location → if greater than
//     `ArrivalRadius`, `AddMovementInput(Dir)` on the owning character;
//     otherwise it clears the request on the component and lets the FSM
//     fall back to Follow / Idle.
//   - Times out after `MaxInvestigateTime` seconds regardless of distance,
//     so a stuck Pal returns to the leader instead of dancing forever.
//   - Exit: re-enables the locomotion follow loop.
//
// W5-6 keeps this very simple — no path requests, no LOS checks, no
// "search circle" patrolling. Those land in W18+ alongside Perception /
// Custom Sense.

#pragma once

#include "CoreMinimal.h"
#include "Pal/Activities/PaldarkBaseActivity.h"
#include "PaldarkActivity_Investigate.generated.h"

UCLASS()
class PALDARKLAB_API UPaldarkActivity_Investigate : public UPaldarkBaseActivity
{
	GENERATED_BODY()

public:
	UPaldarkActivity_Investigate();

	virtual bool CanRun_Implementation() const override;
	virtual bool ShouldContinue_Implementation() const override;
	virtual void EnterActivity_Implementation() override;
	virtual void TickActivity_Implementation(float DeltaSeconds) override;
	virtual void ExitActivity_Implementation() override;

	// Arrival tolerance (cm). Once the Pal is within this distance, the
	// investigate request is cleared and the FSM falls back.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Activity")
	float ArrivalRadius = 150.0f;

	// Hard timeout (seconds). Prevents a stuck Pal from never returning to
	// the leader.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Activity")
	float MaxInvestigateTime = 8.0f;

protected:
	// Cleared on Enter, ticked up in Tick.
	UPROPERTY(Transient)
	float ElapsedActivityTime = 0.0f;
};
