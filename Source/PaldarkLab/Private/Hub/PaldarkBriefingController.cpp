// PALDARK W46 — Briefing room UI controller implementation.

#include "Hub/PaldarkBriefingController.h"

#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Hub/PaldarkBriefingRoom.h"
#include "Hub/PaldarkBriefingSessionComponent.h"
#include "Hub/PaldarkHubBuildingDefinition.h"
#include "Map/PaldarkMapDefinition.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"
#include "Player/PaldarkPlayerController.h"

UPaldarkBriefingController::UPaldarkBriefingController()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPaldarkBriefingController::BeginPlay()
{
	Super::BeginPlay();

	OwnerController = Cast<APaldarkPlayerController>(GetOwner());
	UIStateTag = PaldarkGameplayTags::TAG_Paldark_Hub_Brief_UI_Closed;

	if (OwnerController == nullptr)
	{
		UE_LOG(LogPaldarkUI, Warning,
			TEXT("[Hub.Brief] UPaldarkBriefingController BeginPlay — outer is not APaldarkPlayerController; controller will be a no-op."));
	}
}

UPaldarkBriefingSessionComponent* UPaldarkBriefingController::ResolveSessionComponent() const
{
	if (UWorld* World = GetWorld())
	{
		return UPaldarkBriefingSessionComponent::Get(World);
	}
	return nullptr;
}

// ----------------------------------------------------------------------------
// Client API
// ----------------------------------------------------------------------------

void UPaldarkBriefingController::RequestOpenBriefing(APaldarkBriefingRoom* BriefingRoom)
{
	ActiveBriefingRoom = BriefingRoom;
	UIStateTag = PaldarkGameplayTags::TAG_Paldark_Hub_Brief_UI_Open;

	UE_LOG(LogPaldarkUI, Log,
		TEXT("[Hub.Brief] RequestOpenBriefing — Room=%s, UI=%s"),
		BriefingRoom != nullptr ? *BriefingRoom->GetName() : TEXT("<null>"),
		*UIStateTag.ToString());

	OnBriefingOpened.Broadcast(BriefingRoom);
}

void UPaldarkBriefingController::RequestCloseBriefing()
{
	ActiveBriefingRoom = nullptr;
	UIStateTag = PaldarkGameplayTags::TAG_Paldark_Hub_Brief_UI_Closed;

	UE_LOG(LogPaldarkUI, Log,
		TEXT("[Hub.Brief] RequestCloseBriefing — UI=%s"),
		*UIStateTag.ToString());

	OnBriefingClosed.Broadcast();
}

void UPaldarkBriefingController::RequestVote(FGameplayTag MapTag)
{
	Server_RequestVote(MapTag);
}

void UPaldarkBriefingController::RequestUnvote()
{
	Server_RequestUnvote();
}

void UPaldarkBriefingController::RequestReady()
{
	Server_RequestReady();
}

void UPaldarkBriefingController::RequestUnready()
{
	Server_RequestUnready();
}

// ----------------------------------------------------------------------------
// Server RPCs
// ----------------------------------------------------------------------------

bool UPaldarkBriefingController::Server_RequestVote_Validate(FGameplayTag MapTag)
{
	return MapTag.IsValid();
}

void UPaldarkBriefingController::Server_RequestVote_Implementation(FGameplayTag MapTag)
{
	if (bIsTransactionInFlight)
	{
		Client_ReceiveActionResult(
			PaldarkGameplayTags::TAG_Paldark_Hub_Brief_Action_Vote,
			PaldarkGameplayTags::TAG_Paldark_Hub_Result_Fail_NotAllowed,
			MapTag, 0);
		return;
	}
	bIsTransactionInFlight = true;

	UPaldarkBriefingSessionComponent* Session = ResolveSessionComponent();
	if (Session == nullptr)
	{
		Client_ReceiveActionResult(
			PaldarkGameplayTags::TAG_Paldark_Hub_Brief_Action_Vote,
			PaldarkGameplayTags::TAG_Paldark_Hub_Result_Fail_NotAllowed,
			MapTag, 0);
		bIsTransactionInFlight = false;
		return;
	}

	const bool bAccepted = Session->ServerCastVote(OwnerController, MapTag);
	const FGameplayTag ResultTag = bAccepted
		? PaldarkGameplayTags::TAG_Paldark_Hub_Result_Success
		: PaldarkGameplayTags::TAG_Paldark_Hub_Result_Fail_NotAllowed;

	UE_LOG(LogPaldarkUI, Log,
		TEXT("[Hub.Brief.Server] Server_RequestVote PC=%s MapTag=%s → %s"),
		OwnerController != nullptr ? *OwnerController->GetName() : TEXT("<null>"),
		*MapTag.ToString(),
		*ResultTag.ToString());

	Client_ReceiveActionResult(
		PaldarkGameplayTags::TAG_Paldark_Hub_Brief_Action_Vote,
		ResultTag,
		MapTag,
		Session->GetCountdownRemaining());
	bIsTransactionInFlight = false;
}

bool UPaldarkBriefingController::Server_RequestUnvote_Validate()
{
	return true;
}

