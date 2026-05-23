// PALDARK W44-45 — Pal deposit storage (per-player, PlayerState-scoped).
//
// Lives on `APaldarkPlayerState` so the deposit list survives pawn
// death + respawn (the active roster on `UPaldarkPlayerPalRosterComponent`
// lives on the Character — same as Lyra's per-actor inventory split). Mirrors
// the roster component's shape:
//
//   - TArray<FPaldarkRosterEntry> DepositedEntries — same struct as the
//     active roster, but the list semantics differ: deposited Pals are
//     "out of the active loadout" and don't participate in match combat /
//     deploy flows until the player withdraws them through the stable
//     building.
//
//   - MaxDepositSlots — designer-tunable cap so the deposit list can't
//     grow unbounded. Default 24 (4x the W45 active roster of 6 entries).
//
// Replication: COND_OwnerOnly so other players can't observe each other's
// deposits (matches the active roster's privacy gate).
//
// What this is NOT (deferred):
//   - No save-game persistence — DepositedEntries resets to empty on every
//     match boundary in W44-45 scope. W47 wires the actual cross-match
//     persistence via `USaveGame` + the existing `MigrationReplicationKey`
//     pattern from W37-38 inventory.
//   - No designer-driven "deposit cap by hub building tier" — every player
//     gets a flat MaxDepositSlots. W47 hub progression may tie this to a
//     stable building level (Pal Stable T1 = 16, T2 = 32, T3 = 64).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Pal/PaldarkPlayerPalRosterComponent.h"
#include "PaldarkPalDepositComponent.generated.h"

// Fired on both authority and the owning client when the deposit list
// changes (new deposit or withdrawal). UMG stable widget binds to refresh
// the deposited-tab list.
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPaldarkDepositChanged, class UPaldarkPalDepositComponent* /*Component*/);

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent))
class PALDARKLAB_API UPaldarkPalDepositComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkPalDepositComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Server-only. Append a Pal entry to the deposit list. Returns the new
	// index on success, INDEX_NONE if the list is already at MaxDepositSlots
	// or the caller is not authority.
	int32 DepositEntry(const FPaldarkRosterEntry& Entry);

	// Server-only. Detach a deposited entry by index and return it by value
	// for re-injection into the active roster. Returns a default-constructed
	// FPaldarkRosterEntry when the index is out of range (caller should
	// `IsValidDepositIndex` first).
	FPaldarkRosterEntry WithdrawEntry(int32 DepositIndex);

	bool IsValidDepositIndex(int32 DepositIndex) const { return DepositedEntries.IsValidIndex(DepositIndex); }
	bool IsFull() const { return DepositedEntries.Num() >= MaxDepositSlots; }
	int32 GetMaxDepositSlots() const { return MaxDepositSlots; }

	// W47 — Save game capture/apply. Authority-side. Returns the deposit
	// list as a `FPaldarkRosterSnapshot` (same shape as the active roster).
	// `ApplySnapshot` clamps to `MaxDepositSlots`; entries past the cap are
	// dropped with a warning.
	FPaldarkRosterSnapshot CaptureSnapshot() const;
	void ApplySnapshot(const FPaldarkRosterSnapshot& Snapshot);

	const TArray<FPaldarkRosterEntry>& GetDepositedEntries() const { return DepositedEntries; }

	// Logs the entire deposit list to LogPaldarkPal. Called by
	// `Paldark.Hub.Stable.List` console cmd.
	void DumpToLog() const;

	FOnPaldarkDepositChanged OnDepositChanged;

protected:
	UFUNCTION()
	void OnRep_DepositedEntries();

	// Designer-tunable cap. Default 24 covers the W44-45 designer test loop
	// (1 player taming 8 unique species + spare). W47 may tier this off
	// stable building level.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Hub|Stable",
		meta = (ClampMin = "0", UIMin = "0", UIMax = "128"))
	int32 MaxDepositSlots = 24;

	// COND_OwnerOnly replicated — deposit list is per-player UI state,
	// matches `UPaldarkPlayerPalRosterComponent::Entries` privacy gate.
	UPROPERTY(ReplicatedUsing = OnRep_DepositedEntries)
	TArray<FPaldarkRosterEntry> DepositedEntries;
};
