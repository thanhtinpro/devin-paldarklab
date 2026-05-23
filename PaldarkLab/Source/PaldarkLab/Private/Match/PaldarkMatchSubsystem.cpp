#include "Match/PaldarkMatchSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"

#include "Framework/PaldarkGameStateBase.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"
#include "Player/PaldarkPlayerState.h"

UPaldarkMatchSubsystem::UPaldarkMatchSubsystem() = default;

bool UPaldarkMatchSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// Mirrors UPaldarkSquadSubsystem: server / standalone only. Clients
	// read the replicated mirror on APaldarkGameStateBase.
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}
	const UWorld* World = Cast<UWorld>(Outer);
	if (World == nullptr)
	{
		return false;
	}
	const ENetMode NetMode = World->GetNetMode();
	return NetMode != NM_Client;
}

void UPaldarkMatchSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CurrentPhase             = EPaldarkMatchPhase::Warmup;
	EndReason                = EPaldarkMatchEndReason::None;
	PhaseEndServerTime       = 0.f;
	MatchStartedAtServerTime = 0.f;
	PlayerRows.Reset();

	UE_LOG(LogPaldarkPlayer, Log,
		TEXT("UPaldarkMatchSubsystem::Initialize — phase=Warmup, warmup=%.1fs, max=%.1fs"),
		WarmupDuration, MaxMatchDuration);
}

void UPaldarkMatchSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		FTimerManager& Timers = World->GetTimerManager();
		Timers.ClearTimer(WarmupTimerHandle);
		Timers.ClearTimer(HardTimeoutTimerHandle);
	}
	PlayerRows.Reset();
	Super::Deinitialize();
}

void UPaldarkMatchSubsystem::RegisterPlayer(APlayerState* PlayerState)
{
	if (PlayerState == nullptr)
	{
		return;
	}

	FPaldarkMatchPlayerRow& Row = PlayerRows.FindOrAdd(PlayerState);
	if (Row.PlayerState.IsValid())
	{
		// Already registered.
		return;
	}

	Row.PlayerState         = PlayerState;
	Row.Outcome             = EPaldarkPlayerOutcome::Alive;
	Row.JoinedAtServerTime  = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.f;
	Row.TerminalAtServerTime = 0.f;

	UE_LOG(LogPaldarkPlayer, Log,
		TEXT("UPaldarkMatchSubsystem::RegisterPlayer — %s registered (alive=%d/total=%d)"),
		*PlayerState->GetName(),
		GetAlivePlayerCount(),
		PlayerRows.Num());

	// On the first player join, start the warmup countdown — so a 1-player
	// playtest doesn't hang in Warmup forever.
	if (CurrentPhase == EPaldarkMatchPhase::Warmup
		&& PlayerRows.Num() == 1
		&& WarmupDuration > 0.f
		&& GetWorld() != nullptr)
	{
		GetWorld()->GetTimerManager().SetTimer(
			WarmupTimerHandle,
			FTimerDelegate::CreateUObject(this, &UPaldarkMatchSubsystem::OnWarmupTimerExpired),
			WarmupDuration,
			/*bLoop=*/false);
		PhaseEndServerTime = GetWorld()->GetTimeSeconds() + WarmupDuration;
		MirrorToGameState();
	}
}

void UPaldarkMatchSubsystem::UnregisterPlayer(APlayerState* PlayerState)
{
	if (PlayerState == nullptr)
	{
		return;
	}
	PlayerRows.Remove(PlayerState);
	UE_LOG(LogPaldarkPlayer, Log,
		TEXT("UPaldarkMatchSubsystem::UnregisterPlayer — %s removed (remaining=%d)"),
		*PlayerState->GetName(),
		PlayerRows.Num());
}

