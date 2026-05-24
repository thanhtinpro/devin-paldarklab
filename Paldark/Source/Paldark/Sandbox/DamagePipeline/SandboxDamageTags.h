// Sandbox Lesson 07 — tags used by the damage pipeline.
//
// SetByCaller.Damage: key the caller (weapon/ability) uses to deposit
// the base damage value into the GE spec. ExecutionCalculation reads it.
//
// DamageType.True: dynamic granted tag on the GE spec — when present,
// Execution bypasses armor mitigation (true damage).
#pragma once

#include "NativeGameplayTags.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Sandbox_SetByCaller_Damage);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Sandbox_DamageType_True);
