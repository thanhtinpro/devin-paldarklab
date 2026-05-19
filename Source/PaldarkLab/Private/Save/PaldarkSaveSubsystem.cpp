// PALDARK W47 — Save game subsystem implementation.
// PALDARK W48 — Auto-save hooks (extract / logout) + hub→raid handoff smoke.

#include "Save/PaldarkSaveSubsystem.h"

#include "PaldarkLab.h"
#include "PaldarkGameplayTags.h"
#include "Save/PaldarkPlayerSaveGame.h"
#include "Player/PaldarkPlayerController.h"
#include "Player/PaldarkCharacter.h"
#include "Player/PaldarkPlayerState.h"
#include "Pal/PaldarkPlayerPalRosterComponent.h"
#include "Pal/PaldarkPalDepositComponent.h"
#include "Player/Components/PaldarkPlayerInventoryComponent.h"
#include "Match/PaldarkMatchSubsystem.h"
#include "Match/PaldarkMatchTypes.h"
#include "Net/PaldarkNetSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SaveGame.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Paths.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UObjectGlobals.h"

UPaldarkSaveSubsystem::UPaldarkSaveSubsystem()
{
}

void UPaldarkSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// W48 — bind to PostLoadMapWithWorld so we get a chance to refresh the
	// world-scope match subsystem hook + perform a one-time bind of the
	// game-instance-scope net subsystem logout delegate. Both are deferred
	// from Initialize because peer subsystems may not be ready yet during
	// the FSubsystemCollection bootstrap.
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &UPaldarkSaveSubsystem::HandlePostLoadMapWithWorld);

	UE_LOG(LogPaldarkLab, Log,
		TEXT("[Save] subsystem initialised (default user index=%d, auto-extract=%d, auto-logout=%d)"),
		kDefaultUserIndex,
		bAutoSaveOnExtract ? 1 : 0,
		bAutoSaveOnLogout ? 1 : 0);
}

void UPaldarkSaveSubsystem::Deinitialize()
{
	// W48 — unbind PostLoadMapWithWorld + per-world match outcome hook.
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}
	if (UPaldarkMatchSubsystem* Match = BoundMatchSubsystem.Get())
	{
		Match->OnPlayerOutcomeChanged.Remove(MatchOutcomeHandle);
	}
	MatchOutcomeHandle.Reset();
	BoundMatchSubsystem.Reset();

	if (bBoundNetLogout)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UPaldarkNetSubsystem* Net = GI->GetSubsystem<UPaldarkNetSubsystem>())
			{
				Net->OnPlayerLogout.RemoveDynamic(this, &UPaldarkSaveSubsystem::HandlePlayerLogoutAutoSave);
			}
		}
		bBoundNetLogout = false;
	}

	// Cancel pending in-flight work — the engine's async pipeline doesn't
	// expose a cancellation hook, but we can drop the trigger map so any
	// late-firing delegates short-circuit on slot lookup.
	InFlightSaveSlots.Reset();
	InFlightLoads.Reset();
	PendingSaveTriggers.Reset();
	Super::Deinitialize();
}

// ============================================================================
// Public APIs
// ============================================================================

bool UPaldarkSaveSubsystem::RequestSaveSlot(APaldarkPlayerController* PC,
	FName SlotName,
	FGameplayTag TriggerTag)
{
	if (PC == nullptr)
	{
		UE_LOG(LogPaldarkLab, Warning, TEXT("[Save] RequestSaveSlot rejected: null PC"));
		return false;
	}
	if (SlotName.IsNone())
	{
		SlotName = GetDefaultSlotName(PC);
	}
	if (!TriggerTag.IsValid())
	{
		TriggerTag = PaldarkGameplayTags::TAG_Paldark_Save_Trigger_Manual;
	}
	if (InFlightSaveSlots.Contains(SlotName))
	{
		UE_LOG(LogPaldarkLab, Warning,
			TEXT("[Save] RequestSaveSlot rejected: slot=%s already has an in-flight write"),
			*SlotName.ToString());
		return false;
	}

	UPaldarkPlayerSaveGame* Snapshot = BuildSnapshotForPC(PC);
	if (Snapshot == nullptr)
	{
		UE_LOG(LogPaldarkLab, Warning,
			TEXT("[Save] RequestSaveSlot rejected: failed to capture snapshot for PC=%s"),
			*PC->GetName());
		OnSaveCompleted.Broadcast(SlotName, PaldarkGameplayTags::TAG_Paldark_Save_Result_Fail_IOError, TriggerTag);
		return false;
	}

	InFlightSaveSlots.Add(SlotName);
	PendingSaveTriggers.Add(SlotName, TriggerTag);

	FAsyncSaveGameToSlotDelegate Cb;
	Cb.BindUObject(this, &UPaldarkSaveSubsystem::HandleSaveComplete);

	UE_LOG(LogPaldarkLab, Log,
		TEXT("[Save] async write begin slot=%s trigger=%s schema=%d active=%d deposit=%d inventory=%d"),
		*SlotName.ToString(),
		*TriggerTag.ToString(),
		Snapshot->SchemaVersion,
		Snapshot->ActiveRoster.Num(),
		Snapshot->DepositRoster.Num(),
		Snapshot->Inventory.Num());

	UGameplayStatics::AsyncSaveGameToSlot(Snapshot, SlotName.ToString(), kDefaultUserIndex, Cb);
	return true;
}

