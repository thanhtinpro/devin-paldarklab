// PALDARK W44-45 — Marketplace UI controller.
//
// ActorComponent that lives on every `APaldarkPlayerController`. Owns the
// per-player client/server plumbing the W40-41 `APaldarkMarketplaceKiosk`
// delegates to in its `OnInteract`. Mirror of `UPaldarkStableController`
// but for marketplace catalog browsing + buy/sell transactions.
//
// Public flow (autonomous client):
//   1. MarketplaceKiosk::OnInteract → controller->RequestOpenMarketplace(this).
//   2. UI state tag flips to `Paldark.Hub.UI.OpeningMarket`; async-load the
//      kiosk's `MarketplaceCatalog` soft refs.
//   3. Catalog load completes → state tag flips to `Open` and
//      `OnMarketplaceOpened` fires with the resolved `CachedCatalog`.
//   4. Player clicks "Buy" / "Sell" → `Server_RequestBuy` / `Server_RequestSell`.
//   5. Server validates (credits, catalog membership, inventory space),
//      mutates the inventory component, replies via
//      `Client_ReceiveTransactionResult` with a tagged outcome.
//   6. Widget receives `OnMarketplaceActionResult` and refreshes the row.
//
// Pricing source: per-item `UPaldarkItemFragment_MarketValue` (W44-45 new
// fragment) holds BuyPriceCredits / SellPriceCredits. Items in the kiosk
// catalog without the fragment are treated as "not for sale" — the server
// rejects with `Paldark.Hub.Result.Fail.NotAllowed`.
//
// Catalog membership: server re-validates that the requested ItemTag is in
// the kiosk's `MarketplaceCatalog`. This blocks "spoofed Buy" RPCs from a
// modified client trying to purchase an item that isn't actually sold here.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/StreamableManager.h"
#include "GameplayTagContainer.h"
#include "PaldarkMarketplaceController.generated.h"

class APaldarkMarketplaceKiosk;
class APaldarkPlayerController;
class UPaldarkHubBuildingDefinition;
class UPaldarkItemDefinition;
class UPaldarkPlayerInventoryComponent;

// 1-arg multicast for transaction reply. Same shape as the stable controller
// result, plus an `ItemTag` field so the widget can find the row to refresh.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(
	FPaldarkMarketplaceActionResult,
	FGameplayTag, ActionTag,
	FGameplayTag, ResultTag,
	FGameplayTag, ItemTag,
	int32, CountSettled,
	int32, CreditsRemaining);

// Fired client-side when the kiosk catalog finishes async-loading. The
// `CachedCatalog` array is the snapshot of resolved `UPaldarkItemDefinition`
// pointers in catalog order — widget reads it directly via `GetCachedCatalog`.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FPaldarkMarketplaceOpened, AActor*, KioskActor);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPaldarkMarketplaceClosed);

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent))
class PALDARKLAB_API UPaldarkMarketplaceController : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkMarketplaceController();

	// --- Client API --------------------------------------------------------

	// Called by `APaldarkMarketplaceKiosk::OnInteract` on the autonomous
	// client. Flips UI state to `OpeningMarket`, fires off the catalog
	// async-load; on load completion flips to `Open` + fires `OnMarketplaceOpened`.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Hub|Marketplace")
	void RequestOpenMarketplace(APaldarkMarketplaceKiosk* Kiosk);

	UFUNCTION(BlueprintCallable, Category = "Paldark|Hub|Marketplace")
	void RequestCloseMarketplace();

	// Buy `Count` units of the item identified by `ItemTag`. Server resolves
	// the kiosk catalog → item def → MarketValue fragment, debits
	// `BuyPriceCredits * Count`, calls `AddItem`. Result via delegate.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Hub|Marketplace")
	void RequestBuy(FGameplayTag ItemTag, int32 Count);

	// Sell `Count` units of the item identified by `ItemTag`. Server removes
	// the stack, credits the player with `SellPriceCredits * Count`.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Hub|Marketplace")
	void RequestSell(FGameplayTag ItemTag, int32 Count);

	// --- Read-only state ---------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Paldark|Hub|Marketplace")
	FGameplayTag GetUIStateTag() const { return UIStateTag; }

	UFUNCTION(BlueprintPure, Category = "Paldark|Hub|Marketplace")
	APaldarkPlayerController* GetOwningPaldarkController() const { return OwnerController; }

	// Client-side cached catalog snapshot. Empty until `OnMarketplaceOpened`
	// fires. UMG widget reads this directly to build the row list.
	UFUNCTION(BlueprintPure, Category = "Paldark|Hub|Marketplace")
	const TArray<UPaldarkItemDefinition*>& GetCachedCatalog() const { return CachedCatalog; }

	// --- Delegates ---------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "Paldark|Hub|Marketplace")
	FPaldarkMarketplaceOpened OnMarketplaceOpened;

	UPROPERTY(BlueprintAssignable, Category = "Paldark|Hub|Marketplace")
	FPaldarkMarketplaceClosed OnMarketplaceClosed;

	UPROPERTY(BlueprintAssignable, Category = "Paldark|Hub|Marketplace")
	FPaldarkMarketplaceActionResult OnMarketplaceActionResult;

