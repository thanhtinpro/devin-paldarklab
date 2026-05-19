#include "Pal/Components/PaldarkPalBondComponent.h"

#include "Net/UnrealNetwork.h"

#include "PaldarkLogCategories.h"

UPaldarkPalBondComponent::UPaldarkPalBondComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// W35-36 — Now replicated so HUD widgets on every client can read
	// BondLevel / BondXP. W3-4's stub was bReplicated=false because the
	// stub had no replicated state.
	SetIsReplicatedByDefault(true);
}

void UPaldarkPalBondComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Both replicated to all clients — designer HUD widget on the local
	// player can read the bond state of any nearby Pal (e.g. "Foxparks
	// Bond 8 / 20" floating label).
	DOREPLIFETIME(UPaldarkPalBondComponent, BondLevel);
	DOREPLIFETIME(UPaldarkPalBondComponent, BondXP);
}

void UPaldarkPalBondComponent::AddBondXP(float Amount, FGameplayTag ReasonTag)
{
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	if (!bTracksBond)
	{
		// Hostile Pal opt-out — silently no-op.
		return;
	}

	if (Amount <= 0.0f || BondLevel >= MaxBondLevel)
	{
		// Already capped — no-op, but log so designers tuning the curve
		// can see the ceiling event.
		UE_LOG(LogPaldarkPal, Verbose,
			TEXT("UPaldarkPalBondComponent::AddBondXP — owner=%s capped at level %d (XP %.1f, reason %s)"),
			*GetOwner()->GetName(), BondLevel, BondXP, *ReasonTag.ToString());
		return;
	}

	const float OldXP = BondXP;
	const int32 OldLevel = BondLevel;
	BondXP += Amount;

	// Walk up the curve — a single grant can cross multiple level
	// boundaries (e.g. designer hands out 500 XP for "tame the boss").
	while (BondLevel < MaxBondLevel)
	{
		const float NextThreshold = GetXPThresholdForLevel(BondLevel + 1);
		if (BondXP < NextThreshold)
		{
			break;
		}
		++BondLevel;
		OnBondLevelChanged.Broadcast(BondLevel);
	}

	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkPalBondComponent::AddBondXP — owner=%s reason=%s XP %.1f -> %.1f level %d -> %d"),
		*GetOwner()->GetName(), *ReasonTag.ToString(), OldXP, BondXP, OldLevel, BondLevel);
}

void UPaldarkPalBondComponent::SetBondLevel(int32 NewLevel)
{
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	const int32 Clamped = FMath::Clamp(NewLevel, 0, MaxBondLevel);
	if (Clamped == BondLevel)
	{
		return;
	}

	const int32 OldLevel = BondLevel;
	BondLevel = Clamped;
	// Re-baseline XP to the new level's threshold so a subsequent AddBondXP
	// doesn't accidentally re-trigger the just-crossed level.
	BondXP = GetXPThresholdForLevel(BondLevel);
	OnBondLevelChanged.Broadcast(BondLevel);

	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkPalBondComponent::SetBondLevel — owner=%s level %d -> %d (XP rebaseline %.1f)"),
		*GetOwner()->GetName(), OldLevel, BondLevel, BondXP);
}

float UPaldarkPalBondComponent::GetXPThresholdForLevel(int32 TargetLevel) const
{
	// Designer can override via the runtime curve; fallback is a linear
	// 100 XP/level baseline so a Pal with no curve authored still levels
	// up sanely from console commands during early testing.
	const FRichCurve* Curve = BondXPCurve.GetRichCurveConst();
	if (Curve && Curve->Keys.Num() > 0)
	{
		return Curve->Eval(static_cast<float>(TargetLevel));
	}
	return static_cast<float>(TargetLevel) * 100.0f;
}

void UPaldarkPalBondComponent::OnRep_BondLevel(int32 OldLevel)
{
	if (BondLevel != OldLevel)
	{
		OnBondLevelChanged.Broadcast(BondLevel);
	}
}

void UPaldarkPalBondComponent::OnRep_BondXP(float /*OldXP*/)
{
	// Informational — HUD widgets bind via OnBondLevelChanged for the
	// integer breakpoints. Designers can extend with an XP-level bar
	// reader later in W37-38.
}
