// PALDARK W3-4 — UPaldarkPlayerLocomotionExtComponent skeleton.
//
// Skeleton slot for movement extensions on top of CharacterMovement.
//
// Future home (do NOT implement here in W3-4):
//   W7-8 — sprint, crouch, mantle handlers gated by stamina.
//
// W3-4 keeps this class as a registered `UActorComponent` slot so the owning
// pawn's constructor can `CreateDefaultSubobject<UPaldarkPlayerLocomotionExtComponent>(...)` and
// Blueprint authoring can extend the component without touching C++.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PaldarkPlayerLocomotionExtComponent.generated.h"

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent), Abstract = false)
class PALDARKLAB_API UPaldarkPlayerLocomotionExtComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkPlayerLocomotionExtComponent();
};
