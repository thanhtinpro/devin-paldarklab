// PALDARK W44-45 — Pal Stable UI controller implementation.

#include "Hub/PaldarkStableController.h"

#include "GameFramework/PlayerController.h"

#include "Hub/PaldarkPalStable.h"
#include "Inventory/PaldarkItemDefinition.h"
#include "Pal/PaldarkPalDepositComponent.h"
#include "Pal/PaldarkPlayerPalRosterComponent.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"
#include "Player/Components/PaldarkPlayerInventoryComponent.h"
#include "Player/PaldarkCharacter.h"
#include "Player/PaldarkPlayerController.h"
#include "Player/PaldarkPlayerState.h"

using namespace PaldarkGameplayTags;

UPaldarkStableController::UPaldarkStableController()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	// Initialise the UI state tag to Closed so console/debug code reading
	// `GetUIStateTag` before any Open call still sees a valid tag.
	UIStateTag = TAG_Paldark_Hub_UI_Closed;
}

void UPaldarkStableController::BeginPlay()
{
	Super::BeginPlay();

	OwnerController = Cast<APaldarkPlayerController>(GetOwner());
	if (!OwnerController)
	{
		UE_LOG(LogPaldarkUI, Warning,
			TEXT("UPaldarkStableController::BeginPlay — outer is not APaldarkPlayerController (got %s). Component disabled."),
			GetOwner() ? *GetOwner()->GetName() : TEXT("<null>"));
	}
}

// -----------------------------------------------------------------------------
// Client-side API
// -----------------------------------------------------------------------------

void UPaldarkStableController::RequestOpenStable(APaldarkPalStable* Kiosk)
{
	if (!Kiosk)
	{
		UE_LOG(LogPaldarkUI, Warning,
			TEXT("UPaldarkStableController::RequestOpenStable — null kiosk, ignoring."));
		return;
	}

	ActiveKiosk = Kiosk;
	UIStateTag  = TAG_Paldark_Hub_UI_Open;

	UE_LOG(LogPaldarkUI, Log,
		TEXT("[Hub.Stable.Open] controller=%s kiosk=%s — broadcast OnStableOpened"),
		*GetName(),
		*Kiosk->GetName());

	OnStableOpened.Broadcast(Kiosk);
}

void UPaldarkStableController::RequestCloseStable()
{
	if (UIStateTag == TAG_Paldark_Hub_UI_Closed)
	{
		return;
	}

	ActiveKiosk = nullptr;
	UIStateTag  = TAG_Paldark_Hub_UI_Closed;

	UE_LOG(LogPaldarkUI, Log,
		TEXT("[Hub.Stable.Close] controller=%s — broadcast OnStableClosed"),
		*GetName());

	OnStableClosed.Broadcast();
}

void UPaldarkStableController::RequestDeposit(int32 RosterIndex)
{
	if (!OwnerController)
	{
		return;
	}
	UE_LOG(LogPaldarkUI, Log, TEXT("[Hub.Stable.Deposit] client request idx=%d -> server."), RosterIndex);
	Server_RequestDeposit(RosterIndex);
}

void UPaldarkStableController::RequestWithdraw(int32 DepositIndex)
{
	if (!OwnerController)
	{
		return;
	}
	UE_LOG(LogPaldarkUI, Log, TEXT("[Hub.Stable.Withdraw] client request idx=%d -> server."), DepositIndex);
	Server_RequestWithdraw(DepositIndex);
}

void UPaldarkStableController::RequestHeal(int32 RosterIndex)
{
	if (!OwnerController)
	{
		return;
	}
	UE_LOG(LogPaldarkUI, Log, TEXT("[Hub.Stable.Heal] client request idx=%d -> server."), RosterIndex);
	Server_RequestHeal(RosterIndex);
}

// -----------------------------------------------------------------------------
// Server RPCs
// -----------------------------------------------------------------------------

bool UPaldarkStableController::Server_RequestDeposit_Validate(int32 RosterIndex)
{
	return RosterIndex >= 0;
}

