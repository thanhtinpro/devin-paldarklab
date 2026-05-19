#include "Player/Components/PaldarkPlayerInventoryComponent.h"

#include "Engine/AssetManager.h"
#include "GameFramework/Actor.h"
#include "HAL/PlatformAtomics.h"
#include "Net/UnrealNetwork.h"

#include "Inventory/Fragments/PaldarkItemFragment_Backpack.h"
#include "Inventory/Fragments/PaldarkItemFragment_Container.h"
#include "Inventory/Fragments/PaldarkItemFragment_Stackable.h"
#include "Inventory/Fragments/PaldarkItemFragment_Weight.h"
#include "Inventory/PaldarkItemDefinition.h"
#include "Inventory/PaldarkItemFragment.h"
#include "PaldarkLogCategories.h"

// W39 — L-32 scaffold. Process-wide monotonic counter that feeds
// `FPaldarkInventoryEntry::MigrationReplicationKey` on every freshly-appended
// row. Atomic so AddItem on different threads (e.g. server worker thread
// pre-warming inventories from save-game in W47) can't issue duplicate ids.
// Replaced by `FFastArraySerializerItem::ReplicationKey` in W47 — see the
// W39 § L-32 closure section in `PaldarkLab/README.md` for the migration
// plan.
namespace PaldarkInventoryMigration
{
	static volatile int32 GReplicationKeyCounter = 0;
}

UPaldarkPlayerInventoryComponent::UPaldarkPlayerInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPaldarkPlayerInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UPaldarkPlayerInventoryComponent, BaseMaxWeightKg);
	DOREPLIFETIME(UPaldarkPlayerInventoryComponent, Entries);
	// W37-38 — equipped backpack drives the effective MaxWeight on top of base.
	DOREPLIFETIME(UPaldarkPlayerInventoryComponent, EquippedBackpack);
}

void UPaldarkPlayerInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
}

namespace PaldarkInventoryDetail
{
	static bool IsAuthority(const AActor* Owner)
	{
		return Owner != nullptr && Owner->HasAuthority();
	}
}

const UPaldarkItemDefinition* UPaldarkPlayerInventoryComponent::LoadItemDefSync(const TSoftObjectPtr<UPaldarkItemDefinition>& ItemDef)
{
	if (ItemDef.IsNull())
	{
		return nullptr;
	}
	if (UPaldarkItemDefinition* Loaded = ItemDef.Get())
	{
		return Loaded;
	}
	// W11-12 — synchronous load is acceptable; async + bundle preloading is
	// the W21+ asset manager polish task.
	return ItemDef.LoadSynchronous();
}