void UPaldarkMatchSubsystem::StartMatch()
{
	if (CurrentPhase != EPaldarkMatchPhase::Warmup)
	{
		UE_LOG(LogPaldarkPlayer, Warning,
			TEXT("UPaldarkMatchSubsystem::StartMatch — ignored (phase=%d)."),
			static_cast<int32>(CurrentPhase));
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WarmupTimerHandle);
		MatchStartedAtServerTime = World->GetTimeSeconds();
		if (MaxMatchDuration > 0.f)
		{
			World->GetTimerManager().SetTimer(
				HardTimeoutTimerHandle,
				FTimerDelegate::CreateUObject(this, &UPaldarkMatchSubsystem::OnMatchHardTimeoutExpired),
				MaxMatchDuration,
				/*bLoop=*/false);
			PhaseEndServerTime = MatchStartedAtServerTime + MaxMatchDuration;
		}
		else
		{
			PhaseEndServerTime = 0.f;
		}
	}

	SetPhase(EPaldarkMatchPhase::Active);
}

void UPaldarkMatchSubsystem::RecordExtraction(APlayerState* PlayerState)
{
	if (CurrentPhase == EPaldarkMatchPhase::Ended)
	{
		return;
	}
	if (CurrentPhase == EPaldarkMatchPhase::Warmup)
	{
		// Beacons should not accept overlap during warmup. Defensive
		// guard in case a designer wires a tutorial-time beacon.
		UE_LOG(LogPaldarkPlayer, Warning,
			TEXT("UPaldarkMatchSubsystem::RecordExtraction — ignored during Warmup."));
		return;
	}
	if (!SetPlayerOutcomeInternal(PlayerState, EPaldarkPlayerOutcome::Extracted))
	{
		return;
	}

	// Flip phase to Extracting on the FIRST extraction event of the match.
	// Bookkeeping only — subsequent extractions don't bounce back through
	// SetPhase since the FSM is monotonic Active -> Extracting -> Ended.
	if (CurrentPhase == EPaldarkMatchPhase::Active)
	{
		SetPhase(EPaldarkMatchPhase::Extracting);
	}

	EvaluateEndCondition();
}

void UPaldarkMatchSubsystem::RecordDeath(APlayerState* PlayerState)
{
	if (CurrentPhase == EPaldarkMatchPhase::Ended)
	{
		return;
	}
	if (!SetPlayerOutcomeInternal(PlayerState, EPaldarkPlayerOutcome::KIA))
	{
		return;
	}
	EvaluateEndCondition();
}

void UPaldarkMatchSubsystem::RecordDisconnect(APlayerState* PlayerState)
{
	// Disconnects DO get processed in Ended (so the server's player
	// tracking matches the GameMode's; the outcome remains
	// "Disconnected" even after the match has officially ended).
	if (!SetPlayerOutcomeInternal(PlayerState, EPaldarkPlayerOutcome::Disconnected))
	{
		return;
	}
	if (CurrentPhase != EPaldarkMatchPhase::Ended)
	{
		EvaluateEndCondition();
	}
}

void UPaldarkMatchSubsystem::EndMatch(EPaldarkMatchEndReason Reason)
{
	if (CurrentPhase == EPaldarkMatchPhase::Ended)
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WarmupTimerHandle);
		World->GetTimerManager().ClearTimer(HardTimeoutTimerHandle);
	}
	PhaseEndServerTime = 0.f;
	EndReason = Reason;

	SetPhase(EPaldarkMatchPhase::Ended);

	UE_LOG(LogPaldarkPlayer, Log,
		TEXT("UPaldarkMatchSubsystem::EndMatch — reason=%d alive=%d extracted=%d kia=%d disc=%d total=%d"),
		static_cast<int32>(Reason),
		GetAlivePlayerCount(),
		GetExtractedPlayerCount(),
		GetKIAPlayerCount(),
		GetDisconnectedPlayerCount(),
		PlayerRows.Num());

	OnMatchEnded.Broadcast(Reason);
}

