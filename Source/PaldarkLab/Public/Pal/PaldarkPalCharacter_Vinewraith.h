// PALDARK W29-30 — Vinewraith hostile-Pal subclass (ranged).
//
// Ranged ground caster. Reuses the W20-21 hostile pattern, but extends
// engagement range so the Pal kites the player instead of running into
// melee. Combat tuning:
//   - MinEngageRange 400 cm (Vinewraith stays at distance).
//   - MaxEngageRange 900 cm (matches the VineWhip ability's preferred
//     reach — `UPaldarkGameplayAbility_PalAttack_VineWhip::EffectiveRangeCM`).
//   - AttackInterval 1.5 s.
//   - BasePalDamage 14 (per-hit damage in line with the W18-19 ranged
//     baseline; ranged classes don't get the melee damage bump).
//   - MaxWalkSpeed 520 (fast enough to maintain spacing).
//
// Activity ladder: [Idle 10, Patrol 15, Stalk 25, Combat 40] — same as
// Stoneclad / Direhound. Stalk handles the "kite" feel: while the player
// is between MinEngageRange and MaxEngageRange the Pal alternates between
// Stalk (close gap to maintain distance) and Combat (whip from range).
//
// Designer flow:
//   - Author `BP_GA_Vinewraith_VineWhip` (subclass `UPaldarkGameplayAbility_PalAttack_VineWhip`).
//   - Author `BP_Vinewraith_Default` (subclass APaldarkPalCharacter_Vinewraith).
//     Set CombatSlot.AttackAbilityClass + mesh / anim BP.
//   - Drop spawner with PackTag = Paldark.Pal.Pack.Vinewraith.

#pragma once

#include "CoreMinimal.h"
#include "Pal/PaldarkPalCharacter.h"
#include "PaldarkPalCharacter_Vinewraith.generated.h"

UCLASS(Abstract = false)
class PALDARKLAB_API APaldarkPalCharacter_Vinewraith : public APaldarkPalCharacter
{
	GENERATED_BODY()

public:
	APaldarkPalCharacter_Vinewraith();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
