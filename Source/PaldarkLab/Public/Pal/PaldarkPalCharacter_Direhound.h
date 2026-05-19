// PALDARK W20-21 — Direhound hostile-Pal subclass.
//
// Pack-hunting ground predator. Reuses the entire W18-19 perception + combat
// shape; the subclass exists only to:
//   1. Flip TeamTag → Paldark.Team.Hostile (so player-side Pal perception
//      sees this Pal as a threat).
//   2. Stamp SpeciesTag → Paldark.Pal.Species.Direhound.
//   3. Flip perception's FriendlyTeamTag/HostileTeamTag so this Pal sees
//      Paldark.Team.Player actors (player + companion Pals) as threats.
//   4. Replace the activity FSM's CandidateActivities with the hostile
//      ladder: [Idle 10, Patrol 15, Stalk 25, Combat 40].
//   5. Tune combat for melee: MinEngageRange 60 cm, MaxEngageRange 250 cm,
//      AttackInterval 1.0 s, BasePalDamage 18 (bite is harder than the
//      generic 12). MaxWalkSpeed bumped to 650 (faster than player base).
//
// Designer flow:
//   - Author `BP_GA_Direhound_Bite` (subclass `UPaldarkGameplayAbility_PalAttack`).
//     Set damage GE = `GE_Damage_Standard` (W9-10), apply on TriggerAttack
//     event in the montage.
//   - Author `BP_Direhound_Default` (subclass APaldarkPalCharacter_Direhound).
//     Set CombatSlot.AttackAbilityClass = BP_GA_Direhound_Bite + mesh / anim BP.
//   - Drop an `APaldarkHostilePalSpawner` into the level, set
//     `PalClassToSpawn = BP_Direhound_Default`, `SpawnCount = 4`,
//     `PackTag = Paldark.Pal.Pack.Direhound`. Author 3-4 waypoints.
//
// W20-21 outcome ("4 Direhound patrol, attack player khi gần") falls out of
// this configuration.
//
// What this is NOT (deferred):
//   - No flock / formation. Pack only shares threat sightings. Cohesion
//     lands W30+ with squad system.
//   - No bite animation montage. Designer authors in BP. C++ provides the
//     ability + cooldown rhythm; the montage hangs off the GA's ActivateAbility.
//   - No howl / aggro vocal. Sound design lands W22+.

#pragma once

#include "CoreMinimal.h"
#include "Pal/PaldarkPalCharacter.h"
#include "PaldarkPalCharacter_Direhound.generated.h"

UCLASS(Abstract = false)
class PALDARKLAB_API APaldarkPalCharacter_Direhound : public APaldarkPalCharacter
{
	GENERATED_BODY()

public:
	APaldarkPalCharacter_Direhound();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
