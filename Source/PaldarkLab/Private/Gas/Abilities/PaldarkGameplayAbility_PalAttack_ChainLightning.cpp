#include "Gas/Abilities/PaldarkGameplayAbility_PalAttack_ChainLightning.h"

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

	// Apply the per-species damage GE to a single target. Returns true if
	// the application went through (used by the bounce loop to count the
	// hit before walking to the next link).
	bool ApplyChainDamage(
		UAbilitySystemComponent* SourceASC,
		AActor* Avatar,
		AActor* Target,
		TSubclassOf<UGameplayEffect> DamageEffectClass,
		float Damage,
		int32 AbilityLevel)
	{
		UAbilitySystemComponent* TargetASC = ResolveTargetASC(Target);
		if (TargetASC == nullptr || SourceASC == nullptr)
		{
			return false;
		}

		FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
		Ctx.AddSourceObject(Avatar);
		Ctx.AddInstigator(Avatar, Avatar);

		const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, AbilityLevel, Ctx);
		if (!SpecHandle.IsValid())
		{
			return false;
		}
		SpecHandle.Data->SetSetByCallerMagnitude(PaldarkGameplayTags::TAG_Paldark_SetByCaller_PalDamage, Damage);
		SpecHandle.Data->SetSetByCallerMagnitude(PaldarkGameplayTags::TAG_Paldark_SetByCaller_HeadshotMultiplier, 1.f);
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		return true;
	}
}

UPaldarkGameplayAbility_PalAttack_ChainLightning::UPaldarkGameplayAbility_PalAttack_ChainLightning()
{
	DebugName = TEXT("PalAttack.ChainLightning");

	AbilityTags.Reset();
	AbilityTags.AddTag(PaldarkGameplayTags::TAG_Paldark_Ability_PalAttack);
	AbilityTags.AddTag(PaldarkGameplayTags::TAG_Paldark_Ability_PalAttack_ChainLightning);

	// Boss-tier per-hit damage — Boltmane bounces hurt more even before
	// the chain bonus.
	FallbackBaseDamage = 18.f;
}

void UPaldarkGameplayAbility_PalAttack_ChainLightning::ActivateAbility(
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
	UPaldarkPalCombatComponent* CombatComp = Pal != nullptr ? Pal->GetCombatSlot() : nullptr;
	AActor* PrimaryTarget = CombatComp != nullptr ? CombatComp->GetCurrentTarget() : nullptr;

	if (SourceASC == nullptr || Pal == nullptr || PrimaryTarget == nullptr || DamageEffectClass == nullptr)
	{
		UE_LOG(LogPaldarkGAS, Verbose,
			TEXT("UPaldarkGameplayAbility_PalAttack_ChainLightning — missing ASC/Pal/target/DamageEffectClass; chain skipped."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UWorld* World = Pal->GetWorld();
	if (World == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	float Damage = (CombatComp->BasePalDamage > 0.f) ? CombatComp->BasePalDamage : FallbackBaseDamage;
	const FGameplayTag OwnTeamTag = Pal->GetTeamTag();

	TSet<AActor*> Hit;
	Hit.Add(Pal);
	AActor* CurrentNode = PrimaryTarget;

	for (int32 Bounce = 0; Bounce <= MaxBounces; ++Bounce)
	{
		if (CurrentNode == nullptr || !IsValid(CurrentNode))
		{
			break;
		}

		if (!ApplyChainDamage(SourceASC, Avatar, CurrentNode, DamageEffectClass, Damage, GetAbilityLevel()))
		{
			break;
		}
		Hit.Add(CurrentNode);

		if (Bounce == MaxBounces)
		{
			break;
		}

		// Find the nearest unbounced hostile pawn within BounceRangeCM of
		// the current node.
		const FVector NodeLoc = CurrentNode->GetActorLocation();
		TArray<FOverlapResult> Overlaps;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(PaldarkChainLightningBounce), /*bTraceComplex=*/false);
		Params.AddIgnoredActor(Pal);
		World->OverlapMultiByObjectType(
			Overlaps,
			NodeLoc,
			FQuat::Identity,
			FCollisionObjectQueryParams(ECC_Pawn),
			FCollisionShape::MakeSphere(BounceRangeCM),
			Params);

		AActor* NextNode    = nullptr;
		float   NextDistSqr = TNumericLimits<float>::Max();
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* Candidate = Overlap.GetActor();
			if (Candidate == nullptr || Hit.Contains(Candidate))
			{
				continue;
			}
			if (APaldarkPalCharacter* PalCandidate = Cast<APaldarkPalCharacter>(Candidate))
			{
				if (PalCandidate->GetTeamTag() == OwnTeamTag)
				{
					continue;
				}
			}
			const float DistSqr = FVector::DistSquared(NodeLoc, Candidate->GetActorLocation());
			if (DistSqr < NextDistSqr)
			{
				NextDistSqr = DistSqr;
				NextNode    = Candidate;
			}
		}

		if (NextNode == nullptr)
		{
			break;
		}

		CurrentNode  = NextNode;
		Damage      *= BounceFalloff;
	}

	UE_LOG(LogPaldarkGAS, Log,
		TEXT("UPaldarkGameplayAbility_PalAttack_ChainLightning — pal=%s links=%d primary=%s"),
		*Pal->GetName(),
		Hit.Num() - 1, // subtract the self entry
		*PrimaryTarget->GetName());

	EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
}