int32 UPaldarkPlayerInventoryComponent::AddItem(const TSoftObjectPtr<UPaldarkItemDefinition>& InItemDef, int32 RequestedCount)
{
	if (!PaldarkInventoryDetail::IsAuthority(GetOwner()))
	{
		UE_LOG(LogPaldark, Warning, TEXT("[Inventory.AddItem] called on client; ignored. Use server RPC."));
		return 0;
	}
	if (InItemDef.IsNull() || RequestedCount <= 0)
	{
		return 0;
	}

	const UPaldarkItemDefinition* Resolved = LoadItemDefSync(InItemDef);
	if (Resolved == nullptr)
	{
		UE_LOG(LogPaldark, Warning, TEXT("[Inventory.AddItem] failed to load %s."), *InItemDef.ToString());
		return 0;
	}

	const UPaldarkItemFragment_Stackable* StackFrag = Resolved->FindFragmentByClass<UPaldarkItemFragment_Stackable>();
	const int32 MaxStack = StackFrag != nullptr ? StackFrag->GetMaxStackSize() : 1;
	const FGameplayTag ItemTag = Resolved->ItemTag;

	int32 Remaining = RequestedCount;

	// Phase 1 — top off existing stackable rows that match this item tag.
	if (StackFrag != nullptr && ItemTag.IsValid())
	{
		for (int32 i = 0; i < Entries.Num() && Remaining > 0; ++i)
		{
			FPaldarkInventoryEntry& Row = Entries[i];
			const UPaldarkItemDefinition* RowDef = LoadItemDefSync(Row.ItemDef);
			if (RowDef == nullptr || RowDef->ItemTag != ItemTag)
			{
				continue;
			}
			const int32 Space = FMath::Max(MaxStack - Row.StackCount, 0);
			if (Space <= 0)
			{
				continue;
			}
			const int32 Place = FMath::Min(Space, Remaining);
			Row.StackCount += Place;
			Remaining       -= Place;
		}
	}

	// Phase 2 — open new entries for the leftover. Non-stackable items always
	// land here (MaxStack=1 → one new row per copy).
	int32 LastNewEntryIndex = INDEX_NONE;
	while (Remaining > 0)
	{
		FPaldarkInventoryEntry NewRow;
		NewRow.ItemDef    = InItemDef;
		const int32 Place = FMath::Min(MaxStack, Remaining);
		NewRow.StackCount = Place;
		// W39 — L-32 scaffold. Stamp a fresh MigrationReplicationKey on
		// every newly-created row so consumers (and the future W47
		// FastArraySerializer dirty tracker) can correlate this row
		// across snapshots. Top-off-existing-stack callers do NOT
		// reissue a key — the row keeps its previous id.
		NewRow.MigrationReplicationKey = BuildNextReplicationKey();
		LastNewEntryIndex = Entries.Add(MoveTemp(NewRow));
		Remaining -= Place;
	}

	const int32 Added = RequestedCount - FMath::Max(Remaining, 0);
	if (Added > 0)
	{
		UE_LOG(
			LogPaldark,
			Verbose,
			TEXT("[Inventory.AddItem] added %d × %s (weight %.2f/%.2f kg)."),
			Added,
			*Resolved->GetDebugLabel(),
			GetCurrentWeightKg(),
			GetMaxWeightKg());
		// W39 — L-32 scaffold. Single funnel for inventory-changed notifications.
		// Pass the last new entry index when a fresh row was opened (Phase 2);
		// otherwise fall back to INDEX_NONE for the array-wide broadcast.
		MarkInventoryDirty(LastNewEntryIndex);
	}
	return Added;
}

int32 UPaldarkPlayerInventoryComponent::RemoveItemByTag(const FGameplayTag& ItemTag, int32 RequestedCount)
{
	if (!PaldarkInventoryDetail::IsAuthority(GetOwner()))
	{
		UE_LOG(LogPaldark, Warning, TEXT("[Inventory.Remove] called on client; ignored. Use server RPC."));
		return 0;
	}
	if (!ItemTag.IsValid() || RequestedCount <= 0)
	{
		return 0;
	}

	int32 Removed = 0;
	for (int32 i = Entries.Num() - 1; i >= 0 && Removed < RequestedCount; --i)
	{
		FPaldarkInventoryEntry& Row = Entries[i];
		const UPaldarkItemDefinition* RowDef = LoadItemDefSync(Row.ItemDef);
		if (RowDef == nullptr || RowDef->ItemTag != ItemTag)
		{
			continue;
		}
		const int32 Take = FMath::Min(Row.StackCount, RequestedCount - Removed);
		Row.StackCount -= Take;
		Removed        += Take;
		if (Row.StackCount <= 0)
		{
			Entries.RemoveAt(i);
		}
	}

	if (Removed > 0)
	{
		UE_LOG(
			LogPaldark,
			Verbose,
			TEXT("[Inventory.Remove] removed %d × %s (entries=%d)."),
			Removed,
			*ItemTag.ToString(),
			Entries.Num());
		// W39 — L-32 scaffold. RemoveItemByTag may touch multiple rows
		// (decrement stacks, erase emptied rows); we don't know which
		// single row to mark dirty so route through INDEX_NONE
		// (array-wide refresh). W47 FastArraySerializer migration may
		// revisit this to mark each touched row individually if the
		// inventory grows past ~50 entries.
		MarkInventoryDirty(INDEX_NONE);
	}
	return Removed;
}

