// PALDARK W5-6 — Pal Activity FSM base class.
//
// Port of the spirit (not the full surface area) of RoN's UBaseActivity. Each
// Pal Activity is a `UObject` that an `UPaldarkPalActivityComponent` owns and
// ticks. The component evaluates `CanRun()` on every candidate each frame
// (rate-limited via `SelectionInterval` on the component) and picks the
// highest-priority activity. On switch, the previous activity's `ExitActivity`
// runs, then the new activity's `EnterActivity`. The active activity's
// `TickActivity(float)` runs every component tick.
//
// W5-6 deliberately leaves out the full RoN feature set:
//   - no internal sub-state machine (UActivityFiniteStateMachine in RoN);
//     each activity is its own atomic state — sub-states land in W18+ when
//     combat activities show up.
//   - no path follow / nav query helpers; the locomotion component handles
//     low-level movement input.
//   - no replication; activities live server-side, Pal pawn movement
//     replicates via ACharacter defaults.
//
// Authoring flow: Blueprint subclass this, point `ActivityTag` at one of
// `Paldark.Pal.Activity.*`, override `CanRun_Implementation` /
// `EnterActivity_Implementation` / `TickActivity_Implementation` /
// `ExitActivity_Implementation` (BlueprintNativeEvent) if you need designer
// hooks, or override the C++ virtuals directly.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "PaldarkBaseActivity.generated.h"

class APaldarkPalCharacter;
class APawn;
class UPaldarkPalActivityComponent;
class UPaldarkPalLocomotionComponent;

/**
 * Base class for every Pal Activity (Idle / Follow / Investigate / Combat / ...).
 * Subclasses live under `Pal/Activities/` and are instantiated server-side by
 * `UPaldarkPalActivityComponent::BeginPlay`.
 */
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, Transient)
class PALDARKLAB_API UPaldarkBaseActivity : public UObject
{
	GENERATED_BODY()

public:
	UPaldarkBaseActivity();

	// Cache owning component + character. Called by the component immediately
	// after `NewObject<>` during BeginPlay. Returns true if the activity can
	// be hosted on this component (currently always true; subclasses may
	// override to reject the bind).
	virtual bool InitActivity(UPaldarkPalActivityComponent* InComponent);

	// Returns true if this activity wants to run right now. Called every
	// selection tick on every candidate. Higher-`Priority` activities win
	// ties. Subclasses MUST override.
	UFUNCTION(BlueprintNativeEvent, Category = "Paldark|Pal|Activity")
	bool CanRun() const;
	virtual bool CanRun_Implementation() const { return false; }

	// W5-6 — Optional hysteresis predicate. Default returns `CanRun()`; an
	// activity that wants to "stick" once entered (e.g. Investigate should
	// keep running until it finishes its move even if the player gets close)
	// can override.
	UFUNCTION(BlueprintNativeEvent, Category = "Paldark|Pal|Activity")
	bool ShouldContinue() const;
	virtual bool ShouldContinue_Implementation() const { return CanRun(); }

	// Lifecycle hooks. Default implementations log to LogPaldarkPal so
	// designers can see the transition in PIE.
	UFUNCTION(BlueprintNativeEvent, Category = "Paldark|Pal|Activity")
	void EnterActivity();
	virtual void EnterActivity_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category = "Paldark|Pal|Activity")
	void TickActivity(float DeltaSeconds);
	virtual void TickActivity_Implementation(float DeltaSeconds) {}

	UFUNCTION(BlueprintNativeEvent, Category = "Paldark|Pal|Activity")
	void ExitActivity();
	virtual void ExitActivity_Implementation();

	// Identity. The component matches candidates by tag for the
	// `Paldark.Pal.SetActivity` debug command. Default is empty (invalid);
	// concrete subclasses set this in their constructor.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Activity")
	FGameplayTag ActivityTag;

	// Selection priority; higher wins. Designers may bump per-Pal in BP
	// subclasses (e.g. a guard Pal might have higher Follow priority so it
	// stays close instead of investigating).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Activity")
	int32 Priority = 0;

	// Accessors. Cached in `InitActivity` so subclasses can read them in
	// CanRun without re-walking the component graph every frame.
	UFUNCTION(BlueprintPure, Category = "Paldark|Pal|Activity")
	UPaldarkPalActivityComponent* GetActivityComponent() const { return OwningComponent.Get(); }

	UFUNCTION(BlueprintPure, Category = "Paldark|Pal|Activity")
	APaldarkPalCharacter* GetPalOwner() const { return PalOwner.Get(); }

	// Convenience helpers used by every default activity. Return nullptr /
	// max-float when the Pal is not properly initialised yet.
	APawn* GetFollowedPawn() const;
	UPaldarkPalLocomotionComponent* GetLocomotionSlot() const;
	float GetPlanarDistanceToFollowedPawn() const;

	virtual UWorld* GetWorld() const override final;

protected:
	UPROPERTY(Transient)
	TWeakObjectPtr<UPaldarkPalActivityComponent> OwningComponent;

	UPROPERTY(Transient)
	TWeakObjectPtr<APaldarkPalCharacter> PalOwner;
};
