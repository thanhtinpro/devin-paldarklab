#include "Inventory/Fragments/PaldarkItemFragment_Backpack.h"

FString UPaldarkItemFragment_Backpack::GetDebugDescription() const
{
	return FString::Printf(
		TEXT("Backpack(Tier=%s,+%.1fkg,+%dslots)"),
		BackpackTierTag.IsValid() ? *BackpackTierTag.ToString() : TEXT("<none>"),
		GetMaxWeightBonusKg(),
		GetExtraSlots());
}
