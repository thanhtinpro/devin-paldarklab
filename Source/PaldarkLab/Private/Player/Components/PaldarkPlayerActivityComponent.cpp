#include "Player/Components/PaldarkPlayerActivityComponent.h"

#include "PaldarkLogCategories.h"

UPaldarkPlayerActivityComponent::UPaldarkPlayerActivityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}
