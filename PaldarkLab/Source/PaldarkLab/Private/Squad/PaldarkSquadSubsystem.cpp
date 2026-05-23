// PALDARK W22-23 — Squad subsystem implementation.

#include "Squad/PaldarkSquadSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"
#include "Player/PaldarkCharacter.h"

UPaldarkSquadSubsystem::UPaldarkSquadSubsystem() = default;

bool UPaldarkSquadSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}

	// Mirror the W20-21 pack subsystem — only host (server / standalone) needs
	// the registry. Clients get nothing; squad membership is server-auth.
	const UWorld* OuterWorld = Cast<UWorld>(Outer);
	if (OuterWorld == nullptr)
	{
		return false;
	}

	const ENetMode NetMode = OuterWorld->GetNetMode();
	return NetMode == NM_Standalone || NetMode == NM_DedicatedServer || NetMode == NM_ListenServer;
}

void UPaldarkSquadSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogPaldarkPlayer, Verbose, TEXT("[Squad] Subsystem initialised (host)."));
}

void UPaldarkSquadSubsystem::Deinitialize()
{
	SquadRegistry.Reset();
	StandingCommandPerSquad.Reset();
	StandingCommandLocationPerSquad.Reset();
	OnSquadCommandIssued.Clear();
	UE_LOG(LogPaldarkPlayer, Verbose, TEXT("[Squad] Subsystem deinitialised."));
	Super::Deinitialize();
}

void UPaldarkSquadSubsystem::RegisterPlayerToSquad(
	APaldarkCharacter* InPlayer,
	const FGameplayTag& InSquadTag)
{
	if (InPlayer == nullptr)
	{
		UE_LOG(LogPaldarkPlayer, Warning, TEXT("[Squad] Register: null player ignored."));
		return;
	}

	const FGameplayTag ResolvedTag = InSquadTag.IsValid()
		? InSquadTag
		: PaldarkGameplayTags::TAG_Paldark_Squad_Default;

	FPaldarkSquadRoster& Roster = SquadRegistry.FindOrAdd(ResolvedTag);

	// Idempotent — collapse double-registration. Walk and skip if already
	// present to keep insertion order stable for debug dump.
	for (const TWeakObjectPtr<APaldarkCharacter>& Existing : Roster.Members)
	{
		if (Existing.Get() == InPlayer)
		{
			return;
		}
	}

	Roster.Members.Add(InPlayer);
	UE_LOG(LogPaldarkPlayer, Verbose,
		TEXT("[Squad] Registered %s into squad %s (roster=%d)."),
		*InPlayer->GetName(), *ResolvedTag.ToString(), Roster.Members.Num());
}

void UPaldarkSquadSubsystem::UnregisterPlayerFromSquad(APaldarkCharacter* InPlayer)
{
	if (InPlayer == nullptr)
	{
		return;
	}

	// Walk every roster (small N — squad count is single-digit even in
	// PvP) and strip the player. Cheap given the call frequency (EndPlay).
	for (auto It = SquadRegistry.CreateIterator(); It; ++It)
	{
		FPaldarkSquadRoster& Roster = It.Value();
		const int32 Removed = Roster.Members.RemoveAll(
			[InPlayer](const TWeakObjectPtr<APaldarkCharacter>& Member)
			{
				return Member.Get() == InPlayer || !Member.IsValid();
			});

		if (Removed > 0)
		{
			UE_LOG(LogPaldarkPlayer, Verbose,
				TEXT("[Squad] Unregistered %s from squad %s (roster=%d)."),
				*InPlayer->GetName(), *It.Key().ToString(), Roster.Members.Num());
		}

		// Clean up empty rosters so DumpToLog stays terse.
		if (Roster.Members.Num() == 0)
		{
			It.RemoveCurrent();
		}
	}
}

