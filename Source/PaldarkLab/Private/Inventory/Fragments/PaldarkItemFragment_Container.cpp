#include "Inventory/Fragments/PaldarkItemFragment_Container.h"

FString UPaldarkItemFragment_Container::GetDebugDescription() const
{
	return FString::Printf(
		TEXT("Container(InnerCap=%.1fkg,%dslots)"),
		GetInnerMaxWeightKg(),
		GetInnerMaxSlots());
}
