// PALDARK W47 — Save game subsystem (player progression orchestration).
//
// `UGameInstanceSubsystem` so it survives `UWorld` teardowns (e.g.
// ServerTravel between hub and raid maps) and lives for the entire game
// process.
//
// W48 polish adds auto-save hooks + a smoke-save entry point:
//
//   * `bAutoSaveOnExtract` (default true) — subsystem listens to the
//     world's `UPaldarkMatchSubsystem::OnPlayerOutcomeChanged` (re-bound
//     on every map load via `FCoreUObjectDelegates::PostLoadMapWithWorld`)
//     and calls `RequestSaveSlot(..., Trigger.HubReturn)` automatically
//     when a player transitions to `Extracted`.
//
//   * `bAutoSaveOnLogout` (default true) — subsystem listens to the
//     `UPaldarkNetSubsystem::OnPlayerLogout` dynamic delegate (bound
//     once on first map-load when both subsystems exist) and calls
//     `RequestSaveSlot(..., Trigger.Logout)` before the player
//     controller's destruction completes.
//
//   * `RequestHubToRaidHandoffSmoke(PC, SlotName)` — manual smoke-test
//     entry point that captures the live state under `Paldark.Save.
//     Trigger.Travel` so QA can simulate the hub→raid handoff (save,
//     ServerTravel, load on the new map, verify roster/inventory
//     preserved) via the new `Paldark.QA.HubToRaidHandoff` console
//     command without touching real travel plumbing.
//
// Owns:
//
//   1. The capture pipeline — `BuildSnapshotForPC(PC)` walks the live
//      components (`UPaldarkPlayerPalRosterComponent` on the character,
//      `UPaldarkPalDepositComponent` on the PlayerState,
//      `UPaldarkPlayerInventoryComponent` on the character) and calls
//      their `CaptureSnapshot()` helpers to produce a
//      `UPaldarkPlayerSaveGame*`.
//
//   2. The async write pipeline — `RequestSaveSlot(PC, SlotName, Trigger)`
//      wraps the engine's `UGameplayStatics::AsyncSaveGameToSlot` and
//      surfaces completion through `OnSaveCompleted` plus a
//      `Paldark.Save.Result.*` tag.
//
//   3. The async read pipeline — `RequestLoadSlot(PC, SlotName)` wraps
//      `UGameplayStatics::AsyncLoadGameFromSlot` and, on completion,
//      validates the schema version then routes through
//      `ApplySnapshotToPC(PC, SaveGame)` which walks the same component
//      chain in reverse via `ApplySnapshot()`.
//
//   4. Slot mgmt — `ClearSlot`, `DoesSlotExist`, `ListSlotsToLog`.
//
//   5. Static helpers — `GetDefaultSlotName(PC)` keys on the PlayerController's
//      unique net id (or "DefaultPlayer" on standalone / PIE-1) so multiple
//      PIE instances don't stomp each other's saves.
//
// Threading: AsyncSaveGameToSlot uses TaskGraph threads internally; the
// delegate fires on the game thread when the write completes. The subsystem
// owns no own background threads.
//
// Authority: all snapshot capture / apply work happens on the local machine
// against the LOCAL player's PC. On a dedicated server (where there is no
// local player) the subsystem refuses Save/Load with
// `Paldark.Save.Result.Fail.NotAllowed`. This is fine for W47 scope —
// per-player save lands listen-host + standalone first; dedicated-server
// save (via per-player .sav files on the server's Saved dir) ships in
// W48 polish.

#pragma once

#include "CoreMinimal.h"
#include "Match/PaldarkMatchTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "PaldarkSaveSubsystem.generated.h"

class AController;
class APaldarkPlayerController;
class APlayerState;
class UPaldarkMatchSubsystem;
class UPaldarkNetSubsystem;
class UPaldarkPlayerSaveGame;
class UWorld;

// Fired on the game thread after `AsyncSaveGameToSlot` completes. The result
// tag is one of the `Paldark.Save.Result.*` set so UMG / HUD can branch on
// success vs. failure without re-implementing string matching.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPaldarkSaveCompleted,
	FName, SlotName,
	FGameplayTag, ResultTag,
	FGameplayTag, TriggerTag);

