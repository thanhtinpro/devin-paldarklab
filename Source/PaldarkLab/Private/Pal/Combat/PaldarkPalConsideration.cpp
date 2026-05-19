#include "Pal/Combat/PaldarkPalConsideration.h"

namespace PaldarkPalConsiderations
{
	float ComputeUtilityScore(const TArray<FPaldarkConsideration>& Considerations)
	{
		// Empty table → 0 so an activity that consults an unconfigured
		// consideration set fails CanRun gracefully (no division by zero,
		// no early-return surprise).
		if (Considerations.Num() == 0)
		{
			return 0.f;
		}

		float WeightedSum = 0.f;
		float TotalWeight = 0.f;
		for (const FPaldarkConsideration& C : Considerations)
		{
			// Skip zero/negative weights so designers can disable a row
			// without removing it (handy for A/B tuning).
			if (C.Weight <= 0.f)
			{
				continue;
			}
			WeightedSum += C.NormalizedScore * C.Weight;
			TotalWeight += C.Weight;
		}

		if (TotalWeight <= 0.f)
		{
			return 0.f;
		}

		return WeightedSum / TotalWeight;
	}
}
