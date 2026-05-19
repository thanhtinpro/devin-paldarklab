// PALDARK W46 — Briefing session replicated state implementation.

#include "Hub/PaldarkBriefingSessionComponent.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Framework/PaldarkGameStateBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Hub/PaldarkBriefingRoom.h"
#include "Hub/PaldarkHubBuildingDefinition.h"
#include "Hub/PaldarkHubSubsystem.h"
#include "Map/PaldarkMapDefinition.h"
#include "Net/PaldarkNetSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"
#include "Player/PaldarkPlayerState.h"
#include "TimerManager.h"

UPaldarkBriefingSessionComponent::UPaldarkBriefingSessionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPaldarkBriefingSessionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UPaldarkBriefingSessionComponent, PhaseTag);
	DOREPLIFETIME(UPaldarkBriefingSessionComponent, VoterStates);
	DOREPLIFETIME(UPaldarkBriefingSessionComponent, CountdownRemaining);
}

void UPaldarkBriefingSessionComponent::BeginPlay()
{
	Super::BeginPlay();
	// Default to Idle until votes start coming in (server-only init; client
	// gets the value via the initial replication burst).
	if (GetOwner() != nullptr && GetOwner()->HasAuthority())
	{
		PhaseTag = PaldarkGameplayTags::TAG_Paldark_Hub_Brief_Phase_Idle;
	}
}

void UPaldarkBriefingSessionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CountdownTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

UPaldarkBriefingSessionComponent* UPaldarkBriefingSessionComponent::Get(const UWorld* World)
{
	if (World == nullptr)
	{
		return nullptr;
	}
	if (AGameStateBase* GS = World->GetGameState())
	{
		return GS->FindComponentByClass<UPaldarkBriefingSessionComponent>();
	}
	return nullptr;
}

// ----------------------------------------------------------------------------
// Authority API
// ----------------------------------------------------------------------------

FPaldarkBriefingVoterState* UPaldarkBriefingSessionComponent::FindVoterRow(APaldarkPlayerController* PC)
{
	if (PC == nullptr)
	{
		return nullptr;
	}
	APlayerState* PS = PC->PlayerState;
	if (PS == nullptr)
	{
		return nullptr;
	}
	for (FPaldarkBriefingVoterState& Row : VoterStates)
	{
		if (Row.PlayerState.Get() == PS)
		{
			return &Row;
		}
	}
	return nullptr;
}

FPaldarkBriefingVoterState* UPaldarkBriefingSessionComponent::FindOrAddVoterRow(APaldarkPlayerController* PC)
{
	if (FPaldarkBriefingVoterState* Existing = FindVoterRow(PC))
	{
		return Existing;
	}
	if (PC == nullptr || PC->PlayerState == nullptr)
	{
		return nullptr;
	}
	FPaldarkBriefingVoterState NewRow;
	NewRow.PlayerState = Cast<APaldarkPlayerState>(PC->PlayerState);
	if (!NewRow.PlayerState.IsValid())
	{
		NewRow.PlayerState = nullptr;
	}
	const int32 Index = VoterStates.Add(NewRow);
	return &VoterStates[Index];
}

bool UPaldarkBriefingSessionComponent::ServerCastVote(APaldarkPlayerController* PC, const FGameplayTag& MapTag)
{
	if (PC == nullptr || !MapTag.IsValid())
	{
		return false;
	}
	if (PhaseTag == PaldarkGameplayTags::TAG_Paldark_Hub_Brief_Phase_Travelling)
	{
		// Votes locked while the shard is mid-travel.
		return false;
	}

	FPaldarkBriefingVoterState* Row = FindOrAddVoterRow(PC);
	if (Row == nullptr)
	{
		return false;
	}
	const bool bChanged = (Row->VotedMapTag != MapTag);
	Row->VotedMapTag = MapTag;

	if (bChanged)
	{
		OnRep_VoterStates();
	}

	ServerEvaluateThreshold();
	return true;
}

