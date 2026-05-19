#include "Framework/PaldarkGameStateBase.h"

#include "Net/UnrealNetwork.h"

#include "Hub/PaldarkBriefingSessionComponent.h"
#include "PaldarkLogCategories.h"
#include "Experience/PaldarkExperienceDefinition.h"

APaldarkGameStateBase::APaldarkGameStateBase()
{
	bReplicates = true;

	// W46 — Briefing session component default subobject. Replicated by
	// virtue of the GameState replicating + the component opting in via
	// SetIsReplicatedByDefault in its ctor.
	BriefingSessionComponent = CreateDefaultSubobject<UPaldarkBriefingSessionComponent>(TEXT("BriefingSessionComponent"));
}

void APaldarkGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APaldarkGameStateBase, CurrentExperienceId);
	DOREPLIFETIME(APaldarkGameStateBase, MatchPhase);
	DOREPLIFETIME(APaldarkGameStateBase, MatchEndReason);
	DOREPLIFETIME(APaldarkGameStateBase, PhaseEndServerTime);
}

void APaldarkGameStateBase::SetCurrentExperience(const FPrimaryAssetId& AssetId, const UPaldarkExperienceDefinition* Experience)
{
	if (!HasAuthority())
	{
		UE_LOG(LogPaldark, Warning, TEXT("SetCurrentExperience called on client — ignored."));
		return;
	}

	CurrentExperienceId = AssetId;
	CurrentExperience = Experience;

	UE_LOG(
		LogPaldark,
		Log,
		TEXT("APaldarkGameStateBase::SetCurrentExperience — id=%s, ptr=%s"),
		*CurrentExperienceId.ToString(),
		Experience != nullptr ? *Experience->GetPathName() : TEXT("nullptr"));

	OnExperienceLoaded.Broadcast(Experience);
}

void APaldarkGameStateBase::OnRep_CurrentExperienceId()
{
	UE_LOG(
		LogPaldark,
		Log,
		TEXT("APaldarkGameStateBase::OnRep_CurrentExperienceId — id=%s (client resolving soft pointer)."),
		*CurrentExperienceId.ToString());

	// Client-side resolve. Synchronous is acceptable here because the asset will
	// already be loaded by the time RepNotify fires (server cooked it into the
	// session). Replace with FStreamableManager async load if cold-start latency
	// becomes a problem (W14+ networking work).
	if (CurrentExperienceId.IsValid() && CurrentExperience == nullptr)
	{
		if (UAssetManager* Manager = UAssetManager::GetIfInitialized())
		{
			const FSoftObjectPath Path = Manager->GetPrimaryAssetPath(CurrentExperienceId);
			if (!Path.IsNull())
			{
				CurrentExperience = ::Cast<UPaldarkExperienceDefinition>(Path.TryLoad());
			}
		}
	}

	OnExperienceLoaded.Broadcast(CurrentExperience);
}

void APaldarkGameStateBase::SetMatchPhaseFromSubsystem(
	EPaldarkMatchPhase NewPhase,
	EPaldarkMatchEndReason NewReason,
	float NewPhaseEndServerTime)
{
	if (!HasAuthority())
	{
		UE_LOG(LogPaldark, Warning,
			TEXT("APaldarkGameStateBase::SetMatchPhaseFromSubsystem called on client — ignored."));
		return;
	}

	const bool bPhaseChanged   = (MatchPhase != NewPhase);
	const bool bReasonChanged  = (MatchEndReason != NewReason);
	const bool bTimerChanged   = (!FMath::IsNearlyEqual(PhaseEndServerTime, NewPhaseEndServerTime, 0.001f));

	MatchPhase         = NewPhase;
	MatchEndReason     = NewReason;
	PhaseEndServerTime = NewPhaseEndServerTime;

	if (bPhaseChanged)
	{
		OnRep_MatchPhase();
	}
	if (bReasonChanged)
	{
		OnRep_MatchEndReason();
	}
	if (bTimerChanged)
	{
		OnRep_PhaseEndServerTime();
	}
}

void APaldarkGameStateBase::OnRep_MatchPhase()
{
	UE_LOG(LogPaldark, Log,
		TEXT("APaldarkGameStateBase::OnRep_MatchPhase — phase=%d"),
		static_cast<int32>(MatchPhase));
	OnMatchPhaseReplicated.Broadcast(MatchPhase);
}

void APaldarkGameStateBase::OnRep_MatchEndReason()
{
	UE_LOG(LogPaldark, Log,
		TEXT("APaldarkGameStateBase::OnRep_MatchEndReason — reason=%d"),
		static_cast<int32>(MatchEndReason));
}

void APaldarkGameStateBase::OnRep_PhaseEndServerTime()
{
	UE_LOG(LogPaldark, VeryVerbose,
		TEXT("APaldarkGameStateBase::OnRep_PhaseEndServerTime — t=%.2f"),
		PhaseEndServerTime);
}