void UPaldarkPlayerInventoryComponent::DropAllItems()
{
	if (!PaldarkInventoryDetail::IsAuthority(GetOwner()))
	{
		UE_LOG(LogPaldark, Warning, TEXT("[Inventory.DropAll] called on client; ignored."));
		return;
	}
	if (Entries.Num() == 0)
	{
		return;
	}
	const int32 Cleared = Entries.Num();
	Entries.Reset();
	UE_LOG(
		LogPaldark,
		Log,
		TEXT("[Inventory.DropAll] cleared %d entries (world pickup-actor spawn is a follow-up PR)."),
		Cleared);
	// W39 — L-32 scaffold. Array wiped → no per-row dirty mark possible;
	// W47 FastArraySerializer migration calls `MarkArrayDirty()` for the
	// equivalent semantics.
	MarkInventoryDirty(INDEX_NONE);
}

float UPaldarkPlayerInventoryComponent::GetCurrentWeightKg() const
{
	float Total = 0.0f;
	for (const FPaldarkInventoryEntry& Row : Entries)
	{
		const UPaldarkItemDefinition* RowDef = LoadItemDefSync(Row.ItemDef);
		if (RowDef == nullptr)
		{
			continue;
		}
		if (const UPaldarkItemFragment_Weight* W = RowDef->FindFragmentByClass<UPaldarkItemFragment_Weight>())
		{
			Total += W->GetWeightKgPerUnit() * static_cast<float>(Row.StackCount);
		}
		// W37-38 — composite container rows pay the weight of their inner items
		// on top of the container item's own Weight fragment.
		for (const FPaldarkInventoryEntry& Inner : Row.InnerEntries)
		{
			const UPaldarkItemDefinition* InnerDef = LoadItemDefSync(Inner.ItemDef);
			if (InnerDef == nullptr) { continue; }
			if (const UPaldarkItemFragment_Weight* IW = InnerDef->FindFragmentByClass<UPaldarkItemFragment_Weight>())
			{
				Total += IW->GetWeightKgPerUnit() * static_cast<float>(Inner.StackCount);
			}
		}
	}
	return Total;
}

float UPaldarkPlayerInventoryComponent::GetMaxWeightKg() const
{
	float Cap = BaseMaxWeightKg;
	if (const UPaldarkItemDefinition* Def = LoadItemDefSync(EquippedBackpack))
	{
		if (const UPaldarkItemFragment_Backpack* B = Def->FindFragmentByClass<UPaldarkItemFragment_Backpack>())
		{
			Cap += B->GetMaxWeightBonusKg();
		}
	}
	return FMath::Max(Cap, 0.0f);
}

int32 UPaldarkPlayerInventoryComponent::GetExtraSlotsFromBackpack() const
{
	if (const UPaldarkItemDefinition* Def = LoadItemDefSync(EquippedBackpack))
	{
		if (const UPaldarkItemFragment_Backpack* B = Def->FindFragmentByClass<UPaldarkItemFragment_Backpack>())
		{
			return B->GetExtraSlots();
		}
	}
	return 0;
}

