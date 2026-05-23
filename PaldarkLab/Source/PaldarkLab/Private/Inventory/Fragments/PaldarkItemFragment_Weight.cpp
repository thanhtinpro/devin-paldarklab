#include "Inventory/Fragments/PaldarkItemFragment_Weight.h"

FString UPaldarkItemFragment_Weight::GetDebugDescription() const
{
	return FString::Printf(TEXT("Weight(%.3fkg/unit)"), GetWeightKgPerUnit());
}
