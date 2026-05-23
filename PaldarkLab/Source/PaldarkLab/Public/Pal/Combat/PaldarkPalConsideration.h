// PALDARK W18-19 — Pal combat consideration (Utility-AI-lite).
//
// One row = one normalized input to a Utility AI score. The W18-19 Pal
// Combat activity composes three:
//   - "ThreatDistance" — 1 - clamp(D / MaxEngageRange, 0, 1).
//   - "PalHealth"      — CurrentHealth / MaxHealth.
//   - "AttackReady"    — 1 if combat component cooldown elapsed else 0.
//
// Each consideration ships with a designer-tunable Weight. The activity asks
// `ComputeUtilityScore` for the weighted average and runs when the result
// exceeds `MinUtilityScore` (default 0.45).
//
// Vendor-neutral mirror of the Utility AI pattern from `12.Udemy-ue4-pro-cpp`
// (course P09 lesson 4) — kept intentionally simple so future weeks can swap
// in a Mass / curve-driven implementation without touching the activity.
//
// Roadmap reference: Documents/PALDARK/03-Roadmap_1_Year.md § Tuần 18–19.
//
// Future homes (do NOT implement here):
//   - W20-21 — hostile-Pal pack AI (Direhound / Razorbird) extends the
//     consideration set with "Pack cohesion" / "Aerial advantage".
//   - W48-49 — Mass + curve table replacement (`UCurveFloat` per
//     consideration so designers don't recompile to retune the curve).

#pragma once

#include "CoreMinimal.h"
#include "UObject/NameTypes.h"
#include "PaldarkPalConsideration.generated.h"

/**
 * One normalized consideration in a Pal utility score.
 *
 * NormalizedScore must be in [0, 1]; the helper `ComputeUtilityScore` does
 * not re-clamp so callers are responsible for normalisation (kept that way
 * so future curve-table lookups can drive the score directly).
 *
 * Weight is a non-negative float; a zero weight effectively removes the
 * consideration from the average without disturbing the rest of the table.
 */
USTRUCT(BlueprintType)
struct PALDARKLAB_API FPaldarkConsideration
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paldark|Pal|Consideration")
	FName Name = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paldark|Pal|Consideration", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float NormalizedScore = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paldark|Pal|Consideration", meta = (ClampMin = "0.0"))
	float Weight = 1.f;
};

namespace PaldarkPalConsiderations
{
	/**
	 * Returns the weighted-average utility score of a consideration array.
	 *
	 * Score = Σ(NormalizedScore * Weight) / Σ(Weight).
	 *
	 * Returns 0 when the table is empty or every weight is zero. Does NOT
	 * clamp the result — callers can compare against any threshold they want.
	 */
	PALDARKLAB_API float ComputeUtilityScore(const TArray<FPaldarkConsideration>& Considerations);
}