bool UPaldarkPlayerInventoryComponent::SetEquippedBackpack(const TSoftObjectPtr<UPaldarkItemDefinition>& InItemDef)
{
	if (!PaldarkInventoryDetail::IsAuthority(GetOwner()))
	{
		UE_LOG(LogPaldark, Warning, TEXT("[Inventory.SetEquippedBackpack] called on client; ignored."));
		return false;
	}

	// Clearing the backpack.
	if (InItemDef.IsNull())
	{
		if (EquippedBackpack.IsNull())
		{
			return false;
		}
		EquippedBackpack.Reset();
		// W39 — L-32 scaffold. Backpack swap is a cap-change, not a row
		// mutation → INDEX_NONE.
		MarkInventoryDirty(INDEX_NONE);
		UE_LOG(LogPaldark, Log, TEXT("[Inventory.SetEquippedBackpack] cleared (cap %.1f kg)."), GetMaxWeightKg());
		return true;
	}

	const UPaldarkItemDefinition* Def = LoadItemDefSync(InItemDef);
	if (Def == nullptr)
	{
		UE_LOG(LogPaldark, Warning, TEXT("[Inventory.SetEquippedBackpack] failed to load %s."), *InItemDef.ToString());
		return false;
	}

	const UPaldarkItemFragment_Backpack* B = Def->FindFragmentByClass<UPaldarkItemFragment_Backpack>();
	if (B == nullptr)
	{
		UE_LOG(LogPaldark, Warning, TEXT("[Inventory.SetEquippedBackpack] %s carries no Backpack fragment."), *Def->GetDebugLabel());
		return false;
	}

	EquippedBackpack = InItemDef;
	UE_LOG(
		LogPaldark,
		Log,
		TEXT("[Inventory.SetEquippedBackpack] %s equipped (+%.1f kg, +%d slots, cap %.1f kg)."),
		*Def->GetDebugLabel(),
		B->GetMaxWeightBonusKg(),
		B->GetExtraSlots(),
		GetMaxWeightKg());
	// W39 — L-32 scaffold. Backpack equip is a cap-change → INDEX_NONE.
	MarkInventoryDirty(INDEX_NONE);
	return true;
}

int32 UPaldarkPlayerInventoryComponent::FindEntryIndexByTag(const FGameplayTag& ItemTag) const
{
	if (!ItemTag.IsValid())
	{
		return INDEX_NONE;
	}
	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		const UPaldarkItemDefinition* RowDef = LoadItemDefSync(Entries[i].ItemDef);
		if (RowDef != nullptr && RowDef->ItemTag == ItemTag)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

const UPaldarkItemDefinition* UPaldarkPlayerInventoryComponent::FindFirstItemDefByTag(const FGameplayTag& ItemTag) const
{
	const int32 Index = FindEntryIndexByTag(ItemTag);
	return Index != INDEX_NONE ? LoadItemDefSync(Entries[Index].ItemDef) : nullptr;
}

void UPaldarkPlayerInventoryComponent::DumpToLog() const
{
	UE_LOG(
		LogPaldark,
		Log,
		TEXT("[Inventory.Dump] owner=%s entries=%d weight=%.2f/%.2f kg over=%s"),
		GetOwner() != nullptr ? *GetOwner()->GetName() : TEXT("<no-owner>"),
		Entries.Num(),
		GetCurrentWeightKg(),
		GetMaxWeightKg(),
		IsOverEncumbered() ? TEXT("YES") : TEXT("no"));

	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		const FPaldarkInventoryEntry& Row = Entries[i];
		const UPaldarkItemDefinition* RowDef = LoadItemDefSync(Row.ItemDef);
		if (RowDef == nullptr)
		{
			UE_LOG(LogPaldark, Log, TEXT("  [%d] <unresolved %s> x%d"), i, *Row.ItemDef.ToString(), Row.StackCount);
			continue;
		}
		FString Frags;
		for (UPaldarkItemFragment* Frag : RowDef->Fragments)
		{
			if (Frag == nullptr) { continue; }
			if (!Frags.IsEmpty()) { Frags += TEXT(", "); }
			Frags += Frag->GetDebugDescription();
		}
		UE_LOG(
			LogPaldark,
			Log,
			TEXT("  [%d] %s x%d (tag=%s, fragments=[%s])"),
			i,
			*RowDef->GetDebugLabel(),
			Row.StackCount,
			RowDef->ItemTag.IsValid() ? *RowDef->ItemTag.ToString() : TEXT("<none>"),
			*Frags);
	}
}

