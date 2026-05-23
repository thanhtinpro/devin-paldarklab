#include "Pal/Components/PaldarkPalAnimDriverComponent.h"

#include "PaldarkLogCategories.h"

UPaldarkPalAnimDriverComponent::UPaldarkPalAnimDriverComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}
