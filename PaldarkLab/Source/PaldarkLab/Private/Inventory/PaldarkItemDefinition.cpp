#include "Inventory/PaldarkItemDefinition.h"

#include "Inventory/PaldarkItemFragment.h"

UPaldarkItemDefinition::UPaldarkItemDefinition() = default;

UPaldarkItemFragment* UPaldarkItemDefinition::FindFragmentByClass(TSubclassOf<UPaldarkItemFragment> FragmentClass) const
{
	if (FragmentClass == nullptr)
	{
		return nullptr;
	}
	for (UPaldarkItemFragment* Fragment : Fragments)
	{
		if (Fragment != nullptr && Fragment->IsA(FragmentClass))
		{
			return Fragment;
		}
	}
	return nullptr;
}

FString UPaldarkItemDefinition::GetDebugLabel() const
{
	if (!DisplayName.IsEmpty())
	{
		return DisplayName.ToString();
	}
	if (ItemTag.IsValid())
	{
		return ItemTag.ToString();
	}
	return GetName();
}

FPrimaryAssetId UPaldarkItemDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("PaldarkItem"), GetFName());
}
