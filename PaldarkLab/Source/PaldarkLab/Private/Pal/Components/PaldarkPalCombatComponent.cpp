#include "Pal/Components/PaldarkPalCombatComponent.h"

#include "AbilitySystemInterface.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameplayAbilitySpec.h"

#include "Gas/Abilities/PaldarkGameplayAbility_PalAttack.h"
#include "Gas/PaldarkAbilitySystemComponent.h"
#include "Pal/Components/PaldarkPalPerceptionComponent.h"
#include "Pal/PaldarkHostilePackSubsystem.h"
#include "Pal/PaldarkPalCharacter.h"
#include "PaldarkLogCategories.h"

UPaldarkPalCombatComponent::UPaldarkPalCombatComponent()
{
	// Component does NOT tick — the Combat activity drives `TryFireAttack`
	// at its own cadence. Replication off (server-only state).
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UPaldarkPalCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	PalOwner = Cast<APaldarkPalCharacter>(GetOwner());
	if (!PalOwner.IsValid())
	{
		UE_LOG(LogPaldarkPal, Warning,
			TEXT("UPaldarkPalCombatComponent::BeginPlay — owner is not APaldarkPalCharacter (%s); combat disabled."),
			*GetNameSafe(GetOwner()));
		return;
	}

	// Bind to perception's threat-changed multicast so the target pointer
	// follows the perception's pick. Resolving via FindComponentByClass —
	// the Pal owns both component slots so this is a single-step lookup.
	if (UPaldarkPalPerceptionComponent* Perception = PalOwner->FindComponentByClass<UPaldarkPalPerceptionComponent>())
	{
		PerceptionRef = Perception;
		Perception->OnThreatChanged.AddUObject(this, &UPaldarkPalCombatComponent::HandleThreatChanged);
	}

	// Ability granting is authority-only. The Pal's ASC is owned by the
	// Pal pawn (self-hosted, no PlayerState path — established in W7-8).
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	if (IAbilitySystemInterface* AsAsi = Cast<IAbilitySystemInterface>(PalOwner.Get()))
	{
		CachedASC = Cast<UPaldarkAbilitySystemComponent>(AsAsi->GetAbilitySystemComponent());
	}

	if (!CachedASC.IsValid())
	{
		UE_LOG(LogPaldarkPal, Warning,
			TEXT("UPaldarkPalCombatComponent::BeginPlay — Pal %s has no UPaldarkAbilitySystemComponent; combat disabled."),
			*GetNameSafe(GetOwner()));
		return;
	}

	if (AttackAbilityClass.IsNull())
	{
		// Designer didn't author an ability — combat still "works" (the
		// activity will try to fire and the component returns false), but
		// no damage will land. Log so the missing wiring is loud.
		UE_LOG(LogPaldarkPal, Warning,
			TEXT("UPaldarkPalCombatComponent::BeginPlay — Pal %s has no AttackAbilityClass; combat will activate but apply no damage."),
			*GetNameSafe(GetOwner()));
		return;
	}

	TSubclassOf<UPaldarkGameplayAbility_PalAttack> LoadedClass = AttackAbilityClass.LoadSynchronous();
	if (!LoadedClass)
	{
		UE_LOG(LogPaldarkPal, Warning,
			TEXT("UPaldarkPalCombatComponent::BeginPlay — failed to load AttackAbilityClass=%s for Pal %s."),
			*AttackAbilityClass.ToString(),
			*GetNameSafe(GetOwner()));
		return;
	}

	// Grant the ability. SourceObject = this component so the ability can
	// FindComponentByClass back to us if it needs the target pointer (see
	// `GetCurrentTarget`).
	FGameplayAbilitySpec Spec(LoadedClass, /*Level=*/1, /*InputID=*/INDEX_NONE, /*SourceObject=*/this);
	GrantedAttackSpec = CachedASC->GiveAbility(Spec);

	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkPalCombatComponent::BeginPlay — granted attack ability %s to Pal %s (interval=%.2fs range=[%.1f, %.1f] damage=%.1f)"),
		*LoadedClass->GetName(),
		*GetNameSafe(GetOwner()),
		AttackInterval,
		MinEngageRange,
		MaxEngageRange,
		BasePalDamage);
}

void UPaldarkPalCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UPaldarkPalPerceptionComponent* Perception = PerceptionRef.Get())
	{
		Perception->OnThreatChanged.RemoveAll(this);
	}
	if (UPaldarkAbilitySystemComponent* ASC = CachedASC.Get())
	{
		if (GrantedAttackSpec.IsValid())
		{
			ASC->ClearAbility(GrantedAttackSpec);
		}
	}
	CurrentTargetActor.Reset();
	Super::EndPlay(EndPlayReason);
}

bool UPaldarkPalCombatComponent::IsAttackReady() const
{
	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}
	return (World->GetTimeSeconds() - LastAttackTime) >= AttackInterval;
}

float UPaldarkPalCombatComponent::GetCooldownRemaining() const
{
	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return 0.f;
	}
	const float Elapsed = World->GetTimeSeconds() - LastAttackTime;
	return FMath::Max(0.f, AttackInterval - Elapsed);
}

bool UPaldarkPalCombatComponent::TryFireAttack()
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority())
	{
		return false;
	}
	if (!IsAttackReady())
	{
		return false;
	}

	AActor* Target = CurrentTargetActor.Get();
	if (Target == nullptr || !IsValid(Target) || Target->IsActorBeingDestroyed())
	{
		return false;
	}

	if (!PalOwner.IsValid())
	{
		return false;
	}

	const float DistanceToTarget = FVector::Dist(Target->GetActorLocation(), PalOwner->GetActorLocation());
	if (DistanceToTarget > MaxEngageRange)
	{
		return false;
	}

	UPaldarkAbilitySystemComponent* ASC = CachedASC.Get();
	if (ASC == nullptr || !GrantedAttackSpec.IsValid())
	{
		return false;
	}

	const bool bActivated = ASC->TryActivateAbility(GrantedAttackSpec);
	if (bActivated)
	{
		LastAttackTime = GetWorld()->GetTimeSeconds();
		UE_LOG(LogPaldarkPal, Verbose,
			TEXT("UPaldarkPalCombatComponent::TryFireAttack — pal=%s target=%s distance=%.1f activated=1"),
			*PalOwner->GetName(),
			*Target->GetName(),
			DistanceToTarget);
	}
	else
	{
		UE_LOG(LogPaldarkPal, Verbose,
			TEXT("UPaldarkPalCombatComponent::TryFireAttack — pal=%s target=%s distance=%.1f activated=0 (TryActivateAbility refused)"),
			*PalOwner->GetName(),
			*Target->GetName(),
			DistanceToTarget);
	}
	return bActivated;
}

void UPaldarkPalCombatComponent::HandleThreatChanged(AActor* /*OldThreat*/, AActor* NewThreat)
{
	CurrentTargetActor = NewThreat;
	UE_LOG(LogPaldarkPal, Verbose,
		TEXT("UPaldarkPalCombatComponent::HandleThreatChanged — pal=%s target=%s"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(NewThreat));

	// W20-21 — Pack broadcast. If the Pal belongs to a hostile pack, share
	// the sighting with packmates inside `PackBroadcastRadius`. The
	// subsystem handles the feedback-loop guard (skip packmates that
	// already see the same target). Lone Pals (PackTag empty) early-out
	// inside the subsystem.
	if (NewThreat == nullptr || !PalOwner.IsValid())
	{
		return;
	}
	if (const UWorld* World = GetWorld())
	{
		if (UPaldarkHostilePackSubsystem* PackSub = World->GetSubsystem<UPaldarkHostilePackSubsystem>())
		{
			PackSub->BroadcastPackThreat(PalOwner.Get(), NewThreat);
		}
	}
}

void UPaldarkPalCombatComponent::DumpToLog() const
{
	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkPalCombatComponent[%s] target=%s ready=%d cooldown_remaining=%.2f base_damage=%.1f range=[%.1f, %.1f] ability_class=%s"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(CurrentTargetActor.Get()),
		IsAttackReady() ? 1 : 0,
		GetCooldownRemaining(),
		BasePalDamage,
		MinEngageRange,
		MaxEngageRange,
		*AttackAbilityClass.ToString());
}
