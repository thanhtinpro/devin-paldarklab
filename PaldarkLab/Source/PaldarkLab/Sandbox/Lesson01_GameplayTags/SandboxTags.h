// Sandbox tag registry — mirrors PaldarkGameplayTags.h pattern
// SANDBOX: Trong thực tế đây là PaldarkLabCore/Public/PaldarkGameplayTags.h
// với 100+ tags chia theo namespace (Experience, Pawn, InputTag, Ability, State, etc.)

#pragma once
#include "NativeGameplayTags.h"

// SANDBOX: Trong thực tế đây là PALDARKLABCORE_API
// Không cần API macro vì sandbox nằm cùng module

namespace SandboxTags
{
	// SANDBOX: Trong thực tế = TAG_Paldark_Pawn_Player / TAG_Paldark_Pawn_Pal
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Sandbox_Role_Tank);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Sandbox_Role_Healer);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Sandbox_Role_DPS);

	// SANDBOX: Trong thực tế = TAG_Paldark_State_IsDead / TAG_Paldark_State_Sprinting
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Sandbox_State_Alive);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Sandbox_State_Dead);
}
