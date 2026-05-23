#include "Inventory/Fragments/PaldarkItemFragment_Stackable.h"

FString UPaldarkItemFragment_Stackable::GetDebugDescription() const
{
	return FString::Printf(TEXT("Stackable(MaxStack=%d)"), GetMaxStackSize());
}
