// PALDARK W9-10 — Damage execution calculation.
//
// Implements the canonical "GE_Damage_Standard → ExecutionCalc → IncomingDamage"
// path described in `Documents/PALDARK/03-Roadmap_1_Year.md` § Tuần 9–10 and the
// `Documents/UE5_Course/08-Pillar_GAS.md` damage lesson. Ported from the Aura
// course [11] pattern (`Source/Aura/.../ExecCalc_Damage.cpp`) but stripped to
// the minimum surface that the W9-10 sandbox needs:
//
//   * Capture Target.Armor                     (defensive stat).
//   * Read SetByCaller magnitudes:
//       - Paldark.SetByCaller.BaseDamage           (the weapon's base damage).
//       - Paldark.SetByCaller.HeadshotMultiplier   (1.0 default, 2.0 if the
//         spec carries the Paldark.Hit.Headshot tag).
//   * Compute: Damage = BaseDamage × HeadshotMult × (100 / (100 + Armor)).
//   * Write the result into UPaldarkAttributeSet::IncomingDamage so
//     PostGameplayEffectExecute (server-side) consumes it onto Health and
//     broadcasts the death delegate when Health hits 0.
//
// Designers author GE_Damage_Standard as a Blueprint subclass of
// UGameplayEffect with one "Executions" entry pointing at this class. The
// hitscan ability fills in the SetByCaller magnitudes via
// `FGameplayEffectSpec::SetSetByCallerMagnitude` at the call site so designers
// can rebalance damage per-weapon without re-cooking C++.
//
// Future extensions (do NOT implement here in W9-10):
//   - Element resistances (Fire / Lightning / Arcane) — port from Aura when
//     Pal elemental types land in W21+.
//   - Critical hit chance / damage — W13-15 (Activity sandbox).
//   - Damage debuffs (DoT) — W21+ combat polish.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "PaldarkDamageExecutionCalculation.generated.h"

UCLASS()
class PALDARKLAB_API UPaldarkDamageExecutionCalculation : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UPaldarkDamageExecutionCalculation();

	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
