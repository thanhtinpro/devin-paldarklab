// Sandbox Experience tag registry — mirrors PaldarkGameplayTags.h pattern
// SANDBOX: Register các tag cần thiết cho Lesson 02 Experience System tests
// Trong thực tế đây nằm trong PaldarkLabCore/Public/PaldarkGameplayTags.h

#pragma once
#include "NativeGameplayTags.h"

namespace SandboxExpTags
{
    // Mode tags — which experience is active
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Sandbox_Mode_Raid);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Sandbox_Mode_Hub);

    // Feature tags — granted by ActionSets
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Sandbox_Feature_Combat);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Sandbox_Feature_LagComp);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Sandbox_Feature_Trading);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Sandbox_Feature_Stable);
}