bool UPaldarkBriefingSessionComponent::ServerClearVote(APaldarkPlayerController* PC)
{
	FPaldarkBriefingVoterState* Row = FindVoterRow(PC);
	if (Row == nullptr || !Row->VotedMapTag.IsValid())
	{
		return false;
	}
	Row->VotedMapTag = FGameplayTag();
	OnRep_VoterStates();
	ServerEvaluateThreshold();
	return true;
}

bool UPaldarkBriefingSessionComponent::ServerSetReady(APaldarkPlayerController* PC, bool bReady)
{
	if (PhaseTag == PaldarkGameplayTags::TAG_Paldark_Hub_Brief_Phase_Travelling)
	{
		// Ready locked while the shard is mid-travel.
		return false;
	}
	FPaldarkBriefingVoterState* Row = bReady
		? FindOrAddVoterRow(PC)
		: FindVoterRow(PC);
	if (Row == nullptr)
	{
		return false;
	}
	if (Row->bIsReady == bReady)
	{
		return false;
	}
	Row->bIsReady = bReady;
	OnRep_VoterStates();
	ServerEvaluateThreshold();
	return true;
}

bool UPaldarkBriefingSessionComponent::ServerRemovePlayer(APaldarkPlayerController* PC)
{
	if (PC == nullptr || PC->PlayerState == nullptr)
	{
		return false;
	}
	APlayerState* PS = PC->PlayerState;
	const int32 Removed = VoterStates.RemoveAll([PS](const FPaldarkBriefingVoterState& Row)
	{
		return Row.PlayerState.Get() == PS;
	});
	if (Removed > 0)
	{
		OnRep_VoterStates();
		ServerEvaluateThreshold();
		return true;
	}
	return false;
}

void UPaldarkBriefingSessionComponent::ServerForceTravel(const FGameplayTag& MapTag, const FString& ResolvedMapName)
{
	UE_LOG(LogPaldarkUI, Log,
		TEXT("[Hub.Brief.Server] ServerForceTravel — MapTag=%s ResolvedMapName=%s (bypassing vote/ready)"),
		*MapTag.ToString(),
		*ResolvedMapName);
	ServerIssueTravelForMap(MapTag, ResolvedMapName);
}

// ----------------------------------------------------------------------------
// Read-only state
// ----------------------------------------------------------------------------

TArray<FPaldarkBriefingVoteTally> UPaldarkBriefingSessionComponent::GetVoteTallies() const
{
	TMap<FGameplayTag, int32> Counts;
	for (const FPaldarkBriefingVoterState& Row : VoterStates)
	{
		if (!Row.VotedMapTag.IsValid())
		{
			continue;
		}
		Counts.FindOrAdd(Row.VotedMapTag) += 1;
	}
	TArray<FPaldarkBriefingVoteTally> Tallies;
	Tallies.Reserve(Counts.Num());
	for (const TPair<FGameplayTag, int32>& Pair : Counts)
	{
		FPaldarkBriefingVoteTally Tally;
		Tally.MapTag = Pair.Key;
		Tally.VoteCount = Pair.Value;
		Tallies.Add(Tally);
	}
	Tallies.Sort([](const FPaldarkBriefingVoteTally& A, const FPaldarkBriefingVoteTally& B)
	{
		if (A.VoteCount != B.VoteCount)
		{
			return A.VoteCount > B.VoteCount;
		}
		return A.MapTag.ToString() < B.MapTag.ToString();
	});
	return Tallies;
}

int32 UPaldarkBriefingSessionComponent::GetReadyCount() const
{
	int32 Count = 0;
	for (const FPaldarkBriefingVoterState& Row : VoterStates)
	{
		if (Row.bIsReady)
		{
			++Count;
		}
	}
	return Count;
}

int32 UPaldarkBriefingSessionComponent::GetVoteCount() const
{
	int32 Count = 0;
	for (const FPaldarkBriefingVoterState& Row : VoterStates)
	{
		if (Row.VotedMapTag.IsValid())
		{
			++Count;
		}
	}
	return Count;
}

FGameplayTag UPaldarkBriefingSessionComponent::GetLeadingMapTag() const
{
	const TArray<FPaldarkBriefingVoteTally> Tallies = GetVoteTallies();
	if (Tallies.Num() == 0)
	{
		return FGameplayTag();
	}
	return Tallies[0].MapTag;
}

