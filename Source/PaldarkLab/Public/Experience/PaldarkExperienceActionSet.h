// PALDARK W1 day 8-10 — Experience action set.
//
// Vendor-neutral mirror of Lyra's `ULyraExperienceActionSet`. An "action set" is
// a bundle of additive extensions that an experience composes in. Examples:
//   - "Raid_Sandbox/CombatLoop"  → adds combat gameplay tags, spawn rules.
//   - "Raid_Sandbox/Voice"       → adds voice chat ability set, audio buses.
//   - "Shared/DebugCommands"     → adds debug console commands, F-key bindings.
//
// Treating these as separate data assets lets designers re-mix the same building
// blocks across multiple experiences without duplication.
//
// W1 day 8-10 ships the skeleton — concrete payload types (ability sets, game
// feature plugins, modular components) get wired in at their respective weeks
// (P08 GAS W7+, P17 Lyra W33+). For now an action set just carries:
//   - a list of `FGameplayTag`s the experience adds to its tag container,
//   - an optional `HelloWorldNote` (debug surface, parallels day 6-7).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PaldarkExperienceActionSet.generated.h"

UCLASS(BlueprintType, Const, meta = (DisplayName = "Paldark Experience Action Set"))
class PALDARKLAB_API UPaldarkExperienceActionSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPaldarkExperienceActionSet();

	// Gameplay tags merged into the experience's tag container when this action
	// set is applied. Used at W3+ to gate components, abilities, UI hooks.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Experience")
	FGameplayTagContainer GrantedTags;

	// Free-form note logged when the action set is applied; mirrors the
	// HelloWorldMessage pattern from W1 day 6-7 so we can verify routing.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Experience")
	FText DebugNote;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