void UPaldarkPlayerInventoryComponent::OnRep_Entries()
{
	// W39 — L-32 scaffold. Client-side OnRep doesn't know which row
	// changed (the whole TArray comes in as a blob); INDEX_NONE keeps
	// the W37-38 listener contract. W47 FastArraySerializer will receive
	// per-row PostReplicatedAdd / PostReplicatedChange / PreReplicatedRemove
	// callbacks instead, each of which can call MarkInventoryDirty with
	// the actual row index.
	MarkInventoryDirty(INDEX_NONE);
}

void UPaldarkPlayerInventoryComponent::OnRep_EquippedBackpack()
{
	// HUD reads GetMaxWeightKg() which now derives off the new backpack —
	// fire the broadcast so the same listener path used for Entries updates
	// also catches encumbrance-cap changes.
	// W39 — L-32 scaffold. Backpack swap is a cap-change → INDEX_NONE.
	MarkInventoryDirty(INDEX_NONE);
}

int32 UPaldarkPlayerInventoryComponent::BuildNextReplicationKey()
{
	// W39 — L-32 scaffold. Atomic monotonic counter — see the namespace
	// comment at the top of this file. `FPlatformAtomics::InterlockedIncrement`
	// returns the post-increment value (so the first issued key is 1, not 0).
	// Zero is reserved as a sentinel meaning "row was never stamped"
	// (e.g. designer-authored default-constructed entries in a Blueprint).
	return FPlatformAtomics::InterlockedIncrement(&PaldarkInventoryMigration::GReplicationKeyCounter);
}

void UPaldarkPlayerInventoryComponent::MarkInventoryDirty(int32 EntryIndex)
{
	// W39 — L-32 scaffold. Best-effort bounds-check: an out-of-range
	// EntryIndex is logged at Verbose (so designers see it in PIE without
	// spamming shipping logs) and treated as INDEX_NONE. The broadcast
	// still fires so listeners refresh — better to over-notify than skip
	// a real change.
	if (EntryIndex != INDEX_NONE && (EntryIndex < 0 || EntryIndex >= Entries.Num()))
	{
		UE_LOG(
			LogPaldark,
			Verbose,
			TEXT("[L-32][Inventory.MarkDirty] EntryIndex=%d out of range [0,%d); falling back to INDEX_NONE."),
			EntryIndex,
			Entries.Num());
		EntryIndex = INDEX_NONE;
	}

	// W47 migration point — replace this body with:
	//   if (EntryIndex == INDEX_NONE) { Entries.MarkArrayDirty(); }
	//   else                          { Entries.MarkItemDirty(Entries.Items[EntryIndex]); }
	//   OnInventoryChanged.Broadcast(this);
	// Today (W39 scaffold), we just fire the existing array-wide broadcast
	// — per-row delta replication requires FastArraySerializer.
	OnInventoryChanged.Broadcast(this);
}

void UPaldarkPlayerInventoryComponent::DumpCompositeToLog() const
{
	UE_LOG(
		LogPaldark,
		Log,
		TEXT("[Inventory.DumpComposite] owner=%s entries=%d weight=%.2f/%.2f kg backpack=%s extra_slots=%d"),
		GetOwner() != nullptr ? *GetOwner()->GetName() : TEXT("<no-owner>"),
		Entries.Num(),
		GetCurrentWeightKg(),
		GetMaxWeightKg(),
		EquippedBackpack.IsNull() ? TEXT("<none>") : *EquippedBackpack.ToString(),
		GetExtraSlotsFromBackpack());

	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		const FPaldarkInventoryEntry& Row = Entries[i];
		const UPaldarkItemDefinition* RowDef = LoadItemDefSync(Row.ItemDef);
		if (RowDef == nullptr)
		{
			UE_LOG(LogPaldark, Log, TEXT("  [%d] <unresolved %s> x%d"), i, *Row.ItemDef.ToString(), Row.StackCount);
			continue;
		}
		UE_LOG(
			LogPaldark,
			Log,
			TEXT("  [%d] %s x%d (tag=%s, inner=%d)"),
			i,
			*RowDef->GetDebugLabel(),
			Row.StackCount,
			RowDef->ItemTag.IsValid() ? *RowDef->ItemTag.ToString() : TEXT("<none>"),
			Row.InnerEntries.Num());
		for (int32 j = 0; j < Row.InnerEntries.Num(); ++j)
		{
			const FPaldarkInventoryEntry& Inner = Row.InnerEntries[j];
			const UPaldarkItemDefinition* InnerDef = LoadItemDefSync(Inner.ItemDef);
			if (InnerDef == nullptr)
			{
				UE_LOG(LogPaldark, Log, TEXT("      [%d.%d] <unresolved %s> x%d"), i, j, *Inner.ItemDef.ToString(), Inner.StackCount);
				continue;
			}
			UE_LOG(
				LogPaldark,
				Log,
				TEXT("      [%d.%d] %s x%d (tag=%s)"),
				i,
				j,
				*InnerDef->GetDebugLabel(),
				Inner.StackCount,
				InnerDef->ItemTag.IsValid() ? *InnerDef->ItemTag.ToString() : TEXT("<none>"));
		}
	}
}

