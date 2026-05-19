// PALDARK W44-45 — Marketplace UI controller implementation.

#include "Hub/PaldarkMarketplaceController.h"

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

#include "Hub/PaldarkHubBuildingDefinition.h"
#include "Hub/PaldarkMarketplaceKiosk.h"
#include "Inventory/Fragments/PaldarkItemFragment_MarketValue.h"
#include "Inventory/PaldarkItemDefinition.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"
#include "Player/Components/PaldarkPlayerInventoryComponent.h"
#include "Player/PaldarkCharacter.h"
#include "Player/PaldarkPlayerController.h"

using namespace PaldarkGameplayTags;

UPaldarkMarketplaceController::UPaldarkMarketplaceController()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	UIStateTag = TAG_Paldark_Hub_UI_Closed;
}

void UPaldarkMarketplaceController::BeginPlay()
{
	Super::BeginPlay();

	OwnerController = Cast<APaldarkPlayerController>(GetOwner());
	if (!OwnerController)
	{
		UE_LOG(LogPaldarkUI, Warning,
			TEXT("UPaldarkMarketplaceController::BeginPlay — outer is not APaldarkPlayerController (got %s). Component disabled."),
			GetOwner() ? *GetOwner()->GetName() : TEXT("<null>"));
	}
}

// -----------------------------------------------------------------------------
// Client-side API
// -----------------------------------------------------------------------------

void UPaldarkMarketplaceController::RequestOpenMarketplace(APaldarkMarketplaceKiosk* Kiosk)
{
	if (!Kiosk)
	{
		UE_LOG(LogPaldarkUI, Warning,
			TEXT("UPaldarkMarketplaceController::RequestOpenMarketplace — null kiosk, ignoring."));
		return;
	}

	ActiveKiosk = Kiosk;
	UIStateTag  = TAG_Paldark_Hub_UI_OpeningMarket;
	CachedCatalog.Reset();

	const UPaldarkHubBuildingDefinition* Def = Kiosk->BuildingDefinition.Get();
	if (!Def)
	{
		// Try sync load — soft ref may not be loaded yet on a fresh hub
		// entry. W40-41 ships the definition load synchronously from the
		// kiosk's BeginPlay so by the time the player interacts it's
		// almost always loaded; this is a defensive fallback.
		Def = Kiosk->BuildingDefinition.LoadSynchronous();
	}
	if (!Def || Def->MarketplaceCatalog.Num() == 0)
	{
		UE_LOG(LogPaldarkUI, Warning,
			TEXT("[Hub.Market.Open] kiosk=%s — building def missing or catalog empty."),
			*Kiosk->GetName());

		// Still fire OnMarketplaceOpened so the widget mounts with an
		// empty list — designer might want to show an "out of stock"
		// message instead of refusing to open.
		UIStateTag = TAG_Paldark_Hub_UI_Open;
		OnMarketplaceOpened.Broadcast(Kiosk);
		return;
	}

	// Build the async-load request from the catalog soft refs.
	TArray<FSoftObjectPath> SoftPaths;
	SoftPaths.Reserve(Def->MarketplaceCatalog.Num());
	for (const TSoftObjectPtr<UPaldarkItemDefinition>& Soft : Def->MarketplaceCatalog)
	{
		if (!Soft.IsNull())
		{
			SoftPaths.Add(Soft.ToSoftObjectPath());
		}
	}

	if (SoftPaths.Num() == 0)
	{
		UIStateTag = TAG_Paldark_Hub_UI_Open;
		OnMarketplaceOpened.Broadcast(Kiosk);
		return;
	}

	UE_LOG(LogPaldarkUI, Log,
		TEXT("[Hub.Market.Open] kiosk=%s — async-load %d catalog soft refs."),
		*Kiosk->GetName(), SoftPaths.Num());

	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
	CatalogLoadHandle = Streamable.RequestAsyncLoad(
		SoftPaths,
		FStreamableDelegate::CreateUObject(this, &UPaldarkMarketplaceController::OnCatalogLoadCompleted),
		FStreamableManager::DefaultAsyncLoadPriority,
		/*bManageActiveHandle=*/false);
}

