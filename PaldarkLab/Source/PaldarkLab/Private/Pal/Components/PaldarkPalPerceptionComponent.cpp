#include "Pal/Components/PaldarkPalPerceptionComponent.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

#include "Combat/PaldarkDummyTarget.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"
#include "Pal/PaldarkPalCharacter.h"
#include "Player/PaldarkCharacter.h"

UPaldarkPalPerceptionComponent::UPaldarkPalPerceptionComponent()
{
	// Tick on authority only — the scan + hysteresis logic produces no
	// meaningful client output; SetIsReplicated stays false so a stray
	// client-side iteration cannot cause divergence.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false; // Enabled in BeginPlay on authority.
	SetIsReplicatedByDefault(false);

	// Defaults match the W18-19 sandbox; designers override per Pal species
	// via the Blueprint subclass.
	FriendlyTeamTag = PaldarkGameplayTags::TAG_Paldark_Team_Player;
	HostileTeamTag  = PaldarkGameplayTags::TAG_Paldark_Team_Hostile;
}

void UPaldarkPalPerceptionComponent::BeginPlay()
{
	Super::BeginPlay();

	PalOwner = Cast<APaldarkPalCharacter>(GetOwner());

	if (GetOwner() != nullptr && GetOwner()->HasAuthority())
	{
		SetComponentTickEnabled(true);
		TimeUntilNextScan = ScanInterval;

		UE_LOG(LogPaldarkPal, Log,
			TEXT("UPaldarkPalPerceptionComponent::BeginPlay — authority. Pal=%s radius=%.1f scan=%.2fs hysteresis=%.1f grace=%.2fs"),
			*GetNameSafe(GetOwner()),
			ThreatRadius,
			ScanInterval,
			AggroSwitchHysteresisCm,
			ThreatGracePeriodSeconds);
	}
	else
	{
		// Client-side: stay quiet, no tick. Clients render replicated
		// character movement only, so they get nothing useful from a local
		// scan.
		SetComponentTickEnabled(false);
	}
}

void UPaldarkPalPerceptionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CurrentThreat.Reset();
	CurrentThreatDistance = 0.f;
	ThreatGraceRemaining = 0.f;
	Super::EndPlay(EndPlayReason);
}

void UPaldarkPalPerceptionComponent::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);

	if (GetOwner() == nullptr || !GetOwner()->HasAuthority())
	{
		// Belt-and-braces: BeginPlay already gates tick by authority, but a
		// listen-server host that ServerTravels can flip authority underneath
		// us — re-checking here costs nothing.
		return;
	}

	// Grace timer runs every frame so the threat clear lands on the exact
	// frame the grace expires, not on the next scan tick. Only counts down
	// when CurrentThreat exists and is no longer threatening (despawned /
	// dead / out of range when next scanned).
	if (CurrentThreat.IsValid() && !IsActorThreatening(CurrentThreat.Get()))
	{
		ThreatGraceRemaining -= DeltaSeconds;
		if (ThreatGraceRemaining <= 0.f)
		{
			AActor* Old = CurrentThreat.Get();
			CurrentThreat.Reset();
			CurrentThreatDistance = 0.f;
			OnThreatChanged.Broadcast(Old, nullptr);
		}
	}

	TimeUntilNextScan -= DeltaSeconds;
	if (TimeUntilNextScan > 0.f)
	{
		return;
	}
	TimeUntilNextScan = ScanInterval;
	RunScan();
}

