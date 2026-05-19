#include "Player/Components/PaldarkPlayerCameraExtensionComponent.h"

#include "PaldarkLogCategories.h"

UPaldarkPlayerCameraExtensionComponent::UPaldarkPlayerCameraExtensionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}