void UPaldarkMarketplaceController::OnCatalogLoadCompleted()
{
	if (!ActiveKiosk)
	{
		// Player closed the kiosk before the async load finished — drop
		// the snapshot rather than firing a stale OnMarketplaceOpened.
		CachedCatalog.Reset();
		CatalogLoadHandle.Reset();
		return;
	}

	const UPaldarkHubBuildingDefinition* Def = ActiveKiosk->BuildingDefinition.Get();
	if (Def)
	{
		CachedCatalog.Reset(Def->MarketplaceCatalog.Num());
		for (const TSoftObjectPtr<UPaldarkItemDefinition>& Soft : Def->MarketplaceCatalog)
		{
			if (UPaldarkItemDefinition* Loaded = Soft.Get())
			{
				CachedCatalog.Add(Loaded);
			}
		}
	}

	UIStateTag = TAG_Paldark_Hub_UI_Open;
	OnMarketplaceOpened.Broadcast(ActiveKiosk);

	UE_LOG(LogPaldarkUI, Log,
		TEXT("[Hub.Market.Open] kiosk=%s — catalog ready, items=%d, OnMarketplaceOpened fired."),
		*ActiveKiosk->GetName(), CachedCatalog.Num());

	CatalogLoadHandle.Reset();
}

void UPaldarkMarketplaceController::RequestCloseMarketplace()
{
	if (UIStateTag == TAG_Paldark_Hub_UI_Closed)
	{
		return;
	}
	ActiveKiosk = nullptr;
	CachedCatalog.Reset();
	UIStateTag  = TAG_Paldark_Hub_UI_Closed;

	UE_LOG(LogPaldarkUI, Log, TEXT("[Hub.Market.Close] controller=%s — broadcast OnMarketplaceClosed"), *GetName());
	OnMarketplaceClosed.Broadcast();
}

void UPaldarkMarketplaceController::RequestBuy(FGameplayTag ItemTag, int32 Count)
{
	if (!OwnerController || !ItemTag.IsValid() || Count <= 0)
	{
		return;
	}
	UE_LOG(LogPaldarkUI, Log, TEXT("[Hub.Market.Buy] client request tag=%s count=%d -> server."), *ItemTag.ToString(), Count);
	Server_RequestBuy(ItemTag, Count);
}

void UPaldarkMarketplaceController::RequestSell(FGameplayTag ItemTag, int32 Count)
{
	if (!OwnerController || !ItemTag.IsValid() || Count <= 0)
	{
		return;
	}
	UE_LOG(LogPaldarkUI, Log, TEXT("[Hub.Market.Sell] client request tag=%s count=%d -> server."), *ItemTag.ToString(), Count);
	Server_RequestSell(ItemTag, Count);
}

// -----------------------------------------------------------------------------
// Server RPCs
// -----------------------------------------------------------------------------

bool UPaldarkMarketplaceController::Server_RequestBuy_Validate(FGameplayTag ItemTag, int32 Count)
{
	return ItemTag.IsValid() && Count > 0 && Count <= 999;
}

