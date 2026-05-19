// PALDARK W24-25 — Extraction beacon actor.
//
// Replicated `AActor` that designers place in the level. Players overlap the
// beacon's sphere trigger and (when conditions are met) accumulate
// extraction progress; once `ExtractionDuration` elapses continuously,
// the player's outcome flips to `Extracted` via `UPaldarkMatchSubsystem`.
//
// Server-authoritative: only the server owns the per-player progress map.
// Clients see the beacon as a static replicated actor whose visual state is
// driven by Blueprint (designer wires a particle / decal + audio cue in
// `BP_ExtractionBeacon`). The progress value is replicated onto each
// player's APaldarkPlayerState (ExtractionProgress 0..1) so the local HUD
// can render a ramp meter without a custom RPC.
//
// Default behaviour:
//   - 400 cm sphere trigger.
//   - 5.0 s extraction duration.
//   - Passive overlap accumulates (no key press required). Designer may
//     flip `bRequireExplicitConsent` to require the player press
//     `Paldark.InputTag.InteractExtraction` (E by default) while in the
//     overlap to advance the ramp.
//   - Only accepts non-terminal players (alive AND not already extracted).
//   - Match phase must be Active or Extracting; Warmup / Ended overlap is
//     ignored.
//
// What this is NOT (deferred):
//   - No "shared progress" requirement. Each player accumulates
//     independently. PvE-coop "stand on the pad together" is a designer
//     polish for W22+.
//   - No reverse-progress when leaving the trigger mid-ramp. Designer can
//     enable via `bDecayOnLeave` if the spec requires it.
//   - No visual scaling (beacon never grows / changes color). Pure C++
//     skeleton; visuals live in `BP_ExtractionBeacon`.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

#include "PaldarkExtractionBeacon.generated.h"

class APaldarkCharacter;
class APaldarkPlayerState;
class APlayerState;
class USphereComponent;
class UPaldarkMatchSubsystem;

UCLASS()
class PALDARKLAB_API APaldarkExtractionBeacon : public AActor
{
	GENERATED_BODY()

public:
	APaldarkExtractionBeacon();

	// Tick the per-player progress map on the server. Cheap — bounded by
	// the count of overlapping players (max 4 per W14-15 cap).
	virtual void Tick(float DeltaSeconds) override;

	// Server-only. Called by the player controller's
	// `Paldark.Match.Extract` input handler when a player presses the
	// `InteractExtraction` input while overlapping. If
	// `bRequireExplicitConsent` is false, this is a no-op (passive ramp).
	void RegisterConsentToExtract(APlayerState* PlayerState);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	// Root + trigger.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Beacon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> ExtractionTrigger;

	// Designer knobs.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paldark|Beacon", meta = (ClampMin = "50.0", ClampMax = "5000.0"))
	float TriggerRadius = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paldark|Beacon", meta = (ClampMin = "0.5", ClampMax = "60.0"))
	float ExtractionDuration = 5.f;

	// Require explicit `InteractExtraction` input. Default false (passive
	// ramp on overlap).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paldark|Beacon")
	bool bRequireExplicitConsent = false;

	// Reverse progress when the player leaves the trigger before
	// completing extraction. Default false (progress just freezes).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paldark|Beacon")
	bool bDecayOnLeave = false;

	// Designer-tunable progress wire throttle. The beacon updates the
	// PlayerState's ExtractionProgress at most every WireUpdateInterval
	// seconds so high-tick scenarios don't flood replication.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paldark|Beacon", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float WireUpdateInterval = 0.1f;

private:
	// Per-player accumulator. Reset to 0 on overlap end (when bDecayOnLeave
	// false, the value freezes instead of resetting — see Tick logic). Keyed
	// by raw PlayerState pointer; the per-row weak ref guards against stale
	// access.
	struct FProgressEntry
	{
		TWeakObjectPtr<APaldarkPlayerState> PlayerState;
		float ProgressSeconds = 0.f;
		float LastWireUpdateTime = 0.f;
		bool  bConsented = false;
	};

	UPROPERTY(Transient)
	TSet<TObjectPtr<APlayerState>> OverlappingPlayerStates;

	TMap<TObjectPtr<APlayerState>, FProgressEntry> ProgressByPlayer;

	UPaldarkMatchSubsystem* GetMatchSubsystem() const;

	// Decide whether the beacon currently accepts overlap. Honors match
	// phase + terminal-outcome guard.
	bool ShouldAcceptOverlap(APaldarkPlayerState* PlayerState) const;
};
