// PALDARK W44-45 — Pal deposit storage implementation.

#include "Pal/PaldarkPalDepositComponent.h"

#include "Net/UnrealNetwork.h"

#include "PaldarkLogCategories.h"

UPaldarkPalDepositComponent::UPaldarkPalDepositComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPaldarkPalDepositComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UPaldarkPalDepositComponent, DepositedEntries, COND_OwnerOnly);
}

int32 UPaldarkPalDepositComponent::DepositEntry(const FPaldarkRosterEntry& Entry)
{
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		return INDEX_NONE;
	}
	if (IsFull())
	{
		UE_LOG(LogPaldarkPal, Warning,
			TEXT("UPaldarkPalDepositComponent::DepositEntry — owner=%s deposit full (%d/%d), reject."),
			*GetOwner()->GetName(), DepositedEntries.Num(), MaxDepositSlots);
		return INDEX_NONE;
	}

	const int32 NewIndex = DepositedEntries.Add(Entry);
	OnDepositChanged.Broadcast(this);

	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkPalDepositComponent::DepositEntry — owner=%s species=%s -> idx=%d (size=%d/%d)"),
		*GetOwner()->GetName(),
		*Entry.SpeciesTag.ToString(),
		NewIndex,
		DepositedEntries.Num(), MaxDepositSlots);

	return NewIndex;
}

FPaldarkRosterEntry UPaldarkPalDepositComponent::WithdrawEntry(int32 DepositIndex)
{
	FPaldarkRosterEntry Removed;
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		return Removed;
	}
	if (!DepositedEntries.IsValidIndex(DepositIndex))
	{
		return Removed;
	}

	Removed = DepositedEntries[DepositIndex];
	DepositedEntries.RemoveAt(DepositIndex);
	OnDepositChanged.Broadcast(this);

	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkPalDepositComponent::WithdrawEntry — owner=%s idx=%d species=%s (new size=%d/%d)"),
		*GetOwner()->GetName(),
		DepositIndex,
		*Removed.SpeciesTag.ToString(),
		DepositedEntries.Num(), MaxDepositSlots);

	return Removed;
}

// W47 — Save game capture/apply. Same strip-then-rebuild shape as
// `UPaldarkPlayerPalRosterComponent::CaptureSnapshot`. Deposit list is
// hub-persistent (the per-match active roster resets every raid), so this
// is the field that actually carries cross-match progression.
FPaldarkRosterSnapshot UPaldarkPalDepositComponent::CaptureSnapshot() const
{
	FPaldarkRosterSnapshot Out;
	Out.Entries.Reserve(DepositedEntries.Num());
	for (const FPaldarkRosterEntry& E : DepositedEntries)
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

void UPaldarkPalDepositComponent::ApplySnapshot(const FPaldarkRosterSnapshot& Snapshot)
{
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		UE_LOG(LogPaldarkPal, Warning,
			TEXT("UPaldarkPalDepositComponent::ApplySnapshot rejected: non-authority"));
		return;
	}

	DepositedEntries.Reset();
	const int32 ToApply = FMath::Min(Snapshot.Entries.Num(), MaxDepositSlots);
	int32 DroppedInvalid = 0;
	int32 DroppedCap = FMath::Max(0, Snapshot.Entries.Num() - MaxDepositSlots);
	DepositedEntries.Reserve(ToApply);

	for (int32 i = 0; i < ToApply; ++i)
	{
		const FPaldarkRosterEntrySaved& Saved = Snapshot.Entries[i];
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
		const FString FullTag = Saved.SpeciesTag.GetTagName().ToString();
		int32 LastDot = INDEX_NONE;
		if (FullTag.FindLastChar(TEXT('.'), LastDot) && LastDot < FullTag.Len() - 1)
		{
			const FString Leaf = FullTag.Mid(LastDot + 1);
			E.PalDefinitionId = FPrimaryAssetId(TEXT("PaldarkPalDefinition"), FName(*Leaf));
		}
		DepositedEntries.Add(E);
	}
	OnDepositChanged.Broadcast(this);

	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkPalDepositComponent::ApplySnapshot — applied=%d dropped_invalid=%d dropped_cap=%d (snapshot size=%d, cap=%d)"),
		DepositedEntries.Num(), DroppedInvalid, DroppedCap,
		Snapshot.Entries.Num(), MaxDepositSlots);
}

void UPaldarkPalDepositComponent::DumpToLog() const
{
	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkPalDepositComponent::DumpToLog — owner=%s count=%d/%d"),
		GetOwner() ? *GetOwner()->GetName() : TEXT("<no owner>"),
		DepositedEntries.Num(), MaxDepositSlots);

	for (int32 Idx = 0; Idx < DepositedEntries.Num(); ++Idx)
	{
		const FPaldarkRosterEntry& E = DepositedEntries[Idx];
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

void UPaldarkPalDepositComponent::OnRep_DepositedEntries()
{
	OnDepositChanged.Broadcast(this);
}
