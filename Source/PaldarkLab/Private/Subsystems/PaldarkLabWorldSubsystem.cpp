// PALDARK W1 — Implementation of UPaldarkLabWorldSubsystem. Demonstrates:
//   * Initialize / Deinitialize hooks.
//   * ShouldCreateSubsystem filter (skip for editor preview worlds).
//   * Tickable subsystem at 1 Hz reporting.
//   * Stable registration API consumed by APaldarkLabSampleActor.

#include "Subsystems/PaldarkLabWorldSubsystem.h"

#include "Actors/PaldarkLabSampleActor.h"
#include "Engine/World.h"
#include "PaldarkLab.h"

namespace
{
	constexpr float ReportIntervalSeconds = 1.0f;
}

bool UPaldarkLabWorldSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	if (World == nullptr)
	{
		return false;
	}

	// Only spin up for live worlds — skip editor preview, inactive, asset preview, etc.
	const EWorldType::Type Type = World->WorldType;
	return Type == EWorldType::Game
		|| Type == EWorldType::PIE
		|| Type == EWorldType::GamePreview
		|| Type == EWorldType::Editor;
}

void UPaldarkLabWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	TrackedActors.Reset();
	SecondsSinceLastReport = 0.0f;

	UE_LOG(LogPaldark, Log, TEXT("UPaldarkLabWorldSubsystem initialized for world %s."),
		GetWorld() ? *GetWorld()->GetName() : TEXT("<none>"));
}

void UPaldarkLabWorldSubsystem::Deinitialize()
{
	UE_LOG(LogPaldark, Log, TEXT("UPaldarkLabWorldSubsystem deinitialized (was tracking %d actor(s))."),
		TrackedActors.Num());

	TrackedActors.Reset();

	Super::Deinitialize();
}

bool UPaldarkLabWorldSubsystem::IsTickable() const
{
	const UWorld* World = GetWorld();
	return World != nullptr && World->HasBegunPlay();
}

TStatId UPaldarkLabWorldSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UPaldarkLabWorldSubsystem, STATGROUP_Tickables);
}

void UPaldarkLabWorldSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SecondsSinceLastReport += DeltaTime;
	if (SecondsSinceLastReport < ReportIntervalSeconds)
	{
		return;
	}
	SecondsSinceLastReport = 0.0f;

	// Compact stale entries — EndPlay typically removes them but bullet-proof against
	// streamed-out levels just in case.
	TrackedActors.RemoveAll([](const TObjectPtr<APaldarkLabSampleActor>& Entry)
	{
		return Entry == nullptr || !IsValid(Entry.Get());
	});

	UE_LOG(LogPaldark, Verbose, TEXT("Tracking %d SampleActor(s)."), TrackedActors.Num());
}

void UPaldarkLabWorldSubsystem::RegisterSampleActor(APaldarkLabSampleActor* Actor)
{
	if (Actor == nullptr)
	{
		return;
	}

	TrackedActors.AddUnique(Actor);
	UE_LOG(LogPaldark, Verbose, TEXT("Registered SampleActor %s (total %d)."),
		*Actor->GetName(), TrackedActors.Num());
}

void UPaldarkLabWorldSubsystem::UnregisterSampleActor(APaldarkLabSampleActor* Actor)
{
	if (Actor == nullptr)
	{
		return;
	}

	const int32 Removed = TrackedActors.Remove(Actor);
	if (Removed > 0)
	{
		UE_LOG(LogPaldark, Verbose, TEXT("Unregistered SampleActor %s (remaining %d)."),
			*Actor->GetName(), TrackedActors.Num());
	}
}
