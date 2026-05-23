#include "Player/Components/PaldarkPlayerInteractionComponent.h"

#include "PaldarkLogCategories.h"

UPaldarkPlayerInteractionComponent::UPaldarkPlayerInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}
