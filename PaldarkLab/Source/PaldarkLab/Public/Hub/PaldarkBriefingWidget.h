// PALDARK W46 — Briefing room UMG widget stub.
//
// `UUserWidget` C++ base. Designer authors `WBP_PaldarkBriefingWidget` as
// a Blueprint subclass and binds the `BlueprintImplementableEvent` hooks
// to refresh the map picker, vote tally bars, ready list, and countdown
// label. The W40-41 `UPaldarkHubBuildingDefinition::WidgetClass` field
// already points at a soft-class reference, so the briefing-room kiosk
// will spawn this widget via the briefing controller once designer
// authors the BP subclass.
//
// Why a thin C++ base rather than a pure-BP widget:
//
//   - The BP can subclass `UUserWidget` directly, but then we lose the
//     `BlueprintPure` accessor for the matching controller (so designers
//     would have to fish it off the player controller manually in BP).
//     The C++ base owns the controller resolve so the BP graph stays
//     about layout, not plumbing.
//   - Matches the W44-45 `UPaldarkStableWidget` / `UPaldarkMarketplaceWidget`
//     pattern so designers learn one shape.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PaldarkBriefingWidget.generated.h"

class APaldarkPlayerController;
class UPaldarkBriefingController;
class UPaldarkBriefingSessionComponent;

UCLASS(Abstract, Blueprintable)
class PALDARKLAB_API UPaldarkBriefingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPaldarkBriefingWidget(const FObjectInitializer& ObjectInitializer);

	// --- BP-visible accessors ----------------------------------------------

	// The briefing controller on the local player controller. May be null
	// (e.g. spectator / replay). Designer-authored BP must null-check.
	UFUNCTION(BlueprintPure, Category = "Paldark|Hub|Briefing")
	UPaldarkBriefingController* GetBriefingController() const;

	// Shortcut to the replicated session component on GameState.
	UFUNCTION(BlueprintPure, Category = "Paldark|Hub|Briefing")
	UPaldarkBriefingSessionComponent* GetSessionComponent() const;

	// --- BP hooks (designer authors these) ---------------------------------

	// Called after RequestOpenBriefing fires the OnBriefingOpened delegate
	// and the C++ base has bound the session-changed delegate. Designer
	// uses this to populate the initial vote tally / map list / countdown
	// label.
	UFUNCTION(BlueprintImplementableEvent, Category = "Paldark|Hub|Briefing",
		meta = (DisplayName = "On Briefing Opened"))
	void K2_OnBriefingOpened(AActor* BriefingRoomActor);

	// Called after RequestCloseBriefing fires OnBriefingClosed. Designer
	// uses this to unmount any per-session UI (e.g. countdown label).
	UFUNCTION(BlueprintImplementableEvent, Category = "Paldark|Hub|Briefing",
		meta = (DisplayName = "On Briefing Closed"))
	void K2_OnBriefingClosed();

	// Called after every Server_RequestX RPC round-trip; passes the result
	// tag back so the designer can show a toast / play a sound on success
	// or failure.
	UFUNCTION(BlueprintImplementableEvent, Category = "Paldark|Hub|Briefing",
		meta = (DisplayName = "On Briefing Action Result"))
	void K2_OnBriefingActionResult(
		FGameplayTag ActionTag,
		FGameplayTag ResultTag,
		FGameplayTag MapTag,
		int32 CountdownRemaining);

	// Called after the replicated session component's OnRep_VoterStates
	// or OnRep_PhaseTag or OnRep_CountdownRemaining fires. Designer uses
	// this to refresh tally bars + ready dots + countdown numeric.
	UFUNCTION(BlueprintImplementableEvent, Category = "Paldark|Hub|Briefing",
		meta = (DisplayName = "On Session Changed"))
	void K2_OnSessionChanged();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void HandleBriefingOpened(AActor* BriefingRoomActor);

	UFUNCTION()
	void HandleBriefingClosed();

	UFUNCTION()
	void HandleBriefingActionResult(
		FGameplayTag ActionTag,
		FGameplayTag ResultTag,
		FGameplayTag MapTag,
		int32 CountdownRemaining);

	UFUNCTION()
	void HandleSessionChanged();

private:
	// Bound on NativeOnInitialized; unbound on NativeDestruct.
	UPROPERTY(Transient)
	TObjectPtr<UPaldarkBriefingController> BoundController;

	UPROPERTY(Transient)
	TObjectPtr<UPaldarkBriefingSessionComponent> BoundSession;
};
