// PALDARK W11-12 — Stackable fragment.
//
// Declares that an item can occupy a single inventory entry with `MaxStackSize`
// copies. Items WITHOUT this fragment are treated as MaxStackSize=1 (one entry
// per copy, like a Pistol). Stack splitting / merging is handled by
// UPaldarkPlayerInventoryComponent at AddItem time — the fragment just carries
// the cap.
//
// Authoring tip: keep MaxStackSize at the upper bound of what the gameplay
// economy allows (e.g. Ammo_9mm=120 because a magazine holds 17 and we want
// ~7 mags carried). The inventory UI shows current/max so partial stacks are
// always visible — there's no "infinite stack" sentinel.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/PaldarkItemFragment.h"
#include "PaldarkItemFragment_Stackable.generated.h"

UCLASS(DisplayName = "Stackable")
class PALDARKLAB_API UPaldarkItemFragment_Stackable : public UPaldarkItemFragment
{
	GENERATED_BODY()

public:
	// Max number of items that can occupy a single inventory entry. Clamped
	// to >= 1 at access time so a malformed asset still behaves like a
	// non-stackable item rather than crashing on divide-by-zero.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Inventory|Stackable", meta = (ClampMin = "1", UIMin = "1"))
	int32 MaxStackSize = 1;

	int32 GetMaxStackSize() const { return FMath::Max(MaxStackSize, 1); }

	virtual FString GetDebugDescription() const override;
};
