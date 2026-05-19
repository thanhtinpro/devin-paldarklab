// PALDARK W48 — Save slot picker UMG stub.
//
// `UUserWidget` C++ base. Designer authors `WBP_PaldarkSaveSlotPicker` as
// a Blueprint subclass and binds the `BlueprintImplementableEvent` hooks
// to draw the slot list, the save / load / clear buttons, and the
// status toast.
//
// Why a thin C++ base rather than a pure-BP widget:
//
//   * The widget needs to call into `UPaldarkSaveSubsystem` for
//     `RequestSaveSlot` / `RequestLoadSlot` / `RequestDumpSlot` /
//     `ClearSlot` / `ListSlotsToLog`. Wrapping those in
//     `BlueprintCallable` helpers on the C++ base lets designer Blueprints
//     stay focused on layout, not plumbing.
//
//   * The widget needs to listen for `OnSaveCompleted` / `OnLoadCompleted`.
//     The C++ base binds them in `NativeOnInitialized` and unbinds in
//     `NativeDestruct`, forwarding to `K2_OnSaveCompleted` /
//     `K2_OnLoadCompleted` so the designer doesn't have to remember the
//     bind / unbind lifecycle.
//
//   * Matches the W44-45 `UPaldarkStableWidget` / `UPaldarkMarketplaceWidget`
//     and W46 `UPaldarkBriefingWidget` pattern so designers learn one
//     shape across the hub UI.
//
// What this is NOT (deferred to designer):
//
//   * Actual UMG layout — the BP subclass owns slot row widgets, list view,
//     status label, buttons. No native UI is authored here.
//
//   * Slot enumeration — `ListSlotsToLog` just logs; the designer calls it
//     for a console-style readout. A future iteration can expose a
//     structured `TArray<FName> ListSlots()` for binding to a list view.
//     Out of scope for W48 because the live `UGameplayStatics::DoesSaveGameExist`
//     pass-through is enough for the smoke test loop.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PaldarkSaveSlotPickerWidget.generated.h"

class APaldarkPlayerController;
class UPaldarkSaveSubsystem;

UCLASS(Abstract, Blueprintable)
class PALDARKLAB_API UPaldarkSaveSlotPickerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPaldarkSaveSlotPickerWidget(const FObjectInitializer& ObjectInitializer);

	// --- BP-visible accessors ----------------------------------------------

	// The save subsystem on the owning game instance. May be null very
	// early in startup; designer-authored BP must null-check before
	// chaining further calls.
	UFUNCTION(BlueprintPure, Category = "Paldark|Save|Picker")
	UPaldarkSaveSubsystem* GetSaveSubsystem() const;

	// The owning local player controller, downcast to APaldarkPlayerController.
	// Returns null on spectator / replay / dedicated server.
	UFUNCTION(BlueprintPure, Category = "Paldark|Save|Picker")
	APaldarkPlayerController* GetOwningPaldarkPC() const;

	// --- BP-callable wrappers ----------------------------------------------

	// Wraps `UPaldarkSaveSubsystem::RequestSaveSlot(PC, SlotName, Trigger.Manual)`.
	// Returns true if the async write was issued (false synchronously means
	// the subsystem rejected the request — see log).
	UFUNCTION(BlueprintCallable, Category = "Paldark|Save|Picker")
	bool RequestSave(FName SlotName);

	// Wraps `UPaldarkSaveSubsystem::RequestLoadSlot`. Returns true if the
	// async read was issued. Schema mismatch / IO error are surfaced via
	// the `K2_OnLoadCompleted` hook, not the synchronous return.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Save|Picker")
	bool RequestLoad(FName SlotName);

	// Wraps `UPaldarkSaveSubsystem::RequestDumpSlot`. Sync-loads + logs
	// the metadata. Useful for "Preview" button in the BP.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Save|Picker")
	bool RequestDump(FName SlotName);

	// Wraps `UPaldarkSaveSubsystem::ClearSlot` (DeleteGameInSlot).
	UFUNCTION(BlueprintCallable, Category = "Paldark|Save|Picker")
	bool RequestClear(FName SlotName);

	// Wraps `UPaldarkSaveSubsystem::ListSlotsToLog`. Logs the slot table
	// and returns the count.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Save|Picker")
	int32 RequestList();

	// --- BP hooks (designer authors these) ---------------------------------

	// Called after `OnSaveCompleted` fires (async write returned). The
	// designer uses this to flash a toast / play a SFX / refresh the
	// slot list. `ResultTag` is one of the `Paldark.Save.Result.*`
	// hierarchy; `TriggerTag` echoes back what `RequestSave` (or
	// auto-save) passed in so the BP can branch on "save kind".
	UFUNCTION(BlueprintImplementableEvent, Category = "Paldark|Save|Picker",
		meta = (DisplayName = "On Save Completed"))
	void K2_OnSaveCompleted(
		FName SlotName,
		FGameplayTag ResultTag,
		FGameplayTag TriggerTag);

	// Called after `OnLoadCompleted` fires (async read returned + snapshot
	// applied). Designer uses this to refresh the HUD (hot bar, mini-map
	// roster, etc.) once the live PC reflects the loaded state.
	UFUNCTION(BlueprintImplementableEvent, Category = "Paldark|Save|Picker",
		meta = (DisplayName = "On Load Completed"))
	void K2_OnLoadCompleted(
		FName SlotName,
		FGameplayTag ResultTag);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void HandleSaveCompleted(
		FName SlotName,
		FGameplayTag ResultTag,
		FGameplayTag TriggerTag);

	UFUNCTION()
	void HandleLoadCompleted(
		FName SlotName,
		FGameplayTag ResultTag);

private:
	// Bound in NativeOnInitialized; unbound in NativeDestruct.
	UPROPERTY(Transient)
	TObjectPtr<UPaldarkSaveSubsystem> BoundSubsystem;
};
