// PALDARK W11-12 — Weight fragment.
//
// Declares an item's weight contribution per single unit (one copy, NOT per
// stack). The inventory component's encumbrance math multiplies WeightKgPerUnit
// by the stack count to compute total weight contribution — so an ammo stack
// of 60 × 0.05kg = 3kg, not 0.05kg.
//
// Items WITHOUT this fragment contribute 0kg to the inventory's running total
// (a deliberate weightless default — handy for placeholder / quest items that
// shouldn't count against carrying capacity).

#pragma once

#include "CoreMinimal.h"
#include "Inventory/PaldarkItemFragment.h"
#include "PaldarkItemFragment_Weight.generated.h"

UCLASS(DisplayName = "Weight")
class PALDARKLAB_API UPaldarkItemFragment_Weight : public UPaldarkItemFragment
{
	GENERATED_BODY()

public:
	// Per-unit (per-copy) weight contribution in kilograms. Clamped to >=0 at
	// access time so a designer-authored negative weight can't refund encumbrance.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Inventory|Weight", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "Kilograms"))
	float WeightKgPerUnit = 0.0f;

	float GetWeightKgPerUnit() const { return FMath::Max(WeightKgPerUnit, 0.0f); }

	virtual FString GetDebugDescription() const override;
};
