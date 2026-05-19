#include "Pal/Activities/PaldarkActivity_BossPhase.h"

#include "Engine/AssetManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Gas/PaldarkAttributeSet.h"
#include "Pal/Components/PaldarkPalActivityComponent.h"
#include "Pal/Components/PaldarkPalCombatComponent.h"
#include "Pal/Components/PaldarkPalPerceptionComponent.h"
#include "Pal/PaldarkPalCharacter.h"
#include "Pal/PaldarkPalDefinition.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"

UPaldarkActivity_BossPhase::UPaldarkActivity_BossPhase()
{
	ActivityTag = PaldarkGameplayTags::TAG_Paldark_Pal_Activity_BossPhase;
	Priority    = 50; // Higher than Combat (40) so the boss never falls back to Combat.
}

bool UPaldarkActivity_BossPhase::InitActivity(UPaldarkPalActivityComponent* InComponent)
{
	if (!Super::InitActivity(InComponent))
	{
		return false;
	}

	if (APaldarkPalCharacter* Pal = GetPalOwner())
	{
		CombatRef     = Pal->GetCombatSlot();
		PerceptionRef = Pal->GetPerceptionSlot();
	}

	return true;
}

bool UPaldarkActivity_BossPhase::CanRun_Implementation() const
{
	UPaldarkPalCombatComponent* Combat = CombatRef.Get();
	if (Combat == nullptr)
	{
		return false;
	}
	AActor* Target = Combat->GetCurrentTarget();
	return Target != nullptr && IsValid(Target);
}

bool UPaldarkActivity_BossPhase::ShouldContinue_Implementation() const
{
	// Boss never reverts out of the boss activity once entered — even if
	// the target temporarily walks out of range, the boss stays in the
	// phased state and the next selection tick will re-enter combat
	// once a target is back in perception range.
	return true;
}

void UPaldarkActivity_BossPhase::EnterActivity_Implementation()
{
	Super::EnterActivity_Implementation();

	CurrentPhase = EPaldarkBossPhase::Normal;

	if (APaldarkPalCharacter* Pal = GetPalOwner())
	{
		if (UCharacterMovementComponent* Movement = Pal->GetCharacterMovement())
		{
			OriginalMaxWalkSpeed = Movement->MaxWalkSpeed;
		}
		if (UPaldarkPalCombatComponent* Combat = Pal->GetCombatSlot())
		{
			OriginalAttackInterval = Combat->AttackInterval;
		}
	}

	// W39 — L-28 closure. Read per-species `PhaseHealthThresholds[]` from
	// `UPaldarkPalDefinition` and overwrite `EnragedThresholdPct` /
	// `TelegraphThresholdPct` if the definition declares them. This is
	// invoked every enter so a hot-reload of the DA (designer iteration
	// in PIE) takes effect on the next activity re-entry without a
	// recompile. Silently falls back to the activity's CDO defaults
	// (0.5 / 0.25) on missing / malformed definition data.
	HydrateThresholdsFromPalDefinition();

	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkActivity_BossPhase::EnterActivity — pal=%s phase=Normal enraged_pct=%.2f telegraph_pct=%.2f"),
		*GetNameSafe(GetPalOwner()),
		EnragedThresholdPct,
		TelegraphThresholdPct);
}

void UPaldarkActivity_BossPhase::TickActivity_Implementation(float DeltaSeconds)
{
	Super::TickActivity_Implementation(DeltaSeconds);

	APaldarkPalCharacter* Pal = GetPalOwner();
	UPaldarkPalCombatComponent* Combat = CombatRef.Get();
	if (Pal == nullptr || Combat == nullptr)
	{
		return;
	}

	// Phase transition check — monotonic (Normal → Enraged → Telegraph).
	const float HealthPct = GetCurrentHealthFraction();
	if (CurrentPhase == EPaldarkBossPhase::Normal && HealthPct <= EnragedThresholdPct)
	{
		TransitionToPhase(EPaldarkBossPhase::Enraged);
	}
	else if (CurrentPhase == EPaldarkBossPhase::Enraged && HealthPct <= TelegraphThresholdPct)
	{
		TransitionToPhase(EPaldarkBossPhase::Telegraph);
	}

	// Re-orient toward target (mirrors UPaldarkActivity_Combat).
	AActor* Target = Combat->GetCurrentTarget();
	if (Target != nullptr && IsValid(Target))
	{
		const FVector ToTarget = (Target->GetActorLocation() - Pal->GetActorLocation());
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(ToTarget.Y, ToTarget.X));
		const FRotator CurrentRot = Pal->GetActorRotation();
		const FRotator DesiredRot(0.f, Yaw, 0.f);
		const FRotator Interp = FMath::RInterpConstantTo(CurrentRot, DesiredRot, DeltaSeconds, FaceTargetRateDegPerSec);
		Pal->SetActorRotation(Interp);

		// Disengage guard — same role as UPaldarkActivity_Combat's
		// DisengageDistance, but here we just clear the target locally so
		// the next tick re-aggros instead of reverting to Idle.
		const float DistSqr = ToTarget.SizeSquared();
		if (DistSqr > FMath::Square(DisengageDistance))
		{
			// Don't actually clear target — perception will refresh it.
			// Skip the attack tick this frame to avoid firing into the
			// void if the target is way out of range.
			return;
		}

		Combat->TryFireAttack();
	}
}

