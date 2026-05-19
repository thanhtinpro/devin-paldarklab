#include "Pal/Components/PaldarkPalActivityComponent.h"

#include "GameFramework/Actor.h"

#include "Pal/Activities/PaldarkActivity_Combat.h"
#include "Pal/Activities/PaldarkActivity_Follow.h"
#include "Pal/Activities/PaldarkActivity_FollowSquadCommand.h"
#include "Pal/Activities/PaldarkActivity_Idle.h"
#include "Pal/Activities/PaldarkActivity_Investigate.h"
#include "Pal/Activities/PaldarkBaseActivity.h"
#include "PaldarkLogCategories.h"

UPaldarkPalActivityComponent::UPaldarkPalActivityComponent()
{
	// The FSM drives behaviour per tick; needs to tick.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup    = TG_PrePhysics;
	SetIsReplicatedByDefault(false); // FSM state lives on the server only.

	// Default candidate set (friendly Pal): Idle / Follow / FollowSquadCommand /
	// Investigate / Combat. Combat (Priority 40) preempts everything else when
	// the perception locks onto a hostile (W18-19). W22-23 adds
	// FollowSquadCommand (Priority 22) — sits between Follow (P20) and
	// Investigate (P30) so an active investigation isn't yanked by a Follow
	// radial command. Hostile-Pal subclasses (Direhound / Razorbird) override
	// this list in their own constructors, so they never run the squad
	// listener — see PaldarkPalCharacter_Direhound.cpp / _Razorbird.cpp.
	CandidateActivities = {
		UPaldarkActivity_Idle::StaticClass(),
		UPaldarkActivity_Follow::StaticClass(),
		UPaldarkActivity_FollowSquadCommand::StaticClass(),
		UPaldarkActivity_Investigate::StaticClass(),
		UPaldarkActivity_Combat::StaticClass(),
	};
}

void UPaldarkPalActivityComponent::BeginPlay()
{
	Super::BeginPlay();

	const AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr || !OwnerActor->HasAuthority())
	{
		// FSM runs on authority only; clients see replicated movement via Character defaults.
		return;
	}

	InstantiateActivities();

	// Idle is the always-running fallback; kick it off so we have a baseline state.
	if (!SetCurrentActivityByClass(UPaldarkActivity_Idle::StaticClass()) && ActivityInstances.Num() > 0)
	{
		SwitchToActivity(ActivityInstances[0]);
	}
}

void UPaldarkPalActivityComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CurrentActivity != nullptr)
	{
		CurrentActivity->ExitActivity();
		CurrentActivity = nullptr;
	}
	ActivityInstances.Reset();
	Super::EndPlay(EndPlayReason);
}

void UPaldarkPalActivityComponent::TickComponent(
	float DeltaSeconds,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);

	const AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr || !OwnerActor->HasAuthority())
	{
		return;
	}

	TimeSinceLastSelection += DeltaSeconds;
	if (TimeSinceLastSelection >= SelectionInterval)
	{
		TimeSinceLastSelection = 0.0f;
		EvaluateAndSwitch();
	}

	if (CurrentActivity != nullptr)
	{
		CurrentActivity->TickActivity(DeltaSeconds);
	}
}

void UPaldarkPalActivityComponent::InstantiateActivities()
{
	ActivityInstances.Reset();
	for (const TSubclassOf<UPaldarkBaseActivity>& Class : CandidateActivities)
	{
		if (Class == nullptr)
		{
			continue;
		}
		UPaldarkBaseActivity* Instance = NewObject<UPaldarkBaseActivity>(this, Class);
		if (Instance == nullptr)
		{
			continue;
		}
		if (!Instance->InitActivity(this))
		{
			continue;
		}
		ActivityInstances.Add(Instance);
		UE_LOG(LogPaldarkPal, Verbose,
			TEXT("UPaldarkPalActivityComponent::InstantiateActivities — pal=%s activity=%s tag=%s priority=%d"),
			*GetNameSafe(GetOwner()),
			*Instance->GetName(),
			*Instance->ActivityTag.ToString(),
			Instance->Priority);
	}
}

