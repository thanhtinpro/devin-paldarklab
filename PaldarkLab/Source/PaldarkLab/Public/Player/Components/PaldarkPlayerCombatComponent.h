// PALDARK W3-4 — UPaldarkPlayerCombatComponent skeleton.
//
// Skeleton slot for player weapon + targeting.
//
// Future home (do NOT implement here in W3-4):
//   W9-10 Damage chain — owns active weapon, fires via GE_Damage_Standard.
//
// W3-4 keeps this class as a registered `UActorComponent` slot so the owning
// pawn's constructor can `CreateDefaultSubobject<UPaldarkPlayerCombatComponent>(...)` and
// Blueprint authoring can extend the component without touching C++.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PaldarkPlayerCombatComponent.generated.h"

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent), Abstract = false)
class PALDARKLAB_API UPaldarkPlayerCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkPlayerCombatComponent();
};