// ----------------------------------------------------------------------------
// Server-only threshold evaluation + countdown
// ----------------------------------------------------------------------------

void UPaldarkBriefingSessionComponent::ServerEvaluateThreshold()
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (PhaseTag == PaldarkGameplayTags::TAG_Paldark_Hub_Brief_Phase_Travelling)
	{
		// Don't re-evaluate once we've committed to a travel.
		return;
	}

	const int32 ReadyCount = GetReadyCount();
	const FGameplayTag LeadingMap = GetLeadingMapTag();
	const bool bShouldCountdown = ReadyCount >= MinReadyPlayers && LeadingMap.IsValid();

	if (bShouldCountdown && PhaseTag != PaldarkGameplayTags::TAG_Paldark_Hub_Brief_Phase_Countdown)
	{
		// Transition Idle → Countdown.
		PhaseTag = PaldarkGameplayTags::TAG_Paldark_Hub_Brief_Phase_Countdown;
		CountdownRemaining = CountdownSeconds;
		OnRep_PhaseTag();
		OnRep_CountdownRemaining();

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				CountdownTimerHandle, this,
				&UPaldarkBriefingSessionComponent::ServerTickCountdown,
				1.f, /*bLoop=*/ true);
		}
		UE_LOG(LogPaldarkUI, Log,
			TEXT("[Hub.Brief.Server] Idle → Countdown — LeadingMap=%s, Ready=%d/%d, Countdown=%ds"),
			*LeadingMap.ToString(), ReadyCount, MinReadyPlayers, CountdownRemaining);
	}
	else if (!bShouldCountdown && PhaseTag == PaldarkGameplayTags::TAG_Paldark_Hub_Brief_Phase_Countdown)
	{
		// Transition Countdown → Idle (someone unreadied / unvoted).
		PhaseTag = PaldarkGameplayTags::TAG_Paldark_Hub_Brief_Phase_Idle;
		CountdownRemaining = 0;
		OnRep_PhaseTag();
		OnRep_CountdownRemaining();

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(CountdownTimerHandle);
		}
		UE_LOG(LogPaldarkUI, Log,
			TEXT("[Hub.Brief.Server] Countdown → Idle — threshold dropped (Ready=%d/%d, LeadingMap=%s)"),
			ReadyCount, MinReadyPlayers,
			LeadingMap.IsValid() ? *LeadingMap.ToString() : TEXT("<none>"));
	}
}

void UPaldarkBriefingSessionComponent::ServerTickCountdown()
{
	if (PhaseTag != PaldarkGameplayTags::TAG_Paldark_Hub_Brief_Phase_Countdown)
	{
		return;
	}
	CountdownRemaining = FMath::Max(0, CountdownRemaining - 1);
	OnRep_CountdownRemaining();

	if (CountdownRemaining <= 0)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(CountdownTimerHandle);
		}
		const FGameplayTag LeadingMap = GetLeadingMapTag();
		ServerIssueTravelForMap(LeadingMap, FString());
	}
}

