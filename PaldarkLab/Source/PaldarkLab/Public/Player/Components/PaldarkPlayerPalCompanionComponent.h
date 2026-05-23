// PALDARK W3-4 — UPaldarkPlayerPalCompanionComponent skeleton.
//
// Skeleton slot tracking the player's active Pal companion(s).
//
// Future home (do NOT implement here in W3-4):
//   W3-4+ — caches APaldarkPalCharacter instances; broadcasts ping/command events.
//
// W3-4 keeps this class as a registered `UActorComponent` slot so the owning
// pawn's constructor can `CreateDefaultSubobject<UPaldarkPlayerPalCompanionComponent>(...)` and
// Blueprint authoring can extend the component without touching C++.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PaldarkPlayerPalCompanionComponent.generated.h"

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent), Abstract = false)
class PALDARKLAB_API UPaldarkPlayerPalCompanionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkPlayerPalCompanionComponent();
};