void UPaldarkActivity_BossPhase::ExitActivity_Implementation()
{
	Super::ExitActivity_Implementation();

	// Revert multipliers so the Pal pawn returns to its baseline values
	// in case the activity tears down mid-fight (e.g. boss EndPlay
	// reason).
	if (APaldarkPalCharacter* Pal = GetPalOwner())
	{
		if (UCharacterMovementComponent* Movement = Pal->GetCharacterMovement())
		{
			if (OriginalMaxWalkSpeed > 0.f)
			{
				Movement->MaxWalkSpeed = OriginalMaxWalkSpeed;
			}
		}
		if (UPaldarkPalCombatComponent* Combat = Pal->GetCombatSlot())
		{
			if (OriginalAttackInterval > 0.f)
			{
				Combat->AttackInterval = OriginalAttackInterval;
			}
		}
	}
}

float UPaldarkActivity_BossPhase::GetCurrentHealthFraction() const
{
	const APaldarkPalCharacter* Pal = GetPalOwner();
	if (Pal == nullptr)
	{
		return 1.f;
	}
	const UPaldarkAttributeSet* Attr = Pal->GetPaldarkAttributeSet();
	if (Attr == nullptr)
	{
		return 1.f;
	}
	const float Max = Attr->GetMaxHealth();
	if (Max <= 0.f)
	{
		return 1.f;
	}
	return FMath::Clamp(Attr->GetHealth() / Max, 0.f, 1.f);
}

void UPaldarkActivity_BossPhase::TransitionToPhase(EPaldarkBossPhase NewPhase)
{
	if (NewPhase == CurrentPhase)
	{
		return;
	}
	const EPaldarkBossPhase OldPhase = CurrentPhase;
	CurrentPhase = NewPhase;

	APaldarkPalCharacter* Pal = GetPalOwner();
	if (Pal != nullptr)
	{
		if (NewPhase == EPaldarkBossPhase::Enraged || NewPhase == EPaldarkBossPhase::Telegraph)
		{
			if (UCharacterMovementComponent* Movement = Pal->GetCharacterMovement())
			{
				Movement->MaxWalkSpeed = OriginalMaxWalkSpeed * EnragedMoveSpeedMultiplier;
			}
			if (UPaldarkPalCombatComponent* Combat = Pal->GetCombatSlot())
			{
				Combat->AttackInterval = OriginalAttackInterval * EnragedAttackIntervalMultiplier;
			}
		}
	}

	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkActivity_BossPhase::TransitionToPhase — pal=%s %s -> %s"),
		*GetNameSafe(Pal),
		*UEnum::GetValueAsString(OldPhase),
		*UEnum::GetValueAsString(NewPhase));

	OnBossPhaseChanged.Broadcast(OldPhase, NewPhase);
}

