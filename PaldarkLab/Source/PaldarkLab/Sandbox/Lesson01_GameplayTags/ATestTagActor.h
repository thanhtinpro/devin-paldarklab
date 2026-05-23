// ATestTagActor.h
#pragma once
#include "GameFramework/Actor.h"
#include "ATestTagActor.generated.h"

UCLASS()
class ATestTagActor : public AActor
{
	GENERATED_BODY()
public:
	virtual void BeginPlay() override;

private:
	// Mỗi hàm = 1 test case
	void Test01_NativeTagExists();
	void Test02_NativeTagCompareCheap();
	void Test03_StringLookup_TypoFailsSilently();
	void Test04_NativeTag_TypoFailsAtCompile();
	void Test05_TagHierarchy_ParentChildMatch();
	void Test06_LogCategory_FilterBySubsystem();
	void Test07_TagContainer_HasTag();

	// Helper
	void LogResult(const FString& TestName, bool bPassed, const FString& Detail);
};
