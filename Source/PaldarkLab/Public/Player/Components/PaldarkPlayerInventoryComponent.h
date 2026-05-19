// PALDARK W11-12 — Player inventory component (real implementation).
//
// W3-4 shipped this class as an empty UActorComponent slot so the player pawn
// constructor could `CreateDefaultSubobject<UPaldarkPlayerInventoryComponent>`
// without forward-declaring future work. W11-12 lights it up:
//
//   * `FPaldarkInventoryEntry` — replicated row { ItemDef, StackCount }.
//   * `Entries` — TArray<FPaldarkInventoryEntry>, replicated, OnRep hook
//     fires a single multicast delegate so HUD / activity selection can react.
//   * `MaxWeightKg` — encumbrance cap (default 30kg). `GetCurrentWeightKg`
//     sums `WeightFragment->WeightKgPerUnit * StackCount` across entries.
//   * Authority API: `AddItem`, `RemoveItemByTag`, `DropAllItems` — all
//     server-only; clients receive the replicated `Entries` snapshot.
//   * Lookup helpers: `FindEntryIndexByTag`, `FindFirstItemDefByTag`.
//
// Pickup actor + UMG widget are NOT in W11-12 scope — they consume this
// component once UMG lands. Until then designers drive the inventory via the
// `Paldark.Inventory.*` console commands in PaldarkLab.cpp.
//
// Reference: 09.Udemy-ue5-inventory-systems → "Item Manager Component" chapter.
// Vendor-neutral port — no Druid namespace, no FastArraySerializer (W11-12
// uses plain DOREPLIFETIME; FastArraySerializer is W21-24 polish).
//
// W37-38 — Inventory full. Three additions on top of W11-12:
//   * `EquippedBackpack` (TSoftObjectPtr<UPaldarkItemDefinition>, replicated):
//     The currently-worn backpack item def. Its `UPaldarkItemFragment_Backpack`
//     contribution adds `MaxWeightBonusKg` to the encumbrance cap and surfaces
//     `ExtraSlots` for a future UMG grid (W50).
//   * `BaseMaxWeightKg` (replicated, designer-tuned default 30 kg) plus the
//     derived `GetMaxWeightKg()` that returns base + backpack bonus.
//   * `FPaldarkInventoryEntry::InnerEntries` to carry composite-container
//     contents one level deep. Container item defs declare a
//     `UPaldarkItemFragment_Container` with `InnerMaxWeightKg` / `InnerMaxSlots`
//     and the inventory enforces both at runtime AddItem time. Cycle guard:
//     a container item cannot contain itself.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Save/PaldarkSaveTypes.h"
#include "PaldarkPlayerInventoryComponent.generated.h"

class UPaldarkItemDefinition;

