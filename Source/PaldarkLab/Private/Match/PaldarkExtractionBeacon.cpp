#include "Match/PaldarkExtractionBeacon.h"

#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

#include "Match/PaldarkMatchSubsystem.h"
#include "Match/PaldarkMatchTypes.h"
#include "PaldarkLogCategories.h"
#include "Player/PaldarkCharacter.h"
#include "Player/PaldarkPlayerState.h"

APaldarkExtractionBeacon::APaldarkExtractionBeacon()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	NetUpdateFrequency = 5.f;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	ExtractionTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("ExtractionTrigger"));
	RootComponent = ExtractionTrigger;
	ExtractionTrigger->InitSphereRadius(TriggerRadius);
	ExtractionTrigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	ExtractionTrigger->SetGenerateOverlapEvents(true);
}

void APaldarkExtractionBeacon::BeginPlay()
{
	Super::BeginPlay();

	// Re-apply designer radius (constructor value may have been overridden
	// in Details panel between CDO creation and PIE start).
	if (ExtractionTrigger != nullptr)
	{
		ExtractionTrigger->SetSphereRadius(TriggerRadius);
	}

	if (HasAuthority() && ExtractionTrigger != nullptr)
	{
		ExtractionTrigger->OnComponentBeginOverlap.AddDynamic(
			this, &APaldarkExtractionBeacon::OnSphereBeginOverlap);
		ExtractionTrigger->OnComponentEndOverlap.AddDynamic(
			this, &APaldarkExtractionBeacon::OnSphereEndOverlap);
	}

	UE_LOG(LogPaldarkPlayer, Log,
		TEXT("APaldarkExtractionBeacon::BeginPlay — %s radius=%.1f duration=%.1f consent=%d decay=%d"),
		*GetName(), TriggerRadius, ExtractionDuration, bRequireExplicitConsent ? 1 : 0, bDecayOnLeave ? 1 : 0);
}

void APaldarkExtractionBeacon::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority() && ExtractionTrigger != nullptr)
	{
		ExtractionTrigger->OnComponentBeginOverlap.RemoveAll(this);
		ExtractionTrigger->OnComponentEndOverlap.RemoveAll(this);
	}
	OverlappingPlayerStates.Reset();
	ProgressByPlayer.Reset();
	Super::EndPlay(EndPlayReason);
}

void APaldarkExtractionBeacon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority())
	{
		return;
	}
	if (OverlappingPlayerStates.Num() == 0 && ProgressByPlayer.Num() == 0)
	{
		return;
	}

	UPaldarkMatchSubsystem* MatchSub = GetMatchSubsystem();
	const EPaldarkMatchPhase Phase = MatchSub != nullptr ? MatchSub->GetPhase() : EPaldarkMatchPhase::Warmup;
	const bool bPhaseAccepts =
		(Phase == EPaldarkMatchPhase::Active || Phase == EPaldarkMatchPhase::Extracting);

	const float Now = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.f;

	// Advance per-player progress.
	TArray<TObjectPtr<APlayerState>> ToRemove;
	for (TPair<TObjectPtr<APlayerState>, FProgressEntry>& Pair : ProgressByPlayer)
	{
		APlayerState* PS = Pair.Key;
		FProgressEntry& Entry = Pair.Value;
		APaldarkPlayerState* PaldarkPS = Cast<APaldarkPlayerState>(PS);

		if (PaldarkPS == nullptr)
		{
			ToRemove.Add(PS);
			continue;
		}
		if (MatchSub != nullptr && MatchSub->GetPlayerOutcome(PS) != EPaldarkPlayerOutcome::Alive)
		{
			// Player terminal (dead or already extracted). Stop tracking.
			ToRemove.Add(PS);
			PaldarkPS->SetExtractionProgress(0.f);
			continue;
		}

		const bool bStillOverlapping = OverlappingPlayerStates.Contains(PS);
		const bool bConsentOk = bRequireExplicitConsent ? Entry.bConsented : true;
		const bool bRamping = bStillOverlapping && bConsentOk && bPhaseAccepts;

		if (bRamping)
		{
			Entry.ProgressSeconds = FMath::Clamp(
				Entry.ProgressSeconds + DeltaSeconds,
				0.f,
				ExtractionDuration);
		}
		else if (bDecayOnLeave && !bStillOverlapping)
		{
			Entry.ProgressSeconds = FMath::Max(0.f, Entry.ProgressSeconds - DeltaSeconds);
		}
		// Else: progress freezes at its current value.

		// Throttled wire update of the replicated PlayerState progress.
		if (Now - Entry.LastWireUpdateTime >= WireUpdateInterval)
		{
			const float Normalized = ExtractionDuration > KINDA_SMALL_NUMBER
				? Entry.ProgressSeconds / ExtractionDuration
				: 0.f;
			PaldarkPS->SetExtractionProgress(Normalized);
			Entry.LastWireUpdateTime = Now;
		}

		// Completion gate.
		if (Entry.ProgressSeconds >= ExtractionDuration)
		{
			UE_LOG(LogPaldarkPlayer, Log,
				TEXT("APaldarkExtractionBeacon::Tick — %s completed extraction on %s."),
				*PaldarkPS->GetName(), *GetName());
			PaldarkPS->SetExtractionProgress(1.f);
			if (MatchSub != nullptr)
			{
				MatchSub->RecordExtraction(PaldarkPS);
			}
			ToRemove.Add(PS);
		}
		else if (!bStillOverlapping && !bDecayOnLeave && Entry.ProgressSeconds <= KINDA_SMALL_NUMBER)
		{
			// Empty + not overlapping + no decay = drop the row to free RAM.
			ToRemove.Add(PS);
		}
	}

	for (const TObjectPtr<APlayerState>& Key : ToRemove)
	{
		ProgressByPlayer.Remove(Key);
	}
}