void UPaldarkPalActivityComponent::EvaluateAndSwitch()
{
	// If the current activity wants to keep running, let it. This is the
	// hysteresis hook (Follow uses it to avoid Idle flicker around the
	// follow threshold; Investigate uses it to "stick" until arrival).
	if (CurrentActivity != nullptr && CurrentActivity->ShouldContinue())
	{
		// Still let higher-priority activities preempt — that's how Investigate
		// interrupts Follow.
		UPaldarkBaseActivity* BestCandidate = CurrentActivity;
		for (UPaldarkBaseActivity* Candidate : ActivityInstances)
		{
			if (Candidate == nullptr || Candidate == CurrentActivity)
			{
				continue;
			}
			if (Candidate->Priority > BestCandidate->Priority && Candidate->CanRun())
			{
				BestCandidate = Candidate;
			}
		}
		if (BestCandidate != CurrentActivity)
		{
			SwitchToActivity(BestCandidate);
		}
		return;
	}

	// Otherwise, pick the highest-priority candidate that CanRun.
	UPaldarkBaseActivity* BestCandidate = nullptr;
	for (UPaldarkBaseActivity* Candidate : ActivityInstances)
	{
		if (Candidate == nullptr || !Candidate->CanRun())
		{
			continue;
		}
		if (BestCandidate == nullptr || Candidate->Priority > BestCandidate->Priority)
		{
			BestCandidate = Candidate;
		}
	}

	if (BestCandidate != nullptr && BestCandidate != CurrentActivity)
	{
		SwitchToActivity(BestCandidate);
	}
}

void UPaldarkPalActivityComponent::SwitchToActivity(UPaldarkBaseActivity* InActivity)
{
	if (InActivity == CurrentActivity)
	{
		return;
	}

	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkPalActivityComponent::SwitchToActivity — pal=%s from=%s to=%s"),
		*GetNameSafe(GetOwner()),
		CurrentActivity != nullptr ? *CurrentActivity->ActivityTag.ToString() : TEXT("<none>"),
		InActivity != nullptr ? *InActivity->ActivityTag.ToString() : TEXT("<none>"));

	if (CurrentActivity != nullptr)
	{
		CurrentActivity->ExitActivity();
	}
	CurrentActivity = InActivity;
	if (CurrentActivity != nullptr)
	{
		CurrentActivity->EnterActivity();
	}
}

bool UPaldarkPalActivityComponent::SetCurrentActivityByTag(const FGameplayTag& InTag)
{
	for (UPaldarkBaseActivity* Candidate : ActivityInstances)
	{
		if (Candidate != nullptr && Candidate->ActivityTag == InTag)
		{
			SwitchToActivity(Candidate);
			return true;
		}
	}
	UE_LOG(LogPaldarkPal, Warning,
		TEXT("UPaldarkPalActivityComponent::SetCurrentActivityByTag — pal=%s tag=%s not found among %d candidates"),
		*GetNameSafe(GetOwner()),
		*InTag.ToString(),
		ActivityInstances.Num());
	return false;
}

bool UPaldarkPalActivityComponent::SetCurrentActivityByClass(TSubclassOf<UPaldarkBaseActivity> InClass)
{
	if (InClass == nullptr)
	{
		return false;
	}
	for (UPaldarkBaseActivity* Candidate : ActivityInstances)
	{
		if (Candidate != nullptr && Candidate->IsA(InClass))
		{
			SwitchToActivity(Candidate);
			return true;
		}
	}
	return false;
}

void UPaldarkPalActivityComponent::RequestInvestigate(const FVector& InLocation)
{
	InvestigateTarget       = InLocation;
	bHasInvestigateRequest  = true;
	UE_LOG(LogPaldarkPal, Log, TEXT("UPaldarkPalActivityComponent::RequestInvestigate — pal=%s target=%s"),
		*GetNameSafe(GetOwner()),
		*InLocation.ToCompactString());
}

void UPaldarkPalActivityComponent::ClearInvestigateRequest()
{
	if (!bHasInvestigateRequest)
	{
		return;
	}
	bHasInvestigateRequest = false;
	InvestigateTarget      = FVector::ZeroVector;
	UE_LOG(LogPaldarkPal, Log, TEXT("UPaldarkPalActivityComponent::ClearInvestigateRequest — pal=%s"),
		*GetNameSafe(GetOwner()));
}

FGameplayTag UPaldarkPalActivityComponent::GetCurrentActivityTag() const
{
	return CurrentActivity != nullptr ? CurrentActivity->ActivityTag : FGameplayTag::EmptyTag;
}
