// PALDARK W11-12 — Equipable fragment.
//
// Declares that an item can be equipped to a body / loadout slot. The slot
// itself is a gameplay tag (Paldark.Equip.Slot.Primary / Consumable / Throwable
// — see PaldarkGameplayTags.h) so the inventory UI can filter equipables and
// future equipment components can route to the right socket.
//
// W11-12 scope: the fragment just records the slot. The actual
// equip-to-socket + animation pose binding lives in W17-18 weapon polish.
// AbilityToGrant is a soft pointer so an item asset doesn't drag a full GA
// class into memory just to sit in inventory — it's resolved on equip.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Inventory/PaldarkItemFragment.h"
#include "PaldarkItemFragment_Equipable.generated.h"

class UGameplayAbility;

UCLASS(DisplayName = "Equipable")
class PALDARKLAB_API UPaldarkItemFragment_Equipable : public UPaldarkItemFragment
{
	GENERATED_BODY()

public:
	// Slot the item targets. Should be one of the Paldark.Equip.Slot.* tags;
	// the inventory component does not currently validate the prefix (designer
	// discipline + future Asset Validator). An empty tag means the item can
	// be equipped to any slot (defensive default).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Inventory|Equipable", meta = (Categories = "Paldark.Equip.Slot"))
	FGameplayTag EquipSlot;

	// Optional GA class granted on equip. Soft pointer to avoid pulling the
	// full ability + its referenced GEs into memory while the item just sits
	// in inventory. W11-12 does NOT activate this — equip wiring is W17-18.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Inventory|Equipable")
	TSoftClassPtr<UGameplayAbility> AbilityToGrantOnEquip;

	virtual FString GetDebugDescription() const override;
};
