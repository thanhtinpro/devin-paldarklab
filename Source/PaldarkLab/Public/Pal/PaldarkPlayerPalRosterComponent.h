// PALDARK W35-36 — Player Pal roster (per-player, match-scoped).
//
// Component lives on every `APaldarkCharacter` (13th player slot). Stores the
// player's tamed Pals for the current match. State is transient — the roster
// resets to empty on every new match, and is NOT persisted to save-game
// (W47). Replication condition is `COND_OwnerOnly` so other players can't
// observe each other's rosters (cheap UI privacy gate).
//
// Server-only mutation: `TamePal` is called by `UPaldarkPalTameComponent`
// after a successful tame attempt. Roster entry snapshots the tamed Pal's
// species + health at the time of capture; the Pal itself is destroyed
// immediately after registration (see TameComponent::RegisterAndDestroy).
//
// What this is NOT (deferred):
//   - No persistent roster across matches (W47 save-game).
//   - No "deploy from roster into match" UI flow (W37-38 polish).
//   - No bond level synchronisation across re-captures of the same species
//     — every TamePal call appends a fresh entry. De-dup is a future polish.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Save/PaldarkSaveTypes.h"
#include "PaldarkPlayerPalRosterComponent.generated.h"

class APaldarkPalCharacter;

// One Pal in the player's roster. All fields are snapshots from the moment
// of tame — the original `APaldarkPalCharacter` is destroyed once the entry
// is appended, so the entry must carry everything HUD / future deploy needs.
USTRUCT(BlueprintType)
struct PALDARKLAB_API FPaldarkRosterEntry
{
	GENERATED_BODY()

	// Primary asset id of the Pal definition (e.g.
	// `PaldarkPalDefinition:Direhound`). Future "deploy from roster" flow
	// will async-load this asset to respawn the Pal in a future match.
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Pal|Roster")
	FPrimaryAssetId PalDefinitionId;

	// Species tag (e.g. `Paldark.Pal.Species.Direhound`). Cheaper than
	// resolving the PalDefinition every frame for HUD reads.
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Pal|Roster")
	FGameplayTag SpeciesTag;

	// Bond level snapshot at tame time. Always starts at 1 for a fresh
	// tame (the act of taming is itself a "Bond Event = Tame" worth of XP);
	// the player can grind it up through subsequent shared raids.
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Pal|Roster")
	int32 BondLevel = 1;

	// Health snapshot at tame time. Carried over so deploying from the
	// roster doesn't reset the Pal's HP to full (you tamed a 12 HP boss,
	// you deploy a 12 HP boss).
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Pal|Roster")
	float HealthAtTame = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Pal|Roster")
	float MaxHealthAtTame = 0.0f;

	// Optional player-given nickname. Empty until the future "Rename" UI
	// flow lands (W47+); HUD falls back to `SpeciesTag` leaf name.
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Pal|Roster")
	FString Nickname;
};

// Fired on both authority and the owning client when the roster contents
// change (new tame, or future remove). HUD widget binds to refresh.
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPaldarkRosterChanged, class UPaldarkPlayerPalRosterComponent* /*Component*/);

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent))
class PALDARKLAB_API UPaldarkPlayerPalRosterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkPlayerPalRosterComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Server-only — invoked by `UPaldarkPalTameComponent::RegisterAndDestroy`
	// when a tame succeeds. Snapshots species / health / bond at the moment
	// of capture and appends to the roster. `RolledProbability` is captured
	// for debug logging only.
	void TamePal(APaldarkPalCharacter* TamedPal, float RolledProbability);

	// W44-45 — Server-only. Detach an entry by index from the active roster
	// and return its snapshot by value. Returns a default-constructed
	// FPaldarkRosterEntry when the index is out of range; callers should
	// check `EntryIndex` first (e.g. via `IsValidEntryIndex`). Used by
	// `UPaldarkStableController::ServerRequestDeposit` to relocate the entry
	// into the deposit storage on the PlayerState.
	FPaldarkRosterEntry RemoveEntryByIndex(int32 EntryIndex);

	// W44-45 — Server-only. Append a previously-deposited entry back into
	// the active roster. Returns the new index, or INDEX_NONE if the active
	// roster is already at capacity (no max cap in W44-45 scope yet — this
	// always succeeds, but the signature stays future-proof for W47 caps).
	int32 AppendEntry(const FPaldarkRosterEntry& Entry);

	// W44-45 — Server-only. Refill HealthAtTame to MaxHealthAtTame on one
	// roster entry. Returns true on success, false if the index is out of
	// range or MaxHealthAtTame is non-positive (no-op). Marketplace
	// controller decrements the player's credits before calling this so the
	// component itself stays unaware of the price formula.
	bool HealEntryToFull(int32 EntryIndex);

	// W44-45 — Bounds-checked helper used by validator paths + controller.
	bool IsValidEntryIndex(int32 EntryIndex) const { return Entries.IsValidIndex(EntryIndex); }

	// W47 — Save game capture/apply. Authority-side; clients route through
	// the replicated `Entries` array via OnRep. `CaptureSnapshot` is `const`
	// and safe to call from non-authority for debug dump paths, but the
	// returned blob is only meaningful when the local replicate is fully
	// up to date. `ApplySnapshot` wipes existing entries first so re-loading
	// a snapshot is idempotent.
	FPaldarkRosterSnapshot CaptureSnapshot() const;
	void ApplySnapshot(const FPaldarkRosterSnapshot& Snapshot);

	// Used by console + future UI to inspect the roster.
	const TArray<FPaldarkRosterEntry>& GetEntries() const { return Entries; }

	// Logs the entire roster to LogPaldarkPal. Called by
	// `Paldark.Tame.DumpRoster` console cmd.
	void DumpToLog() const;

	FOnPaldarkRosterChanged OnRosterChanged;

protected:
	UFUNCTION()
	void OnRep_Entries();

	// COND_OwnerOnly replicated — other players can't see this player's
	// roster (matches Lyra's per-player UI state pattern).
	UPROPERTY(ReplicatedUsing = OnRep_Entries)
	TArray<FPaldarkRosterEntry> Entries;
};
