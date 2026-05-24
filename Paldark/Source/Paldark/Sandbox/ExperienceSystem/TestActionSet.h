// Sandbox Lesson 02 — ActionSet is the composition unit.
//
// WHY a separate ActionSet object instead of stuffing all tags into ExperienceDefinition?
//   Reuse: "Voice" action set can be mixed into Hub, Lobby, Tutorial without copying tags.
//   Modularity: designers compose experiences from a small library of action sets.
//   Data-locality: one cooked asset per concern; experience definition = small manifest.
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"
#include "TestActionSet.generated.h"

UCLASS(BlueprintType)
class PALDARK_API UTestActionSet : public UObject
{
	GENERATED_BODY()

public:
	// SANDBOX: trong thực tế đây là FPrimaryAssetId / asset name path
	UPROPERTY()
	FName ActionSetName;

	// SANDBOX: trong thực tế còn có: AbilitySets, EffectsToGrant, AttributeSets,
	// InputConfigs, MeshOverrides, ... — ở đây chỉ giữ Tags để chứng minh
	// flow merge: ExperienceDefinition.Intrinsic + Σ(ActionSet.Granted).
	UPROPERTY()
	FGameplayTagContainer GrantedTags;
};
