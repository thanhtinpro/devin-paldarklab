#include "SandboxTags.h"

// UE 4.27: macro is UE_DEFINE_GAMEPLAY_TAG(VarName, "Tag.String") — no comment variant
// (the _COMMENT version exists in UE 5.x). Each macro = one global FNativeGameplayTag
// whose static constructor registers the tag with UGameplayTagsManager at module load,
// no .ini required.
UE_DEFINE_GAMEPLAY_TAG(TAG_Sandbox_State,            "Sandbox.State");
UE_DEFINE_GAMEPLAY_TAG(TAG_Sandbox_State_Sprinting,  "Sandbox.State.Sprinting");
UE_DEFINE_GAMEPLAY_TAG(TAG_Sandbox_State_Crouching,  "Sandbox.State.Crouching");
UE_DEFINE_GAMEPLAY_TAG(TAG_Sandbox_Ability_Sprint,   "Sandbox.Ability.Sprint");
UE_DEFINE_GAMEPLAY_TAG(TAG_Sandbox_Ability_Jump,     "Sandbox.Ability.Jump");
