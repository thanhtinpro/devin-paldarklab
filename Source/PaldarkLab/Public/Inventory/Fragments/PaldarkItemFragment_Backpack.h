// PALDARK W37-38 — Backpack fragment.
//
// Declares that an item, when equipped to `Paldark.Equip.Slot.Backpack`, bumps
// the carrying player's encumbrance cap by `MaxWeightBonusKg` and grants
// `ExtraSlots` virtual rows on top of the inventory's base slot allotment.
// Three tiers are authored by the asset team:
//
//   DA_Item_Backpack_T1  → Tactical Vest   +10 kg / +4 slots
//   DA_Item_Backpack_T2  → Field Pack      +25 kg / +8 slots
//   DA_Item_Backpack_T3  → Ranger Rig      +50 kg / +16 slots
//
// `BackpackTierTag` identifies the tier for save-game + console-command
// lookup (`Paldark.Inventory.EquipBackpack T1/T2/T3`). Inventory weight cap
// is recomputed on `UPaldarkPlayerInventoryComponent::SetEquippedBackpack`
// — there is no need to listen for stat changes mid-equip.
//
// The fragment does NOT spawn a skeletal-mesh attach on the player's back;
// that's a W50 UI polish + art track item. The inventory layer is the only
// gameplay consumer in W37-38.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Inventory/PaldarkItemFragment.h"
#include "PaldarkItemFragment_Backpack.generated.h"

UCLASS(DisplayName = "Backpack")
class PALDARKLAB_API UPaldarkItemFragment_Backpack : public UPaldarkItemFragment
{
	GENERATED_BODY()

public:
	// Tier identity tag. Should be one of `Paldark.Item.Backpack.T{1,2,3}`.
	// Used by `Paldark.Inventory.EquipBackpack` to find the matching item
	// definition without designer renaming the asset path.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Inventory|Backpack",
		meta = (Categories = "Paldark.Item.Backpack"))
	FGameplayTag BackpackTierTag;

	// Kilograms added on top of `UPaldarkPlayerInventoryComponent::BaseMaxWeightKg`
	// while this backpack is equipped. Clamped to >= 0 at access time so a
	// malformed asset cannot refund encumbrance.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Inventory|Backpack",
		meta = (ClampMin = "0.0", UIMin = "0.0", Units = "Kilograms"))
	float MaxWeightBonusKg = 0.0f;

	// Extra virtual slots granted on top of the inventory's base slot count.
	// W37-38 inventory does not enforce a hard slot cap (weight is the cap),
	// but the value is replicated + read by future UMG grid widget code in
	// W50 to size the grid.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Inventory|Backpack",
		meta = (ClampMin = "0", UIMin = "0"))
	int32 ExtraSlots = 0;

	float GetMaxWeightBonusKg() const { return FMath::Max(MaxWeightBonusKg, 0.0f); }
	int32 GetExtraSlots() const { return FMath::Max(ExtraSlots, 0); }

	virtual FString GetDebugDescription() const override;
};
