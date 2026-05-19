// PALDARK W29-30 — Boltmane hostile-Pal subclass (boss).
//
// Solo boss — Boltmane never gets a pack peer (one encounter = one Pal).
// Mirrors W20-21 pattern except:
//   1. SpeciesTag → Paldark.Pal.Species.Boltmane.
//   2. Does NOT register with the hostile pack subsystem (boss is alone;
//      pack broadcast would have no peers anyway, but skipping the call
//      avoids leaving a stale entry if the pack subsystem later starts
//      cross-species broadcasting).
//   3. Activity ladder uses `UPaldarkActivity_BossPhase` (Priority 50)
//      INSTEAD of `UPaldarkActivity_Combat` (Priority 40). The boss
//      phase activity tracks Health % thresholds and broadcasts
//      `FOnBossPhaseChanged` to designers / HUD when the boss enters
//      Enraged or Telegraph phases.
//   4. Combat tuning is boss-tier: MaxHealth bumped via the AttributeSet
//      initialisation (designer authors `GE_BoltmaneInit`), MaxWalkSpeed
//      base 500, BasePalDamage 18 (with chain-lightning bonus from
//      `UPaldarkGameplayAbility_PalAttack_ChainLightning`).
//   5. MinEngageRange 300 cm — the chain lightning's primary jump is
//      ranged so the boss tries to stay at mid-distance instead of
//      hugging the player.
//
// Designer flow:
//   - Author `BP_GA_Boltmane_ChainLightning` (subclass `UPaldarkGameplayAbility_PalAttack_ChainLightning`).
//   - Author `BP_Boltmane_Default` (subclass APaldarkPalCharacter_Boltmane).
//     Set CombatSlot.AttackAbilityClass = BP_GA_Boltmane_ChainLightning +
//     boss-tier mesh + custom AnimBP with telegraph montage.
//   - Drop a single APaldarkHostilePalSpawner per arena, SpawnCount = 1,
//     PackTag = NAME_None (boss is solo).
//   - Wire `UPaldarkActivity_BossPhase::OnBossPhaseChanged` to a HUD
//     widget that flashes the phase banner.

#pragma once

#include "CoreMinimal.h"
#include "Pal/PaldarkPalCharacter.h"
#include "PaldarkPalCharacter_Boltmane.generated.h"

UCLASS(Abstract = false)
class PALDARKLAB_API APaldarkPalCharacter_Boltmane : public APaldarkPalCharacter
{
	GENERATED_BODY()

public:
	APaldarkPalCharacter_Boltmane();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
