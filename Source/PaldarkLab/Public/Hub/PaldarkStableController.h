// PALDARK W44-45 — Pal Stable UI controller.
//
// ActorComponent that lives on every `APaldarkPlayerController`. Owns the
// per-player client/server plumbing the W40-41 `APaldarkPalStable` building
// delegates to in its `OnInteract` — it's the bridge between the UMG widget
// (designer-authored, `UPaldarkStableWidget` subclass) and the authoritative
// state living on:
//
//   * `UPaldarkPlayerPalRosterComponent` (W35-36)        — active roster
//   * `UPaldarkPalDepositComponent`      (W44-45)        — deposit storage
//   * `UPaldarkPlayerInventoryComponent` (W11-12 / W37-38) — credits debit
//
// Public flow (autonomous client):
//   1. PalStable::OnInteract  → controller->RequestOpenStable(this).
//   2. UI state tag flips to `Paldark.Hub.UI.OpeningStable` → `Open`.
//   3. UMG widget binds `OnStableOpened` and reads roster + deposit via the
//      existing replicated components (no separate snapshot RPC needed —
//      both arrays are already `COND_OwnerOnly`).
//   4. Player clicks "Deposit" / "Withdraw" / "Heal" → controller wraps the
//      action into a `Server_RequestX` RPC.
//   5. Server validates, mutates the components, replies via
//      `Client_ReceiveActionResult` with a tagged outcome + index hint +
//      remaining credits.
//   6. Widget receives `OnStableActionResult` delegate, refreshes its
//      row state, toasts the error (if any).
//
// Design notes:
//   - All blueprint-facing delegates are `DECLARE_DYNAMIC_MULTICAST_DELEGATE_*`
//     so UMG can wire them without C++.
//   - Heal price is a UPROPERTY on the controller (designer can tune from a
//     Blueprint subclass) — keeping it on the controller avoids a one-off
//     UDeveloperSettings page.
//   - `bIsTransactionInFlight` guards against rapid-fire double-clicks (W44-45
//     does not yet ship a UMG cooldown spinner — designer wires that in W45+
//     polish).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "PaldarkStableController.generated.h"

class APaldarkPalStable;
class APaldarkPlayerController;
class UPaldarkPalDepositComponent;
class UPaldarkPlayerInventoryComponent;
class UPaldarkPlayerPalRosterComponent;
class UPaldarkStableWidget;

// 1-arg multicast — autonomous-client widget binds to refresh on every
// transaction reply. ActionTag = Paldark.Hub.Stable.Action.*,
// ResultTag = Paldark.Hub.Result.*, IndexHint = the roster/deposit slot
// the request targeted (-1 if not slot-scoped), CreditsRemaining = the
// player's credits stack after the transaction settled (server-authoritative).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FPaldarkStableActionResult,
	FGameplayTag, ActionTag,
	FGameplayTag, ResultTag,
	int32, IndexHint,
	int32, CreditsRemaining);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FPaldarkStableOpened, AActor*, KioskActor);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPaldarkStableClosed);

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent))
class PALDARKLAB_API UPaldarkStableController : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkStableController();

	// --- Client API --------------------------------------------------------

	// Called by `APaldarkPalStable::OnInteract` on the autonomous client (or
	// listen-server host). Flips the UI state tag to `Paldark.Hub.UI.Open`
	// and fires `OnStableOpened` so the widget mounts. Caches the kiosk
	// so subsequent action RPCs can carry a reference back to the building
	// in future (W47 multi-building hub scope).
	UFUNCTION(BlueprintCallable, Category = "Paldark|Hub|Stable")
	void RequestOpenStable(APaldarkPalStable* Kiosk);

	// Called by widget on close button / Escape. Restores the UI state tag
	// to `Paldark.Hub.UI.Closed` and fires `OnStableClosed`. Caller may
	// re-open immediately afterwards.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Hub|Stable")
	void RequestCloseStable();

	// Move a Pal from active roster → deposit storage. RosterIndex is the
	// index into `UPaldarkPlayerPalRosterComponent::GetEntries()`. Client
	// wrapper that fires `Server_RequestDeposit`; result arrives via
	// `OnStableActionResult` with ActionTag = Stable.Action.Deposit.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Hub|Stable")
	void RequestDeposit(int32 RosterIndex);

	// Move a Pal from deposit storage → active roster. DepositIndex is the
	// index into `UPaldarkPalDepositComponent::GetDepositedEntries()`.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Hub|Stable")
	void RequestWithdraw(int32 DepositIndex);

	// Refill HealthAtTame to MaxHealthAtTame on the active roster entry at
	// RosterIndex. Server validates the player can pay `HealPriceCredits`.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Hub|Stable")
	void RequestHeal(int32 RosterIndex);

	// --- Read-only state ---------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Paldark|Hub|Stable")
	FGameplayTag GetUIStateTag() const { return UIStateTag; }

	UFUNCTION(BlueprintPure, Category = "Paldark|Hub|Stable")
	int32 GetHealPriceCredits() const { return HealPriceCredits; }

	// Always non-null. Cached on BeginPlay from the outer actor.
	UFUNCTION(BlueprintPure, Category = "Paldark|Hub|Stable")
	APaldarkPlayerController* GetOwningPaldarkController() const { return OwnerController; }

	// --- Delegates ---------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "Paldark|Hub|Stable")
	FPaldarkStableOpened OnStableOpened;

	UPROPERTY(BlueprintAssignable, Category = "Paldark|Hub|Stable")
	FPaldarkStableClosed OnStableClosed;

	UPROPERTY(BlueprintAssignable, Category = "Paldark|Hub|Stable")
	FPaldarkStableActionResult OnStableActionResult;

	// --- Designer knobs ----------------------------------------------------

	// Flat credit cost for a `RequestHeal` transaction. Designer-tunable in
	// Blueprint subclasses; default 50 covers the W44-45 designer test loop.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Hub|Stable",
		meta = (ClampMin = "0", UIMin = "0"))
	int32 HealPriceCredits = 50;