bool UPaldarkSaveSubsystem::RequestLoadSlot(APaldarkPlayerController* PC,
	FName SlotName)
{
	if (PC == nullptr)
	{
		UE_LOG(LogPaldarkLab, Warning, TEXT("[Save] RequestLoadSlot rejected: null PC"));
		return false;
	}
	if (SlotName.IsNone())
	{
		SlotName = GetDefaultSlotName(PC);
	}
	if (!UGameplayStatics::DoesSaveGameExist(SlotName.ToString(), kDefaultUserIndex))
	{
		UE_LOG(LogPaldarkLab, Warning,
			TEXT("[Save] RequestLoadSlot rejected: slot=%s does not exist"),
			*SlotName.ToString());
		OnLoadCompleted.Broadcast(SlotName, PaldarkGameplayTags::TAG_Paldark_Save_Result_Fail_NoSlot);
		return false;
	}

	FPendingLoad Pending;
	Pending.SlotName = SlotName;
	Pending.PC = PC;
	InFlightLoads.Add(Pending);

	FAsyncLoadGameFromSlotDelegate Cb;
	Cb.BindUObject(this, &UPaldarkSaveSubsystem::HandleLoadComplete);

	UE_LOG(LogPaldarkLab, Log,
		TEXT("[Save] async read begin slot=%s for PC=%s"),
		*SlotName.ToString(),
		*PC->GetName());

	UGameplayStatics::AsyncLoadGameFromSlot(SlotName.ToString(), kDefaultUserIndex, Cb);
	return true;
}

bool UPaldarkSaveSubsystem::RequestDumpSlot(FName SlotName)
{
	if (SlotName.IsNone())
	{
		UE_LOG(LogPaldarkLab, Warning, TEXT("[Save] RequestDumpSlot rejected: empty slot name"));
		return false;
	}
	if (!UGameplayStatics::DoesSaveGameExist(SlotName.ToString(), kDefaultUserIndex))
	{
		UE_LOG(LogPaldarkLab, Warning,
			TEXT("[Save] RequestDumpSlot rejected: slot=%s does not exist"),
			*SlotName.ToString());
		return false;
	}

	USaveGame* Raw = UGameplayStatics::LoadGameFromSlot(SlotName.ToString(), kDefaultUserIndex);
	UPaldarkPlayerSaveGame* SaveGame = Cast<UPaldarkPlayerSaveGame>(Raw);
	if (SaveGame == nullptr)
	{
		UE_LOG(LogPaldarkLab, Warning,
			TEXT("[Save] RequestDumpSlot: slot=%s is not a UPaldarkPlayerSaveGame (wrong type / corrupt)"),
			*SlotName.ToString());
		return false;
	}

	UE_LOG(LogPaldarkLab, Log,
		TEXT("[Save] DUMP slot=%s %s"),
		*SlotName.ToString(),
		*SaveGame->DescribeForLog());

	return true;
}

