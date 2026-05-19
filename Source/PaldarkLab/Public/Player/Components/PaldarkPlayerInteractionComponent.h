// PALDARK W3-4 — UPaldarkPlayerInteractionComponent skeleton.
//
// Skeleton slot for interaction sphere + IInteractInterface dispatch.
//
// Future home (do NOT implement here in W3-4):
//   W11-12 — interaction trace; pickups, doors, Pal Sphere capture.
//
// W3-4 keeps this class as a registered `UActorComponent` slot so the owning
// pawn's constructor can `CreateDefaultSubobject<UPaldarkPlayerInteractionComponent>(...)` and
// Blueprint authoring can extend the component without touching C++.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PaldarkPlayerInteractionComponent.generated.h"

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent), Abstract = false)
class PALDARKLAB_API UPaldarkPlayerInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkPlayerInteractionComponent();
};
