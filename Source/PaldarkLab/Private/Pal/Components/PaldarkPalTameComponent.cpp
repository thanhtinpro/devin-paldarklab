#include "Pal/Components/PaldarkPalTameComponent.h"

#include "AbilitySystemComponent.h"
#include "Engine/AssetManager.h"

#include "Character/PaldarkCharacter.h"
#include "Gas/PaldarkAttributeSet.h"
#include "Pal/Components/PaldarkPalBondComponent.h"
#include "Pal/PaldarkPalCharacter.h"
#include "Pal/PaldarkPalDefinition.h"
#include "Pal/PaldarkPlayerPalRosterComponent.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"

UPaldarkPalTameComponent::UPaldarkPalTameComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	// Server-only — no need to replicate the component or its state. Result
	// broadcast travels via gameplay events on the instigator's ASC + the
	// roster component (COND_OwnerOnly) instead of full property replication.
	SetIsReplicatedByDefault(false);
}

void UPaldarkPalTameComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	if (!bIsTameable)
	{
		// Player-side companion / non-tameable Pal — early return, no extra
		// work. Component still exists so a designer can flip the flag in BP
		// without recompiling C++.
		return;
	}

	HydrateFromPalDefinition();

	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkPalTameComponent::BeginPlay — owner=%s tameable difficulty=%.2f base=%.2f"),
		*GetOwner()->GetName(), TameDifficulty, BaseCaptureProbability);
}