protected:
	virtual void BeginPlay() override;

	// --- Server RPCs -------------------------------------------------------

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestDeposit(int32 RosterIndex);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestWithdraw(int32 DepositIndex);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestHeal(int32 RosterIndex);

	// --- Client RPC --------------------------------------------------------

	UFUNCTION(Client, Reliable)
	void Client_ReceiveActionResult(
		FGameplayTag ActionTag,
		FGameplayTag ResultTag,
		int32 IndexHint,
		int32 CreditsRemaining);

	// --- Resolve helpers (server + client) ---------------------------------

	UPaldarkPlayerPalRosterComponent* ResolveRosterComponent() const;
	UPaldarkPalDepositComponent*      ResolveDepositComponent() const;
	UPaldarkPlayerInventoryComponent* ResolveInventoryComponent() const;

private:
	// Cached on BeginPlay so server RPCs don't keep re-casting.
	UPROPERTY(Transient)
	TObjectPtr<APaldarkPlayerController> OwnerController;

	// Client-side: cached kiosk reference for the "currently open" stable.
	// Nulled on `RequestCloseStable`. Server doesn't track this — the
	// transaction RPCs are kiosk-agnostic for now (multi-stable hub
	// support is W47+).
	UPROPERTY(Transient)
	TObjectPtr<APaldarkPalStable> ActiveKiosk;

	// Client-side UI state. Flipped by RequestOpenStable / RequestCloseStable
	// and read by the UMG widget via `GetUIStateTag`. Never replicated —
	// each client has its own UI state.
	FGameplayTag UIStateTag;

	// Server-side flag — guards against a malicious client spamming
	// Server_RequestX while the previous one is still processing. Reset
	// once `Client_ReceiveActionResult` completes on the server side
	// (i.e. immediately after the RPC reply is dispatched).
	bool bIsTransactionInFlight = false;

	// Internal helper: server-side count of `Paldark.Item.Resource.Currency.Credits`
	// in the player's inventory. Walks all entries because credits may live
	// in nested backpack containers (W37-38 composite). Returns 0 if the
	// inventory component is null.
	int32 ServerGetCreditsBalance() const;

	// Internal helper: debit `Amount` credits from the player's inventory.
	// Returns true on success, false if the inventory short-changes (caller
	// should re-check `ServerGetCreditsBalance` first to avoid this).
	bool ServerDebitCredits(int32 Amount);
};