void UPaldarkMarketplaceController::Server_RequestBuy_Implementation(FGameplayTag ItemTag, int32 Count)
{
	if (bIsTransactionInFlight)
	{
		Client_ReceiveTransactionResult(
			TAG_Paldark_Hub_Market_Action_Buy,
			TAG_Paldark_Hub_Result_Fail_NotAllowed,
			ItemTag, 0,
			ServerGetCreditsBalance());
		return;
	}
	bIsTransactionInFlight = true;

	const UPaldarkItemDefinition* ItemDef = nullptr;
	if (!ServerIsItemInKioskCatalog(ItemTag, ItemDef) || !ItemDef)
	{
		Client_ReceiveTransactionResult(
			TAG_Paldark_Hub_Market_Action_Buy,
			TAG_Paldark_Hub_Result_Fail_NotAllowed,
			ItemTag, 0,
			ServerGetCreditsBalance());
		bIsTransactionInFlight = false;
		return;
	}

	const UPaldarkItemFragment_MarketValue* Price =
		ItemDef->FindFragmentByClass<UPaldarkItemFragment_MarketValue>();
	if (!Price || !Price->bAllowBuy)
	{
		Client_ReceiveTransactionResult(
			TAG_Paldark_Hub_Market_Action_Buy,
			TAG_Paldark_Hub_Result_Fail_NotAllowed,
			ItemTag, 0,
			ServerGetCreditsBalance());
		bIsTransactionInFlight = false;
		return;
	}

	const int32 TotalCost = Price->BuyPriceCredits * Count;
	const int32 Balance   = ServerGetCreditsBalance();
	if (Balance < TotalCost)
	{
		Client_ReceiveTransactionResult(
			TAG_Paldark_Hub_Market_Action_Buy,
			TAG_Paldark_Hub_Result_Fail_Insufficient,
			ItemTag, 0,
			Balance);
		bIsTransactionInFlight = false;
		return;
	}

	UPaldarkPlayerInventoryComponent* Inv = ResolveInventoryComponent();
	if (!Inv)
	{
		Client_ReceiveTransactionResult(
			TAG_Paldark_Hub_Market_Action_Buy,
			TAG_Paldark_Hub_Result_Fail_NotAllowed,
			ItemTag, 0,
			Balance);
		bIsTransactionInFlight = false;
		return;
	}

	// Reserve credits first so AddItem failure refunds cleanly.
	if (!ServerDebitCredits(TotalCost))
	{
		Client_ReceiveTransactionResult(
			TAG_Paldark_Hub_Market_Action_Buy,
			TAG_Paldark_Hub_Result_Fail_Insufficient,
			ItemTag, 0,
			ServerGetCreditsBalance());
		bIsTransactionInFlight = false;
		return;
	}

	// Construct a soft pointer-ish wrapper around the loaded def so we
	// can pass it to AddItem. AddItem accepts a TSoftObjectPtr, we have a
	// resolved const ptr — wrap it back into a soft ptr.
	TSoftObjectPtr<UPaldarkItemDefinition> SoftDef(const_cast<UPaldarkItemDefinition*>(ItemDef));
	const int32 Added = Inv->AddItem(SoftDef, Count);

	if (Added < Count)
	{
		// Inventory full / weight cap — refund the unsettled credits and
		// report partial / fail. We're conservative: refund the full cost
		// when Added < Count (the typical W37-38 inventory either accepts
		// the whole stack or rejects entirely; partial isn't expected but
		// guard for future).
		const int32 RefundCount = Count - Added;
		const int32 Refund      = Price->BuyPriceCredits * RefundCount;
		ServerCreditCredits(Refund);

		const FGameplayTag Result = (Added == 0)
			? TAG_Paldark_Hub_Result_Fail_Full
			: TAG_Paldark_Hub_Result_Success;

		Client_ReceiveTransactionResult(
			TAG_Paldark_Hub_Market_Action_Buy,
			Result,
			ItemTag, Added,
			ServerGetCreditsBalance());
		bIsTransactionInFlight = false;
		return;
	}

	Client_ReceiveTransactionResult(
		TAG_Paldark_Hub_Market_Action_Buy,
		TAG_Paldark_Hub_Result_Success,
		ItemTag, Added,
		ServerGetCreditsBalance());
	bIsTransactionInFlight = false;
}

bool UPaldarkMarketplaceController::Server_RequestSell_Validate(FGameplayTag ItemTag, int32 Count)
{
	return ItemTag.IsValid() && Count > 0 && Count <= 999;
}

