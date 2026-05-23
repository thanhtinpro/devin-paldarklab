// PALDARK W47 — Player save game container implementation.

#include "Save/PaldarkPlayerSaveGame.h"

UPaldarkPlayerSaveGame::UPaldarkPlayerSaveGame()
{
	SchemaVersion = kCurrentSchema;
	SaveTime = FDateTime::UtcNow();
	PlayerName = TEXT("");
}

FString UPaldarkPlayerSaveGame::DescribeForLog() const
{
	return FString::Printf(
		TEXT("schema=%d saved=%s player=%s active_roster=%d deposit=%d inventory=%d tutorials=%d unlocks=%d last_map=%s raids=%d"),
		SchemaVersion,
		*SaveTime.ToIso8601(),
		*PlayerName,
		ActiveRoster.Num(),
		DepositRoster.Num(),
		Inventory.Num(),
		Progress.CompletedTutorials.Num(),
		Progress.UnlockedExperiences.Num(),
		*Progress.LastHubMapName.ToString(),
		Progress.TotalRaidsCompleted);
}
