// USandboxCharacterSim.h
// SANDBOX: Gộp logic từ APaldarkCharacter + APaldarkPlayerController
// vào 1 class để tối giản. Trong thực tế:
//   - APaldarkPlayerController::OnPossess → ResolvePawnData → SetPawnData → ApplyMappingContexts
//   - APaldarkCharacter::SetupPlayerInputComponent → BindNativeInputActions + BindAbilityInputActions

#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "USandboxCharacterSim.generated.h"

class USandboxPawnData;
class USandboxInputConfig;

UCLASS()
class USandboxCharacterSim : public UObject
{
    GENERATED_BODY()
public:
    // Mirrors APaldarkPlayerController::OnPossess flow
    void SimulateOnPossess(USandboxPawnData* PawnData);

    // Query bound actions
    FString GetBoundHandlerForTag(const FGameplayTag& Tag) const;
    TArray<FString> GetActiveMappingContexts() const;
    int32 GetNativeBindCount() const;
    int32 GetAbilityBindCount() const;
    const USandboxPawnData* GetPawnData() const;

    // Simulated input press/release — returns log of what happened
    FString SimulateInputPress(const FGameplayTag& InputTag);
    FString SimulateInputRelease(const FGameplayTag& InputTag);

private:
    // SANDBOX: Trong thực tế = APaldarkCharacter::CachedPawnData
    const USandboxPawnData* CachedPawnData = nullptr;

    // SANDBOX: Trong thực tế = UEnhancedInputComponent bindings
    // Simplified: Tag → "handler function name"
    TMap<FString, FString> NativeBindings; // tag string → handler name
    TMap<FString, FString> AbilityBindings; // tag string → handler name

    // SANDBOX: Trong thực tế = list of active IMCs on UEnhancedInputLocalPlayerSubsystem
    TArray<FString> ActiveMappingContexts;

    bool bNativeInputBound = false;
    bool bAbilityInputBound = false;

    // Internal — mirrors APaldarkCharacter methods
    void BindNativeInputActions(const USandboxInputConfig* Config);
    void BindAbilityInputActions(const USandboxInputConfig* Config);
    void ApplyMappingContexts(const USandboxPawnData* PawnData);
};
