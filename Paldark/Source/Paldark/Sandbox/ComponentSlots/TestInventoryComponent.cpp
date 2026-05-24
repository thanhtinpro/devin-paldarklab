#include "TestInventoryComponent.h"

#include "SandboxShellLog.h"

UTestInventoryComponent::UTestInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTestInventoryComponent::AddItem(FName ItemId)
{
	Items.Add(ItemId);
	UE_LOG(LogSandboxShell, Display, TEXT("Inventory: +%s (size now %d)"), *ItemId.ToString(), Items.Num());
}

void UTestInventoryComponent::DropAll()
{
	const int32 Dropped = Items.Num();
	Items.Reset();
	UE_LOG(LogSandboxShell, Display, TEXT("Inventory: DropAll -> dropped %d items"), Dropped);
}
