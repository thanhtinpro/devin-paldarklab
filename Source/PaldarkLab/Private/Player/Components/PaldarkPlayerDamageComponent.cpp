#include "Player/Components/PaldarkPlayerDamageComponent.h"

#include "PaldarkLogCategories.h"

UPaldarkPlayerDamageComponent::UPaldarkPlayerDamageComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}
