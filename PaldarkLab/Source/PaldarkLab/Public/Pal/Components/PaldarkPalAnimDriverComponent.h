// PALDARK W3-4 — UPaldarkPalAnimDriverComponent skeleton.
//
// Skeleton slot exposing locomotion blend-space + montage helpers.
//
// Future home (do NOT implement here in W3-4):
//   W3-4 — handed off to the Pal's AnimBP via blueprint reads.
//
// W3-4 keeps this class as a registered `UActorComponent` slot so the owning
// pawn's constructor can `CreateDefaultSubobject<UPaldarkPalAnimDriverComponent>(...)` and
// Blueprint authoring can extend the component without touching C++.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PaldarkPalAnimDriverComponent.generated.h"

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent), Abstract = false)
class PALDARKLAB_API UPaldarkPalAnimDriverComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkPalAnimDriverComponent();
};