int32 UPaldarkMatchSubsystem::GetAlivePlayerCount() const
{
	int32 Count = 0;
	for (const TPair<TObjectPtr<APlayerState>, FPaldarkMatchPlayerRow>& Pair : PlayerRows)
	{
		if (Pair.Value.Outcome == EPaldarkPlayerOutcome::Alive)
		{
			++Count;
		}
	}
	return Count;
}

int32 UPaldarkMatchSubsystem::GetExtractedPlayerCount() const
{
	int32 Count = 0;
	for (const TPair<TObjectPtr<APlayerState>, FPaldarkMatchPlayerRow>& Pair : PlayerRows)
	{
		if (Pair.Value.Outcome == EPaldarkPlayerOutcome::Extracted)
		{
			++Count;
		}
	}
	return Count;
}

int32 UPaldarkMatchSubsystem::GetKIAPlayerCount() const
{
	int32 Count = 0;
	for (const TPair<TObjectPtr<APlayerState>, FPaldarkMatchPlayerRow>& Pair : PlayerRows)
	{
		if (Pair.Value.Outcome == EPaldarkPlayerOutcome::KIA)
		{
			++Count;
		}
	}
	return Count;
}

int32 UPaldarkMatchSubsystem::GetDisconnectedPlayerCount() const
{
	int32 Count = 0;
	for (const TPair<TObjectPtr<APlayerState>, FPaldarkMatchPlayerRow>& Pair : PlayerRows)
	{
		if (Pair.Value.Outcome == EPaldarkPlayerOutcome::Disconnected)
		{
			++Count;
		}
	}
	return Count;
}

EPaldarkPlayerOutcome UPaldarkMatchSubsystem::GetPlayerOutcome(APlayerState* PlayerState) const
{
	if (PlayerState == nullptr)
	{
		return EPaldarkPlayerOutcome::Alive;
	}
	if (const FPaldarkMatchPlayerRow* Row = PlayerRows.Find(PlayerState))
	{
		return Row->Outcome;
	}
	return EPaldarkPlayerOutcome::Alive;
}

void UPaldarkMatchSubsystem::DumpToLog() const
{
	UE_LOG(LogPaldarkPlayer, Log,
		TEXT("[Match] phase=%d reason=%d phase_end=%.2f match_start=%.2f rows=%d "
		     "(alive=%d extracted=%d kia=%d disc=%d)"),
		static_cast<int32>(CurrentPhase),
		static_cast<int32>(EndReason),
		PhaseEndServerTime,
		MatchStartedAtServerTime,
		PlayerRows.Num(),
		GetAlivePlayerCount(),
		GetExtractedPlayerCount(),
		GetKIAPlayerCount(),
		GetDisconnectedPlayerCount());

	for (const TPair<TObjectPtr<APlayerState>, FPaldarkMatchPlayerRow>& Pair : PlayerRows)
	{
		const APlayerState* PS = Pair.Value.PlayerState.Get();
		const FString PSName = PS != nullptr ? PS->GetName() : TEXT("<dead>");
		UE_LOG(LogPaldarkPlayer, Log,
			TEXT("  player=%s outcome=%d joined=%.2f terminal=%.2f"),
			*PSName,
			static_cast<int32>(Pair.Value.Outcome),
			Pair.Value.JoinedAtServerTime,
			Pair.Value.TerminalAtServerTime);
	}
}

void UPaldarkMatchSubsystem::SetPhase(EPaldarkMatchPhase NewPhase)
{
	if (CurrentPhase == NewPhase)
	{
		return;
	}
	const EPaldarkMatchPhase OldPhase = CurrentPhase;
	CurrentPhase = NewPhase;

	UE_LOG(LogPaldarkPlayer, Log,
		TEXT("UPaldarkMatchSubsystem::SetPhase — %d -> %d"),
		static_cast<int32>(OldPhase),
		static_cast<int32>(NewPhase));

	MirrorToGameState();
	OnMatchPhaseChanged.Broadcast(NewPhase);
}

