// PALDARK W27 — Lyra-style anim layer interface.
//
// Mirrors the way Lyra structures per-character animation overrides: the base
// AnimBP runs the locomotion state machine, but each character's AnimBP can
// implement layer functions that override the base "Idle" / "Move" / "Jump"
// poses with character-specific assets (e.g. Razorbird's flap loop vs the
// default biped idle).
//
// The interface is intentionally tiny — three BlueprintImplementableEvents.
// We do NOT declare them as `BlueprintNativeEvent` because there is no
// useful native default; every concrete AnimBP either overrides the layer or
// inherits the base state machine's pose.
//
// Usage from C++ (none in W27 — the base AnimBP calls the layers via the
// "Linked Anim Layer" node in the AnimGraph, designer-side). Listed here
// purely so the interface is part of the validated public API and other
// systems can later cast to it for diagnostics.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PaldarkAnimLayerInterface.generated.h"

UINTERFACE(BlueprintType, meta = (CannotImplementInterfaceInBlueprint = "false"))
class PALDARKLAB_API UPaldarkAnimLayerInterface : public UInterface
{
	GENERATED_BODY()
};

class PALDARKLAB_API IPaldarkAnimLayerInterface
{
	GENERATED_BODY()

public:
	// Idle pose layer. Default base AnimBP plays a standing idle; designer
	// AnimBP override plays e.g. a hostile-Pal alert idle.
	UFUNCTION(BlueprintImplementableEvent, Category = "Paldark|Anim|Layer")
	void Layer_LocomotionIdle();

	// Locomotion pose layer (walk + run blend). Designer AnimBP override
	// returns a blend space + curve compensation for stride length.
	UFUNCTION(BlueprintImplementableEvent, Category = "Paldark|Anim|Layer")
	void Layer_LocomotionMove();

	// Jump / falling pose layer. Designer AnimBP override returns an
	// in-air loop + a landing additive.
	UFUNCTION(BlueprintImplementableEvent, Category = "Paldark|Anim|Layer")
	void Layer_LocomotionJump();
};
