// PALDARK W3-4 — UPaldarkPlayerDamageComponent skeleton.
//
// Skeleton slot for damage receive + headshot / armor mitigation.
//
// Future home (do NOT implement here in W3-4):
//   W9-10 — receives GE_Damage events, applies armor + headshot multiplier.
//
// W3-4 keeps this class as a registered `UActorComponent` slot so the owning
// pawn's constructor can `CreateDefaultSubobject<UPaldarkPlayerDamageComponent>(...)` and
// Blueprint authoring can extend the component without touching C++.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PaldarkPlayerDamageComponent.generated.h"

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent), Abstract = false)
class PALDARKLAB_API UPaldarkPlayerDamageComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkPlayerDamageComponent();
};
