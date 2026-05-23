#include "Gas/Abilities/PaldarkGameplayAbility_PalAttack_Bite.h"

#include "PaldarkGameplayTags.h"

UPaldarkGameplayAbility_PalAttack_Bite::UPaldarkGameplayAbility_PalAttack_Bite()
{
	DebugName = TEXT("PalAttack.Bite");

	// Replace the base AbilityTag added in the parent ctor with the
	// per-species ability tag so designers can target this ability
	// specifically via tag-driven query (`TryActivateAbilitiesByTag`).
	AbilityTags.Reset();
	AbilityTags.AddTag(PaldarkGameplayTags::TAG_Paldark_Ability_PalAttack);
	AbilityTags.AddTag(PaldarkGameplayTags::TAG_Paldark_Ability_PalAttack_Bite);

	// Cheap fallback damage if the combat component hasn't been wired
	// (Tombat is small — half the W18-19 default).
	FallbackBaseDamage = 6.f;
}