void UPaldarkMatchSubsystem::EvaluateEndCondition()
{
	if (CurrentPhase == EPaldarkMatchPhase::Ended
		|| CurrentPhase == EPaldarkMatchPhase::Warmup)
	{
		return;
	}

	// Count non-terminal players. If zero, every player either extracted,
	// died, or disconnected — match is over.
	const int32 Alive     = GetAlivePlayerCount();
	const int32 Extracted = GetExtractedPlayerCount();
	const int32 KIA       = GetKIAPlayerCount();

	if (Alive > 0)
	{
		return;
	}

	// Decide reason from the counts. Extracted > 0 means at least one
	// player got out; otherwise it's a wipe. Disconnected-only counts as
	// a wipe too (the run has no survivors).
	EPaldarkMatchEndReason Reason = EPaldarkMatchEndReason::TeamWipe;
	if (Extracted > 0 && KIA == 0)
	{
		Reason = EPaldarkMatchEndReason::AllExtracted;
	}
	else if (Extracted > 0)
	{
		// Mixed outcome — call it AllExtracted because at least one player
		// won. Designer can override via console.
		Reason = EPaldarkMatchEndReason::AllExtracted;
	}

	EndMatch(Reason);
}

bool UPaldarkMatchSubsystem::SetPlayerOutcomeInternal(
	APlayerState* PlayerState,
	EPaldarkPlayerOutcome NewOutcome)
{
	if (PlayerState == nullptr)
	{
		return false;
	}
	FPaldarkMatchPlayerRow* Row = PlayerRows.Find(PlayerState);
	if (Row == nullptr)
	{
		UE_LOG(LogPaldarkPlayer, Warning,
			TEXT("UPaldarkMatchSubsystem::SetPlayerOutcomeInternal — %s not registered."),
			*PlayerState->GetName());
		return false;
	}
	if (Row->Outcome != EPaldarkPlayerOutcome::Alive)
	{
		// Terminal players are immutable; subsequent events are no-ops.
		return false;
	}
	Row->Outcome              = NewOutcome;
	Row->TerminalAtServerTime = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.f;

	UE_LOG(LogPaldarkPlayer, Log,
		TEXT("UPaldarkMatchSubsystem::SetPlayerOutcomeInternal — %s -> %d (alive=%d/total=%d)"),
		*PlayerState->GetName(),
		static_cast<int32>(NewOutcome),
		GetAlivePlayerCount(),
		PlayerRows.Num());

	// Stamp the outcome tag onto the PlayerState so the (designer-authored)
	// end-screen widget can branch off a tag instead of an enum.
	if (APaldarkPlayerState* PaldarkPS = Cast<APaldarkPlayerState>(PlayerState))
	{
		PaldarkPS->SetMatchOutcome(NewOutcome);
	}

	OnPlayerOutcomeChanged.Broadcast(PlayerState, NewOutcome);
	return true;
}

void UPaldarkMatchSubsystem::MirrorToGameState()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}
	APaldarkGameStateBase* GS = World->GetGameState<APaldarkGameStateBase>();
	if (GS == nullptr)
	{
		return;
	}
	GS->SetMatchPhaseFromSubsystem(CurrentPhase, EndReason, PhaseEndServerTime);
}

void UPaldarkMatchSubsystem::OnWarmupTimerExpired()
{
	if (CurrentPhase == EPaldarkMatchPhase::Warmup)
	{
		UE_LOG(LogPaldarkPlayer, Log,
			TEXT("UPaldarkMatchSubsystem::OnWarmupTimerExpired — auto-advance to Active."));
		StartMatch();
	}
}

void UPaldarkMatchSubsystem::OnMatchHardTimeoutExpired()
{
	if (CurrentPhase != EPaldarkMatchPhase::Ended)
	{
		UE_LOG(LogPaldarkPlayer, Warning,
			TEXT("UPaldarkMatchSubsystem::OnMatchHardTimeoutExpired — forcing Timeout."));
		EndMatch(EPaldarkMatchEndReason::Timeout);
	}
}