void UPaldarkBriefingController::Server_RequestUnvote_Implementation()
{
	if (bIsTransactionInFlight)
	{
		Client_ReceiveActionResult(
			PaldarkGameplayTags::TAG_Paldark_Hub_Brief_Action_Unvote,
			PaldarkGameplayTags::TAG_Paldark_Hub_Result_Fail_NotAllowed,
			FGameplayTag(), 0);
		return;
	}
	bIsTransactionInFlight = true;

	UPaldarkBriefingSessionComponent* Session = ResolveSessionComponent();
	const bool bCleared = Session != nullptr && Session->ServerClearVote(OwnerController);
	const FGameplayTag ResultTag = bCleared
		? PaldarkGameplayTags::TAG_Paldark_Hub_Result_Success
		: PaldarkGameplayTags::TAG_Paldark_Hub_Result_Fail_Empty;

	UE_LOG(LogPaldarkUI, Log,
		TEXT("[Hub.Brief.Server] Server_RequestUnvote PC=%s → %s"),
		OwnerController != nullptr ? *OwnerController->GetName() : TEXT("<null>"),
		*ResultTag.ToString());

	Client_ReceiveActionResult(
		PaldarkGameplayTags::TAG_Paldark_Hub_Brief_Action_Unvote,
		ResultTag,
		FGameplayTag(),
		Session != nullptr ? Session->GetCountdownRemaining() : 0);
	bIsTransactionInFlight = false;
}

bool UPaldarkBriefingController::Server_RequestReady_Validate()
{
	return true;
}

void UPaldarkBriefingController::Server_RequestReady_Implementation()
{
	if (bIsTransactionInFlight)
	{
		Client_ReceiveActionResult(
			PaldarkGameplayTags::TAG_Paldark_Hub_Brief_Action_Ready,
			PaldarkGameplayTags::TAG_Paldark_Hub_Result_Fail_NotAllowed,
			FGameplayTag(), 0);
		return;
	}
	bIsTransactionInFlight = true;

	UPaldarkBriefingSessionComponent* Session = ResolveSessionComponent();
	const bool bChanged = Session != nullptr && Session->ServerSetReady(OwnerController, true);
	const FGameplayTag ResultTag = bChanged
		? PaldarkGameplayTags::TAG_Paldark_Hub_Result_Success
		: PaldarkGameplayTags::TAG_Paldark_Hub_Result_Fail_NotAllowed;

	UE_LOG(LogPaldarkUI, Log,
		TEXT("[Hub.Brief.Server] Server_RequestReady PC=%s → %s (countdown=%d)"),
		OwnerController != nullptr ? *OwnerController->GetName() : TEXT("<null>"),
		*ResultTag.ToString(),
		Session != nullptr ? Session->GetCountdownRemaining() : 0);

	Client_ReceiveActionResult(
		PaldarkGameplayTags::TAG_Paldark_Hub_Brief_Action_Ready,
		ResultTag,
		FGameplayTag(),
		Session != nullptr ? Session->GetCountdownRemaining() : 0);
	bIsTransactionInFlight = false;
}

bool UPaldarkBriefingController::Server_RequestUnready_Validate()
{
	return true;
}

void UPaldarkBriefingController::Server_RequestUnready_Implementation()
{
	if (bIsTransactionInFlight)
	{
		Client_ReceiveActionResult(
			PaldarkGameplayTags::TAG_Paldark_Hub_Brief_Action_Unready,
			PaldarkGameplayTags::TAG_Paldark_Hub_Result_Fail_NotAllowed,
			FGameplayTag(), 0);
		return;
	}
	bIsTransactionInFlight = true;

	UPaldarkBriefingSessionComponent* Session = ResolveSessionComponent();
	const bool bChanged = Session != nullptr && Session->ServerSetReady(OwnerController, false);
	const FGameplayTag ResultTag = bChanged
		? PaldarkGameplayTags::TAG_Paldark_Hub_Result_Success
		: PaldarkGameplayTags::TAG_Paldark_Hub_Result_Fail_Empty;

	UE_LOG(LogPaldarkUI, Log,
		TEXT("[Hub.Brief.Server] Server_RequestUnready PC=%s → %s"),
		OwnerController != nullptr ? *OwnerController->GetName() : TEXT("<null>"),
		*ResultTag.ToString());

	Client_ReceiveActionResult(
		PaldarkGameplayTags::TAG_Paldark_Hub_Brief_Action_Unready,
		ResultTag,
		FGameplayTag(),
		Session != nullptr ? Session->GetCountdownRemaining() : 0);
	bIsTransactionInFlight = false;
}

// ----------------------------------------------------------------------------
// Client RPC
// ----------------------------------------------------------------------------

void UPaldarkBriefingController::Client_ReceiveActionResult_Implementation(
	FGameplayTag ActionTag,
	FGameplayTag ResultTag,
	FGameplayTag MapTag,
	int32 CountdownRemaining)
{
	UE_LOG(LogPaldarkUI, Log,
		TEXT("[Hub.Brief.Client] Client_ReceiveActionResult — Action=%s Result=%s Map=%s Countdown=%d"),
		*ActionTag.ToString(),
		*ResultTag.ToString(),
		MapTag.IsValid() ? *MapTag.ToString() : TEXT("<none>"),
		CountdownRemaining);

	OnBriefingActionResult.Broadcast(ActionTag, ResultTag, MapTag, CountdownRemaining);
}
