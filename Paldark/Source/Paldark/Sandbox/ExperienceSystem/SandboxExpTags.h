// Sandbox Lesson 02 — tags used by the Experience System sandbox.
// Two families:
//   Experience.*  — identity tags (1-per-experience, useful for queries like "is this a raid?")
//   Granted.*     — gameplay capability tags handed out by ActionSets
//   Intrinsic.*   — capability tags every player gets regardless of experience
#pragma once

#include "NativeGameplayTags.h"

// SANDBOX: thực tế có thể là Experience.Raid.Heist, Experience.Hub.Lobby, ...
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Experience_Raid);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Experience_Hub);

// SANDBOX: thực tế là Ability.Reload / Status.HasVoiceChat / Input.Move
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Intrinsic_PlayerBase);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Granted_Combat);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Granted_Social);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Granted_Voice);
