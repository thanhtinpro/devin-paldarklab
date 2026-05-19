// PALDARK W33-34 — Loot table implementation.

#include "Loot/PaldarkLootTable.h"

#include "Inventory/PaldarkItemDefinition.h"
#include "Math/RandomStream.h"
#include "PaldarkLogCategories.h"

UPaldarkLootTable::UPaldarkLootTable() = default;

TArray<FPaldarkLootRollResult> UPaldarkLootTable::RollEntries(FRandomStream& InRng,
	const FGameplayTagContainer& InContextTags) const
{
	TArray<FPaldarkLootRollResult> Result;

	// Guaranteed entries — added once at MaxCount before the weighted picks.
	// Designers list them by setting `GuaranteedWhenTag` on the entry; the
	// loot-drop component fills `InContextTags` with the dying Pal's
	// SpeciesTag + team tag.
	for (const FPaldarkLootEntry& Entry : Entries)
	{
		if (!Entry.IsValid()) { continue; }
		if (Entry.GuaranteedWhenTag.IsValid() && InContextTags.HasTag(Entry.GuaranteedWhenTag))
		{
			FPaldarkLootRollResult& Roll = Result.AddDefaulted_GetRef();
			Roll.ItemDefinition = Entry.ItemDefinition;
			Roll.Count = Entry.MaxCount;
			UE_LOG(LogPaldarkInventory, Verbose,
				TEXT("LootTable '%s' — guaranteed entry %s × %d (tag %s matched)"),
				*GetName(),
				*Entry.ItemDefinition.ToString(),
				Entry.MaxCount,
				*Entry.GuaranteedWhenTag.ToString());
		}
	}

	// Weighted picks. Build a flat weight sum each roll (cheap — entries are
	// tiny). Skip entries that already fired as guaranteed if the designer
	// wants exclusive behaviour later; for W33-34 we let them double-dip so
	// the boss can both guarantee + roll the same item.
	int32 TotalWeight = 0;
	for (const FPaldarkLootEntry& Entry : Entries)
	{
		if (Entry.IsValid()) { TotalWeight += Entry.Weight; }
	}
	if (TotalWeight <= 0)
	{
		// All entries zero-weight (or invalid). Return guaranteed-only result.
		return Result;
	}

	const int32 Rolls = FMath::Clamp(MaxRolls, 1, 8);
	for (int32 RollIdx = 0; RollIdx < Rolls; ++RollIdx)
	{
		int32 Ticket = InRng.RandRange(0, TotalWeight - 1);
		const FPaldarkLootEntry* Picked = nullptr;
		for (const FPaldarkLootEntry& Entry : Entries)
		{
			if (!Entry.IsValid()) { continue; }
			if (Ticket < Entry.Weight) { Picked = &Entry; break; }
			Ticket -= Entry.Weight;
		}
		if (!Picked) { continue; }

		const int32 Count = InRng.RandRange(Picked->MinCount, Picked->MaxCount);

		// Merge duplicate item picks so the bag carries one stacked entry
		// per item, not N parallel rows. Hostile-standard table tends to
		// roll the same Battery multiple times — merging keeps the bag
		// payload terse.
		FPaldarkLootRollResult* Existing = Result.FindByPredicate(
			[&](const FPaldarkLootRollResult& R)
			{
				return R.ItemDefinition == Picked->ItemDefinition;
			});
		if (Existing)
		{
			Existing->Count += Count;
		}
		else
		{
			FPaldarkLootRollResult& Roll = Result.AddDefaulted_GetRef();
			Roll.ItemDefinition = Picked->ItemDefinition;
			Roll.Count = Count;
		}
	}

	return Result;
}

FPrimaryAssetId UPaldarkLootTable::GetPrimaryAssetId() const
{
	// Lyra convention — type matches the row name in DefaultGame.ini's
	// AssetManagerSettings scan rules so editor + runtime discovery stay
	// aligned without an extra registration step.
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("PaldarkLootTable")), GetFName());
}

FString UPaldarkLootTable::GetDebugLabel() const
{
	if (!DisplayName.IsEmpty())
	{
		return DisplayName.ToString();
	}
	if (LootTableTag.IsValid())
	{
		return LootTableTag.ToString();
	}
	return GetName();
}
