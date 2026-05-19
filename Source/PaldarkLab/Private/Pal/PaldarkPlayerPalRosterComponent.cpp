#include "Pal/PaldarkPlayerPalRosterComponent.h"

#include "Net/UnrealNetwork.h"

#include "Gas/PaldarkAttributeSet.h"
#include "Pal/Components/PaldarkPalBondComponent.h"
#include "Pal/PaldarkPalCharacter.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"

UPaldarkPlayerPalRosterComponent::UPaldarkPlayerPalRosterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPaldarkPlayerPalRosterComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// COND_OwnerOnly — roster is per-player UI state. Other players don't
	// observe each other's tamed Pals, matches Lyra's per-player widget
	// state replication pattern.
	DOREPLIFETIME_CONDITION(UPaldarkPlayerPalRosterComponent, Entries, COND_OwnerOnly);
}

void UPaldarkPlayerPalRosterComponent::TamePal(APaldarkPalCharacter* TamedPal, float RolledProbability)
{
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	if (!TamedPal)
	{
		return;
	}

	FPaldarkRosterEntry Entry;
	Entry.SpeciesTag = TamedPal->GetSpeciesTag();
	Entry.BondLevel  = 1; // Fresh tame starts at Bond Level 1 per roadmap.

	// Snapshot health from the AttributeSet so future "deploy from roster"
	// flows respawn the Pal at the captured HP, not full HP.
	if (const UPaldarkAttributeSet* Attr = TamedPal->GetPaldarkAttributeSet())
	{
		Entry.HealthAtTame    = Attr->GetHealth();
		Entry.MaxHealthAtTame = Attr->GetMaxHealth();
	}

	// Snapshot a Pal-definition primary asset id from the species tag leaf
	// name. Convention: `DA_PalDef_<Leaf>` under `/Game/Paldark/Pals/`. If
	// the species tag is empty (designer authoring oversight), the entry
	// still records the snapshot but `PalDefinitionId` is left invalid.
	if (Entry.SpeciesTag.IsValid())
	{
		const FString FullTag = Entry.SpeciesTag.GetTagName().ToString();
		int32 LastDot = INDEX_NONE;
		if (FullTag.FindLastChar(TEXT('.'), LastDot) && LastDot < FullTag.Len() - 1)
		{
			const FString Leaf = FullTag.Mid(LastDot + 1);
			Entry.PalDefinitionId = FPrimaryAssetId(TEXT("PaldarkPalDefinition"), FName(*Leaf));
		}
	}

	Entries.Add(Entry);
	OnRosterChanged.Broadcast(this);

	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkPlayerPalRosterComponent::TamePal — owner=%s species=%s health=%.1f/%.1f roll_p=%.3f (roster size=%d)"),
		*GetOwner()->GetName(),
		*Entry.SpeciesTag.ToString(),
		Entry.HealthAtTame, Entry.MaxHealthAtTame,
		RolledProbability,
		Entries.Num());

	// Hand the bond component on the *original* Pal a final XP grant — the
	// Pal will be destroyed by the tame component immediately after this
	// returns, so this is the last chance for the bond delegate to fire.
	if (UPaldarkPalBondComponent* Bond = TamedPal->GetBondSlot())
	{
		Bond->AddBondXP(50.f, TAG_Paldark_Bond_Event_Tame);
	}
}

FPaldarkRosterEntry UPaldarkPlayerPalRosterComponent::RemoveEntryByIndex(int32 EntryIndex)
{
	FPaldarkRosterEntry Removed;
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		return Removed;
	}
	if (!Entries.IsValidIndex(EntryIndex))
	{
		return Removed;
	}

	Removed = Entries[EntryIndex];
	Entries.RemoveAt(EntryIndex);
	OnRosterChanged.Broadcast(this);

	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkPlayerPalRosterComponent::RemoveEntryByIndex — owner=%s idx=%d species=%s (new size=%d)"),
		*GetOwner()->GetName(),
		EntryIndex,
		*Removed.SpeciesTag.ToString(),
		Entries.Num());

	return Removed;
}

int32 UPaldarkPlayerPalRosterComponent::AppendEntry(const FPaldarkRosterEntry& Entry)
{
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		return INDEX_NONE;
	}

	const int32 NewIndex = Entries.Add(Entry);
	OnRosterChanged.Broadcast(this);

	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkPlayerPalRosterComponent::AppendEntry — owner=%s species=%s -> idx=%d (size=%d)"),
		*GetOwner()->GetName(),
		*Entry.SpeciesTag.ToString(),
		NewIndex,
		Entries.Num());

	return NewIndex;
}

