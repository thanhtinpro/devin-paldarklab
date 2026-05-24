// Sandbox Lesson 03 — PawnData: per-pawn customization manifest.
//
// Single Character C++ class can be "configured" by different PawnData rows:
//   KBM PawnData    -> InputConfig with WASD-friendly actions
//   Gamepad PawnData -> InputConfig with stick-based Move/Look actions
//   Spectator PawnData -> InputConfig with only Look + Interact (no Move, no Jump)
//
// Same Pawn class, different behaviour, no inheritance explosion.
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TestInputConfig.h"
#include "TestPawnData.generated.h"

UCLASS(BlueprintType)
class PALDARK_API UTestPawnData : public UObject
{
	GENERATED_BODY()

public:
	// SANDBOX: thực tế là FName "PD_Default" / "PD_Gamepad" — debug-friendly id.
	UPROPERTY()
	FName PawnDataName;

	// SANDBOX: thực tế đây là TSubclassOf<APawn>. Sandbox dùng FName placeholder
	// để chứng minh "data points to class" mà không cần subclass APawn thật.
	UPROPERTY()
	FName PawnClassName;

	// The data-driven input scheme for this pawn.
	UPROPERTY()
	UTestInputConfig* InputConfig = nullptr;
};
