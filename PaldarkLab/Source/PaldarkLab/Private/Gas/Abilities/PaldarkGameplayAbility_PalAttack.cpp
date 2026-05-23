#include "Gas/Abilities/PaldarkGameplayAbility_PalAttack.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"

#include "Pal/Components/PaldarkPalCombatComponent.h"
#include "Pal/PaldarkPalCharacter.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"

UPaldarkGameplayAbility_PalAttack::UPaldarkGameplayAbility_PalAttack()
{
	// No input tag — Pal abilities fire from AI, not from input bindings.
	DebugName = TEXT("PalAttack");

	AbilityTags.AddTag(PaldarkGameplayTags::TAG_Paldark_Ability_PalAttack);

	// Server-only execution; clients see the damage GE land via attribute
	// replication.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UPaldarkGameplayAbility_PalAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	UAbilitySystemComponent* SourceASC = ActorInfo != nullptr ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	AActor*                  Avatar    = ActorInfo != nullptr ? ActorInfo->AvatarActor.Get()           : nullptr;
	if (SourceASC == nullptr || Avatar == nullptr)
	{
		UE_LOG(LogPaldarkGAS, Warning,
			TEXT("UPaldarkGameplayAbility_PalAttack::ActivateAbility — missing ASC/avatar (asc=%s avatar=%s)"),
			SourceASC != nullptr ? TEXT("yes") : TEXT("no"),
			Avatar != nullptr ? TEXT("yes") : TEXT("no"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Resolve the target via the combat component (perception's pick).
	APaldarkPalCharacter* Pal = Cast<APaldarkPalCharacter>(Avatar);
	UPaldarkPalCombatComponent* CombatComp = Pal != nullptr ? Pal->GetCombatSlot() : nullptr;
	if (CombatComp == nullptr)
	{
		// Could be a non-Pal avatar (e.g. designer granted this to a player
		// pawn for testing) — fall back to ability instigator.
		UE_LOG(LogPaldarkGAS, Verbose,
			TEXT("UPaldarkGameplayAbility_PalAttack::ActivateAbility — avatar %s is not APaldarkPalCharacter."),
			*GetNameSafe(Avatar));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	AActor* Target = CombatComp->GetCurrentTarget();
	if (Target == nullptr || !IsValid(Target) || Target->IsActorBeingDestroyed())
	{
		UE_LOG(LogPaldarkGAS, Verbose,
			TEXT("UPaldarkGameplayAbility_PalAttack::ActivateAbility — pal=%s has no valid target."),
			*Pal->GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// Resolve target ASC. Pals + dummies own their ASC directly via
	// IAbilitySystemInterface; player pawns route through PlayerState.
	UAbilitySystemComponent* TargetASC = nullptr;
	if (IAbilitySystemInterface* AsiActor = Cast<IAbilitySystemInterface>(Target))
	{
		TargetASC = AsiActor->GetAbilitySystemComponent();
	}
	if (TargetASC == nullptr)
	{
		if (const APawn* AsPawn = Cast<APawn>(Target))
		{
			if (APlayerState* PS = AsPawn->GetPlayerState())
			{
				if (IAbilitySystemInterface* AsiPs = Cast<IAbilitySystemInterface>(PS))
				{
					TargetASC = AsiPs->GetAbilitySystemComponent();
				}
			}
		}
	}

	if (TargetASC == nullptr)
	{
		UE_LOG(LogPaldarkGAS, Verbose,
			TEXT("UPaldarkGameplayAbility_PalAttack::ActivateAbility — pal=%s target=%s has no ASC."),
			*Pal->GetName(),
			*Target->GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (DamageEffectClass == nullptr)
	{
		// Designer hasn't wired the damage GE yet. Log + end so the activity
		// can keep ticking without applying any effect.
		UE_LOG(LogPaldarkGAS, Verbose,
			TEXT("UPaldarkGameplayAbility_PalAttack::ActivateAbility — pal=%s no DamageEffectClass (placeholder fire)."),
			*Pal->GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	const float Damage = CombatComp->BasePalDamage > 0.f
		? CombatComp->BasePalDamage
		: FallbackBaseDamage;

	FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
	Ctx.AddSourceObject(Avatar);
	Ctx.AddInstigator(Avatar, Avatar);

	const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(
		DamageEffectClass,
		GetAbilityLevel(),
		Ctx);

	if (!SpecHandle.IsValid())
	{
		UE_LOG(LogPaldarkGAS, Warning,
			TEXT("UPaldarkGameplayAbility_PalAttack::ActivateAbility — MakeOutgoingSpec returned invalid for %s."),
			*GetNameSafe(DamageEffectClass));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// Write PalDamage to its own SetByCaller key so the damage execution can
	// distinguish "Pal hit me" from "player hit me" and keep designer-side
	// tuning independent. The execution sums BaseDamage + PalDamage so a
	// shared damage GE works for both sources.
	SpecHandle.Data->SetSetByCallerMagnitude(
		PaldarkGameplayTags::TAG_Paldark_SetByCaller_PalDamage, Damage);

	// Pals never headshot — the multiplier defaults to 1 in the execution
	// when no SetByCaller key is present, but set it explicitly so future
	// readers don't wonder.
	SpecHandle.Data->SetSetByCallerMagnitude(
		PaldarkGameplayTags::TAG_Paldark_SetByCaller_HeadshotMultiplier, 1.0f);

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

	UE_LOG(LogPaldarkGAS, Log,
		TEXT("UPaldarkGameplayAbility_PalAttack — pal=%s target=%s damage=%.1f"),
		*Pal->GetName(),
		*Target->GetName(),
		Damage);

	EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
}
