// PALDARK W35-36 — UsePalSphere ability (server-only).
//
// Consumes one Pal Sphere item from the activating player's inventory and
// spawns a replicated `APaldarkPalSphere` projectile aimed forward from the
// player camera. Authority-only activation — no client prediction in W35-36
// (defer to W38 polish) so the inventory consume is unambiguous.
//
// Activation flow (server):
//   1. Resolve `APaldarkCharacter` avatar + `UPaldarkPlayerInventoryComponent`.
//   2. Pick the sphere tier tag: the GA's `PalSphereTierTag` UPROPERTY
//      defaults to T1; designer authors per-tier GA subclasses (or just
//      override the tag on the BP CDO) for T2/T3 quickslot bindings.
//   3. Call `RemoveItemByTag(SphereTierTag, 1)` on inventory; if 0 removed,
//      abort the ability (player tried to throw with no sphere in stock).
//   4. SpawnActorDeferred `APaldarkPalSphere` at the camera-forward muzzle
//      transform (player location + 60 cm forward + 50 cm up), call
//      `InitForThrow(Avatar, SphereTierTag)`, then FinishSpawning.
//   5. EndAbility(bWasCancelled=false).
//
// Cooldown / cost GE are deferred to designer (BP defaults). The activation
// itself is single-shot — no continued tick.

#pragma once

#include "CoreMinimal.h"
#include "Gas/PaldarkGameplayAbility.h"
#include "GameplayTagContainer.h"
#include "PaldarkGameplayAbility_UsePalSphere.generated.h"

class APaldarkPalSphere;

UCLASS()
class PALDARKLAB_API UPaldarkGameplayAbility_UsePalSphere : public UPaldarkGameplayAbility
{
	GENERATED_BODY()

public:
	UPaldarkGameplayAbility_UsePalSphere();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// Sphere tier consumed from inventory and stamped onto the spawned
	// projectile. Designer overrides per-quickslot — e.g. quickslot 1 ⇒ T1,
	// quickslot 2 ⇒ T2, quickslot 3 ⇒ T3. Default T1.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Ability|PalSphere",
		meta = (Categories = "Paldark.Item.PalSphere"))
	FGameplayTag PalSphereTierTag;

	// Projectile actor class to spawn. Defaults to `APaldarkPalSphere`;
	// designer can swap to a custom-mesh BP subclass for VFX without
	// touching C++.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Ability|PalSphere")
	TSubclassOf<APaldarkPalSphere> PalSphereClass;

	// Muzzle offset from the player root in world space. Default puts the
	// sphere 60 cm in front of the player at chest height (50 cm up) so it
	// clears the player capsule on spawn.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Ability|PalSphere")
	FVector MuzzleOffset = FVector(60.f, 0.f, 50.f);
};
