// PALDARK W33-34 — Loot bag (replicated pickup actor).
//
// Spawned server-side by `UPaldarkLootDropComponent` when a Pal carrying a
// configured `LootTableId` dies. Holds the rolled `(ItemDefinition, Count)`
// entries in a replicated array so clients can show pickup prompts before
// the player walks over. Pickup is a server-side bulk transfer into the
// overlapping player's `UPaldarkPlayerInventoryComponent`.
//
// W33-34 ships the minimal contract:
//   - Sphere overlap → first player to overlap triggers a server-side
//     RequestPickup (server overlap callback for authority overlaps,
//     server RPC for client-triggered ones).
//   - On RequestPickup, server iterates `ReplicatedContents` and feeds
//     each (item, count) into the requester's inventory via the standard
//     AddItem authority API.
//   - When the bag empties (all items picked up or transferred), the
//     server destroys it.
//   - 5-minute server-side auto-cleanup timer if nobody picks it up so
//     a long-running raid doesn't accumulate stale bags across the map.
//
// UMG pickup prompt + ping marker integration are deferred to W50 UI
// polish. For W33-34 the pickup is silent — designer verifies via
// `Paldark.Loot.PickupNearest` console cmd + LogPaldarkInventory.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PaldarkLootBag.generated.h"

class APaldarkCharacter;
class UPaldarkItemDefinition;
class USphereComponent;
class UStaticMeshComponent;

// One entry inside a loot bag. ItemDefinition is a soft pointer (matches
// `FPaldarkInventoryEntry`); Count is the stack size remaining in the bag.
// Replicated to clients via DOREPLIFETIME on the owning actor so HUD code
// can show pickup hints before overlap.
USTRUCT(BlueprintType)
struct PALDARKLAB_API FPaldarkLootBagEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Loot|BagEntry")
	TSoftObjectPtr<UPaldarkItemDefinition> ItemDefinition;

	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Loot|BagEntry")
	int32 Count = 0;
};

// Fired on both authority + clients when the bag's contents change (entry
// removed, count dec) or when the bag is initially populated. Listeners
// re-read GetContents() rather than diffing rows.
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPaldarkLootBagChanged, class APaldarkLootBag* /*Bag*/);

UCLASS(ClassGroup = (Paldark), meta = (DisplayName = "Paldark Loot Bag"))
class PALDARKLAB_API APaldarkLootBag : public AActor
{
	GENERATED_BODY()

public:
	APaldarkLootBag();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Authority-only — called by `UPaldarkLootDropComponent::SpawnBagAt`
	// after the spawn finishes. Bulk-populates `ReplicatedContents` and
	// fires the OnChanged delegate locally before replication catches up
	// on clients.
	void InitializeContents(const TArray<FPaldarkLootBagEntry>& InEntries);

	// Authority-only — best-effort transfer of every entry into the
	// requester's inventory. Skips entries whose ItemDefinition is null /
	// fails to resolve. Decrements `ReplicatedContents[].Count` by the
	// number actually placed in the inventory (so the bag stays around if
	// the player is encumbered + can't fit everything). When the array
	// empties, the bag self-destroys.
	void RequestPickup(APaldarkCharacter* Requester);

	const TArray<FPaldarkLootBagEntry>& GetContents() const { return ReplicatedContents; }
	bool IsEmpty() const { return ReplicatedContents.Num() == 0; }

	FOnPaldarkLootBagChanged OnBagChanged;

	// `Paldark.Loot.PickupNearest` looks for the nearest non-empty bag to
	// the local player. Surfaced here as a static helper so the console
	// command lives in `PaldarkLootConsoleCommands.cpp` without needing
	// to grep TActorIterator inline there.
	static APaldarkLootBag* FindNearestBag(const UObject* WorldContext,
		const FVector& Location, float MaxRadius = 0.f);

protected:
	// Visuals — placeholder static mesh so designers see *something* drop.
	// Replaced with real tech-themed art by the asset team. SetHiddenInGame
	// stays false (the bag is visible) so playtest builds can spot it.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Loot|Bag")
	TObjectPtr<UStaticMeshComponent> BagMesh;

	// Overlap volume that the player crosses to pick the bag up. Default
	// radius matches a 50 cm pickup zone — designer can override per-instance.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Loot|Bag")
	TObjectPtr<USphereComponent> PickupSphere;

	// Authority-only seconds before the bag auto-cleans up if nobody picked
	// it. Default 300 s. Set to 0 to disable.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Loot|Bag",
		meta = (ClampMin = "0", UIMin = "0", Units = "Seconds"))
	float AutoCleanupSeconds = 300.f;

	// Replicated entry list. OnRep fires `OnBagChanged` on clients.
	UPROPERTY(ReplicatedUsing = OnRep_ReplicatedContents)
	TArray<FPaldarkLootBagEntry> ReplicatedContents;

	UFUNCTION()
	void OnRep_ReplicatedContents();

	// Sphere overlap callback. Server-side only — clients ignore overlap
	// callbacks so RequestPickup is always authoritative. Looks for an
	// `APaldarkCharacter` requester and routes to RequestPickup.
	UFUNCTION()
	void OnPickupSphereOverlap(UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// Authority-side cleanup timer handle.
	FTimerHandle AutoCleanupHandle;

	UFUNCTION()
	void HandleAutoCleanup();
};
