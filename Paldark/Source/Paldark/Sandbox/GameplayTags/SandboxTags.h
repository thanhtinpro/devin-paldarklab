// Sandbox Lesson 01 — native gameplay tag declarations.
//
// WHY native (UE_DEFINE_GAMEPLAY_TAG_COMMENT) over FName("Some.Tag") strings:
//   1. Compile-time safety: typo -> linker error, not silent runtime invalid.
//   2. O(1) comparison: each FGameplayTag is a single shared FName/index. Equality
//      is integer compare, not string compare.
//   3. IDE autocomplete: typing TAG_Sandbox_ shows all registered tags.
//   4. Auto-registration: static constructor adds to the tag manager at module load.
#pragma once

#include "NativeGameplayTags.h"

// SANDBOX: trong thực tế đây là các tag thật như TAG_Status_Death_Dying,
// TAG_GameplayEvent_Damage, TAG_InputTag_Move ... — ở đây chỉ giữ tối thiểu
// để chứng minh: parent/child, container query, typo safety.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Sandbox_State);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Sandbox_State_Sprinting);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Sandbox_State_Crouching);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Sandbox_Ability_Sprint);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Sandbox_Ability_Jump);
