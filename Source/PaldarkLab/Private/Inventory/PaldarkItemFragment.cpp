#include "Inventory/PaldarkItemFragment.h"

FString UPaldarkItemFragment::GetDebugDescription() const
{
	return GetClass() != nullptr ? GetClass()->GetName() : TEXT("UPaldarkItemFragment");
}
