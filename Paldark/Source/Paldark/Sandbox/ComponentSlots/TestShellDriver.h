// Sandbox Lesson 04 — auto driver: spawns a ATestShellPawn on world begin
// play and runs the test suite on the next tick (after the pawn's BeginPlay
// has finished brokering its components).
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TestShellDriver.generated.h"

class ATestShellPawn;

UCLASS()
class PALDARK_API UTestShellDriver : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

protected:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
	void OnWorldBeginPlay();
	void RunAllTestCases();

	UPROPERTY()
	ATestShellPawn* SpawnedPawn = nullptr;

	FDelegateHandle WorldBeginPlayHandle;
};