void UPaldarkBriefingSessionComponent::ServerIssueTravelForMap(const FGameplayTag& MapTag, const FString& OverrideMapName)
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority())
	{
		return;
	}
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	// Stamp the pending raid tag on the W40-41 hub subsystem (cheap; designer
	// debug log shows what we actually travelled to).
	if (UPaldarkHubSubsystem* Hub = UPaldarkHubSubsystem::Get(World))
	{
		Hub->SetPendingRaidMapTag(MapTag);
	}

	// Resolve the map name. Caller may pass an override (force-travel path);
	// otherwise derive from the W31-32 MapDefinition (preferred), then fall
	// back to "Map_<tag leaf>" so designer test tags work.
	FString MapName = OverrideMapName;
	if (MapName.IsEmpty() && MapTag.IsValid())
	{
		const FString TagStr = MapTag.ToString();
		int32 LastDot = INDEX_NONE;
		if (TagStr.FindLastChar(TEXT('.'), LastDot) && LastDot < TagStr.Len() - 1)
		{
			MapName = FString::Printf(TEXT("Map_%s"), *TagStr.RightChop(LastDot + 1));
		}
	}

	if (MapName.IsEmpty())
	{
		UE_LOG(LogPaldarkUI, Warning,
			TEXT("[Hub.Brief.Server] ServerIssueTravelForMap — could not resolve a map name from tag %s. Aborting travel."),
			*MapTag.ToString());
		return;
	}

	PhaseTag = PaldarkGameplayTags::TAG_Paldark_Hub_Brief_Phase_Travelling;
	OnRep_PhaseTag();

	UE_LOG(LogPaldarkUI, Log,
		TEXT("[Hub.Brief.Server] ServerIssueTravelForMap — MapTag=%s MapName=%s; issuing HostHubServer."),
		*MapTag.ToString(),
		*MapName);

	UGameInstance* GI = World->GetGameInstance();
	UPaldarkNetSubsystem* Net = GI != nullptr ? GI->GetSubsystem<UPaldarkNetSubsystem>() : nullptr;
	if (Net == nullptr)
	{
		UE_LOG(LogPaldarkUI, Warning,
			TEXT("[Hub.Brief.Server] ServerIssueTravelForMap — UPaldarkNetSubsystem not available; aborting."));
		return;
	}
	// HostHubServer issues ServerTravel on the auth GameMode — the entire
	// hub shard hops together to the raid map.
	Net->HostHubServer(MapName, FString());
}

void UPaldarkBriefingSessionComponent::DumpToLog() const
{
	UE_LOG(LogPaldarkUI, Log,
		TEXT("[Hub.Brief.Dump] Phase=%s Countdown=%ds Voters=%d Votes=%d Ready=%d/%d"),
		*PhaseTag.ToString(),
		CountdownRemaining,
		VoterStates.Num(),
		GetVoteCount(),
		GetReadyCount(),
		MinReadyPlayers);
	const TArray<FPaldarkBriefingVoteTally> Tallies = GetVoteTallies();
	for (int32 Idx = 0; Idx < Tallies.Num(); ++Idx)
	{
		UE_LOG(LogPaldarkUI, Log,
			TEXT("  [%d] %s — %d vote(s)"),
			Idx,
			*Tallies[Idx].MapTag.ToString(),
			Tallies[Idx].VoteCount);
	}
	for (int32 Idx = 0; Idx < VoterStates.Num(); ++Idx)
	{
		const FPaldarkBriefingVoterState& Row = VoterStates[Idx];
		UE_LOG(LogPaldarkUI, Log,
			TEXT("  voter[%d] PS=%s Voted=%s Ready=%s"),
			Idx,
			Row.PlayerState.IsValid() ? *Row.PlayerState->GetName() : TEXT("<gone>"),
			Row.VotedMapTag.IsValid() ? *Row.VotedMapTag.ToString() : TEXT("<none>"),
			Row.bIsReady ? TEXT("Y") : TEXT("N"));
	}
}

// ----------------------------------------------------------------------------
// OnRep handlers
// ----------------------------------------------------------------------------

void UPaldarkBriefingSessionComponent::OnRep_PhaseTag()
{
	UE_LOG(LogPaldarkUI, Log,
		TEXT("[Hub.Brief.Rep] PhaseTag → %s"),
		*PhaseTag.ToString());
	OnSessionChanged.Broadcast();
}

void UPaldarkBriefingSessionComponent::OnRep_VoterStates()
{
	UE_LOG(LogPaldarkUI, VeryVerbose,
		TEXT("[Hub.Brief.Rep] VoterStates → %d entries (Votes=%d, Ready=%d)"),
		VoterStates.Num(), GetVoteCount(), GetReadyCount());
	OnSessionChanged.Broadcast();
}

void UPaldarkBriefingSessionComponent::OnRep_CountdownRemaining()
{
	UE_LOG(LogPaldarkUI, VeryVerbose,
		TEXT("[Hub.Brief.Rep] CountdownRemaining → %d"),
		CountdownRemaining);
	OnSessionChanged.Broadcast();
}
