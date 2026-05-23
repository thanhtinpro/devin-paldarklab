// PALDARK W29-30 — Stoneclad hostile-Pal subclass (tank).
//
// Heavy ground melee. Mirrors the W20-21 Direhound pattern but tuned for
// "slow tank" instead of "fast bite":
//   1. Flips TeamTag → Paldark.Team.Hostile.
//   2. Stamps SpeciesTag → Paldark.Pal.Species.Stoneclad.
//   3. Flips perception's FriendlyTeamTag/HostileTeamTag so Stoneclad
//      sees Paldark.Team.Player actors as threats.
//   4. Tunes combat for slow melee: MinEngageRange 80 cm, MaxEngageRange
//      280 cm, AttackInterval 2.0 s (heavy windup), BasePalDamage 28
//      (higher than Direhound's 18 to compensate for the slower cadence).
//      MaxWalkSpeed dropped to 380 (slower than the player base to read
//      as a tank).
//   5. Replaces CandidateActivities with the standard hostile ladder
//      [Idle 10, Patrol 15, Stalk 25, Combat 40] — Stoneclad does NOT
//      get the W29-30 BossPhase activity (that one is Boltmane-only).
//   6. BeginPlay/EndPlay register/unregister with the hostile pack
//      subsystem under PackTag = Paldark.Pal.Pack.Stoneclad.
//
// Designer flow:
//   - Author `BP_GA_Stoneclad_Charge` (subclass `UPaldarkGameplayAbility_PalAttack_Charge`).
//   - Author `BP_Stoneclad_Default` (subclass APaldarkPalCharacter_Stoneclad).
//     Set CombatSlot.AttackAbilityClass = BP_GA_Stoneclad_Charge + mesh / anim BP.
//   - Drop an `APaldarkHostilePalSpawner` with PalClassToSpawn = BP_Stoneclad_Default
//     and PackTag = Paldark.Pal.Pack.Stoneclad.

#pragma once

#include "CoreMinimal.h"
#include "Pal/PaldarkPalCharacter.h"
#include "PaldarkPalCharacter_Stoneclad.generated.h"

UCLASS(Abstract = false)
class PALDARKLAB_API APaldarkPalCharacter_Stoneclad : public APaldarkPalCharacter
{
	GENERATED_BODY()

public:
	APaldarkPalCharacter_Stoneclad();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
