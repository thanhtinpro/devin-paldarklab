// PALDARK W3-4 — UPaldarkPlayerEquipmentComponent skeleton.
//
// Skeleton slot for equipped items (armor / backpack tier).
//
// Future home (do NOT implement here in W3-4):
//   W11-12 — paired with Inventory; equip flow goes through quick-bar slots.
//
// W3-4 keeps this class as a registered `UActorComponent` slot so the owning
// pawn's constructor can `CreateDefaultSubobject<UPaldarkPlayerEquipmentComponent>(...)` and
// Blueprint authoring can extend the component without touching C++.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PaldarkPlayerEquipmentComponent.generated.h"

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent), Abstract = false)
class PALDARKLAB_API UPaldarkPlayerEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkPlayerEquipmentComponent();
};
