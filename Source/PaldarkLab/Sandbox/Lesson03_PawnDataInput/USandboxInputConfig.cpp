#include "USandboxInputConfig.h"

DEFINE_LOG_CATEGORY_STATIC(LogSandboxInputCfg, Log, All);

FString USandboxInputConfig::FindNativeActionForTag(const FGameplayTag& Tag, bool bLogNotFound) const
{
    // Mirrors PaldarkInputConfig.cpp line 16-31
    for (const FSandboxInputAction& Row : NativeInputActions)
    {
        if (!Row.ActionName.IsEmpty() && Row.InputTag == Tag)
        {
            return Row.ActionName;
        }
    }
    if (bLogNotFound)
    {
        UE_LOG(LogSandboxInputCfg, Warning, TEXT("No NativeInputAction for tag %s"), *Tag.ToString());
    }
    return TEXT("");
}

FString USandboxInputConfig::FindAbilityActionForTag(const FGameplayTag& Tag, bool bLogNotFound) const
{
    // Mirrors PaldarkInputConfig.cpp line 33-47
    for (const FSandboxInputAction& Row : AbilityInputActions)
    {
        if (!Row.ActionName.IsEmpty() && Row.InputTag == Tag)
        {
            return Row.ActionName;
        }
    }
    if (bLogNotFound)
    {
        UE_LOG(LogSandboxInputCfg, Warning, TEXT("No AbilityInputAction for tag %s"), *Tag.ToString());
    }
    return TEXT("");
}
