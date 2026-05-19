// PALDARK W7-8 — Thin AbilitySystemComponent subclass.
//
// Subclassing `UAbilitySystemComponent` is the standard GAS practice (Aura,
// Lyra, Crunch, RoN all do it) so we have a project-specific extension point
// for cross-cutting hooks (input bind, gameplay event broadcasts, ability
// activation logging) without touching every ability. W7-8 ships the minimal
// shape — just one `OnAbilityInputAction` helper used by the character to
// translate Enhanced Input → ability activation by tag.
//
// Roadmap reference: Documents/PALDARK/03-Roadmap_1_Year.md § Tuần 7–8.

#pragma once

#include "AbilitySystemComponent.h"
#include "CoreMinimal.h"
#include "PaldarkAbilitySystemComponent.generated.h"

UCLASS()
class PALDARKLAB_API UPaldarkAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UPaldarkAbilitySystemComponent();

	// W7-8 — Activate every ability whose AbilityTags contain `InActivationTag`.
	// Returns the number of successful activations. Authority + autonomous
	// clients should both be allowed to call this so input prediction works.
	int32 TryActivateAbilityByActivationTag(const FGameplayTag& InActivationTag);

	// W7-8 — Symmetric cancel for "release on key up" abilities like Sprint.
	// Sends `InActivationTag` as the cancel filter so multi-instance abilities
	// don't accidentally cancel each other.
	void CancelAbilityByActivationTag(const FGameplayTag& InActivationTag);
};
