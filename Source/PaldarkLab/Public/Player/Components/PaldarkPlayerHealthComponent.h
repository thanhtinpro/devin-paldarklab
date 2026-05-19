// PALDARK W3-4 — UPaldarkPlayerHealthComponent skeleton.
//
// Skeleton slot for player health/damage book-keeping.
//
// Future home (do NOT implement here in W3-4):
//   W7-8 GAS — proxies AttributeSet.Health changes onto HUD + death events.
//
// W3-4 keeps this class as a registered `UActorComponent` slot so the owning
// pawn's constructor can `CreateDefaultSubobject<UPaldarkPlayerHealthComponent>(...)` and
// Blueprint authoring can extend the component without touching C++.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PaldarkPlayerHealthComponent.generated.h"

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent), Abstract = false)
class PALDARKLAB_API UPaldarkPlayerHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkPlayerHealthComponent();
};
