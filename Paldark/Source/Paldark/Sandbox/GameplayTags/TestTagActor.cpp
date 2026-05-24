#include "TestTagActor.h"

#include "SandboxLogCategories.h"
#include "TestTagSubsystem.h"

ATestTagActor::ATestTagActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATestTagActor::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogSandboxTags, Display, TEXT("--- ATestTagActor::BeginPlay → re-running tag tests in level context ---"));
	UTestTagSubsystem::RunAllTests();
}
