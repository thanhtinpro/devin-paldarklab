#include "Player/PaldarkPlayerState.h"

#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

#include "Gas/PaldarkAbilitySystemComponent.h"
#include "Gas/PaldarkAttributeSet.h"
#include "Match/PaldarkMatchTypes.h"
#include "Pal/PaldarkPalDepositComponent.h"
#include "PaldarkLogCategories.h"

APaldarkPlayerState::APaldarkPlayerState()
{
	// Inherited from APlayerState, but spelled out so the intent is obvious.
	bReplicates = true;

	// W7-8 — ASC + AttributeSet sub-object on the PlayerState. ASC is created
	// here (not on the character) so it survives Pawn replacement (death +
	// respawn, possession swap). NetUpdateFrequency stays high so stamina
	// drains during sprint replicate smoothly to simulated proxies.
	AbilitySystemComponent = CreateDefaultSubobject<UPaldarkAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UPaldarkAttributeSet>(TEXT("AttributeSet"));

	// W44-45 — Pal deposit storage component. Same rationale as the ASC —
	// lives on the PlayerState so deposits survive pawn death + respawn.
	PalDepositComponent = CreateDefaultSubobject<UPaldarkPalDepositComponent>(TEXT("PalDepositComponent"));

	NetUpdateFrequency = 100.0f;
}

UAbilitySystemComponent* APaldarkPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void APaldarkPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APaldarkPlayerState, TeamIndex);
	DOREPLIFETIME(APaldarkPlayerState, MatchOutcome);
	DOREPLIFETIME(APaldarkPlayerState, ExtractionProgress);
}

void APaldarkPlayerState::SetTeamIndex(int32 NewTeamIndex)
{
	if (!HasAuthority())
	{
		UE_LOG(LogPaldark, Warning, TEXT("APaldarkPlayerState::SetTeamIndex called on client — ignored."));
		return;
	}
	if (TeamIndex == NewTeamIndex)
	{
		return;
	}
	TeamIndex = NewTeamIndex;
	UE_LOG(LogPaldark, Log, TEXT("APaldarkPlayerState::SetTeamIndex — %s -> team %d"), *GetName(), TeamIndex);
	// Authority fires the RepNotify manually so server-side observers see the
	// same event ordering as clients.
	OnRep_TeamIndex();
}

void APaldarkPlayerState::OnRep_TeamIndex()
{
	UE_LOG(LogPaldark, Log, TEXT("APaldarkPlayerState::OnRep_TeamIndex — %s now on team %d"), *GetName(), TeamIndex);
}

void APaldarkPlayerState::SetMatchOutcome(EPaldarkPlayerOutcome NewOutcome)
{
	if (!HasAuthority())
	{
		UE_LOG(LogPaldark, Warning,
			TEXT("APaldarkPlayerState::SetMatchOutcome called on client — ignored."));
		return;
	}
	if (MatchOutcome == NewOutcome)
	{
		return;
	}
	MatchOutcome = NewOutcome;
	UE_LOG(LogPaldark, Log,
		TEXT("APaldarkPlayerState::SetMatchOutcome — %s outcome=%d"),
		*GetName(), static_cast<int32>(NewOutcome));
	// Server-side mirror call so observers see the same ordering as clients.
	OnRep_MatchOutcome();
}

void APaldarkPlayerState::SetExtractionProgress(float NewProgress)
{
	if (!HasAuthority())
	{
		return;
	}
	const float Clamped = FMath::Clamp(NewProgress, 0.f, 1.f);
	if (FMath::IsNearlyEqual(ExtractionProgress, Clamped, 0.001f))
	{
		return;
	}
	ExtractionProgress = Clamped;
	OnRep_ExtractionProgress();
}

void APaldarkPlayerState::OnRep_MatchOutcome()
{
	UE_LOG(LogPaldark, Log,
		TEXT("APaldarkPlayerState::OnRep_MatchOutcome — %s outcome=%d"),
		*GetName(), static_cast<int32>(MatchOutcome));
	OnMatchOutcomeChanged.Broadcast(MatchOutcome);
}

void APaldarkPlayerState::OnRep_ExtractionProgress()
{
	UE_LOG(LogPaldark, VeryVerbose,
		TEXT("APaldarkPlayerState::OnRep_ExtractionProgress — %s progress=%.2f"),
		*GetName(), ExtractionProgress);
}
