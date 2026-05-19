#include "Gas/Abilities/PaldarkGameplayAbility_PalAttack_Charge.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

#include "Pal/Components/PaldarkPalCombatComponent.h"
#include "Pal/PaldarkPalCharacter.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"

UPaldarkGameplayAbility_PalAttack_Charge::UPaldarkGameplayAbility_PalAttack_Charge()
{
	DebugName = TEXT("PalAttack.Charge");

	AbilityTags.Reset();
	AbilityTags.AddTag(PaldarkGameplayTags::TAG_Paldark_Ability_PalAttack);
	AbilityTags.AddTag(PaldarkGameplayTags::TAG_Paldark_Ability_PalAttack_Charge);

	// Heavy melee — Stoneclad bites are slow but bite hard.
	FallbackBaseDamage = 28.f;
}

void UPaldarkGameplayAbility_PalAttack_Charge::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// Resolve avatar + target BEFORE the parent ends the ability so we can
	// apply the knockback on the same frame as the damage.
	AActor* Avatar = ActorInfo != nullptr ? ActorInfo->AvatarActor.Get() : nullptr;
	APaldarkPalCharacter* Pal = Cast<APaldarkPalCharacter>(Avatar);
	UPaldarkPalCombatComponent* CombatComp = Pal != nullptr ? Pal->GetCombatSlot() : nullptr;
	AActor* Target = CombatComp != nullptr ? CombatComp->GetCurrentTarget() : nullptr;

	// Chain the inherited single-target damage path first. The parent
	// handles CommitAbility + EndAbility itself; if Commit fails, the
	// parent already ended the ability and the impulse below early-outs.
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (Avatar == nullptr || Target == nullptr || !IsValid(Target))
	{
		return;
	}

	APawn* TargetPawn = Cast<APawn>(Target);
	if (TargetPawn == nullptr)
	{
		return;
	}

	ACharacter* TargetCharacter = Cast<ACharacter>(TargetPawn);
	UCharacterMovementComponent* Movement = TargetCharacter != nullptr ? TargetCharacter->GetCharacterMovement() : nullptr;
	if (Movement == nullptr)
	{
		UE_LOG(LogPaldarkGAS, Verbose,
			TEXT("UPaldarkGameplayAbility_PalAttack_Charge — target %s has no CharacterMovement; skipping impulse."),
			*Target->GetName());
		return;
	}

	const FVector AvatarLoc = Avatar->GetActorLocation();
	const FVector TargetLoc = Target->GetActorLocation();
	FVector Direction = (TargetLoc - AvatarLoc);
	Direction.Z = 0.f;
	Direction = Direction.GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		return;
	}

	Direction.Z = FMath::Clamp(VerticalLiftRatio, 0.f, 1.f);

	const FVector Launch = Direction * KnockbackImpulse;
	TargetCharacter->LaunchCharacter(Launch, /*bXYOverride=*/false, /*bZOverride=*/false);

	UE_LOG(LogPaldarkGAS, Log,
		TEXT("UPaldarkGameplayAbility_PalAttack_Charge — pal=%s target=%s impulse=%.0f"),
		*Pal->GetName(),
		*Target->GetName(),
		KnockbackImpulse);
}
