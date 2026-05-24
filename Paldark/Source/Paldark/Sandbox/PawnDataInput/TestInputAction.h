// Sandbox Lesson 03 — placeholder for UInputAction.
//
// In real Paldark this is `UInputAction` from EnhancedInput (object with
// ValueType=Bool/Axis2D + triggers + modifiers). For sandbox we only need
// an identity FName so test logs can show which action got dispatched.
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TestInputAction.generated.h"

UCLASS(BlueprintType)
class PALDARK_API UTestInputAction : public UObject
{
	GENERATED_BODY()

public:
	// SANDBOX: thực tế là FPrimaryAssetId "IA_Move" trỏ tới UInputAction asset.
	UPROPERTY()
	FName ActionId;
};
