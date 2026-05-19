// PALDARK W47 — Save game payload structs (player progression).
//
// These USTRUCTs are the *serialisable* shape of the runtime components. We
// don't reuse `FPaldarkRosterEntry` / `FPaldarkInventoryEntry` directly in
// the save blob for two reasons:
//
//   1. Runtime structs carry replication-only fields (e.g. the L-32
//      migration scaffold's `MigrationReplicationKey`) that we don't want
//      to persist. Saved-form structs drop those.
//
//   2. Soft-ptr `TSoftObjectPtr<UPaldarkItemDefinition>` content is brittle
//      across content patches — if an item's asset path is renamed the
//      save blob breaks. Saved entries store `Paldark.Item.X` tags instead;
//      load-time lookup goes through the runtime tag table and silently
//      drops rows whose tags no longer resolve. This lets designers rename
//      assets without bumping `SchemaVersion`.
//
// Out of scope (W47 scaffold):
//   * No nested-container persistence (W37-38 InnerEntries) — flattened on
//     save, restored as top-level entries on load. Designer-driven loadouts
//     for containers ship in W48 polish.
//   * No per-row replication key — the FastArraySerializer migration prep
//     (W39 L-32 scaffold) explicitly marked the key `Transient` so this is
//     a no-op decision, but documented here for future-me.
//   * No cosmetic state (skin tints, emote presets) — cosmetic system
//     lands Q1-next. `FPaldarkProgressSnapshot::CosmeticsPlaceholder` is
//     reserved for forward-compat (always-empty container today).

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PaldarkSaveTypes.generated.h"

// One Pal in the saved roster / deposit. Mirrors `FPaldarkRosterEntry`'s
// public fields minus the `PalDefinitionId` soft-ref (resolved on load via
// the species tag → PalDefinition asset table).
USTRUCT(BlueprintType)
struct PALDARKLAB_API FPaldarkRosterEntrySaved
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Save|Roster")
	FGameplayTag SpeciesTag;

	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Save|Roster")
	int32 BondLevel = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Save|Roster")
	float HealthAtTame = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Save|Roster")
	float MaxHealthAtTame = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Save|Roster")
	FString Nickname;

	bool IsValid() const { return SpeciesTag.IsValid(); }
};

// Saved-form for the active roster (12-slot match loadout) or the deposit
// list (24-slot hub-persistent storage). Same shape — only the consumer
// component differs.
USTRUCT(BlueprintType)
struct PALDARKLAB_API FPaldarkRosterSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Save|Roster")
	TArray<FPaldarkRosterEntrySaved> Entries;

	int32 Num() const { return Entries.Num(); }
	bool IsEmpty() const { return Entries.Num() == 0; }
};

// One row in the saved inventory. ItemTag replaces `FPaldarkInventoryEntry::
// ItemDef` (soft-ptr) so the blob survives asset renames. `InnerEntries` is
// dropped on save and the contents are flattened into the top level — this
// is a deliberate W47 scoping choice (composite-container persistence ships
// in W48 polish).
USTRUCT(BlueprintType)
struct PALDARKLAB_API FPaldarkInventoryEntrySaved
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Save|Inventory")
	FGameplayTag ItemTag;

	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Save|Inventory")
	int32 StackCount = 1;

	bool IsValid() const { return ItemTag.IsValid() && StackCount > 0; }
};

// Saved-form for the player inventory component. The backpack tier item
// (W37-38 EquippedBackpack) is captured as a separate tag so the load
// path can re-equip it before applying the rest of the entries (so the
// derived weight cap matches the save-time cap).
USTRUCT(BlueprintType)
struct PALDARKLAB_API FPaldarkInventorySnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Save|Inventory")
	TArray<FPaldarkInventoryEntrySaved> Entries;

	// Tag of the equipped backpack (or empty if unequipped). Resolved on
	// load via the runtime item table.
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Save|Inventory")
	FGameplayTag EquippedBackpackTag;

	int32 Num() const { return Entries.Num(); }
};

// Misc per-character progression state. Today this is a thin shim — the
// tutorial / unlock systems land in W48-49 polish. The fields are placed
// here so the save blob doesn't grow new top-level fields per polish week.
USTRUCT(BlueprintType)
struct PALDARKLAB_API FPaldarkProgressSnapshot
{
	GENERATED_BODY()

	// Tags whose presence marks "this onboarding step has been seen". UMG
	// tutorial widget consults this on hub spawn to decide whether to
	// re-prompt.
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Save|Progress")
	FGameplayTagContainer CompletedTutorials;

	// Map ids that have been unlocked through hub progression. Designer can
	// gate `Map_PalCorp` behind a quest by clearing the corresponding tag
	// here on a fresh save.
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Save|Progress")
	FGameplayTagContainer UnlockedExperiences;

	// Last hub map the player was on before save. On load, the subsystem
	// can use this to decide whether to ServerTravel to that map first
	// (W48 polish — today we just log it).
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Save|Progress")
	FName LastHubMapName;

	// Total raids completed (any outcome). Surface in the load-game preview
	// blurb so designers can quickly differentiate save slots.
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Save|Progress")
	int32 TotalRaidsCompleted = 0;

	// W47 reserves a generic tag container for cosmetic state (skin tints,
	// emote presets) so the schema doesn't need a version bump when the
	// cosmetic system lands in Q1-next. Always empty in W47.
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Save|Progress")
	FGameplayTagContainer CosmeticsPlaceholder;
};