bool UPaldarkSaveSubsystem::ClearSlot(FName SlotName)
{
	if (SlotName.IsNone())
	{
		UE_LOG(LogPaldarkLab, Warning, TEXT("[Save] ClearSlot rejected: empty slot name"));
		return false;
	}
	const bool bSucceeded = UGameplayStatics::DeleteGameInSlot(SlotName.ToString(), kDefaultUserIndex);
	UE_LOG(LogPaldarkLab, Log,
		TEXT("[Save] ClearSlot slot=%s result=%s"),
		*SlotName.ToString(),
		bSucceeded ? TEXT("ok") : TEXT("fail"));
	return bSucceeded;
}

bool UPaldarkSaveSubsystem::DoesSlotExist(FName SlotName) const
{
	if (SlotName.IsNone())
	{
		return false;
	}
	return UGameplayStatics::DoesSaveGameExist(SlotName.ToString(), kDefaultUserIndex);
}

int32 UPaldarkSaveSubsystem::ListSlotsToLog() const
{
	const FString SaveDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"));
	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *SaveDir, TEXT("*.sav"),
		/*Files*/ true, /*Directories*/ false, /*ClearFileNames*/ true);

	UE_LOG(LogPaldarkLab, Log,
		TEXT("[Save] ListSlots dir=%s count=%d"),
		*SaveDir,
		Files.Num());

	for (const FString& AbsPath : Files)
	{
		const FString Rel = FPaths::GetCleanFilename(AbsPath);
		const int64 Size = IFileManager::Get().FileSize(*AbsPath);
		const FDateTime MTime = IFileManager::Get().GetTimeStamp(*AbsPath);
		UE_LOG(LogPaldarkLab, Log,
			TEXT("[Save]   %s  %lld bytes  mtime=%s"),
			*Rel,
			Size,
			*MTime.ToIso8601());
	}
	return Files.Num();
}

int32 UPaldarkSaveSubsystem::WipeAllSlots()
{
	const FString SaveDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"));
	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *SaveDir, TEXT("*.sav"),
		true, false, true);

	int32 Deleted = 0;
	for (const FString& AbsPath : Files)
	{
		// `DeleteGameInSlot` expects a slot string (filename without extension)
		// and the user index. The on-disk layout is `<save>/<user>/<slot>.sav`,
		// so extracting the basename gives us the slot name back.
		const FString BaseName = FPaths::GetBaseFilename(AbsPath);
		if (UGameplayStatics::DeleteGameInSlot(BaseName, kDefaultUserIndex))
		{
			Deleted++;
		}
	}
	UE_LOG(LogPaldarkLab, Log,
		TEXT("[Save] WipeAllSlots deleted=%d of %d candidate files"),
		Deleted, Files.Num());
	return Deleted;
}

// ============================================================================
// Static helpers
// ============================================================================

FName UPaldarkSaveSubsystem::GetDefaultSlotName(APaldarkPlayerController* PC)
{
	if (PC == nullptr)
	{
		return FName(TEXT("DefaultPlayer"));
	}
	if (const APlayerState* PS = PC->PlayerState)
	{
		// `GetUniqueId()` returns the OSS net id wrapper; on standalone /
		// listen-host PIE this often resolves to a dummy "INVALID" — fall
		// back to the player name in that case so two PIE players don't
		// stomp each other.
		const FUniqueNetIdRepl& NetId = PS->GetUniqueId();
		if (NetId.IsValid() && NetId->IsValid())
		{
			const FString IdString = NetId->ToString();
			if (!IdString.IsEmpty() && IdString != TEXT("INVALID"))
			{
				return FName(*FString::Printf(TEXT("Paldark_%s"), *IdString));
			}
		}
		const FString PlayerName = PS->GetPlayerName();
		if (!PlayerName.IsEmpty())
		{
			return FName(*FString::Printf(TEXT("Paldark_%s"), *PlayerName));
		}
	}
	return FName(TEXT("DefaultPlayer"));
}

// ============================================================================
// Private helpers
// ============================================================================

