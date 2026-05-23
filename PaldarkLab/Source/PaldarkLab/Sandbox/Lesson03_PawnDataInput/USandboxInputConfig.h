// SANDBOX: Trong thực tế = UPaldarkInputConfig (UPrimaryDataAsset)
// Ở đây dùng UObject đơn giản vì sandbox không cần AssetManager
//
// Trong thực tế, class này chứa 2 TArray<FPaldarkInputAction>:
//   - NativeInputActions (Move/Look/Jump) — character xử lý trực tiếp
//   - AbilityInputActions (Sprint/Fire) — forward vào GAS
// (xem PaldarkInputConfig.h dòng 40-65)

#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "USandboxInputConfig.generated.h"

// SANDBOX: Trong thực tế = FPaldarkInputAction
// Thay UInputAction* bằng FString vì sandbox không dùng Enhanced Input system thật
USTRUCT(BlueprintType)
struct FSandboxInputAction
{
    GENERATED_BODY()

    // SANDBOX: Trong thực tế = TObjectPtr<const UInputAction> InputAction
    // Placeholder: tên action dạng string, đủ để log/verify
    FString ActionName;

    // SANDBOX: Trong thực tế = FGameplayTag InputTag
    // Dùng FGameplayTag thật vì sandbox có thể register tags
    FGameplayTag InputTag;
};

UCLASS(BlueprintType)
class USandboxInputConfig : public UObject
{
    GENERATED_BODY()
public:
    // SANDBOX: Trong thực tế = UPaldarkInputConfig::NativeInputActions
    TArray<FSandboxInputAction> NativeInputActions;

    // SANDBOX: Trong thực tế = UPaldarkInputConfig::AbilityInputActions
    TArray<FSandboxInputAction> AbilityInputActions;

    // SANDBOX: Trong thực tế = UPaldarkInputConfig::FindNativeInputActionForTag
    // Linear scan by tag — returns ActionName or empty string
    FString FindNativeActionForTag(const FGameplayTag& Tag, bool bLogNotFound = true) const;

    // SANDBOX: Trong thực tế = UPaldarkInputConfig::FindAbilityInputActionForTag
    FString FindAbilityActionForTag(const FGameplayTag& Tag, bool bLogNotFound = true) const;
};
