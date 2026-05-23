// PALDARK W22-23 — Ping marker implementation.

#include "Squad/PaldarkPingMarker.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "PaldarkLogCategories.h"
#include "Player/PaldarkCharacter.h"

APaldarkPingMarker::APaldarkPingMarker()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;
	bReplicates = true;
	SetReplicateMovement(false);
	bAlwaysRelevant = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

void APaldarkPingMarker::BeginPlay()
{
	Super::BeginPlay();
}

void APaldarkPingMarker::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APaldarkPingMarker, PingType);
	DOREPLIFETIME(APaldarkPingMarker, OwningPlayer);
	DOREPLIFETIME(APaldarkPingMarker, MarkedActor);
	DOREPLIFETIME(APaldarkPingMarker, ExpirationServerTime);
}

void APaldarkPingMarker::InitMarker(
	const FGameplayTag& InPingType,
	APaldarkCharacter* InOwningPlayer,
	AActor* InMarkedActor,
	float InLifetime)
{
	if (!HasAuthority())
	{
		UE_LOG(LogPaldarkPlayer, Warning,
			TEXT("[PingMarker] InitMarker called on non-authority — ignored."));
		return;
	}

	PingType = InPingType;
	OwningPlayer = InOwningPlayer;
	MarkedActor = InMarkedActor;

	const UWorld* World = GetWorld();
	const float NowSec = World != nullptr ? World->GetTimeSeconds() : 0.f;
	ExpirationServerTime = NowSec + FMath::Max(InLifetime, 0.1f);

	// Snap to MarkedActor on spawn if provided; otherwise rely on the
	// caller's SetActorLocation prior to InitMarker.
	if (AActor* Live = MarkedActor.Get())
	{
		SetActorLocation(Live->GetActorLocation());
	}
}

void APaldarkPingMarker::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Server-only lifetime management + Enemy-ping re-snap.
	if (!HasAuthority())
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const float NowSec = World->GetTimeSeconds();
	if (NowSec >= ExpirationServerTime)
	{
		Destroy();
		return;
	}

	if (AActor* Live = MarkedActor.Get())
	{
		SetActorLocation(Live->GetActorLocation());
	}
}

float APaldarkPingMarker::GetRemainingLifetime() const
{
	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return 0.f;
	}
	return FMath::Max(0.f, ExpirationServerTime - World->GetTimeSeconds());
}
