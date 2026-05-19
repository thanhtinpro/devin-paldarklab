// SANDBOX: Trong thực tế = UPaldarkPawnData (UPrimaryDataAsset)
// Tối giản: chỉ giữ PawnClassName, InputConfig, MappingContexts
// Bỏ: GrantedAbilities, StartupEffects, DefaultAnimInstanceClass, HUDClass, PalCompanions
// (chúng sẽ xuất hiện ở Lesson 04+ khi cần)

#pragma once
#include "CoreMinimal.h"
#include "USandboxPawnData.generated.h"

class USandboxInputConfig;

// SANDBOX: Trong thực tế = FPaldarkMappingContextAndPriority
// Thay UInputMappingContext* bằng FString vì sandbox không cần IMC asset thật
USTRUCT(BlueprintType)
struct FSandboxMappingContext
{
    GENERATED_BODY()

    // SANDBOX: Trong thực tế = TSoftObjectPtr<UInputMappingContext> InputMapping
    FString MappingContextName;

    // SANDBOX: Trong thực tế = int32 Priority (0=base, 1=gameplay, 2=vehicle, 3=UI)
    int32 Priority = 0;
};

UCLASS(BlueprintType)
class USandboxPawnData : public UObject
{
    GENERATED_BODY()
public:
    // SANDBOX: Trong thực tế = TSoftClassPtr<APawn> PawnClass
    FString PawnClassName;

    // SANDBOX: Trong thực tế = TArray<FPaldarkMappingContextAndPriority> DefaultMappingContexts
    TArray<FSandboxMappingContext> MappingContexts;

    // SANDBOX: Trong thực tế = TSoftObjectPtr<UPaldarkInputConfig> InputConfig
    USandboxInputConfig* InputConfig = nullptr;

    // SANDBOX: Placeholder cho GrantedAbilities (sẽ chi tiết ở Lesson GAS)
    TArray<FString> GrantedAbilityNames;
};
