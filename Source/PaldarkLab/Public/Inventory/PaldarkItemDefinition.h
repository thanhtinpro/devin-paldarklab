// PALDARK W11-12 — Item definition (UPrimaryDataAsset).
//
// Top-level asset class for every gameplay item. One DataAsset per item:
//   /Game/Paldark/Items/DA_Item_Pistol
//   /Game/Paldark/Items/DA_Item_Ammo_9mm
//   /Game/Paldark/Items/DA_Item_Bandage
//   /Game/Paldark/Items/DA_Item_Pal_Sphere
//   /Game/Paldark/Items/DA_Item_Energy_Drink
//
// Composition over inheritance: per-item behaviour (stackability, weight,
// equip slot, …) lives on a TArray of `UPaldarkItemFragment` instanced
// subobjects rather than on the definition itself. Adding a new behaviour
// axis (e.g. "Consumable" healing/buff) is a matter of authoring a new
// fragment subclass and dropping it onto the affected assets — no edits to
// this header.
//
// Registered as an Asset Manager primary asset type ("PaldarkItem") via
// `[/Script/Engine.AssetManagerSettings]` rows in `Config/DefaultGame.ini`
// and `UPaldarkAssetManager` runtime registration. The primary asset id is
// `PaldarkItem:<AssetName>` (Lyra convention).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PaldarkItemDefinition.generated.h"

class UPaldarkItemFragment;

UCLASS(BlueprintType, Const, meta = (DisplayName = "Paldark Item Definition"))
class PALDARKLAB_API UPaldarkItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPaldarkItemDefinition();

	// Stable identity tag for this item. Used as the lookup key in
	// `UPaldarkPlayerInventoryComponent::FindEntryByTag` and the console
	// commands (`Paldark.Inventory.Add Paldark.Item.Pistol`). Should be one of
	// the Paldark.Item.* tags from PaldarkGameplayTags so save data + console
	// stays stable even if a designer renames the asset path.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Inventory|Item", meta = (Categories = "Paldark.Item"))
	FGameplayTag ItemTag;

	// Localised display name shown in the inventory UI / pickup prompt. Free
	// to leave empty for tests — `GetDebugLabel` falls back to ItemTag.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Inventory|Item")
	FText DisplayName;

	// Per-instance fragment composition. EditInline + Instanced means the
	// editor lets a designer add a fragment subclass and tune its fields
	// inside the same asset (each fragment is a unique UObject owned by this
	// definition). Null entries are tolerated at access time but trigger a
	// CI validator warning.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category = "Paldark|Inventory|Item")
	TArray<TObjectPtr<UPaldarkItemFragment>> Fragments;

	// Returns the first fragment that matches `FragmentClass` (exact or
	// subclass). Nullptr if no match. Linear scan — fragment arrays are tiny
	// (<= 5 elements in practice) so a map index is not worth the overhead.
	UPaldarkItemFragment* FindFragmentByClass(TSubclassOf<UPaldarkItemFragment> FragmentClass) const;

	// Templated helper. Caller specifies the concrete fragment type and gets
	// back a typed pointer, avoiding the cast at the call site.
	template <typename TFragment>
	TFragment* FindFragmentByClass() const
	{
		return Cast<TFragment>(FindFragmentByClass(TFragment::StaticClass()));
	}

	// Short debug label used by the inventory dump console command. Returns
	// `DisplayName` when set, otherwise the ItemTag string, otherwise the
	// asset name.
	FString GetDebugLabel() const;

	// Primary asset id with the `PaldarkItem` type. Matches the row in
	// `[/Script/Engine.AssetManagerSettings]` so editor + runtime discovery
	// stay aligned.
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
