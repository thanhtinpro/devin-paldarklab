#include "Player/Components/PaldarkPlayerLocomotionExtComponent.h"

#include "PaldarkLogCategories.h"

UPaldarkPlayerLocomotionExtComponent::UPaldarkPlayerLocomotionExtComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}