void UPaldarkMarketplaceController::Server_RequestSell_Implementation(FGameplayTag ItemTag, int32 Count)
{
	if (bIsTransactionInFlight)
	{
		Client_ReceiveTransactionResult(
			TAG_Paldark_Hub_Market_Action_Sell,
			TAG_Paldark_Hub_Result_Fail_NotAllowed,
			ItemTag, 0,
			ServerGetCreditsBalance());
		return;
	}
	bIsTransactionInFlight = true;

	const UPaldarkItemDefinition* ItemDef = nullptr;
	if (!ServerIsItemInKioskCatalog(ItemTag, ItemDef) || !ItemDef)
	{
		Client_ReceiveTransactionResult(
			TAG_Paldark_Hub_Market_Action_Sell,
			TAG_Paldark_Hub_Result_Fail_NotAllowed,
			ItemTag, 0,
			ServerGetCreditsBalance());
		bIsTransactionInFlight = false;
		return;
	}

	const UPaldarkItemFragment_MarketValue* Price =
		ItemDef->FindFragmentByClass<UPaldarkItemFragment_MarketValue>();
	if (!Price || !Price->bAllowSell)
	{
		Client_ReceiveTransactionResult(
			TAG_Paldark_Hub_Market_Action_Sell,
			TAG_Paldark_Hub_Result_Fail_NotAllowed,
			ItemTag, 0,
			ServerGetCreditsBalance());
		bIsTransactionInFlight = false;
		return;
	}

	UPaldarkPlayerInventoryComponent* Inv = ResolveInventoryComponent();
	if (!Inv)
	{
		Client_ReceiveTransactionResult(
			TAG_Paldark_Hub_Market_Action_Sell,
			TAG_Paldark_Hub_Result_Fail_NotAllowed,
			ItemTag, 0,
			ServerGetCreditsBalance());
		bIsTransactionInFlight = false;
		return;
	}

	const int32 Removed = Inv->RemoveItemByTag(ItemTag, Count);
	if (Removed <= 0)
	{
		Client_ReceiveTransactionResult(
			TAG_Paldark_Hub_Market_Action_Sell,
			TAG_Paldark_Hub_Result_Fail_Insufficient,
			ItemTag, 0,
			ServerGetCreditsBalance());
		bIsTransactionInFlight = false;
		return;
	}

	const int32 TotalGain = Price->SellPriceCredits * Removed;
	ServerCreditCredits(TotalGain);

	const FGameplayTag Result = (Removed == Count)
		? TAG_Paldark_Hub_Result_Success
		: TAG_Paldark_Hub_Result_Success; // partial = still a success; settled count carries the truth.

	Client_ReceiveTransactionResult(
		TAG_Paldark_Hub_Market_Action_Sell,
		Result,
		ItemTag, Removed,
		ServerGetCreditsBalance());
	bIsTransactionInFlight = false;
}

// -----------------------------------------------------------------------------
// Client RPC
// -----------------------------------------------------------------------------

void UPaldarkMarketplaceController::Client_ReceiveTransactionResult_Implementation(
	FGameplayTag ActionTag,
	FGameplayTag ResultTag,
	FGameplayTag ItemTag,
	int32 CountSettled,
	int32 CreditsRemaining)
{
	UE_LOG(LogPaldarkUI, Log,
		TEXT("[Hub.Market.Result] action=%s result=%s item=%s count=%d credits=%d"),
		*ActionTag.ToString(),
		*ResultTag.ToString(),
		*ItemTag.ToString(),
		CountSettled,
		CreditsRemaining);

	OnMarketplaceActionResult.Broadcast(ActionTag, ResultTag, ItemTag, CountSettled, CreditsRemaining);
}

// -----------------------------------------------------------------------------
// Server helpers
// -----------------------------------------------------------------------------

bool UPaldarkMarketplaceController::ServerIsItemInKioskCatalog(FGameplayTag ItemTag, const UPaldarkItemDefinition*& OutDef) const
{
	OutDef = nullptr;
	if (!ActiveKiosk)
	{
		// No open kiosk on the server side. Server doesn't track the
		// client's open/close state, so this branch covers:
		//   (a) console-driven buy/sell (designer test loop) — fall back
		//       to global lookup via the player's currently-overlapped
		//       hub building. For W44-45 we just look up the first
		//       marketplace kiosk in the world.
		//   (b) genuine race (player closed the widget between client
		//       request + server arrival) — same fallback.
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<APaldarkMarketplaceKiosk> It(World); It; ++It)
			{
				APaldarkMarketplaceKiosk* Kiosk = *It;
				if (!Kiosk)
				{
					continue;
				}
				const UPaldarkHubBuildingDefinition* Def = Kiosk->BuildingDefinition.LoadSynchronous();
				if (!Def)
				{
					continue;
				}
				for (const TSoftObjectPtr<UPaldarkItemDefinition>& Soft : Def->MarketplaceCatalog)
				{
					const UPaldarkItemDefinition* Loaded = Soft.LoadSynchronous();
					if (Loaded && Loaded->ItemTag == ItemTag)
					{
						OutDef = Loaded;
						return true;
					}
				}
			}
		}
		return false;
	}

	const UPaldarkHubBuildingDefinition* Def = ActiveKiosk->BuildingDefinition.LoadSynchronous();
	if (!Def)
	{
		return false;
	}

	for (const TSoftObjectPtr<UPaldarkItemDefinition>& Soft : Def->MarketplaceCatalog)
	{
		const UPaldarkItemDefinition* Loaded = Soft.LoadSynchronous();
		if (Loaded && Loaded->ItemTag == ItemTag)
		{
			OutDef = Loaded;
			return true;
		}
	}
	return false;
}

