#include "Player/Components/PaldarkPlayerEquipmentComponent.h"

#include "PaldarkLogCategories.h"

UPaldarkPlayerEquipmentComponent::UPaldarkPlayerEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}