UPaldarkPlayerSaveGame* UPaldarkSaveSubsystem::BuildSnapshotForPC(APaldarkPlayerController* PC) const
{
	check(PC != nullptr);

	UPaldarkPlayerSaveGame* Snapshot = Cast<UPaldarkPlayerSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UPaldarkPlayerSaveGame::StaticClass()));
	if (Snapshot == nullptr)
	{
		return nullptr;
	}

	Snapshot->SchemaVersion = UPaldarkPlayerSaveGame::kCurrentSchema;
	Snapshot->SaveTime = FDateTime::UtcNow();
	if (const APlayerState* PS = PC->PlayerState)
	{
		Snapshot->PlayerName = PS->GetPlayerName();
	}

	// Active roster lives on the Character (per-pawn).
	if (APaldarkCharacter* Char = Cast<APaldarkCharacter>(PC->GetPawn()))
	{
		if (UPaldarkPlayerPalRosterComponent* Roster =
			Char->FindComponentByClass<UPaldarkPlayerPalRosterComponent>())
		{
			Snapshot->ActiveRoster = Roster->CaptureSnapshot();
		}
		if (UPaldarkPlayerInventoryComponent* Inventory =
			Char->FindComponentByClass<UPaldarkPlayerInventoryComponent>())
		{
			Snapshot->Inventory = Inventory->CaptureSnapshot();
		}
	}

	// Deposit lives on the PlayerState (survives pawn death).
	if (APlayerState* PS = PC->PlayerState)
	{
		if (UPaldarkPalDepositComponent* Deposit =
			PS->FindComponentByClass<UPaldarkPalDepositComponent>())
		{
			Snapshot->DepositRoster = Deposit->CaptureSnapshot();
		}
	}

	// Progress snapshot — placeholder until W48 polish wires the tutorial /
	// unlock systems. Fields stay default-initialised.
	Snapshot->Progress.LastHubMapName = NAME_None;
	if (const UWorld* World = PC->GetWorld())
	{
		Snapshot->Progress.LastHubMapName = *World->GetMapName();
	}

	return Snapshot;
}

FGameplayTag UPaldarkSaveSubsystem::ApplySnapshotToPC(APaldarkPlayerController* PC,
	UPaldarkPlayerSaveGame* SaveGame) const
{
	if (PC == nullptr || SaveGame == nullptr)
	{
		return PaldarkGameplayTags::TAG_Paldark_Save_Result_Fail_IOError;
	}

	if (APaldarkCharacter* Char = Cast<APaldarkCharacter>(PC->GetPawn()))
	{
		if (UPaldarkPlayerPalRosterComponent* Roster =
			Char->FindComponentByClass<UPaldarkPlayerPalRosterComponent>())
		{
			Roster->ApplySnapshot(SaveGame->ActiveRoster);
		}
		if (UPaldarkPlayerInventoryComponent* Inventory =
			Char->FindComponentByClass<UPaldarkPlayerInventoryComponent>())
		{
			Inventory->ApplySnapshot(SaveGame->Inventory);
		}
	}
	if (APlayerState* PS = PC->PlayerState)
	{
		if (UPaldarkPalDepositComponent* Deposit =
			PS->FindComponentByClass<UPaldarkPalDepositComponent>())
		{
			Deposit->ApplySnapshot(SaveGame->DepositRoster);
		}
	}

	UE_LOG(LogPaldarkLab, Log,
		TEXT("[Save] ApplySnapshot complete: %s"),
		*SaveGame->DescribeForLog());

	return PaldarkGameplayTags::TAG_Paldark_Save_Result_Success;
}

void UPaldarkSaveSubsystem::HandleSaveComplete(const FString& SlotName,
	const int32 UserIndex,
	bool bSucceeded)
{
	const FName SlotFName(*SlotName);
	InFlightSaveSlots.Remove(SlotFName);

	FGameplayTag TriggerTag = PaldarkGameplayTags::TAG_Paldark_Save_Trigger_Manual;
	if (FGameplayTag* Found = PendingSaveTriggers.Find(SlotFName))
	{
		TriggerTag = *Found;
		PendingSaveTriggers.Remove(SlotFName);
	}

	const FGameplayTag ResultTag = bSucceeded
		? PaldarkGameplayTags::TAG_Paldark_Save_Result_Success
		: PaldarkGameplayTags::TAG_Paldark_Save_Result_Fail_IOError;

	UE_LOG(LogPaldarkLab, Log,
		TEXT("[Save] async write complete slot=%s user=%d result=%s trigger=%s"),
		*SlotName, UserIndex,
		*ResultTag.ToString(), *TriggerTag.ToString());

	OnSaveCompleted.Broadcast(SlotFName, ResultTag, TriggerTag);
}