UPaldarkPlayerInventoryComponent* UPaldarkMarketplaceController::ResolveInventoryComponent() const
{
	if (!OwnerController)
	{
		return nullptr;
	}
	const APaldarkCharacter* Char = Cast<APaldarkCharacter>(OwnerController->GetPawn());
	return Char ? Char->GetInventorySlot() : nullptr;
}

int32 UPaldarkMarketplaceController::ServerGetCreditsBalance() const
{
	const UPaldarkPlayerInventoryComponent* Inv = ResolveInventoryComponent();
	if (!Inv)
	{
		return 0;
	}
	int32 Total = 0;
	for (const FPaldarkInventoryEntry& Entry : Inv->GetEntries())
	{
		const UPaldarkItemDefinition* Def = Entry.ItemDef.Get();
		if (Def && Def->ItemTag == TAG_Paldark_Item_Resource_Currency_Credits)
		{
			Total += Entry.StackCount;
		}
	}
	return Total;
}

bool UPaldarkMarketplaceController::ServerCreditCredits(int32 Amount)
{
	if (Amount <= 0)
	{
		return true;
	}
	UPaldarkPlayerInventoryComponent* Inv = ResolveInventoryComponent();
	if (!Inv)
	{
		return false;
	}

	// Find a credits item def in the player's existing inventory to
	// re-use as the soft ptr seed; if the player has no credits yet,
	// fall back to scanning the world for any item def carrying the
	// currency tag (designer-authored DA_Item_Credits asset).
	TSoftObjectPtr<UPaldarkItemDefinition> CurrencyDef;
	for (const FPaldarkInventoryEntry& Entry : Inv->GetEntries())
	{
		const UPaldarkItemDefinition* Def = Entry.ItemDef.Get();
		if (Def && Def->ItemTag == TAG_Paldark_Item_Resource_Currency_Credits)
		{
			CurrencyDef = Entry.ItemDef;
			break;
		}
	}
	if (CurrencyDef.IsNull())
	{
		// Use the inventory component's tag-based helper to find any
		// item def that already lives elsewhere in the player's inventory
		// (or the hub kiosk's catalog when nothing matches).
		if (const UPaldarkItemDefinition* Def = Inv->FindFirstItemDefByTag(TAG_Paldark_Item_Resource_Currency_Credits))
		{
			CurrencyDef = TSoftObjectPtr<UPaldarkItemDefinition>(const_cast<UPaldarkItemDefinition*>(Def));
		}
	}
	if (CurrencyDef.IsNull() && ActiveKiosk)
	{
		const UPaldarkHubBuildingDefinition* HubDef = ActiveKiosk->BuildingDefinition.LoadSynchronous();
		if (HubDef)
		{
			for (const TSoftObjectPtr<UPaldarkItemDefinition>& Soft : HubDef->MarketplaceCatalog)
			{
				const UPaldarkItemDefinition* Def = Soft.LoadSynchronous();
				if (Def && Def->ItemTag == TAG_Paldark_Item_Resource_Currency_Credits)
				{
					CurrencyDef = Soft;
					break;
				}
			}
		}
	}

	if (CurrencyDef.IsNull())
	{
		UE_LOG(LogPaldarkUI, Warning,
			TEXT("[Hub.Market.Credit] could not resolve a Currency.Credits item def to credit %d — designer must include DA_Item_Credits in player inventory or kiosk catalog."),
			Amount);
		return false;
	}

	const int32 Added = Inv->AddItem(CurrencyDef, Amount);
	return Added > 0;
}

bool UPaldarkMarketplaceController::ServerDebitCredits(int32 Amount)
{
	if (Amount <= 0)
	{
		return true;
	}
	UPaldarkPlayerInventoryComponent* Inv = ResolveInventoryComponent();
	if (!Inv)
	{
		return false;
	}

	const int32 Removed = Inv->RemoveItemByTag(
		TAG_Paldark_Item_Resource_Currency_Credits,
		Amount);

	return Removed >= Amount;
}