int32 UPaldarkPlayerInventoryComponent::TopOffExistingStack(int32 /*EntryIndex*/, int32 /*RequestedCount*/)
{
	// Reserved for FastArraySerializer migration (W21+). The current AddItem
	// path inlines the top-off logic so this helper is unused; kept on the
	// header so a follow-up PR can swap the implementation without touching
	// the public API.
	return 0;
}

// W47 — Save game capture. Flatten nested containers into top-level entries
// (composite-container persistence is a W48 polish task) so the load path
// doesn't need to reconstruct the tree.
FPaldarkInventorySnapshot UPaldarkPlayerInventoryComponent::CaptureSnapshot() const
{
	FPaldarkInventorySnapshot Out;
	Out.Entries.Reserve(Entries.Num());

	for (const FPaldarkInventoryEntry& Row : Entries)
	{
		const UPaldarkItemDefinition* RowDef = LoadItemDefSync(Row.ItemDef);
		if (RowDef == nullptr || !RowDef->ItemTag.IsValid())
		{
			// Skip unresolved rows — the load path can't reconstruct them
			// anyway. Log so designers see the loss in PIE.
			UE_LOG(LogPaldark, Warning,
				TEXT("[Save.Capture] dropping unresolved row %s x%d (no item tag)."),
				*Row.ItemDef.ToString(), Row.StackCount);
			continue;
		}
		FPaldarkInventoryEntrySaved Saved;
		Saved.ItemTag    = RowDef->ItemTag;
		Saved.StackCount = Row.StackCount;
		Out.Entries.Add(Saved);

		// Flatten composite-container inner rows (W37-38) — restored as
		// independent top-level entries on load. The flattened entries
		// pay the same weight contribution as the originals, so the
		// post-load weight cap is preserved.
		for (const FPaldarkInventoryEntry& Inner : Row.InnerEntries)
		{
			const UPaldarkItemDefinition* InnerDef = LoadItemDefSync(Inner.ItemDef);
			if (InnerDef == nullptr || !InnerDef->ItemTag.IsValid())
			{
				continue;
			}
			FPaldarkInventoryEntrySaved SavedInner;
			SavedInner.ItemTag    = InnerDef->ItemTag;
			SavedInner.StackCount = Inner.StackCount;
			Out.Entries.Add(SavedInner);
		}
	}

	if (const UPaldarkItemDefinition* BackpackDef = LoadItemDefSync(EquippedBackpack))
	{
		if (BackpackDef->ItemTag.IsValid())
		{
			Out.EquippedBackpackTag = BackpackDef->ItemTag;
		}
	}

	return Out;
}

