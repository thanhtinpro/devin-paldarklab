#include "Player/Components/PaldarkPlayerHealthComponent.h"

#include "PaldarkLogCategories.h"

UPaldarkPlayerHealthComponent::UPaldarkPlayerHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}
