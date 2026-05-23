// PALDARK W20-21 — Pal Patrol activity.
//
// Default Priority-15 fallback for hostile Pals. Walks the Pal between the
// designer-authored waypoints stored in `UPaldarkPalPatrolComponent`. Sits
// below Stalk (25) and Combat (40) — any perception threat preempts it.
//
// CanRun:
//   - The Pal has a `UPaldarkPalPatrolComponent` with at least one waypoint.
//   - The Pal has no current threat (perception is clear). Without this gate
//     Patrol would compete with Stalk + Combat purely on priority; the
//     explicit gate keeps DumpThreat output readable ("idle" vs "patrolling
//     while target visible").
//
// ShouldContinue: same as CanRun. Patrol "sticks" while there are no threats
//   and there are still waypoints — the activity itself handles wrap-around.
//
// EnterActivity:
//   - Pause the locomotion follow loop (Pal would otherwise leash toward
//     `FollowedPawn` if one was set — for hostile Pals, no follow target
//     exists, but the gate is cheap and defensive).
//
// TickActivity:
//   - Read `Patrol->GetCurrentWaypoint()` → planar delta toward it.
//   - If distance <= `Patrol->ArrivalRadius`, advance to the next waypoint
//     (wrap-around handled by the component).
//   - Otherwise `AddMovementInput(delta.normal(), 1.0)` on the character so
//     the character movement comp drives the actual locomotion.
//
// ExitActivity:
//   - Re-enable the follow loop (no-op for hostile Pals that don't follow).
//
// What this is NOT (deferred):
//   - No nav query — designers must place waypoints reachable by walking.
//     `MoveTo()` integration lands W30+ when designer levels grow past the
//     sandbox scale.
//   - No dwell timer at each waypoint — Pal advances immediately on arrival.
//     "Stand and look around" pose lands W22+ with animation montages.
//   - No replication — Patrol activity runs on authority only; clients see
//     the resulting `AddMovementInput` via ACharacter's transform stream.

#pragma once

#include "CoreMinimal.h"
#include "Pal/Activities/PaldarkBaseActivity.h"
#include "PaldarkActivity_Patrol.generated.h"

class UPaldarkPalPatrolComponent;

UCLASS()
class PALDARKLAB_API UPaldarkActivity_Patrol : public UPaldarkBaseActivity
{
	GENERATED_BODY()

public:
	UPaldarkActivity_Patrol();

	virtual bool InitActivity(UPaldarkPalActivityComponent* InComponent) override;

	virtual bool CanRun_Implementation() const override;
	virtual bool ShouldContinue_Implementation() const override;
	virtual void EnterActivity_Implementation() override;
	virtual void TickActivity_Implementation(float DeltaSeconds) override;
	virtual void ExitActivity_Implementation() override;

protected:
	// Cached on InitActivity so CanRun / Tick don't re-walk the component
	// graph each frame.
	UPROPERTY(Transient)
	TWeakObjectPtr<UPaldarkPalPatrolComponent> PatrolRef;
};
