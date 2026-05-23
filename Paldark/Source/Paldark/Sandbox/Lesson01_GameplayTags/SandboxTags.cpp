#include "SandboxTags.h"

namespace SandboxTags
{
    // Mỗi UE_DEFINE_GAMEPLAY_TAG register vào UGameplayTagsManager lúc static init
    // SANDBOX: Trong thực tế có 100+ dòng như thế này
    UE_DEFINE_GAMEPLAY_TAG(TAG_Sandbox_Role_Tank,    "Sandbox.Role.Tank");
    UE_DEFINE_GAMEPLAY_TAG(TAG_Sandbox_Role_Healer,  "Sandbox.Role.Healer");
    UE_DEFINE_GAMEPLAY_TAG(TAG_Sandbox_Role_DPS,     "Sandbox.Role.DPS");

    UE_DEFINE_GAMEPLAY_TAG(TAG_Sandbox_State_Alive,  "Sandbox.State.Alive");
    UE_DEFINE_GAMEPLAY_TAG(TAG_Sandbox_State_Dead,   "Sandbox.State.Dead");
}
