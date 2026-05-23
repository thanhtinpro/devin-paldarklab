// PALDARK W1 — Sample actor implementation. Spins around Z and bobs vertically. The
// goal is to exercise component composition, replication, and subsystem registration
// — not to be a useful gameplay actor.

#include "Actors/PaldarkLabSampleActor.h"

#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "PaldarkLab.h"
#include "Subsystems/PaldarkLabWorldSubsystem.h"

APaldarkLabSampleActor::APaldarkLabSampleActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
	MeshComponent->SetMobility(EComponentMobility::Movable);
	MeshComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
}

void APaldarkLabSampleActor::BeginPlay()
{
	Super::BeginPlay();

	InitialLocation = GetActorLocation();
	ElapsedSeconds = 0.0f;

	if (UPaldarkLabWorldSubsystem* Subsystem = UWorld::GetSubsystem<UPaldarkLabWorldSubsystem>(GetWorld()))
	{
		Subsystem->RegisterSampleActor(this);
	}

	UE_LOG(LogPaldark, Verbose, TEXT("SampleActor %s BeginPlay at %s"),
		*GetName(), *InitialLocation.ToCompactString());
}

void APaldarkLabSampleActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UPaldarkLabWorldSubsystem* Subsystem = UWorld::GetSubsystem<UPaldarkLabWorldSubsystem>(GetWorld()))
	{
		Subsystem->UnregisterSampleActor(this);
	}

	Super::EndPlay(EndPlayReason);
}

void APaldarkLabSampleActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ElapsedSeconds += DeltaSeconds;

	FRotator NewRotation = GetActorRotation();
	NewRotation.Yaw += RotationSpeedDegPerSec * DeltaSeconds;

	const float BobOffset = BobAmplitudeCm * FMath::Sin(ElapsedSeconds * BobFrequencyHz * 2.0f * PI);
	const FVector NewLocation = InitialLocation + FVector(0.0f, 0.0f, BobOffset);

	SetActorLocationAndRotation(NewLocation, NewRotation);
}

void APaldarkLabSampleActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APaldarkLabSampleActor, RotationSpeedDegPerSec);
}