bool UPaldarkPlayerPalRosterComponent::HealEntryToFull(int32 EntryIndex)
{
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		return false;
	}
	if (!Entries.IsValidIndex(EntryIndex))
	{
		return false;
	}

	FPaldarkRosterEntry& Entry = Entries[EntryIndex];
	if (Entry.MaxHealthAtTame <= 0.0f)
	{
		return false;
	}

	if (FMath::IsNearlyEqual(Entry.HealthAtTame, Entry.MaxHealthAtTame))
	{
		// Already full — treat as success so the marketplace doesn't bill
		// the player; the controller checks `bWasAlreadyFull` itself.
		return true;
	}

	Entry.HealthAtTame = Entry.MaxHealthAtTame;
	OnRosterChanged.Broadcast(this);

	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkPlayerPalRosterComponent::HealEntryToFull — owner=%s idx=%d species=%s hp=%.1f/%.1f"),
		*GetOwner()->GetName(),
		EntryIndex,
		*Entry.SpeciesTag.ToString(),
		Entry.HealthAtTame, Entry.MaxHealthAtTame);

	return true;
}

// W47 — Save game capture/apply. Strips replication-only fields (e.g. the
// soft-ref `PalDefinitionId`, re-derived from `SpeciesTag` on load) so the
// blob travels well across content patches.
FPaldarkRosterSnapshot UPaldarkPlayerPalRosterComponent::CaptureSnapshot() const
{
	FPaldarkRosterSnapshot Out;
	Out.Entries.Reserve(Entries.Num());
	for (const FPaldarkRosterEntry& E : Entries)
	{
		FPaldarkRosterEntrySaved Saved;
		Saved.SpeciesTag       = E.SpeciesTag;
		Saved.BondLevel        = E.BondLevel;
		Saved.HealthAtTame     = E.HealthAtTame;
		Saved.MaxHealthAtTame  = E.MaxHealthAtTame;
		Saved.Nickname         = E.Nickname;
		Out.Entries.Add(Saved);
	}
	return Out;
}

void UPaldarkPlayerPalRosterComponent::ApplySnapshot(const FPaldarkRosterSnapshot& Snapshot)
{
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		UE_LOG(LogPaldarkPal, Warning,
			TEXT("UPaldarkPlayerPalRosterComponent::ApplySnapshot rejected: non-authority"));
		return;
	}

	Entries.Reset();
	Entries.Reserve(Snapshot.Entries.Num());
	int32 DroppedInvalid = 0;
	for (const FPaldarkRosterEntrySaved& Saved : Snapshot.Entries)
	{
		if (!Saved.IsValid())
		{
			DroppedInvalid++;
			continue;
		}
		FPaldarkRosterEntry E;
		E.SpeciesTag       = Saved.SpeciesTag;
		E.BondLevel        = Saved.BondLevel;
		E.HealthAtTame     = Saved.HealthAtTame;
		E.MaxHealthAtTame  = Saved.MaxHealthAtTame;
		E.Nickname         = Saved.Nickname;
		// Re-derive PalDefinitionId from the species tag leaf — same logic
		// as the TamePal path so loaded entries are indistinguishable from
		// freshly-tamed ones.
		const FString FullTag = Saved.SpeciesTag.GetTagName().ToString();
		int32 LastDot = INDEX_NONE;
		if (FullTag.FindLastChar(TEXT('.'), LastDot) && LastDot < FullTag.Len() - 1)
		{
			const FString Leaf = FullTag.Mid(LastDot + 1);
			E.PalDefinitionId = FPrimaryAssetId(TEXT("PaldarkPalDefinition"), FName(*Leaf));
		}
		Entries.Add(E);
	}
	OnRosterChanged.Broadcast(this);

	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkPlayerPalRosterComponent::ApplySnapshot — applied=%d dropped=%d (snapshot size=%d)"),
		Entries.Num(), DroppedInvalid, Snapshot.Entries.Num());
}

void UPaldarkPlayerPalRosterComponent::DumpToLog() const
{
	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkPlayerPalRosterComponent::DumpToLog — owner=%s count=%d"),
		GetOwner() ? *GetOwner()->GetName() : TEXT("<no owner>"),
		Entries.Num());

	for (int32 Idx = 0; Idx < Entries.Num(); ++Idx)
	{
		const FPaldarkRosterEntry& E = Entries[Idx];
		UE_LOG(LogPaldarkPal, Log,
			TEXT("  [%d] species=%s bond=%d hp=%.1f/%.1f def=%s nick=%s"),
			Idx,
			*E.SpeciesTag.ToString(),
			E.BondLevel,
			E.HealthAtTame, E.MaxHealthAtTame,
			*E.PalDefinitionId.ToString(),
			*E.Nickname);
	}
}

void UPaldarkPlayerPalRosterComponent::OnRep_Entries()
{
	OnRosterChanged.Broadcast(this);
}
