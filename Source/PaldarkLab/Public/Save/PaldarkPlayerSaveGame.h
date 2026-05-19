// PALDARK W47 — Player save game container (USaveGame subclass).
//
// Top-level container that `UGameplayStatics::AsyncSaveGameToSlot` serialises
// to disk. The class is *intentionally* a thin DTO — all heavy lifting
// (capturing live component state, applying it back to live components,
// async write/read orchestration) lives in `UPaldarkSaveSubsystem`. This
// separation matches Lyra's `ULyraSaveGame` shape and keeps the save asset
// free of transient runtime ptrs.
//
// SchemaVersion guard:
//   * Default value is `kCurrentSchema = 1`. Bump on every breaking change
//     (renamed UPROPERTY, new required field, struct field reorder).
//   * Load path in `UPaldarkSaveSubsystem::HandleLoadComplete` rejects any
//     blob whose `SchemaVersion != kCurrentSchema` with
//     `Paldark.Save.Result.Fail.SchemaMismatch`. Migration hooks land in
//     W48 polish.
//
// Storage backend:
//   * `UGameplayStatics::AsyncSaveGameToSlot(slot, userIdx)` → writes to
//     `<project>/Saved/SaveGames/<slot>.sav`.
//   * `UGameplayStatics::AsyncLoadGameFromSlot(slot, userIdx)` → reads.
//   * Cloud / Steam / EOS backends are Q4-stretch; the subsystem's API
//     surface is forward-compat so a future PR can swap the backend
//     without churning callers.
//
// What this class does NOT carry:
//   * Pal companion AI state (active waypoint, current activity tag) —
//     re-derived from `Paldark.Pal.Activity.Idle` on respawn.
//   * Squad command history / ping history — transient, per-match.
//   * Match phase / extraction state — match-scoped, never persisted.
//   * Match results / loot / KIA history — separate "Match Result" save
//     (W48-49 polish; out of W47 scope).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Misc/DateTime.h"
#include "Save/PaldarkSaveTypes.h"
#include "PaldarkPlayerSaveGame.generated.h"

UCLASS(BlueprintType)
class PALDARKLAB_API UPaldarkPlayerSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPaldarkPlayerSaveGame();

	// Schema version this blob was written under. Compared against
	// `kCurrentSchema` at load time; mismatches reject the blob.
	static constexpr int32 kCurrentSchema = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Save|Meta")
	int32 SchemaVersion = kCurrentSchema;

	// UTC timestamp of when the snapshot was captured. Shown in the load
	// preview blurb so designers can disambiguate slots quickly.
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Save|Meta")
	FDateTime SaveTime;

	// Cached `APlayerState::GetPlayerName()` at save time. Pure debug aid —
	// the load path ignores this field and applies the snapshot to whoever
	// owns the active PlayerController.
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Save|Meta")
	FString PlayerName;

	// Active roster (the W35-36 12-slot match loadout).
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Save|Roster")
	FPaldarkRosterSnapshot ActiveRoster;

	// Hub-persistent deposit (the W44-45 24-slot stable storage).
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Save|Roster")
	FPaldarkRosterSnapshot DepositRoster;

	// Player inventory snapshot. Credits balance is folded in as an entry
	// row whose tag is `Paldark.Item.Resource.Currency.Credits` — no
	// separate top-level field needed.
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Save|Inventory")
	FPaldarkInventorySnapshot Inventory;

	// Misc progression state (tutorials, unlocked experiences, last map).
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Save|Progress")
	FPaldarkProgressSnapshot Progress;

	// Convenience for the dump command — formats the metadata into a single
	// line for `LogPaldarkLab` consumption.
	FString DescribeForLog() const;
};
