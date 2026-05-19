#include "Gas/Abilities/PaldarkGameplayAbility_HitscanFire.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Camera/PlayerCameraManager.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"

#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"

UPaldarkGameplayAbility_HitscanFire::UPaldarkGameplayAbility_HitscanFire()
{
	// Tag-keyed input lookup — the character binds Paldark.InputTag.Fire to
	// this ability's activation tag via UPaldarkInputConfig.AbilityInputActions.
	ActivationInputTag = PaldarkGameplayTags::TAG_Paldark_InputTag_Fire;
	DebugName          = TEXT("HitscanFire");

	AbilityTags.AddTag(PaldarkGameplayTags::TAG_Paldark_Ability_Fire);

	// Server-only for W9-10. Damage is server-authoritative so the line
	// trace + GE apply both run on the authority. Promote to LocalPredicted
	// in W22+ once we have a non-instant montage to hide latency behind.
	NetExecutionPolicy   = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	InstancingPolicy     = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UPaldarkGameplayAbility_HitscanFire::ComputeTrace(
	const FGameplayAbilityActorInfo* ActorInfo,
	FVector& OutStart,
	FVector& OutEnd) const
{
	if (ActorInfo == nullptr)
	{
		return false;
	}

	const APawn* AvatarPawn = Cast<APawn>(ActorInfo->AvatarActor.Get());
	if (AvatarPawn == nullptr)
	{
		return false;
	}

	// Prefer the player camera (matches the visible aim) for human pawns;
	// fall back to controller view location or pawn forward for AI-driven
	// avatars so the same ability can be granted to a Pal later.
	FVector  ViewLoc = AvatarPawn->GetActorLocation();
	FRotator ViewRot = AvatarPawn->GetActorRotation();

	if (const APlayerController* PC = Cast<APlayerController>(AvatarPawn->GetController()))
	{
		if (const APlayerCameraManager* Cam = PC->PlayerCameraManager)
		{
			ViewLoc = Cam->GetCameraLocation();
			ViewRot = Cam->GetCameraRotation();
		}
		else
		{
			PC->GetPlayerViewPoint(ViewLoc, ViewRot);
		}
	}
	else if (const AController* Controller = AvatarPawn->GetController())
	{
		Controller->GetPlayerViewPoint(ViewLoc, ViewRot);
	}

	OutStart = ViewLoc;
	OutEnd   = ViewLoc + ViewRot.Vector() * FireRange;
	return true;
}

void UPaldarkGameplayAbility_HitscanFire::ActivateAbility(
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

	UWorld* World = ActorInfo != nullptr && ActorInfo->AvatarActor.IsValid()
		? ActorInfo->AvatarActor->GetWorld()
		: nullptr;
	UAbilitySystemComponent* SourceASC = ActorInfo != nullptr ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	AActor*                  Avatar    = ActorInfo != nullptr ? ActorInfo->AvatarActor.Get()           : nullptr;

	if (World == nullptr || SourceASC == nullptr || Avatar == nullptr)
	{
		UE_LOG(LogPaldarkGAS, Warning,
			TEXT("UPaldarkGameplayAbility_HitscanFire::ActivateAbility — missing world/ASC/avatar (world=%s asc=%s avatar=%s)"),
			World != nullptr ? TEXT("yes") : TEXT("no"),
			SourceASC != nullptr ? TEXT("yes") : TEXT("no"),
			Avatar != nullptr ? TEXT("yes") : TEXT("no"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FVector TraceStart = FVector::ZeroVector;
	FVector TraceEnd   = FVector::ZeroVector;
	if (!ComputeTrace(ActorInfo, TraceStart, TraceEnd))
	{
		UE_LOG(LogPaldarkGAS, Warning,
			TEXT("UPaldarkGameplayAbility_HitscanFire — failed to compute trace transform (avatar=%s)."),
			*GetNameSafe(Avatar));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// Trace channel = Visibility (default mannequin head/body collide on this
	// channel). Designers can extend with a project-specific WeaponTrace
	// channel later via DefaultEngine.ini.
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PaldarkHitscanFire), /*bTraceComplex=*/true);
	QueryParams.AddIgnoredActor(Avatar);

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(
		Hit,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams);

	if (bDrawDebugTrace)
	{
		DrawDebugLine(
			World,
			TraceStart,
			bHit ? Hit.ImpactPoint : TraceEnd,
			bHit ? FColor::Red : FColor::Green,
			/*bPersistentLines=*/false,
			/*LifeTime=*/2.0f,
			/*DepthPriority=*/0,
			/*Thickness=*/1.5f);
	}

	if (!bHit || Hit.GetActor() == nullptr)
	{
		UE_LOG(LogPaldarkGAS, Log,
			TEXT("UPaldarkGameplayAbility_HitscanFire — miss (start=%s end=%s)"),
			*TraceStart.ToCompactString(), *TraceEnd.ToCompactString());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	AActor* HitActor = Hit.GetActor();

	// Resolve the target ASC. Try the actor itself first (Pal / dummy own
	// their ASC) then fall back to the actor's PlayerState (player path).
	UAbilitySystemComponent* TargetASC = nullptr;
	if (IAbilitySystemInterface* AsiActor = Cast<IAbilitySystemInterface>(HitActor))
	{
		TargetASC = AsiActor->GetAbilitySystemComponent();
	}
	if (TargetASC == nullptr)
	{
		if (const APawn* AsPawn = Cast<APawn>(HitActor))
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

	const bool bHeadshot = !Hit.BoneName.IsNone() && Hit.BoneName == HeadBoneName;

	UE_LOG(LogPaldarkGAS, Log,
		TEXT("UPaldarkGameplayAbility_HitscanFire — hit=%s bone=%s headshot=%d distance=%.1f target_asc=%s"),
		*HitActor->GetName(),
		*Hit.BoneName.ToString(),
		bHeadshot ? 1 : 0,
		Hit.Distance,
		TargetASC != nullptr ? *TargetASC->GetName() : TEXT("<null>"));

	if (TargetASC == nullptr || DamageEffectClass == nullptr)
	{
		// Trace-only path (no GE configured) or non-GAS target. End cleanly so
		// the next Fire input can re-activate immediately.
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// Build the GE spec. Context carries the instigator so the AttributeSet's
	// OnHealthZeroed broadcast can attribute the kill.
	FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
	Ctx.AddSourceObject(Avatar);
	Ctx.AddInstigator(Avatar, Avatar);
	Ctx.AddHitResult(Hit);

	const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(
		DamageEffectClass,
		GetAbilityLevel(),
		Ctx);

	if (!SpecHandle.IsValid())
	{
		UE_LOG(LogPaldarkGAS, Warning,
			TEXT("UPaldarkGameplayAbility_HitscanFire — MakeOutgoingSpec returned invalid for %s."),
			*GetNameSafe(DamageEffectClass));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// SetByCaller magnitudes drive the damage formula in the execution calc.
	SpecHandle.Data->SetSetByCallerMagnitude(
		PaldarkGameplayTags::TAG_Paldark_SetByCaller_BaseDamage, BaseDamage);

	SpecHandle.Data->SetSetByCallerMagnitude(
		PaldarkGameplayTags::TAG_Paldark_SetByCaller_HeadshotMultiplier,
		bHeadshot ? HeadshotMultiplier : 1.0f);

	if (bHeadshot)
	{
		// Tag the spec so debug / observers can branch on it without
		// re-inspecting the hit result later.
		SpecHandle.Data->AddDynamicAssetTag(PaldarkGameplayTags::TAG_Paldark_Hit_Headshot);
	}

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

	EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
}
