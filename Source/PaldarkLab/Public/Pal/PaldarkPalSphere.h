// PALDARK W35-36 — Pal Sphere projectile (replicated AActor).
//
// Spawned by `UPaldarkGameplayAbility_UsePalSphere` on the server when the
// player consumes one Pal Sphere from inventory. Carries:
//   - `OwnerPlayer` (replicated) — the throwing player; passed to the
//     `UPaldarkPalTameComponent::BeginTameAttempt` as `Instigator`.
//   - `PalSphereTierTag` (replicated) — tier identity (T1/T2/T3) used by the
//     tame component to look up the formula multiplier.
//
// Lifecycle:
//   - Spawned with a `UProjectileMovementComponent` configured for a short
//     gravity-affected arc (1500 cm/s initial, gravity scale 1, no homing).
//   - Auto-destroys 10 s after spawn (server-side `SetLifeSpan(10)`) so a
//     thrown sphere that lands outside any Pal collision never accumulates.
//   - On collision overlap with an `APaldarkPalCharacter`:
//       * Server only — calls `TameComponent->BeginTameAttempt(OwnerPlayer, PalSphereTierTag)`.
//       * Destroys self regardless of result (a sphere is one-shot).
//
// Replication:
//   - Replicates so simulated proxies see the trail VFX (designer authors
//     a particle system attached to the static mesh later).
//   - bReplicateMovement = true so clients see the arc; ProjectileMovement
//     component does the actual interpolation client-side.
//
// What this is NOT (deferred):
//   - No client-prediction throw — server is authoritative.
//   - No mesh / VFX / sound — designer authors `BP_PalSphere_T<N>` later.
//   - No bounce / ricochet — first overlap consumes the sphere.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "PaldarkPalSphere.generated.h"

class APaldarkCharacter;
class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class PALDARKLAB_API APaldarkPalSphere : public AActor
{
	GENERATED_BODY()

public:
	APaldarkPalSphere();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Initialise the sphere on the authority before NotifyActorBeginPlay.
	// Called by `UPaldarkGameplayAbility_UsePalSphere::ActivateAbility` after
	// `SpawnActorDeferred` and before `FinishSpawning`. The tier tag drives
	// the capture-formula multiplier on overlap; `InOwnerPlayer` becomes the
	// `Instigator` passed to `BeginTameAttempt`.
	void InitForThrow(APaldarkCharacter* InOwnerPlayer, FGameplayTag InPalSphereTierTag);

	UFUNCTION(BlueprintPure, Category = "Paldark|Pal|Sphere")
	APaldarkCharacter* GetOwnerPlayer() const { return OwnerPlayer; }

	UFUNCTION(BlueprintPure, Category = "Paldark|Pal|Sphere")
	FGameplayTag GetPalSphereTierTag() const { return PalSphereTierTag; }

protected:
	virtual void BeginPlay() override;

	// Sphere collision — root component. Set to BlockAllDynamic on the
	// designer-side mesh later; default profile is OverlapAllDynamic so a
	// scaffold sphere still routes the overlap event without colliding.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Pal|Sphere", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> CollisionComponent;

	// Visual stub. Designer swaps `SM_PalSphere_T<N>` per tier in BP defaults
	// later; default is null (invisible projectile, but the collision still
	// works for overlap).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Pal|Sphere", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// Projectile motion. Tuned in the ctor for a thrown-ball arc.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Pal|Sphere", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	// Replicated so simulated proxies see who threw the sphere (for VFX
	// trail attribution + designer-side per-team coloring).
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Paldark|Pal|Sphere")
	TObjectPtr<APaldarkCharacter> OwnerPlayer;

	// Replicated so simulated proxies pick the right mesh / trail per tier.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Paldark|Pal|Sphere")
	FGameplayTag PalSphereTierTag;

	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	// Re-entrancy guard. Once an overlap has resolved (success or fail) we
	// destroy self; a chained Pal overlap on the same tick shouldn't
	// re-fire the tame component.
	bool bHasResolved = false;
};
