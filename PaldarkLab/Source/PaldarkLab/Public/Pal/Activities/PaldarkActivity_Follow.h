// PALDARK W5-6 — Pal Follow activity.
//
// CanRun when:
//   - The Pal has a valid `FollowedPawn` on its locomotion slot, AND
//   - The planar distance to that pawn is > `EnterDistance` (default 700cm).
//
// ShouldContinue uses a smaller `ExitDistance` (default 450cm) so the FSM
// hysteresises around the follow threshold and doesn't flicker Idle↔Follow
// when the Pal is hovering near `TargetFollowDistance` on the locomotion
// component (default 500cm).
//
// On Enter it re-enables the locomotion follow loop; on Exit it leaves the
// loop enabled (Idle disables it). Tick does nothing — the locomotion
// component drives the actual `AddMovementInput` call.

#pragma once

#include "CoreMinimal.h"
#include "Pal/Activities/PaldarkBaseActivity.h"
#include "PaldarkActivity_Follow.generated.h"

UCLASS()
class PALDARKLAB_API UPaldarkActivity_Follow : public UPaldarkBaseActivity
{
	GENERATED_BODY()

public:
	UPaldarkActivity_Follow();

	virtual bool CanRun_Implementation() const override;
	virtual bool ShouldContinue_Implementation() const override;
	virtual void EnterActivity_Implementation() override;

	// Distance (cm) above which Follow wants to start. Designers override per
	// species (e.g. ranged-combat Pal stays further away).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Activity")
	float EnterDistance = 700.0f;

	// Distance (cm) below which Follow stops running (hysteresis). Must be
	// smaller than `EnterDistance`.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Activity")
	float ExitDistance = 450.0f;
};