void UPaldarkPalPerceptionComponent::RunScan()
{
	const APaldarkPalCharacter* Pal = PalOwner.Get();
	UWorld* World = GetWorld();
	if (Pal == nullptr || World == nullptr)
	{
		return;
	}

	const FVector PalLocation = Pal->GetActorLocation();
	const float   RadiusSq    = ThreatRadius * ThreatRadius;

	AActor* BestCandidate = nullptr;
	float   BestDistSq    = TNumericLimits<float>::Max();

	// W18-19 sandbox scale (a handful of pawns + dummies). Iterator filter
	// stays in this single scope so future weeks can swap to a registry
	// subsystem (W48-49) without touching every consumer.
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Candidate = *It;
		if (Candidate == nullptr || Candidate == Pal)
		{
			continue;
		}
		if (!IsActorThreatening(Candidate))
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Candidate->GetActorLocation(), PalLocation);
		if (DistSq > RadiusSq)
		{
			continue;
		}
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestCandidate = Candidate;
		}
	}

	AActor* const OldThreat = CurrentThreat.Get();

	if (BestCandidate == nullptr)
	{
		// No candidate in range. If we already have a threat, arm the
		// grace timer; tick handles the eventual broadcast when the
		// timer expires.
		if (CurrentThreat.IsValid())
		{
			ThreatGraceRemaining = ThreatGracePeriodSeconds;
		}
		return;
	}

	const float BestDistance = FMath::Sqrt(BestDistSq);

	if (!CurrentThreat.IsValid())
	{
		// First acquisition — adopt immediately.
		CurrentThreat = BestCandidate;
		CurrentThreatDistance = BestDistance;
		ThreatGraceRemaining = 0.f;
		OnThreatChanged.Broadcast(OldThreat, BestCandidate);
		UE_LOG(LogPaldarkPal, Verbose,
			TEXT("Pal=%s acquired threat=%s distance=%.1f"),
			*Pal->GetName(),
			*BestCandidate->GetName(),
			BestDistance);
		return;
	}

	if (CurrentThreat.Get() == BestCandidate)
	{
		// Same threat — refresh distance and clear the grace timer.
		CurrentThreatDistance = BestDistance;
		ThreatGraceRemaining = 0.f;
		return;
	}

	// Different actor than CurrentThreat. Apply hysteresis: the new
	// candidate must be at least AggroSwitchHysteresisCm closer than the
	// current pick before we commit to the switch.
	if (CurrentThreatDistance - BestDistance < AggroSwitchHysteresisCm)
	{
		// Not enough delta — keep the current threat for stability. Refresh
		// distance against the old actor (it might have moved).
		const float OldActorDistanceSq = FVector::DistSquared(CurrentThreat->GetActorLocation(), PalLocation);
		CurrentThreatDistance = FMath::Sqrt(OldActorDistanceSq);
		ThreatGraceRemaining = 0.f;
		return;
	}

	// Aggro switch.
	CurrentThreat = BestCandidate;
	CurrentThreatDistance = BestDistance;
	ThreatGraceRemaining = 0.f;
	OnThreatChanged.Broadcast(OldThreat, BestCandidate);
	UE_LOG(LogPaldarkPal, Log,
		TEXT("Pal=%s aggro switch old=%s new=%s new_distance=%.1f"),
		*Pal->GetName(),
		*GetNameSafe(OldThreat),
		*BestCandidate->GetName(),
		BestDistance);
}

void UPaldarkPalPerceptionComponent::ForceThreat(AActor* InThreat)
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority())
	{
		return;
	}
	AActor* OldThreat = CurrentThreat.Get();
	CurrentThreat = InThreat;
	CurrentThreatDistance = InThreat != nullptr && PalOwner.IsValid()
		? FVector::Dist(InThreat->GetActorLocation(), PalOwner->GetActorLocation())
		: 0.f;
	ThreatGraceRemaining = 0.f;
	if (OldThreat != InThreat)
	{
		OnThreatChanged.Broadcast(OldThreat, InThreat);
	}
	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkPalPerceptionComponent::ForceThreat — pal=%s threat=%s"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(InThreat));
}

void UPaldarkPalPerceptionComponent::DumpToLog() const
{
	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkPalPerceptionComponent[%s] threat=%s distance=%.1f grace=%.2f friendly_tag=%s hostile_tag=%s radius=%.1f"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(CurrentThreat.Get()),
		CurrentThreatDistance,
		ThreatGraceRemaining,
		*FriendlyTeamTag.ToString(),
		*HostileTeamTag.ToString(),
		ThreatRadius);
}

FGameplayTag UPaldarkPalPerceptionComponent::GetActorTeamTag(const AActor* InActor)
{
	if (InActor == nullptr)
	{
		return FGameplayTag();
	}
	if (const APaldarkCharacter* AsPlayer = Cast<const APaldarkCharacter>(InActor))
	{
		return AsPlayer->GetTeamTag();
	}
	if (const APaldarkPalCharacter* AsPal = Cast<const APaldarkPalCharacter>(InActor))
	{
		return AsPal->GetTeamTag();
	}
	if (const APaldarkDummyTarget* AsDummy = Cast<const APaldarkDummyTarget>(InActor))
	{
		return AsDummy->GetTeamTag();
	}
	return FGameplayTag();
}

bool UPaldarkPalPerceptionComponent::IsActorThreatening(const AActor* Candidate) const
{
	if (Candidate == nullptr || !IsValid(Candidate) || Candidate->IsActorBeingDestroyed())
	{
		return false;
	}
	if (Candidate == GetOwner())
	{
		return false;
	}
	const FGameplayTag CandidateTeam = GetActorTeamTag(Candidate);
	if (!CandidateTeam.IsValid())
	{
		return false;
	}
	// Skip friendlies first so an actor that somehow carries both the
	// friendly and hostile tag still gets treated as friend.
	if (FriendlyTeamTag.IsValid() && CandidateTeam.MatchesTag(FriendlyTeamTag))
	{
		return false;
	}
	if (HostileTeamTag.IsValid() && CandidateTeam.MatchesTag(HostileTeamTag))
	{
		return true;
	}
	return false;
}