void UPaldarkStableController::Server_RequestDeposit_Implementation(int32 RosterIndex)
{
	if (bIsTransactionInFlight)
	{
		Client_ReceiveActionResult(
			TAG_Paldark_Hub_Stable_Action_Deposit,
			TAG_Paldark_Hub_Result_Fail_NotAllowed,
			RosterIndex,
			ServerGetCreditsBalance());
		return;
	}
	bIsTransactionInFlight = true;

	UPaldarkPlayerPalRosterComponent* Roster  = ResolveRosterComponent();
	UPaldarkPalDepositComponent*      Deposit = ResolveDepositComponent();

	if (!Roster || !Deposit)
	{
		Client_ReceiveActionResult(
			TAG_Paldark_Hub_Stable_Action_Deposit,
			TAG_Paldark_Hub_Result_Fail_NotAllowed,
			RosterIndex,
			ServerGetCreditsBalance());
		bIsTransactionInFlight = false;
		return;
	}

	if (!Roster->IsValidEntryIndex(RosterIndex))
	{
		Client_ReceiveActionResult(
			TAG_Paldark_Hub_Stable_Action_Deposit,
			TAG_Paldark_Hub_Result_Fail_Empty,
			RosterIndex,
			ServerGetCreditsBalance());
		bIsTransactionInFlight = false;
		return;
	}

	if (Deposit->IsFull())
	{
		Client_ReceiveActionResult(
			TAG_Paldark_Hub_Stable_Action_Deposit,
			TAG_Paldark_Hub_Result_Fail_Full,
			RosterIndex,
			ServerGetCreditsBalance());
		bIsTransactionInFlight = false;
		return;
	}

	FPaldarkRosterEntry Removed = Roster->RemoveEntryByIndex(RosterIndex);
	const int32 NewDepositIndex = Deposit->DepositEntry(Removed);

	const FGameplayTag Result = (NewDepositIndex != INDEX_NONE)
		? TAG_Paldark_Hub_Result_Success
		: TAG_Paldark_Hub_Result_Fail_Full;

	// If the deposit failed (race / inconsistent state), put the entry
	// back so the player doesn't lose their Pal.
	if (NewDepositIndex == INDEX_NONE)
	{
		Roster->AppendEntry(Removed);
	}

	Client_ReceiveActionResult(
		TAG_Paldark_Hub_Stable_Action_Deposit,
		Result,
		NewDepositIndex,
		ServerGetCreditsBalance());

	bIsTransactionInFlight = false;
}

bool UPaldarkStableController::Server_RequestWithdraw_Validate(int32 DepositIndex)
{
	return DepositIndex >= 0;
}

void UPaldarkStableController::Server_RequestWithdraw_Implementation(int32 DepositIndex)
{
	if (bIsTransactionInFlight)
	{
		Client_ReceiveActionResult(
			TAG_Paldark_Hub_Stable_Action_Withdraw,
			TAG_Paldark_Hub_Result_Fail_NotAllowed,
			DepositIndex,
			ServerGetCreditsBalance());
		return;
	}
	bIsTransactionInFlight = true;

	UPaldarkPlayerPalRosterComponent* Roster  = ResolveRosterComponent();
	UPaldarkPalDepositComponent*      Deposit = ResolveDepositComponent();

	if (!Roster || !Deposit)
	{
		Client_ReceiveActionResult(
			TAG_Paldark_Hub_Stable_Action_Withdraw,
			TAG_Paldark_Hub_Result_Fail_NotAllowed,
			DepositIndex,
			ServerGetCreditsBalance());
		bIsTransactionInFlight = false;
		return;
	}

	if (!Deposit->IsValidDepositIndex(DepositIndex))
	{
		Client_ReceiveActionResult(
			TAG_Paldark_Hub_Stable_Action_Withdraw,
			TAG_Paldark_Hub_Result_Fail_Empty,
			DepositIndex,
			ServerGetCreditsBalance());
		bIsTransactionInFlight = false;
		return;
	}

	FPaldarkRosterEntry Removed = Deposit->WithdrawEntry(DepositIndex);
	const int32 NewRosterIndex = Roster->AppendEntry(Removed);

	const FGameplayTag Result = (NewRosterIndex != INDEX_NONE)
		? TAG_Paldark_Hub_Result_Success
		: TAG_Paldark_Hub_Result_Fail_Full;

	if (NewRosterIndex == INDEX_NONE)
	{
		// Active roster rejected (capacity not enforced in W44-45 but
		// guard for W47 cap). Put the entry back in deposit.
		Deposit->DepositEntry(Removed);
	}

	Client_ReceiveActionResult(
		TAG_Paldark_Hub_Stable_Action_Withdraw,
		Result,
		NewRosterIndex,
		ServerGetCreditsBalance());

	bIsTransactionInFlight = false;
}

