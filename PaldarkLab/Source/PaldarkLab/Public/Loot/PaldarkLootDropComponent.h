// PALDARK W33-34 — Loot drop component (server-only).
//
// Default subobject on `APaldarkPalCharacter` (9th component slot). Listens
// to the owning Pal's `UPaldarkAttributeSet::OnHealthZeroed` broadcast on
// the authority. On health zero:
//
//   1. Capture the Pal's transform + species tag by value into a closure
//      (the Pal will be destroyed before the async load callback fires).
//   2. Async-load the configured `LootTableId` via
//      `UAssetManager::LoadPrimaryAsset`. Coalesces by id so two Pals
//      sharing a table issue one load.
//   3. On load complete, roll the table (`RollEntries`), spawn an
//      `APaldarkLootBag` at the captured transform, and populate its
//      contents with the rolled entries.
//
// Empty `LootTableId` = no-op (component subscribes nothing). This is the
// default state on the base `APaldarkPalCharacter`, hostile-Pal subclasses
// stamp the id in their constructor / Blueprint defaults.
//
// What this is NOT (deferred):
//   - No client-side prediction (loot drop is server-authoritative and
//     replicated through `APaldarkLootBag`).
//   - No per-killer attribution. Component sees the source ASC via
//     OnHealthZeroed signature but W33-34 ignores it — the bag is fair game
//     for any overlapping player. Per-killer credit lands W37-38 economy.
//   - No drop-rate buffs (luck / magic find). Q4 polish.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/AssetManager.h"
#include "PaldarkLootDropComponent.generated.h"

class APaldarkLootBag;
class UPaldarkLootTable;

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent))
class PALDARKLAB_API UPaldarkLootDropComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkLootDropComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Designer-assigned. Primary asset id of the `UPaldarkLootTable`
	// (`PaldarkLootTable:<RowName>`) to roll on the owning Pal's death.
	// Empty = component is a no-op (default for player-side companions).
	// Hostile-Pal subclasses stamp this in their ctor or Blueprint defaults.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paldark|Loot|Drop")
	FPrimaryAssetId LootTableId;

	// Override the LootBag class to spawn. Defaults to APaldarkLootBag at
	// runtime; subclasses can swap to a custom UI-themed bag without
	// modifying the component.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Loot|Drop")
	TSubclassOf<APaldarkLootBag> LootBagClass;

	// World-space offset applied to the spawn transform so the bag doesn't
	// clip into the Pal's collision capsule. Default lifts it 20 cm.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Loot|Drop")
	FVector SpawnOffset = FVector(0.f, 0.f, 20.f);

	// Authority-only — call to force-roll the loot table and spawn a bag.
	// Used by the `Paldark.Loot.ForceDrop` console command to bypass the
	// OnHealthZeroed path during testing.
	void ForceDropAt(const FTransform& InTransform);

protected:
	// `UPaldarkAttributeSet::OnHealthZeroed` binding handle so we can
	// unsubscribe in EndPlay. Component lifetime is shorter than the
	// AttributeSet's (the Pal can be force-destroyed without ever firing
	// OnHealthZeroed).
	FDelegateHandle HealthZeroedHandle;

	// Captures Pal transform + species context tag, async-loads the table,
	// rolls + spawns a bag. Authority only.
	void HandleHealthZeroed(class UAbilitySystemComponent* Instigator);

	// Async load completion. RollResult parameter captured by value via
	// closure since the owner is already destroyed by the time this fires.
	void OnLootTableLoaded(const FPrimaryAssetId InLootTableId,
		const FTransform InSpawnTransform,
		const FGameplayTagContainer InContextTags);
};
