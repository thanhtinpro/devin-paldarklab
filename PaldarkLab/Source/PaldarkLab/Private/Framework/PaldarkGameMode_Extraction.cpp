#include "Framework/PaldarkGameMode_Extraction.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

#include "Match/PaldarkMatchSubsystem.h"
#include "PaldarkLogCategories.h"

APaldarkGameMode_Extraction::APaldarkGameMode_Extraction() = default;

void APaldarkGameMode_Extraction::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (NewPlayer == nullptr)
	{
		return;
	}
	APlayerState* PS = NewPlayer->PlayerState;
	if (PS == nullptr)
	{
		UE_LOG(LogPaldarkPlayer, Warning,
			TEXT("APaldarkGameMode_Extraction::PostLogin — %s has no PlayerState yet; skipping match register."),
			*NewPlayer->GetName());
		return;
	}
	if (UPaldarkMatchSubsystem* MatchSub = GetMatchSubsystem())
	{
		MatchSub->RegisterPlayer(PS);
	}
}

void APaldarkGameMode_Extraction::Logout(AController* Exiting)
{
	if (Exiting != nullptr)
	{
		if (APlayerState* PS = Exiting->PlayerState)
		{
			if (UPaldarkMatchSubsystem* MatchSub = GetMatchSubsystem())
			{
				MatchSub->RecordDisconnect(PS);
				MatchSub->UnregisterPlayer(PS);
			}
		}
	}
	Super::Logout(Exiting);
}

void APaldarkGameMode_Extraction::BeginPlay()
{
	Super::BeginPlay();

	if (UPaldarkMatchSubsystem* MatchSub = GetMatchSubsystem())
	{
		MatchEndedHandle = MatchSub->OnMatchEnded.AddUObject(
			this,
			&APaldarkGameMode_Extraction::HandleMatchEnded);
	}
}

void APaldarkGameMode_Extraction::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UPaldarkMatchSubsystem* MatchSub = GetMatchSubsystem())
	{
		MatchSub->OnMatchEnded.Remove(MatchEndedHandle);
	}
	MatchEndedHandle.Reset();

	Super::EndPlay(EndPlayReason);
}

void APaldarkGameMode_Extraction::HandleMatchEnded(EPaldarkMatchEndReason Reason)
{
	UE_LOG(LogPaldarkPlayer, Log,
		TEXT("APaldarkGameMode_Extraction::HandleMatchEnded — reason=%d. Scoreboard follows."),
		static_cast<int32>(Reason));

	if (UPaldarkMatchSubsystem* MatchSub = GetMatchSubsystem())
	{
		MatchSub->DumpToLog();
	}

	// Lobby travel + win/loss widget broadcast deferred (W40+ hub, W22+
	// polish). This is the integration seam designers extend in BP.
}

UPaldarkMatchSubsystem* APaldarkGameMode_Extraction::GetMatchSubsystem() const
{
	UWorld* World = GetWorld();
	return World != nullptr ? World->GetSubsystem<UPaldarkMatchSubsystem>() : nullptr;
}
