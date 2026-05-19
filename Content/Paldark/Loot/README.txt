PALDARK W33-34 — /Game/Paldark/Loot/ directory.

This directory hosts:
  - DA_LootTable_<Name>.uasset data assets (UPaldarkLootTable) that pair a
    loot table identity tag (Paldark.LootTable.*) with a weighted entries
    list (ItemDefinition soft ref + Weight + Min/MaxCount + optional
    GuaranteedWhenTag).
  - Companion DA_Item_Tech_<Name>.uasset items (UPaldarkItemDefinition) for
    the tech-themed drop pool (Battery, Circuit, Coolant, Polymer,
    Microchip). Items can technically live under /Game/Paldark/Items/
    instead — designer convention.

Designer authoring contract (W33-34):
  1. Author 5 tech-themed item data assets under /Game/Paldark/Items/:
       DA_Item_Tech_Battery       (tag Paldark.Item.Tech.Battery)
       DA_Item_Tech_Circuit       (tag Paldark.Item.Tech.Circuit)
       DA_Item_Tech_Coolant       (tag Paldark.Item.Tech.Coolant)
       DA_Item_Tech_Polymer       (tag Paldark.Item.Tech.Polymer)
       DA_Item_Tech_Microchip     (tag Paldark.Item.Tech.Microchip)
     Weight + Stack/Equipment fragments per W11-12 convention.
  2. Author DA_LootTable_HostileStandard.uasset in this directory:
       LootTableTag = Paldark.LootTable.HostileStandard.
       MaxRolls = 1.
       Entries:
         - Battery   Weight=60 Min=1 Max=2
         - Circuit   Weight=30 Min=1 Max=1
         - Coolant   Weight=20 Min=1 Max=1
         - Polymer   Weight=20 Min=1 Max=1
         - Microchip Weight= 5 Min=1 Max=1
  3. Author DA_LootTable_Boss.uasset (Boltmane / mini-boss pool):
       LootTableTag = Paldark.LootTable.Boss.
       MaxRolls = 3.
       Entries:
         - Battery   Weight=40 Min=2 Max=4
         - Circuit   Weight=30 Min=1 Max=2
         - Coolant   Weight=25 Min=1 Max=2
         - Polymer   Weight=25 Min=1 Max=2
         - Microchip Weight=15 Min=1 Max=1
                     GuaranteedWhenTag = Paldark.Pal.Species.Boltmane
     The Microchip row's GuaranteedWhenTag ensures every Boltmane kill
     yields at least one Microchip (in addition to the weighted picks).
  4. Wire LootTableId on hostile-Pal Blueprint subclasses:
       BP_Pal_Direhound  -> LootDropSlot.LootTableId = PaldarkLootTable:DA_LootTable_HostileStandard.
       BP_Pal_Razorbird  -> same.
       BP_Pal_Stoneclad  -> same.
       BP_Pal_Vinewraith -> same.
       BP_Pal_Boltmane   -> LootDropSlot.LootTableId = PaldarkLootTable:DA_LootTable_Boss.
  5. Block out Map_PalCorp.umap under /Game/Paldark/Maps/ (indoor close-
     quarter, ~600×600 m). Drop:
       - 4× APaldarkPointOfInterest with POITag=Paldark.POI.Type.IndoorRoom.
       - 1× APaldarkPointOfInterest with POITag=Paldark.POI.Type.SafeZone.
       - 1× APaldarkPointOfInterest with POITag=Paldark.POI.Type.ExtractionPad.
       - Spawn points referencing the hostile-Pal Blueprint subclasses
         above.
     Author DA_MapDef_PalCorp:
       MapTag = Paldark.Map.PalCorp.
       DisplayName = "Cơ Xưởng PalCorp — Indoor Facility".
       MapAsset = /Game/Paldark/Maps/Map_PalCorp.
       bIsIndoor = true.
       RecommendedPlayerCount = 4.
       RecommendedPOICount = 6.
       RecommendedSpawnPointCount = 12.

Verification:
  - `Paldark.Loot.DumpTable DA_LootTable_HostileStandard` in PIE — should
    print the weighted entries with their resolved percentages.
  - `Paldark.Loot.ForceDrop Direhound_0` in PIE — should spawn an
    APaldarkLootBag at the Direhound's location populated with rolled tech
    items, bypassing the death pipeline.
  - Walk over the bag — `Paldark.Inventory.List` should now show the
    transferred entries, and the bag destroys itself when empty.
  - Kill a Boltmane via standard damage — drop bag must contain at least
    one Microchip thanks to GuaranteedWhenTag.
  - `Paldark.Map.LoadDefinition DA_MapDef_PalCorp` — should resolve
    MapTag=Paldark.Map.PalCorp + bIsIndoor=true.

This directory ships as a placeholder in the C++ scaffold PR (W33-34). The
actual .uasset content is designer-authored work after the C++ scaffold
compiles.
