// PALDARK W27 — Pal-specific AnimInstance.
//
// Extends `UPaldarkAnimInstance` with the four Pal-only fields the AnimBP
// needs to switch between the friendly / hostile poses, the flying loop
// (Razorbird), and the combat-stance blend. All four are derived in the
// thread-safe pass from the snapshot the GT pass captures.
//
// Designer-side: subclass this in `ABP_Paldark_Pal`, then `ABP_Paldark_Hostile_Direhound`
// / `ABP_Paldark_Hostile_Razorbird` inherit it and override the
// `Paldark|Anim|Layer` interface functions (see PaldarkAnimLayerInterface.h).

#pragma once

#include "CoreMinimal.h"
#include "Anim/PaldarkAnimInstance.h"
#include "GameplayTagContainer.h"
#include "PaldarkPalAnimInstance.generated.h"

class APaldarkPalCharacter;
class UPaldarkPalActivityComponent;

UCLASS(BlueprintType)
class PALDARKLAB_API UPaldarkPalAnimInstance : public UPaldarkAnimInstance
{
	GENERATED_BODY()

public:
	// True when the owning Pal's `TeamTag` matches `Paldark.Team.Hostile`
	// (W18-19 perception). Drives the AnimBP "Stalk" vs "Follow" sub-tree.
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Anim|Pal", Transient)
	bool bIsHostile = false;

	// True when the owning Pal's CharacterMovement is in `MOVE_Flying`
	// (Razorbird default). Drives the AnimBP flap loop vs ground locomotion.
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Anim|Pal", Transient)
	bool bIsFlying = false;

	// True when the current Activity tag is `Paldark.Pal.Activity.Combat`
	// (W18-19 utility AI). Drives the AnimBP combat-stance additive.
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Anim|Pal", Transient)
	bool bIsInCombat = false;

	// Mirror of the activity component's `GetCurrentActivityTag()`. Exposed so
	// the AnimBP can switch on the full tag enum (Idle / Follow / Investigate
	// / Patrol / Stalk / Combat / FollowSquadCommand) when designers want
	// fine-grained transitions.
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Anim|Pal", Transient)
	FGameplayTag CurrentActivityTag;

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void GatherSubclassSnapshot_GameThread(float DeltaSeconds) override;
	virtual void DeriveSubclassAnimProperties_ThreadSafe(float DeltaSeconds) override;

	TWeakObjectPtr<APaldarkPalCharacter>         CachedPalCharacter;
	TWeakObjectPtr<UPaldarkPalActivityComponent> CachedActivityComponent;

	// Cached during init so the snapshot doesn't pay the tag-registry cost
	// every frame.
	FGameplayTag CombatActivityTag;
	FGameplayTag HostileTeamTag;
};
