// PALDARK W11-12 — Inventory item fragment base.
//
// Vendor-neutral port of the Lyra / Udemy [09] inventory fragment system:
// instead of cramming all per-item data onto one monolithic class (weight,
// stack size, ammo type, equip slot, ability to grant, …), an authored
// UPaldarkItemDefinition owns a TArray of these `UPaldarkItemFragment`
// subobjects. Each fragment carries one axis of behaviour, so designers can
// compose new items by mixing fragments without touching C++:
//
//   DA_Item_Pistol         → [Equipable(Primary), Weight(1.0)]
//   DA_Item_Ammo_9mm       → [Stackable(MaxStack=120), Weight(0.05)]
//   DA_Item_Bandage        → [Stackable(MaxStack=8),   Weight(0.2),  Equipable(Consumable)]
//   DA_Item_Pal_Sphere     → [Stackable(MaxStack=10),  Weight(0.3),  Equipable(Throwable)]
//   DA_Item_Energy_Drink   → [Stackable(MaxStack=4),   Weight(0.15), Equipable(Consumable)]
//
// W11-12 scope: fragments are pure data — runtime hooks (OnAddedToInstance,
// OnEquipped, etc.) are stubs for now. The Inventory component reads fragment
// fields synchronously via `FindFragmentByClass<T>()` on the item definition;
// it does NOT call virtual fragment methods at runtime yet (that's W17-18
// weapon polish + W21+ UI binding work).

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PaldarkItemFragment.generated.h"

class UPaldarkItemDefinition;

UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced, CollapseCategories)
class PALDARKLAB_API UPaldarkItemFragment : public UObject
{
	GENERATED_BODY()

public:
	// Short, human-readable name used by the inventory dump console command and
	// the future tooltip widget. Defaults to the fragment class name; concrete
	// subclasses override when a more useful summary fits (e.g. "Stackable
	// (1/8)" for a stackable fragment with a stack count).
	virtual FString GetDebugDescription() const;
};
