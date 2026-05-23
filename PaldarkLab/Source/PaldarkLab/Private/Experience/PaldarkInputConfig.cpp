#include "Experience/PaldarkInputConfig.h"

#include "InputAction.h"

#include "PaldarkLogCategories.h"

UPaldarkInputConfig::UPaldarkInputConfig() = default;

FPrimaryAssetId UPaldarkInputConfig::GetPrimaryAssetId() const
{
	// Matches the type registered in DefaultGame.ini under
	// [/Script/Engine.AssetManagerSettings].
	return FPrimaryAssetId(TEXT("PaldarkInputConfig"), GetFName());
}

const UInputAction* UPaldarkInputConfig::FindNativeInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound) const
{
	for (const FPaldarkInputAction& Row : NativeInputActions)
	{
		if (Row.InputAction != nullptr && Row.InputTag == InputTag)
		{
			return Row.InputAction;
		}
	}
	if (bLogNotFound)
	{
		UE_LOG(LogPaldark, Warning, TEXT("PaldarkInputConfig %s — no NativeInputAction bound for tag %s."),
			*GetName(), *InputTag.ToString());
	}
	return nullptr;
}

const UInputAction* UPaldarkInputConfig::FindAbilityInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound) const
{
	for (const FPaldarkInputAction& Row : AbilityInputActions)
	{
		if (Row.InputAction != nullptr && Row.InputTag == InputTag)
		{
			return Row.InputAction;
		}
	}
	if (bLogNotFound)
	{
		UE_LOG(LogPaldark, Warning, TEXT("PaldarkInputConfig %s — no AbilityInputAction bound for tag %s."),
			*GetName(), *InputTag.ToString());
	}
	return nullptr;
}
