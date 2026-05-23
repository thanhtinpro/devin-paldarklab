#include "Player/Components/PaldarkPlayerPalCompanionComponent.h"

#include "PaldarkLogCategories.h"

UPaldarkPlayerPalCompanionComponent::UPaldarkPlayerPalCompanionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}
