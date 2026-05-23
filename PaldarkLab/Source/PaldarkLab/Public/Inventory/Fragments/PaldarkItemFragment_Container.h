// PALDARK W37-38 — Container fragment.
//
// Declares that an item is itself a *container* — it owns a nested inventory
// of `FPaldarkInventoryEntry` rows stored inside the outer entry's
// `InnerEntries` array (see `UPaldarkPlayerInventoryComponent`). Designer
// drops a Container fragment on a definition like `DA_Item_LootCrate` and
// authors the default contents on the entry that spawns from a loot table.
//
// W37-38 scope: the fragment carries the **caps** (max inner weight + max
// inner slots) used by `AddItem`-into-container helpers when they land in a
// follow-up PR; runtime nested put/take is currently authority-only via
// console commands. The container item itself still pays its own
// `UPaldarkItemFragment_Weight` contribution in the outer inventory, on top
// of the cumulative weight of the inner items (the player carries
// everything in the world model).
//
// Anti-pattern guard: containers MUST NOT contain themselves (cycle). The
// inventory component scans nested rows when adding a container and refuses
// to add a row whose ItemDef matches the outer container's ItemDef.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/PaldarkItemFragment.h"
#include "PaldarkItemFragment_Container.generated.h"

UCLASS(DisplayName = "Container")
class PALDARKLAB_API UPaldarkItemFragment_Container : public UPaldarkItemFragment
{
	GENERATED_BODY()

public:
	// Total weight cap of items inside the container (separate from outer
	// inventory weight). Outer inventory still pays the container item's
	// own `Weight` fragment + the cumulative weight of inner rows. Clamped
	// to >= 0 at access time.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Inventory|Container",
		meta = (ClampMin = "0.0", UIMin = "0.0", Units = "Kilograms"))
	float InnerMaxWeightKg = 5.0f;

	// Hard cap on the number of inner entries. Replicated nested arrays are
	// the bottleneck so the cap is intentionally tight; FastArraySerializer
	// migration in W47 raises this.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Inventory|Container",
		meta = (ClampMin = "0", UIMin = "0", ClampMax = "16"))
	int32 InnerMaxSlots = 4;

	float GetInnerMaxWeightKg() const { return FMath::Max(InnerMaxWeightKg, 0.0f); }
	int32 GetInnerMaxSlots() const { return FMath::Clamp(InnerMaxSlots, 0, 16); }

	virtual FString GetDebugDescription() const override;
};
