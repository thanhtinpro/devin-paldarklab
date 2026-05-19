#include "Inventory/Fragments/PaldarkItemFragment_Equipable.h"

FString UPaldarkItemFragment_Equipable::GetDebugDescription() const
{
	return FString::Printf(
		TEXT("Equipable(Slot=%s,Ability=%s)"),
		EquipSlot.IsValid() ? *EquipSlot.ToString() : TEXT("<none>"),
		AbilityToGrantOnEquip.IsNull() ? TEXT("<none>") : *AbilityToGrantOnEquip.ToString());
}
