// Sandbox Lesson 01 — auto-runs the 6 gameplay-tag test cases when a GameInstance
// is created (PIE Play, standalone game, dedicated server). No level setup needed.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TestTagSubsystem.generated.h"

UCLASS()
class PALDARK_API UTestTagSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Static so ATestTagActor (or future actors) can re-run the same suite in-level.
	static void RunAllTests();
};