void UPaldarkSaveSubsystem::HandleLoadComplete(const FString& SlotName,
	const int32 UserIndex,
	USaveGame* SaveObject)
{
	const FName SlotFName(*SlotName);

	APaldarkPlayerController* PC = nullptr;
	for (int32 i = InFlightLoads.Num() - 1; i >= 0; --i)
	{
		if (InFlightLoads[i].SlotName == SlotFName)
		{
			PC = InFlightLoads[i].PC.Get();
			InFlightLoads.RemoveAtSwap(i);
			break;
		}
	}

	UPaldarkPlayerSaveGame* SaveGame = Cast<UPaldarkPlayerSaveGame>(SaveObject);
	FGameplayTag ResultTag;
	if (SaveGame == nullptr)
	{
		ResultTag = PaldarkGameplayTags::TAG_Paldark_Save_Result_Fail_IOError;
		UE_LOG(LogPaldarkLab, Warning,
			TEXT("[Save] async read complete slot=%s user=%d result=Fail.IOError (cast failed)"),
			*SlotName, UserIndex);
	}
	else if (SaveGame->SchemaVersion != UPaldarkPlayerSaveGame::kCurrentSchema)
	{
		ResultTag = PaldarkGameplayTags::TAG_Paldark_Save_Result_Fail_SchemaMismatch;
		UE_LOG(LogPaldarkLab, Warning,
			TEXT("[Save] async read complete slot=%s schema=%d current=%d result=Fail.SchemaMismatch"),
			*SlotName,
			SaveGame->SchemaVersion,
			UPaldarkPlayerSaveGame::kCurrentSchema);
	}
	else if (PC == nullptr)
	{
		ResultTag = PaldarkGameplayTags::TAG_Paldark_Save_Result_Fail_IOError;
		UE_LOG(LogPaldarkLab, Warning,
			TEXT("[Save] async read complete slot=%s result=Fail.IOError (PC gone before apply)"),
			*SlotName);
	}
	else
	{
		ResultTag = ApplySnapshotToPC(PC, SaveGame);
	}

	OnLoadCompleted.Broadcast(SlotFName, ResultTag);
}

// ============================================================================
// W48 polish: smoke-save + auto-save hooks
// ============================================================================

bool UPaldarkSaveSubsystem::RequestHubToRaidHandoffSmoke(
	APaldarkPlayerController* PC,
	FName SlotName)
{
	if (PC == nullptr)
	{
		UE_LOG(LogPaldarkLab, Warning,
			TEXT("[Save][W48] HubToRaidHandoffSmoke rejected: null PC"));
		return false;
	}
	if (SlotName.IsNone())
	{
		SlotName = GetDefaultSlotName(PC);
	}

	UE_LOG(LogPaldarkLab, Log,
		TEXT("[Save][W48] HubToRaidHandoffSmoke begin slot=%s PC=%s"),
		*SlotName.ToString(), *PC->GetName());

	return RequestSaveSlot(PC,
		SlotName,
		PaldarkGameplayTags::TAG_Paldark_Save_Trigger_Travel);
}

void UPaldarkSaveSubsystem::HandlePostLoadMapWithWorld(UWorld* LoadedWorld)
{
	if (LoadedWorld == nullptr)
	{
		return;
	}

	// Unbind any prior world's match subsystem hook before binding the new
	// world's. Weak-ref check guards against the subsystem already torn
	// down during world teardown.
	if (UPaldarkMatchSubsystem* PriorMatch = BoundMatchSubsystem.Get())
	{
		PriorMatch->OnPlayerOutcomeChanged.Remove(MatchOutcomeHandle);
	}
	MatchOutcomeHandle.Reset();
	BoundMatchSubsystem.Reset();

	if (bAutoSaveOnExtract)
	{
		if (UPaldarkMatchSubsystem* Match = LoadedWorld->GetSubsystem<UPaldarkMatchSubsystem>())
		{
			MatchOutcomeHandle = Match->OnPlayerOutcomeChanged.AddUObject(
				this, &UPaldarkSaveSubsystem::HandlePlayerOutcomeChanged);
			BoundMatchSubsystem = Match;
			UE_LOG(LogPaldarkLab, Log,
				TEXT("[Save][W48] bound MatchSubsystem.OnPlayerOutcomeChanged in world=%s"),
				*LoadedWorld->GetMapName());
		}
		else
		{
			// Hub map / front-end map will not have a match subsystem
			// (ShouldCreateSubsystem gates clients + non-extraction maps
			// out). This is expected — log Verbose, not Warning.
			UE_LOG(LogPaldarkLab, Verbose,
				TEXT("[Save][W48] no MatchSubsystem in world=%s — auto-save-on-extract idle here"),
				*LoadedWorld->GetMapName());
		}
	}

	if (bAutoSaveOnLogout && !bBoundNetLogout)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UPaldarkNetSubsystem* Net = GI->GetSubsystem<UPaldarkNetSubsystem>())
			{
				Net->OnPlayerLogout.AddDynamic(this, &UPaldarkSaveSubsystem::HandlePlayerLogoutAutoSave);
				bBoundNetLogout = true;
				UE_LOG(LogPaldarkLab, Log,
					TEXT("[Save][W48] bound NetSubsystem.OnPlayerLogout (game-instance scope)"));
			}
		}
	}
}