EPaldarkTameResult UPaldarkPalTameComponent::BeginTameAttempt(APaldarkCharacter* Instigator, FGameplayTag PalSphereTierTag)
{
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		return EPaldarkTameResult::NotAttempted;
	}

	if (!bIsTameable || bTameAttemptInProgress)
	{
		UE_LOG(LogPaldarkPal, Verbose,
			TEXT("UPaldarkPalTameComponent::BeginTameAttempt — owner=%s rejected (tameable=%d inflight=%d)"),
			*GetOwner()->GetName(), bIsTameable ? 1 : 0, bTameAttemptInProgress ? 1 : 0);
		return EPaldarkTameResult::NotAttempted;
	}

	bTameAttemptInProgress = true;

	APaldarkPalCharacter* PalOwner = Cast<APaldarkPalCharacter>(GetOwner());
	if (!PalOwner)
	{
		bTameAttemptInProgress = false;
		return EPaldarkTameResult::NotAttempted;
	}

	const UPaldarkAttributeSet* Attr = PalOwner->GetPaldarkAttributeSet();
	if (!Attr)
	{
		bTameAttemptInProgress = false;
		return EPaldarkTameResult::NotAttempted;
	}

	const float MaxHp   = FMath::Max(1.0f, Attr->GetMaxHealth());
	const float Hp      = FMath::Max(0.0f, Attr->GetHealth());
	const float HpPct   = FMath::Clamp(Hp / MaxHp, 0.0f, 1.0f);
	const float MaxStn  = FMath::Max(1.0f, Attr->GetMaxStun());
	const float Stn     = FMath::Max(0.0f, Attr->GetStun());
	const float StunPct = FMath::Clamp(Stn / MaxStn, 0.0f, 1.0f);

	// HP gate: Pal must be below 80% HP to roll. Roadmap calls this out as
	// "weaken first, then throw" — a fresh full-HP Pal returns
	// `FailHpTooHigh` immediately and the sphere is consumed.
	constexpr float HpGate = 0.8f;
	if (HpPct >= HpGate)
	{
		bTameAttemptInProgress = false;
		OnTameAttemptResolved.Broadcast(Instigator, EPaldarkTameResult::FailHpTooHigh, 0.0f);
		UE_LOG(LogPaldarkPal, Log,
			TEXT("UPaldarkPalTameComponent::BeginTameAttempt — owner=%s instigator=%s FAIL_HP_TOO_HIGH (HpPct %.2f >= gate %.2f)"),
			*GetOwner()->GetName(),
			Instigator ? *Instigator->GetName() : TEXT("<none>"),
			HpPct, HpGate);
		return EPaldarkTameResult::FailHpTooHigh;
	}

	const float TierMul = GetPalSphereTierMultiplier(PalSphereTierTag);
	const float SafeDifficulty = FMath::Max(0.1f, TameDifficulty);

	const float RawProb = BaseCaptureProbability
		* (1.0f - HpPct)
		* (1.0f + StunPct)
		* TierMul
		/ SafeDifficulty;
	const float Prob = FMath::Clamp(RawProb, MinCaptureProbability, MaxCaptureProbability);

	const float Roll = FMath::FRand();
	const bool bSuccess = Roll < Prob;

	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkPalTameComponent::BeginTameAttempt — owner=%s instigator=%s tier=%s HpPct=%.2f StunPct=%.2f tier_mul=%.2f base=%.2f diff=%.2f raw_p=%.3f p=%.3f roll=%.3f -> %s"),
		*GetOwner()->GetName(),
		Instigator ? *Instigator->GetName() : TEXT("<none>"),
		*PalSphereTierTag.ToString(),
		HpPct, StunPct, TierMul, BaseCaptureProbability, SafeDifficulty,
		RawProb, Prob, Roll,
		bSuccess ? TEXT("SUCCESS") : TEXT("FAIL_RNG_MISS"));

	if (bSuccess)
	{
		OnTameAttemptResolved.Broadcast(Instigator, EPaldarkTameResult::Success, Prob);
		RegisterAndDestroy(Instigator, Prob);
		// `RegisterAndDestroy` destroys the owner; the component is gone.
		// Don't touch bTameAttemptInProgress.
		return EPaldarkTameResult::Success;
	}

	// Fail path: drain a chunk of MaxStun so the next throw doesn't get
	// the same stun boost. ApplyModToAttribute is the canonical way to
	// adjust a base attribute outside a GE.
	if (UAbilitySystemComponent* PalASC = PalOwner->GetAbilitySystemComponent())
	{
		const float Drain = MaxStn * FailStunDrainFraction;
		PalASC->ApplyModToAttribute(UPaldarkAttributeSet::GetStunAttribute(),
			EGameplayModOp::Additive,
			-Drain);
	}

	bTameAttemptInProgress = false;
	OnTameAttemptResolved.Broadcast(Instigator, EPaldarkTameResult::FailRngMiss, Prob);
	return EPaldarkTameResult::FailRngMiss;
}

void UPaldarkPalTameComponent::ForceTame(APaldarkCharacter* Instigator)
{
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	if (bTameAttemptInProgress)
	{
		return;
	}

	bTameAttemptInProgress = true;

	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkPalTameComponent::ForceTame — owner=%s instigator=%s (formula bypass)"),
		*GetOwner()->GetName(),
		Instigator ? *Instigator->GetName() : TEXT("<none>"));

	OnTameAttemptResolved.Broadcast(Instigator, EPaldarkTameResult::Success, 1.0f);
	RegisterAndDestroy(Instigator, 1.0f);
}

float UPaldarkPalTameComponent::GetPalSphereTierMultiplier(FGameplayTag PalSphereTierTag)
{
	// Tier multiplier table — kept small + static so designers can read off
	// the formula without spelunking through a curve table.
	if (PalSphereTierTag == TAG_Paldark_Item_PalSphere_T3)
	{
		return 2.0f;
	}
	if (PalSphereTierTag == TAG_Paldark_Item_PalSphere_T2)
	{
		return 1.5f;
	}
	// T1 + unknown tag both baseline at 1.0 so a console-cmd force-tame
	// without a tier still computes a sane formula.
	return 1.0f;
}

