// PALDARK W3-4 — UPaldarkPlayerNetworkComponent skeleton.
//
// Skeleton slot for player replication helpers (RPC routing, lag comp).
//
// Future home (do NOT implement here in W3-4):
//   W14-17 — server-side rewind + replication snapshot of player state.
//
// W3-4 keeps this class as a registered `UActorComponent` slot so the owning
// pawn's constructor can `CreateDefaultSubobject<UPaldarkPlayerNetworkComponent>(...)` and
// Blueprint authoring can extend the component without touching C++.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PaldarkPlayerNetworkComponent.generated.h"

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent), Abstract = false)
class PALDARKLAB_API UPaldarkPlayerNetworkComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkPlayerNetworkComponent();
};
