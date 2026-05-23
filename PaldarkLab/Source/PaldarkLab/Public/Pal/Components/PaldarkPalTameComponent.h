// PALDARK W35-36 — Tame component (server-only).
//
// 11th default subobject on `APaldarkPalCharacter`. Lives on every Pal but
// only "wakes up" on hostile-Pal subclasses (W20-21, W29-30) where the ctor
// flips `bIsTameable` to true. Player-side companions leave the flag false
// and the component's `BeginTameAttempt` API early-returns.
//
// Invocation paths (server-only):
//   1. `APaldarkPalSphere` projectile hit → routes to `BeginTameAttempt`
//      with the throwing player + the Pal Sphere tier tag.
//   2. `Paldark.Tame.Force` console command → calls `ForceTame` to bypass
//      the capture-probability formula.
//
// Capture probability formula (see `BeginTameAttempt`):
//
//     P_raw = BaseCaptureProbability
//             * (1 - HpPct)
//             * (1 + StunPct)
//             * SphereTierMultiplier
//             / TameDifficulty
//
//     P = clamp(P_raw, MinCaptureProbability, MaxCaptureProbability)
//
//   HpPct           = current Health / MaxHealth  (low HP ⇒ higher P).
//   StunPct         = current Stun   / MaxStun    (high stun ⇒ higher P).
//   SphereTier      = 1.0 (T1) / 1.5 (T2) / 2.0 (T3).
//   TameDifficulty  = species multiplier from `UPaldarkPalDefinition`
//                     (Boltmane 2.5, Direhound 1.0, Foxparks 0.6).
//
// On success the component spawns a tag-only "tame success" event onto the
// instigator's ASC (so HUD widgets / abilities can react), calls
// `UPaldarkPlayerPalRosterComponent::TamePal` on the instigator, then
// destroys the owning Pal. On fail it drains 30% of MaxStun (so back-to-back
// throws don't compound stun build-up) and broadcasts the fail tag.
//
// What this is NOT (deferred):
//   - No client prediction. Server is authoritative; the client just sees
//     the Pal disappear or stay.
//   - No multi-player tame contention. First throw to hit wins via the
//     `bTameAttemptInProgress` lock. Polish in W37-38 if needed.
//   - No tame-specific GA cooldown — the Pal Sphere ability's standard
//     GAS cooldown handles repeated throws.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "PaldarkPalTameComponent.generated.h"

class APaldarkCharacter;
class APaldarkPalCharacter;
class UPaldarkPalDefinition;

// Tame result enum mirrors the 3 result tags so C++ callers (validator +
// console cmds) can branch without re-parsing the tag string.
UENUM(BlueprintType)
enum class EPaldarkTameResult : uint8
{
	NotAttempted    UMETA(DisplayName = "Not Attempted"),
	Success         UMETA(DisplayName = "Success"),
	FailHpTooHigh   UMETA(DisplayName = "Fail — HP Too High"),
	FailRngMiss     UMETA(DisplayName = "Fail — RNG Miss"),
};

// Multicast delegate fired after every tame attempt resolves. Subscribers
// see (Instigator, Result, RolledProbability) so HUD / debug overlays can
// render a "32% — Failed" banner.
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnPaldarkTameAttemptResolved,
	APaldarkCharacter* /*Instigator*/,
	EPaldarkTameResult /*Result*/,
	float /*RolledProbability*/);

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent))
class PALDARKLAB_API UPaldarkPalTameComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkPalTameComponent();

	virtual void BeginPlay() override;

	// Designer-facing. When false, the component is a no-op even if a Pal
	// Sphere hits the owner. Default false on the base APaldarkPalCharacter;
	// hostile-Pal subclasses (Direhound / Razorbird / Stoneclad / Vinewraith
	// / Boltmane) flip to true in their ctor.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Pal|Tame")
	bool bIsTameable = false;

	// Designer baseline. Capture probability before HP / Stun / tier / species
	// multipliers. Read by `BeginTameAttempt`. Hostile-Pal subclasses can
	// override per-species in BP defaults; the W27-28 `UPaldarkPalDefinition`
	// also exposes a `BaseCaptureProbability` field that wins on BeginPlay if
	// the component can resolve the definition (otherwise this UPROPERTY is
	// the fallback).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Pal|Tame", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float BaseCaptureProbability = 0.35f;

	// Designer baseline. Species difficulty multiplier — divisor in the
	// capture formula, so higher = harder. Default 1.0 (baseline Direhound /
	// Razorbird). Boltmane sets 2.5 (hardest), Foxparks 0.6 (easiest).
	// Overridden by `UPaldarkPalDefinition::TameDifficulty` on BeginPlay.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Pal|Tame", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float TameDifficulty = 1.0f;

	// Floor / ceiling for the rolled probability so a designer can't author
	// a Pal that is either unconditionally tame on T1 or impossible to tame
	// even at 1 HP + full stun + T3.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Pal|Tame", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float MinCaptureProbability = 0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Pal|Tame", meta = (ClampMin = "0.5", ClampMax = "1.0"))
	float MaxCaptureProbability = 0.99f;

	// Fraction of MaxStun drained on a failed tame attempt. Keeps the
	// minigame interesting — a spammed T1 thrower can't permanently freeze
	// a Pal at 100% stun by repeatedly failing.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Pal|Tame", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FailStunDrainFraction = 0.3f;

	// Server-only entry point. Computes the formula, rolls the RNG, applies
	// result (destroy owner + roster register on success; stun drain on
	// fail). Returns the result enum so callers (Pal Sphere actor, console
	// cmd) can log without subscribing to the delegate.
	EPaldarkTameResult BeginTameAttempt(APaldarkCharacter* Instigator, FGameplayTag PalSphereTierTag);

	// Server-only entry point for the `Paldark.Tame.Force` console cmd.
	// Bypasses the formula and always succeeds. Still routes through
	// `RegisterAndDestroy` so the roster + delegate broadcast still fire.
	void ForceTame(APaldarkCharacter* Instigator);

	// Reads sphere tier tag and returns the formula multiplier. Static so
	// `APaldarkPalSphere` can call it without a component instance.
	static float GetPalSphereTierMultiplier(FGameplayTag PalSphereTierTag);

	// Broadcast after every BeginTameAttempt / ForceTame resolves. Multiple
	// subscribers OK (HUD overlay + AI Director telemetry).
	FOnPaldarkTameAttemptResolved OnTameAttemptResolved;

protected:
	// Reads the owner's `UPaldarkPalDefinition` (via the Pal data component
	// from W3-4 / W27-28) and copies its TameDifficulty + BaseCaptureProbability
	// into this component's UPROPERTYs so the formula doesn't have to soft-
	// resolve the definition on every tame attempt.
	void HydrateFromPalDefinition();

	// Common tail of both BeginTameAttempt success and ForceTame. Calls
	// `UPaldarkPlayerPalRosterComponent::TamePal` on the instigator and
	// destroys the owning Pal.
	void RegisterAndDestroy(APaldarkCharacter* Instigator, float RolledProbability);

	// Re-entrancy lock — set true while a tame attempt is in flight (one
	// frame's worth of work for now, but reserved for future async UI
	// confirmation flow). Multi-player tame contention safe because writes
	// are server-authoritative.
	bool bTameAttemptInProgress = false;
};