void UPaldarkSaveSubsystem::HandlePlayerOutcomeChanged(
	APlayerState* PlayerState,
	EPaldarkPlayerOutcome NewOutcome)
{
	if (!bAutoSaveOnExtract)
	{
		return;
	}
	if (PlayerState == nullptr)
	{
		return;
	}
	if (NewOutcome != EPaldarkPlayerOutcome::Extracted)
	{
		// Only the Extracted transition triggers auto-save. KIA / Disconnected
		// intentionally skip — KIA loses on-pawn state by design (drop on
		// death already wiped the live inventory), Disconnected is handled
		// by the net subsystem logout path below.
		return;
	}

	// Resolve the local player controller for this PlayerState. The save
	// subsystem only acts on the local machine's PC (listen-host / standalone);
	// on dedicated server the per-player save would need a different routing
	// path and is out of scope for W48.
	APaldarkPlayerController* PaldarkPC = nullptr;
	APlayerController* OwningPC = PlayerState->GetPlayerController();
	if (OwningPC != nullptr && OwningPC->IsLocalController())
	{
		PaldarkPC = Cast<APaldarkPlayerController>(OwningPC);
	}
	if (PaldarkPC == nullptr)
	{
		UE_LOG(LogPaldarkLab, Verbose,
			TEXT("[Save][W48] auto-save-on-extract skipped: PlayerState=%s has no local Paldark PC (likely remote / dedicated server)"),
			*PlayerState->GetName());
		return;
	}

	const FName SlotName = GetDefaultSlotName(PaldarkPC);
	UE_LOG(LogPaldarkLab, Log,
		TEXT("[Save][W48] auto-save-on-extract firing: PC=%s slot=%s trigger=HubReturn"),
		*PaldarkPC->GetName(), *SlotName.ToString());

	RequestSaveSlot(PaldarkPC,
		SlotName,
		PaldarkGameplayTags::TAG_Paldark_Save_Trigger_HubReturn);
}

void UPaldarkSaveSubsystem::HandlePlayerLogoutAutoSave(AController* Exiting)
{
	if (!bAutoSaveOnLogout)
	{
		return;
	}
	if (Exiting == nullptr)
	{
		return;
	}

	APaldarkPlayerController* PaldarkPC = Cast<APaldarkPlayerController>(Exiting);
	if (PaldarkPC == nullptr)
	{
		// AIController / spectator — nothing to save.
		return;
	}

	// Logout fires on the server. For listen-host the leaving PC is local
	// iff `IsLocalController()` is true (i.e. the host themselves is leaving);
	// remote clients disconnecting are server-only at this point and the
	// per-player save plumbing for dedicated server is W48-next-stretch.
	if (!PaldarkPC->IsLocalController())
	{
		UE_LOG(LogPaldarkLab, Verbose,
			TEXT("[Save][W48] auto-save-on-logout skipped: PC=%s is remote (dedicated-server save deferred)"),
			*PaldarkPC->GetName());
		return;
	}

	const FName SlotName = GetDefaultSlotName(PaldarkPC);
	UE_LOG(LogPaldarkLab, Log,
		TEXT("[Save][W48] auto-save-on-logout firing: PC=%s slot=%s trigger=Logout"),
		*PaldarkPC->GetName(), *SlotName.ToString());

	RequestSaveSlot(PaldarkPC,
		SlotName,
		PaldarkGameplayTags::TAG_Paldark_Save_Trigger_Logout);
}
