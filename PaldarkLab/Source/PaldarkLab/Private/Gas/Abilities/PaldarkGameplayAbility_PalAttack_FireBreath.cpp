#include "Gas/Abilities/PaldarkGameplayAbility_PalAttack_FireBreath.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameplayEffect.h"

#include "Pal/Components/PaldarkPalCombatComponent.h"
#include "Pal/PaldarkPalCharacter.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"

namespace
{
	// Resolve the AbilitySystemComponent on an arbitrary actor — Pals own
	// theirs directly via IAbilitySystemInterface; player pawns route via
	// PlayerState. Mirrors the helper used in the W18-19 base PalAttack.
	UAbilitySystemComponent* ResolveTargetASC(AActor* Target)
	{
		if (Target == nullptr)
		{
			return nullptr;
		}
		if (IAbilitySystemInterface* AsiActor = Cast<IAbilitySystemInterface>(Target))
		{
			if (UAbilitySystemComponent* ASC = AsiActor->GetAbilitySystemComponent())
			{
				return ASC;
			}
		}
		if (const APawn* AsPawn = Cast<APawn>(Target))
		{
			if (APlayerState* PS = AsPawn->GetPlayerState())
			{
				if (IAbilitySystemInterface* AsiPs = Cast<IAbilitySystemInterface>(PS))
				{
					return AsiPs->GetAbilitySystemComponent();
				}
			}
		}
		return nullptr;
	}
}

UPaldarkGameplayAbility_PalAttack_FireBreath::UPaldarkGameplayAbility_PalAttack_FireBreath()
{
	DebugName = TEXT("PalAttack.FireBreath");

	AbilityTags.Reset();
	AbilityTags.AddTag(PaldarkGameplayTags::TAG_Paldark_Ability_PalAttack);
	AbilityTags.AddTag(PaldarkGameplayTags::TAG_Paldark_Ability_PalAttack_FireBreath);

	// Per-target damage is lower than single-target abilities because the
	// cone can hit multiple actors.
	FallbackBaseDamage = 10.f;
}

void UPaldarkGameplayAbility_PalAttack_FireBreath::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* SourceASC = ActorInfo != nullptr ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	AActor*                  Avatar    = ActorInfo != nullptr ? ActorInfo->AvatarActor.Get()           : nullptr;
	APaldarkPalCharacter*    Pal       = Cast<APaldarkPalCharacter>(Avatar);

	if (SourceASC == nullptr || Pal == nullptr || DamageEffectClass == nullptr)
	{
		UE_LOG(LogPaldarkGAS, Verbose,
			TEXT("UPaldarkGameplayAbility_PalAttack_FireBreath — missing ASC/Pal/DamageEffectClass; cone skipped."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UWorld* World = Pal->GetWorld();
	if (World == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UPaldarkPalCombatComponent* CombatComp = Pal->GetCombatSlot();
	const float Damage = (CombatComp != nullptr && CombatComp->BasePalDamage > 0.f)
		? CombatComp->BasePalDamage
		: FallbackBaseDamage;

	const FVector  ConeOrigin    = Pal->GetActorLocation();
	const FVector  ConeForward   = Pal->GetActorForwardVector();
	const float    CosHalfAngle  = FMath::Cos(FMath::DegreesToRadians(ConeHalfAngleDeg));
	const FGameplayTag OwnTeamTag = Pal->GetTeamTag();

	// Single sphere overlap covers the cone bounding volume; we cone-filter
	// in code. This avoids the per-call cost of FCollisionShape::MakeCone
	// which doesn't exist as a native shape anyway.
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PaldarkFireBreathCone), /*bTraceComplex=*/false);
	Params.AddIgnoredActor(Pal);
	World->OverlapMultiByObjectType(
		Overlaps,
		ConeOrigin,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(ConeRangeCM),
		Params);

	int32 HitCount = 0;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (HitCount >= MaxConeTargets)
		{
			break;
		}

		AActor* Candidate = Overlap.GetActor();
		if (Candidate == nullptr || Candidate == Pal || !IsValid(Candidate))
		{
			continue;
		}

		// Friendly-fire filter — skip actors that share our team tag.
		if (APaldarkPalCharacter* PalCandidate = Cast<APaldarkPalCharacter>(Candidate))
		{
			if (PalCandidate->GetTeamTag() == OwnTeamTag)
			{
				continue;
			}
		}

		// Cone filter — drop anything outside the half-angle.
		FVector ToTarget = (Candidate->GetActorLocation() - ConeOrigin);
		ToTarget.Z = 0.f;
		ToTarget = ToTarget.GetSafeNormal();
		const float Dot = FVector::DotProduct(ConeForward, ToTarget);
		if (Dot < CosHalfAngle)
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = ResolveTargetASC(Candidate);
		if (TargetASC == nullptr)
		{
			continue;
		}

		FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
		Ctx.AddSourceObject(Avatar);
		Ctx.AddInstigator(Avatar, Avatar);

		const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), Ctx);
		if (!SpecHandle.IsValid())
		{
			continue;
		}
		SpecHandle.Data->SetSetByCallerMagnitude(PaldarkGameplayTags::TAG_Paldark_SetByCaller_PalDamage, Damage);
		SpecHandle.Data->SetSetByCallerMagnitude(PaldarkGameplayTags::TAG_Paldark_SetByCaller_HeadshotMultiplier, 1.f);
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		++HitCount;
	}

	UE_LOG(LogPaldarkGAS, Log,
		TEXT("UPaldarkGameplayAbility_PalAttack_FireBreath — pal=%s cone targets=%d damage=%.1f"),
		*Pal->GetName(),
		HitCount,
		Damage);

	EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
}