// One row in the inventory grid. ItemDef is a soft pointer so a fresh client
// doesn't pay the cost of resolving every item asset on first join — the
// inventory UI loads on demand. StackCount is clamped to [1, MaxStack] by the
// component, never written directly by gameplay code.
//
// W37-38 — `InnerEntries` opt-in nested content for items carrying a
// `UPaldarkItemFragment_Container` fragment (composite container). Stored
// inline so a single DOREPLIFETIME on the outer `Entries` array replicates
// the whole tree. Container items must not contain themselves (validated at
// AddItem time).
//
// W39 — L-32 scaffold (Q3 Bug-Bash, P2). The struct is *prepared* for the
// W47 FastArraySerializer migration but is NOT yet a FastArraySerializerItem.
// See `MigrationReplicationKey` below and the migration plan in the W39 §
// L-32 closure section of `PaldarkLab/README.md` for the four-step W47
// conversion. Until W47 lands, replication is the same plain
// `DOREPLIFETIME(TArray<FPaldarkInventoryEntry>)` shape W37-38 shipped — the
// scaffold only threads through the *callers* (component + bag) so the W47
// PR can swap the storage with no public-API churn.
USTRUCT(BlueprintType)
struct PALDARKLAB_API FPaldarkInventoryEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Inventory|Entry")
	TSoftObjectPtr<UPaldarkItemDefinition> ItemDef;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Inventory|Entry", meta = (ClampMin = "1"))
	int32 StackCount = 1;

	// W37-38 — nested container payload. Only populated when ItemDef carries
	// a Container fragment; otherwise empty. Inner rows themselves cannot
	// recurse further (one level of nesting only — depth > 1 is rejected by
	// AddItem to keep replication bounded).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Inventory|Entry")
	TArray<FPaldarkInventoryEntry> InnerEntries;

	// W39 — L-32 scaffold. Per-row monotonic id stamped by the owning
	// `UPaldarkPlayerInventoryComponent` (or `APaldarkLootBag`) at the
	// moment a fresh entry is appended to the outer container. NOT
	// replicated today (the outer `TArray<FPaldarkInventoryEntry>` is
	// replicated as a single DOREPLIFETIME blob and per-row identity is
	// implicit-by-position), but provides a stable handle callers can use
	// to correlate logs / reorder detection across snapshots without
	// pinning a row's array index.
	//
	// W47 migration path — when `FPaldarkInventoryEntry` inherits
	// `FFastArraySerializerItem`, the base class will provide its own
	// `int32 ReplicationKey` field (auto-managed by FastArraySerializer
	// to track per-item dirty state). The W47 PR will REMOVE
	// `MigrationReplicationKey` and route the existing callers through
	// the inherited `ReplicationKey` instead — the name divergence here
	// is intentional so future-me doesn't merge-conflict with the base
	// class field. Document the migration in `README.md` § W39 — L-32
	// closure (Inventory FastArraySerializer scaffold).
	//
	// `Transient` so save-game (W47) doesn't persist the per-process key;
	// `NotReplicated` so the field is omitted from the outer array's
	// replication payload (otherwise every row pays an extra 4 bytes for
	// data the client doesn't need until FastArraySerializer lands).
	UPROPERTY(NotReplicated, Transient, BlueprintReadOnly, Category = "Paldark|Inventory|Entry|Migration")
	int32 MigrationReplicationKey = 0;

	bool IsValid() const { return !ItemDef.IsNull() && StackCount > 0; }
};

