// Sandbox Lesson 03 — InputConfig is a Tag <-> Action mapping table.
//
// Design point: this is the file designers edit when they want to retarget
// what "Input.Jump" physically does (different action on KBM vs gamepad).
// The Pawn / Character class itself does NOT know which UInputAction is
// jump — it only knows "bind whatever is mapped to TAG_Input_Jump".
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"
#include "TestInputAction.h"
#include "TestInputConfig.generated.h"

USTRUCT()
struct FTestInputActionBinding
{
	GENERATED_BODY()

	// Intent tag (TAG_Input_Move, TAG_Input_Jump, ...). Stable, drives code.
	UPROPERTY()
	FGameplayTag InputTag;

	// Concrete action that fires when this tag's input is triggered.
	UPROPERTY()
	UTestInputAction* InputAction = nullptr;
};

UCLASS(BlueprintType)
class PALDARK_API UTestInputConfig : public UObject
{
	GENERATED_BODY()

public:
	// SANDBOX: trong thực tế là TArray<FInputAction> hoặc TMap<FGameplayTag, UInputAction*>.
	UPROPERTY()
	TArray<FTestInputActionBinding> NativeInputActions;

	// Returns nullptr if tag is not bound — caller is expected to handle
	// gracefully (log + ignore), not crash.
	UTestInputAction* FindActionByTag(const FGameplayTag& InputTag) const
	{
		for (const FTestInputActionBinding& B : NativeInputActions)
		{
			if (B.InputTag == InputTag)
			{
				return B.InputAction;
			}
		}
		return nullptr;
	}
};
