#include "Match/PaldarkMatchTypes.h"

#include "PaldarkGameplayTags.h"

FGameplayTag PaldarkOutcomeToTag(EPaldarkPlayerOutcome Outcome)
{
	using namespace PaldarkGameplayTags;
	switch (Outcome)
	{
	case EPaldarkPlayerOutcome::Extracted:    return TAG_Paldark_Match_Outcome_Extracted;
	case EPaldarkPlayerOutcome::KIA:          return TAG_Paldark_Match_Outcome_KIA;
	case EPaldarkPlayerOutcome::Disconnected: return TAG_Paldark_Match_Outcome_Disconnected;
	case EPaldarkPlayerOutcome::Alive:        // fallthrough
	default:                                  return FGameplayTag();
	}
}

FGameplayTag PaldarkPhaseToTag(EPaldarkMatchPhase Phase)
{
	using namespace PaldarkGameplayTags;
	switch (Phase)
	{
	case EPaldarkMatchPhase::Warmup:     return TAG_Paldark_Match_Phase_Warmup;
	case EPaldarkMatchPhase::Active:     return TAG_Paldark_Match_Phase_Active;
	case EPaldarkMatchPhase::Extracting: return TAG_Paldark_Match_Phase_Extracting;
	case EPaldarkMatchPhase::Ended:      return TAG_Paldark_Match_Phase_Ended;
	default:                             return FGameplayTag();
	}
}