void UPaldarkSquadSubsystem::BroadcastSquadCommand(
	APaldarkCharacter* Issuer,
	const FGameplayTag& CommandTag,
	const FVector& OptionalLocation)
{
	if (Issuer == nullptr || !CommandTag.IsValid())
	{
		UE_LOG(LogPaldarkPlayer, Warning,
			TEXT("[Squad] BroadcastSquadCommand: null issuer or invalid tag — ignored."));
		return;
	}

	const FGameplayTag SquadTag = GetSquadTagForPlayer(Issuer);
	if (!SquadTag.IsValid())
	{
		UE_LOG(LogPaldarkPlayer, Warning,
			TEXT("[Squad] BroadcastSquadCommand: %s not in any squad — ignored."),
			*Issuer->GetName());
		return;
	}

	// Cache the standing command for late-joining Pal listeners.
	StandingCommandPerSquad.Add(SquadTag, CommandTag);
	StandingCommandLocationPerSquad.Add(SquadTag, OptionalLocation);

	UE_LOG(LogPaldarkPlayer, Log,
		TEXT("[Squad] %s (squad=%s) issued command %s at %s."),
		*Issuer->GetName(),
		*SquadTag.ToString(),
		*CommandTag.ToString(),
		*OptionalLocation.ToCompactString());

	OnSquadCommandIssued.Broadcast(Issuer, CommandTag, OptionalLocation);
}

int32 UPaldarkSquadSubsystem::GetSquadSize(const FGameplayTag& InSquadTag) const
{
	const FPaldarkSquadRoster* Roster = SquadRegistry.Find(InSquadTag);
	if (Roster == nullptr)
	{
		return 0;
	}

	int32 LiveCount = 0;
	for (const TWeakObjectPtr<APaldarkCharacter>& Member : Roster->Members)
	{
		if (Member.IsValid())
		{
			++LiveCount;
		}
	}
	return LiveCount;
}

FGameplayTag UPaldarkSquadSubsystem::GetSquadTagForPlayer(const APaldarkCharacter* InPlayer) const
{
	if (InPlayer == nullptr)
	{
		return FGameplayTag::EmptyTag;
	}

	for (const TPair<FGameplayTag, FPaldarkSquadRoster>& Pair : SquadRegistry)
	{
		for (const TWeakObjectPtr<APaldarkCharacter>& Member : Pair.Value.Members)
		{
			if (Member.Get() == InPlayer)
			{
				return Pair.Key;
			}
		}
	}
	return FGameplayTag::EmptyTag;
}

void UPaldarkSquadSubsystem::GetSquadMembersForPlayer(
	const APaldarkCharacter* InPlayer,
	TArray<APaldarkCharacter*>& OutMembers) const
{
	OutMembers.Reset();

	const FGameplayTag SquadTag = GetSquadTagForPlayer(InPlayer);
	if (!SquadTag.IsValid())
	{
		return;
	}

	const FPaldarkSquadRoster* Roster = SquadRegistry.Find(SquadTag);
	if (Roster == nullptr)
	{
		return;
	}

	OutMembers.Reserve(Roster->Members.Num());
	for (const TWeakObjectPtr<APaldarkCharacter>& Member : Roster->Members)
	{
		if (APaldarkCharacter* Live = Member.Get())
		{
			OutMembers.Add(Live);
		}
	}
}

void UPaldarkSquadSubsystem::DumpToLog() const
{
	UE_LOG(LogPaldarkPlayer, Log, TEXT("[Squad] Dump (host) — %d squad(s) registered."),
		SquadRegistry.Num());

	for (const TPair<FGameplayTag, FPaldarkSquadRoster>& Pair : SquadRegistry)
	{
		UE_LOG(LogPaldarkPlayer, Log,
			TEXT("[Squad]   squad=%s members=%d standing=%s loc=%s"),
			*Pair.Key.ToString(),
			Pair.Value.Members.Num(),
			*GetStandingCommand(Pair.Key).ToString(),
			*GetStandingCommandLocation(Pair.Key).ToCompactString());

		for (const TWeakObjectPtr<APaldarkCharacter>& Member : Pair.Value.Members)
		{
			if (const APaldarkCharacter* Live = Member.Get())
			{
				UE_LOG(LogPaldarkPlayer, Log,
					TEXT("[Squad]     - %s @ %s"),
					*Live->GetName(), *Live->GetActorLocation().ToCompactString());
			}
			else
			{
				UE_LOG(LogPaldarkPlayer, Log, TEXT("[Squad]     - <stale weak ref>"));
			}
		}
	}
}

FGameplayTag UPaldarkSquadSubsystem::GetStandingCommand(const FGameplayTag& InSquadTag) const
{
	if (const FGameplayTag* Found = StandingCommandPerSquad.Find(InSquadTag))
	{
		return *Found;
	}
	return FGameplayTag::EmptyTag;
}

FVector UPaldarkSquadSubsystem::GetStandingCommandLocation(const FGameplayTag& InSquadTag) const
{
	if (const FVector* Found = StandingCommandLocationPerSquad.Find(InSquadTag))
	{
		return *Found;
	}
	return FVector::ZeroVector;
}
