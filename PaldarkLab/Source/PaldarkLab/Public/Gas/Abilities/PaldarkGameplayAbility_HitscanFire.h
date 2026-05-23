// PALDARK W9-10 — Hitscan fire ability (pistol baseline).
//
// Outcome guarded by `Documents/PALDARK/03-Roadmap_1_Year.md` § Tuần 9–10:
//   "Player shoot dummy → dummy chết với damage chain (HP/Armor/Headshot)."
//
// Behaviour:
//   - ActivationInputTag = `Paldark.InputTag.Fire` so the Enhanced Input →
//     ability bridge can activate it via the same tag table the W7-8 Sprint
//     ability already uses.
//   - On Activate (server / standalone): trace a line from the camera forward,
//     find the first hit actor that exposes IAbilitySystemInterface, build a
//     GE spec from the designer-authored DamageEffectClass, fill in the
//     SetByCaller magnitudes (`BaseDamage` + `HeadshotMultiplier` when the
//     trace hit a head bone), and apply the spec to the target ASC.
//   - EndAbility immediately (one-shot, no hold semantics — semi-auto pistol).
//
// Authority model:
//   - The W9-10 path is server-authoritative: the ability runs on the server
//     only (NetExecutionPolicy = ServerOnly) so the line trace and damage
//     application both happen on the authority. Clients see Health attribute
//     replicate down through the standard GAS path.
//   - W11+ can promote this to predicted (LocalPredicted) once we have a
//     non-instant montage to play, but for W9-10 the one-frame damage path
//     is fine because there's no animation latency to hide.
//
// Future homes (do NOT implement here in W9-10):
//   - Recoil + bloom — W17-18 weapon polish.
//   - Reload / magazine — W19-20 weapon component.
//   - Predicted client trace — W22+ network polish.

#pragma once

#include "CoreMinimal.h"
#include "Gas/PaldarkGameplayAbility.h"
#include "PaldarkGameplayAbility_HitscanFire.generated.h"

class UGameplayEffect;

UCLASS()
class PALDARKLAB_API UPaldarkGameplayAbility_HitscanFire : public UPaldarkGameplayAbility
{
	GENERATED_BODY()

public:
	UPaldarkGameplayAbility_HitscanFire();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// Designer-authored damage GE (Blueprint subclass of UGameplayEffect with
	// the UPaldarkDamageExecutionCalculation execution attached). When null,
	// the ability still runs the trace + draws debug but applies no damage —
	// useful for trace-only debugging.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Ability|Fire")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// Maximum hitscan distance in centimeters. 10 m default — matches the
	// W9-10 sandbox scale (dummy spawn offset is 5 m).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Ability|Fire")
	float FireRange = 10000.0f;

	// Base damage written into the SetByCaller magnitude
	// `Paldark.SetByCaller.BaseDamage` on the outgoing spec. The execution
	// calc applies armor mitigation + headshot multiplier on top.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Ability|Fire")
	float BaseDamage = 25.0f;

	// Multiplier applied when the trace hit a head bone (bone-name match
	// against `HeadBoneName`). Written into the SetByCaller magnitude
	// `Paldark.SetByCaller.HeadshotMultiplier`.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Ability|Fire")
	float HeadshotMultiplier = 2.0f;

	// Bone name we compare against the hit bone to decide whether the
	// HeadshotMultiplier applies. Defaults to "head" which matches the
	// standard UE5 mannequin / Manny / Quinn skeletons.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Ability|Fire")
	FName HeadBoneName = TEXT("head");

	// Draw a short debug line on the server when a trace runs. Defaults to
	// true in W9-10 so designers can verify the trace direction visually;
	// flip to false once we have weapon FX in W17-18.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Ability|Fire")
	bool bDrawDebugTrace = true;

	// Helper. Resolves the camera transform from the owning pawn's controller
	// (PlayerCameraManager preferred, falls back to pawn forward) and returns
	// the start + end of the hitscan trace.
	bool ComputeTrace(
		const FGameplayAbilityActorInfo* ActorInfo,
		FVector& OutStart,
		FVector& OutEnd) const;
};
