// Sandbox Lesson 04 — Inventory slot.
//
// Just a list of item ids and add/drop methods. Knows nothing about Health;
// Health.OnHealthZeroed is wired to DropAll by the shell pawn (broker pattern).
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TestInventoryComponent.generated.h"

UCLASS(ClassGroup=(Sandbox), meta=(BlueprintSpawnableComponent))
class PALDARK_API UTestInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTestInventoryComponent();

	// SANDBOX: thực tế là TArray<FInstancedStruct<FInventoryEntry>> với
	// stack count, durability, instance id — đây chỉ FName để chứng minh
	// state isolation giữa các component.
	UPROPERTY()
	TArray<FName> Items;

	void AddItem(FName ItemId);
	void DropAll();
};
