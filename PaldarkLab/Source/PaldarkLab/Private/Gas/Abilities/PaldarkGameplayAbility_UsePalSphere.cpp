#include "Gas/Abilities/PaldarkGameplayAbility_UsePalSphere.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"

#include "Pal/PaldarkPalSphere.h"
#include "Player/Components/PaldarkPlayerInventoryComponent.h"
#include "Player/PaldarkCharacter.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"

UPaldarkGameplayAbility_UsePalSphere::UPaldarkGameplayAbility_UsePalSphere()
{
	// Server-only activation policy. The inventory consume is the
	// authoritative side-effect; client-predicted activation would have to
	// rollback the predicted consume on a mis-predict, which is more
	// machinery than W35-36 needs. Defer prediction to W38 polish.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	ActivationInputTag = TAG_Paldark_Ability_UsePalSphere;
	DebugName = TEXT("UsePalSphere");
	PalSphereTierTag = TAG_Paldark_Item_PalSphere_T1;
	PalSphereClass = APaldarkPalSphere::StaticClass();

	// Tag this ability with its identity so designers can resolve it via
	// `TryActivateAbilitiesByTag` from a quickslot binding.
	AbilityTags.AddTag(TAG_Paldark_Ability_UsePalSphere);
}

void UPaldarkGameplayAbility_UsePalSphere::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!ActorInfo || !ActorInfo->OwnerActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicate*/true, /*bWasCancelled*/true);
		return;
	}

	// Authority-only side effects — server consumes inventory + spawns the
	// projectile. The early return for non-authority is defensive (the GA
	// already declares ServerInitiated).
	APaldarkCharacter* Avatar = Cast<APaldarkCharacter>(ActorInfo->AvatarActor.Get());
	if (!Avatar)
	{
		UE_LOG(LogPaldarkPal, Warning,
			TEXT("UPaldarkGameplayAbility_UsePalSphere::ActivateAbility — avatar is not APaldarkCharacter"));
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicate*/true, /*bWasCancelled*/true);
		return;
	}

	if (Avatar->GetLocalRole() != ROLE_Authority)
	{
		// Non-authoritative activation — let server-side roll through.
		Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicate*/true, /*bWasCancelled*/false);
		return;
	}

	UPaldarkPlayerInventoryComponent* Inventory = Avatar->GetInventorySlot();
	if (!Inventory)
	{
		UE_LOG(LogPaldarkPal, Warning,
			TEXT("UPaldarkGameplayAbility_UsePalSphere::ActivateAbility — avatar %s missing inventory slot"),
			*Avatar->GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicate*/true, /*bWasCancelled*/true);
		return;
	}

	const int32 Removed = Inventory->RemoveItemByTag(PalSphereTierTag, 1);
	if (Removed <= 0)
	{
		UE_LOG(LogPaldarkPal, Log,
			TEXT("UPaldarkGameplayAbility_UsePalSphere::ActivateAbility — avatar %s has no %s in inventory"),
			*Avatar->GetName(), *PalSphereTierTag.ToString());
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicate*/true, /*bWasCancelled*/true);
		return;
	}

	if (!PalSphereClass.Get())
	{
		// Designer set the class to null in BP — fall back to the C++ base
		// so the throw still works in a barebones authoring environment.
		PalSphereClass = APaldarkPalSphere::StaticClass();
	}

	// Spawn transform — muzzle offset rotated into avatar forward space so
	// the sphere appears ahead of the player at chest height.
	const FVector AvatarLoc = Avatar->GetActorLocation();
	const FRotator AvatarRot = Avatar->GetActorRotation();
	const FVector SpawnLoc = AvatarLoc + AvatarRot.RotateVector(MuzzleOffset);
	const FTransform SpawnXf(AvatarRot, SpawnLoc);

	UWorld* World = Avatar->GetWorld();
	if (!World)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicate*/true, /*bWasCancelled*/true);
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = Avatar;
	Params.Instigator = Avatar;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	APaldarkPalSphere* Sphere = World->SpawnActorDeferred<APaldarkPalSphere>(
		PalSphereClass.Get(),
		SpawnXf,
		Avatar,
		Avatar,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

	if (!Sphere)
	{
		UE_LOG(LogPaldarkPal, Warning,
			TEXT("UPaldarkGameplayAbility_UsePalSphere::ActivateAbility — failed to deferred-spawn projectile"));
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicate*/true, /*bWasCancelled*/true);
		return;
	}

	Sphere->InitForThrow(Avatar, PalSphereTierTag);
	Sphere->FinishSpawning(SpawnXf);

	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkGameplayAbility_UsePalSphere::ActivateAbility — avatar=%s tier=%s spawned %s at %s"),
		*Avatar->GetName(),
		*PalSphereTierTag.ToString(),
		*Sphere->GetName(),
		*SpawnLoc.ToString());

	EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicate*/true, /*bWasCancelled*/false);
}
