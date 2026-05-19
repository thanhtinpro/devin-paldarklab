// PALDARK W33-34 — Loot drop component implementation.

#include "Loot/PaldarkLootDropComponent.h"

#include "AbilitySystemComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "Gas/PaldarkAttributeSet.h"
#include "GameFramework/Actor.h"
#include "Loot/PaldarkLootBag.h"
#include "Loot/PaldarkLootTable.h"
#include "Math/RandomStream.h"
#include "Pal/PaldarkPalCharacter.h"
#include "PaldarkLogCategories.h"

UPaldarkLootDropComponent::UPaldarkLootDropComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UPaldarkLootDropComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}
	if (!LootTableId.IsValid())
	{
		// No-op state — leave HealthZeroedHandle unbound so a hot designer
		// reload that fills in the id later can re-init via BeginPlay if
		// needed. We don't auto-bind because subscribing without a table is
		// just dead callback work on every Pal death broadcast.
		return;
	}

	APaldarkPalCharacter* Pal = Cast<APaldarkPalCharacter>(Owner);
	if (!Pal)
	{
		UE_LOG(LogPaldarkInventory, Warning,
			TEXT("LootDropComponent attached to non-Pal owner %s — skipping."),
			*Owner->GetName());
		return;
	}

	// AttributeSet pointer comes through the GAS interface. We don't
	// `Cast` directly to avoid pulling AbilitySystem from a const Pal
	// accessor on the const subobject.
	const UPaldarkAttributeSet* AttrSet = Pal->GetPaldarkAttributeSet();
	if (!AttrSet)
	{
		UE_LOG(LogPaldarkInventory, Warning,
			TEXT("LootDropComponent on %s — no Paldark AttributeSet, binding skipped."),
			*Owner->GetName());
		return;
	}

	// Cast away const for the delegate add — the AttributeSet exposes the
	// multicast as a mutable member, the const accessor is just a
	// convenience. Safe because AttributeSet lifetime > component lifetime.
	UPaldarkAttributeSet* MutableAttrSet = const_cast<UPaldarkAttributeSet*>(AttrSet);
	HealthZeroedHandle = MutableAttrSet->OnHealthZeroed.AddUObject(this,
		&UPaldarkLootDropComponent::HandleHealthZeroed);
}

void UPaldarkLootDropComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HealthZeroedHandle.IsValid())
	{
		if (const APaldarkPalCharacter* Pal = Cast<APaldarkPalCharacter>(GetOwner()))
		{
			if (UPaldarkAttributeSet* MutableAttrSet =
				const_cast<UPaldarkAttributeSet*>(Pal->GetPaldarkAttributeSet()))
			{
				MutableAttrSet->OnHealthZeroed.Remove(HealthZeroedHandle);
			}
		}
		HealthZeroedHandle.Reset();
	}
	Super::EndPlay(EndPlayReason);
}

void UPaldarkLootDropComponent::HandleHealthZeroed(UAbilitySystemComponent* /*Instigator*/)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority()) { return; }
	if (!LootTableId.IsValid()) { return; }

	// Capture transform + context tags by value before the owner gets
	// destroyed by the death pipeline. Async load callback fires after
	// destroy.
	const FTransform SpawnTransform = FTransform(Owner->GetActorRotation(),
		Owner->GetActorLocation() + SpawnOffset);
	FGameplayTagContainer ContextTags;
	if (const APaldarkPalCharacter* Pal = Cast<APaldarkPalCharacter>(Owner))
	{
		const FGameplayTag SpeciesTag = Pal->GetSpeciesTag();
		if (SpeciesTag.IsValid()) { ContextTags.AddTag(SpeciesTag); }
		const FGameplayTag TeamTag = Pal->GetTeamTag();
		if (TeamTag.IsValid()) { ContextTags.AddTag(TeamTag); }
	}

	const FPrimaryAssetId TableId = LootTableId;

	UAssetManager& Manager = UAssetManager::Get();
	const FStreamableDelegate Delegate = FStreamableDelegate::CreateUObject(this,
		&UPaldarkLootDropComponent::OnLootTableLoaded,
		TableId, SpawnTransform, ContextTags);
	Manager.LoadPrimaryAsset(TableId, TArray<FName>(), Delegate);
}

