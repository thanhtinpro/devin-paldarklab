// PALDARK W3-4 — UPaldarkPalDataComponent skeleton.
//
// Skeleton slot referencing this Pal's PrimaryDataAsset (species def).
//
// Future home (do NOT implement here in W3-4):
//   W27-28 AssetManager — async loads Pal species data on spawn.
//
// W3-4 keeps this class as a registered `UActorComponent` slot so the owning
// pawn's constructor can `CreateDefaultSubobject<UPaldarkPalDataComponent>(...)` and
// Blueprint authoring can extend the component without touching C++.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PaldarkPalDataComponent.generated.h"

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent), Abstract = false)
class PALDARKLAB_API UPaldarkPalDataComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkPalDataComponent();
};
