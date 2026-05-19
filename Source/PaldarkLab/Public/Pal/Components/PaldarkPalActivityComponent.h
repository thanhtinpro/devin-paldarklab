// PALDARK W5-6 — Pal Activity FSM driver component.
//
// Owns the active `UPaldarkBaseActivity` for the Pal and a pool of candidate
// activity instances. Every `SelectionInterval` seconds the component
// re-evaluates `CanRun()` on each candidate, picks the highest-priority
// activity, and switches if it differs from the current one. The active
// activity's `TickActivity(DeltaSeconds)` runs every component tick.
//
// W5-6 outcome (from Documents/PALDARK/03-Roadmap_1_Year.md § Tuần 5-6):
//   "Pal switch giữa Idle ↔ Follow ↔ Investigate dựa trên player distance + ping."
//
// Authoring:
//   - Configure `CandidateActivities` on the Blueprint subclass of the Pal
//     character (default in C++ ctor is Idle/Follow/Investigate).
//   - Or feed candidates from a Pal data asset in W18+ (RoN-style designer
//     authored activity sets).
//   - Force a state from the console: `Paldark.Pal.SetActivity <Idle|Follow|Investigate>`.
//   - Trigger Investigate from the console: `Paldark.Pal.Ping X Y Z`.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "PaldarkPalActivityComponent.generated.h"

class UPaldarkBaseActivity;

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent))
class PALDARKLAB_API UPaldarkPalActivityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkPalActivityComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// W5-6 — Force a specific activity by tag (debug / scripted hook). Returns
	// true if a candidate matching the tag was found and switched in.
	bool SetCurrentActivityByTag(const FGameplayTag& InTag);

	// W5-6 — Force a specific activity by class. Used by the spawn flow to
	// kick off Idle as the initial state.
	bool SetCurrentActivityByClass(TSubclassOf<UPaldarkBaseActivity> InClass);

	// Pal Ping support — feeds the Investigate activity.
	void RequestInvestigate(const FVector& InLocation);
	void ClearInvestigateRequest();
	bool HasActiveInvestigateRequest() const { return bHasInvestigateRequest; }
	FVector GetInvestigateTarget() const { return InvestigateTarget; }

	// Accessors for HUD / debug console commands.
	UPaldarkBaseActivity* GetCurrentActivity() const { return CurrentActivity; }
	FGameplayTag GetCurrentActivityTag() const;
	const TArray<TObjectPtr<UPaldarkBaseActivity>>& GetActivityInstances() const { return ActivityInstances; }

	// Designer-authored list of activity classes the FSM may select from.
	// Default is Idle / Follow / Investigate (set in the C++ constructor).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Activity")
	TArray<TSubclassOf<UPaldarkBaseActivity>> CandidateActivities;

	// Re-evaluation cadence. Lower = more responsive switching, higher = cheaper.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Activity")
	float SelectionInterval = 0.25f;

protected:
	// Internal: instantiate every CandidateActivity and InitActivity it.
	// Called on BeginPlay (authority only).
	void InstantiateActivities();

	// Internal: pick the best candidate based on CanRun + Priority, then
	// either keep CurrentActivity (if ShouldContinue) or switch in the new one.
	void EvaluateAndSwitch();

	// Internal: swap Enter/Exit hooks atomically.
	void SwitchToActivity(UPaldarkBaseActivity* InActivity);

private:
	// All instantiated activities, kept alive by UPROPERTY so GC doesn't collect.
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPaldarkBaseActivity>> ActivityInstances;

	UPROPERTY(Transient)
	TObjectPtr<UPaldarkBaseActivity> CurrentActivity;

	UPROPERTY(Transient)
	float TimeSinceLastSelection = 0.0f;

	// Investigate request state. The Investigate activity reads these via
	// HasActiveInvestigateRequest() / GetInvestigateTarget() and clears them
	// when done.
	UPROPERTY(Transient)
	bool bHasInvestigateRequest = false;

	UPROPERTY(Transient)
	FVector InvestigateTarget = FVector::ZeroVector;
};
