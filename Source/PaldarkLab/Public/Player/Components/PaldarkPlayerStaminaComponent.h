// PALDARK W3-4 — UPaldarkPlayerStaminaComponent skeleton.
//
// Skeleton slot for player stamina (sprint / climb cost).
//
// Future home (do NOT implement here in W3-4):
//   W7-8 GAS — drives stamina drain via GA_Sprint, regen via GE_StaminaRegen.
//
// W3-4 keeps this class as a registered `UActorComponent` slot so the owning
// pawn's constructor can `CreateDefaultSubobject<UPaldarkPlayerStaminaComponent>(...)` and
// Blueprint authoring can extend the component without touching C++.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PaldarkPlayerStaminaComponent.generated.h"

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent), Abstract = false)
class PALDARKLAB_API UPaldarkPlayerStaminaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkPlayerStaminaComponent();
};