// Fired on the game thread after `AsyncLoadGameFromSlot` completes and the
// snapshot has been applied to the live components. ResultTag is
// `Paldark.Save.Result.Success` on the happy path; otherwise one of the
// `Fail.*` variants.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPaldarkLoadCompleted,
	FName, SlotName,
	FGameplayTag, ResultTag);

UCLASS()
class PALDARKLAB_API UPaldarkSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPaldarkSaveSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ---- Async APIs --------------------------------------------------------

	// Captures the current state of `PC`'s live components, then async-writes
	// the resulting `UPaldarkPlayerSaveGame` to `<SlotName>.sav`. Returns
	// false synchronously if validation fails (PC is null, PC is a remote
	// proxy with no local components, not authority + not listen-host).
	// `TriggerTag` should be one of `Paldark.Save.Trigger.{HubReturn,
	// Logout, Manual}` — used by the dump command + future telemetry.
	bool RequestSaveSlot(APaldarkPlayerController* PC,
		FName SlotName,
		FGameplayTag TriggerTag);

	// Async-loads `<SlotName>.sav` and applies the snapshot to `PC`'s live
	// components on completion. Returns false synchronously if PC is null
	// or the slot doesn't exist. Schema mismatch / IO error are surfaced
	// via the delegate, not the synchronous return.
	bool RequestLoadSlot(APaldarkPlayerController* PC,
		FName SlotName);

	// Sync-loads `<SlotName>.sav` and logs the metadata + roster / inventory
	// row counts. Does NOT apply state to live components. Useful for
	// designers verifying a save without mutating their session.
	bool RequestDumpSlot(FName SlotName);

	// Deletes `<SlotName>.sav` via `UGameplayStatics::DeleteGameInSlot`.
	// Returns true on success.
	bool ClearSlot(FName SlotName);

	// `UGameplayStatics::DoesSaveGameExist` pass-through.
	bool DoesSlotExist(FName SlotName) const;

	// Logs all `*.sav` files in `<project>/Saved/SaveGames/` (mtime + size)
	// via `LogPaldarkLab`. Backs the `Paldark.Save.ListSlots` console cmd.
	int32 ListSlotsToLog() const;

	// Authority-only nuclear option — deletes every `.sav` file in the save
	// dir. Returns the count of files deleted. Designer / QA only.
	int32 WipeAllSlots();

	// ---- W48 polish: smoke-save -------------------------------------------

	// Captures the current state for `PC` and writes it under `SlotName`
	// (or `GetDefaultSlotName(PC)` if NAME_None) with `Paldark.Save.
	// Trigger.Travel`. Mirrors `RequestSaveSlot` but pre-stamps the
	// trigger so QA + designers can grep the resulting log line for the
	// hub→raid handoff smoke check.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Save")
	bool RequestHubToRaidHandoffSmoke(APaldarkPlayerController* PC,
		FName SlotName);

	// ---- W48 polish: auto-save knobs --------------------------------------

	// When true (default), subsystem watches the world's match subsystem
	// and auto-saves the local PC's state on its first transition to
	// `Extracted`. Re-bound on every map load.
	UPROPERTY(EditDefaultsOnly, Config, Category = "Paldark|Save")
	bool bAutoSaveOnExtract = true;

	// When true (default), subsystem watches the net subsystem's
	// `OnPlayerLogout` and auto-saves the corresponding local PC's
	// state before the controller is destroyed.
	UPROPERTY(EditDefaultsOnly, Config, Category = "Paldark|Save")
	bool bAutoSaveOnLogout = true;

	// ---- Delegates ---------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "Paldark|Save")
	FOnPaldarkSaveCompleted OnSaveCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Paldark|Save")
	FOnPaldarkLoadCompleted OnLoadCompleted;

	// ---- Static helpers ----------------------------------------------------

	// Returns a stable slot name for `PC`:
	//   * On standalone / listen-host with a unique net id: derived from
	//     the net id string (`Paldark_<NetId>`).
	//   * On PIE / standalone with no net id yet: `"DefaultPlayer"`.
	//   * On null PC: `"DefaultPlayer"`.
	// Designer can pass an explicit override via the console command's
	// optional arg — this helper just picks the sensible default.
	static FName GetDefaultSlotName(APaldarkPlayerController* PC);

	// Default user index for `UGameplayStatics::AsyncSaveGameToSlot`. W47
	// hardcodes 0 — multi-user splitscreen profile mgmt is a Q1-next
	// concern.
	static constexpr int32 kDefaultUserIndex = 0;

