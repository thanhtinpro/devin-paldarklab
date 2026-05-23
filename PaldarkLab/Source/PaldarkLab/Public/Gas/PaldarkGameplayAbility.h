// PALDARK W7-8 — Gameplay ability base class.
//
// Thin UGameplayAbility subclass that adds the project-wide conventions every
// Paldark ability follows: an activation tag for tag-keyed input lookup, a
// short debug name, and a designer-friendly category. Concrete abilities
// (GA_Sprint in W7-8; GA_Shoot in W9-10; GA_Bond in W35-36) inherit from this.
//
// Roadmap reference: Documents/PALDARK/03-Roadmap_1_Year.md § Tuần 7–8.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PaldarkGameplayAbility.generated.h"

UCLASS(Abstract, BlueprintType, Blueprintable)
class PALDARKLAB_API UPaldarkGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UPaldarkGameplayAbility();

	// Tag used by Enhanced Input → ability lookup. Authored in the C++
	// constructor of each concrete ability; designers should not need to
	// override this.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Ability")
	FGameplayTag ActivationInputTag;

	// Short human-readable name surfaced by the debug HUD / `Paldark.Gas.DumpAttributes`
	// console command. Optional — falls back to GetName() when empty.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Ability")
	FString DebugName;
};
