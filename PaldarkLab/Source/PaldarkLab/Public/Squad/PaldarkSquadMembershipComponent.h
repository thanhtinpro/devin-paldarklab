// PALDARK W22-23 — Squad membership component.
//
// Per-player `UActorComponent` that auto-registers its owner into the
// `UPaldarkSquadSubsystem` on `BeginPlay` (authority only) and unregisters
// on `EndPlay`. Lives on `APaldarkCharacter` as the new SquadMembershipSlot.
//
// W22-23 outcome (Documents/PALDARK/03-Roadmap_1_Year.md § Tuần 22-23):
//   "4-player team coordinate qua ping + radial command."
//
// Designer override knob:
//   - `SquadTag` defaults to `Paldark.Squad.Default` so a freshly-spawned
//     character lands in the default 4-player squad automatically. PvP
//     subclasses (W40+) set `SquadTag = Paldark.Squad.Red` / `.Blue` in
//     their Blueprint constructor before BeginPlay runs.
//
// Why not just register from APaldarkCharacter::BeginPlay directly:
//   - Composition (P03 pillar) — squad concern is its own component so the
//     base character class stays generic. Mirrors the W18-19 perception
//     pattern where threat detection lives on its own component.
//   - Future plug-ins (PvP team override, hub town join/leave) bind to
//     this component's `SetSquadTag` instead of touching the character.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PaldarkSquadMembershipComponent.generated.h"

class APaldarkCharacter;
class UPaldarkSquadSubsystem;

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent))
class PALDARKLAB_API UPaldarkSquadMembershipComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkSquadMembershipComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Runtime mutator — re-registers under a new squad tag. Authority only.
	// Used by PvP join/leave flows (W40+) and by `Paldark.Squad.SetTag` in
	// the W22-23 console command dump.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Squad")
	void SetSquadTag(const FGameplayTag& InSquadTag);

	UFUNCTION(BlueprintPure, Category = "Paldark|Squad")
	FGameplayTag GetSquadTag() const { return SquadTag; }

protected:
	// Designer override (BP-set). Default = `Paldark.Squad.Default`.
	// Empty falls back to `Paldark.Squad.Default` at register time so a
	// designer can intentionally leave the tag blank.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Squad")
	FGameplayTag SquadTag;

private:
	// Cache the resolved tag so EndPlay's unregister hits the right roster
	// even if SetSquadTag swapped it after BeginPlay.
	UPROPERTY(Transient)
	FGameplayTag RegisteredTag;

	UPaldarkSquadSubsystem* GetSquadSubsystem() const;
	APaldarkCharacter* GetOwnerCharacter() const;
};
