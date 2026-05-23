// PALDARK W3-4 — UPaldarkPalHealthComponent skeleton.
//
// Skeleton slot for Pal health + KO state.
//
// Future home (do NOT implement here in W3-4):
//   W7-8 GAS — mirrors player health component, ticks death/respawn flow.
//
// W3-4 keeps this class as a registered `UActorComponent` slot so the owning
// pawn's constructor can `CreateDefaultSubobject<UPaldarkPalHealthComponent>(...)` and
// Blueprint authoring can extend the component without touching C++.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PaldarkPalHealthComponent.generated.h"

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent), Abstract = false)
class PALDARKLAB_API UPaldarkPalHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkPalHealthComponent();
};
