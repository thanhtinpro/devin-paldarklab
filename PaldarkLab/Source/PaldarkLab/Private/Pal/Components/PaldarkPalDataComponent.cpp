#include "Pal/Components/PaldarkPalDataComponent.h"

#include "PaldarkLogCategories.h"

UPaldarkPalDataComponent::UPaldarkPalDataComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}