// Fired on both authority and clients when the inventory contents change
// (entry added/removed/stack updated, or weight cap changes). Listeners
// should re-read `GetEntries()` rather than diff individual rows — this is
// the simplest contract that survives the OnRep granularity of TArray<>
// without a custom FastArraySerializer.
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPaldarkInventoryChanged, class UPaldarkPlayerInventoryComponent* /*Component*/);

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent), Abstract = false)
class PALDARKLAB_API UPaldarkPlayerInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkPlayerInventoryComponent();

	// Replicated state must be exposed for the engine's replication graph.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ---- Authority API ------------------------------------------------------
	// All Add/Remove/Drop operations are authority-only. Clients calling them
	// log a warning and return 0. The UI / pickup actor / future drop ability
	// will route through a server RPC owned by the player pawn, not directly
	// here, so this component stays single-responsibility.

	// Attempts to add `RequestedCount` copies of `ItemDef` to the inventory.
	// Returns the number actually added (0 = no capacity / weight cap / not
	// authority). Stack splitting is automatic: if the item is stackable, the
	// existing entry fills to MaxStack first, then a new entry opens for the
	// remainder; non-stackable items get one entry per copy.
	int32 AddItem(const TSoftObjectPtr<UPaldarkItemDefinition>& ItemDef, int32 RequestedCount = 1);

	// Removes up to `RequestedCount` copies of any entry whose ItemDef matches
	// `ItemTag`. Returns the number actually removed. Entries that drop to
	// StackCount<=0 are erased from the array.
	int32 RemoveItemByTag(const FGameplayTag& ItemTag, int32 RequestedCount = 1);

	// Wipes the entire inventory. W11-12 just clears the array — actual world
	// pickup actor spawning lands when the pickup-actor class is authored
	// (P11 capstone, follow-up PR).
	void DropAllItems();

	// ---- Queries ------------------------------------------------------------
	const TArray<FPaldarkInventoryEntry>& GetEntries() const { return Entries; }

	// Current vs. cap encumbrance (kg). Authoritative reading sums Weight
	// fragment contributions on the *loaded* ItemDef — if the soft pointer
	// hasn't resolved yet (cold replication), that row contributes 0kg until
	// the next refresh. Treat the value as an estimate on freshly-joined
	// clients.
	//
	// W37-38 — current weight includes nested container `InnerEntries`. The
	// cap is `BaseMaxWeightKg + (EquippedBackpack ? backpack.MaxWeightBonusKg : 0)`.
	float GetCurrentWeightKg() const;
	float GetMaxWeightKg() const;
	float GetBaseMaxWeightKg() const { return BaseMaxWeightKg; }
	bool  IsOverEncumbered() const { return GetCurrentWeightKg() > GetMaxWeightKg() + KINDA_SMALL_NUMBER; }

	// Lookups. Returns INDEX_NONE / nullptr if not found.
	int32 FindEntryIndexByTag(const FGameplayTag& ItemTag) const;
	const UPaldarkItemDefinition* FindFirstItemDefByTag(const FGameplayTag& ItemTag) const;

	// ---- W37-38 — Backpack equip ------------------------------------------
	// Replaces the currently-equipped backpack item def with `InItemDef`.
	// Authority-only. Pass an empty soft ptr to clear. Returns true if the
	// backpack changed (false on auth-fail / null with no current backpack /
	// def missing a Backpack fragment).
	bool SetEquippedBackpack(const TSoftObjectPtr<UPaldarkItemDefinition>& InItemDef);

	const TSoftObjectPtr<UPaldarkItemDefinition>& GetEquippedBackpack() const { return EquippedBackpack; }
	int32 GetExtraSlotsFromBackpack() const;

	// Listeners (HUD, activity selection, debug overlay). Multicast — bind in
	// BeginPlay and unbind in EndPlay; fired both server-side (post-mutation)
	// and client-side (post-OnRep).
	FOnPaldarkInventoryChanged OnInventoryChanged;

	// ---- Debug --------------------------------------------------------------
	// Logs the entire inventory state to LogPaldark (one line per entry,
	// total weight, cap). Called by the `Paldark.Inventory.List` console
	// command in PaldarkLab.cpp.
	void DumpToLog() const;

	// W37-38 — Logs each row plus a nested dump of any container `InnerEntries`.
	// Backs the `Paldark.Inventory.DumpComposite` console command.
	void DumpCompositeToLog() const;

	// ---- W47 — Save game capture/apply ------------------------------------
	// Captures the inventory state as a `FPaldarkInventorySnapshot`. Soft-ref
	// item defs are resolved to their `ItemTag` so the blob is content-patch
	// safe. Nested `InnerEntries` (composite containers, W37-38) are FLATTENED
	// into the top-level entry list — restoring nested layout is a W48 polish
	// task. The equipped backpack is captured as a separate tag so the load
	// path can re-equip it before applying the rest of the entries.
	//
	// `CaptureSnapshot` is `const`; safe to call from any thread that
	// already holds a stable reference to this component.
	FPaldarkInventorySnapshot CaptureSnapshot() const;

	// Applies a previously-captured snapshot. Authority-only. Wipes the
	// current `Entries` array + `EquippedBackpack` and rebuilds them from
	// the snapshot. ItemTag → ItemDef lookup goes through `UAssetManager`'s
	// primary asset table (Pal item defs are registered under
	// `PrimaryAssetType=PaldarkItemDefinition` — see W37-38). Rows whose tag
	// no longer resolves are silently dropped with a Warning log entry so
	// designer asset renames don't brick saves.
	void ApplySnapshot(const FPaldarkInventorySnapshot& Snapshot);

	// ---- W39 — L-32 scaffold (FastArraySerializer migration prep) ---------

	// Thread-safe monotonic counter used to stamp each freshly-appended
	// `FPaldarkInventoryEntry::MigrationReplicationKey` with a unique id.
	// Shared process-wide (server-side) so keys are unique across players in
	// a match — useful for log correlation when a row hops between
	// inventories (e.g. pickup → drop → pickup). In W47 this is replaced
	// by `FFastArraySerializerItem`'s automatically-managed `ReplicationKey`
	// (the base class increments it inside `MarkItemDirty`). Callers should
	// stamp NEWLY-CREATED rows only — top-off-existing-stack should NOT
	// reissue a key (the row keeps its previous id).
	//
	// The counter is **never** persisted (save-game has no `ReplicationKey`
	// column — the field is `Transient`). On dedicated-server restart the
	// counter resets to 0; that's fine because clients re-receive the
	// entire `Entries` array on PostNetInit anyway.
	static int32 BuildNextReplicationKey();

	// W39 — L-32 scaffold. Single funnel for inventory mutation notifications.
	// Today this is the only call site for `OnInventoryChanged.Broadcast(this)`
	// — every Add / Remove / Drop / SetEquippedBackpack / OnRep_* path routes
	// through here. In W47 the FastArraySerializer migration will route
	// `EntryIndex >= 0` through `FastArraySerializer::MarkItemDirty(Entries[idx])`
	// for per-row delta replication, while `INDEX_NONE` keeps the array-wide
	// broadcast (used for cap change, backpack swap, drop-all). Threading
	// this funnel ahead of W47 means the migration PR can swap the
	// implementation without touching every call site.
	//
	// `EntryIndex` is the position in the outer `Entries` array AFTER the
	// mutation has been applied. Pass `INDEX_NONE` if the mutation doesn't
	// map to a single row (e.g. encumbrance-cap change, drop-all, backpack
	// swap). Bounds-check is best-effort: an out-of-range index logs a
	// Verbose-level note and is treated as `INDEX_NONE` (the broadcast
	// still fires so listeners refresh correctly).
	void MarkInventoryDirty(int32 EntryIndex = INDEX_NONE);