private:
	// Builds an in-memory `UPaldarkPlayerSaveGame` from `PC`'s live state.
	// Called from `RequestSaveSlot`. Outer is `this` (the subsystem) so the
	// engine can stamp `SAVEGAME` archive flags correctly during write.
	UPaldarkPlayerSaveGame* BuildSnapshotForPC(APaldarkPlayerController* PC) const;

	// Walks `SaveGame` and applies its content back to `PC`'s live
	// components. Returns a `Paldark.Save.Result.*` tag — usually
	// `Success`, but can be `Fail.NotAllowed` if PC's character / state
	// chain isn't ready (e.g. mid-respawn).
	FGameplayTag ApplySnapshotToPC(APaldarkPlayerController* PC,
		UPaldarkPlayerSaveGame* SaveGame) const;

	// Engine callback. Forwarded into `OnSaveCompleted` after re-wrapping
	// the bool result into our result tag set.
	UFUNCTION()
	void HandleSaveComplete(const FString& SlotName, const int32 UserIndex, bool bSucceeded);

	// Engine callback. Validates schema version, applies the snapshot, then
	// fires `OnLoadCompleted`.
	UFUNCTION()
	void HandleLoadComplete(const FString& SlotName, const int32 UserIndex, USaveGame* SaveObject);

	// ---- W48 polish: auto-save plumbing -----------------------------------

	// `FCoreUObjectDelegates::PostLoadMapWithWorld` handler. Re-binds the
	// per-world match subsystem outcome delegate + (on first call) the
	// game-instance-scope net subsystem logout delegate.
	void HandlePostLoadMapWithWorld(UWorld* LoadedWorld);

	// Match subsystem hook. Fired for every per-player outcome flip
	// (Alive → Extracted/KIA/Disconnected). Filters for the Extracted
	// transition + local PlayerController, then calls `RequestSaveSlot`
	// with `Paldark.Save.Trigger.HubReturn`.
	void HandlePlayerOutcomeChanged(APlayerState* PlayerState,
		EPaldarkPlayerOutcome NewOutcome);

	// Net subsystem hook. Fired on the server every time a controller
	// disconnects. Filters for an `APaldarkPlayerController` with a
	// reachable component chain and calls `RequestSaveSlot` with
	// `Paldark.Save.Trigger.Logout`.
	UFUNCTION()
	void HandlePlayerLogoutAutoSave(AController* Exiting);

	// Per-in-flight context. We need to remember the PC the load was for
	// because the engine's load callback only passes the slot string back.
	struct FPendingLoad
	{
		FName SlotName;
		TWeakObjectPtr<APaldarkPlayerController> PC;
	};

	// Single in-flight save / load supported at a time. Overlapping requests
	// log a warning and return false; the existing in-flight is left
	// untouched. (Two parallel writes to the same slot file isn't safe
	// even on different threads.)
	TArray<FName> InFlightSaveSlots;
	TArray<FPendingLoad> InFlightLoads;

	// Trigger tag remembered between `RequestSaveSlot` (capture) and
	// `HandleSaveComplete` (broadcast). Keyed on slot name because the
	// engine's save callback only echoes back the slot string.
	TMap<FName, FGameplayTag> PendingSaveTriggers;

	// PostLoadMapWithWorld delegate handle so we can unbind in Deinitialize.
	FDelegateHandle PostLoadMapHandle;

	// Last bound match subsystem (per-world) — weak ref so a world
	// teardown doesn't keep the subsystem alive. Cleared + re-bound by
	// `HandlePostLoadMapWithWorld`.
	TWeakObjectPtr<UPaldarkMatchSubsystem> BoundMatchSubsystem;

	// Match outcome delegate handle so we can unbind cleanly when the
	// match subsystem changes worlds.
	FDelegateHandle MatchOutcomeHandle;

	// Net subsystem logout delegate is GameInstance-scope (lives as long
	// as we do). Bind once on first map-load; this flag guards against
	// double-bind on subsequent map loads.
	bool bBoundNetLogout = false;
};
