#include "Player/Components/PaldarkPlayerStaminaComponent.h"

#include "PaldarkLogCategories.h"

UPaldarkPlayerStaminaComponent::UPaldarkPlayerStaminaComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}
