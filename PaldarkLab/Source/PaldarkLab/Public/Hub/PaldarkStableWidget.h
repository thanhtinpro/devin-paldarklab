// PALDARK W44-45 — Pal Stable UMG widget base class.
//
// Thin C++ stub that designer-authored `WBP_PaldarkStableWidget` subclasses
// inherit from. Designer authors the visual layout (List Views for active
// roster + deposit + Heal button) in UMG; the C++ side exposes the
// controller reference + two `BlueprintImplementableEvent` hooks the
// designer's Blueprint graph binds to:
//
//   * `K2_OnStableOpened(KioskActor)` — controller fires when the building
//     mounts; designer pushes the widget on the local HUD and refreshes
//     the list view rows from the (already replicated) roster + deposit
//     components.
//
//   * `K2_OnStableActionResult(ActionTag, ResultTag, IndexHint,
//     CreditsRemaining)` — controller fires after a server transaction
//     reply; designer pops a toast and re-enables the action buttons.
//
// Why a C++ stub instead of pure UMG: the controller is a C++ ActorComponent
// on the local PlayerController, and we want a single canonical "find the
// controller and forward to UMG" call site so designer code can't get the
// resolve wrong. Lyra uses the same pattern (CommonUI widget subclass with a
// tiny C++ ResolveLocalPlayerXyzController helper).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "PaldarkStableWidget.generated.h"

class APaldarkPalStable;
class UPaldarkStableController;

UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisplayName = "Paldark Stable Widget"))
class PALDARKLAB_API UPaldarkStableWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Returns the local autonomous PlayerController's stable controller (or
	// null if the widget mounted before the local PC was set up — rare,
	// but designer code should still defensive-check).
	UFUNCTION(BlueprintPure, Category = "Paldark|Hub|Stable")
	UPaldarkStableController* GetStableController() const;

	// Convenience close — calls `Controller->RequestCloseStable()` if the
	// controller is resolvable, then `RemoveFromParent` on this widget.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Hub|Stable")
	void CloseAndDestroy();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	// Designer hook — fires when the controller broadcasts OnStableOpened.
	UFUNCTION(BlueprintImplementableEvent, Category = "Paldark|Hub|Stable",
		meta = (DisplayName = "On Stable Opened"))
	void K2_OnStableOpened(AActor* KioskActor);

	// Designer hook — fires when the controller broadcasts OnStableClosed.
	UFUNCTION(BlueprintImplementableEvent, Category = "Paldark|Hub|Stable",
		meta = (DisplayName = "On Stable Closed"))
	void K2_OnStableClosed();

	// Designer hook — fires when the controller receives a transaction
	// reply from the server. Designer code reads the tags to update list
	// rows + show toast.
	UFUNCTION(BlueprintImplementableEvent, Category = "Paldark|Hub|Stable",
		meta = (DisplayName = "On Stable Action Result"))
	void K2_OnStableActionResult(
		FGameplayTag ActionTag,
		FGameplayTag ResultTag,
		int32 IndexHint,
		int32 CreditsRemaining);

private:
	// Bound in NativeOnInitialized, unbound in NativeDestruct. Designer
	// code does NOT need to call AddDynamic itself — the C++ stub wires
	// the bridge so every `WBP_PaldarkStableWidget` subclass picks up the
	// hooks for free.
	UFUNCTION()
	void HandleStableOpened(AActor* KioskActor);

	UFUNCTION()
	void HandleStableClosed();

	UFUNCTION()
	void HandleStableActionResult(
		FGameplayTag ActionTag,
		FGameplayTag ResultTag,
		int32 IndexHint,
		int32 CreditsRemaining);

	// Cached controller pointer. Resolved lazily in `GetStableController`;
	// invalidated in `NativeDestruct` to break any lingering delegate edge.
	UPROPERTY(Transient)
	mutable TWeakObjectPtr<UPaldarkStableController> CachedController;
};
