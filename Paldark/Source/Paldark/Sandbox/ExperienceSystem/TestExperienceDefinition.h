// Sandbox Lesson 02 — ExperienceDefinition: "rules in a data asset, not in GameMode subclass".
//
// In a real Paldark this is a UPrimaryDataAsset cooked to disk. Here we use a plain
// UObject seeded programmatically by the manager so the sandbox stays code-only.
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"
#include "TestActionSet.h"
#include "TestExperienceDefinition.generated.h"

UCLASS(BlueprintType)
class PALDARK_API UTestExperienceDefinition : public UObject
{
	GENERATED_BODY()

public:
	// Matches the URL option value: "?Experience=Raid" -> ExperienceName == "Raid"
	UPROPERTY()
	FName ExperienceName;

	// SANDBOX: thực tế là rules tổng (PvP/PvE flags, score limit, time limit, ...)
	UPROPERTY()
	int32 MaxPlayers = 0;

	// SANDBOX: trong thực tế là TSubclassOf<APlayerController>.
	// Dùng FName để chứng minh class-override mechanic mà không cần spawn
	// PC thật (sandbox không có gameplay actor).
	UPROPERTY()
	FName OverridePlayerControllerClassName;

	// SANDBOX: trong thực tế là TSubclassOf<APlayerState>.
	UPROPERTY()
	FName OverridePlayerStateClassName;

	// Tags granted regardless of which ActionSets are picked (e.g. base movement).
	UPROPERTY()
	FGameplayTagContainer IntrinsicTags;

	// Composable units. Effective tags = IntrinsicTags + ∪(ActionSet.GrantedTags).
	UPROPERTY()
	TArray<UTestActionSet*> ActionSets;

	// Helper: collapse Intrinsic + ActionSet contributions into one container.
	FGameplayTagContainer GetEffectiveGrantedTags() const
	{
		FGameplayTagContainer Out = IntrinsicTags;
		for (const UTestActionSet* Set : ActionSets)
		{
			if (Set)
			{
				Out.AppendTags(Set->GrantedTags);
			}
		}
		return Out;
	}
};
