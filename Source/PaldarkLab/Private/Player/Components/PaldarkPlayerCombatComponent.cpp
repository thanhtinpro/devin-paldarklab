#include "Player/Components/PaldarkPlayerCombatComponent.h"

#include "PaldarkLogCategories.h"

UPaldarkPlayerCombatComponent::UPaldarkPlayerCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}
