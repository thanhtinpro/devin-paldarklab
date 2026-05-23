// PALDARK W33-34 — Loot table (UPrimaryDataAsset).
//
// One DataAsset per drop pool under `/Game/Paldark/Loot/DA_LootTable_<Name>`.
// Carries a tagged identity (`Paldark.LootTable.*`) plus a list of weighted
// entries. The W33-34 hostile-Pal drop flow looks like:
//
//   1. Designer authors `DA_LootTable_HostileStandard` with N entries
//      (ItemDefinition soft ref + Weight + MinCount + MaxCount + optional
//      GuaranteedWhenTag).
//   2. Designer wires `LootTableId` on the hostile Pal subclass (Direhound /
//      Razorbird / Stoneclad / Vinewraith / Boltmane) via the Pal's
//      `LootDropSlot` component default property.
//   3. On Pal death (server, `UPaldarkAttributeSet::OnHealthZeroed`), the
//      `UPaldarkLootDropComponent` async-loads the table, rolls entries,
//      and spawns one `APaldarkLootBag` carrying the rolled items.
//
// Boss-style "guaranteed drop" is supported via `GuaranteedWhenTag`: when
// the Pal's `SpeciesTag` matches this tag (or the GE source carries it),
// the entry skips the weight roll and always drops at MaxCount. Designer
// wires this for Boltmane to guarantee a Microchip drop.
//
// Registered as primary asset type `PaldarkLootTable` via
// `Config/DefaultGame.ini`. Primary asset id is
// `PaldarkLootTable:<RowName>` (Lyra convention).
//
// What this is NOT (deferred):
//   - No rarity tier metadata (Common/Rare/Epic) — W37-38 inventory polish
//     gets a `FPaldarkItemRarity` enum + per-entry rarity tag.
//   - No per-player-level scaling — Q4 balance pass.
//   - No level-bag spawn override (e.g. "always drop a key on this POI") —
//     authoring lives on the spawn point / POI metadata in W31-32, not here.
//   - No FastArraySerializer for `Entries` — the loot table is a data asset
//     loaded once per Pal-death, not a replicated runtime collection.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PaldarkLootTable.generated.h"

class UPaldarkItemDefinition;

// One weighted row in a loot table. ItemDefinition is a soft pointer so the
// loot table can be loaded without paying for every referenced item; the
// loot-drop component async-resolves the picked entry's item when it
// spawns the loot bag.
USTRUCT(BlueprintType)
struct PALDARKLAB_API FPaldarkLootEntry
{
	GENERATED_BODY()

	// Item to drop when this entry is rolled. Soft ref so the table doesn't
	// root-load every item asset on AssetManager init.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Loot|Entry")
	TSoftObjectPtr<UPaldarkItemDefinition> ItemDefinition;

	// Relative weight inside the table. The roll picks an entry with
	// probability `Weight / Sum(Weight)`. Designer convention: 100 = baseline,
	// 50 = half, 10 = rare. Zero means "never rolled" but useful for keeping
	// an entry visible in the asset for diffing.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Loot|Entry",
		meta = (ClampMin = "0", UIMin = "0"))
	int32 Weight = 100;

	// Minimum number of copies dropped when this entry is picked. Roll
	// is uniform over [MinCount, MaxCount] inclusive. Equal MinCount /
	// MaxCount means "always exactly N copies".
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Loot|Entry",
		meta = (ClampMin = "1", UIMin = "1"))
	int32 MinCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Loot|Entry",
		meta = (ClampMin = "1", UIMin = "1"))
	int32 MaxCount = 1;

	// Optional guarantee tag. When the dying Pal carries this tag on its
	// SpeciesTag (or the GE source's owner does), this entry is *always*
	// rolled in addition to the regular weighted picks, and drops at
	// MaxCount. Designer wires `Paldark.Pal.Species.Boltmane` here on the
	// Microchip row in `DA_LootTable_Boss` so every Boltmane kill yields
	// a Microchip. Empty = no guarantee, entry rolls normally.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Loot|Entry",
		meta = (Categories = "Paldark"))
	FGameplayTag GuaranteedWhenTag;

	bool IsValid() const { return !ItemDefinition.IsNull() && Weight >= 0 && MinCount > 0 && MaxCount >= MinCount; }
};

// Result of rolling a loot table. Each element is a flattened (item, count)
// pair ready to hand to a loot-bag spawn or directly to an inventory.
struct PALDARKLAB_API FPaldarkLootRollResult
{
	TSoftObjectPtr<UPaldarkItemDefinition> ItemDefinition;
	int32 Count = 0;
};

UCLASS(BlueprintType, Const, meta = (DisplayName = "Paldark Loot Table"))
class PALDARKLAB_API UPaldarkLootTable : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPaldarkLootTable();

	// Stable identity tag. Must match a tag under `Paldark.LootTable.*`
	// (e.g. `Paldark.LootTable.HostileStandard`). Read by the loot-drop
	// component for log lines + the `Paldark.Loot.DumpTable` console
	// command output.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Loot|Table",
		meta = (Categories = "Paldark.LootTable"))
	FGameplayTag LootTableTag;

	// Localised display name shown in the `Paldark.Loot.DumpTable` console
	// output. Optional — falls back to LootTableTag if empty.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Loot|Table")
	FText DisplayName;

	// Number of independent weighted picks per roll. The hostile-standard
	// table ships at 1; the boss table ships at 3 (so a boss drops 3
	// weighted picks *plus* whatever GuaranteedWhenTag entries match).
	// Each pick is independent — duplicate items are merged in the
	// flattened result. Clamped to [1, 8] to keep the bag payload
	// bounded.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Loot|Table",
		meta = (ClampMin = "1", UIMin = "1", ClampMax = "8", UIMax = "8"))
	int32 MaxRolls = 1;

	// Per-entry rows. Order is not significant — picks are weighted.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Loot|Table")
	TArray<FPaldarkLootEntry> Entries;

	// Rolls the table given an RNG stream + the dying Pal's species tag (so
	// boss-only guaranteed entries can match). Returns a flattened list of
	// (item, count) pairs — duplicate items from independent picks are
	// merged. Empty result = empty table / all entries invalid.
	TArray<FPaldarkLootRollResult> RollEntries(struct FRandomStream& InRng,
		const FGameplayTagContainer& InContextTags) const;

	// Primary asset id with the `PaldarkLootTable` type. Matches the row in
	// `[/Script/Engine.AssetManagerSettings]`.
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	// Short debug label for `Paldark.Loot.DumpTable`. Returns DisplayName
	// if set, otherwise LootTableTag string, otherwise asset name.
	FString GetDebugLabel() const;
};
