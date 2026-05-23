#include "Combat/PaldarkDummyTarget.h"

#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "TimerManager.h"

#include "Gas/PaldarkAbilitySystemComponent.h"
#include "Gas/PaldarkAttributeSet.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"

APaldarkDummyTarget::APaldarkDummyTarget()
{
	bReplicates = true;
	SetReplicateMovement(true);

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		// The dummy is a stationary target — disable orientation steering so
		// the visualisation stays still while taking hits.
		Movement->bOrientRotationToMovement = false;
		Movement->MaxWalkSpeed              = 0.0f;
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		// Slightly chunkier capsule than the player so the line trace is
		// easier to hit while iterating on the formula.
		Capsule->InitCapsuleSize(45.0f, 96.0f);
	}

	// W9-10 — Self-owned ASC + AttributeSet. Minimal replication mode
	// because the dummy never owns a client-side prediction window (the
	// hitscan ability is server-authoritative).
	AbilitySystemComponent = CreateDefaultSubobject<UPaldarkAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UPaldarkAttributeSet>(TEXT("AttributeSet"));

	// W18-19 — Dummies default to hostile so the Pal perception scan picks
	// them up as a threat without an extra "make this dummy a target"
	// configuration step in the editor. Designer can flip to friendly per
	// dummy if they want a VFX test prop.
	TeamTag = PaldarkGameplayTags::TAG_Paldark_Team_Hostile;
}

UAbilitySystemComponent* APaldarkDummyTarget::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void APaldarkDummyTarget::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent != nullptr)
	{
		AbilitySystemComponent->InitAbilityActorInfo(/*OwnerActor=*/this, /*AvatarActor=*/this);
	}

	// Apply the init GE on the authority so Health / MaxHealth / Armor seed
	// to designer-authored values. Skip when the GE class is null — the
	// AttributeSet's constructor defaults (100/100/0) keep the dummy alive.
	if (HasAuthority() && AbilitySystemComponent != nullptr && InitAttributesEffect != nullptr)
	{
		FGameplayEffectContextHandle Ctx = AbilitySystemComponent->MakeEffectContext();
		Ctx.AddSourceObject(this);
		const FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(
			InitAttributesEffect, /*Level=*/1.0f, Ctx);
		if (Spec.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}

	// Bind the AttributeSet's OnHealthZeroed multicast on the authority. The
	// AttributeSet outlives the dummy actor by 1 GC tick so binding via a
	// raw multicast (non-UObject) handle is safe — we explicitly remove the
	// binding in EndPlay.
	if (HasAuthority() && AttributeSet != nullptr)
	{
		HealthZeroedHandle = AttributeSet->OnHealthZeroed.AddUObject(
			this, &APaldarkDummyTarget::HandleHealthZeroed);
	}

	UE_LOG(LogPaldarkGAS, Log,
		TEXT("APaldarkDummyTarget::BeginPlay — %s ready auth=%d health=%.1f/%.1f armor=%.1f init=%s"),
		*GetName(),
		HasAuthority() ? 1 : 0,
		AttributeSet != nullptr ? AttributeSet->GetHealth()    : -1.0f,
		AttributeSet != nullptr ? AttributeSet->GetMaxHealth() : -1.0f,
		AttributeSet != nullptr ? AttributeSet->GetArmor()     : -1.0f,
		*GetNameSafe(InitAttributesEffect));
}

void APaldarkDummyTarget::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AttributeSet != nullptr && HealthZeroedHandle.IsValid())
	{
		AttributeSet->OnHealthZeroed.Remove(HealthZeroedHandle);
		HealthZeroedHandle.Reset();
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DestroyTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void APaldarkDummyTarget::HandleHealthZeroed(UAbilitySystemComponent* Instigator)
{
	if (bIsDead)
	{
		return;
	}
	bIsDead = true;

	// Tag the ASC so HUD / debug / future activity selection can branch on
	// "this actor is dead". Loose-tagged so it survives GE re-evaluation.
	if (AbilitySystemComponent != nullptr)
	{
		AbilitySystemComponent->AddLooseGameplayTag(
			PaldarkGameplayTags::TAG_Paldark_State_IsDead);
	}

	UE_LOG(LogPaldarkGAS, Log,
		TEXT("APaldarkDummyTarget::HandleHealthZeroed — %s killed by %s"),
		*GetName(),
		Instigator != nullptr ? *GetNameSafe(Instigator->GetOwnerActor()) : TEXT("<unknown>"));

	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			5.0f,
			FColor::Yellow,
			FString::Printf(TEXT("Dummy down: %s"), *GetName()));
	}

	// Disable further damage / movement / collision so the corpse is inert
	// for the destroy-delay window.
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->DisableMovement();
	}

	if (UWorld* World = GetWorld())
	{
		// Schedule destroy — the timer manager handles deletion safely on
		// the next tick after the delay even if EndPlay tears us down first.
		World->GetTimerManager().SetTimer(
			DestroyTimerHandle,
			FTimerDelegate::CreateUObject(this, &AActor::K2_DestroyActor),
			FMath::Max(DestroyDelaySeconds, 0.05f),
			/*bLoop=*/false);
	}
}
