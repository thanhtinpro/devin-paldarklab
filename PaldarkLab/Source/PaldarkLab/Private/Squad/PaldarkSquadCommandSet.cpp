// PALDARK W22-23 — Squad command set implementation.

#include "Squad/PaldarkSquadCommandSet.h"

UPaldarkSquadCommandSet::UPaldarkSquadCommandSet() = default;

FPrimaryAssetId UPaldarkSquadCommandSet::GetPrimaryAssetId() const
{
	// PrimaryAssetType "PaldarkSquadCommandSet" registered in DefaultGame.ini
	// alongside the W1 day 8-10 PaldarkExperienceActionSet wiring.
	return FPrimaryAssetId(TEXT("PaldarkSquadCommandSet"), GetFName());
}

bool UPaldarkSquadCommandSet::FindRowByTag(
	const FGameplayTag& InCommandTag,
	FPaldarkSquadCommandRow& OutRow) const
{
	if (!InCommandTag.IsValid())
	{
		return false;
	}

	for (const FPaldarkSquadCommandRow& Row : Commands)
	{
		if (Row.CommandTag == InCommandTag)
		{
			OutRow = Row;
			return true;
		}
	}
	return false;
}
