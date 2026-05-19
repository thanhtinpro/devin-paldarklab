PALDARK W31-32 — /Game/Paldark/Maps/ directory.

This directory hosts:
  - .umap files for each playable map shipped (e.g. Map_RungHong, Raid_Sandbox).
  - DA_MapDef_<Name>.uasset data assets (UPaldarkMapDefinition) that pair a
    map identity tag (Paldark.Map.*) with the .umap soft ref + recommended
    POI / spawn-point counts.

Designer authoring contract (W31-32):
  1. Block out Map 2 "Rừng Hỏng" — 1×1 km landscape with:
     - 3 SafeZone POIs (spawn / staging area at edges).
     - 1 ExtractionPad POI (player exit beacon, central).
     - 2 Cache POIs (loot crates, mid-tier risk).
     - 1 RuinedTower POI (high-tier vantage point + Razorbird spawn).
     - 1 MiniBoss POI (Boltmane arena).
     Save as Map_RungHong.umap in this directory.
  2. Drop ~16 APaldarkPalSpawnPoint actors across the map:
     - 4 Direhound (Outpost cluster).
     - 4 Razorbird (RuinedTower aerial).
     - 2 Stoneclad (Cache guard).
     - 2 Vinewraith (forest border patrol).
     - 1 Boltmane (MiniBoss arena, no pack).
     - 3 spawn points reserved for companion DA_PalDef_Foxparks /
       DA_PalDef_Tombat fields (Q3 polish, leave bSpawnOnBeginPlay = false).
  3. Drop APaldarkPointOfInterest actors at each POI. Set POITag + DangerTier
     per the list above. Optionally link nearby spawn points via
     AssociatedPOITag for the validator hint.
  4. Author DA_MapDef_RungHong.uasset (UPaldarkMapDefinition):
     - MapTag = Paldark.Map.RungHong.
     - DisplayName = "Rừng Hỏng — Forest Ruins".
     - MapAsset = /Game/Paldark/Maps/Map_RungHong.
     - RecommendedPOICount = 8.
     - RecommendedSpawnPointCount = 16.
     - IntrinsicTagsToAssert = all 6 POI types + species tags expected on map.
  5. Wire the experience: open PX_RaidSandbox (or author a new
     PX_RungHongRaid) and set its MapDefinition = DA_MapDef_RungHong.

Verification:
  - `Paldark.Map.LoadDefinition DA_MapDef_RungHong` in PIE — should resolve
    MapTag + DisplayName + recommended counts.
  - `Paldark.POI.Dump` after PIE start — should list 8 POIs grouped by tier.
  - `Paldark.POI.HighlightTier High` — should draw debug spheres around the
    MiniBoss + RuinedTower POIs.

This directory ships as a placeholder in the C++ scaffold PR (W31-32). The
actual .umap + .uasset content is designer-authored work after the C++ scaffold
compiles.