bool UPaldarkStableController::Server_RequestHeal_Validate(int32 RosterIndex)
{
	return RosterIndex >= 0;
}

void UPaldarkStableController::Server_RequestHeal_Implementation(int32 RosterIndex)
{
	if (bIsTransactionInFlight)
	{
		Client_ReceiveActionResult(
			TAG_Paldark_Hub_Stable_Action_Heal,
			TAG_Paldark_Hub_Result_Fail_NotAllowed,
			RosterIndex,
			ServerGetCreditsBalance());
		return;
	}
	bIsTransactionInFlight = true;

	UPaldarkPlayerPalRosterComponent* Roster = ResolveRosterComponent();

	if (!Roster || !Roster->IsValidEntryIndex(RosterIndex))
	{
		Client_ReceiveActionResult(
			TAG_Paldark_Hub_Stable_Action_Heal,
			TAG_Paldark_Hub_Result_Fail_Empty,
			RosterIndex,
			ServerGetCreditsBalance());
		bIsTransactionInFlight = false;
		return;
	}

	const int32 Balance = ServerGetCreditsBalance();
	if (Balance < HealPriceCredits)
	{
		Client_ReceiveActionResult(
			TAG_Paldark_Hub_Stable_Action_Heal,
			TAG_Paldark_Hub_Result_Fail_Insufficient,
			RosterIndex,
			Balance);
		bIsTransactionInFlight = false;
		return;
	}

	if (!ServerDebitCredits(HealPriceCredits))
	{
		Client_ReceiveActionResult(
			TAG_Paldark_Hub_Stable_Action_Heal,
			TAG_Paldark_Hub_Result_Fail_Insufficient,
			RosterIndex,
			ServerGetCreditsBalance());
		bIsTransactionInFlight = false;
		return;
	}

	const bool bHealed = Roster->HealEntryToFull(RosterIndex);
	const FGameplayTag Result = bHealed
		? TAG_Paldark_Hub_Result_Success
		: TAG_Paldark_Hub_Result_Fail_NotAllowed;

	Client_ReceiveActionResult(
		TAG_Paldark_Hub_Stable_Action_Heal,
		Result,
		RosterIndex,
		ServerGetCreditsBalance());

	bIsTransactionInFlight = false;
}

// -----------------------------------------------------------------------------
// Client RPC
// -----------------------------------------------------------------------------

void UPaldarkStableController::Client_ReceiveActionResult_Implementation(
	FGameplayTag ActionTag,
	FGameplayTag ResultTag,
	int32 IndexHint,
	int32 CreditsRemaining)
{
	UE_LOG(LogPaldarkUI, Log,
		TEXT("[Hub.Stable.Result] action=%s result=%s idx=%d credits=%d"),
		*ActionTag.ToString(),
		*ResultTag.ToString(),
		IndexHint,
		CreditsRemaining);

	OnStableActionResult.Broadcast(ActionTag, ResultTag, IndexHint, CreditsRemaining);
}

// -----------------------------------------------------------------------------
// Resolve helpers
// -----------------------------------------------------------------------------

UPaldarkPlayerPalRosterComponent* UPaldarkStableController::ResolveRosterComponent() const
{
	if (!OwnerController)
	{
		return nullptr;
	}
	const APaldarkCharacter* Char = Cast<APaldarkCharacter>(OwnerController->GetPawn());
	return Char ? Char->GetRosterSlot() : nullptr;
}

UPaldarkPalDepositComponent* UPaldarkStableController::ResolveDepositComponent() const
{
	if (!OwnerController)
	{
		return nullptr;
	}
	APaldarkPlayerState* PS = OwnerController->GetPlayerState<APaldarkPlayerState>();
	return PS ? PS->FindComponentByClass<UPaldarkPalDepositComponent>() : nullptr;
}

UPaldarkPlayerInventoryComponent* UPaldarkStableController::ResolveInventoryComponent() const
{
	if (!OwnerController)
	{
		return nullptr;
	}
	const APaldarkCharacter* Char = Cast<APaldarkCharacter>(OwnerController->GetPawn());
	return Char ? Char->GetInventorySlot() : nullptr;
}

int32 UPaldarkStableController::ServerGetCreditsBalance() const
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

bool UPaldarkStableController::ServerDebitCredits(int32 Amount)
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