protected:
	virtual void BeginPlay() override;

	// --- Server RPCs -------------------------------------------------------

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestBuy(FGameplayTag ItemTag, int32 Count);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestSell(FGameplayTag ItemTag, int32 Count);

	// --- Client RPC --------------------------------------------------------

	UFUNCTION(Client, Reliable)
	void Client_ReceiveTransactionResult(
		FGameplayTag ActionTag,
		FGameplayTag ResultTag,
		FGameplayTag ItemTag,
		int32 CountSettled,
		int32 CreditsRemaining);

private:
	// Cached on BeginPlay so server RPCs don't re-cast.
	UPROPERTY(Transient)
	TObjectPtr<APaldarkPlayerController> OwnerController;

	// Client-side: kiosk reference for the "currently open" marketplace.
	UPROPERTY(Transient)
	TObjectPtr<APaldarkMarketplaceKiosk> ActiveKiosk;

	// Client-side: resolved catalog after async-load completes. UMG widget
	// reads this directly via `GetCachedCatalog`.
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPaldarkItemDefinition>> CachedCatalog;

	// Async-load handle for the catalog soft refs. Held so the streamable
	// manager doesn't release the assets while the UI is open.
	TSharedPtr<FStreamableHandle> CatalogLoadHandle;

	// Client-side UI state. Same shape as the stable controller — Closed /
	// OpeningMarket / Open. Never replicated.
	FGameplayTag UIStateTag;

	// Server-side flag. Same purpose as `UPaldarkStableController::bIsTransactionInFlight`.
	bool bIsTransactionInFlight = false;

	// Server-side: catalog soft-ref membership check. Returns true if the
	// requested ItemTag corresponds to an item def in the kiosk's catalog.
	// Walks `ActiveKiosk->GetBuildingDefinition()->MarketplaceCatalog` and
	// resolves the soft refs synchronously; W44-45 catalogs are small (< 20
	// items in practice) so this is cheap.
	bool ServerIsItemInKioskCatalog(FGameplayTag ItemTag, const UPaldarkItemDefinition*& OutDef) const;

	// Server-side: credits balance counter.
	int32 ServerGetCreditsBalance() const;
	bool  ServerCreditCredits(int32 Amount);
	bool  ServerDebitCredits(int32 Amount);

	UPaldarkPlayerInventoryComponent* ResolveInventoryComponent() const;

	// Async-load callback. Resolves the catalog soft refs into
	// `CachedCatalog`, flips state to `Open`, fires `OnMarketplaceOpened`.
	void OnCatalogLoadCompleted();
};
