// PALDARK W20-21 — Razorbird hostile-Pal subclass.
//
// Aerial dive-bomber. Reuses the W18-19 perception + combat shape; the
// subclass exists to:
//   1. Flip TeamTag → Paldark.Team.Hostile.
//   2. Stamp SpeciesTag → Paldark.Pal.Species.Razorbird.
//   3. Flip perception's FriendlyTeamTag/HostileTeamTag (Razorbird sees
//      Paldark.Team.Player as enemy).
//   4. Switch character movement to MOVE_Flying — gravity off, drives via
//      `AddMovementInput` in world-space (Activity_Stalk / _Patrol already
//      uses AddMovementInput so this "just works" — Z component now matters).
//   5. Replace CandidateActivities with [Idle 10, Patrol 15, Stalk 25, Combat 40].
//   6. Tune combat for ranged dive-peck: MinEngageRange 100 cm,
//      MaxEngageRange 800 cm, AttackInterval 2.0 s (slower than Direhound
//      bite), BasePalDamage 14. MaxFlySpeed bumped to 700.
//
// Why no separate AerialActivity_Orbit:
//   Stalk already approaches the target. With MOVE_Flying the same activity
//   produces a 3-D approach. Designer-side fine-tuning (hover height, orbit
//   radius) lands W30+ in the aerial polish week — Patrol waypoints can
//   already be placed above the ground for the hover-then-dive feel.
//
// Designer flow:
//   - Author `BP_GA_Razorbird_DivePeck` (subclass `UPaldarkGameplayAbility_PalAttack`)
//     with the dive animation montage + damage GE.
//   - Author `BP_Razorbird_Default` (subclass APaldarkPalCharacter_Razorbird).
//     Set CombatSlot.AttackAbilityClass = BP_GA_Razorbird_DivePeck + flying
//     mesh / anim BP.
//   - Drop an APaldarkHostilePalSpawner with `PalClassToSpawn = BP_Razorbird_Default`,
//     `SpawnCount = 1`, no pack tag (or a separate one if you want a flock).
//
// What this is NOT (deferred):
//   - No flocking / formation. W30+ aerial polish.
//   - No dive trajectory C++ helper. The Pal AddsMovementInput toward the
//     target every Stalk tick; if a designer wants a parabolic dive arc,
//     author it in the BP_GA_Razorbird_DivePeck montage motion warp.
//   - No takeoff / landing animation. Spawned flying.

#pragma once

#include "CoreMinimal.h"
#include "Pal/PaldarkPalCharacter.h"
#include "PaldarkPalCharacter_Razorbird.generated.h"

UCLASS(Abstract = false)
class PALDARKLAB_API APaldarkPalCharacter_Razorbird : public APaldarkPalCharacter
{
	GENERATED_BODY()

public:
	APaldarkPalCharacter_Razorbird();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
