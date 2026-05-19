#include "Gas/Abilities/PaldarkGameplayAbility_PalAttack_VineWhip.h"

#include "PaldarkGameplayTags.h"

UPaldarkGameplayAbility_PalAttack_VineWhip::UPaldarkGameplayAbility_PalAttack_VineWhip()
{
	DebugName = TEXT("PalAttack.VineWhip");

	AbilityTags.Reset();
	AbilityTags.AddTag(PaldarkGameplayTags::TAG_Paldark_Ability_PalAttack);
	AbilityTags.AddTag(PaldarkGameplayTags::TAG_Paldark_Ability_PalAttack_VineWhip);

	// Ranged hostile — moderate per-hit damage.
	FallbackBaseDamage = 14.f;
}
