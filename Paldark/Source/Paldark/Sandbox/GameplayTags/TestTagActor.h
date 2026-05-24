// Sandbox Lesson 01 — drop this actor into a level if you want to verify tags
// re-run in a level/world context (after subsystem already ran on Initialize).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TestTagActor.generated.h"

UCLASS()
class PALDARK_API ATestTagActor : public AActor
{
	GENERATED_BODY()

public:
	ATestTagActor();

protected:
	virtual void BeginPlay() override;
};
