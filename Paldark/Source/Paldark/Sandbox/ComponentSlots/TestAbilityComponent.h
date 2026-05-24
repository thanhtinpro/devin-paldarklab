// Sandbox Lesson 04 — Ability slot.
//
// Tracks which ability tags are currently active. Cancels them when the
// shell tells it to (e.g. on death). Reuses native tags from Lesson 01.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "TestAbilityComponent.generated.h"

UCLASS(ClassGroup=(Sandbox), meta=(BlueprintSpawnableComponent))
class PALDARK_API UTestAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTestAbilityComponent();

	// SANDBOX: thực tế là UAbilitySystemComponent với GameplayAbility instances,
	// cost/cooldown effects, activation policies — Lesson 06+ sẽ build pipeline thật.
	UPROPERTY()
	FGameplayTagContainer ActiveAbilities;

	void ActivateAbility(const FGameplayTag& AbilityTag);
	void CancelAll();
};
