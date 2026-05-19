#include "Player/Components/PaldarkPlayerNetworkComponent.h"

#include "PaldarkLogCategories.h"

UPaldarkPlayerNetworkComponent::UPaldarkPlayerNetworkComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}