void UPaldarkActivity_BossPhase::HydrateThresholdsFromPalDefinition()
{
	// W39 — L-28 closure. Pull per-species phase thresholds from
	// `UPaldarkPalDefinition::PhaseHealthThresholds[]` and overwrite the
	// activity's CDO defaults if the array is well-formed. Lookup pattern
	// mirrors `UPaldarkPalTameComponent::HydrateFromPalDefinition` (Species
	// leaf → `DA_PalDef_<Leaf>` primary asset id, sync-read from the
	// AssetManager). Silently no-op (UPROPERTY defaults stand) on any
	// missing / malformed input — designer sees a Warning in PIE so they
	// can fix the DA without re-running the encounter.
	const APaldarkPalCharacter* Pal = GetPalOwner();
	if (Pal == nullptr)
	{
		return;
	}

	const FGameplayTag Species = Pal->GetSpeciesTag();
	if (!Species.IsValid())
	{
		// Boss subclass forgot to stamp SpeciesTag — fall through to
		// the CDO defaults (0.5 / 0.25 — W29-30 baseline).
		UE_LOG(LogPaldarkPal, Verbose,
			TEXT("[L-28] UPaldarkActivity_BossPhase::HydrateThresholdsFromPalDefinition — pal=%s has empty SpeciesTag, keeping activity defaults (enraged=%.2f telegraph=%.2f)"),
			*GetNameSafe(Pal), EnragedThresholdPct, TelegraphThresholdPct);
		return;
	}

	const FString FullTag = Species.GetTagName().ToString();
	int32 LastDot = INDEX_NONE;
	if (!FullTag.FindLastChar(TEXT('.'), LastDot) || LastDot >= FullTag.Len() - 1)
	{
		UE_LOG(LogPaldarkPal, Verbose,
			TEXT("[L-28] UPaldarkActivity_BossPhase::HydrateThresholdsFromPalDefinition — pal=%s species tag '%s' has no leaf segment; keeping activity defaults"),
			*GetNameSafe(Pal), *FullTag);
		return;
	}

	const FString Leaf = FullTag.Mid(LastDot + 1);
	const FPrimaryAssetId DefId(TEXT("PaldarkPalDefinition"), FName(*Leaf));

	UAssetManager* AM = UAssetManager::GetIfValid();
	if (AM == nullptr)
	{
		return;
	}

	UObject* Loaded = AM->GetPrimaryAssetObject(DefId);
	const UPaldarkPalDefinition* Def = Cast<UPaldarkPalDefinition>(Loaded);
	if (Def == nullptr)
	{
		// PalDef not warm yet (early spawn) or designer hasn't authored
		// `DA_PalDef_<Leaf>` (W29-30 designer task L-25). Keep activity
		// defaults so the boss still functions; designer sees the L-25
		// missing DA warning elsewhere.
		UE_LOG(LogPaldarkPal, Verbose,
			TEXT("[L-28] UPaldarkActivity_BossPhase::HydrateThresholdsFromPalDefinition — pal=%s species=%s no PalDef resolved (DA_PalDef_%s); keeping activity defaults"),
			*GetNameSafe(Pal), *FullTag, *Leaf);
		return;
	}

	const TArray<float>& Thresholds = Def->PhaseHealthThresholds;
	if (Thresholds.Num() == 0)
	{
		// Designer authored DA but left thresholds empty — back-compat
		// path; treated identically to "no DA" (CDO defaults).
		UE_LOG(LogPaldarkPal, Verbose,
			TEXT("[L-28] UPaldarkActivity_BossPhase::HydrateThresholdsFromPalDefinition — pal=%s species=%s PalDef has empty PhaseHealthThresholds[]; keeping activity defaults"),
			*GetNameSafe(Pal), *FullTag);
		return;
	}

	if (Thresholds.Num() < 2)
	{
		// Need at least 2 entries (Enraged + Telegraph) for the 3-phase
		// FSM. A single-entry array is almost certainly a designer typo —
		// log a Warning so it surfaces in PIE.
		UE_LOG(LogPaldarkPal, Warning,
			TEXT("[L-28] UPaldarkActivity_BossPhase::HydrateThresholdsFromPalDefinition — pal=%s species=%s PalDef PhaseHealthThresholds has %d entry, need >= 2 (Enraged + Telegraph); keeping activity defaults (enraged=%.2f telegraph=%.2f)"),
			*GetNameSafe(Pal), *FullTag, Thresholds.Num(),
			EnragedThresholdPct, TelegraphThresholdPct);
		return;
	}

	const float Enraged   = Thresholds[0];
	const float Telegraph = Thresholds[1];

	if (Enraged <= Telegraph)
	{
		// Order must be strictly descending (Enraged HP gate > Telegraph
		// HP gate) — otherwise the boss skips Enraged entirely or never
		// reaches Telegraph. Designer typo; surface as Warning and keep
		// CDO defaults so the encounter remains playable.
		UE_LOG(LogPaldarkPal, Warning,
			TEXT("[L-28] UPaldarkActivity_BossPhase::HydrateThresholdsFromPalDefinition — pal=%s species=%s PalDef PhaseHealthThresholds malformed (enraged=%.2f <= telegraph=%.2f, must be strictly descending); keeping activity defaults"),
			*GetNameSafe(Pal), *FullTag, Enraged, Telegraph);
		return;
	}

	if (Enraged < 0.f || Enraged > 1.f || Telegraph < 0.f || Telegraph > 1.f)
	{
		// HP fractions must live in [0..1] — values outside that range
		// are designer typos (e.g. 60.0 instead of 0.60). Warn + ignore.
		UE_LOG(LogPaldarkPal, Warning,
			TEXT("[L-28] UPaldarkActivity_BossPhase::HydrateThresholdsFromPalDefinition — pal=%s species=%s PalDef PhaseHealthThresholds out of range (enraged=%.2f telegraph=%.2f, must be in [0,1]); keeping activity defaults"),
			*GetNameSafe(Pal), *FullTag, Enraged, Telegraph);
		return;
	}

	const float OldEnraged   = EnragedThresholdPct;
	const float OldTelegraph = TelegraphThresholdPct;
	EnragedThresholdPct   = Enraged;
	TelegraphThresholdPct = Telegraph;

	UE_LOG(LogPaldarkPal, Log,
		TEXT("[L-28] UPaldarkActivity_BossPhase::HydrateThresholdsFromPalDefinition — pal=%s species=%s overrode thresholds (enraged %.2f -> %.2f, telegraph %.2f -> %.2f) from DA_PalDef_%s.PhaseHealthThresholds"),
		*GetNameSafe(Pal), *FullTag,
		OldEnraged, EnragedThresholdPct,
		OldTelegraph, TelegraphThresholdPct,
		*Leaf);
}
