// PALDARK W22-23 — Ping subsystem implementation.

#include "Squad/PaldarkPingSubsystem.h"

#include "EngineUtils.h"
#include "Engine/World.h"
#include "PaldarkLogCategories.h"
#include "Player/PaldarkCharacter.h"
#include "Squad/PaldarkPingMarker.h"

UPaldarkPingSubsystem::UPaldarkPingSubsystem()
{
	MarkerClass = APaldarkPingMarker::StaticClass();
}

bool UPaldarkPingSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}

	// Same host-only gate as the squad + pack subsystems.
	const UWorld* OuterWorld = Cast<UWorld>(Outer);
	if (OuterWorld == nullptr)
	{
		return false;
	}

	const ENetMode NetMode = OuterWorld->GetNetMode();
	return NetMode == NM_Standalone || NetMode == NM_DedicatedServer || NetMode == NM_ListenServer;
}

void UPaldarkPingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogPaldarkPlayer, Verbose, TEXT("[Ping] Subsystem initialised (host)."));
}

void UPaldarkPingSubsystem::Deinitialize()
{
	LastPingTimePerPlayer.Reset();
	UE_LOG(LogPaldarkPlayer, Verbose, TEXT("[Ping] Subsystem deinitialised."));
	Super::Deinitialize();
}

APaldarkPingMarker* UPaldarkPingSubsystem::SpawnPing(
	APaldarkCharacter* Issuer,
	const FVector& WorldLocation,
	AActor* MarkedActor,
	const FGameplayTag& PingType)
{
	if (Issuer == nullptr)
	{
		UE_LOG(LogPaldarkPlayer, Warning, TEXT("[Ping] SpawnPing: null issuer."));
		return nullptr;
	}

	if (MarkerClass == nullptr)
	{
		UE_LOG(LogPaldarkPlayer, Warning, TEXT("[Ping] SpawnPing: MarkerClass unset."));
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (World == nullptr || World->GetNetMode() == NM_Client)
	{
		UE_LOG(LogPaldarkPlayer, Warning,
			TEXT("[Ping] SpawnPing: invalid world or running on client."));
		return nullptr;
	}

	const float NowSec = World->GetTimeSeconds();
	TWeakObjectPtr<APaldarkCharacter> IssuerWeak = Issuer;
	if (const float* LastTime = LastPingTimePerPlayer.Find(IssuerWeak))
	{
		if (NowSec - *LastTime < MinSecondsBetweenPings)
		{
			UE_LOG(LogPaldarkPlayer, Verbose,
				TEXT("[Ping] SpawnPing: %s rate-limited (delta=%.2fs < min=%.2fs)."),
				*Issuer->GetName(), NowSec - *LastTime, MinSecondsBetweenPings);
			return nullptr;
		}
	}

	const FVector SpawnLocation = (MarkedActor != nullptr)
		? MarkedActor->GetActorLocation()
		: WorldLocation;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = Issuer;
	Params.Instigator = Issuer;

	APaldarkPingMarker* Marker = World->SpawnActor<APaldarkPingMarker>(
		MarkerClass, SpawnLocation, FRotator::ZeroRotator, Params);
	if (Marker == nullptr)
	{
		UE_LOG(LogPaldarkPlayer, Warning, TEXT("[Ping] SpawnPing: SpawnActor failed."));
		return nullptr;
	}

	Marker->InitMarker(PingType, Issuer, MarkedActor, DefaultPingLifetime);
	LastPingTimePerPlayer.Add(IssuerWeak, NowSec);

	UE_LOG(LogPaldarkPlayer, Log,
		TEXT("[Ping] %s fired %s ping at %s (marked=%s, lifetime=%.1fs)."),
		*Issuer->GetName(),
		*PingType.ToString(),
		*SpawnLocation.ToCompactString(),
		MarkedActor != nullptr ? *MarkedActor->GetName() : TEXT("<world>"),
		DefaultPingLifetime);

	return Marker;
}

int32 UPaldarkPingSubsystem::GetActivePingCount() const
{
	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return 0;
	}

	int32 Count = 0;
	for (TActorIterator<APaldarkPingMarker> It(const_cast<UWorld*>(World)); It; ++It)
	{
		if (IsValid(*It))
		{
			++Count;
		}
	}
	return Count;
}