protected:
	virtual void BeginPlay() override;

	// W37-38 — Designer-tuned base encumbrance cap (default 30 kg). The
	// effective cap returned by `GetMaxWeightKg()` adds the equipped
	// backpack's `MaxWeightBonusKg` contribution on top.
	UPROPERTY(EditDefaultsOnly, Replicated, BlueprintReadOnly, Category = "Paldark|Inventory", meta = (ClampMin = "0.0", Units = "Kilograms"))
	float BaseMaxWeightKg = 30.0f;

	// Replicated entry list. OnRep dispatches OnInventoryChanged on clients.
	UPROPERTY(ReplicatedUsing = OnRep_Entries)
	TArray<FPaldarkInventoryEntry> Entries;

	// W37-38 — currently-worn backpack item definition (soft ptr to avoid
	// pulling its mesh / icon into memory until the inventory UI opens).
	// Replicated; OnRep fires the OnInventoryChanged delegate so HUD code
	// recomputes its weight bar.
	UPROPERTY(ReplicatedUsing = OnRep_EquippedBackpack)
	TSoftObjectPtr<UPaldarkItemDefinition> EquippedBackpack;

	UFUNCTION()
	void OnRep_Entries();

	UFUNCTION()
	void OnRep_EquippedBackpack();

private:
	// Internal helper: try to top off an existing stackable entry. Returns
	// the number of copies actually placed on the existing row (0 if the
	// item isn't stackable or the row is already at MaxStack).
	int32 TopOffExistingStack(int32 EntryIndex, int32 RequestedCount);

	// Resolves the ItemDef ptr synchronously. LoadSynchronous is acceptable
	// in W11-12 — async + bundle preloading is a P14 follow-up.
	static const UPaldarkItemDefinition* LoadItemDefSync(const TSoftObjectPtr<UPaldarkItemDefinition>& ItemDef);
};
