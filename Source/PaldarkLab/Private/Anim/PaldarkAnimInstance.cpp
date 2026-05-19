// PALDARK W27 — UPaldarkAnimInstance impl.

#include "Anim/PaldarkAnimInstance.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Gas/PaldarkAbilitySystemComponent.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"

UPaldarkAnimInstance::UPaldarkAnimInstance()
{
	// Lyra parity: enable parallel update so the AnimGraph evaluates on a
	// worker. The thread-safe override below is what makes parallel safe.
	bUseMultiThreadedAnimationUpdate = true;
}

void UPaldarkAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (ACharacter* Character = Cast<ACharacter>(GetOwningActor()))
	{
		CachedCharacter = Character;

		// Resolve the ASC once. The pawn / its PlayerState implements
		// IAbilitySystemInterface (W7-8). Failure is benign — the sprinting
		// tag just stays false and the AnimGraph never enters the sprint
		// state.
		if (UAbilitySystemComponent* GenericASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Character))
		{
			if (UPaldarkAbilitySystemComponent* TypedASC = Cast<UPaldarkAbilitySystemComponent>(GenericASC))
			{
				CachedAbilitySystem = TypedASC;
			}
		}
	}

	SprintingStateTag = PaldarkGameplayTags::TAG_Paldark_State_Sprinting;

	UE_LOG(LogPaldark, Verbose,
		TEXT("UPaldarkAnimInstance::NativeInitializeAnimation — owner=%s ASC=%s sprint_tag=%s"),
		*GetNameSafe(GetOwningActor()),
		CachedAbilitySystem.IsValid() ? TEXT("yes") : TEXT("no"),
		*SprintingStateTag.ToString());
}

void UPaldarkAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// Snapshot only on game thread. Touching `Character->GetCharacterMovement()`
	// from a worker is unsafe under parallel anim update.
	const ACharacter* Character = CachedCharacter.Get();
	if (Character == nullptr)
	{
		return;
	}

	const UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	if (Move != nullptr)
	{
		Snapshot.Velocity     = Move->Velocity;
		Snapshot.Acceleration = Move->GetCurrentAcceleration();
		Snapshot.MovementMode = static_cast<uint8>(Move->MovementMode);
		Snapshot.bIsCrouched  = Character->bIsCrouched;
		Snapshot.bIsFalling   = Move->IsFalling();
	}

	Snapshot.BaseRotation = Character->GetActorRotation();
	if (const AController* Controller = Character->GetController())
	{
		Snapshot.ControlRotation = Controller->GetControlRotation();
	}
	else
	{
		Snapshot.ControlRotation = Snapshot.BaseRotation;
	}

	Snapshot.bHasSprintingTag = false;
	if (CachedAbilitySystem.IsValid() && SprintingStateTag.IsValid())
	{
		Snapshot.bHasSprintingTag = CachedAbilitySystem->HasMatchingGameplayTag(SprintingStateTag);
	}

	// Subclass extension hook for Pal-specific fields. Base class leaves
	// `CurrentActivityTag` empty.
	GatherSubclassSnapshot_GameThread(DeltaSeconds);
}

void UPaldarkAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	// Speed = horizontal velocity magnitude. AnimGraph uses this for the
	// Blend Space X (walk / run blend) and the State Machine's "Should Move"
	// transition.
	FVector HorizontalVelocity = Snapshot.Velocity;
	HorizontalVelocity.Z = 0.0f;
	Speed = HorizontalVelocity.Size();

	// Direction = signed yaw delta between velocity and actor facing, in
	// [-180, 180]. Matches the standard `UKismetAnimationLibrary::CalculateDirection`
	// without forcing a dependency on that module.
	if (HorizontalVelocity.IsNearlyZero())
	{
		Direction = 0.0f;
	}
	else
	{
		const FRotator VelocityRot = HorizontalVelocity.Rotation();
		const FRotator Delta       = VelocityRot - Snapshot.BaseRotation;
		Direction = FRotator::NormalizeAxis(Delta.Yaw);
	}

	bIsCrouching    = Snapshot.bIsCrouched;
	bIsInAir        = Snapshot.bIsFalling;
	bIsAccelerating = !Snapshot.Acceleration.IsNearlyZero();
	bIsSprinting    = Snapshot.bHasSprintingTag;

	// "Should Move" mirrors Lyra: accelerating AND grounded AND moving fast
	// enough to leave the Idle state. The 3 cm/s floor prevents jitter when
	// physics nudges the capsule by a frame.
	bShouldMove = bIsAccelerating && !bIsInAir && Speed > 3.0f;

	// Lean — naive proxy: lerp toward Direction scaled. Better Lyra-style
	// implementation (yaw history) is W30+ polish. Keeping a real field so the
	// AnimBP can hook it now and start blending without re-publishing the
	// AnimInstance later.
	LeanAngle = FMath::FInterpTo(LeanAngle, Direction * 0.25f, DeltaSeconds, 5.0f);

	DeriveSubclassAnimProperties_ThreadSafe(DeltaSeconds);
}
