// PALDARK W27 — UPaldarkAnimInstance (base AnimInstance, Lyra-style).
//
// Closes Q1 K-01 / Q2 L-06 (AnimBP T-pose risk) on the **code side**. The
// remaining work to actually see animations in PIE is designer-side:
// (1) inherit `ABP_Paldark_Player` from this class in the editor,
// (2) author a State Machine (Idle ↔ Locomotion ↔ Jump/Fall) reading the
// `Speed / Direction / bIsInAir / bIsAccelerating / bIsSprinting / bShouldMove`
// properties below,
// (3) plug a Blend Space (walk / run) keyed on `Speed` + `Direction`.
//
// Why a custom subclass:
// - The Pal subclass needs `bIsHostile / bIsFlying / bIsInCombat / SpeciesTag`
//   without forcing the designer to query gameplay tags from the AnimBP.
// - We want the **Lyra thread-safe snapshot** pattern: the game-thread
//   `NativeUpdateAnimation` captures a `FPaldarkAnimSnapshot` from the owning
//   `ACharacter` + ASC + (Pal-only) ActivityComponent, then the worker-thread
//   `NativeThreadSafeUpdateAnimation` derives the property fields that the
//   parallel AnimGraph reads. The worker side never touches a `UObject*`, so
//   parallel evaluation stays safe.
//
// Snapshot policy:
//   FPaldarkAnimSnapshot holds **primitive values + tags only**. Adding new
//   fields that hold raw `UObject*` is forbidden — the entire reason to split
//   GT vs WT is to keep WT off UObject memory.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayTagContainer.h"
#include "PaldarkAnimInstance.generated.h"

class ACharacter;
class UCharacterMovementComponent;
class UPaldarkAbilitySystemComponent;

// W27 — Thread snapshot. Game thread (`NativeUpdateAnimation`) writes; worker
// thread (`NativeThreadSafeUpdateAnimation`) reads. Primitive-only; do NOT add
// `UObject*` fields — that's the whole point of the split.
USTRUCT()
struct FPaldarkAnimSnapshot
{
	GENERATED_BODY()

	FVector  Velocity         = FVector::ZeroVector;
	FVector  Acceleration     = FVector::ZeroVector;
	FRotator BaseRotation     = FRotator::ZeroRotator;
	FRotator ControlRotation  = FRotator::ZeroRotator;
	uint8    MovementMode     = 0; // EMovementMode cast to uint8 on GT.
	bool     bIsCrouched      = false;
	bool     bIsFalling       = false;
	bool     bHasSprintingTag = false;
	bool     bIsHostileTeam   = false;

	// Pal-only — populated by `UPaldarkPalAnimInstance::NativeUpdateAnimation`.
	// Base class leaves it empty.
	FGameplayTag CurrentActivityTag;
};

UCLASS(BlueprintType, Abstract)
class PALDARKLAB_API UPaldarkAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UPaldarkAnimInstance();

	// AnimGraph-facing property fields. All read by State Machine transitions,
	// Blend Space `X / Y` inputs, and Boolean booleans driving "Should Move"
	// / "In Air" branches. Designers must NOT write to these from BP — they
	// are derived from the snapshot every frame.
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Anim", Transient)
	float Speed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Anim", Transient)
	float Direction = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Anim", Transient)
	float LeanAngle = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Anim", Transient)
	bool bIsInAir = false;

	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Anim", Transient)
	bool bIsAccelerating = false;

	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Anim", Transient)
	bool bIsCrouching = false;

	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Anim", Transient)
	bool bIsSprinting = false;

	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Anim", Transient)
	bool bShouldMove = false;

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

	// W27 — Hook for subclasses (Pal) to extend the GT snapshot. Called from
	// `NativeUpdateAnimation` after the base class fills the common fields.
	// Implementations must NOT cache the `ACharacter*` between frames; the
	// `CachedCharacter` weak ptr is already maintained by the base class.
	virtual void GatherSubclassSnapshot_GameThread(float DeltaSeconds) {}

	// W27 — Worker-thread hook. Called after the base derives Speed / Direction
	// / etc. Subclass writes its own AnimGraph-facing property fields here.
	virtual void DeriveSubclassAnimProperties_ThreadSafe(float DeltaSeconds) {}

	// Snapshot shared with subclasses. Subclasses extend it indirectly via
	// `GatherSubclassSnapshot_GameThread`; they must add their own snapshot
	// fields as protected members on the subclass (NOT mutate this struct's
	// layout — keep it stable so AnimGraph nodes referencing it via reflection
	// don't break across hot reload).
	FPaldarkAnimSnapshot Snapshot;

	// Cached on `NativeInitializeAnimation`. WeakObjectPtr so a destroyed
	// owner doesn't dangle. GT-only access — never dereferenced from
	// `NativeThreadSafeUpdateAnimation`.
	TWeakObjectPtr<ACharacter> CachedCharacter;

	// Resolved once on init via IAbilitySystemInterface on the player state /
	// pawn. Used GT-side to query the sprinting state tag.
	TWeakObjectPtr<UPaldarkAbilitySystemComponent> CachedAbilitySystem;

	// `Paldark.State.Sprinting` resolved on init so the GT loop doesn't hit
	// the tag registry every frame.
	FGameplayTag SprintingStateTag;
};
