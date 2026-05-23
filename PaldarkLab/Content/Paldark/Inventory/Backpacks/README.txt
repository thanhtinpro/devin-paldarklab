PALDARK W37-38 — /Game/Paldark/Inventory/Backpacks/ directory.

This directory hosts the three backpack item data assets that drive the
W37-38 "Inventory full" outcome:

  - DA_Item_Backpack_T1.uasset (tag Paldark.Item.Backpack.T1).
  - DA_Item_Backpack_T2.uasset (tag Paldark.Item.Backpack.T2).
  - DA_Item_Backpack_T3.uasset (tag Paldark.Item.Backpack.T3).

All three are UPaldarkItemDefinition assets composed via fragments
(W11-12 pattern). The new W37-38 fragment is
`UPaldarkItemFragment_Backpack` which advertises MaxWeightBonusKg +
ExtraSlots. `UPaldarkPlayerInventoryComponent::SetEquippedBackpack` /
`GetMaxWeightKg` consume those values to bump the carry cap.

Designer authoring contract (W37-38):

  1. Author DA_Item_Backpack_T1 with the following fragments:
       UPaldarkItemFragment_Stackable  (MaxStackSize=1).
       UPaldarkItemFragment_Weight     (WeightKg=2.0).
       UPaldarkItemFragment_Equipable  (EquipSlot=Paldark.Equip.Slot.Backpack).
       UPaldarkItemFragment_Backpack   (BackpackTierTag=Paldark.Item.Backpack.T1,
                                        MaxWeightBonusKg=10.0,
                                        ExtraSlots=2).
     ItemTag = Paldark.Item.Backpack.T1.

  2. Repeat for T2 (MaxWeightBonusKg=25.0, ExtraSlots=5) and T3
     (MaxWeightBonusKg=45.0, ExtraSlots=10). Designer can rebalance
     these numbers freely — the component re-reads them on swap.

  3. Register every backpack DA with the AssetManager so
     `Paldark.Inventory.EquipBackpack T1` can resolve by tag. The
     PaldarkAssetManager `PaldarkItem` PrimaryAssetType already covers
     this — just place the DAs under `/Game/Paldark/Inventory/`.

  4. (Optional) For each backpack, give designers a visible mesh +
     attach socket. This is a Blueprint-side concern and lives on the
     Equipable fragment's `AttachSocketTag` (W11-12). Mesh authoring is
     not blocking for W37-38 outcome.

Test loop (UE Editor, after compile):
  1. Author the three DAs above.
  2. PIE Raid_Sandbox.
  3. `Paldark.Inventory.GiveAll`         — adds 1 of every item;
                                           weight bar should fill up to
                                           the default 30kg cap (T1 +
                                           ammo + medkits already
                                           encumber the player).
  4. `Paldark.Inventory.EquipBackpack T1` — cap bumps 30 → 40 kg.
  5. `Paldark.Inventory.EquipBackpack T2` — cap bumps 30 → 55 kg.
  6. `Paldark.Inventory.EquipBackpack T3` — cap bumps 30 → 75 kg.
  7. `Paldark.Inventory.EquipBackpack clear` — cap reverts to 30 kg.

Save-game persistence of the equipped backpack is W47 — for W37-38 the
slot lives in memory only and resets on match-end.
