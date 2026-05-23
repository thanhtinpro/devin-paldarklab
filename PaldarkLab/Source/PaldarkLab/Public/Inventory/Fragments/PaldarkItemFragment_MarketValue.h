// PALDARK W44-45 — Market value item fragment.
//
// Attached to a `UPaldarkItemDefinition` via its `Fragments` instanced array.
// Carries the per-item buy / sell pricing used by the marketplace kiosk + its
// controller component on the player's PlayerController:
//
//   - BuyPriceCredits  — price the player pays to acquire one unit. Compared
//                        against `Paldark.Item.Resource.Currency.Credits`
//                        stack size in the player's inventory.
//   - SellPriceCredits — price the player receives per unit handed over. By
//                        convention < BuyPriceCredits so designers can tune
//                        the buy/sell spread.
//   - bAllowBuy / bAllowSell — gate either direction independently. Some
//                              items (e.g. quest rewards) ship at price 0
//                              but should still not be sellable.
//
// Per-fragment composition keeps the price knob designer-tunable per item
// without bloating `UPaldarkItemDefinition` itself (Lyra-style). The
// marketplace controller looks the fragment up via
// `UPaldarkItemDefinition::FindFragmentByClass<UPaldarkItemFragment_MarketValue>()`
// at server transaction time; items with no fragment are treated as
// "not for sale" (Result.Fail.NotAllowed) so designer-renamed assets can't
// accidentally hit an undefined price column.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/PaldarkItemFragment.h"
#include "PaldarkItemFragment_MarketValue.generated.h"

UCLASS(DisplayName = "Market Value", meta = (DisplayName = "Market Value"))
class PALDARKLAB_API UPaldarkItemFragment_MarketValue : public UPaldarkItemFragment
{
	GENERATED_BODY()

public:
	// Authority cost the player pays to acquire one unit. Clamped at >= 0 in
	// the editor so designers can't author negative buy prices (which would
	// effectively pay the player to buy).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Inventory|MarketValue",
		meta = (ClampMin = "0", UIMin = "0"))
	int32 BuyPriceCredits = 100;

	// Authority cost the player receives per unit handed over. Convention is
	// SellPriceCredits < BuyPriceCredits so designers can tune the spread;
	// validator does not enforce that (some designer might run a "fair
	// trade" town with spread = 0).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Inventory|MarketValue",
		meta = (ClampMin = "0", UIMin = "0"))
	int32 SellPriceCredits = 50;

	// Gate the buy direction independently. False on quest rewards / unique
	// items so the catalog can still display them as "Trophy: cannot buy".
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Inventory|MarketValue")
	bool bAllowBuy = true;

	// Gate the sell direction independently. False on bind-on-pickup quest
	// items / Pal eggs so the player can't accidentally vendor them.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Inventory|MarketValue")
	bool bAllowSell = true;

	virtual FString GetDebugDescription() const override;
};