void UPaldarkPalTameComponent::HydrateFromPalDefinition()
{
	if (!GetOwner())
	{
		return;
	}

	APaldarkPalCharacter* PalOwner = Cast<APaldarkPalCharacter>(GetOwner());
	if (!PalOwner)
	{
		return;
	}

	const FGameplayTag Species = PalOwner->GetSpeciesTag();
	if (!Species.IsValid())
	{
		// Hostile-Pal subclass forgot to stamp SpeciesTag — fall through to
		// UPROPERTY defaults instead of failing the spawn.
		UE_LOG(LogPaldarkPal, Verbose,
			TEXT("UPaldarkPalTameComponent::HydrateFromPalDefinition — owner=%s has empty SpeciesTag, using UPROPERTY defaults"),
			*GetOwner()->GetName());
		return;
	}

	// Convention: PalDefinition primary asset id row name == species tag's
	// leaf name. W27-28 designers author `DA_PalDef_<Leaf>` under
	// `/Game/Paldark/Pals/`. We sync-load here because:
	//   - The hostile Pal has already spawned at this point (BeginPlay),
	//     so the "Spawn" bundle for this species is already warm.
	//   - The PalDefinition asset itself is tiny (a handful of soft refs).
	// If the async load isn't warm yet (e.g. early frame on a freshly
	// spawned hostile), we silently fall back to UPROPERTY defaults.
	const FString LeafName = Species.GetTagName().ToString();
	int32 LastDot = INDEX_NONE;
	if (LeafName.FindLastChar(TEXT('.'), LastDot) && LastDot < LeafName.Len() - 1)
	{
		const FString Leaf = LeafName.Mid(LastDot + 1);
		const FPrimaryAssetId DefId(TEXT("PaldarkPalDefinition"), FName(*Leaf));

		if (UAssetManager* AM = UAssetManager::GetIfValid())
		{
			if (UObject* Loaded = AM->GetPrimaryAssetObject(DefId))
			{
				if (UPaldarkPalDefinition* Def = Cast<UPaldarkPalDefinition>(Loaded))
				{
					// Definition wins if the designer authored non-default
					// values. The Pal might still not have a DataAsset at
					// all (early test), in which case UPROPERTY defaults
					// stand.
					if (Def->TameDifficulty > 0.0f)
					{
						TameDifficulty = Def->TameDifficulty;
					}
					if (Def->BaseCaptureProbability > 0.0f)
					{
						BaseCaptureProbability = Def->BaseCaptureProbability;
					}
				}
			}
		}
	}
}

void UPaldarkPalTameComponent::RegisterAndDestroy(APaldarkCharacter* Instigator, float RolledProbability)
{
	APaldarkPalCharacter* PalOwner = Cast<APaldarkPalCharacter>(GetOwner());
	if (!PalOwner)
	{
		return;
	}

	// Register on the instigator's roster, if present. The roster TamePal
	// API handles species / health / nickname snapshot.
	if (Instigator)
	{
		if (UPaldarkPlayerPalRosterComponent* Roster =
			Instigator->FindComponentByClass<UPaldarkPlayerPalRosterComponent>())
		{
			Roster->TamePal(PalOwner, RolledProbability);
		}

		// Optional bond-event grant — if the instigator owns Pals in their
		// roster, an existing tamed copy of this species gets a Bond XP
		// bump. Reason tag Paldark.Bond.Event.Tame for telemetry. Currently
		// no-op until a bond-driven sub-roster lands; logged for future.
		UE_LOG(LogPaldarkPal, Verbose,
			TEXT("UPaldarkPalTameComponent::RegisterAndDestroy — instigator=%s queued bond event Tame for species=%s"),
			*Instigator->GetName(),
			*PalOwner->GetSpeciesTag().ToString());
	}

	// Finally destroy the owner. Component dies with it.
	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkPalTameComponent::RegisterAndDestroy — destroying owner=%s after tame (p=%.3f)"),
		*PalOwner->GetName(), RolledProbability);
	PalOwner->Destroy();
}
