#include "Pal/Components/PaldarkPalHealthComponent.h"

#include "PaldarkLogCategories.h"

UPaldarkPalHealthComponent::UPaldarkPalHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}
