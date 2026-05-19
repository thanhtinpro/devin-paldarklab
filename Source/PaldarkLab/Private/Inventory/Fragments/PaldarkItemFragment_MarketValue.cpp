// PALDARK W44-45 — Market value item fragment implementation.

#include "Inventory/Fragments/PaldarkItemFragment_MarketValue.h"

FString UPaldarkItemFragment_MarketValue::GetDebugDescription() const
{
	return FString::Printf(
		TEXT("MarketValue(Buy=%dc%s,Sell=%dc%s)"),
		BuyPriceCredits,
		bAllowBuy ? TEXT("") : TEXT("/locked"),
		SellPriceCredits,
		bAllowSell ? TEXT("") : TEXT("/locked"));
}