void APaldarkExtractionBeacon::OnSphereBeginOverlap(
	UPrimitiveComponent* /*OverlappedComp*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	if (!HasAuthority())
	{
		return;
	}
	APaldarkCharacter* Char = Cast<APaldarkCharacter>(OtherActor);
	if (Char == nullptr)
	{
		return;
	}
	APaldarkPlayerState* PS = Char->GetPlayerState<APaldarkPlayerState>();
	if (PS == nullptr)
	{
		return;
	}
	if (!ShouldAcceptOverlap(PS))
	{
		return;
	}

	OverlappingPlayerStates.Add(PS);
	FProgressEntry& Entry = ProgressByPlayer.FindOrAdd(PS);
	Entry.PlayerState = PS;
	// Don't reset ProgressSeconds — a player who left + re-entered without
	// decay should pick up where they left off (designer-friendly).

	UE_LOG(LogPaldarkPlayer, Log,
		TEXT("APaldarkExtractionBeacon::OnSphereBeginOverlap — %s entered %s (progress=%.2f/%.2f)."),
		*PS->GetName(), *GetName(), Entry.ProgressSeconds, ExtractionDuration);
}

void APaldarkExtractionBeacon::OnSphereEndOverlap(
	UPrimitiveComponent* /*OverlappedComp*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/)
{
	if (!HasAuthority())
	{
		return;
	}
	APaldarkCharacter* Char = Cast<APaldarkCharacter>(OtherActor);
	if (Char == nullptr)
	{
		return;
	}
	APaldarkPlayerState* PS = Char->GetPlayerState<APaldarkPlayerState>();
	if (PS == nullptr)
	{
		return;
	}
	OverlappingPlayerStates.Remove(PS);

	UE_LOG(LogPaldarkPlayer, Log,
		TEXT("APaldarkExtractionBeacon::OnSphereEndOverlap — %s left %s."),
		*PS->GetName(), *GetName());
}

void APaldarkExtractionBeacon::RegisterConsentToExtract(APlayerState* PlayerState)
{
	if (!HasAuthority() || PlayerState == nullptr)
	{
		return;
	}
	FProgressEntry* Entry = ProgressByPlayer.Find(PlayerState);
	if (Entry == nullptr)
	{
		// Player not currently overlapping — consent has no effect.
		return;
	}
	Entry->bConsented = true;
	UE_LOG(LogPaldarkPlayer, Log,
		TEXT("APaldarkExtractionBeacon::RegisterConsentToExtract — %s consented on %s."),
		*PlayerState->GetName(), *GetName());
}

UPaldarkMatchSubsystem* APaldarkExtractionBeacon::GetMatchSubsystem() const
{
	UWorld* World = GetWorld();
	return World != nullptr ? World->GetSubsystem<UPaldarkMatchSubsystem>() : nullptr;
}

bool APaldarkExtractionBeacon::ShouldAcceptOverlap(APaldarkPlayerState* PlayerState) const
{
	if (PlayerState == nullptr)
	{
		return false;
	}
	UPaldarkMatchSubsystem* MatchSub = GetMatchSubsystem();
	if (MatchSub == nullptr)
	{
		// No match subsystem (rare — e.g. PIE without proper experience).
		// Accept overlap; the Tick guard will keep things safe.
		return true;
	}
	const EPaldarkMatchPhase Phase = MatchSub->GetPhase();
	if (Phase == EPaldarkMatchPhase::Warmup || Phase == EPaldarkMatchPhase::Ended)
	{
		return false;
	}
	if (MatchSub->GetPlayerOutcome(PlayerState) != EPaldarkPlayerOutcome::Alive)
	{
		return false;
	}
	return true;
}
