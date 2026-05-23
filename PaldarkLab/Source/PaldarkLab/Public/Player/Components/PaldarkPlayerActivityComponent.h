// PALDARK W3-4 — UPaldarkPlayerActivityComponent skeleton.
//
// Skeleton slot for player Activity FSM (RoN-style).
//
// Future home (do NOT implement here in W3-4):
//   W5-6 — ports BaseActivity FSM, ticks one activity per pawn.
//
// W3-4 keeps this class as a registered `UActorComponent` slot so the owning
// pawn's constructor can `CreateDefaultSubobject<UPaldarkPlayerActivityComponent>(...)` and
// Blueprint authoring can extend the component without touching C++.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PaldarkPlayerActivityComponent.generated.h"

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent), Abstract = false)
class PALDARKLAB_API UPaldarkPlayerActivityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkPlayerActivityComponent();
};
