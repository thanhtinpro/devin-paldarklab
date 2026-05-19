// PALDARK W3-4 — UPaldarkPlayerCameraExtensionComponent skeleton.
//
// Skeleton slot for camera mode stack (ADS, cinematic, vehicle).
//
// Future home (do NOT implement here in W3-4):
//   W14+ — feature-mode camera blending on top of the spring arm.
//
// W3-4 keeps this class as a registered `UActorComponent` slot so the owning
// pawn's constructor can `CreateDefaultSubobject<UPaldarkPlayerCameraExtensionComponent>(...)` and
// Blueprint authoring can extend the component without touching C++.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PaldarkPlayerCameraExtensionComponent.generated.h"

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent), Abstract = false)
class PALDARKLAB_API UPaldarkPlayerCameraExtensionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkPlayerCameraExtensionComponent();
};
