// PALDARK W29-30 — Tombat (companion) bite ability.
//
// The fastest single-target attack in the W29-30 roster. Tombat is a
// small companion Pal that exists primarily to chip the player's
// current target — low per-hit damage, very short cooldown.
//
// W29-30 scaffold scope:
//   - Subclass exists only to differentiate AbilityTag + designer-facing
//     defaults from the W18-19 base. ActivateAbility is inherited as-is
//     (single-target, server-only).
//   - Designer authors `BP_GA_Tombat_Bite` (subclass of THIS class) +
//     `GE_Damage_Tombat_Bite` (low damage GE), wires both via
//     `UPaldarkPalDefinition::GrantedAbilities` (W27-28 data-driven flow)
//     or via a Blueprint Pal subclass's CombatSlot.AttackAbilityClass.
//
// What this is NOT:
//   - No multi-bite combo. The "fast attack" feel comes from the lower
//     CooldownIntervalOverride defaults, not from C++ orchestration.
//   - No animation montage trigger. Designer authors per-species montage
//     in `BP_GA_Tombat_Bite::ActivateAbility` override.

#pragma once

#include "CoreMinimal.h"
#include "Gas/Abilities/PaldarkGameplayAbility_PalAttack.h"
#include "PaldarkGameplayAbility_PalAttack_Bite.generated.h"

UCLASS()
class PALDARKLAB_API UPaldarkGameplayAbility_PalAttack_Bite : public UPaldarkGameplayAbility_PalAttack
{
	GENERATED_BODY()

public:
	UPaldarkGameplayAbility_PalAttack_Bite();

	// Designer-facing override for the companion's preferred attack
	// interval. Tombat is the only species in W29-30 that wants a much
	// faster rhythm than the W18-19 default (1.0 s on Direhound). The
	// combat component's `AttackInterval` still wins at runtime — this
	// is the value designers should set there for Tombat-derived BPs.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Ability|PalAttack|Bite", meta = (ClampMin = "0.1"))
	float RecommendedAttackInterval = 0.4f;
};
