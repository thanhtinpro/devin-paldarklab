// PALDARK W5-6 — Pal Idle activity.
//
// Lowest-priority fallback. Always wants to run (CanRun = true) so the FSM
// always has a state to fall back to. On Enter it disables the locomotion
// follow loop so the Pal stops moving even if its `FollowedPawn` is still
// set. On Exit it re-enables the loop so the next activity (Follow,
// Investigate, ...) can resume.

#pragma once

#include "CoreMinimal.h"
#include "Pal/Activities/PaldarkBaseActivity.h"
#include "PaldarkActivity_Idle.generated.h"

UCLASS()
class PALDARKLAB_API UPaldarkActivity_Idle : public UPaldarkBaseActivity
{
	GENERATED_BODY()

public:
	UPaldarkActivity_Idle();

	virtual bool CanRun_Implementation() const override { return true; }
	virtual void EnterActivity_Implementation() override;
	virtual void ExitActivity_Implementation() override;
};
