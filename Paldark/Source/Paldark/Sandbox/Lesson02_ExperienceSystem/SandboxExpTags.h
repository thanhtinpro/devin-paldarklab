// Sandbox tags for Experience System lesson
// SANDBOX: Trong thực tế = PaldarkGameplayTags.h với tags như TAG_Paldark_Experience_Raid, TAG_Paldark_Experience_Hub
#pragma once
#include "NativeGameplayTags.h"

namespace SandboxExpTags
{
    // Mode tags
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Sandbox_Mode_Raid);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Sandbox_Mode_Hub);

    // Feature tags
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Sandbox_Feature_Combat);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Sandbox_Feature_LagComp);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Sandbox_Feature_Trading);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Sandbox_Feature_Stable);
}
