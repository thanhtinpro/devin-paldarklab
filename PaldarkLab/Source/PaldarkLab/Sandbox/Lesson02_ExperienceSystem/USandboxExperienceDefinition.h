// SANDBOX: Trong thực tế = UPaldarkExperienceDefinition (UPrimaryDataAsset)
// Ở đây dùng UObject đơn giản vì sandbox không cần AssetManager/cook pipeline
//
// Trong thực tế, class này là UPrimaryDataAsset với ~15 UPROPERTY fields:
// DefaultPawnData, ActionSets, PlayerControllerClass, PlayerStateClass,
// RequiredGameModeClass, IntrinsicTags, RequiredGameFeatures, PreWarmPalDefinitions, etc.
// (xem PaldarkExperienceDefinition.h dòng 32-135)

#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "USandboxExperienceDefinition.generated.h"

// SANDBOX: Trong thực tế = UPaldarkExperienceActionSet (UPrimaryDataAsset)
// Chỉ giữ GrantedTags + DebugNote — đủ để chứng minh additive tag merging
USTRUCT(BlueprintType)
struct FSandboxActionSet
{
    GENERATED_BODY()

    // SANDBOX: Trong thực tế = UPaldarkExperienceActionSet::GrantedTags
    UPROPERTY(EditAnywhere)
    FGameplayTagContainer GrantedTags;

    // SANDBOX: Trong thực tế = UPaldarkExperienceActionSet::DebugNote
    UPROPERTY(EditAnywhere)
    FString DebugNote;
};

UCLASS(BlueprintType)
class USandboxExperienceDefinition : public UObject
{
    GENERATED_BODY()
public:
    // SANDBOX: Trong thực tế = UPaldarkExperienceDefinition::DisplayName
    FString DisplayName;

    // SANDBOX: Trong thực tế = UPaldarkExperienceDefinition::DefaultPawnData (TSoftObjectPtr<UPaldarkPawnData>)
    // Placeholder: chỉ dùng FString tên class, đủ để log "which pawn"
    FString PawnClassName;

    // SANDBOX: Trong thực tế = UPaldarkExperienceDefinition::PlayerControllerClass (TSoftClassPtr)
    FString PlayerControllerClassName;

    // SANDBOX: Trong thực tế = UPaldarkExperienceDefinition::PlayerStateClass (TSoftClassPtr)
    FString PlayerStateClassName;

    // SANDBOX: Trong thực tế = UPaldarkExperienceDefinition::IntrinsicTags
    FGameplayTagContainer IntrinsicTags;

    // SANDBOX: Trong thực tế = UPaldarkExperienceDefinition::ActionSets (TArray<TSoftObjectPtr<UPaldarkExperienceActionSet>>)
    TArray<FSandboxActionSet> ActionSets;

    // SANDBOX: Trong thực tế = UPaldarkExperienceDefinition::MaxPlayers (int32)
    int32 MaxPlayers = 4;

    // SANDBOX: Trong thực tế = UPaldarkExperienceDefinition::HelloWorldMessage
    FString HelloWorldMessage;
};