void UPaldarkLootDropComponent::OnLootTableLoaded(const FPrimaryAssetId InLootTableId,
	const FTransform InSpawnTransform, const FGameplayTagContainer InContextTags)
{
	UWorld* World = GetWorld();
	if (!World) { return; }

	UAssetManager& Manager = UAssetManager::Get();
	UObject* RawAsset = Manager.GetPrimaryAssetObject(InLootTableId);
	UPaldarkLootTable* Table = Cast<UPaldarkLootTable>(RawAsset);
	if (!Table)
	{
		UE_LOG(LogPaldarkInventory, Warning,
			TEXT("LootDropComponent — failed to resolve LootTable %s after async load."),
			*InLootTableId.ToString());
		return;
	}

	// Seed RNG with engine timestamp + table name hash so two concurrent
	// kills don't produce identical rolls but the result is debuggable
	// from logs.
	FRandomStream Rng(static_cast<int32>(FPlatformTime::Cycles() ^ GetTypeHash(InLootTableId.PrimaryAssetName)));
	const TArray<FPaldarkLootRollResult> RolledEntries = Table->RollEntries(Rng, InContextTags);
	if (RolledEntries.Num() == 0)
	{
		UE_LOG(LogPaldarkInventory, Verbose,
			TEXT("LootDropComponent — LootTable '%s' rolled zero entries."),
			*Table->GetDebugLabel());
		return;
	}

	TSubclassOf<APaldarkLootBag> SpawnClass = LootBagClass.Get();
	if (!SpawnClass) { SpawnClass = APaldarkLootBag::StaticClass(); }

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	APaldarkLootBag* Bag = World->SpawnActor<APaldarkLootBag>(SpawnClass, InSpawnTransform, SpawnParams);
	if (!Bag)
	{
		UE_LOG(LogPaldarkInventory, Warning,
			TEXT("LootDropComponent — failed to spawn LootBag for table '%s'."),
			*Table->GetDebugLabel());
		return;
	}

	TArray<FPaldarkLootBagEntry> BagEntries;
	BagEntries.Reserve(RolledEntries.Num());
	for (const FPaldarkLootRollResult& Roll : RolledEntries)
	{
		FPaldarkLootBagEntry& Entry = BagEntries.AddDefaulted_GetRef();
		Entry.ItemDefinition = Roll.ItemDefinition;
		Entry.Count = Roll.Count;
	}
	Bag->InitializeContents(BagEntries);

	UE_LOG(LogPaldarkInventory, Log,
		TEXT("LootDropComponent — LootTable '%s' dropped %d entries at %s."),
		*Table->GetDebugLabel(), BagEntries.Num(), *InSpawnTransform.GetLocation().ToCompactString());
}

void UPaldarkLootDropComponent::ForceDropAt(const FTransform& InTransform)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority()) { return; }
	if (!LootTableId.IsValid())
	{
		UE_LOG(LogPaldarkInventory, Warning,
			TEXT("LootDropComponent on %s — ForceDropAt with empty LootTableId, skipping."),
			*Owner->GetName());
		return;
	}

	FGameplayTagContainer ContextTags;
	if (const APaldarkPalCharacter* Pal = Cast<APaldarkPalCharacter>(Owner))
	{
		const FGameplayTag SpeciesTag = Pal->GetSpeciesTag();
		if (SpeciesTag.IsValid()) { ContextTags.AddTag(SpeciesTag); }
	}

	UAssetManager& Manager = UAssetManager::Get();
	const FStreamableDelegate Delegate = FStreamableDelegate::CreateUObject(this,
		&UPaldarkLootDropComponent::OnLootTableLoaded,
		LootTableId, InTransform, ContextTags);
	Manager.LoadPrimaryAsset(LootTableId, TArray<FName>(), Delegate);
}