void UPaldarkPlayerInventoryComponent::ApplySnapshot(const FPaldarkInventorySnapshot& Snapshot)
{
	if (!PaldarkInventoryDetail::IsAuthority(GetOwner()))
	{
		UE_LOG(LogPaldark, Warning,
			TEXT("[Save.Apply] called on client; ignored. Snapshot only applies on authority."));
		return;
	}

	// Wipe current state first so a partial apply doesn't double-stack.
	const int32 OldCount = Entries.Num();
	Entries.Reset();
	EquippedBackpack.Reset();

	// Re-equip backpack BEFORE adding entries so weight cap matches save-time
	// cap during the add loop. We don't strictly enforce the cap here (the
	// snapshot was already validated at save time), but later AddItem paths
	// rely on the equipped state.
	int32 BackpackApplied = 0;
	if (Snapshot.EquippedBackpackTag.IsValid())
	{
		// Lookup via the inventory's existing tag-table is post-add only
		// (FindFirstItemDefByTag scans Entries which is empty at this
		// point), so route the resolve through the asset manager.
		if (UAssetManager* AM = UAssetManager::GetIfValid())
		{
			TArray<FAssetData> Found;
			const FPrimaryAssetType ItemType(TEXT("PaldarkItemDefinition"));
			AM->GetPrimaryAssetDataList(ItemType, Found);
			for (const FAssetData& AD : Found)
			{
				TSoftObjectPtr<UPaldarkItemDefinition> Soft(AD.ToSoftObjectPath());
				const UPaldarkItemDefinition* Def = LoadItemDefSync(Soft);
				if (Def != nullptr && Def->ItemTag == Snapshot.EquippedBackpackTag)
				{
					if (SetEquippedBackpack(Soft))
					{
						BackpackApplied = 1;
					}
					break;
				}
			}
		}
	}

	int32 RowsApplied = 0;
	int32 RowsDropped = 0;
	if (UAssetManager* AM = UAssetManager::GetIfValid())
	{
		TArray<FAssetData> AllItems;
		AM->GetPrimaryAssetDataList(FPrimaryAssetType(TEXT("PaldarkItemDefinition")), AllItems);

		// Build a one-shot tag → soft-ptr map so we don't re-scan asset
		// data once per saved row.
		TMap<FGameplayTag, TSoftObjectPtr<UPaldarkItemDefinition>> TagToDef;
		TagToDef.Reserve(AllItems.Num());
		for (const FAssetData& AD : AllItems)
		{
			TSoftObjectPtr<UPaldarkItemDefinition> Soft(AD.ToSoftObjectPath());
			const UPaldarkItemDefinition* Def = LoadItemDefSync(Soft);
			if (Def != nullptr && Def->ItemTag.IsValid())
			{
				TagToDef.FindOrAdd(Def->ItemTag) = Soft;
			}
		}

		for (const FPaldarkInventoryEntrySaved& Saved : Snapshot.Entries)
		{
			if (!Saved.IsValid())
			{
				RowsDropped++;
				continue;
			}
			TSoftObjectPtr<UPaldarkItemDefinition>* Found = TagToDef.Find(Saved.ItemTag);
			if (Found == nullptr)
			{
				UE_LOG(LogPaldark, Warning,
					TEXT("[Save.Apply] no item def resolves tag %s — dropping x%d."),
					*Saved.ItemTag.ToString(), Saved.StackCount);
				RowsDropped++;
				continue;
			}
			const int32 Added = AddItem(*Found, Saved.StackCount);
			if (Added > 0)
			{
				RowsApplied++;
			}
			else
			{
				RowsDropped++;
			}
		}
	}
	else
	{
		UE_LOG(LogPaldark, Warning,
			TEXT("[Save.Apply] UAssetManager unavailable; cannot resolve %d snapshot rows."),
			Snapshot.Entries.Num());
		RowsDropped = Snapshot.Entries.Num();
	}

	UE_LOG(LogPaldark, Log,
		TEXT("[Save.Apply] cleared=%d applied=%d dropped=%d backpack_applied=%d (snapshot size=%d)"),
		OldCount, RowsApplied, RowsDropped, BackpackApplied, Snapshot.Entries.Num());

	MarkInventoryDirty(INDEX_NONE);
}
