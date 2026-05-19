// PALDARK W29-30 — Vinewraith (hostile) vine-whip ranged ability.
//
// Long-reach single-target attack — Vinewraith is the ranged hostile of
// the W29-30 roster, contrasting with Stoneclad (melee tank) and
// Boltmane (boss multi-target). The vine "whips out" mechanically as
// just a longer effective range against a single locked target.
//
// W29-30 scaffold scope:
//   - Subclass differentiates AbilityTag + exposes `EffectiveRangeCM`
//     so the Pal's combat component can match `MaxEngageRange` against
//     the ability's preferred reach.
//   - ActivateAbility inherited as-is — Vinewraith's combat component
//     already filters targets within MaxEngageRange (set to 900 in the
//     subclass ctor); the ability just applies the damage GE.
//   - Designer authors `BP_GA_Vinewraith_VineWhip` + per-species montage.
//
// What this is NOT:
//   - No projectile spawn. The vine is a visual conceit — damage applies
//     immediately on activation (same as Direhound bite from W20-21).
//   - No DoT / bleed GE. Designer can author a separate GE_Bleed and
//     ApplyGameplayEffectToTarget in a BP_GA override if needed.

#pragma once

#include "CoreMinimal.h"
#include "Gas/Abilities/PaldarkGameplayAbility_PalAttack.h"
#include "PaldarkGameplayAbility_PalAttack_VineWhip.generated.h"

UCLASS()
class PALDARKLAB_API UPaldarkGameplayAbility_PalAttack_VineWhip : public UPaldarkGameplayAbility_PalAttack
{
	GENERATED_BODY()

public:
	UPaldarkGameplayAbility_PalAttack_VineWhip();

	// Designer-facing reach hint. The combat component's `MaxEngageRange`
	// still wins at runtime (perception decides whether the ability even
	// activates), but Vinewraith-derived BPs should set MaxEngageRange
	// to at least this value so the ranged feel actually plays out.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Ability|PalAttack|VineWhip", meta = (ClampMin = "100.0"))
	float EffectiveRangeCM = 900.f;
};
