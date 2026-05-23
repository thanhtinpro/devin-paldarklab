#!/usr/bin/env python3
"""Structural validation for PaldarkLab/ — runs in CI without UE5 SDK.

Checks (all must pass):
1. PaldarkLab.uproject parses as JSON and declares the 3 expected modules
   with correct loading phases (PreDefault / Default / Editor).
2. Each declared module has a matching folder + Build.cs whose class name
   equals "<Module>ModuleRules".
3. Each declared module has an .h + .cpp with IMPLEMENT_(PRIMARY_)GAME_MODULE.
4. The 3 Target.cs files exist and declare the expected target classes.
5. PaldarkLogCategories.h declares exactly the 5 log categories listed in
   Config/DefaultEngine.ini [Core.Log], and PaldarkLabCore.cpp defines
   exactly those categories.
6. No file under PaldarkLab/Source/ has a UTF-8 BOM (roadmap W1 day 3).
7. The W1 day 6-7 experience framework: each expected header under
   `Public/Framework/` and `Public/Experience/` declares the right UCLASS,
   has a matching .cpp, and DefaultEngine.ini / DefaultGame.ini wire the
   GameInstance / GameMode / AssetManager / primary asset types.
"""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
PROJECT_DIR = REPO_ROOT / "PaldarkLab"

EXPECTED_MODULES = {
    "PaldarkLab": {"Type": "Runtime", "LoadingPhase": "Default"},
    "PaldarkLabCore": {"Type": "Runtime", "LoadingPhase": "PreDefault"},
    "PaldarkLabEditor": {"Type": "Editor", "LoadingPhase": "Default"},
}

EXPECTED_TARGETS = {
    "PaldarkLab.Target.cs": "PaldarkLabTarget",
    "PaldarkLabEditor.Target.cs": "PaldarkLabEditorTarget",
    "PaldarkLabServer.Target.cs": "PaldarkLabServerTarget",
}

EXPECTED_LOG_CATEGORIES = {
    "LogPaldark",
    "LogPaldarkPal",
    "LogPaldarkInventory",
    "LogPaldarkNet",
    "LogPaldarkGAS",
    # W44-45 — Hub UI (Stable + Marketplace controllers + UMG widget stubs +
    # 8 designer console commands). Kept separate so the noisy
    # "click → server RPC → client callback → widget refresh" trace doesn't
    # bleed into LogPaldark when designers are debugging marketplace pricing
    # or stable swaps.
    "LogPaldarkUI",
    # W42-43 — AWS backend integration. Kept distinct from LogPaldarkNet so
    # the noisy HTTP request/response lines do not drown out the
    # listen-server / lag-comp logs designers already grep.
    "LogPaldarkBackend",
}

# W1 day 6-7 + W1 day 8-10 — framework + experience + player headers we expect
# to ship. Map: header path (relative to PaldarkLab/Source/PaldarkLab/Public) ->
# class name expected to appear via UCLASS macro in that header. Each header
# also requires a matching .cpp under .../Private/<same-subfolder>/<basename>.cpp.
EXPECTED_FRAMEWORK_CLASSES = {
    "Framework/PaldarkAssetManager.h": "UPaldarkAssetManager",
    "Framework/PaldarkGameInstance.h": "UPaldarkGameInstance",
    "Framework/PaldarkGameModeBase.h": "APaldarkGameModeBase",
    "Framework/PaldarkGameStateBase.h": "APaldarkGameStateBase",
    "Experience/PaldarkExperienceDefinition.h": "UPaldarkExperienceDefinition",
    "Experience/PaldarkPawnData.h": "UPaldarkPawnData",
    # W1 day 8-10 additions.
    "Experience/PaldarkExperienceActionSet.h": "UPaldarkExperienceActionSet",
    "Player/PaldarkPlayerController.h": "APaldarkPlayerController",
    "Player/PaldarkPlayerState.h": "APaldarkPlayerState",
    "Player/PaldarkCharacter.h": "APaldarkCharacter",
    # W1 day 11-14 additions.
    "Experience/PaldarkInputConfig.h": "UPaldarkInputConfig",
    # W3-4 additions — Pal pawn root.
    "Pal/PaldarkPalCharacter.h": "APaldarkPalCharacter",
    # W27 additions — AnimInstance base + Pal subclass. Designer authors
    # `ABP_Paldark_Player` / `ABP_Paldark_Pal` as Blueprint subclasses of
    # these. The Lyra-style layer interface (`PaldarkAnimLayerInterface.h`,
    # `UINTERFACE` macro, header-only) is NOT listed here because this map is
    # restricted to `UCLASS` headers with a paired `.cpp`. The interface's
    # shape is checked instead by `check_anim_instance_shape`.
    "Anim/PaldarkAnimInstance.h":    "UPaldarkAnimInstance",
    "Anim/PaldarkPalAnimInstance.h": "UPaldarkPalAnimInstance",
}

# W3-4 — 12 player component slots + 8 Pal component slots. All must be empty
# UActorComponent skeleton classes; the locomotion one (Pal) has real follow
# logic but the validator only checks shape, not behaviour.
EXPECTED_PLAYER_COMPONENT_CLASSES = {
    "Player/Components/PaldarkPlayerHealthComponent.h":           "UPaldarkPlayerHealthComponent",
    "Player/Components/PaldarkPlayerStaminaComponent.h":          "UPaldarkPlayerStaminaComponent",
    "Player/Components/PaldarkPlayerCombatComponent.h":           "UPaldarkPlayerCombatComponent",
    "Player/Components/PaldarkPlayerInventoryComponent.h":        "UPaldarkPlayerInventoryComponent",
    "Player/Components/PaldarkPlayerEquipmentComponent.h":        "UPaldarkPlayerEquipmentComponent",
    "Player/Components/PaldarkPlayerPalCompanionComponent.h":     "UPaldarkPlayerPalCompanionComponent",
    "Player/Components/PaldarkPlayerLocomotionExtComponent.h":    "UPaldarkPlayerLocomotionExtComponent",
    "Player/Components/PaldarkPlayerActivityComponent.h":         "UPaldarkPlayerActivityComponent",
    "Player/Components/PaldarkPlayerInteractionComponent.h":      "UPaldarkPlayerInteractionComponent",
    "Player/Components/PaldarkPlayerCameraExtensionComponent.h":  "UPaldarkPlayerCameraExtensionComponent",
    "Player/Components/PaldarkPlayerNetworkComponent.h":          "UPaldarkPlayerNetworkComponent",
    "Player/Components/PaldarkPlayerDamageComponent.h":           "UPaldarkPlayerDamageComponent",
}
EXPECTED_PAL_COMPONENT_CLASSES = {
    "Pal/Components/PaldarkPalLocomotionComponent.h":  "UPaldarkPalLocomotionComponent",
    "Pal/Components/PaldarkPalHealthComponent.h":      "UPaldarkPalHealthComponent",
    "Pal/Components/PaldarkPalCombatComponent.h":      "UPaldarkPalCombatComponent",
    "Pal/Components/PaldarkPalBondComponent.h":        "UPaldarkPalBondComponent",
    "Pal/Components/PaldarkPalActivityComponent.h":    "UPaldarkPalActivityComponent",
    "Pal/Components/PaldarkPalDataComponent.h":        "UPaldarkPalDataComponent",
    "Pal/Components/PaldarkPalPerceptionComponent.h":  "UPaldarkPalPerceptionComponent",
    "Pal/Components/PaldarkPalAnimDriverComponent.h":  "UPaldarkPalAnimDriverComponent",
    # W20-21 — 9th Pal component slot (waypoint list for hostile patrol).
    "Pal/Components/PaldarkPalPatrolComponent.h":      "UPaldarkPalPatrolComponent",
}

# W1 day 8-10 — PaldarkGameplayTags lives in PaldarkLabCore (used by both game
# module and editor module). Validated separately because the singleton is a
# namespace + native tag macros, not a UCLASS.
EXPECTED_GAMEPLAY_TAG_HEADER = (
    "Source/PaldarkLabCore/Public/PaldarkGameplayTags.h",
    "Source/PaldarkLabCore/Private/PaldarkGameplayTags.cpp",
)
EXPECTED_GAMEPLAY_TAGS = {
    "TAG_Paldark_Experience_Default",
    "TAG_Paldark_Experience_RaidSandbox",
    "TAG_Paldark_Pawn_Player",
    "TAG_Paldark_Pawn_Pal",
    # W1 day 11-14 — Enhanced Input lookup keys for UPaldarkInputConfig.
    "TAG_Paldark_InputTag_Move",
    "TAG_Paldark_InputTag_Look",
    "TAG_Paldark_InputTag_Jump",
    # W5-6 — Activity FSM identity tags.
    "TAG_Paldark_Pal_Activity_Idle",
    "TAG_Paldark_Pal_Activity_Follow",
    "TAG_Paldark_Pal_Activity_Investigate",
    # W7-8 — Sprint ability tag chain (input → ability → owner state tag).
    "TAG_Paldark_InputTag_Sprint",
    "TAG_Paldark_Ability_Sprint",
    "TAG_Paldark_State_Sprinting",
    # W9-10 — Fire ability tag chain + damage SetByCaller magnitudes.
    "TAG_Paldark_InputTag_Fire",
    "TAG_Paldark_Ability_Fire",
    "TAG_Paldark_Hit_Headshot",
    "TAG_Paldark_State_IsDead",
    "TAG_Paldark_SetByCaller_BaseDamage",
    "TAG_Paldark_SetByCaller_HeadshotMultiplier",
    # W11-12 — Inventory item identity tags (one per authored DataAsset).
    "TAG_Paldark_Item_Pistol",
    "TAG_Paldark_Item_Ammo_9mm",
    "TAG_Paldark_Item_Bandage",
    "TAG_Paldark_Item_Pal_Sphere",
    "TAG_Paldark_Item_Energy_Drink",
    # W11-12 — Equipment slot tags (Equipable fragment EquipSlot).
    "TAG_Paldark_Equip_Slot_Primary",
    "TAG_Paldark_Equip_Slot_Consumable",
    "TAG_Paldark_Equip_Slot_Throwable",
    # W14-15 — Network role + topology tags (read by UPaldarkNetSubsystem).
    "TAG_Paldark_Net_Role_Host",
    "TAG_Paldark_Net_Role_Client",
    "TAG_Paldark_Net_Mode_Standalone",
    "TAG_Paldark_Net_Mode_ListenServer",
    "TAG_Paldark_Net_Mode_DedicatedServer",
    # W16-17 — Server-side rewind result tags + Bodyshot companion.
    "TAG_Paldark_LagComp_HitConfirmed",
    "TAG_Paldark_LagComp_HitRejected",
    "TAG_Paldark_Hit_Bodyshot",
    # W18-19 — Pal combat activity / ability / team / damage SetByCaller.
    "TAG_Paldark_Pal_Activity_Combat",
    "TAG_Paldark_Ability_PalAttack",
    "TAG_Paldark_Team_Player",
    "TAG_Paldark_Team_Hostile",
    "TAG_Paldark_SetByCaller_PalDamage",
    # W20-21 — Hostile-Pal species / activity / pack identity tags.
    "TAG_Paldark_Pal_Species_Direhound",
    "TAG_Paldark_Pal_Species_Razorbird",
    "TAG_Paldark_Pal_Activity_Patrol",
    "TAG_Paldark_Pal_Activity_Stalk",
    "TAG_Paldark_Pal_Pack_Direhound",
    # W22-23 — Squad system tags (membership + radial command set + ping types
    # + input hotkeys + friendly-Pal listener activity).
    "TAG_Paldark_Squad_Default",
    "TAG_Paldark_Squad_Command_Attack",
    "TAG_Paldark_Squad_Command_Follow",
    "TAG_Paldark_Squad_Command_Stay",
    "TAG_Paldark_Squad_Command_Regroup",
    "TAG_Paldark_Ping_Type_Spot",
    "TAG_Paldark_Ping_Type_Enemy",
    "TAG_Paldark_Ping_Type_Loot",
    "TAG_Paldark_Ping_Type_Danger",
    "TAG_Paldark_InputTag_Ping",
    "TAG_Paldark_InputTag_MarkEnemy",
    "TAG_Paldark_InputTag_RadialOpen",
    "TAG_Paldark_Pal_Activity_FollowSquadCommand",
    # W24-25 — Match phase + per-player outcome + extraction input tags.
    "TAG_Paldark_Match_Phase_Warmup",
    "TAG_Paldark_Match_Phase_Active",
    "TAG_Paldark_Match_Phase_Extracting",
    "TAG_Paldark_Match_Phase_Ended",
    "TAG_Paldark_Match_Outcome_Extracted",
    "TAG_Paldark_Match_Outcome_KIA",
    "TAG_Paldark_Match_Outcome_Disconnected",
    "TAG_Paldark_InputTag_InteractExtraction",
    # W29-30 — 5-species roster (Stoneclad / Vinewraith / Foxparks /
    # Boltmane / Tombat) + 2 new pack tags (hostile species that come in
    # packs; boss + companions don't get one) + 1 new activity (boss FSM)
    # + 5 new ability tags (one per species ability subclass).
    "TAG_Paldark_Pal_Species_Stoneclad",
    "TAG_Paldark_Pal_Species_Vinewraith",
    "TAG_Paldark_Pal_Species_Foxparks",
    "TAG_Paldark_Pal_Species_Boltmane",
    "TAG_Paldark_Pal_Species_Tombat",
    "TAG_Paldark_Pal_Pack_Stoneclad",
    "TAG_Paldark_Pal_Pack_Vinewraith",
    "TAG_Paldark_Pal_Activity_BossPhase",
    "TAG_Paldark_Ability_PalAttack_Bite",
    "TAG_Paldark_Ability_PalAttack_VineWhip",
    "TAG_Paldark_Ability_PalAttack_Charge",
    "TAG_Paldark_Ability_PalAttack_FireBreath",
    "TAG_Paldark_Ability_PalAttack_ChainLightning",
    # W31-32 — Map 2 "Rừng Hỏng" blockout. POI type + danger tier + map
    # id + spawn point listener tags.
    "TAG_Paldark_POI_Type_Outpost",
    "TAG_Paldark_POI_Type_Cache",
    "TAG_Paldark_POI_Type_RuinedTower",
    "TAG_Paldark_POI_Type_MiniBoss",
    "TAG_Paldark_POI_Type_ExtractionPad",
    "TAG_Paldark_POI_Type_SafeZone",
    "TAG_Paldark_POI_DangerTier_Low",
    "TAG_Paldark_POI_DangerTier_Medium",
    "TAG_Paldark_POI_DangerTier_High",
    "TAG_Paldark_Map_RaidSandbox",
    "TAG_Paldark_Map_RungHong",
    "TAG_Paldark_Pal_Activity_SpawnPoint",
    # W33-34 — Map 3 "Cơ Xưởng PalCorp" + tech-themed loot drop. One map
    # id + one POI type (IndoorRoom) + 5 tech-themed item ids + 2 loot
    # table ids (HostileStandard + Boss).
    "TAG_Paldark_Map_PalCorp",
    "TAG_Paldark_POI_Type_IndoorRoom",
    "TAG_Paldark_Item_Tech_Battery",
    "TAG_Paldark_Item_Tech_Circuit",
    "TAG_Paldark_Item_Tech_Coolant",
    "TAG_Paldark_Item_Tech_Polymer",
    "TAG_Paldark_Item_Tech_Microchip",
    "TAG_Paldark_LootTable_HostileStandard",
    "TAG_Paldark_LootTable_Boss",
    # W35-36 — Pal Bond + Capture (Tame minigame). 1 ability + 3 PalSphere
    # item tiers + 3 Tame result tags + 3 Bond event reason tags.
    "TAG_Paldark_Ability_UsePalSphere",
    "TAG_Paldark_Item_PalSphere_T1",
    "TAG_Paldark_Item_PalSphere_T2",
    "TAG_Paldark_Item_PalSphere_T3",
    "TAG_Paldark_Tame_Result_Success",
    "TAG_Paldark_Tame_Result_Fail_HpTooHigh",
    "TAG_Paldark_Tame_Result_Fail_RngMiss",
    "TAG_Paldark_Bond_Event_Damage",
    "TAG_Paldark_Bond_Event_Tame",
    "TAG_Paldark_Bond_Event_ShareFood",
    # W37-38 — Inventory full. 17 item identity tags spanning Weapon /
    # Ammo / Consumable / Throwable / Resource, plus 3 backpack tier tags
    # (T1/T2/T3) for the Backpack fragment, plus 1 Equip.Slot.Backpack tag
    # that the equipable fragment slots into.
    "TAG_Paldark_Item_Weapon_Rifle",
    "TAG_Paldark_Item_Weapon_Shotgun",
    "TAG_Paldark_Item_Weapon_SMG",
    "TAG_Paldark_Item_Weapon_Melee_Baton",
    "TAG_Paldark_Item_Ammo_Rifle",
    "TAG_Paldark_Item_Ammo_Shotgun",
    "TAG_Paldark_Item_Ammo_SMG",
    "TAG_Paldark_Item_Consumable_Medkit",
    "TAG_Paldark_Item_Consumable_Stimpack",
    "TAG_Paldark_Item_Consumable_PalFood",
    "TAG_Paldark_Item_Throwable_Frag",
    "TAG_Paldark_Item_Throwable_Smoke",
    "TAG_Paldark_Item_Throwable_Flash",
    "TAG_Paldark_Item_Resource_Wood",
    "TAG_Paldark_Item_Resource_Stone",
    "TAG_Paldark_Item_Resource_PalCore",
    "TAG_Paldark_Item_Resource_Currency_Credits",
    "TAG_Paldark_Item_Backpack_T1",
    "TAG_Paldark_Item_Backpack_T2",
    "TAG_Paldark_Item_Backpack_T3",
    "TAG_Paldark_Equip_Slot_Backpack",
    # W40-41 — Hub Town (Q4 feature week #1). 1 map id + 1 experience id + 3
    # hub building type tags + 3 hub interact input tags + 2 mode-kind tags.
    "TAG_Paldark_Map_HubTown",
    "TAG_Paldark_Experience_HubTown",
    "TAG_Paldark_Hub_Building_PalStable",
    "TAG_Paldark_Hub_Building_Marketplace",
    "TAG_Paldark_Hub_Building_BriefingRoom",
    "TAG_Paldark_InputTag_Interact_Stable",
    "TAG_Paldark_InputTag_Interact_Marketplace",
    "TAG_Paldark_InputTag_Interact_Briefing",
    "TAG_Paldark_Mode_Hub",
    "TAG_Paldark_Mode_Raid",
    # W42-43 — AWS backend integration. 12 tags total (4 state + 5 result + 3
    # fleet). Mirrors UPaldarkBackendSubsystem::StateTag transitions and the
    # FPaldarkBackendError::ResultTag + FPaldarkFleetSpec::FleetTag fields.
    "TAG_Paldark_Backend_State_LoggedOut",
    "TAG_Paldark_Backend_State_Authenticating",
    "TAG_Paldark_Backend_State_LoggedIn",
    "TAG_Paldark_Backend_State_Refreshing",
    "TAG_Paldark_Backend_Result_Success",
    "TAG_Paldark_Backend_Result_Fail_Network",
    "TAG_Paldark_Backend_Result_Fail_Auth",
    "TAG_Paldark_Backend_Result_Fail_Timeout",
    "TAG_Paldark_Backend_Result_Fail_NoFleet",
    "TAG_Paldark_Backend_Fleet_Hub",
    "TAG_Paldark_Backend_Fleet_Raid",
    "TAG_Paldark_Backend_Fleet_Reserved",
    # W44-45 — Stable + Marketplace UI controller. 14 tags total
    # (4 UI state + 3 stable action + 2 market action + 5 shared result).
    "TAG_Paldark_Hub_UI_Closed",
    "TAG_Paldark_Hub_UI_OpeningStable",
    "TAG_Paldark_Hub_UI_OpeningMarket",
    "TAG_Paldark_Hub_UI_Open",
    "TAG_Paldark_Hub_Stable_Action_Deposit",
    "TAG_Paldark_Hub_Stable_Action_Withdraw",
    "TAG_Paldark_Hub_Stable_Action_Heal",
    "TAG_Paldark_Hub_Market_Action_Buy",
    "TAG_Paldark_Hub_Market_Action_Sell",
    "TAG_Paldark_Hub_Result_Success",
    "TAG_Paldark_Hub_Result_Fail_Empty",
    "TAG_Paldark_Hub_Result_Fail_Full",
    "TAG_Paldark_Hub_Result_Fail_Insufficient",
    "TAG_Paldark_Hub_Result_Fail_NotAllowed",
    # W46 — Briefing room widget + multi-player vote/ready handoff (Q4
    # hub QA pass). 10 tags total (3 UI state + 4 action + 3 phase).
    # Re-uses W44-45 `Paldark.Hub.Result.*` for action results.
    "TAG_Paldark_Hub_Brief_UI_Closed",
    "TAG_Paldark_Hub_Brief_UI_Opening",
    "TAG_Paldark_Hub_Brief_UI_Open",
    "TAG_Paldark_Hub_Brief_Action_Vote",
    "TAG_Paldark_Hub_Brief_Action_Unvote",
    "TAG_Paldark_Hub_Brief_Action_Ready",
    "TAG_Paldark_Hub_Brief_Action_Unready",
    "TAG_Paldark_Hub_Brief_Phase_Idle",
    "TAG_Paldark_Hub_Brief_Phase_Countdown",
    "TAG_Paldark_Hub_Brief_Phase_Travelling",
    # W47 — Save game system. 7 tags (4 result + 3 trigger). The result
    # tags are stamped onto OnSave/LoadCompleted delegate payloads; the
    # trigger tags are passed *into* RequestSaveSlot so listeners can
    # branch on "why did we save?". Console / auto-save / UMG picker all
    # converge on these tags.
    "TAG_Paldark_Save_Result_Success",
    "TAG_Paldark_Save_Result_Fail_IOError",
    "TAG_Paldark_Save_Result_Fail_SchemaMismatch",
    "TAG_Paldark_Save_Result_Fail_NoSlot",
    "TAG_Paldark_Save_Trigger_HubReturn",
    "TAG_Paldark_Save_Trigger_Logout",
    "TAG_Paldark_Save_Trigger_Manual",
    # W48 — Polish adds a 4th trigger for hub→raid handoff smoke + future
    # automatic ServerTravel boundary saves.
    "TAG_Paldark_Save_Trigger_Travel",
}

# W5-6 — Activity FSM classes (one base + 3 concrete). Same shape contract
# as EXPECTED_FRAMEWORK_CLASSES: header lives under .../Public/Pal/Activities/
# and must declare `class PALDARKLAB_API <ClassName>` + have a matching .cpp
# under .../Private/Pal/Activities/<basename>.cpp.
EXPECTED_ACTIVITY_CLASSES = {
    "Pal/Activities/PaldarkBaseActivity.h":          "UPaldarkBaseActivity",
    "Pal/Activities/PaldarkActivity_Idle.h":         "UPaldarkActivity_Idle",
    "Pal/Activities/PaldarkActivity_Follow.h":       "UPaldarkActivity_Follow",
    "Pal/Activities/PaldarkActivity_Investigate.h":  "UPaldarkActivity_Investigate",
    # W18-19 — Combat activity completes the FSM quartet.
    "Pal/Activities/PaldarkActivity_Combat.h":       "UPaldarkActivity_Combat",
    # W20-21 — Hostile activities (Patrol P15, Stalk P25).
    "Pal/Activities/PaldarkActivity_Patrol.h":       "UPaldarkActivity_Patrol",
    "Pal/Activities/PaldarkActivity_Stalk.h":        "UPaldarkActivity_Stalk",
    # W22-23 — Squad-command listener (P22) for friendly Pal companions.
    "Pal/Activities/PaldarkActivity_FollowSquadCommand.h": "UPaldarkActivity_FollowSquadCommand",
}

# W7-8 — GAS skeleton classes. AttributeSet derives from UAttributeSet (not
# UActorComponent), ASC derives from UAbilitySystemComponent, GAs derive from
# UGameplayAbility. The shape check is loosened to "class PALDARKLAB_API
# <ClassName>" (no fixed base-class match) because the engine base classes
# differ per file.
EXPECTED_GAS_CLASSES = {
    "Gas/PaldarkAttributeSet.h":                       "UPaldarkAttributeSet",
    "Gas/PaldarkAbilitySystemComponent.h":             "UPaldarkAbilitySystemComponent",
    "Gas/PaldarkGameplayAbility.h":                    "UPaldarkGameplayAbility",
    "Gas/Abilities/PaldarkGameplayAbility_Sprint.h":   "UPaldarkGameplayAbility_Sprint",
    # W9-10 damage chain — execution calc + hitscan ability.
    "Gas/PaldarkDamageExecutionCalculation.h":         "UPaldarkDamageExecutionCalculation",
    "Gas/Abilities/PaldarkGameplayAbility_HitscanFire.h": "UPaldarkGameplayAbility_HitscanFire",
    # W18-19 — Pal attack ability (AI-driven, server-only).
    "Gas/Abilities/PaldarkGameplayAbility_PalAttack.h":   "UPaldarkGameplayAbility_PalAttack",
}

# W9-10 — Combat actors (DummyTarget). Same shape contract as the framework
# classes: header under Public/Combat/, matching .cpp under Private/Combat/.
EXPECTED_COMBAT_CLASSES = {
    "Combat/PaldarkDummyTarget.h": "APaldarkDummyTarget",
}

# Pairs of (ini-relative-path, regex that must match). Used to assert that the
# framework classes are actually wired in config so the engine picks them up.
EXPECTED_INI_WIRING = [
    (
        "Config/DefaultEngine.ini",
        r"(?m)^GameInstanceClass\s*=\s*/Script/PaldarkLab\.PaldarkGameInstance\s*$",
    ),
    (
        "Config/DefaultEngine.ini",
        r"(?m)^GlobalDefaultGameMode\s*=\s*/Script/PaldarkLab\.PaldarkGameModeBase\s*$",
    ),
    (
        "Config/DefaultEngine.ini",
        r"(?m)^AssetManagerClassName\s*=\s*/Script/PaldarkLab\.PaldarkAssetManager\s*$",
    ),
    (
        "Config/DefaultGame.ini",
        r'PrimaryAssetType\s*=\s*"PaldarkExperience"',
    ),
    (
        "Config/DefaultGame.ini",
        r'PrimaryAssetType\s*=\s*"PaldarkPawnData"',
    ),
    # W1 day 8-10 — action set primary asset type.
    (
        "Config/DefaultGame.ini",
        r'PrimaryAssetType\s*=\s*"PaldarkExperienceActionSet"',
    ),
    # W1 day 11-14 — input config primary asset type.
    (
        "Config/DefaultGame.ini",
        r'PrimaryAssetType\s*=\s*"PaldarkInputConfig"',
    ),
    # W1 day 11-14 — Enhanced Input player + input component classes.
    (
        "Config/DefaultInput.ini",
        r"(?m)^DefaultPlayerInputClass\s*=\s*/Script/EnhancedInput\.EnhancedPlayerInput\s*$",
    ),
    (
        "Config/DefaultInput.ini",
        r"(?m)^DefaultInputComponentClass\s*=\s*/Script/EnhancedInput\.EnhancedInputComponent\s*$",
    ),
    # W14-15 — LAN OnlineSubsystem + 4-player MaxPlayers cap + IpNetDriver
    # block. These are the smallest possible config surface the dedicated
    # server test needs.
    (
        "Config/DefaultEngine.ini",
        r"(?ms)^\[OnlineSubsystem\][^\[]*?^DefaultPlatformService\s*=\s*NULL",
    ),
    (
        "Config/DefaultEngine.ini",
        r"(?ms)^\[/Script/Engine\.GameSession\][^\[]*?^MaxPlayers\s*=\s*4",
    ),
    (
        "Config/DefaultEngine.ini",
        r"(?ms)^\[/Script/OnlineSubsystemUtils\.IpNetDriver\][^\[]*?^NetServerMaxTickRate\s*=\s*\d+",
    ),
]

errors: list[str] = []


def fail(msg: str) -> None:
    errors.append(msg)


def check_uproject() -> dict | None:
    uproject = PROJECT_DIR / "PaldarkLab.uproject"
    if not uproject.is_file():
        fail(f"missing {uproject.relative_to(REPO_ROOT)}")
        return None
    try:
        data = json.loads(uproject.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        fail(f"{uproject.relative_to(REPO_ROOT)} is not valid JSON: {exc}")
        return None

    if data.get("FileVersion") != 3:
        fail(f"FileVersion must be 3, got {data.get('FileVersion')!r}")
    if data.get("EngineAssociation") != "5.4":
        fail(f"EngineAssociation must be \"5.4\", got {data.get('EngineAssociation')!r}")

    declared = {m.get("Name"): m for m in data.get("Modules", [])}
    for name, expected in EXPECTED_MODULES.items():
        mod = declared.get(name)
        if mod is None:
            fail(f"uproject is missing module {name!r}")
            continue
        for key, value in expected.items():
            if mod.get(key) != value:
                fail(
                    f"module {name!r} has {key}={mod.get(key)!r}, expected {value!r}"
                )
    extra = set(declared) - set(EXPECTED_MODULES)
    if extra:
        fail(f"uproject declares unexpected modules: {sorted(extra)}")
    return data


def check_module_layout() -> None:
    source = PROJECT_DIR / "Source"
    for module in EXPECTED_MODULES:
        mod_dir = source / module
        if not mod_dir.is_dir():
            fail(f"missing module folder {mod_dir.relative_to(REPO_ROOT)}")
            continue
        build_cs = mod_dir / f"{module}.Build.cs"
        if not build_cs.is_file():
            fail(f"missing {build_cs.relative_to(REPO_ROOT)}")
            continue
        text = build_cs.read_text(encoding="utf-8")
        # UE convention: `public class <Module> : ModuleRules`.
        pattern = rf"public\s+class\s+{re.escape(module)}\s*:\s*ModuleRules"
        if not re.search(pattern, text):
            fail(
                f"{build_cs.relative_to(REPO_ROOT)} does not declare "
                f"'public class {module} : ModuleRules'"
            )


def check_module_impl() -> None:
    source = PROJECT_DIR / "Source"

    primary = source / "PaldarkLab" / "PaldarkLab.cpp"
    if not primary.is_file():
        fail(f"missing {primary.relative_to(REPO_ROOT)}")
    elif "IMPLEMENT_PRIMARY_GAME_MODULE" not in primary.read_text(encoding="utf-8"):
        fail(f"{primary.relative_to(REPO_ROOT)} must use IMPLEMENT_PRIMARY_GAME_MODULE")

    for module in ("PaldarkLabCore", "PaldarkLabEditor"):
        cpp = source / module / "Private" / f"{module}.cpp"
        if not cpp.is_file():
            fail(f"missing {cpp.relative_to(REPO_ROOT)}")
            continue
        text = cpp.read_text(encoding="utf-8")
        if "IMPLEMENT_MODULE" not in text:
            fail(f"{cpp.relative_to(REPO_ROOT)} must use IMPLEMENT_MODULE")


def check_targets() -> None:
    source = PROJECT_DIR / "Source"
    for filename, classname in EXPECTED_TARGETS.items():
        path = source / filename
        if not path.is_file():
            fail(f"missing {path.relative_to(REPO_ROOT)}")
            continue
        text = path.read_text(encoding="utf-8")
        if classname not in text:
            fail(f"{path.relative_to(REPO_ROOT)} does not declare class {classname!r}")


def check_log_categories() -> None:
    header = PROJECT_DIR / "Source" / "PaldarkLabCore" / "Public" / "PaldarkLogCategories.h"
    cpp = PROJECT_DIR / "Source" / "PaldarkLabCore" / "Private" / "PaldarkLabCore.cpp"
    if not header.is_file():
        fail(f"missing {header.relative_to(REPO_ROOT)}")
        return
    if not cpp.is_file():
        fail(f"missing {cpp.relative_to(REPO_ROOT)}")
        return

    declared = set(re.findall(
        r"DECLARE_LOG_CATEGORY_EXTERN\(\s*(\w+)",
        header.read_text(encoding="utf-8"),
    ))
    defined = set(re.findall(
        r"DEFINE_LOG_CATEGORY\(\s*(\w+)",
        cpp.read_text(encoding="utf-8"),
    ))

    if declared != EXPECTED_LOG_CATEGORIES:
        fail(
            "log category DECLARE set mismatch: "
            f"got {sorted(declared)}, expected {sorted(EXPECTED_LOG_CATEGORIES)}"
        )
    if defined != EXPECTED_LOG_CATEGORIES:
        fail(
            "log category DEFINE set mismatch: "
            f"got {sorted(defined)}, expected {sorted(EXPECTED_LOG_CATEGORIES)}"
        )

    # DefaultEngine.ini must list each category under [Core.Log].
    ini = PROJECT_DIR / "Config" / "DefaultEngine.ini"
    ini_text = ini.read_text(encoding="utf-8") if ini.is_file() else ""
    for cat in EXPECTED_LOG_CATEGORIES:
        if not re.search(rf"(?m)^{cat}\s*=", ini_text):
            fail(f"DefaultEngine.ini missing {cat}= line under [Core.Log]")


def check_no_bom() -> None:
    bom = b"\xef\xbb\xbf"
    for path in (PROJECT_DIR / "Source").rglob("*"):
        if not path.is_file():
            continue
        if path.suffix.lower() not in {".cs", ".h", ".cpp", ".hpp", ".inl"}:
            continue
        with path.open("rb") as handle:
            head = handle.read(3)
        if head == bom:
            fail(f"{path.relative_to(REPO_ROOT)} has UTF-8 BOM (W1 day 3 rule)")


def check_framework_classes() -> None:
    """W1 day 6-7 — framework + experience headers exist, declare UCLASS, have .cpp."""
    public_root = PROJECT_DIR / "Source" / "PaldarkLab" / "Public"
    private_root = PROJECT_DIR / "Source" / "PaldarkLab" / "Private"

    for rel_header, class_name in EXPECTED_FRAMEWORK_CLASSES.items():
        header = public_root / rel_header
        if not header.is_file():
            fail(f"missing {header.relative_to(REPO_ROOT)}")
            continue
        text = header.read_text(encoding="utf-8")
        # Check `UCLASS` and `class PALDARKLAB_API <Name>` separately — UCLASS may
        # contain nested parentheses from `meta = (DisplayName = ...)` so a single
        # regex over the whole `UCLASS(...)` block would be fragile.
        if "UCLASS" not in text:
            fail(f"{header.relative_to(REPO_ROOT)} is missing a UCLASS macro.")
        class_pattern = rf"class\s+PALDARKLAB_API\s+{re.escape(class_name)}\b"
        if not re.search(class_pattern, text):
            fail(
                f"{header.relative_to(REPO_ROOT)} does not declare "
                f"`class PALDARKLAB_API {class_name}` (header drift)."
            )

        # Matching .cpp under Private/<same-subfolder>/<basename>.cpp.
        cpp = private_root / rel_header
        cpp = cpp.with_suffix(".cpp")
        if not cpp.is_file():
            fail(f"missing matching .cpp for {header.relative_to(REPO_ROOT)} (expected {cpp.relative_to(REPO_ROOT)})")


def check_ini_wiring() -> None:
    """W1 day 6-7 — DefaultEngine.ini / DefaultGame.ini wire the framework classes."""
    for rel_path, pattern in EXPECTED_INI_WIRING:
        ini = PROJECT_DIR / rel_path
        if not ini.is_file():
            fail(f"missing {ini.relative_to(REPO_ROOT)} (required for framework wiring)")
            continue
        text = ini.read_text(encoding="utf-8")
        if not re.search(pattern, text):
            fail(
                f"{ini.relative_to(REPO_ROOT)} is missing required wiring "
                f"matching regex: {pattern!r}"
            )


def check_gameplay_tags() -> None:
    """W1 day 8-10 — PaldarkGameplayTags native singleton declares + defines tags."""
    header_rel, cpp_rel = EXPECTED_GAMEPLAY_TAG_HEADER
    header = PROJECT_DIR / header_rel
    cpp = PROJECT_DIR / cpp_rel
    if not header.is_file():
        fail(f"missing {header.relative_to(REPO_ROOT)}")
        return
    if not cpp.is_file():
        fail(f"missing {cpp.relative_to(REPO_ROOT)}")
        return

    header_text = header.read_text(encoding="utf-8")
    cpp_text = cpp.read_text(encoding="utf-8")

    declared = set(re.findall(
        r"UE_DECLARE_GAMEPLAY_TAG_EXTERN\(\s*(\w+)",
        header_text,
    ))
    defined = set(re.findall(
        r"UE_DEFINE_GAMEPLAY_TAG\w*\(\s*(\w+)",
        cpp_text,
    ))

    if declared != EXPECTED_GAMEPLAY_TAGS:
        fail(
            "gameplay tag DECLARE set mismatch: "
            f"got {sorted(declared)}, expected {sorted(EXPECTED_GAMEPLAY_TAGS)}"
        )
    if defined != EXPECTED_GAMEPLAY_TAGS:
        fail(
            "gameplay tag DEFINE set mismatch: "
            f"got {sorted(defined)}, expected {sorted(EXPECTED_GAMEPLAY_TAGS)}"
        )


def check_gameplay_tags_dependency() -> None:
    """W1 day 8-10 — PaldarkLabCore.Build.cs must depend on GameplayTags so the
    NativeGameplayTags.h header resolves for downstream modules."""
    build_cs = PROJECT_DIR / "Source" / "PaldarkLabCore" / "PaldarkLabCore.Build.cs"
    if not build_cs.is_file():
        return  # other check will catch missing Build.cs
    text = build_cs.read_text(encoding="utf-8")
    if not re.search(r'"GameplayTags"', text):
        fail(
            f"{build_cs.relative_to(REPO_ROOT)} does not list \"GameplayTags\" in "
            "PublicDependencyModuleNames (required by PaldarkGameplayTags.h)."
        )


def check_enhanced_input_dependency() -> None:
    """W1 day 11-14 — PaldarkLab.Build.cs must depend on EnhancedInput so the
    UEnhancedInputComponent / UEnhancedInputLocalPlayerSubsystem APIs resolve
    on the game module side (character + player controller)."""
    build_cs = PROJECT_DIR / "Source" / "PaldarkLab" / "PaldarkLab.Build.cs"
    if not build_cs.is_file():
        return  # caught by module layout check
    text = build_cs.read_text(encoding="utf-8")
    if not re.search(r'"EnhancedInput"', text):
        fail(
            f"{build_cs.relative_to(REPO_ROOT)} does not list \"EnhancedInput\" in "
            "PublicDependencyModuleNames (required by APaldarkPlayerController + "
            "APaldarkCharacter Enhanced Input binding)."
        )


def check_input_config_shape() -> None:
    """W1 day 11-14 — UPaldarkInputConfig has the expected (Native + Ability)
    arrays + lookup helpers; PawnData exposes the new DefaultMappingContexts +
    InputConfig fields; and PaldarkLab.cpp registers the console command."""
    input_header = (
        PROJECT_DIR
        / "Source"
        / "PaldarkLab"
        / "Public"
        / "Experience"
        / "PaldarkInputConfig.h"
    )
    pawn_header = (
        PROJECT_DIR
        / "Source"
        / "PaldarkLab"
        / "Public"
        / "Experience"
        / "PaldarkPawnData.h"
    )
    module_cpp = PROJECT_DIR / "Source" / "PaldarkLab" / "PaldarkLab.cpp"

    if input_header.is_file():
        text = input_header.read_text(encoding="utf-8")
        # Both array fields + lookup helpers must be present so designers /
        # native code agree on the shape of the asset.
        required = (
            "NativeInputActions",
            "AbilityInputActions",
            "FindNativeInputActionForTag",
            "FindAbilityInputActionForTag",
        )
        for name in required:
            if name not in text:
                fail(
                    f"{input_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {name!r} (W1 day 11-14 InputConfig shape)."
                )
        if "FPaldarkInputAction" not in text:
            fail(
                f"{input_header.relative_to(REPO_ROOT)} is missing FPaldarkInputAction "
                "struct (tag-IA pair)."
            )

    if pawn_header.is_file():
        text = pawn_header.read_text(encoding="utf-8")
        if "DefaultMappingContexts" not in text:
            fail(
                f"{pawn_header.relative_to(REPO_ROOT)} must expose "
                "DefaultMappingContexts (W1 day 11-14)."
            )
        if "InputConfig" not in text:
            fail(
                f"{pawn_header.relative_to(REPO_ROOT)} must expose InputConfig "
                "TSoftObjectPtr<UPaldarkInputConfig> (W1 day 11-14)."
            )
        if "FPaldarkMappingContextAndPriority" not in text:
            fail(
                f"{pawn_header.relative_to(REPO_ROOT)} must declare "
                "FPaldarkMappingContextAndPriority struct (W1 day 11-14)."
            )

    if module_cpp.is_file():
        text = module_cpp.read_text(encoding="utf-8")
        if "Paldark.Input.ListBindings" not in text:
            fail(
                f"{module_cpp.relative_to(REPO_ROOT)} must register the "
                "`Paldark.Input.ListBindings` console command (W1 day 11-14)."
            )


def _check_component_classes(rel_headers: dict[str, str], week_label: str) -> None:
    """Shared shape check for player + Pal component classes (W3-4).

    Each header must:
      - exist under PaldarkLab/Source/PaldarkLab/Public/<rel_header>,
      - declare a `class PALDARKLAB_API <ClassName> : public UActorComponent`,
      - have a matching .cpp under .../Private/<same path>/<basename>.cpp.
    """
    public_root = PROJECT_DIR / "Source" / "PaldarkLab" / "Public"
    private_root = PROJECT_DIR / "Source" / "PaldarkLab" / "Private"

    for rel_header, class_name in rel_headers.items():
        header = public_root / rel_header
        if not header.is_file():
            fail(f"missing {header.relative_to(REPO_ROOT)} ({week_label} slot)")
            continue
        text = header.read_text(encoding="utf-8")
        if "UCLASS" not in text:
            fail(f"{header.relative_to(REPO_ROOT)} is missing a UCLASS macro ({week_label}).")
        decl_pattern = (
            rf"class\s+PALDARKLAB_API\s+{re.escape(class_name)}\s*:\s*public\s+UActorComponent"
        )
        if not re.search(decl_pattern, text):
            fail(
                f"{header.relative_to(REPO_ROOT)} does not declare "
                f"`class PALDARKLAB_API {class_name} : public UActorComponent` ({week_label})."
            )

        cpp = private_root / rel_header
        cpp = cpp.with_suffix(".cpp")
        if not cpp.is_file():
            fail(
                f"missing matching .cpp for {header.relative_to(REPO_ROOT)} "
                f"(expected {cpp.relative_to(REPO_ROOT)}, {week_label})."
            )


def check_player_slot_classes() -> None:
    """W3-4 — 12 player component slot classes exist + have matching .cpp."""
    _check_component_classes(EXPECTED_PLAYER_COMPONENT_CLASSES, "W3-4 player slot")
    if len(EXPECTED_PLAYER_COMPONENT_CLASSES) != 12:
        fail(
            "EXPECTED_PLAYER_COMPONENT_CLASSES should hold exactly 12 entries "
            f"(roadmap W3-4); got {len(EXPECTED_PLAYER_COMPONENT_CLASSES)}."
        )


def check_pal_slot_classes() -> None:
    """Pal component slot classes — W3-4 ships 8, W20-21 adds Patrol (9 total)."""
    _check_component_classes(EXPECTED_PAL_COMPONENT_CLASSES, "Pal slot")
    if len(EXPECTED_PAL_COMPONENT_CLASSES) != 9:
        fail(
            "EXPECTED_PAL_COMPONENT_CLASSES should hold exactly 9 entries "
            "(W3-4 8 slots + W20-21 PatrolSlot); "
            f"got {len(EXPECTED_PAL_COMPONENT_CLASSES)}."
        )


def check_player_character_slot_wiring() -> None:
    """W3-4 — PaldarkCharacter.cpp must CreateDefaultSubobject for each player slot."""
    cpp = (
        PROJECT_DIR
        / "Source"
        / "PaldarkLab"
        / "Private"
        / "Player"
        / "PaldarkCharacter.cpp"
    )
    if not cpp.is_file():
        return  # caught by check_framework_classes
    text = cpp.read_text(encoding="utf-8")
    for cls in EXPECTED_PLAYER_COMPONENT_CLASSES.values():
        pattern = rf"CreateDefaultSubobject<\s*{re.escape(cls)}\s*>"
        if not re.search(pattern, text):
            fail(
                f"{cpp.relative_to(REPO_ROOT)} must CreateDefaultSubobject<{cls}>(...) "
                "for the W3-4 player slot."
            )


def check_pal_character_slot_wiring() -> None:
    """W3-4 — PaldarkPalCharacter.cpp must CreateDefaultSubobject for each Pal slot."""
    cpp = (
        PROJECT_DIR
        / "Source"
        / "PaldarkLab"
        / "Private"
        / "Pal"
        / "PaldarkPalCharacter.cpp"
    )
    if not cpp.is_file():
        return  # caught by check_framework_classes
    text = cpp.read_text(encoding="utf-8")
    for cls in EXPECTED_PAL_COMPONENT_CLASSES.values():
        pattern = rf"CreateDefaultSubobject<\s*{re.escape(cls)}\s*>"
        if not re.search(pattern, text):
            fail(
                f"{cpp.relative_to(REPO_ROOT)} must CreateDefaultSubobject<{cls}>(...) "
                "for the W3-4 Pal slot."
            )


def check_pal_pawn_data_shape() -> None:
    """W3-4 — PaldarkPawnData exposes DefaultPalCompanions + FPaldarkPalCompanionSpec."""
    pawn_header = (
        PROJECT_DIR
        / "Source"
        / "PaldarkLab"
        / "Public"
        / "Experience"
        / "PaldarkPawnData.h"
    )
    module_cpp = PROJECT_DIR / "Source" / "PaldarkLab" / "PaldarkLab.cpp"

    if pawn_header.is_file():
        text = pawn_header.read_text(encoding="utf-8")
        if "FPaldarkPalCompanionSpec" not in text:
            fail(
                f"{pawn_header.relative_to(REPO_ROOT)} must declare "
                "FPaldarkPalCompanionSpec struct (W3-4)."
            )
        if "DefaultPalCompanions" not in text:
            fail(
                f"{pawn_header.relative_to(REPO_ROOT)} must expose "
                "DefaultPalCompanions field (W3-4)."
            )

    if module_cpp.is_file():
        text = module_cpp.read_text(encoding="utf-8")
        if "Paldark.Pal.SpawnTestCompanion" not in text:
            fail(
                f"{module_cpp.relative_to(REPO_ROOT)} must register the "
                "`Paldark.Pal.SpawnTestCompanion` console command (W3-4)."
            )


def check_activity_fsm_shape() -> None:
    """W5-6 — Activity FSM headers/cpps exist, declare UPaldarkBaseActivity +
    3 concrete activities; PalActivityComponent is the real driver (not just
    the W3-4 stub); locomotion exposes the bFollowEnabled gate; PaldarkLab.cpp
    registers the 3 new console commands.
    """
    public_root = PROJECT_DIR / "Source" / "PaldarkLab" / "Public"
    private_root = PROJECT_DIR / "Source" / "PaldarkLab" / "Private"

    # Base + 3 concrete activity classes (header + .cpp + UCLASS).
    for rel_header, class_name in EXPECTED_ACTIVITY_CLASSES.items():
        header = public_root / rel_header
        if not header.is_file():
            fail(f"missing {header.relative_to(REPO_ROOT)} (W5-6 activity).")
            continue
        text = header.read_text(encoding="utf-8")
        if "UCLASS" not in text:
            fail(f"{header.relative_to(REPO_ROOT)} is missing a UCLASS macro (W5-6 activity).")
        class_pattern = rf"class\s+PALDARKLAB_API\s+{re.escape(class_name)}\b"
        if not re.search(class_pattern, text):
            fail(
                f"{header.relative_to(REPO_ROOT)} does not declare "
                f"`class PALDARKLAB_API {class_name}` (W5-6 activity)."
            )
        cpp = (private_root / rel_header).with_suffix(".cpp")
        if not cpp.is_file():
            fail(
                f"missing matching .cpp for {header.relative_to(REPO_ROOT)} "
                f"(expected {cpp.relative_to(REPO_ROOT)}, W5-6 activity)."
            )

    # UPaldarkBaseActivity must be Abstract (it's the base) and expose the
    # Enter/Tick/Exit/CanRun lifecycle.
    base_header = public_root / "Pal/Activities/PaldarkBaseActivity.h"
    if base_header.is_file():
        text = base_header.read_text(encoding="utf-8")
        if not re.search(r"UCLASS\([^)]*Abstract", text):
            fail(
                f"{base_header.relative_to(REPO_ROOT)} must declare UCLASS(Abstract, ...) "
                "(W5-6: UPaldarkBaseActivity is the abstract root)."
            )
        for symbol in ("CanRun", "EnterActivity", "TickActivity", "ExitActivity", "ActivityTag", "Priority"):
            if symbol not in text:
                fail(
                    f"{base_header.relative_to(REPO_ROOT)} is missing {symbol!r} "
                    "(W5-6: BaseActivity lifecycle)."
                )

    # PalActivityComponent must be the real FSM driver: TickComponent override,
    # CandidateActivities array, SetCurrentActivityByTag, RequestInvestigate,
    # ClearInvestigateRequest, GetCurrentActivity, GetCurrentActivityTag.
    activity_header = public_root / "Pal/Components/PaldarkPalActivityComponent.h"
    if activity_header.is_file():
        text = activity_header.read_text(encoding="utf-8")
        for symbol in (
            "CandidateActivities",
            "SetCurrentActivityByTag",
            "SetCurrentActivityByClass",
            "RequestInvestigate",
            "ClearInvestigateRequest",
            "HasActiveInvestigateRequest",
            "GetInvestigateTarget",
            "GetCurrentActivity",
            "GetCurrentActivityTag",
            "SelectionInterval",
            "TickComponent",
            "BeginPlay",
        ):
            if symbol not in text:
                fail(
                    f"{activity_header.relative_to(REPO_ROOT)} is missing required FSM "
                    f"symbol {symbol!r} (W5-6: real driver, not W3-4 stub)."
                )

    activity_cpp = private_root / "Pal/Components/PaldarkPalActivityComponent.cpp"
    if activity_cpp.is_file():
        text = activity_cpp.read_text(encoding="utf-8")
        # The constructor must seed CandidateActivities with the 3 default
        # activity classes so the FSM has something to switch between.
        for symbol in (
            "UPaldarkActivity_Idle::StaticClass",
            "UPaldarkActivity_Follow::StaticClass",
            "UPaldarkActivity_Investigate::StaticClass",
            "InstantiateActivities",
            "EvaluateAndSwitch",
            "SwitchToActivity",
        ):
            if symbol not in text:
                fail(
                    f"{activity_cpp.relative_to(REPO_ROOT)} is missing required FSM "
                    f"symbol {symbol!r} (W5-6: real driver, not W3-4 stub)."
                )

    # Locomotion must expose the Activity FSM gate so Idle/Investigate can
    # pause the leash without losing the FollowedPawn cache.
    loco_header = public_root / "Pal/Components/PaldarkPalLocomotionComponent.h"
    if loco_header.is_file():
        text = loco_header.read_text(encoding="utf-8")
        for symbol in ("SetFollowEnabled", "IsFollowEnabled", "GetPlanarDistanceToFollowedPawn"):
            if symbol not in text:
                fail(
                    f"{loco_header.relative_to(REPO_ROOT)} is missing {symbol!r} "
                    "(W5-6: Activity FSM gate / distance accessor)."
                )

    # PaldarkLab.cpp must register the 3 new console commands.
    module_cpp = PROJECT_DIR / "Source" / "PaldarkLab" / "PaldarkLab.cpp"
    if module_cpp.is_file():
        text = module_cpp.read_text(encoding="utf-8")
        for cmd in (
            "Paldark.Pal.CurrentActivity",
            "Paldark.Pal.SetActivity",
            "Paldark.Pal.Ping",
        ):
            if cmd not in text:
                fail(
                    f"{module_cpp.relative_to(REPO_ROOT)} must register the "
                    f"`{cmd}` console command (W5-6)."
                )


def check_gas_dependency() -> None:
    """W7-8 — PaldarkLab.Build.cs must depend on GameplayAbilities + GameplayTags
    + GameplayTasks so the AttributeSet / ASC / GameplayAbility types resolve."""
    build_cs = PROJECT_DIR / "Source" / "PaldarkLab" / "PaldarkLab.Build.cs"
    if not build_cs.is_file():
        return  # caught by module layout check
    text = build_cs.read_text(encoding="utf-8")
    for dep in ("GameplayAbilities", "GameplayTags", "GameplayTasks"):
        if not re.search(rf'"{dep}"', text):
            fail(
                f"{build_cs.relative_to(REPO_ROOT)} does not list \"{dep}\" in "
                "PublicDependencyModuleNames (required for W7-8 GAS classes)."
            )


def check_gas_shape() -> None:
    """W7-8 — GAS skeleton: AttributeSet + ASC + GameplayAbility base + GA_Sprint.

    Each header must declare `class PALDARKLAB_API <ClassName>` and have a
    matching .cpp. AttributeSet must hold the 5 expected attributes; ASC must
    expose the tag-keyed activation helpers; GA_Sprint must override
    ActivateAbility / EndAbility. PawnData must expose the new GAS arrays
    (GrantedAbilities + StartupEffects); PaldarkPlayerState must own a
    UPaldarkAbilitySystemComponent + UPaldarkAttributeSet via UPROPERTY and
    implement IAbilitySystemInterface; PaldarkPalCharacter must own its own
    self-hosted ASC + AttributeSet; PaldarkCharacter must bind ability inputs
    and expose Input_Sprint_Pressed / Input_Sprint_Released; the player
    controller must run a server-side ability grant on possess; module CPP
    must register the two new debug commands.
    """
    public_root = PROJECT_DIR / "Source" / "PaldarkLab" / "Public"
    private_root = PROJECT_DIR / "Source" / "PaldarkLab" / "Private"

    for rel_header, class_name in EXPECTED_GAS_CLASSES.items():
        header = public_root / rel_header
        if not header.is_file():
            fail(f"missing {header.relative_to(REPO_ROOT)} (W7-8 GAS).")
            continue
        text = header.read_text(encoding="utf-8")
        if "UCLASS" not in text:
            fail(f"{header.relative_to(REPO_ROOT)} is missing a UCLASS macro (W7-8 GAS).")
        class_pattern = rf"class\s+PALDARKLAB_API\s+{re.escape(class_name)}\b"
        if not re.search(class_pattern, text):
            fail(
                f"{header.relative_to(REPO_ROOT)} does not declare "
                f"`class PALDARKLAB_API {class_name}` (W7-8 GAS)."
            )
        cpp = (private_root / rel_header).with_suffix(".cpp")
        if not cpp.is_file():
            fail(
                f"missing matching .cpp for {header.relative_to(REPO_ROOT)} "
                f"(expected {cpp.relative_to(REPO_ROOT)}, W7-8 GAS)."
            )

    # AttributeSet must declare the 5 W7-8 attributes + the standard clamp
    # hooks (PreAttributeChange / PostGameplayEffectExecute). W9-10 adds
    # Armor, IncomingDamage meta-attribute, OnRep_Armor, and the
    # OnHealthZeroed multicast (broadcast on authority when Health → 0).
    attr_header = public_root / "Gas/PaldarkAttributeSet.h"
    if attr_header.is_file():
        text = attr_header.read_text(encoding="utf-8")
        for symbol in (
            "Health",
            "MaxHealth",
            "Stamina",
            "MaxStamina",
            "MoveSpeed",
            "PreAttributeChange",
            "PostGameplayEffectExecute",
            "OnRep_Health",
            "OnRep_Stamina",
            "OnRep_MoveSpeed",
            "ATTRIBUTE_ACCESSORS",
            # W9-10 — defensive + meta-damage attributes + death broadcast.
            "Armor",
            "OnRep_Armor",
            "IncomingDamage",
            "FOnPaldarkHealthZeroed",
            "OnHealthZeroed",
        ):
            if symbol not in text:
                fail(
                    f"{attr_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W7-8 + W9-10 AttributeSet shape)."
                )

    # ASC subclass must expose the tag-keyed activation/cancel helpers.
    asc_header = public_root / "Gas/PaldarkAbilitySystemComponent.h"
    if asc_header.is_file():
        text = asc_header.read_text(encoding="utf-8")
        for symbol in (
            "TryActivateAbilityByActivationTag",
            "CancelAbilityByActivationTag",
        ):
            if symbol not in text:
                fail(
                    f"{asc_header.relative_to(REPO_ROOT)} is missing required "
                    f"helper {symbol!r} (W7-8 ASC shape)."
                )

    # Base GameplayAbility must expose ActivationInputTag for tag-keyed lookup.
    ga_header = public_root / "Gas/PaldarkGameplayAbility.h"
    if ga_header.is_file():
        text = ga_header.read_text(encoding="utf-8")
        if "ActivationInputTag" not in text:
            fail(
                f"{ga_header.relative_to(REPO_ROOT)} is missing ActivationInputTag "
                "(W7-8 base ability shape)."
            )

    # GA_Sprint must override Activate/EndAbility and reference the cost +
    # move speed GE properties.
    sprint_header = public_root / "Gas/Abilities/PaldarkGameplayAbility_Sprint.h"
    if sprint_header.is_file():
        text = sprint_header.read_text(encoding="utf-8")
        for symbol in (
            "ActivateAbility",
            "EndAbility",
            "SprintCostEffect",
            "SprintMoveSpeedEffect",
            "SprintingStateTag",
        ):
            if symbol not in text:
                fail(
                    f"{sprint_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W7-8 GA_Sprint shape)."
                )

    # PawnData exposes the GAS arrays so the player controller can grant
    # abilities + apply startup effects on possess.
    pawn_header = public_root / "Experience/PaldarkPawnData.h"
    if pawn_header.is_file():
        text = pawn_header.read_text(encoding="utf-8")
        for symbol in ("GrantedAbilities", "StartupEffects"):
            if symbol not in text:
                fail(
                    f"{pawn_header.relative_to(REPO_ROOT)} must expose "
                    f"{symbol!r} (W7-8 PawnData GAS shape)."
                )

    # PlayerState owns the ASC + AttributeSet + implements IAbilitySystemInterface.
    ps_header = public_root / "Player/PaldarkPlayerState.h"
    if ps_header.is_file():
        text = ps_header.read_text(encoding="utf-8")
        for symbol in (
            "IAbilitySystemInterface",
            "UPaldarkAbilitySystemComponent",
            "UPaldarkAttributeSet",
            "GetAbilitySystemComponent",
            "GetPaldarkAbilitySystemComponent",
        ):
            if symbol not in text:
                fail(
                    f"{ps_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W7-8 PlayerState GAS wiring)."
                )

    # Pal owns its own ASC (no PlayerState) + implements IAbilitySystemInterface.
    pal_header = public_root / "Pal/PaldarkPalCharacter.h"
    if pal_header.is_file():
        text = pal_header.read_text(encoding="utf-8")
        for symbol in (
            "IAbilitySystemInterface",
            "UPaldarkAbilitySystemComponent",
            "UPaldarkAttributeSet",
            "GetAbilitySystemComponent",
        ):
            if symbol not in text:
                fail(
                    f"{pal_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W7-8 Pal GAS wiring)."
                )

    # Player character binds Sprint input. W9-10 adds Fire input handler
    # (Input_Fire_Pressed → TryActivateAbilityByActivationTag).
    char_header = public_root / "Player/PaldarkCharacter.h"
    if char_header.is_file():
        text = char_header.read_text(encoding="utf-8")
        for symbol in (
            "InitAbilitySystem",
            "BindAbilityInputActions",
            "Input_Sprint_Pressed",
            "Input_Sprint_Released",
            # W9-10 — Fire input handler (semi-auto, no Released handler).
            "Input_Fire_Pressed",
        ):
            if symbol not in text:
                fail(
                    f"{char_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W7-8 + W9-10 ability input binding)."
                )

    # PlayerController grants abilities on the server.
    pc_header = public_root / "Player/PaldarkPlayerController.h"
    if pc_header.is_file():
        text = pc_header.read_text(encoding="utf-8")
        if "GrantGasFromPawnData" not in text:
            fail(
                f"{pc_header.relative_to(REPO_ROOT)} is missing GrantGasFromPawnData "
                "(W7-8 server-side ability grant)."
            )

    # Module CPP registers the two W7-8 GAS debug commands + the two W9-10
    # combat / damage-chain debug commands + the four W11-12 inventory debug
    # commands. All four W11-12 commands operate on the local player's
    # UPaldarkPlayerInventoryComponent.
    module_cpp = PROJECT_DIR / "Source" / "PaldarkLab" / "PaldarkLab.cpp"
    if module_cpp.is_file():
        text = module_cpp.read_text(encoding="utf-8")
        for cmd in (
            "Paldark.Gas.DumpAttributes",
            "Paldark.Gas.Damage",
            # W9-10 — Combat / damage chain debug helpers.
            "Paldark.Combat.SpawnDummy",
            "Paldark.Combat.Fire",
            # W11-12 — Inventory fragment debug helpers.
            "Paldark.Inventory.List",
            "Paldark.Inventory.Add",
            "Paldark.Inventory.Remove",
            "Paldark.Inventory.Drop",
        ):
            if cmd not in text:
                fail(
                    f"{module_cpp.relative_to(REPO_ROOT)} must register the "
                    f"`{cmd}` console command (W7-8 / W9-10 / W11-12)."
                )


def check_combat_damage_shape() -> None:
    """W9-10 — Damage chain: DamageExecutionCalculation + GA_HitscanFire +
    DummyTarget.

    Validates the new combat surface: an UDamageExecutionCalculation that
    captures Armor / Health + SetByCaller magnitudes (BaseDamage,
    HeadshotMultiplier), the Fire hitscan ability that line-traces from the
    camera and applies the damage GE, and the APaldarkDummyTarget actor that
    owns its own ASC + AttributeSet, applies an init GE on BeginPlay, and
    binds OnHealthZeroed to destroy itself on death.

    Each header must declare `class PALDARKLAB_API <ClassName>` and have a
    matching .cpp under Private/.
    """
    public_root = PROJECT_DIR / "Source" / "PaldarkLab" / "Public"
    private_root = PROJECT_DIR / "Source" / "PaldarkLab" / "Private"

    for rel_header, class_name in EXPECTED_COMBAT_CLASSES.items():
        header = public_root / rel_header
        if not header.is_file():
            fail(f"missing {header.relative_to(REPO_ROOT)} (W9-10 combat).")
            continue
        text = header.read_text(encoding="utf-8")
        if "UCLASS" not in text:
            fail(f"{header.relative_to(REPO_ROOT)} is missing a UCLASS macro (W9-10 combat).")
        class_pattern = rf"class\s+PALDARKLAB_API\s+{re.escape(class_name)}\b"
        if not re.search(class_pattern, text):
            fail(
                f"{header.relative_to(REPO_ROOT)} does not declare "
                f"`class PALDARKLAB_API {class_name}` (W9-10 combat)."
            )
        cpp = (private_root / rel_header).with_suffix(".cpp")
        if not cpp.is_file():
            fail(
                f"missing matching .cpp for {header.relative_to(REPO_ROOT)} "
                f"(expected {cpp.relative_to(REPO_ROOT)}, W9-10 combat)."
            )

    # DamageExecutionCalculation captures Target.Armor for armor mitigation
    # and reads the BaseDamage / HeadshotMultiplier SetByCaller magnitudes
    # before writing the final value into the AttributeSet's IncomingDamage
    # meta-attribute slot. Health is intentionally NOT captured here — the
    # AttributeSet's PostGameplayEffectExecute drains IncomingDamage onto
    # Health, keeping the execution calc focused on damage math.
    dec_header = public_root / "Gas/PaldarkDamageExecutionCalculation.h"
    if dec_header.is_file():
        text = dec_header.read_text(encoding="utf-8")
        for symbol in (
            "UGameplayEffectExecutionCalculation",
            "Execute_Implementation",
        ):
            if symbol not in text:
                fail(
                    f"{dec_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W9-10 execution calc shape)."
                )

    dec_cpp = private_root / "Gas/PaldarkDamageExecutionCalculation.cpp"
    if dec_cpp.is_file():
        text = dec_cpp.read_text(encoding="utf-8")
        for symbol in (
            # Aura-style capture macros — ArmorDef / Target capture / etc.
            "DECLARE_ATTRIBUTE_CAPTUREDEF",
            "DEFINE_ATTRIBUTE_CAPTUREDEF",
            "Armor",
            "GetSetByCallerMagnitude",
            "GetIncomingDamageAttribute",
        ):
            if symbol not in text:
                fail(
                    f"{dec_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W9-10 execution calc body)."
                )

    # GA_HitscanFire is the Fire ability — line trace from camera + apply
    # GE_Damage_Standard with SetByCaller BaseDamage magnitude. Must expose
    # the DamageEffect class slot + a configurable trace distance (`FireRange`)
    # so designers don't have to recompile to tune range.
    fire_header = public_root / "Gas/Abilities/PaldarkGameplayAbility_HitscanFire.h"
    if fire_header.is_file():
        text = fire_header.read_text(encoding="utf-8")
        for symbol in (
            "ActivateAbility",
            "DamageEffectClass",
            "FireRange",
            "BaseDamage",
            "HeadshotMultiplier",
        ):
            if symbol not in text:
                fail(
                    f"{fire_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W9-10 hitscan ability shape)."
                )

    fire_cpp = private_root / "Gas/Abilities/PaldarkGameplayAbility_HitscanFire.cpp"
    if fire_cpp.is_file():
        text = fire_cpp.read_text(encoding="utf-8")
        for symbol in (
            "LineTraceSingleByChannel",
            "MakeOutgoingSpec",
            "SetSetByCallerMagnitude",
            "ApplyGameplayEffectSpecToTarget",
        ):
            if symbol not in text:
                fail(
                    f"{fire_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W9-10 hitscan ability body)."
                )

    # DummyTarget owns ASC + AttributeSet, implements IAbilitySystemInterface,
    # and handles the OnHealthZeroed broadcast.
    dummy_header = public_root / "Combat/PaldarkDummyTarget.h"
    if dummy_header.is_file():
        text = dummy_header.read_text(encoding="utf-8")
        for symbol in (
            "IAbilitySystemInterface",
            "UPaldarkAbilitySystemComponent",
            "UPaldarkAttributeSet",
            "GetAbilitySystemComponent",
            "InitAttributesEffect",
            "HandleHealthZeroed",
        ):
            if symbol not in text:
                fail(
                    f"{dummy_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W9-10 DummyTarget shape)."
                )


# W11-12 — Inventory fragment + ItemDefinition + InventoryComponent.
#
# Headers under Public/Inventory/ ; matching .cpp under Private/Inventory/.
# The fragment base + 3 concrete fragments together with the
# UPaldarkItemDefinition primary data asset replace the W3-4 stub inventory
# component.
EXPECTED_INVENTORY_CLASSES = {
    "Inventory/PaldarkItemFragment.h":                       "UPaldarkItemFragment",
    "Inventory/Fragments/PaldarkItemFragment_Stackable.h":   "UPaldarkItemFragment_Stackable",
    "Inventory/Fragments/PaldarkItemFragment_Equipable.h":   "UPaldarkItemFragment_Equipable",
    "Inventory/Fragments/PaldarkItemFragment_Weight.h":      "UPaldarkItemFragment_Weight",
    "Inventory/PaldarkItemDefinition.h":                     "UPaldarkItemDefinition",
}


def check_inventory_shape() -> None:
    """W11-12 — Inventory fragment system.

    Validates the per-item composition pattern (Item Fragment base + 3
    concrete fragments), the UPaldarkItemDefinition primary data asset that
    composes fragments, the rewritten UPaldarkPlayerInventoryComponent
    (replicated FPaldarkInventoryEntry array + weight cap + authority API),
    and the DefaultGame.ini registration of the PaldarkItem primary asset
    type.
    """
    public_root = PROJECT_DIR / "Source" / "PaldarkLab" / "Public"
    private_root = PROJECT_DIR / "Source" / "PaldarkLab" / "Private"

    # Fragment base + concrete fragments + ItemDefinition shape.
    for rel_header, class_name in EXPECTED_INVENTORY_CLASSES.items():
        header = public_root / rel_header
        if not header.is_file():
            fail(f"missing {header.relative_to(REPO_ROOT)} (W11-12 inventory).")
            continue
        text = header.read_text(encoding="utf-8")
        if "UCLASS" not in text:
            fail(f"{header.relative_to(REPO_ROOT)} is missing a UCLASS macro (W11-12 inventory).")
        class_pattern = rf"class\s+PALDARKLAB_API\s+{re.escape(class_name)}\b"
        if not re.search(class_pattern, text):
            fail(
                f"{header.relative_to(REPO_ROOT)} does not declare "
                f"`class PALDARKLAB_API {class_name}` (W11-12 inventory)."
            )
        cpp = (private_root / rel_header).with_suffix(".cpp")
        if not cpp.is_file():
            fail(
                f"missing matching .cpp for {header.relative_to(REPO_ROOT)} "
                f"(expected {cpp.relative_to(REPO_ROOT)}, W11-12 inventory)."
            )

    # Fragment base must be EditInlineNew + DefaultToInstanced so designers can
    # compose fragments inside an ItemDefinition asset.
    frag_header = public_root / "Inventory/PaldarkItemFragment.h"
    if frag_header.is_file():
        text = frag_header.read_text(encoding="utf-8")
        for symbol in (
            "EditInlineNew",
            "DefaultToInstanced",
            "Abstract",
            "GetDebugDescription",
        ):
            if symbol not in text:
                fail(
                    f"{frag_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W11-12 fragment base shape)."
                )

    # Stackable fragment declares MaxStackSize.
    stack_header = public_root / "Inventory/Fragments/PaldarkItemFragment_Stackable.h"
    if stack_header.is_file():
        text = stack_header.read_text(encoding="utf-8")
        if "MaxStackSize" not in text:
            fail(
                f"{stack_header.relative_to(REPO_ROOT)} must expose MaxStackSize "
                "(W11-12 Stackable fragment)."
            )

    # Equipable fragment declares EquipSlot + AbilityToGrantOnEquip.
    equip_header = public_root / "Inventory/Fragments/PaldarkItemFragment_Equipable.h"
    if equip_header.is_file():
        text = equip_header.read_text(encoding="utf-8")
        for symbol in ("EquipSlot", "AbilityToGrantOnEquip"):
            if symbol not in text:
                fail(
                    f"{equip_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W11-12 Equipable fragment)."
                )

    # Weight fragment declares WeightKgPerUnit.
    weight_header = public_root / "Inventory/Fragments/PaldarkItemFragment_Weight.h"
    if weight_header.is_file():
        text = weight_header.read_text(encoding="utf-8")
        if "WeightKgPerUnit" not in text:
            fail(
                f"{weight_header.relative_to(REPO_ROOT)} must expose "
                "WeightKgPerUnit (W11-12 Weight fragment)."
            )

    # ItemDefinition is UPrimaryDataAsset with an Instanced fragment array
    # + FindFragmentByClass<T> helper + ItemTag identity key.
    def_header = public_root / "Inventory/PaldarkItemDefinition.h"
    if def_header.is_file():
        text = def_header.read_text(encoding="utf-8")
        for symbol in (
            "UPrimaryDataAsset",
            "Instanced",
            "Fragments",
            "ItemTag",
            "FindFragmentByClass",
            "GetPrimaryAssetId",
        ):
            if symbol not in text:
                fail(
                    f"{def_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W11-12 ItemDefinition shape)."
                )

    # Inventory component is the replicated authority API + lookup helpers.
    inv_header = public_root / "Player/Components/PaldarkPlayerInventoryComponent.h"
    if inv_header.is_file():
        text = inv_header.read_text(encoding="utf-8")
        for symbol in (
            "FPaldarkInventoryEntry",
            # W37-38 renamed the backing field from MaxWeightKg to
            # BaseMaxWeightKg (the effective cap = base + backpack bonus
            # is exposed via GetMaxWeightKg). Both names appear in the
            # header now — the rename is checked here, the new
            # backpack-aware accessor is checked in
            # check_w37_inventory_full_shape.
            "BaseMaxWeightKg",
            "GetMaxWeightKg",
            "Entries",
            "AddItem",
            "RemoveItemByTag",
            "DropAllItems",
            "GetCurrentWeightKg",
            "IsOverEncumbered",
            "FindEntryIndexByTag",
            "FindFirstItemDefByTag",
            "DumpToLog",
            "OnRep_Entries",
            "OnInventoryChanged",
        ):
            if symbol not in text:
                fail(
                    f"{inv_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W11-12 inventory component shape)."
                )

    inv_cpp = private_root / "Player/Components/PaldarkPlayerInventoryComponent.cpp"
    if inv_cpp.is_file():
        text = inv_cpp.read_text(encoding="utf-8")
        for symbol in (
            "GetLifetimeReplicatedProps",
            # W37-38 — renamed to BaseMaxWeightKg; the W37 check enforces
            # the exact macro shape, so just confirm SOME max-weight
            # replication line is present.
            "DOREPLIFETIME(UPaldarkPlayerInventoryComponent, BaseMaxWeightKg)",
            "DOREPLIFETIME(UPaldarkPlayerInventoryComponent, Entries)",
            "SetIsReplicatedByDefault(true)",
            "HasAuthority",
            "LoadSynchronous",
        ):
            if symbol not in text:
                fail(
                    f"{inv_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W11-12 inventory component body)."
                )

    # PaldarkItem primary asset type registered in DefaultGame.ini.
    config = PROJECT_DIR / "Config" / "DefaultGame.ini"
    if config.is_file():
        text = config.read_text(encoding="utf-8")
        if re.search(r'PrimaryAssetType\s*=\s*"PaldarkItem"', text) is None:
            fail(
                f"{config.relative_to(REPO_ROOT)} must register the PaldarkItem "
                "primary asset type (W11-12)."
            )


def check_net_subsystem_shape() -> None:
    """W14-15 — Net subsystem + GameMode net hooks + Pal locomotion replication.

    Validates the UGameInstanceSubsystem entry points (HostListenServer /
    JoinServer / DisconnectFromServer / GetNetSnapshot / RefreshTopology),
    the 4 console command names (Paldark.Net.{Host,Join,Disconnect,Status}),
    the GameMode PreLogin/PostLogin/Logout hooks + MaxPlayers cap, and the
    Pal locomotion component replication wiring (FollowedPawn +
    bFollowEnabled via GetLifetimeReplicatedProps).
    """
    public_root = PROJECT_DIR / "Source" / "PaldarkLab" / "Public"
    private_root = PROJECT_DIR / "Source" / "PaldarkLab" / "Private"

    # Net subsystem header.
    net_header = public_root / "Net/PaldarkNetSubsystem.h"
    if not net_header.is_file():
        fail(
            f"missing {net_header.relative_to(REPO_ROOT)} "
            "(W14-15 — Paldark net subsystem entry points)."
        )
    else:
        text = net_header.read_text(encoding="utf-8")
        if "UCLASS" not in text:
            fail(
                f"{net_header.relative_to(REPO_ROOT)} is missing a UCLASS "
                "macro (W14-15 net subsystem)."
            )
        class_pattern = r"class\s+PALDARKLAB_API\s+UPaldarkNetSubsystem\b"
        if not re.search(class_pattern, text):
            fail(
                f"{net_header.relative_to(REPO_ROOT)} does not declare "
                "`class PALDARKLAB_API UPaldarkNetSubsystem` (W14-15 net subsystem)."
            )
        if "UGameInstanceSubsystem" not in text:
            fail(
                f"{net_header.relative_to(REPO_ROOT)} must inherit from "
                "UGameInstanceSubsystem (W14-15 — subsystem must survive map travels)."
            )
        for symbol in (
            "FPaldarkNetSnapshot",
            "HostListenServer",
            "JoinServer",
            "DisconnectFromServer",
            "GetNetSnapshot",
            "RefreshTopology",
        ):
            if symbol not in text:
                fail(
                    f"{net_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W14-15 net subsystem shape)."
                )

        # W39 — L-20 closure. BlueprintAssignable delegates for login/logout so
        # UMG widgets (W40-41 hub town lobby, marketplace player list) can react
        # without subclassing the GameMode. Validator enforces the full shape:
        # 2 DYNAMIC_MULTICAST_DELEGATE_OneParam declarations + 2 broadcast
        # method signatures + 2 BlueprintAssignable UPROPERTY fields.
        l20_required_header_symbols = (
            "DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPaldarkPlayerLoginSignature",
            "DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPaldarkPlayerLogoutSignature",
            "BroadcastPlayerLogin",
            "BroadcastPlayerLogout",
            "BlueprintAssignable",
            "OnPlayerLogin",
            "OnPlayerLogout",
            "FPaldarkPlayerLoginSignature OnPlayerLogin",
            "FPaldarkPlayerLogoutSignature OnPlayerLogout",
        )
        for symbol in l20_required_header_symbols:
            if symbol not in text:
                fail(
                    f"{net_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W39 — L-20 closure: BlueprintAssignable "
                    f"login/logout delegates on PaldarkNetSubsystem)."
                )

    # Net subsystem body — console commands + travel calls + failure relays.
    net_cpp = private_root / "Net/PaldarkNetSubsystem.cpp"
    if not net_cpp.is_file():
        fail(
            f"missing {net_cpp.relative_to(REPO_ROOT)} "
            "(W14-15 net subsystem body)."
        )
    else:
        text = net_cpp.read_text(encoding="utf-8")
        for symbol in (
            "Paldark.Net.Host",
            "Paldark.Net.Join",
            "Paldark.Net.Disconnect",
            "Paldark.Net.Status",
            "ServerTravel",
            "ClientTravel",
            "OnPostWorldInitialization",
            "RefreshTopology",
        ):
            if symbol not in text:
                fail(
                    f"{net_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W14-15 net subsystem body)."
                )

        # W39 — L-20 closure. Body must actually implement Broadcast* + call
        # `OnPlayerLogin.Broadcast(...)` / `OnPlayerLogout.Broadcast(...)` —
        # without the dot-Broadcast call the delegate never fires even if
        # listeners are bound. Re-check `NM_Client` guard so client-side
        # callers don't accidentally fire delegates that wouldn't reach
        # anyone (authority-only broadcast contract).
        l20_required_cpp_symbols = (
            "UPaldarkNetSubsystem::BroadcastPlayerLogin",
            "UPaldarkNetSubsystem::BroadcastPlayerLogout",
            "OnPlayerLogin.Broadcast",
            "OnPlayerLogout.Broadcast",
            "NM_Client",
        )
        for symbol in l20_required_cpp_symbols:
            if symbol not in text:
                fail(
                    f"{net_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W39 — L-20 closure: Broadcast* impl "
                    f"+ delegate fire + authority guard)."
                )

    # GameMode net hooks — PreLogin/PostLogin/Logout + MaxPlayers cap.
    gm_header = public_root / "Framework/PaldarkGameModeBase.h"
    if gm_header.is_file():
        text = gm_header.read_text(encoding="utf-8")
        for symbol in ("PreLogin", "PostLogin", "Logout", "GetMaxPlayers"):
            if symbol not in text:
                fail(
                    f"{gm_header.relative_to(REPO_ROOT)} is missing required "
                    f"net override {symbol!r} (W14-15 GameMode net hooks)."
                )

    gm_cpp = private_root / "Framework/PaldarkGameModeBase.cpp"
    if gm_cpp.is_file():
        text = gm_cpp.read_text(encoding="utf-8")
        for symbol in (
            "PreLogin",
            "PostLogin",
            "Logout",
            "ConnectedPlayerCount",
            "MaxPlayersPerSession",
            "LogPaldarkNet",
        ):
            if symbol not in text:
                fail(
                    f"{gm_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"net hook {symbol!r} (W14-15 GameMode body)."
                )

        # W39 — L-20 closure. The GameMode authority hook MUST resolve the
        # subsystem + call Broadcast* on every PostLogin / Logout so widgets
        # don't have to subclass GameMode to listen. The include + the call
        # are both verified — a typo'd include passes the call check but
        # crashes at compile, and a call without an include is equally
        # broken (header-only forward declarations don't exist for the
        # subsystem getter macro).
        for symbol in (
            'include "Net/PaldarkNetSubsystem.h"',
            "GetSubsystem<UPaldarkNetSubsystem>",
            "BroadcastPlayerLogin(NewPlayer)",
            "BroadcastPlayerLogout(Exiting)",
        ):
            if symbol not in text:
                fail(
                    f"{gm_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W39 — L-20 closure: GameMode hooks "
                    f"must fire OnPlayerLogin / OnPlayerLogout via the net "
                    f"subsystem)."
                )

    # Pal locomotion replication — FollowedPawn + bFollowEnabled.
    loco_header = public_root / "Pal/Components/PaldarkPalLocomotionComponent.h"
    if loco_header.is_file():
        text = loco_header.read_text(encoding="utf-8")
        for symbol in ("Replicated", "FollowedPawn", "bFollowEnabled"):
            if symbol not in text:
                fail(
                    f"{loco_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W14-15 Pal locomotion replication)."
                )

    loco_cpp = private_root / "Pal/Components/PaldarkPalLocomotionComponent.cpp"
    if loco_cpp.is_file():
        text = loco_cpp.read_text(encoding="utf-8")
        for symbol in (
            "GetLifetimeReplicatedProps",
            "DOREPLIFETIME(UPaldarkPalLocomotionComponent, FollowedPawn)",
            "DOREPLIFETIME(UPaldarkPalLocomotionComponent, bFollowEnabled)",
            "SetIsReplicatedByDefault(true)",
        ):
            if symbol not in text:
                fail(
                    f"{loco_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W14-15 Pal locomotion replication body)."
                )

    # Pal character NetUpdateFrequency tuning (cm^2 cull + slower update rate
    # so 4 Pals on dedicated server don't saturate bandwidth).
    pal_cpp = private_root / "Pal/PaldarkPalCharacter.cpp"
    if pal_cpp.is_file():
        text = pal_cpp.read_text(encoding="utf-8")
        for symbol in ("NetUpdateFrequency", "MinNetUpdateFrequency", "NetCullDistanceSquared"):
            if symbol not in text:
                fail(
                    f"{pal_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"net-tuning symbol {symbol!r} (W14-15 Pal character)."
                )


def check_lag_compensation_shape() -> None:
    """W16-17 — Server-side rewind component + per-bone hitboxes + time sync RPCs.

    Validates the lag compensation data model (FPaldarkBoxInformation +
    FPaldarkFramePackage + FPaldarkServerSideRewindResult), the lag comp
    component shape (FrameHistory + ServerSideRewind + ServerScoreRequest +
    Cache/Move/Reset), the per-bone hitbox map on APaldarkCharacter, the
    time-sync RPC pair on APaldarkPlayerController, and the three
    Paldark.LagComp.* console commands wired in PaldarkPlayerController.cpp.
    """
    public_root = PROJECT_DIR / "Source" / "PaldarkLab" / "Public"
    private_root = PROJECT_DIR / "Source" / "PaldarkLab" / "Private"

    # 1. Data model header — three USTRUCT types.
    types_header = public_root / "Combat/PaldarkLagCompensationTypes.h"
    if not types_header.is_file():
        fail(
            f"missing {types_header.relative_to(REPO_ROOT)} "
            "(W16-17 — lag compensation data structs)."
        )
    else:
        text = types_header.read_text(encoding="utf-8")
        for symbol in (
            "FPaldarkBoxInformation",
            "FPaldarkFramePackage",
            "FPaldarkServerSideRewindResult",
            "USTRUCT",
            "BoxExtent",
            "HitBoxes",
            "bHitConfirmed",
            "bHeadShot",
            "ResultTag",
        ):
            if symbol not in text:
                fail(
                    f"{types_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W16-17 — lag compensation types)."
                )

    # 2. Lag compensation component header + body shape.
    lc_header = public_root / "Combat/PaldarkLagCompensationComponent.h"
    if not lc_header.is_file():
        fail(
            f"missing {lc_header.relative_to(REPO_ROOT)} "
            "(W16-17 — lag compensation component header)."
        )
    else:
        text = lc_header.read_text(encoding="utf-8")
        if not re.search(
            r"class\s+PALDARKLAB_API\s+UPaldarkLagCompensationComponent\b",
            text,
        ):
            fail(
                f"{lc_header.relative_to(REPO_ROOT)} does not declare "
                "`class PALDARKLAB_API UPaldarkLagCompensationComponent` "
                "(W16-17 lag compensation component shape)."
            )
        for symbol in (
            "UActorComponent",
            "ServerSideRewind",
            "ServerScoreRequest_Hitscan",
            "TDoubleLinkedList",
            "FPaldarkFramePackage",
            "MaxRecordTime",
            "HeadBoneName",
            "GetHistoryCount",
            "DumpHistoryToLog",
        ):
            if symbol not in text:
                fail(
                    f"{lc_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W16-17 lag comp component shape)."
                )

    lc_cpp = private_root / "Combat/PaldarkLagCompensationComponent.cpp"
    if not lc_cpp.is_file():
        fail(
            f"missing {lc_cpp.relative_to(REPO_ROOT)} "
            "(W16-17 lag compensation component body)."
        )
    else:
        text = lc_cpp.read_text(encoding="utf-8")
        for symbol in (
            "SaveFramePackage",
            "CacheBoxPositions",
            "MoveBoxes",
            "ResetBoxes",
            "EnableCharacterMeshCollision",
            "InterpBetweenFrames",
            "ConfirmHit",
            "ServerScoreRequest_Hitscan_Implementation",
            "TAG_Paldark_LagComp_HitConfirmed",
            "TAG_Paldark_LagComp_HitRejected",
            "LogPaldarkNet",
        ):
            if symbol not in text:
                fail(
                    f"{lc_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W16-17 lag comp body)."
                )

    # 3. APaldarkCharacter — LagCompSlot + HitCollisionBoxes + 5+ hitbox specs.
    char_header = public_root / "Player/PaldarkCharacter.h"
    if char_header.is_file():
        text = char_header.read_text(encoding="utf-8")
        for symbol in (
            "LagCompSlot",
            "HitCollisionBoxes",
            "GetHitCollisionBoxes",
            "UPaldarkLagCompensationComponent",
            "UBoxComponent",
        ):
            if symbol not in text:
                fail(
                    f"{char_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W16-17 — character lag comp wiring)."
                )

    char_cpp = private_root / "Player/PaldarkCharacter.cpp"
    if char_cpp.is_file():
        text = char_cpp.read_text(encoding="utf-8")
        for symbol in (
            "UPaldarkLagCompensationComponent",
            "HitCollisionBoxes",
            "LagCompSlot",
            "head",
            "pelvis",
            "spine_02",
            "upperarm_l",
            "calf_r",
        ):
            if symbol not in text:
                fail(
                    f"{char_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W16-17 — character constructor hitboxes)."
                )

    # 4. APaldarkPlayerController — time sync RPCs + GetServerTime + console cmds.
    pc_header = public_root / "Player/PaldarkPlayerController.h"
    if pc_header.is_file():
        text = pc_header.read_text(encoding="utf-8")
        for symbol in (
            "ServerRequestServerTime",
            "ClientReportServerTime",
            "GetServerTime",
            "GetSingleTripTime",
            "ClientServerDelta",
            "TimeSyncFrequency",
        ):
            if symbol not in text:
                fail(
                    f"{pc_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W16-17 — controller time sync)."
                )

    pc_cpp = private_root / "Player/PaldarkPlayerController.cpp"
    if pc_cpp.is_file():
        text = pc_cpp.read_text(encoding="utf-8")
        for symbol in (
            "ServerRequestServerTime_Implementation",
            "ClientReportServerTime_Implementation",
            "Paldark.LagComp.DumpHistory",
            "Paldark.LagComp.SimulateHit",
            "Paldark.LagComp.ArtificialPing",
            "FAutoConsoleCommandWithWorldAndArgs",
            "PaldarkLagCompConsole",
        ):
            if symbol not in text:
                fail(
                    f"{pc_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W16-17 — controller body)."
                )


def check_pal_combat_shape() -> None:
    """W18-19 — Pal combat infrastructure.

    Validates:
      1. FPaldarkConsideration struct + ComputeUtilityScore helper exist
         under Public/Pal/Combat/ with a matching .cpp.
      2. UPaldarkPalPerceptionComponent is the real W18-19 component (threat
         scan + hysteresis + grace timer + OnThreatChanged delegate + the
         DumpToLog / ForceThreat debug entry points).
      3. UPaldarkPalCombatComponent grants AttackAbilityClass on BeginPlay,
         tracks LastAttackTime / AttackInterval, exposes TryFireAttack +
         GetCurrentTarget + DumpToLog.
      4. UPaldarkActivity_Combat declares Priority 40 / MinUtilityScore /
         DisengageDistance and overrides CanRun/Tick/Enter/Exit.
      5. UPaldarkGameplayAbility_PalAttack is server-only, declares the
         DamageEffectClass + FallbackBaseDamage knobs, and is added to the
         expected GAS class table (see EXPECTED_GAS_CLASSES — that already
         covers the .h+.cpp existence + PALDARKLAB_API check).
      6. APaldarkCharacter / APaldarkPalCharacter / APaldarkDummyTarget all
         expose `GetTeamTag()` + a UPROPERTY TeamTag member, and the cpp
         constructors initialise to the expected defaults.
      7. DamageExecution reads SetByCaller.PalDamage in addition to BaseDamage.
      8. UPaldarkPalActivityComponent default CandidateActivities seeds
         UPaldarkActivity_Combat::StaticClass() so the FSM picks it up.
      9. PaldarkPalConsoleCommands.cpp registers the 3 W18-19 console
         commands (DumpThreat / SpawnHostileDummy / ForceCombat).
    """
    public_root = PROJECT_DIR / "Source" / "PaldarkLab" / "Public"
    private_root = PROJECT_DIR / "Source" / "PaldarkLab" / "Private"

    # 1. Consideration struct + helper.
    cons_header = public_root / "Pal/Combat/PaldarkPalConsideration.h"
    if not cons_header.is_file():
        fail(
            f"missing {cons_header.relative_to(REPO_ROOT)} "
            "(W18-19 — Utility AI consideration struct)."
        )
    else:
        text = cons_header.read_text(encoding="utf-8")
        for symbol in (
            "FPaldarkConsideration",
            "USTRUCT",
            "NormalizedScore",
            "Weight",
            "ComputeUtilityScore",
        ):
            if symbol not in text:
                fail(
                    f"{cons_header.relative_to(REPO_ROOT)} is missing "
                    f"required symbol {symbol!r} (W18-19 consideration shape)."
                )
    cons_cpp = private_root / "Pal/Combat/PaldarkPalConsideration.cpp"
    if not cons_cpp.is_file():
        fail(
            f"missing {cons_cpp.relative_to(REPO_ROOT)} "
            "(W18-19 — Utility AI consideration helper body)."
        )

    # 2. Perception component — promoted from W3-4 stub to real shape.
    perc_header = public_root / "Pal/Components/PaldarkPalPerceptionComponent.h"
    if perc_header.is_file():
        text = perc_header.read_text(encoding="utf-8")
        for symbol in (
            "ScanInterval",
            "ThreatRadius",
            "AggroSwitchHysteresisCm",
            "ThreatGracePeriodSeconds",
            "FriendlyTeamTag",
            "HostileTeamTag",
            "OnThreatChanged",
            "GetCurrentThreat",
            "GetCurrentThreatDistance",
            "HasThreat",
            "ForceThreat",
            "DumpToLog",
            "TickComponent",
            "BeginPlay",
        ):
            if symbol not in text:
                fail(
                    f"{perc_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W18-19 — real perception shape, not W3-4 stub)."
                )
    perc_cpp = private_root / "Pal/Components/PaldarkPalPerceptionComponent.cpp"
    if perc_cpp.is_file():
        text = perc_cpp.read_text(encoding="utf-8")
        for symbol in (
            "TActorIterator",
            "TAG_Paldark_Team_Player",
            "TAG_Paldark_Team_Hostile",
            "RunScan",
            "IsActorThreatening",
            "GetActorTeamTag",
            "OnThreatChanged.Broadcast",
        ):
            if symbol not in text:
                fail(
                    f"{perc_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W18-19 — perception body)."
                )

    # 3. Combat component — real W18-19 shape.
    cc_header = public_root / "Pal/Components/PaldarkPalCombatComponent.h"
    if cc_header.is_file():
        text = cc_header.read_text(encoding="utf-8")
        for symbol in (
            "AttackAbilityClass",
            "AttackInterval",
            "MinEngageRange",
            "MaxEngageRange",
            "BasePalDamage",
            "TryFireAttack",
            "IsAttackReady",
            "GetCurrentTarget",
            "GetCooldownRemaining",
            "DumpToLog",
            "HandleThreatChanged",
            "GrantedAttackSpec",
            "BeginPlay",
        ):
            if symbol not in text:
                fail(
                    f"{cc_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W18-19 — combat component, not W3-4 stub)."
                )
    cc_cpp = private_root / "Pal/Components/PaldarkPalCombatComponent.cpp"
    if cc_cpp.is_file():
        text = cc_cpp.read_text(encoding="utf-8")
        for symbol in (
            "GiveAbility",
            "TryActivateAbility",
            "LoadSynchronous",
            "OnThreatChanged.AddUObject",
            "ClearAbility",
            "FGameplayAbilitySpec",
            "LastAttackTime",
        ):
            if symbol not in text:
                fail(
                    f"{cc_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W18-19 — combat body)."
                )

    # 4. Combat activity.
    act_header = public_root / "Pal/Activities/PaldarkActivity_Combat.h"
    if act_header.is_file():
        text = act_header.read_text(encoding="utf-8")
        for symbol in (
            "MinUtilityScore",
            "DisengageDistance",
            "ThreatDistanceWeight",
            "PalHealthWeight",
            "AttackReadyWeight",
            "FaceTargetRateDegPerSec",
            "BuildConsiderations",
            "CombatRef",
            "PerceptionRef",
            "CanRun_Implementation",
            "ShouldContinue_Implementation",
            "EnterActivity_Implementation",
            "TickActivity_Implementation",
            "ExitActivity_Implementation",
        ):
            if symbol not in text:
                fail(
                    f"{act_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W18-19 — combat activity shape)."
                )
    act_cpp = private_root / "Pal/Activities/PaldarkActivity_Combat.cpp"
    if act_cpp.is_file():
        text = act_cpp.read_text(encoding="utf-8")
        for symbol in (
            "TAG_Paldark_Pal_Activity_Combat",
            "Priority    = 40",
            "ComputeUtilityScore",
            "TryFireAttack",
            "SetFollowEnabled",
            "RInterpConstantTo",
        ):
            if symbol not in text:
                fail(
                    f"{act_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W18-19 — combat activity body)."
                )

    # 5. PalAttack ability — server-only execution policy + designer knobs.
    pa_header = public_root / "Gas/Abilities/PaldarkGameplayAbility_PalAttack.h"
    if pa_header.is_file():
        text = pa_header.read_text(encoding="utf-8")
        for symbol in (
            "DamageEffectClass",
            "FallbackBaseDamage",
            "ActivateAbility",
        ):
            if symbol not in text:
                fail(
                    f"{pa_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W18-19 — pal attack ability shape)."
                )
    pa_cpp = private_root / "Gas/Abilities/PaldarkGameplayAbility_PalAttack.cpp"
    if pa_cpp.is_file():
        text = pa_cpp.read_text(encoding="utf-8")
        for symbol in (
            "NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly",
            "TAG_Paldark_Ability_PalAttack",
            "TAG_Paldark_SetByCaller_PalDamage",
            "ApplyGameplayEffectSpecToTarget",
            "GetCurrentTarget",
        ):
            if symbol not in text:
                fail(
                    f"{pa_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W18-19 — pal attack ability body)."
                )

    # 6. TeamTag on the 3 character classes.
    team_tag_targets = [
        (public_root  / "Player/PaldarkCharacter.h",   private_root / "Player/PaldarkCharacter.cpp",   "TAG_Paldark_Team_Player"),
        (public_root  / "Pal/PaldarkPalCharacter.h",   private_root / "Pal/PaldarkPalCharacter.cpp",   "TAG_Paldark_Team_Player"),
        (public_root  / "Combat/PaldarkDummyTarget.h", private_root / "Combat/PaldarkDummyTarget.cpp", "TAG_Paldark_Team_Hostile"),
    ]
    for header, cpp, default_tag in team_tag_targets:
        if header.is_file():
            text = header.read_text(encoding="utf-8")
            for symbol in ("GetTeamTag", "TeamTag", "FGameplayTag"):
                if symbol not in text:
                    fail(
                        f"{header.relative_to(REPO_ROOT)} is missing required "
                        f"symbol {symbol!r} (W18-19 — TeamTag shape)."
                    )
        if cpp.is_file():
            text = cpp.read_text(encoding="utf-8")
            if default_tag not in text:
                fail(
                    f"{cpp.relative_to(REPO_ROOT)} does not initialise TeamTag "
                    f"to {default_tag!r} (W18-19 — default team identity)."
                )

    # 7. DamageExecution reads PalDamage in addition to BaseDamage.
    dmg_cpp = private_root / "Gas/PaldarkDamageExecutionCalculation.cpp"
    if dmg_cpp.is_file():
        text = dmg_cpp.read_text(encoding="utf-8")
        for symbol in (
            "TAG_Paldark_SetByCaller_BaseDamage",
            "TAG_Paldark_SetByCaller_PalDamage",
            "WeaponDamage + PalDamage",
        ):
            if symbol not in text:
                fail(
                    f"{dmg_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W18-19 — DamageExecution sums PalDamage)."
                )

    # 8. ActivityComponent default candidates include Combat.
    ac_cpp = private_root / "Pal/Components/PaldarkPalActivityComponent.cpp"
    if ac_cpp.is_file():
        text = ac_cpp.read_text(encoding="utf-8")
        if "UPaldarkActivity_Combat::StaticClass" not in text:
            fail(
                f"{ac_cpp.relative_to(REPO_ROOT)} does not seed "
                "UPaldarkActivity_Combat::StaticClass() into CandidateActivities "
                "(W18-19 — FSM must include the Combat activity by default)."
            )

    # 9. Console commands TU.
    cc_console = private_root / "Pal/PaldarkPalConsoleCommands.cpp"
    if not cc_console.is_file():
        fail(
            f"missing {cc_console.relative_to(REPO_ROOT)} "
            "(W18-19 — Pal combat console commands TU)."
        )
    else:
        text = cc_console.read_text(encoding="utf-8")
        for symbol in (
            "Paldark.Pal.DumpThreat",
            "Paldark.Pal.SpawnHostileDummy",
            "Paldark.Pal.ForceCombat",
            "FAutoConsoleCommandWithWorldAndArgs",
        ):
            if symbol not in text:
                fail(
                    f"{cc_console.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W18-19 — console command registration)."
                )


def check_hostile_pal_shape() -> None:
    """W20-21 — Hostile-Pal AI subclasses + pack subsystem + spawner.

    Validates:
      1. UPaldarkPalPatrolComponent header + cpp expose Waypoints array,
         CurrentWaypointIndex, ArrivalRadius, the waypoint accessors, and
         SetWaypointsFromLocations.
      2. UPaldarkActivity_Patrol declares Priority 15 + the waypoint steering
         knobs, overrides CanRun/Tick/Enter/Exit.
      3. UPaldarkActivity_Stalk declares Priority 25 + StalkSpeedScale +
         LoseSightDistance + the activity FSM overrides.
      4. UPaldarkHostilePackSubsystem declares PackBroadcastRadius +
         RegisterPalToPack / UnregisterPalFromPack / BroadcastPackThreat +
         DumpToLog, and is a UWorldSubsystem.
      5. APaldarkPalCharacter exposes SpeciesTag + PackTag + SetPackTag +
         GetPatrolSlot accessors, and the ctor constructs the PatrolSlot
         component.
      6. APaldarkPalCharacter_Direhound + APaldarkPalCharacter_Razorbird
         exist as C++ subclasses of APaldarkPalCharacter, override the
         constructor + BeginPlay + EndPlay, and stamp the right SpeciesTag.
      7. APaldarkHostilePalSpawner exists with the designer knobs
         (PalClassToSpawn / SpawnCount / WaypointActors / PackTag), the
         BeginPlay spawn loop, and the SpawnHostilePackAt console helper.
      8. UPaldarkPalCombatComponent::HandleThreatChanged calls
         BroadcastPackThreat on the pack subsystem.
      9. PaldarkPalConsoleCommands.cpp registers the 3 W20-21 console
         commands (SpawnDirehoundPack / SpawnRazorbird / DumpPackState).
    """
    public_root = PROJECT_DIR / "Source" / "PaldarkLab" / "Public"
    private_root = PROJECT_DIR / "Source" / "PaldarkLab" / "Private"

    # 1. Patrol component (9th Pal slot).
    pat_header = public_root / "Pal/Components/PaldarkPalPatrolComponent.h"
    if pat_header.is_file():
        text = pat_header.read_text(encoding="utf-8")
        for symbol in (
            "Waypoints",
            "CurrentWaypointIndex",
            "ArrivalRadius",
            "bRandomizeStartIndex",
            "HasWaypoints",
            "GetCurrentWaypoint",
            "AdvanceToNextWaypoint",
            "SetWaypointsFromLocations",
            "DumpToLog",
        ):
            if symbol not in text:
                fail(
                    f"{pat_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W20-21 — patrol component shape)."
                )
    pat_cpp = private_root / "Pal/Components/PaldarkPalPatrolComponent.cpp"
    if not pat_cpp.is_file():
        fail(
            f"missing {pat_cpp.relative_to(REPO_ROOT)} "
            "(W20-21 — patrol component body)."
        )

    # 2. Patrol activity.
    patrol_act = public_root / "Pal/Activities/PaldarkActivity_Patrol.h"
    if patrol_act.is_file():
        text = patrol_act.read_text(encoding="utf-8")
        for symbol in (
            "CanRun_Implementation",
            "ShouldContinue_Implementation",
            "EnterActivity_Implementation",
            "TickActivity_Implementation",
            "ExitActivity_Implementation",
        ):
            if symbol not in text:
                fail(
                    f"{patrol_act.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W20-21 — patrol activity overrides)."
                )
    patrol_cpp = private_root / "Pal/Activities/PaldarkActivity_Patrol.cpp"
    if patrol_cpp.is_file():
        text = patrol_cpp.read_text(encoding="utf-8")
        for symbol in (
            "TAG_Paldark_Pal_Activity_Patrol",
            "Priority    = 15",
            "AddMovementInput",
        ):
            if symbol not in text:
                fail(
                    f"{patrol_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W20-21 — patrol activity body)."
                )

    # 3. Stalk activity.
    stalk_act = public_root / "Pal/Activities/PaldarkActivity_Stalk.h"
    if stalk_act.is_file():
        text = stalk_act.read_text(encoding="utf-8")
        for symbol in (
            "StalkSpeedScale",
            "LoseSightDistance",
            "CanRun_Implementation",
            "ShouldContinue_Implementation",
            "EnterActivity_Implementation",
            "TickActivity_Implementation",
            "ExitActivity_Implementation",
        ):
            if symbol not in text:
                fail(
                    f"{stalk_act.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W20-21 — stalk activity shape)."
                )
    stalk_cpp = private_root / "Pal/Activities/PaldarkActivity_Stalk.cpp"
    if stalk_cpp.is_file():
        text = stalk_cpp.read_text(encoding="utf-8")
        for symbol in (
            "TAG_Paldark_Pal_Activity_Stalk",
            "Priority    = 25",
            "MaxWalkSpeed",
        ):
            if symbol not in text:
                fail(
                    f"{stalk_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W20-21 — stalk activity body)."
                )

    # 4. Pack subsystem.
    pack_header = public_root / "Pal/PaldarkHostilePackSubsystem.h"
    if pack_header.is_file():
        text = pack_header.read_text(encoding="utf-8")
        for symbol in (
            "UWorldSubsystem",
            "PackBroadcastRadius",
            "RegisterPalToPack",
            "UnregisterPalFromPack",
            "BroadcastPackThreat",
            "GetPackSize",
            "DumpToLog",
        ):
            if symbol not in text:
                fail(
                    f"{pack_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W20-21 — pack subsystem shape)."
                )
    pack_cpp = private_root / "Pal/PaldarkHostilePackSubsystem.cpp"
    if pack_cpp.is_file():
        text = pack_cpp.read_text(encoding="utf-8")
        for symbol in (
            "ForceThreat",
            "ShouldCreateSubsystem",
        ):
            if symbol not in text:
                fail(
                    f"{pack_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W20-21 — pack subsystem body)."
                )

    # 5. Base APaldarkPalCharacter exposes SpeciesTag / PackTag / PatrolSlot.
    base_h = public_root / "Pal/PaldarkPalCharacter.h"
    if base_h.is_file():
        text = base_h.read_text(encoding="utf-8")
        for symbol in (
            "SpeciesTag",
            "PackTag",
            "PatrolSlot",
            "GetSpeciesTag",
            "GetPackTag",
            "GetPatrolSlot",
            "SetPackTag",
        ):
            if symbol not in text:
                fail(
                    f"{base_h.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W20-21 — base Pal exposes hostile hooks)."
                )
    base_cpp = private_root / "Pal/PaldarkPalCharacter.cpp"
    if base_cpp.is_file():
        text = base_cpp.read_text(encoding="utf-8")
        if "UPaldarkPalPatrolComponent" not in text:
            fail(
                f"{base_cpp.relative_to(REPO_ROOT)} does not construct the "
                "PatrolSlot component (W20-21 — base Pal must own the 9th slot)."
            )
        if "SetPackTag" not in text:
            fail(
                f"{base_cpp.relative_to(REPO_ROOT)} does not implement "
                "SetPackTag (W20-21 — spawner needs runtime pack stamp)."
            )

    # 6. Direhound + Razorbird subclasses.
    species = [
        (
            public_root / "Pal/PaldarkPalCharacter_Direhound.h",
            private_root / "Pal/PaldarkPalCharacter_Direhound.cpp",
            "APaldarkPalCharacter_Direhound",
            "TAG_Paldark_Pal_Species_Direhound",
        ),
        (
            public_root / "Pal/PaldarkPalCharacter_Razorbird.h",
            private_root / "Pal/PaldarkPalCharacter_Razorbird.cpp",
            "APaldarkPalCharacter_Razorbird",
            "TAG_Paldark_Pal_Species_Razorbird",
        ),
    ]
    for header, cpp, classname, species_tag in species:
        if not header.is_file():
            fail(f"missing {header.relative_to(REPO_ROOT)} (W20-21 hostile-Pal subclass).")
            continue
        text = header.read_text(encoding="utf-8")
        for symbol in (
            "PaldarkPalCharacter.h",
            classname,
            "BeginPlay",
            "EndPlay",
        ):
            if symbol not in text:
                fail(
                    f"{header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W20-21 hostile subclass shape)."
                )
        if not cpp.is_file():
            fail(f"missing {cpp.relative_to(REPO_ROOT)} (W20-21 hostile-Pal subclass body).")
            continue
        text = cpp.read_text(encoding="utf-8")
        for symbol in (
            species_tag,
            "TAG_Paldark_Team_Hostile",
            "UPaldarkActivity_Patrol::StaticClass",
            "UPaldarkActivity_Stalk::StaticClass",
            "UPaldarkActivity_Combat::StaticClass",
            "RegisterPalToPack",
            "UnregisterPalFromPack",
        ):
            if symbol not in text:
                fail(
                    f"{cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W20-21 hostile subclass body)."
                )

    # Razorbird specific — must flip to MOVE_Flying.
    razor_cpp = private_root / "Pal/PaldarkPalCharacter_Razorbird.cpp"
    if razor_cpp.is_file():
        text = razor_cpp.read_text(encoding="utf-8")
        if "MOVE_Flying" not in text:
            fail(
                f"{razor_cpp.relative_to(REPO_ROOT)} does not set movement "
                "mode to MOVE_Flying (W20-21 — Razorbird is aerial)."
            )

    # 7. Spawner actor.
    spawn_header = public_root / "Pal/PaldarkHostilePalSpawner.h"
    if spawn_header.is_file():
        text = spawn_header.read_text(encoding="utf-8")
        for symbol in (
            "APaldarkHostilePalSpawner",
            "PalClassToSpawn",
            "SpawnCount",
            "WaypointActors",
            "PackTag",
            "SpawnHostilePackAt",
            "BeginPlay",
        ):
            if symbol not in text:
                fail(
                    f"{spawn_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W20-21 — spawner shape)."
                )
    spawn_cpp = private_root / "Pal/PaldarkHostilePalSpawner.cpp"
    if spawn_cpp.is_file():
        text = spawn_cpp.read_text(encoding="utf-8")
        for symbol in (
            "SpawnActor<APaldarkPalCharacter>",
            "SetWaypointsFromLocations",
            "SetPackTag",
        ):
            if symbol not in text:
                fail(
                    f"{spawn_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W20-21 — spawner body)."
                )

    # 8. Combat component broadcasts to pack on HandleThreatChanged.
    cc_cpp = private_root / "Pal/Components/PaldarkPalCombatComponent.cpp"
    if cc_cpp.is_file():
        text = cc_cpp.read_text(encoding="utf-8")
        for symbol in (
            "UPaldarkHostilePackSubsystem",
            "BroadcastPackThreat",
        ):
            if symbol not in text:
                fail(
                    f"{cc_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} "
                    "(W20-21 — combat must broadcast threat to packmates)."
                )

    # 9. Console commands TU registers the 3 W20-21 commands.
    cc_console = private_root / "Pal/PaldarkPalConsoleCommands.cpp"
    if cc_console.is_file():
        text = cc_console.read_text(encoding="utf-8")
        for symbol in (
            "Paldark.Pal.SpawnDirehoundPack",
            "Paldark.Pal.SpawnRazorbird",
            "Paldark.Pal.DumpPackState",
        ):
            if symbol not in text:
                fail(
                    f"{cc_console.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W20-21 — console command registration)."
                )


def check_squad_shape() -> None:
    """W22-23 — Squad subsystem + ping subsystem + radial command set +
    membership/command components + Pal squad-listener activity + player
    controller RPCs + 3 console commands.

    Validates:
      1. UPaldarkSquadSubsystem header + cpp expose the per-squad registry +
         broadcast delegate + standing-command cache. Server-only via
         ShouldCreateSubsystem.
      2. UPaldarkSquadCommandSet (UPrimaryDataAsset) declares the row struct
         + Commands array + FindRowByTag + GetPrimaryAssetId override.
      3. UPaldarkSquadMembershipComponent + UPaldarkSquadCommandComponent
         declare the expected lifecycle hooks + Server RPC + accessor pair.
      4. APaldarkPingMarker is a replicated AActor with the 4 replicated
         properties (PingType / OwningPlayer / MarkedActor / ExpirationServerTime),
         lifetime tick, and DOREPLIFETIME registration in the cpp.
      5. UPaldarkPingSubsystem (UWorldSubsystem) exposes SpawnPing +
         GetActivePingCount + the designer knobs (marker class, lifetime,
         rate limit). Server-only via ShouldCreateSubsystem.
      6. UPaldarkActivity_FollowSquadCommand declares Priority 22 + the
         expected CurrentCommandTag/CurrentCommandLocation/ArrivalToleranceCm
         knobs + overrides + HandleSquadCommandIssued delegate handler.
      7. APaldarkCharacter.h declares the 2 new component slots + getters,
         and the cpp constructs them via CreateDefaultSubobject.
      8. APaldarkPlayerController.h declares Server_RequestPing +
         Server_RequestMarkUnderCrosshair (UFUNCTION Server Reliable) +
         MarkEnemyTraceDistance knob. The cpp implements both _Implementation
         entry points and registers the 3 W22-23 console commands.
      9. UPaldarkPalActivityComponent.cpp seeds the new FollowSquadCommand
         class in the default CandidateActivities list.
      10. DefaultGame.ini registers PaldarkSquadCommandSet as a
          PrimaryAssetType so the editor's Asset Manager can scan it.
    """
    public_root = PROJECT_DIR / "Source" / "PaldarkLab" / "Public"
    private_root = PROJECT_DIR / "Source" / "PaldarkLab" / "Private"

    # 1. Squad subsystem.
    sub_header = public_root / "Squad/PaldarkSquadSubsystem.h"
    if sub_header.is_file():
        text = sub_header.read_text(encoding="utf-8")
        for symbol in (
            "UWorldSubsystem",
            "FPaldarkSquadRoster",
            "FPaldarkOnSquadCommandIssued",
            "RegisterPlayerToSquad",
            "UnregisterPlayerFromSquad",
            "BroadcastSquadCommand",
            "GetSquadSize",
            "GetSquadTagForPlayer",
            "GetSquadMembersForPlayer",
            "GetStandingCommand",
            "GetStandingCommandLocation",
            "OnSquadCommandIssued",
            "DumpToLog",
        ):
            if symbol not in text:
                fail(
                    f"{sub_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W22-23 — squad subsystem shape)."
                )
    else:
        fail(f"missing {sub_header.relative_to(REPO_ROOT)} (W22-23 squad subsystem).")
    sub_cpp = private_root / "Squad/PaldarkSquadSubsystem.cpp"
    if sub_cpp.is_file():
        text = sub_cpp.read_text(encoding="utf-8")
        for symbol in (
            "ShouldCreateSubsystem",
            "OnSquadCommandIssued.Broadcast",
            "StandingCommandPerSquad",
        ):
            if symbol not in text:
                fail(
                    f"{sub_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W22-23 — squad subsystem body)."
                )
    else:
        fail(f"missing {sub_cpp.relative_to(REPO_ROOT)} (W22-23 squad subsystem body).")

    # 2. Squad command set asset.
    set_header = public_root / "Squad/PaldarkSquadCommandSet.h"
    if set_header.is_file():
        text = set_header.read_text(encoding="utf-8")
        for symbol in (
            "UPrimaryDataAsset",
            "FPaldarkSquadCommandRow",
            "CommandTag",
            "DisplayName",
            "Icon",
            "Commands",
            "FindRowByTag",
            "GetPrimaryAssetId",
        ):
            if symbol not in text:
                fail(
                    f"{set_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W22-23 — radial command set shape)."
                )
    else:
        fail(f"missing {set_header.relative_to(REPO_ROOT)} (W22-23 radial command set).")
    set_cpp = private_root / "Squad/PaldarkSquadCommandSet.cpp"
    if not set_cpp.is_file():
        fail(f"missing {set_cpp.relative_to(REPO_ROOT)} (W22-23 radial command set body).")

    # 3. Squad membership + command components.
    mem_header = public_root / "Squad/PaldarkSquadMembershipComponent.h"
    if mem_header.is_file():
        text = mem_header.read_text(encoding="utf-8")
        for symbol in (
            "UPaldarkSquadMembershipComponent",
            "SquadTag",
            "GetSquadTag",
            "SetSquadTag",
            "BeginPlay",
            "EndPlay",
        ):
            if symbol not in text:
                fail(
                    f"{mem_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W22-23 — squad membership component shape)."
                )
    else:
        fail(f"missing {mem_header.relative_to(REPO_ROOT)} (W22-23 squad membership).")
    mem_cpp = private_root / "Squad/PaldarkSquadMembershipComponent.cpp"
    if mem_cpp.is_file():
        text = mem_cpp.read_text(encoding="utf-8")
        for symbol in (
            "RegisterPlayerToSquad",
            "UnregisterPlayerFromSquad",
            "HasAuthority",
        ):
            if symbol not in text:
                fail(
                    f"{mem_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W22-23 — squad membership body)."
                )

    cmd_header = public_root / "Squad/PaldarkSquadCommandComponent.h"
    if cmd_header.is_file():
        text = cmd_header.read_text(encoding="utf-8")
        for symbol in (
            "UPaldarkSquadCommandComponent",
            "IssueCommand",
            "Server_IssueCommand",
            "GetCommandSet",
            "CommandSetSoft",
        ):
            if symbol not in text:
                fail(
                    f"{cmd_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W22-23 — squad command component shape)."
                )
        if "UFUNCTION(Server" not in text:
            fail(
                f"{cmd_header.relative_to(REPO_ROOT)} must declare a Server RPC "
                "(W22-23 — Server_IssueCommand UFUNCTION)."
            )
    else:
        fail(f"missing {cmd_header.relative_to(REPO_ROOT)} (W22-23 squad command).")
    cmd_cpp = private_root / "Squad/PaldarkSquadCommandComponent.cpp"
    if cmd_cpp.is_file():
        text = cmd_cpp.read_text(encoding="utf-8")
        for symbol in (
            "Server_IssueCommand_Implementation",
            "BroadcastSquadCommand",
            "FindRowByTag",
        ):
            if symbol not in text:
                fail(
                    f"{cmd_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W22-23 — squad command body)."
                )

    # 4. Ping marker actor (replicated).
    marker_header = public_root / "Squad/PaldarkPingMarker.h"
    if marker_header.is_file():
        text = marker_header.read_text(encoding="utf-8")
        for symbol in (
            "APaldarkPingMarker",
            "InitMarker",
            "GetPingType",
            "GetOwningPlayer",
            "GetMarkedActor",
            "GetRemainingLifetime",
            "PingType",
            "OwningPlayer",
            "MarkedActor",
            "ExpirationServerTime",
            "GetLifetimeReplicatedProps",
        ):
            if symbol not in text:
                fail(
                    f"{marker_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W22-23 — ping marker shape)."
                )
    else:
        fail(f"missing {marker_header.relative_to(REPO_ROOT)} (W22-23 ping marker).")
    marker_cpp = private_root / "Squad/PaldarkPingMarker.cpp"
    if marker_cpp.is_file():
        text = marker_cpp.read_text(encoding="utf-8")
        for symbol in (
            "DOREPLIFETIME",
            "bReplicates",
            "HasAuthority",
        ):
            if symbol not in text:
                fail(
                    f"{marker_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W22-23 — ping marker body)."
                )
        for replicated in ("PingType", "OwningPlayer", "MarkedActor", "ExpirationServerTime"):
            pattern = rf"DOREPLIFETIME\s*\(\s*APaldarkPingMarker\s*,\s*{re.escape(replicated)}\s*\)"
            if not re.search(pattern, text):
                fail(
                    f"{marker_cpp.relative_to(REPO_ROOT)} must register "
                    f"DOREPLIFETIME(APaldarkPingMarker, {replicated}) "
                    "(W22-23 — ping marker replication)."
                )

    # 5. Ping subsystem.
    ping_header = public_root / "Squad/PaldarkPingSubsystem.h"
    if ping_header.is_file():
        text = ping_header.read_text(encoding="utf-8")
        for symbol in (
            "UWorldSubsystem",
            "SpawnPing",
            "GetActivePingCount",
            "MarkerClass",
            "DefaultPingLifetime",
            "MinSecondsBetweenPings",
        ):
            if symbol not in text:
                fail(
                    f"{ping_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W22-23 — ping subsystem shape)."
                )
    else:
        fail(f"missing {ping_header.relative_to(REPO_ROOT)} (W22-23 ping subsystem).")
    ping_cpp = private_root / "Squad/PaldarkPingSubsystem.cpp"
    if ping_cpp.is_file():
        text = ping_cpp.read_text(encoding="utf-8")
        for symbol in (
            "ShouldCreateSubsystem",
            "SpawnActor",
            "InitMarker",
            "LastPingTimePerPlayer",
        ):
            if symbol not in text:
                fail(
                    f"{ping_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W22-23 — ping subsystem body)."
                )

    # 6. Pal squad-command listener activity (P22).
    fsc_header = public_root / "Pal/Activities/PaldarkActivity_FollowSquadCommand.h"
    if fsc_header.is_file():
        text = fsc_header.read_text(encoding="utf-8")
        for symbol in (
            "UPaldarkActivity_FollowSquadCommand",
            "CanRun_Implementation",
            "ShouldContinue_Implementation",
            "EnterActivity_Implementation",
            "TickActivity_Implementation",
            "ExitActivity_Implementation",
            "InitActivity",
            "HandleSquadCommandIssued",
            "CurrentCommandTag",
            "CurrentCommandLocation",
            "ArrivalToleranceCm",
        ):
            if symbol not in text:
                fail(
                    f"{fsc_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W22-23 — squad listener activity shape)."
                )
    fsc_cpp = private_root / "Pal/Activities/PaldarkActivity_FollowSquadCommand.cpp"
    if fsc_cpp.is_file():
        text = fsc_cpp.read_text(encoding="utf-8")
        for symbol in (
            "TAG_Paldark_Pal_Activity_FollowSquadCommand",
            "OnSquadCommandIssued",
            "AddUObject",
        ):
            if symbol not in text:
                fail(
                    f"{fsc_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W22-23 — squad listener activity body)."
                )
        if not re.search(r"\bPriority\s*=\s*22\b", text):
            fail(
                f"{fsc_cpp.relative_to(REPO_ROOT)} must set Priority = 22 "
                "(W22-23 — squad listener activity sits between Follow P20 and Investigate P30)."
            )

    # 7. APaldarkCharacter must own the 2 new slots in BOTH header + ctor.
    char_header = public_root / "Player/PaldarkCharacter.h"
    if char_header.is_file():
        text = char_header.read_text(encoding="utf-8")
        for symbol in (
            "SquadMembershipSlot",
            "SquadCommandSlot",
            "GetSquadMembershipSlot",
            "GetSquadCommandSlot",
            "UPaldarkSquadMembershipComponent",
            "UPaldarkSquadCommandComponent",
        ):
            if symbol not in text:
                fail(
                    f"{char_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W22-23 — player adds 2 squad slots)."
                )
    char_cpp = private_root / "Player/PaldarkCharacter.cpp"
    if char_cpp.is_file():
        text = char_cpp.read_text(encoding="utf-8")
        for cls in (
            "UPaldarkSquadMembershipComponent",
            "UPaldarkSquadCommandComponent",
        ):
            pattern = rf"CreateDefaultSubobject<\s*{re.escape(cls)}\s*>"
            if not re.search(pattern, text):
                fail(
                    f"{char_cpp.relative_to(REPO_ROOT)} must CreateDefaultSubobject<{cls}>(...) "
                    "(W22-23 — player constructor instantiates the new squad slots)."
                )

    # 8. APaldarkPlayerController must declare + implement the 2 RPCs and
    # register the 3 console commands.
    pc_header = public_root / "Player/PaldarkPlayerController.h"
    if pc_header.is_file():
        text = pc_header.read_text(encoding="utf-8")
        for symbol in (
            "Server_RequestPing",
            "Server_RequestMarkUnderCrosshair",
            "MarkEnemyTraceDistance",
            "GameplayTagContainer.h",
        ):
            if symbol not in text:
                fail(
                    f"{pc_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W22-23 — player controller squad RPCs)."
                )
    pc_cpp = private_root / "Player/PaldarkPlayerController.cpp"
    if pc_cpp.is_file():
        text = pc_cpp.read_text(encoding="utf-8")
        for symbol in (
            "Server_RequestPing_Implementation",
            "Server_RequestMarkUnderCrosshair_Implementation",
            "Paldark.Squad.Dump",
            "Paldark.Squad.Ping",
            "Paldark.Squad.Command",
            "TAG_Paldark_Ping_Type_Spot",
            "TAG_Paldark_Ping_Type_Enemy",
        ):
            if symbol not in text:
                fail(
                    f"{pc_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W22-23 — player controller squad body)."
                )

    # 9. Friendly Pal activity component seeds FollowSquadCommand.
    pa_cpp = private_root / "Pal/Components/PaldarkPalActivityComponent.cpp"
    if pa_cpp.is_file():
        text = pa_cpp.read_text(encoding="utf-8")
        if "UPaldarkActivity_FollowSquadCommand::StaticClass" not in text:
            fail(
                f"{pa_cpp.relative_to(REPO_ROOT)} must seed "
                "`UPaldarkActivity_FollowSquadCommand::StaticClass()` in "
                "CandidateActivities (W22-23 — friendly Pal listens to squad commands)."
            )

    # 10. Asset manager primary type for the command set asset.
    game_ini = PROJECT_DIR / "Config" / "DefaultGame.ini"
    if game_ini.is_file():
        text = game_ini.read_text(encoding="utf-8")
        if 'PrimaryAssetType="PaldarkSquadCommandSet"' not in text:
            fail(
                f"{game_ini.relative_to(REPO_ROOT)} must register "
                'PrimaryAssetType="PaldarkSquadCommandSet" so the Asset Manager '
                "discovers radial command sets (W22-23)."
            )


def check_match_shape() -> None:
    """W24-25 — Match subsystem + extraction beacon + GameMode_Extraction +
    PlayerState/GameStateBase REPNOTIFY extensions + 3 console commands +
    death hook wiring.

    Validates:
      1. PaldarkMatchTypes.h declares the 3 enums + per-player row struct
         + tag helper functions.
      2. UPaldarkMatchSubsystem (UWorldSubsystem) is server-only and
         exposes the phase FSM + per-player registry + delegates.
      3. APaldarkExtractionBeacon (replicated AActor) declares sphere
         trigger + designer knobs + Tick + overlap delegates.
      4. APaldarkPlayerState extension: REPNOTIFY MatchOutcome /
         ExtractionProgress + GetLifetimeReplicatedProps + delegate.
      5. APaldarkGameStateBase extension: REPNOTIFY MatchPhase /
         MatchEndReason / PhaseEndServerTime + delegate + setter.
      6. APaldarkGameMode_Extraction subclasses APaldarkGameModeBase and
         overrides PostLogin / Logout / BeginPlay / EndPlay.
      7. APaldarkCharacter wires the death hook (Health zero ->
         RecordDeath) in InitAbilitySystem.
      8. APaldarkPlayerController registers the 3 Paldark.Match.*
         console commands.
    """
    public_root  = PROJECT_DIR / "Source" / "PaldarkLab" / "Public"
    private_root = PROJECT_DIR / "Source" / "PaldarkLab" / "Private"

    # 1. Match types header — enums + row struct + tag helpers.
    types_header = public_root / "Match/PaldarkMatchTypes.h"
    if types_header.is_file():
        text = types_header.read_text(encoding="utf-8")
        for symbol in (
            "EPaldarkMatchPhase",
            "EPaldarkPlayerOutcome",
            "EPaldarkMatchEndReason",
            "FPaldarkMatchPlayerRow",
            "PaldarkOutcomeToTag",
            "PaldarkPhaseToTag",
        ):
            if symbol not in text:
                fail(
                    f"{types_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W24-25 — match types shape)."
                )
        for phase in ("Warmup", "Active", "Extracting", "Ended"):
            if phase not in text:
                fail(
                    f"{types_header.relative_to(REPO_ROOT)} is missing match "
                    f"phase enumerator {phase!r} (W24-25)."
                )
        for outcome in ("Alive", "Extracted", "KIA", "Disconnected"):
            if outcome not in text:
                fail(
                    f"{types_header.relative_to(REPO_ROOT)} is missing player "
                    f"outcome enumerator {outcome!r} (W24-25)."
                )
    else:
        fail(f"missing {types_header.relative_to(REPO_ROOT)} (W24-25 match types).")

    # 2. Match subsystem (UWorldSubsystem, server-only, FSM + registry).
    sub_header = public_root / "Match/PaldarkMatchSubsystem.h"
    if sub_header.is_file():
        text = sub_header.read_text(encoding="utf-8")
        for symbol in (
            "UWorldSubsystem",
            "UPaldarkMatchSubsystem",
            "RegisterPlayer",
            "UnregisterPlayer",
            "StartMatch",
            "RecordExtraction",
            "RecordDeath",
            "RecordDisconnect",
            "EndMatch",
            "GetPhase",
            "GetPlayerOutcome",
            "DumpToLog",
            "OnMatchPhaseChanged",
            "OnMatchEnded",
            "OnPlayerOutcomeChanged",
            "MaxMatchDuration",
            "WarmupDuration",
        ):
            if symbol not in text:
                fail(
                    f"{sub_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W24-25 — match subsystem shape)."
                )
    else:
        fail(f"missing {sub_header.relative_to(REPO_ROOT)} (W24-25 match subsystem).")
    sub_cpp = private_root / "Match/PaldarkMatchSubsystem.cpp"
    if sub_cpp.is_file():
        text = sub_cpp.read_text(encoding="utf-8")
        for symbol in (
            "ShouldCreateSubsystem",
            "EvaluateEndCondition",
            "SetPhase",
            "MirrorToGameState",
            "OnMatchPhaseChanged.Broadcast",
            "OnMatchEnded.Broadcast",
            "OnPlayerOutcomeChanged.Broadcast",
        ):
            if symbol not in text:
                fail(
                    f"{sub_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W24-25 — match subsystem body)."
                )
    else:
        fail(f"missing {sub_cpp.relative_to(REPO_ROOT)} (W24-25 match subsystem body).")

    # 3. Extraction beacon (replicated actor + sphere trigger + Tick).
    beacon_header = public_root / "Match/PaldarkExtractionBeacon.h"
    if beacon_header.is_file():
        text = beacon_header.read_text(encoding="utf-8")
        for symbol in (
            "APaldarkExtractionBeacon",
            "USphereComponent",
            "ExtractionTrigger",
            "TriggerRadius",
            "ExtractionDuration",
            "bRequireExplicitConsent",
            "bDecayOnLeave",
            "OnSphereBeginOverlap",
            "OnSphereEndOverlap",
            "RegisterConsentToExtract",
        ):
            if symbol not in text:
                fail(
                    f"{beacon_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W24-25 — extraction beacon shape)."
                )
    else:
        fail(f"missing {beacon_header.relative_to(REPO_ROOT)} (W24-25 extraction beacon).")
    beacon_cpp = private_root / "Match/PaldarkExtractionBeacon.cpp"
    if beacon_cpp.is_file():
        text = beacon_cpp.read_text(encoding="utf-8")
        for symbol in (
            "bReplicates",
            "InitSphereRadius",
            "OnComponentBeginOverlap.AddDynamic",
            "OnComponentEndOverlap.AddDynamic",
            "RecordExtraction",
            "SetExtractionProgress",
            "HasAuthority",
        ):
            if symbol not in text:
                fail(
                    f"{beacon_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W24-25 — extraction beacon body)."
                )
    else:
        fail(f"missing {beacon_cpp.relative_to(REPO_ROOT)} (W24-25 extraction beacon body).")

    # 4. PlayerState extension — REPNOTIFY MatchOutcome / ExtractionProgress.
    ps_header = public_root / "Player/PaldarkPlayerState.h"
    if ps_header.is_file():
        text = ps_header.read_text(encoding="utf-8")
        for symbol in (
            "OnRep_MatchOutcome",
            "OnRep_ExtractionProgress",
            "SetMatchOutcome",
            "SetExtractionProgress",
            "GetMatchOutcome",
            "GetExtractionProgress",
            "IsAliveInMatch",
            "FPaldarkOnPlayerMatchOutcomeChanged",
            "OnMatchOutcomeChanged",
        ):
            if symbol not in text:
                fail(
                    f"{ps_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W24-25 — PlayerState match extension)."
                )
        if "ReplicatedUsing = OnRep_MatchOutcome" not in text:
            fail(
                f"{ps_header.relative_to(REPO_ROOT)} must mark MatchOutcome "
                "ReplicatedUsing=OnRep_MatchOutcome (W24-25)."
            )
        if "ReplicatedUsing = OnRep_ExtractionProgress" not in text:
            fail(
                f"{ps_header.relative_to(REPO_ROOT)} must mark ExtractionProgress "
                "ReplicatedUsing=OnRep_ExtractionProgress (W24-25)."
            )
    ps_cpp = private_root / "Player/PaldarkPlayerState.cpp"
    if ps_cpp.is_file():
        text = ps_cpp.read_text(encoding="utf-8")
        for replicated in ("MatchOutcome", "ExtractionProgress"):
            pattern = rf"DOREPLIFETIME\s*\(\s*APaldarkPlayerState\s*,\s*{re.escape(replicated)}\s*\)"
            if not re.search(pattern, text):
                fail(
                    f"{ps_cpp.relative_to(REPO_ROOT)} must register "
                    f"DOREPLIFETIME(APaldarkPlayerState, {replicated}) (W24-25)."
                )

    # 5. GameStateBase extension — REPNOTIFY MatchPhase / Reason / Timer.
    gs_header = public_root / "Framework/PaldarkGameStateBase.h"
    if gs_header.is_file():
        text = gs_header.read_text(encoding="utf-8")
        for symbol in (
            "OnRep_MatchPhase",
            "OnRep_MatchEndReason",
            "OnRep_PhaseEndServerTime",
            "SetMatchPhaseFromSubsystem",
            "GetMatchPhase",
            "GetMatchEndReason",
            "GetPhaseEndServerTime",
            "FPaldarkOnMatchPhaseReplicated",
            "OnMatchPhaseReplicated",
        ):
            if symbol not in text:
                fail(
                    f"{gs_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W24-25 — GameStateBase match extension)."
                )
        if "ReplicatedUsing = OnRep_MatchPhase" not in text:
            fail(
                f"{gs_header.relative_to(REPO_ROOT)} must mark MatchPhase "
                "ReplicatedUsing=OnRep_MatchPhase (W24-25)."
            )
    gs_cpp = private_root / "Framework/PaldarkGameStateBase.cpp"
    if gs_cpp.is_file():
        text = gs_cpp.read_text(encoding="utf-8")
        for replicated in ("MatchPhase", "MatchEndReason", "PhaseEndServerTime"):
            pattern = rf"DOREPLIFETIME\s*\(\s*APaldarkGameStateBase\s*,\s*{re.escape(replicated)}\s*\)"
            if not re.search(pattern, text):
                fail(
                    f"{gs_cpp.relative_to(REPO_ROOT)} must register "
                    f"DOREPLIFETIME(APaldarkGameStateBase, {replicated}) (W24-25)."
                )

    # 6. GameMode_Extraction subclass.
    gm_header = public_root / "Framework/PaldarkGameMode_Extraction.h"
    if gm_header.is_file():
        text = gm_header.read_text(encoding="utf-8")
        for symbol in (
            "APaldarkGameMode_Extraction",
            "APaldarkGameModeBase",
            "PostLogin",
            "Logout",
            "BeginPlay",
            "EndPlay",
        ):
            if symbol not in text:
                fail(
                    f"{gm_header.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W24-25 — extraction GameMode shape)."
                )
    else:
        fail(f"missing {gm_header.relative_to(REPO_ROOT)} (W24-25 extraction GameMode).")
    gm_cpp = private_root / "Framework/PaldarkGameMode_Extraction.cpp"
    if gm_cpp.is_file():
        text = gm_cpp.read_text(encoding="utf-8")
        for symbol in (
            "RegisterPlayer",
            "RecordDisconnect",
            "UnregisterPlayer",
            "GetMatchSubsystem",
            "OnMatchEnded",
        ):
            if symbol not in text:
                fail(
                    f"{gm_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W24-25 — extraction GameMode body)."
                )

    # 7. Death hook wiring on the player character.
    char_cpp = private_root / "Player/PaldarkCharacter.cpp"
    if char_cpp.is_file():
        text = char_cpp.read_text(encoding="utf-8")
        for symbol in (
            "OnHealthZeroed",
            "RecordDeath",
            "UPaldarkMatchSubsystem",
        ):
            if symbol not in text:
                fail(
                    f"{char_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W24-25 — player death hook wiring)."
                )

    # 8. Player controller match console commands.
    pc_cpp = private_root / "Player/PaldarkPlayerController.cpp"
    if pc_cpp.is_file():
        text = pc_cpp.read_text(encoding="utf-8")
        for symbol in (
            "Paldark.Match.Dump",
            "Paldark.Match.Phase",
            "Paldark.Match.ForceEnd",
        ):
            if symbol not in text:
                fail(
                    f"{pc_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W24-25 — match console commands)."
                )


def check_extraction_gamemode_wired() -> None:
    """W26 — L-01 closure: enforce three-layer defense so designers can't
    silently boot Raid_Sandbox with the wrong GameMode and discover that
    PostLogin / Logout / OnMatchEnded never fire.

    Three layers (any one of them missing breaks the safety net):

    1. UPaldarkExperienceDefinition declares a RequiredGameModeClass field
       (TSoftClassPtr<APaldarkGameModeBase>) — designer authors per-experience.
    2. APaldarkGameModeBase::OnExperienceAssetReady asserts that the spawned
       GameMode is a subclass of RequiredGameModeClass, logs Error level
       through LogPaldark, and surfaces an on-screen red AddOnScreenDebugMessage.
    3. DefaultEngine.ini ships a +GameModeClassAliases entry mapping the
       short URL alias `Extraction` to `APaldarkGameMode_Extraction` so the
       designer fix path (re-launch with `?game=Extraction`) actually resolves.
    """
    public_root = PROJECT_DIR / "Source" / "PaldarkLab" / "Public"
    private_root = PROJECT_DIR / "Source" / "PaldarkLab" / "Private"

    # Layer 1 — Experience definition has the assertion field.
    exp_header = public_root / "Experience/PaldarkExperienceDefinition.h"
    if not exp_header.is_file():
        fail(
            f"missing {exp_header.relative_to(REPO_ROOT)} "
            "(W26 L-01 — experience definition required for GameMode assertion)."
        )
    else:
        text = exp_header.read_text(encoding="utf-8")
        if "class APaldarkGameModeBase;" not in text:
            fail(
                f"{exp_header.relative_to(REPO_ROOT)} must forward-declare "
                "APaldarkGameModeBase (W26 L-01 — needed for RequiredGameModeClass)."
            )
        # The field must be a soft class pointer typed to the GameMode base so
        # any subclass (Extraction, future PvP mode, etc.) satisfies it.
        if not re.search(
            r"TSoftClassPtr\s*<\s*APaldarkGameModeBase\s*>\s+RequiredGameModeClass\s*;",
            text,
        ):
            fail(
                f"{exp_header.relative_to(REPO_ROOT)} must declare "
                "`TSoftClassPtr<APaldarkGameModeBase> RequiredGameModeClass;` "
                "(W26 L-01 — extraction GameMode assertion field)."
            )
        # UPROPERTY decoration so the designer can author it in editor.
        if "RequiredGameModeClass" in text and "EditDefaultsOnly" not in text:
            fail(
                f"{exp_header.relative_to(REPO_ROOT)} must mark "
                "RequiredGameModeClass `EditDefaultsOnly` so designers can "
                "wire it in editor (W26 L-01)."
            )

    # Layer 2 — Base game mode runtime assertion in OnExperienceAssetReady.
    gm_base_cpp = private_root / "Framework/PaldarkGameModeBase.cpp"
    if not gm_base_cpp.is_file():
        fail(
            f"missing {gm_base_cpp.relative_to(REPO_ROOT)} "
            "(W26 L-01 — required for runtime assertion site)."
        )
    else:
        text = gm_base_cpp.read_text(encoding="utf-8")
        # The check must happen inside OnExperienceAssetReady, so confirm both
        # the method and the assertion symbols are present.
        if "OnExperienceAssetReady" not in text:
            fail(
                f"{gm_base_cpp.relative_to(REPO_ROOT)} must define "
                "OnExperienceAssetReady (W26 L-01 — assertion site)."
            )
        for symbol in (
            "RequiredGameModeClass",
            "IsChildOf",
            "AddOnScreenDebugMessage",
            "FColor::Red",
        ):
            if symbol not in text:
                fail(
                    f"{gm_base_cpp.relative_to(REPO_ROOT)} is missing required "
                    f"symbol {symbol!r} (W26 L-01 — RequiredGameModeClass "
                    "assertion + on-screen warning)."
                )
        # Must log at Error verbosity so the message is not lost in the noise
        # of Log/Verbose categories during PIE startup.
        if not re.search(
            r"UE_LOG\s*\(\s*LogPaldark\s*,\s*Error",
            text,
        ):
            fail(
                f"{gm_base_cpp.relative_to(REPO_ROOT)} must log the "
                "RequiredGameModeClass mismatch via UE_LOG(LogPaldark, Error, ...) "
                "(W26 L-01 — Warning/Log would be swallowed during PIE startup)."
            )

    # Layer 3 — DefaultEngine.ini ships the URL-alias fallback.
    ini_path = PROJECT_DIR / "Config/DefaultEngine.ini"
    if not ini_path.is_file():
        fail(
            f"missing {ini_path.relative_to(REPO_ROOT)} "
            "(W26 L-01 — required for +GameModeClassAliases fallback)."
        )
    else:
        ini_text = ini_path.read_text(encoding="utf-8")
        # The alias line itself.
        if not re.search(
            r"\+GameModeClassAliases\s*=\s*\(\s*Name\s*=\s*\"Extraction\"\s*,\s*"
            r"GameMode\s*=\s*\"/Script/PaldarkLab\.PaldarkGameMode_Extraction\"\s*\)",
            ini_text,
        ):
            fail(
                f"{ini_path.relative_to(REPO_ROOT)} must declare "
                "`+GameModeClassAliases=(Name=\"Extraction\",GameMode=\""
                "/Script/PaldarkLab.PaldarkGameMode_Extraction\")` under "
                "[/Script/EngineSettings.GameMapsSettings] (W26 L-01 — "
                "URL-flag fallback so designers can launch the extraction "
                "GameMode without per-map World Override)."
            )
        # The alias must live in the GameMapsSettings section so UE picks it up.
        if not re.search(
            r"\[/Script/EngineSettings\.GameMapsSettings\][^\[]*"
            r"\+GameModeClassAliases\s*=\s*\(\s*Name\s*=\s*\"Extraction\"",
            ini_text,
            flags=re.DOTALL,
        ):
            fail(
                f"{ini_path.relative_to(REPO_ROOT)} must place the "
                "Extraction +GameModeClassAliases entry inside the "
                "[/Script/EngineSettings.GameMapsSettings] section "
                "(W26 L-01 — UE only reads aliases from that section)."
            )


def check_required_game_features_wired() -> None:
    """W39 — L-21 closure: enforce three-layer defense so designers can't
    silently boot an experience that depends on a Game Feature plugin
    which isn't enabled in `PaldarkLab.uproject`.

    Three layers (any one missing breaks the safety net):

    1. `UPaldarkExperienceDefinition` declares a `RequiredGameFeatures`
       field (`TArray<FString>`, EditDefaultsOnly) — designer authors
       per-experience the list of feature-plugin names that must be Active.
    2. `APaldarkGameModeBase::OnExperienceAssetReady` iterates the field,
       resolves each plugin via `IPluginManager::FindPlugin` +
       `UGameFeaturesSubsystem::GetPluginURLByName`, asserts state ==
       Active, logs Error + on-screen red banner per mismatch.
    3. `PaldarkLab.Build.cs` lists `GameFeatures` + `Projects` so the
       implementation compiles (UGameFeaturesSubsystem lives in
       GameFeatures module; IPluginManager in Projects).
    """
    public_root = PROJECT_DIR / "Source" / "PaldarkLab" / "Public"
    private_root = PROJECT_DIR / "Source" / "PaldarkLab" / "Private"

    # Layer 1 — Experience definition declares the RequiredGameFeatures field.
    exp_header = public_root / "Experience/PaldarkExperienceDefinition.h"
    if not exp_header.is_file():
        fail(
            f"missing {exp_header.relative_to(REPO_ROOT)} "
            "(W39 L-21 — experience definition required for "
            "RequiredGameFeatures assertion)."
        )
    else:
        text = exp_header.read_text(encoding="utf-8")
        if not re.search(
            r"TArray\s*<\s*FString\s*>\s+RequiredGameFeatures\s*;",
            text,
        ):
            fail(
                f"{exp_header.relative_to(REPO_ROOT)} must declare "
                "`TArray<FString> RequiredGameFeatures;` (W39 L-21 — "
                "Game Feature plugin assertion field). Plain FString "
                "because UGameFeaturesSubsystem keys plugins by name "
                "resolved via GetPluginURLByName."
            )
        # UPROPERTY decoration so the designer can author it in editor.
        if re.search(r"RequiredGameFeatures", text) and not re.search(
            r"UPROPERTY\([^)]*EditDefaultsOnly[^)]*\)\s*\n\s*TArray\s*<\s*FString\s*>\s+RequiredGameFeatures",
            text,
        ):
            fail(
                f"{exp_header.relative_to(REPO_ROOT)} must mark "
                "RequiredGameFeatures `EditDefaultsOnly` so designers "
                "can wire it in editor (W39 L-21)."
            )

    # Layer 2 — Base game mode runtime assertion in OnExperienceAssetReady.
    gm_base_cpp = private_root / "Framework/PaldarkGameModeBase.cpp"
    if not gm_base_cpp.is_file():
        fail(
            f"missing {gm_base_cpp.relative_to(REPO_ROOT)} "
            "(W39 L-21 — required for runtime assertion site)."
        )
    else:
        text = gm_base_cpp.read_text(encoding="utf-8")
        if "OnExperienceAssetReady" not in text:
            fail(
                f"{gm_base_cpp.relative_to(REPO_ROOT)} must define "
                "OnExperienceAssetReady (W39 L-21 — assertion site)."
            )
        # The subsystem + plugin manager headers must be included so the
        # assertion compiles.
        for include_line in (
            "#include \"GameFeaturesSubsystem.h\"",
            "#include \"Interfaces/IPluginManager.h\"",
        ):
            if include_line not in text:
                fail(
                    f"{gm_base_cpp.relative_to(REPO_ROOT)} must include "
                    f"`{include_line}` (W39 L-21 — RequiredGameFeatures "
                    "assertion needs UGameFeaturesSubsystem + "
                    "IPluginManager APIs)."
                )
        # Core symbols of the assertion: iterate the field, resolve plugin,
        # check Active state, log Error + on-screen red banner.
        for symbol in (
            "RequiredGameFeatures",
            "UGameFeaturesSubsystem",
            "GetPluginURLByName",
            "IPluginManager",
            "FindPlugin",
            "EGameFeaturePluginState::Active",
            "AddOnScreenDebugMessage",
            "FColor::Red",
        ):
            if symbol not in text:
                fail(
                    f"{gm_base_cpp.relative_to(REPO_ROOT)} is missing "
                    f"required symbol {symbol!r} (W39 L-21 — "
                    "RequiredGameFeatures assertion + on-screen warning)."
                )
        # The mismatch must log at Error verbosity so it surfaces above the
        # noise of Log/Verbose categories during PIE startup.
        if not re.search(
            r"UE_LOG\s*\(\s*LogPaldark\s*,\s*Error[^)]*RequiredGameFeatures|"
            r"requires Game Feature plugin",
            text,
        ):
            fail(
                f"{gm_base_cpp.relative_to(REPO_ROOT)} must log the "
                "RequiredGameFeatures mismatch via UE_LOG(LogPaldark, "
                "Error, ...) (W39 L-21 — Warning/Log would be swallowed "
                "during PIE startup)."
            )

    # Layer 3 — Build.cs lists the modules the assertion depends on.
    build_cs = PROJECT_DIR / "Source" / "PaldarkLab" / "PaldarkLab.Build.cs"
    if not build_cs.is_file():
        fail(
            f"missing {build_cs.relative_to(REPO_ROOT)} "
            "(W39 L-21 — required to verify GameFeatures + Projects "
            "module dependencies)."
        )
    else:
        text = build_cs.read_text(encoding="utf-8")
        for required in ("GameFeatures", "Projects"):
            if f"\"{required}\"" not in text:
                fail(
                    f"{build_cs.relative_to(REPO_ROOT)} must list "
                    f"`\"{required}\"` in PublicDependencyModuleNames "
                    f"(W39 L-21 — {required} provides "
                    f"{'UGameFeaturesSubsystem' if required == 'GameFeatures' else 'IPluginManager'})."
                )


def check_anim_instance_shape() -> None:
    """W27 closure — Q1 K-01 / Q2 L-06 (AnimBP T-pose risk) code-side.

    Enforce that the W27 AnimBP Lyra ALS port shipped:

    1. `UPaldarkAnimInstance` (Public/Anim/PaldarkAnimInstance.h) exposes the
       Lyra-style AnimGraph-facing property fields (Speed / Direction /
       bIsInAir / bIsAccelerating / bIsCrouching / bIsSprinting / bShouldMove)
       as `UPROPERTY(BlueprintReadOnly)` so designer State Machine + Blend
       Space nodes can read them.
    2. The same header declares the `NativeThreadSafeUpdateAnimation` override
       (parallel anim graph eval requires it; without the override every value
       is read from a stale snapshot or, worse, from a live UObject on a worker
       thread).
    3. `UPaldarkPalAnimInstance` (Public/Anim/PaldarkPalAnimInstance.h) exposes
       the Pal-specific fields (bIsHostile / bIsFlying / bIsInCombat /
       CurrentActivityTag) so the hostile-Pal AnimBP can branch.
    4. `UPaldarkPawnData::DefaultAnimInstanceClass` exists as a soft pointer to
       `UPaldarkAnimInstance` — so designer can swap the AnimBP per experience
       without rebuilding the pawn BP.
    5. `APaldarkCharacter::SetPawnData` actually applies the soft class via
       `Mesh->SetAnimInstanceClass(...)` — otherwise the PawnData field is
       silently inert.
    """
    src_root = PROJECT_DIR / "Source" / "PaldarkLab"

    base_header      = src_root / "Public" / "Anim" / "PaldarkAnimInstance.h"
    pal_header       = src_root / "Public" / "Anim" / "PaldarkPalAnimInstance.h"
    interface_header = src_root / "Public" / "Anim" / "PaldarkAnimLayerInterface.h"
    pawn_data        = src_root / "Public" / "Experience" / "PaldarkPawnData.h"
    player_cpp       = src_root / "Private" / "Player" / "PaldarkCharacter.cpp"

    # (0) Layer interface exists and declares the 3 layer functions designer
    # AnimBPs use to override per-character poses. Header-only (UINTERFACE);
    # no .cpp required.
    if not interface_header.is_file():
        fail(
            f"missing {interface_header.relative_to(REPO_ROOT)} "
            "(W27 — Lyra-style anim layer interface; designer-authored AnimBPs "
            "implement Layer_Locomotion{Idle,Move,Jump} on their character "
            "Blueprints to override the base State Machine poses)."
        )
    else:
        text = interface_header.read_text(encoding="utf-8")
        if "UINTERFACE" not in text:
            fail(
                f"{interface_header.relative_to(REPO_ROOT)} must use the "
                "`UINTERFACE` macro (W27 — anim layer is a Blueprint interface, "
                "not a UCLASS)."
            )
        for fn in ("Layer_LocomotionIdle", "Layer_LocomotionMove", "Layer_LocomotionJump"):
            if fn not in text:
                fail(
                    f"{interface_header.relative_to(REPO_ROOT)} is missing the "
                    f"`{fn}()` declaration "
                    "(W27 — each of the three locomotion layers must be a "
                    "`BlueprintImplementableEvent`)."
                )

    # (1) Base AnimInstance — Lyra-style AnimGraph property fields.
    if base_header.is_file():
        text = base_header.read_text(encoding="utf-8")
        required_props = [
            ("float Speed",          r"\bfloat\s+Speed\b"),
            ("float Direction",      r"\bfloat\s+Direction\b"),
            ("bool bIsInAir",        r"\bbool\s+bIsInAir\b"),
            ("bool bIsAccelerating", r"\bbool\s+bIsAccelerating\b"),
            ("bool bIsCrouching",    r"\bbool\s+bIsCrouching\b"),
            ("bool bIsSprinting",    r"\bbool\s+bIsSprinting\b"),
            ("bool bShouldMove",     r"\bbool\s+bShouldMove\b"),
        ]
        for label, pattern in required_props:
            if not re.search(pattern, text):
                fail(
                    f"{base_header.relative_to(REPO_ROOT)} is missing the "
                    f"`{label}` property on UPaldarkAnimInstance "
                    "(W27 — AnimGraph reads this field). All AnimGraph-facing "
                    "fields must remain `UPROPERTY(BlueprintReadOnly, Transient)`."
                )

        # (2) NativeThreadSafeUpdateAnimation override declaration.
        if not re.search(
            r"NativeThreadSafeUpdateAnimation\s*\(\s*float\s+\w+\s*\)\s*override",
            text,
        ):
            fail(
                f"{base_header.relative_to(REPO_ROOT)} must declare "
                "`virtual void NativeThreadSafeUpdateAnimation(float "
                "DeltaSeconds) override;` (W27 — parallel anim graph eval "
                "requires the thread-safe pass; without it Speed/Direction "
                "would either be stale or read on the worker from a live "
                "UObject)."
            )
        if "FPaldarkAnimSnapshot" not in text:
            fail(
                f"{base_header.relative_to(REPO_ROOT)} must define "
                "`FPaldarkAnimSnapshot` (W27 — game-thread snapshot consumed "
                "by NativeThreadSafeUpdateAnimation)."
            )

    # (3) Pal subclass — Pal-specific AnimGraph fields.
    if pal_header.is_file():
        text = pal_header.read_text(encoding="utf-8")
        if not re.search(
            r"class\s+\w*\s*UPaldarkPalAnimInstance\s*:\s*public\s+UPaldarkAnimInstance",
            text,
        ):
            fail(
                f"{pal_header.relative_to(REPO_ROOT)} must declare "
                "`class UPaldarkPalAnimInstance : public UPaldarkAnimInstance` "
                "(W27 — Pal AnimInstance inherits the Lyra-style base so the "
                "designer-authored hostile-Pal AnimBP gets the shared "
                "Speed / Direction / bShouldMove fields for free)."
            )
        pal_required = [
            ("bool bIsHostile",  r"\bbool\s+bIsHostile\b"),
            ("bool bIsFlying",   r"\bbool\s+bIsFlying\b"),
            ("bool bIsInCombat", r"\bbool\s+bIsInCombat\b"),
            ("FGameplayTag CurrentActivityTag",
             r"FGameplayTag\s+CurrentActivityTag\b"),
        ]
        for label, pattern in pal_required:
            if not re.search(pattern, text):
                fail(
                    f"{pal_header.relative_to(REPO_ROOT)} is missing the "
                    f"`{label}` property on UPaldarkPalAnimInstance "
                    "(W27 — hostile-Pal AnimBP branches on this)."
                )

    # (4) PawnData soft class slot for the AnimInstance.
    if pawn_data.is_file():
        text = pawn_data.read_text(encoding="utf-8")
        if not re.search(
            r"TSoftClassPtr<\s*UPaldarkAnimInstance\s*>\s*DefaultAnimInstanceClass\b",
            text,
        ):
            fail(
                f"{pawn_data.relative_to(REPO_ROOT)} must expose "
                "`TSoftClassPtr<UPaldarkAnimInstance> DefaultAnimInstanceClass` "
                "(W27 — without it designer cannot swap the AnimBP per "
                "experience without re-authoring the pawn BP)."
            )

    # (5) Player character actually calls SetAnimInstanceClass from PawnData.
    if player_cpp.is_file():
        text = player_cpp.read_text(encoding="utf-8")
        if not re.search(
            r"DefaultAnimInstanceClass[\s\S]{0,600}?SetAnimInstanceClass\s*\(",
            text,
        ):
            fail(
                f"{player_cpp.relative_to(REPO_ROOT)} must apply "
                "`PawnData->DefaultAnimInstanceClass` to the mesh via "
                "`SetAnimInstanceClass(...)` (W27 — otherwise the soft class "
                "field on PawnData is silently inert and the pawn stays in "
                "T-pose until the BP-side `Mesh.AnimClass` is set manually). "
                "Wire it in `SetPawnData` or an equivalent possess hook."
            )


def check_pal_definition_shape() -> None:
    """W27-28 — PaldarkPalDefinition primary asset shape.

    UPaldarkPalDefinition is the data-side handle the spawn subsystem keys off.
    The shape contract:

    1. Header lives at Public/Pal/PaldarkPalDefinition.h with a matching .cpp.
    2. Header declares `class PALDARKLAB_API UPaldarkPalDefinition : public
       UPrimaryDataAsset` — primary-asset registration in DefaultGame.ini
       relies on the parent class to find the type via reflection.
    3. Header exposes the soft refs grouped into the "Spawn" asset bundle:
       PalClass / SkeletalMesh / AnimInstanceClass / GrantedAbilities. Each
       UPROPERTY must carry `meta = (AssetBundles = "Spawn")` so
       `UPaldarkPalSpawnSubsystem::RequestPreWarmAsync` pulls them via the
       streamable manager.
    4. .cpp overrides `GetPrimaryAssetId()` returning a
       `FPrimaryAssetType("PaldarkPalDefinition")` id — DefaultGame.ini's
       `PrimaryAssetTypesToScan` row must use the SAME type string.
    5. DefaultGame.ini has a `+PrimaryAssetTypesToScan=` row for
       `PaldarkPalDefinition` pointing at `/Game/Paldark/Pals`.
    6. UPaldarkExperienceDefinition exposes
       `TArray<TSoftObjectPtr<UPaldarkPalDefinition>> PreWarmPalDefinitions`
       so designers can mark which species the spawn subsystem should
       pre-warm on experience load.
    """
    src_root = PROJECT_DIR / "Source" / "PaldarkLab"

    header = src_root / "Public" / "Pal" / "PaldarkPalDefinition.h"
    impl   = src_root / "Private" / "Pal" / "PaldarkPalDefinition.cpp"
    experience_header = src_root / "Public" / "Experience" / "PaldarkExperienceDefinition.h"
    default_game_ini  = PROJECT_DIR / "Config" / "DefaultGame.ini"

    # (1) header + .cpp present.
    if not header.is_file():
        fail(
            f"missing {header.relative_to(REPO_ROOT)} "
            "(W27-28 — UPaldarkPalDefinition is the Pal species primary data "
            "asset; spawn subsystem cannot pre-warm anything without it)."
        )
        return
    if not impl.is_file():
        fail(
            f"missing {impl.relative_to(REPO_ROOT)} "
            "(W27-28 — UPaldarkPalDefinition needs a .cpp to host the "
            "`GetPrimaryAssetId` override)."
        )

    text = header.read_text(encoding="utf-8")

    # (2) UCLASS shape — derive from UPrimaryDataAsset.
    if not re.search(
        r"class\s+PALDARKLAB_API\s+UPaldarkPalDefinition\s*:\s*public\s+UPrimaryDataAsset",
        text,
    ):
        fail(
            f"{header.relative_to(REPO_ROOT)} must declare "
            "`class PALDARKLAB_API UPaldarkPalDefinition : public "
            "UPrimaryDataAsset` (W27-28 — primary-asset scan + bundle "
            "membership both rely on the UPrimaryDataAsset base)."
        )

    # (3) Soft refs + AssetBundles="Spawn" metadata.
    required_soft_refs = [
        ("TSoftClassPtr<APaldarkPalCharacter> PalClass",
         r"TSoftClassPtr<\s*APaldarkPalCharacter\s*>\s*PalClass\b"),
        ("TSoftObjectPtr<USkeletalMesh> SkeletalMesh",
         r"TSoftObjectPtr<\s*USkeletalMesh\s*>\s*SkeletalMesh\b"),
        ("TSoftClassPtr<UPaldarkAnimInstance> AnimInstanceClass",
         r"TSoftClassPtr<\s*UPaldarkAnimInstance\s*>\s*AnimInstanceClass\b"),
        ("TArray<TSoftClassPtr<UGameplayAbility>> GrantedAbilities",
         r"TArray<\s*TSoftClassPtr<\s*UGameplayAbility\s*>\s*>\s*GrantedAbilities\b"),
    ]
    for label, pattern in required_soft_refs:
        if not re.search(pattern, text):
            fail(
                f"{header.relative_to(REPO_ROOT)} is missing "
                f"`{label}` (W27-28 — required for the \"Spawn\" asset bundle)."
            )

    # Each soft ref must be tagged AssetBundles="Spawn" so the streamable
    # manager pulls them when the subsystem pre-warms.
    spawn_bundle_hits = re.findall(
        r"AssetBundles\s*=\s*\"Spawn\"",
        text,
    )
    if len(spawn_bundle_hits) < 4:
        fail(
            f"{header.relative_to(REPO_ROOT)} must tag PalClass, SkeletalMesh, "
            "AnimInstanceClass, and GrantedAbilities with "
            "`meta = (AssetBundles = \"Spawn\")` (W27-28 — bundle membership "
            "is what makes RequestPreWarmAsync actually stream the per-species "
            "payload). Got "
            f"{len(spawn_bundle_hits)}/4 matches."
        )

    # (4) .cpp overrides GetPrimaryAssetId returning the PaldarkPalDefinition type.
    if impl.is_file():
        impl_text = impl.read_text(encoding="utf-8")
        if not re.search(
            r"FPrimaryAssetType\s*\(\s*TEXT\s*\(\s*\"PaldarkPalDefinition\"\s*\)\s*\)",
            impl_text,
        ):
            fail(
                f"{impl.relative_to(REPO_ROOT)} must return a primary asset id "
                "with `FPrimaryAssetType(TEXT(\"PaldarkPalDefinition\"))` "
                "(W27-28 — the type string MUST match the "
                "DefaultGame.ini PrimaryAssetTypesToScan row)."
            )

    # (5) DefaultGame.ini wiring.
    if default_game_ini.is_file():
        ini_text = default_game_ini.read_text(encoding="utf-8")
        if not re.search(
            r"PrimaryAssetType\s*=\s*\"PaldarkPalDefinition\"",
            ini_text,
        ):
            fail(
                f"{default_game_ini.relative_to(REPO_ROOT)} is missing a "
                "`+PrimaryAssetTypesToScan=(PrimaryAssetType=\"PaldarkPalDefinition\"...)` "
                "row (W27-28 — without it the editor's Asset Manager UI + "
                "runtime scanner will skip the Pals directory)."
            )
        if "/Game/Paldark/Pals" not in ini_text:
            fail(
                f"{default_game_ini.relative_to(REPO_ROOT)} must point the "
                "PaldarkPalDefinition row at `/Game/Paldark/Pals` "
                "(W27-28 — this is the convention the spawn subsystem "
                "console help text references)."
            )

    # (6) Experience exposes PreWarmPalDefinitions.
    if experience_header.is_file():
        exp_text = experience_header.read_text(encoding="utf-8")
        if not re.search(
            r"TArray<\s*TSoftObjectPtr<\s*UPaldarkPalDefinition\s*>\s*>\s*PreWarmPalDefinitions\b",
            exp_text,
        ):
            fail(
                f"{experience_header.relative_to(REPO_ROOT)} must expose "
                "`TArray<TSoftObjectPtr<UPaldarkPalDefinition>> "
                "PreWarmPalDefinitions` (W27-28 — without it the experience "
                "cannot opt-in any species for the async pre-warm pipeline)."
            )

    # (7) Q3 Bug-Bash L-28 — boss phase HP thresholds expose.
    #
    # `UPaldarkPalDefinition` must declare a `TArray<float> PhaseHealthThresholds`
    # UPROPERTY so the Boltmane (and any future) boss-phase activity reads
    # transition thresholds per-species from the DA rather than from
    # `UPaldarkActivity_BossPhase`'s class-CDO defaults. The field is
    # designer-tunable (EditDefaultsOnly) and the L-28 contract is
    # compile-stable; the validator does NOT police whether the
    # `DA_PalDef_*` row actually populates the array (designer task, like
    # L-25 / L-27).
    if not re.search(
        r"TArray<\s*float\s*>\s*PhaseHealthThresholds\b",
        text,
    ):
        fail(
            f"{header.relative_to(REPO_ROOT)} must declare "
            "`TArray<float> PhaseHealthThresholds` (Q3 Bug-Bash L-28 — "
            "per-species boss phase HP transition thresholds; read by "
            "`UPaldarkActivity_BossPhase::HydrateThresholdsFromPalDefinition` "
            "on activity enter. Without the field designers must subclass "
            "the activity per boss to retune, which W29-30 already documented "
            "as an anti-pattern)."
        )
    elif "EditDefaultsOnly" not in text.split("PhaseHealthThresholds")[0].split(
        "UPROPERTY"
    )[-1]:
        fail(
            f"{header.relative_to(REPO_ROOT)} `PhaseHealthThresholds` UPROPERTY "
            "must be `EditDefaultsOnly` (Q3 Bug-Bash L-28 — must be tunable "
            "on the DA_PalDef_<Species> CDO so per-species values stick "
            "across cooks; EditAnywhere would let a runtime instance drift "
            "from the DA, EditInstanceOnly would hide it from the DA editor)."
        )


def check_pal_spawn_subsystem() -> None:
    """W27-28 — UPaldarkPalSpawnSubsystem shape.

    The spawn subsystem owns the FStreamableManager-backed async pipeline.
    The structural contract:

    1. Header lives at Public/Pal/PaldarkPalSpawnSubsystem.h with a matching .cpp.
    2. Declares `class PALDARKLAB_API UPaldarkPalSpawnSubsystem : public
       UWorldSubsystem` — world-scoped because the warm cache must die with
       the map.
    3. Declares the 3 public surfaces designers (and console commands) hit:
       `RequestPreWarmAsync`, `SpawnPalAsync`, `IsPreWarmed`, plus
       `FindWarmedDefinition` (used by `Paldark.Pal.DumpDefinitionRegistry`).
    4. Declares the two delegate signatures the API exposes
       (`FOnPalPreWarmComplete`, `FOnPalSpawnedAsync`).
    5. .cpp actually wires the load via
       `UAssetManager::Get().LoadPrimaryAsset(...)` AND keeps the
       resulting `TSharedPtr<FStreamableHandle>` alive (coalesces under
       `WarmStates`).
    6. APaldarkGameModeBase::OnExperienceAssetReady calls
       `RequestPalDefinitionPreWarm` so the pre-warm fans out automatically
       on experience load (otherwise the field on UPaldarkExperienceDefinition
       is silently inert).
    """
    src_root = PROJECT_DIR / "Source" / "PaldarkLab"

    header = src_root / "Public" / "Pal" / "PaldarkPalSpawnSubsystem.h"
    impl   = src_root / "Private" / "Pal" / "PaldarkPalSpawnSubsystem.cpp"
    game_mode_cpp = src_root / "Private" / "Framework" / "PaldarkGameModeBase.cpp"

    # (1) header + .cpp present.
    if not header.is_file():
        fail(
            f"missing {header.relative_to(REPO_ROOT)} "
            "(W27-28 — UPaldarkPalSpawnSubsystem is the async load + spawn "
            "service; without it RequestPreWarmAsync has nowhere to live)."
        )
        return
    if not impl.is_file():
        fail(
            f"missing {impl.relative_to(REPO_ROOT)} "
            "(W27-28 — UPaldarkPalSpawnSubsystem needs a .cpp for the "
            "FStreamableManager binding)."
        )

    text = header.read_text(encoding="utf-8")

    # (2) UCLASS base.
    if not re.search(
        r"class\s+PALDARKLAB_API\s+UPaldarkPalSpawnSubsystem\s*:\s*public\s+UWorldSubsystem",
        text,
    ):
        fail(
            f"{header.relative_to(REPO_ROOT)} must declare "
            "`class PALDARKLAB_API UPaldarkPalSpawnSubsystem : public "
            "UWorldSubsystem` (W27-28 — world-scoped so the warm cache "
            "tears down with the level)."
        )

    # (3) Public surface methods.
    for sig in (
        r"void\s+RequestPreWarmAsync\s*\(",
        r"void\s+SpawnPalAsync\s*\(",
        r"bool\s+IsPreWarmed\s*\(",
        r"FindWarmedDefinition\s*\(",
        r"GetTrackedDefinitionIds\s*\(",
    ):
        if not re.search(sig, text):
            fail(
                f"{header.relative_to(REPO_ROOT)} is missing the "
                f"`{sig}` declaration "
                "(W27-28 — console commands + experience fan-out call this)."
            )

    # (4) Delegate signatures.
    for delegate_name in ("FOnPalPreWarmComplete", "FOnPalSpawnedAsync"):
        if delegate_name not in text:
            fail(
                f"{header.relative_to(REPO_ROOT)} is missing the "
                f"`{delegate_name}` delegate declaration "
                "(W27-28 — RequestPreWarmAsync / SpawnPalAsync use these as "
                "completion callbacks)."
            )

    # (5) .cpp uses FStreamableManager via UAssetManager.
    if impl.is_file():
        impl_text = impl.read_text(encoding="utf-8")
        if "LoadPrimaryAsset" not in impl_text:
            fail(
                f"{impl.relative_to(REPO_ROOT)} must call "
                "`UAssetManager::Get().LoadPrimaryAsset(...)` "
                "(W27-28 — that is the only API that resolves the \"Spawn\" "
                "asset bundle through the streamable manager)."
            )
        if "TSharedPtr<FStreamableHandle>" not in impl_text and "FStreamableHandle" not in impl_text:
            fail(
                f"{impl.relative_to(REPO_ROOT)} must store "
                "`TSharedPtr<FStreamableHandle>` for every in-flight + warm "
                "request (W27-28 — without holding the handle, the loaded "
                "sub-objects drift to GC immediately after the callback fires)."
            )

    # (6) Game mode wires the experience pre-warm fan-out.
    if game_mode_cpp.is_file():
        gm_text = game_mode_cpp.read_text(encoding="utf-8")
        if "RequestPalDefinitionPreWarm" not in gm_text:
            fail(
                f"{game_mode_cpp.relative_to(REPO_ROOT)} must call "
                "`RequestPalDefinitionPreWarm(*Experience)` from "
                "OnExperienceAssetReady (W27-28 — without it the "
                "PreWarmPalDefinitions array on the experience is silently "
                "inert and SpawnPalAsync pays the streaming cost on every "
                "first-spawn)."
            )


def check_game_feature_plugin() -> None:
    """W27-28 — PaldarkRaidContent game-feature plugin scaffold.

    Game-feature plugins ship raid content (maps, Pal definitions, ability
    sets) as a streamable feature package. The structural contract:

    1. `Plugins/PaldarkRaidContent/PaldarkRaidContent.uplugin` exists and
       parses as JSON.
    2. The manifest sets `"IsGameFeaturePlugin": true` and declares exactly
       one module named `PaldarkRaidContent` of type `Runtime`. (Lyra-style
       feature plugins keep the module name == plugin name.)
    3. The manifest depends on the `GameFeatures` + `ModularGameplay` engine
       plugins (the feature lifecycle services).
    4. `Plugins/PaldarkRaidContent/Source/PaldarkRaidContent/` ships the
       module skeleton: `*.Build.cs` + `Public/PaldarkRaidContent.h` +
       `Private/PaldarkRaidContent.cpp` (with `IMPLEMENT_MODULE`).
    5. `Plugins/PaldarkRaidContent/Content/GameFeatures/` directory exists so
       designers can drop the `GameFeatureData_RaidContent.uasset` there.
    6. The host project's `.uproject` registers PaldarkRaidContent in the
       Plugins block (regardless of whether `Enabled` is true or false —
       presence here is what surfaces the plugin in the Editor's Plugins UI).
    7. The host project enables the `GameFeatures` + `ModularGameplay` engine
       plugins in its `.uproject` so the runtime services exist when the
       feature plugin tries to register itself.
    """
    plugin_root = PROJECT_DIR / "Plugins" / "PaldarkRaidContent"
    uplugin_path = plugin_root / "PaldarkRaidContent.uplugin"
    module_root  = plugin_root / "Source" / "PaldarkRaidContent"
    build_cs     = module_root / "PaldarkRaidContent.Build.cs"
    header       = module_root / "Public" / "PaldarkRaidContent.h"
    impl         = module_root / "Private" / "PaldarkRaidContent.cpp"
    content_dir  = plugin_root / "Content" / "GameFeatures"
    uproject     = PROJECT_DIR / "PaldarkLab.uproject"

    # (1) manifest exists + parses.
    if not uplugin_path.is_file():
        fail(
            f"missing {uplugin_path.relative_to(REPO_ROOT)} "
            "(W27-28 — game-feature plugin manifest)."
        )
        return
    try:
        manifest = json.loads(uplugin_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        fail(
            f"{uplugin_path.relative_to(REPO_ROOT)} did not parse as JSON: {exc} "
            "(W27-28)."
        )
        return

    # (2) IsGameFeaturePlugin true + exactly one module of the right shape.
    if not manifest.get("IsGameFeaturePlugin", False):
        fail(
            f"{uplugin_path.relative_to(REPO_ROOT)} must set "
            "`\"IsGameFeaturePlugin\": true` (W27-28 — without this the "
            "Game Features Subsystem will not pick the plugin up)."
        )
    modules = manifest.get("Modules", [])
    if len(modules) != 1:
        fail(
            f"{uplugin_path.relative_to(REPO_ROOT)} should declare exactly 1 "
            f"module (got {len(modules)}). W27-28 keeps the feature plugin "
            "single-module on purpose; multi-module feature plugins are "
            "rarely necessary."
        )
    elif modules[0].get("Name") != "PaldarkRaidContent" or modules[0].get("Type") != "Runtime":
        fail(
            f"{uplugin_path.relative_to(REPO_ROOT)} module entry must be "
            "`Name=PaldarkRaidContent, Type=Runtime` (W27-28 — module name "
            "must match the plugin name for the cooker to find the .uasset "
            "package)."
        )

    # (3) GameFeatures + ModularGameplay engine plugin dependencies.
    plugin_deps = {entry.get("Name") for entry in manifest.get("Plugins", []) if isinstance(entry, dict)}
    for required in ("GameFeatures", "ModularGameplay"):
        if required not in plugin_deps:
            fail(
                f"{uplugin_path.relative_to(REPO_ROOT)} must list `{required}` "
                "in its Plugins block (W27-28 — required engine service for "
                "the feature lifecycle)."
            )

    # (4) Module skeleton.
    for needed in (build_cs, header, impl):
        if not needed.is_file():
            fail(
                f"missing {needed.relative_to(REPO_ROOT)} "
                "(W27-28 — PaldarkRaidContent runtime module skeleton "
                "incomplete)."
            )
    if impl.is_file():
        impl_text = impl.read_text(encoding="utf-8")
        if "IMPLEMENT_MODULE" not in impl_text:
            fail(
                f"{impl.relative_to(REPO_ROOT)} must call `IMPLEMENT_MODULE("
                "FPaldarkRaidContentModule, PaldarkRaidContent)` (W27-28 — "
                "without it Unreal will not load the runtime module when the "
                "feature activates)."
            )

    # (5) Content/GameFeatures directory placeholder.
    if not content_dir.is_dir():
        fail(
            f"missing {content_dir.relative_to(REPO_ROOT)} "
            "(W27-28 — designer drops `GameFeatureData_RaidContent.uasset` "
            "here; presence of the directory keeps the path stable across "
            "git checkouts)."
        )

    # (6) + (7) host .uproject registers the plugin + enables the engine deps.
    if uproject.is_file():
        try:
            project_manifest = json.loads(uproject.read_text(encoding="utf-8"))
        except json.JSONDecodeError as exc:
            fail(
                f"{uproject.relative_to(REPO_ROOT)} did not parse as JSON: "
                f"{exc} (W27-28)."
            )
            return
        project_plugins = {entry.get("Name"): entry for entry in project_manifest.get("Plugins", []) if isinstance(entry, dict)}
        if "PaldarkRaidContent" not in project_plugins:
            fail(
                f"{uproject.relative_to(REPO_ROOT)} must list "
                "`PaldarkRaidContent` in the Plugins block (W27-28 — listing "
                "it is what surfaces the plugin in the Editor's Plugins "
                "UI; `Enabled` may be false until the GameFeatureData asset "
                "is authored)."
            )
        for required in ("GameFeatures", "ModularGameplay"):
            entry = project_plugins.get(required)
            if entry is None or not entry.get("Enabled", False):
                fail(
                    f"{uproject.relative_to(REPO_ROOT)} must enable the "
                    f"`{required}` engine plugin (W27-28 — required for "
                    "the feature lifecycle services the PaldarkRaidContent "
                    "plugin sits on top of)."
                )

    # (8) Q3 Bug-Bash L-34 — dedicated-server auto-activate contract.
    #
    # The dedicated-server build (`PaldarkLabServer` target) does NOT
    # always drive a feature plugin to `Active` state by the time
    # `UGameInstance::Init` returns, even if the plugin is listed in
    # the .uproject Plugins[]. Without auto-activate, the server boots
    # `Raid_Sandbox` with `PaldarkRaidContent` registered but inert —
    # no `DA_PalDef_*`, no raid maps, no raid abilities — and the
    # client sees broken extraction sessions.
    #
    # The fix is twofold:
    #  (a) .uplugin sets `EnabledByDefault: true` +
    #      `BuiltInInitialFeatureState: Active` so the engine drives
    #      the plugin to Active on Editor / standalone / listen-server.
    #  (b) `UPaldarkGameInstance::Init` calls a server-only helper that
    #      iterates discovered feature plugins and force-activates any
    #      stalled below Active via
    #      `UGameFeaturesSubsystem::LoadAndActivateGameFeaturePlugin`.
    #
    # The .uproject's `PaldarkRaidContent.Enabled` must also be `true`
    # (otherwise (a) is moot — the plugin is excluded from the build).
    enabled_by_default = manifest.get("EnabledByDefault")
    if enabled_by_default is not True:
        fail(
            f"{uplugin_path.relative_to(REPO_ROOT)} must set "
            "`\"EnabledByDefault\": true` (Q3 Bug-Bash L-34 — dedicated "
            "server boot relies on this flag so the engine pulls the "
            "plugin into the build target; with it false the plugin is "
            "excluded from cooked builds entirely)."
        )

    initial_state = manifest.get("BuiltInInitialFeatureState")
    if initial_state != "Active":
        fail(
            f"{uplugin_path.relative_to(REPO_ROOT)} must set "
            "`\"BuiltInInitialFeatureState\": \"Active\"` (Q3 Bug-Bash L-34 "
            "— signals the Game Features Subsystem to drive the plugin "
            "to Active automatically on Editor / standalone / listen-server "
            f"boot; got `{initial_state}`)."
        )

    # `bIsServerOnly` is optional + defaults to false. We assert it is
    # not explicitly set to true — that flag would exclude the plugin
    # from client cooks, which breaks listen-server / standalone where
    # the same plugin must run on the host process serving 0..3 remote
    # clients.
    if manifest.get("bIsServerOnly", False):
        fail(
            f"{uplugin_path.relative_to(REPO_ROOT)} must NOT set "
            "`\"bIsServerOnly\": true` (Q3 Bug-Bash L-34 — listen-server "
            "host needs the plugin on both server + client cook of the "
            "same process; `bIsServerOnly` cooks server-only and breaks "
            "the listen-server topology PaldarkLab ships with)."
        )

    # (9) Host .uproject must enable `PaldarkRaidContent` (Enabled=true).
    if uproject.is_file():
        try:
            project_manifest = json.loads(uproject.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            project_manifest = {}
        project_plugins = {
            entry.get("Name"): entry
            for entry in project_manifest.get("Plugins", [])
            if isinstance(entry, dict)
        }
        raid_entry = project_plugins.get("PaldarkRaidContent")
        if raid_entry is not None and not raid_entry.get("Enabled", False):
            fail(
                f"{uproject.relative_to(REPO_ROOT)} must set "
                "`PaldarkRaidContent.Enabled: true` in the Plugins block "
                "(Q3 Bug-Bash L-34 — dedicated server boot path needs the "
                "plugin in the project's enabled set or `EnabledByDefault` "
                "on the .uplugin has no effect)."
            )

    # (10) `UPaldarkGameInstance` implements the L-34 dedicated-server
    # boot hook. The header must declare the helper and the .cpp must
    # invoke it from `Init()` behind an `IsDedicatedServerInstance()`
    # gate, calling `UGameFeaturesSubsystem::LoadAndActivateGameFeaturePlugin`
    # for plugins below `Active`. We pattern-match the symbols rather
    # than parse C++ so the check tolerates whitespace + comment edits.
    gi_header = (
        PROJECT_DIR
        / "Source" / "PaldarkLab" / "Public" / "Framework" / "PaldarkGameInstance.h"
    )
    gi_impl = (
        PROJECT_DIR
        / "Source" / "PaldarkLab" / "Private" / "Framework" / "PaldarkGameInstance.cpp"
    )
    if not gi_header.is_file():
        fail(
            f"missing {gi_header.relative_to(REPO_ROOT)} (Q3 Bug-Bash L-34 — "
            "dedicated server boot hook lives in UPaldarkGameInstance; "
            "header must declare `EnsureFeaturePluginsActiveForDedicatedServer`)."
        )
    else:
        header_text = gi_header.read_text(encoding="utf-8")
        if "EnsureFeaturePluginsActiveForDedicatedServer" not in header_text:
            fail(
                f"{gi_header.relative_to(REPO_ROOT)} must declare "
                "`EnsureFeaturePluginsActiveForDedicatedServer` (Q3 Bug-Bash "
                "L-34 — dedicated server boot hook is named so designers can "
                "grep for it when triaging missing-plugin reports)."
            )

    if not gi_impl.is_file():
        fail(
            f"missing {gi_impl.relative_to(REPO_ROOT)} (Q3 Bug-Bash L-34 — "
            "dedicated server boot hook lives in UPaldarkGameInstance::Init)."
        )
    else:
        impl_text = gi_impl.read_text(encoding="utf-8")
        required_symbols = {
            "IsDedicatedServerInstance": (
                "the boot hook must gate on `IsDedicatedServerInstance()` "
                "so the activation sweep does NOT run on client / listen "
                "/ standalone / Editor PIE — those paths use "
                "`BuiltInInitialFeatureState` and don't need the force"
            ),
            "GameFeaturesSubsystem.h": (
                "the .cpp must include `GameFeaturesSubsystem.h` for the "
                "`UGameFeaturesSubsystem::Get()` API"
            ),
            "IPluginManager": (
                "the boot hook iterates `IPluginManager::Get().GetDiscoveredPlugins()` "
                "to find every feature plugin (rather than hard-coding "
                "`PaldarkRaidContent`); needs the Interfaces/IPluginManager.h include"
            ),
            "EnsureFeaturePluginsActiveForDedicatedServer": (
                "Init() must call the helper or the boot hook is dead code"
            ),
            "LoadAndActivateGameFeaturePlugin": (
                "the recovery path force-drives the plugin to Active via "
                "`UGameFeaturesSubsystem::LoadAndActivateGameFeaturePlugin`"
            ),
            "GetPluginState": (
                "the boot hook must early-out for plugins already in "
                "`EGameFeaturePluginState::Active`; redundant activation "
                "logs spam and may trigger the subsystem's idempotent path"
            ),
            "bIsGameFeaturePlugin": (
                "the iteration filter must check `FPluginDescriptor::bIsGameFeaturePlugin`, "
                "otherwise the helper would attempt to activate regular content plugins"
            ),
            "[L-34]": (
                "log lines must be tagged `[L-34]` so dedicated-server log "
                "scrapes can attribute auto-activate noise to this bug-bash "
                "issue (matches the README W39 § L-34 grep recipe)"
            ),
        }
        for symbol, why in required_symbols.items():
            if symbol not in impl_text:
                fail(
                    f"{gi_impl.relative_to(REPO_ROOT)} must reference "
                    f"`{symbol}` (Q3 Bug-Bash L-34 — {why})."
                )


def check_w29_pal_roster_shape() -> None:
    """W29-30 — 5-Pal roster (5 species + 5 abilities + 3 hostile subclass + boss FSM activity).

    Validates that the C++ scaffold for the W29-30 roster is wired up:

    1. 5 per-species gameplay tags exist in PaldarkGameplayTags.cpp:
       Stoneclad / Vinewraith / Foxparks / Boltmane / Tombat.
    2. 2 per-species pack tags exist for the two hostile species that
       come in packs (Stoneclad + Vinewraith). Boltmane is a solo boss,
       Foxparks/Tombat are companions — no pack tag for them.
    3. 1 boss-phase activity tag exists.
    4. 5 per-species ability tags exist under Paldark.Ability.PalAttack.*.
    5. 5 per-species ability subclass headers + .cpp exist + each
       derives from `UPaldarkGameplayAbility_PalAttack` and registers
       its species ability tag in the ctor.
    6. 3 hostile-Pal subclass headers + .cpp exist (Stoneclad,
       Vinewraith, Boltmane) + each derives from APaldarkPalCharacter +
       stamps the species TeamTag/SpeciesTag in the ctor + assigns
       CandidateActivities directly (NO `.Add` / `.Append` — same rule
       L-13 enforces for Direhound/Razorbird).
    7. Boltmane explicitly REPLACES the Combat activity with
       UPaldarkActivity_BossPhase in its CandidateActivities (the boss
       FSM at P50 preempts plain Combat at P40).
    8. UPaldarkActivity_BossPhase exists, derives from
       UPaldarkBaseActivity, declares the OnBossPhaseChanged delegate +
       phase enum + the standard activity lifecycle overrides, and the
       .cpp sets Priority 50 + binds the boss-phase activity tag.
    """
    public_root  = PROJECT_DIR / "Source" / "PaldarkLab" / "Public"
    private_root = PROJECT_DIR / "Source" / "PaldarkLab" / "Private"
    tags_cpp     = PROJECT_DIR / "Source" / "PaldarkLabCore" / "Private" / "PaldarkGameplayTags.cpp"

    # (1)–(4) Tag definitions.
    tag_text = tags_cpp.read_text(encoding="utf-8") if tags_cpp.is_file() else ""
    for species in ("Stoneclad", "Vinewraith", "Foxparks", "Boltmane", "Tombat"):
        sym = f"TAG_Paldark_Pal_Species_{species}"
        if sym not in tag_text:
            fail(
                f"{tags_cpp.relative_to(REPO_ROOT)} is missing "
                f"`{sym}` (W29-30 — every roster species needs an "
                "identity tag stamped on its Pal subclass / definition)."
            )
    for pack_species in ("Stoneclad", "Vinewraith"):
        sym = f"TAG_Paldark_Pal_Pack_{pack_species}"
        if sym not in tag_text:
            fail(
                f"{tags_cpp.relative_to(REPO_ROOT)} is missing "
                f"`{sym}` (W29-30 — hostile-Pal subclasses with packs "
                "register peers via the pack subsystem)."
            )
    if "TAG_Paldark_Pal_Activity_BossPhase" not in tag_text:
        fail(
            f"{tags_cpp.relative_to(REPO_ROOT)} is missing "
            "`TAG_Paldark_Pal_Activity_BossPhase` (W29-30 — Boltmane's "
            "boss FSM activity needs an identity tag for the activity "
            "component's tag-keyed selection path)."
        )
    for ability in ("Bite", "VineWhip", "Charge", "FireBreath", "ChainLightning"):
        sym = f"TAG_Paldark_Ability_PalAttack_{ability}"
        if sym not in tag_text:
            fail(
                f"{tags_cpp.relative_to(REPO_ROOT)} is missing "
                f"`{sym}` (W29-30 — each W29-30 ability subclass "
                "registers its species tag in its ctor for tag-driven "
                "activation)."
            )

    # (5) Ability subclass scaffolds.
    ability_specs = [
        ("Bite",           "Tombat"),
        ("VineWhip",       "Vinewraith"),
        ("Charge",         "Stoneclad"),
        ("FireBreath",     "Foxparks"),
        ("ChainLightning", "Boltmane"),
    ]
    for ability, _species in ability_specs:
        h = public_root  / "Gas" / "Abilities" / f"PaldarkGameplayAbility_PalAttack_{ability}.h"
        c = private_root / "Gas" / "Abilities" / f"PaldarkGameplayAbility_PalAttack_{ability}.cpp"
        if not h.is_file():
            fail(
                f"missing {h.relative_to(REPO_ROOT)} "
                f"(W29-30 — {ability} ability subclass header)."
            )
            continue
        text = h.read_text(encoding="utf-8")
        class_name = f"UPaldarkGameplayAbility_PalAttack_{ability}"
        if not re.search(
            rf"class\s+PALDARKLAB_API\s+{class_name}\s*:\s*public\s+UPaldarkGameplayAbility_PalAttack",
            text,
        ):
            fail(
                f"{h.relative_to(REPO_ROOT)} must declare "
                f"`class PALDARKLAB_API {class_name} : public "
                "UPaldarkGameplayAbility_PalAttack` (W29-30 — every "
                "per-species ability derives the W18-19 base so the "
                "shared damage path stays in one place)."
            )
        if not c.is_file():
            fail(
                f"missing {c.relative_to(REPO_ROOT)} "
                f"(W29-30 — {ability} ability subclass body)."
            )
            continue
        body = c.read_text(encoding="utf-8")
        ability_tag = f"TAG_Paldark_Ability_PalAttack_{ability}"
        if ability_tag not in body:
            fail(
                f"{c.relative_to(REPO_ROOT)} must add `{ability_tag}` "
                "to AbilityTags in its ctor (W29-30 — designer needs "
                "the per-species tag for tag-driven activation)."
            )

    # (6) Hostile-Pal subclass scaffolds (Stoneclad / Vinewraith / Boltmane).
    hostile_specs = [
        ("Stoneclad",  "TAG_Paldark_Pal_Species_Stoneclad",  "TAG_Paldark_Pal_Pack_Stoneclad"),
        ("Vinewraith", "TAG_Paldark_Pal_Species_Vinewraith", "TAG_Paldark_Pal_Pack_Vinewraith"),
        ("Boltmane",   "TAG_Paldark_Pal_Species_Boltmane",   None),  # solo boss — no pack tag
    ]
    for species, species_tag, pack_tag in hostile_specs:
        h = public_root  / "Pal" / f"PaldarkPalCharacter_{species}.h"
        c = private_root / "Pal" / f"PaldarkPalCharacter_{species}.cpp"
        if not h.is_file():
            fail(
                f"missing {h.relative_to(REPO_ROOT)} "
                f"(W29-30 — {species} hostile-Pal subclass header)."
            )
            continue
        text = h.read_text(encoding="utf-8")
        class_name = f"APaldarkPalCharacter_{species}"
        if not re.search(
            rf"class\s+PALDARKLAB_API\s+{class_name}\s*:\s*public\s+APaldarkPalCharacter",
            text,
        ):
            fail(
                f"{h.relative_to(REPO_ROOT)} must declare "
                f"`class PALDARKLAB_API {class_name} : public "
                "APaldarkPalCharacter` (W29-30 — hostile subclass shape)."
            )
        if not c.is_file():
            fail(
                f"missing {c.relative_to(REPO_ROOT)} "
                f"(W29-30 — {species} hostile-Pal subclass body)."
            )
            continue
        body = c.read_text(encoding="utf-8")
        for required in (species_tag, "TAG_Paldark_Team_Hostile"):
            if required not in body:
                fail(
                    f"{c.relative_to(REPO_ROOT)} is missing `{required}` "
                    f"(W29-30 — {species} ctor must stamp the species "
                    "+ hostile team tags)."
                )

        # L-13 enforcement scoped to W29-30 — CandidateActivities must
        # be assigned, not appended.
        if re.search(r"CandidateActivities\s*\.\s*(Add|Append|Push)\b", body):
            fail(
                f"{c.relative_to(REPO_ROOT)} appends to CandidateActivities "
                f"(W29-30/L-13 — {species} ctor MUST assign the array "
                "directly so it does not inherit FollowSquadCommand from "
                "any future friendly default)."
            )
        if not re.search(r"CandidateActivities\s*=\s*\{", body):
            fail(
                f"{c.relative_to(REPO_ROOT)} must assign "
                "`CandidateActivities = { ... }` directly in the ctor "
                f"(W29-30 — {species} hostile ladder must be authoritative)."
            )

        if pack_tag is not None:
            if pack_tag not in body:
                fail(
                    f"{c.relative_to(REPO_ROOT)} must default PackTag to "
                    f"`{pack_tag}` (W29-30 — {species} ships in packs)."
                )
            if "RegisterPalToPack" not in body or "UnregisterPalFromPack" not in body:
                fail(
                    f"{c.relative_to(REPO_ROOT)} must register/unregister "
                    "with UPaldarkHostilePackSubsystem in BeginPlay/EndPlay "
                    f"(W29-30 — {species})."
                )

    # (7) Boltmane replaces Combat with BossPhase.
    boltmane_cpp = private_root / "Pal" / "PaldarkPalCharacter_Boltmane.cpp"
    if boltmane_cpp.is_file():
        body = boltmane_cpp.read_text(encoding="utf-8")
        if "UPaldarkActivity_BossPhase::StaticClass" not in body:
            fail(
                f"{boltmane_cpp.relative_to(REPO_ROOT)} must include "
                "`UPaldarkActivity_BossPhase::StaticClass()` in its "
                "CandidateActivities (W29-30 — boss FSM is what "
                "differentiates Boltmane from a generic hostile)."
            )
        if "UPaldarkActivity_Combat::StaticClass" in body:
            fail(
                f"{boltmane_cpp.relative_to(REPO_ROOT)} must NOT include "
                "`UPaldarkActivity_Combat::StaticClass()` — BossPhase "
                "(P50) replaces Combat (P40) for Boltmane (W29-30)."
            )

    # (8) UPaldarkActivity_BossPhase scaffold.
    boss_h = public_root / "Pal" / "Activities" / "PaldarkActivity_BossPhase.h"
    boss_c = private_root / "Pal" / "Activities" / "PaldarkActivity_BossPhase.cpp"
    if not boss_h.is_file():
        fail(
            f"missing {boss_h.relative_to(REPO_ROOT)} "
            "(W29-30 — boss FSM activity header)."
        )
    else:
        text = boss_h.read_text(encoding="utf-8")
        if not re.search(
            r"class\s+PALDARKLAB_API\s+UPaldarkActivity_BossPhase\s*:\s*public\s+UPaldarkBaseActivity",
            text,
        ):
            fail(
                f"{boss_h.relative_to(REPO_ROOT)} must declare "
                "`class PALDARKLAB_API UPaldarkActivity_BossPhase : "
                "public UPaldarkBaseActivity` (W29-30)."
            )
        for sym in (
            "EPaldarkBossPhase",
            "OnBossPhaseChanged",
            "EnragedThresholdPct",
            "TelegraphThresholdPct",
            "CanRun_Implementation",
            "ShouldContinue_Implementation",
            "EnterActivity_Implementation",
            "TickActivity_Implementation",
            "ExitActivity_Implementation",
        ):
            if sym not in text:
                fail(
                    f"{boss_h.relative_to(REPO_ROOT)} is missing "
                    f"`{sym}` (W29-30 — boss phase scaffold shape)."
                )

        # Q3 Bug-Bash L-28 — the activity header must declare the per-
        # species hydrate helper so the .cpp can keep the read-from-DA
        # path in one place (mirrors the W35-36 TameComponent's
        # `HydrateFromPalDefinition` pattern).
        if "HydrateThresholdsFromPalDefinition" not in text:
            fail(
                f"{boss_h.relative_to(REPO_ROOT)} must declare "
                "`HydrateThresholdsFromPalDefinition` (Q3 Bug-Bash L-28 — "
                "the helper reads `UPaldarkPalDefinition::PhaseHealthThresholds[]` "
                "and overrides the activity's CDO defaults per-species. "
                "Pattern mirrors W35-36 UPaldarkPalTameComponent::HydrateFromPalDefinition. "
                "Without the helper a designer must subclass the activity "
                "per boss to retune thresholds, which the W29-30 README "
                "documented as an anti-pattern)."
            )
    if not boss_c.is_file():
        fail(
            f"missing {boss_c.relative_to(REPO_ROOT)} "
            "(W29-30 — boss FSM activity body)."
        )
    else:
        body = boss_c.read_text(encoding="utf-8")
        if "TAG_Paldark_Pal_Activity_BossPhase" not in body:
            fail(
                f"{boss_c.relative_to(REPO_ROOT)} must bind "
                "`TAG_Paldark_Pal_Activity_BossPhase` to ActivityTag in "
                "the ctor (W29-30)."
            )
        if not re.search(r"Priority\s*=\s*50\b", body):
            fail(
                f"{boss_c.relative_to(REPO_ROOT)} must set "
                "`Priority = 50` (W29-30 — boss FSM preempts Combat P40)."
            )
        # Phase transitions must broadcast the delegate so HUD/AnimBP
        # designers can react.
        if "OnBossPhaseChanged.Broadcast" not in body:
            fail(
                f"{boss_c.relative_to(REPO_ROOT)} must call "
                "`OnBossPhaseChanged.Broadcast(...)` on phase transition "
                "(W29-30 — designer hooks HUD banner + montage trigger "
                "off this delegate)."
            )

        # Q3 Bug-Bash L-28 — the .cpp must:
        #   (a) include the PalDefinition header (the symbol lookup
        #       needs the concrete type for Cast<>).
        #   (b) include the AssetManager header (the lookup goes through
        #       `UAssetManager::GetIfValid` like W35-36 TameComponent).
        #   (c) implement the helper body.
        #   (d) call the helper from EnterActivity (otherwise the helper
        #       is dead code).
        #   (e) reference the `PhaseHealthThresholds` field (otherwise
        #       the helper is reading a different field).
        #   (f) tag log lines `[L-28]` so dedicated-server log scrapes
        #       can attribute fallback noise to this bug-bash issue
        #       (matches the README W29-30 § L-28 grep recipe + the
        #       L-34 precedent set in L-34 closure).
        l28_required_symbols = {
            'Pal/PaldarkPalDefinition.h': (
                "the .cpp must include `Pal/PaldarkPalDefinition.h` for the "
                "`UPaldarkPalDefinition` Cast<> on the helper path"
            ),
            'Engine/AssetManager.h': (
                "the .cpp must include `Engine/AssetManager.h` for "
                "`UAssetManager::GetIfValid()` (lookup pattern mirrors "
                "W35-36 UPaldarkPalTameComponent::HydrateFromPalDefinition)"
            ),
            'void UPaldarkActivity_BossPhase::HydrateThresholdsFromPalDefinition': (
                "the helper declared on the header must have a body in the .cpp"
            ),
            'HydrateThresholdsFromPalDefinition()': (
                "EnterActivity_Implementation MUST call the helper, otherwise "
                "the per-species override is dead code and the boss uses "
                "the activity CDO defaults regardless of DA_PalDef_<Species>"
            ),
            'PhaseHealthThresholds': (
                "the helper must read `PhaseHealthThresholds` from the "
                "resolved PalDefinition (this is the field the L-28 contract "
                "exposes on UPaldarkPalDefinition)"
            ),
            '[L-28]': (
                "log lines must be tagged `[L-28]` so dedicated-server log "
                "scrapes can attribute boss threshold fallback noise to this "
                "bug-bash issue (matches the L-34 closure precedent + README "
                "W29-30 § L-28 grep recipe)"
            ),
        }
        for sym, rationale in l28_required_symbols.items():
            if sym not in body:
                fail(
                    f"{boss_c.relative_to(REPO_ROOT)} is missing "
                    f"`{sym}` (Q3 Bug-Bash L-28 — {rationale})."
                )


def check_w31_map_blockout_shape() -> None:
    """W31-32 — Map 2 "Rừng Hỏng" blockout scaffold shape.

    The C++ scaffold backing Map 2 splits into four cooperating pieces:

      1. `UPaldarkMapDefinition` (UPrimaryDataAsset) — declares a map's
         identity tag + soft ref to the .umap + recommended POI / spawn
         counts.
      2. `APaldarkPointOfInterest` (AActor) — designer-placeable POI with
         POITag + DangerTier; registers with the POI subsystem on BeginPlay.
      3. `UPaldarkPOISubsystem` (UWorldSubsystem) — server-only registry +
         query API (`GetAllPOIs`, `GetPOIsByTag`, `GetPOIsByDangerTier`,
         `GetNearestPOI`, `DumpToLog`).
      4. `APaldarkPalSpawnPoint` (AActor) — single-spawn-point counterpart
         to the W20-21 batch spawner; resolves the spawn through either
         the W27-28 definition path (`UPaldarkPalSpawnSubsystem::SpawnPalAsync`)
         or a fallback `SpawnHostilePackAt` class path.

    Plus integration glue:
      5. `UPaldarkExperienceDefinition` exposes a
         `TSoftObjectPtr<UPaldarkMapDefinition> MapDefinition` UPROPERTY.
      6. `DefaultGame.ini` registers the `PaldarkMapDefinition` primary
         asset type pointing at `/Game/Paldark/Maps`.
      7. Three new console commands (`Paldark.POI.Dump`,
         `Paldark.POI.HighlightTier`, `Paldark.Map.LoadDefinition`) live
         under `Private/Map/PaldarkMapConsoleCommands.cpp`.
      8. The `EXPECTED_GAMEPLAY_TAGS` set must include the 12 W31-32 tags
         (already enforced by check_gameplay_tags).
    """
    src_root = PROJECT_DIR / "Source" / "PaldarkLab"

    map_def_h     = src_root / "Public" / "Map" / "PaldarkMapDefinition.h"
    map_def_cpp   = src_root / "Private" / "Map" / "PaldarkMapDefinition.cpp"
    poi_h         = src_root / "Public" / "Map" / "PaldarkPointOfInterest.h"
    poi_cpp       = src_root / "Private" / "Map" / "PaldarkPointOfInterest.cpp"
    poi_sub_h     = src_root / "Public" / "Map" / "PaldarkPOISubsystem.h"
    poi_sub_cpp   = src_root / "Private" / "Map" / "PaldarkPOISubsystem.cpp"
    spawn_pt_h    = src_root / "Public" / "Map" / "PaldarkPalSpawnPoint.h"
    spawn_pt_cpp  = src_root / "Private" / "Map" / "PaldarkPalSpawnPoint.cpp"
    map_cmds_cpp  = src_root / "Private" / "Map" / "PaldarkMapConsoleCommands.cpp"
    experience_h  = src_root / "Public" / "Experience" / "PaldarkExperienceDefinition.h"
    default_game  = PROJECT_DIR / "Config" / "DefaultGame.ini"
    content_dir   = PROJECT_DIR / "Content" / "Paldark" / "Maps"

    # (1) UPaldarkMapDefinition — file presence + shape.
    if not map_def_h.is_file():
        fail(
            f"missing {map_def_h.relative_to(REPO_ROOT)} "
            "(W31-32 — UPaldarkMapDefinition is the per-map data asset; "
            "experiences cannot declare which map they ship with otherwise)."
        )
        return
    if not map_def_cpp.is_file():
        fail(
            f"missing {map_def_cpp.relative_to(REPO_ROOT)} "
            "(W31-32 — UPaldarkMapDefinition needs a .cpp for the "
            "`GetPrimaryAssetId` override)."
        )

    map_def_text = map_def_h.read_text(encoding="utf-8")
    if not re.search(
        r"class\s+PALDARKLAB_API\s+UPaldarkMapDefinition\s*:\s*public\s+UPrimaryDataAsset",
        map_def_text,
    ):
        fail(
            f"{map_def_h.relative_to(REPO_ROOT)} must declare "
            "`class PALDARKLAB_API UPaldarkMapDefinition : public "
            "UPrimaryDataAsset` (W31-32 — primary-asset scan rule relies "
            "on the UPrimaryDataAsset base)."
        )
    for label, pattern in (
        ("FGameplayTag MapTag",                       r"FGameplayTag\s+MapTag\b"),
        ("FText DisplayName",                         r"FText\s+DisplayName\b"),
        ("TSoftObjectPtr<UWorld> MapAsset",           r"TSoftObjectPtr<\s*UWorld\s*>\s*MapAsset\b"),
        ("int32 RecommendedPlayerCount",              r"int32\s+RecommendedPlayerCount\b"),
        ("int32 RecommendedPOICount",                 r"int32\s+RecommendedPOICount\b"),
        ("int32 RecommendedSpawnPointCount",          r"int32\s+RecommendedSpawnPointCount\b"),
        ("FGameplayTagContainer IntrinsicTagsToAssert", r"FGameplayTagContainer\s+IntrinsicTagsToAssert\b"),
    ):
        if not re.search(pattern, map_def_text):
            fail(
                f"{map_def_h.relative_to(REPO_ROOT)} is missing UPROPERTY "
                f"`{label}` (W31-32 — required for map blockout metadata)."
            )

    if map_def_cpp.is_file():
        cpp_text = map_def_cpp.read_text(encoding="utf-8")
        if not re.search(
            r"FPrimaryAssetType\s*\(\s*TEXT\s*\(\s*\"PaldarkMapDefinition\"\s*\)\s*\)",
            cpp_text,
        ):
            fail(
                f"{map_def_cpp.relative_to(REPO_ROOT)} must return a "
                "primary asset id with "
                "`FPrimaryAssetType(TEXT(\"PaldarkMapDefinition\"))` "
                "(W31-32 — the type string MUST match the DefaultGame.ini "
                "PrimaryAssetTypesToScan row)."
            )

    # (2) APaldarkPointOfInterest — file + shape.
    if not poi_h.is_file():
        fail(
            f"missing {poi_h.relative_to(REPO_ROOT)} "
            "(W31-32 — APaldarkPointOfInterest is the designer-placeable "
            "POI marker; map blockout cannot register POIs without it)."
        )
        return
    if not poi_cpp.is_file():
        fail(
            f"missing {poi_cpp.relative_to(REPO_ROOT)} "
            "(W31-32 — APaldarkPointOfInterest needs a .cpp for the "
            "BeginPlay / EndPlay registration handshake)."
        )

    poi_text = poi_h.read_text(encoding="utf-8")
    if not re.search(
        r"class\s+PALDARKLAB_API\s+APaldarkPointOfInterest\s*:\s*public\s+AActor",
        poi_text,
    ):
        fail(
            f"{poi_h.relative_to(REPO_ROOT)} must declare "
            "`class PALDARKLAB_API APaldarkPointOfInterest : public AActor` "
            "(W31-32 — base AActor required for level placement)."
        )
    if "enum class EPaldarkPOIDangerTier" not in poi_text:
        fail(
            f"{poi_h.relative_to(REPO_ROOT)} must declare "
            "`enum class EPaldarkPOIDangerTier : uint8` (W31-32 — three-bin "
            "Low/Medium/High enum required for AI Director density planning)."
        )
    for label, pattern in (
        ("FGameplayTag POITag",                        r"FGameplayTag\s+POITag\b"),
        ("EPaldarkPOIDangerTier DangerTier",           r"EPaldarkPOIDangerTier\s+DangerTier\b"),
        ("FGameplayTagContainer AllowedSpeciesTags",   r"FGameplayTagContainer\s+AllowedSpeciesTags\b"),
        ("float RegistrationRadius",                   r"float\s+RegistrationRadius\b"),
    ):
        if not re.search(pattern, poi_text):
            fail(
                f"{poi_h.relative_to(REPO_ROOT)} is missing UPROPERTY "
                f"`{label}` (W31-32 — required for POI metadata)."
            )

    if poi_cpp.is_file():
        poi_cpp_text = poi_cpp.read_text(encoding="utf-8")
        for token in ("RegisterPOI", "UnregisterPOI"):
            if token not in poi_cpp_text:
                fail(
                    f"{poi_cpp.relative_to(REPO_ROOT)} must call "
                    f"`{token}` (W31-32 — without the BeginPlay/EndPlay "
                    "handshake the POI subsystem never sees the actor)."
                )

    # (3) UPaldarkPOISubsystem — file + shape.
    if not poi_sub_h.is_file():
        fail(
            f"missing {poi_sub_h.relative_to(REPO_ROOT)} "
            "(W31-32 — UPaldarkPOISubsystem owns the POI registry + "
            "query API; without it no consumer can find POIs)."
        )
        return
    if not poi_sub_cpp.is_file():
        fail(
            f"missing {poi_sub_cpp.relative_to(REPO_ROOT)} "
            "(W31-32 — UPaldarkPOISubsystem needs a .cpp for the registry "
            "+ TActorIterator sweep)."
        )

    poi_sub_text = poi_sub_h.read_text(encoding="utf-8")
    if not re.search(
        r"class\s+PALDARKLAB_API\s+UPaldarkPOISubsystem\s*:\s*public\s+UWorldSubsystem",
        poi_sub_text,
    ):
        fail(
            f"{poi_sub_h.relative_to(REPO_ROOT)} must declare "
            "`class PALDARKLAB_API UPaldarkPOISubsystem : public "
            "UWorldSubsystem` (W31-32 — world-scoped so the registry tears "
            "down with the level)."
        )
    for sig in (
        r"void\s+RegisterPOI\s*\(",
        r"void\s+UnregisterPOI\s*\(",
        r"GetAllPOIs\s*\(",
        r"GetPOIsByTag\s*\(",
        r"GetPOIsByDangerTier\s*\(",
        r"GetNearestPOI\s*\(",
        r"DumpToLog\s*\(",
    ):
        if not re.search(sig, poi_sub_text):
            fail(
                f"{poi_sub_h.relative_to(REPO_ROOT)} is missing the "
                f"`{sig}` declaration (W31-32 — required public surface)."
            )

    if poi_sub_cpp.is_file():
        poi_sub_cpp_text = poi_sub_cpp.read_text(encoding="utf-8")
        if "TActorIterator<APaldarkPointOfInterest>" not in poi_sub_cpp_text:
            fail(
                f"{poi_sub_cpp.relative_to(REPO_ROOT)} must sweep the world "
                "via `TActorIterator<APaldarkPointOfInterest>` in Initialize "
                "/ OnWorldBeginPlay (W31-32 — closes the registry race when "
                "POIs finish BeginPlay before the subsystem starts)."
            )

    # (4) APaldarkPalSpawnPoint — file + shape.
    if not spawn_pt_h.is_file():
        fail(
            f"missing {spawn_pt_h.relative_to(REPO_ROOT)} "
            "(W31-32 — APaldarkPalSpawnPoint is the designer-placeable "
            "single-spawn-point counterpart to the W20-21 batch spawner)."
        )
        return
    if not spawn_pt_cpp.is_file():
        fail(
            f"missing {spawn_pt_cpp.relative_to(REPO_ROOT)} "
            "(W31-32 — APaldarkPalSpawnPoint needs a .cpp for the "
            "definition-path / class-path resolver)."
        )

    spawn_pt_text = spawn_pt_h.read_text(encoding="utf-8")
    if not re.search(
        r"class\s+PALDARKLAB_API\s+APaldarkPalSpawnPoint\s*:\s*public\s+AActor",
        spawn_pt_text,
    ):
        fail(
            f"{spawn_pt_h.relative_to(REPO_ROOT)} must declare "
            "`class PALDARKLAB_API APaldarkPalSpawnPoint : public AActor` "
            "(W31-32 — base AActor required for level placement)."
        )
    for label, pattern in (
        ("FGameplayTag SpeciesTag",                          r"FGameplayTag\s+SpeciesTag\b"),
        ("FPrimaryAssetId PalDefinitionId",                  r"FPrimaryAssetId\s+PalDefinitionId\b"),
        ("TSubclassOf<APaldarkPalCharacter> FallbackPalClass", r"TSubclassOf<\s*APaldarkPalCharacter\s*>\s*FallbackPalClass\b"),
        ("bool bSpawnOnBeginPlay",                           r"bool\s+bSpawnOnBeginPlay\b"),
        ("int32 MaxConcurrentInstances",                     r"int32\s+MaxConcurrentInstances\b"),
        ("FGameplayTag AssociatedPOITag",                    r"FGameplayTag\s+AssociatedPOITag\b"),
    ):
        if not re.search(pattern, spawn_pt_text):
            fail(
                f"{spawn_pt_h.relative_to(REPO_ROOT)} is missing UPROPERTY "
                f"`{label}` (W31-32 — required for spawn point metadata)."
            )

    if spawn_pt_cpp.is_file():
        spawn_pt_cpp_text = spawn_pt_cpp.read_text(encoding="utf-8")
        # Definition path must route through UPaldarkPalSpawnSubsystem (W27-28).
        if "SpawnPalAsync" not in spawn_pt_cpp_text:
            fail(
                f"{spawn_pt_cpp.relative_to(REPO_ROOT)} must route the "
                "definition path through `UPaldarkPalSpawnSubsystem::"
                "SpawnPalAsync` (W31-32 — that is the W27-28 async-load + "
                "spawn pipeline). Missing token: SpawnPalAsync."
            )
        # Class path must reuse the W20-21 ground-snap helper for parity
        # with the existing batch spawner.
        if "SpawnHostilePackAt" not in spawn_pt_cpp_text:
            fail(
                f"{spawn_pt_cpp.relative_to(REPO_ROOT)} must reuse "
                "`APaldarkHostilePalSpawner::SpawnHostilePackAt` for the "
                "class path (W31-32 — single-spawn-point semantics with "
                "Count=1 keeps the W20-21 ground-snap behaviour)."
            )

    # (5) Experience exposes MapDefinition soft ref.
    if experience_h.is_file():
        exp_text = experience_h.read_text(encoding="utf-8")
        if not re.search(
            r"TSoftObjectPtr<\s*UPaldarkMapDefinition\s*>\s*MapDefinition\b",
            exp_text,
        ):
            fail(
                f"{experience_h.relative_to(REPO_ROOT)} must expose "
                "`TSoftObjectPtr<UPaldarkMapDefinition> MapDefinition` "
                "(W31-32 — without it the experience cannot declare which "
                "map it ships with)."
            )

    # (6) DefaultGame.ini wiring.
    if default_game.is_file():
        ini_text = default_game.read_text(encoding="utf-8")
        if not re.search(
            r"PrimaryAssetType\s*=\s*\"PaldarkMapDefinition\"",
            ini_text,
        ):
            fail(
                f"{default_game.relative_to(REPO_ROOT)} is missing a "
                "`+PrimaryAssetTypesToScan=(PrimaryAssetType="
                "\"PaldarkMapDefinition\"...)` row (W31-32 — without it "
                "the editor's Asset Manager scanner skips /Game/Paldark/Maps)."
            )
        if "/Game/Paldark/Maps" not in ini_text:
            fail(
                f"{default_game.relative_to(REPO_ROOT)} must point the "
                "PaldarkMapDefinition row at `/Game/Paldark/Maps` "
                "(W31-32 — this is the convention the W31-32 README "
                "references)."
            )

    # (7) Console commands TU + 3 cmd registrations.
    if not map_cmds_cpp.is_file():
        fail(
            f"missing {map_cmds_cpp.relative_to(REPO_ROOT)} "
            "(W31-32 — console commands TU hosts Paldark.POI.Dump + "
            "Paldark.POI.HighlightTier + Paldark.Map.LoadDefinition)."
        )
    else:
        cmds_text = map_cmds_cpp.read_text(encoding="utf-8")
        for cmd_name in (
            "Paldark.POI.Dump",
            "Paldark.POI.HighlightTier",
            "Paldark.Map.LoadDefinition",
        ):
            if cmd_name not in cmds_text:
                fail(
                    f"{map_cmds_cpp.relative_to(REPO_ROOT)} must register "
                    f"the `{cmd_name}` console command (W31-32 — designer "
                    "needs this for level-blockout playtest verification)."
                )

    # (8) Content/Paldark/Maps/ placeholder directory (designer drops .umap).
    if not content_dir.is_dir():
        fail(
            f"missing {content_dir.relative_to(REPO_ROOT)}/ directory "
            "(W31-32 — designer drops Map_RungHong.umap + DA_MapDef_RungHong "
            "here; ship the directory with a README.txt placeholder so the "
            "AssetManager scan rule has a valid path to walk)."
        )


def check_w33_loot_drop_shape() -> None:
    """W33-34 — Map 3 "Cơ Xưởng PalCorp" + tech-themed loot drop scaffold.

    Six cooperating pieces:

      1. `UPaldarkLootTable` (UPrimaryDataAsset) — drop pool data asset
         (LootTableTag + Entries + MaxRolls + GetPrimaryAssetId override).
      2. `UPaldarkLootDropComponent` (UActorComponent) — server-only
         listener on `UPaldarkAttributeSet::OnHealthZeroed`. Designer
         configures `LootTableId` per hostile-Pal BP subclass.
      3. `APaldarkLootBag` (AActor) — replicated pickup actor with
         `ReplicatedContents` (DOREPLIFETIME) + `RequestPickup` API +
         5-minute auto-cleanup.
      4. `APaldarkPalCharacter` exposes a `LootDropSlot` default subobject
         + `GetLootDropSlot()` accessor (W3-4 component slot pattern).
      5. `UPaldarkMapDefinition` gains a `bIsIndoor` designer hint flag.
      6. `Config/DefaultGame.ini` registers the `PaldarkLootTable` primary
         asset type pointing at `/Game/Paldark/Loot`.

    Plus the 3 console commands and the Content/Paldark/Loot/ placeholder.
    """
    src_root = PROJECT_DIR / "Source" / "PaldarkLab"

    table_h       = src_root / "Public"  / "Loot" / "PaldarkLootTable.h"
    table_cpp     = src_root / "Private" / "Loot" / "PaldarkLootTable.cpp"
    drop_h        = src_root / "Public"  / "Loot" / "PaldarkLootDropComponent.h"
    drop_cpp      = src_root / "Private" / "Loot" / "PaldarkLootDropComponent.cpp"
    bag_h         = src_root / "Public"  / "Loot" / "PaldarkLootBag.h"
    bag_cpp       = src_root / "Private" / "Loot" / "PaldarkLootBag.cpp"
    loot_cmds_cpp = src_root / "Private" / "Loot" / "PaldarkLootConsoleCommands.cpp"
    pal_char_h    = src_root / "Public"  / "Pal"  / "PaldarkPalCharacter.h"
    pal_char_cpp  = src_root / "Private" / "Pal"  / "PaldarkPalCharacter.cpp"
    map_def_h     = src_root / "Public"  / "Map"  / "PaldarkMapDefinition.h"
    default_game  = PROJECT_DIR / "Config" / "DefaultGame.ini"
    content_dir   = PROJECT_DIR / "Content" / "Paldark" / "Loot"

    # (1) UPaldarkLootTable — file presence + shape.
    if not table_h.is_file():
        fail(
            f"missing {table_h.relative_to(REPO_ROOT)} "
            "(W33-34 — UPaldarkLootTable is the per-drop-pool data asset; "
            "hostile-Pal subclasses cannot configure loot otherwise)."
        )
        return
    if not table_cpp.is_file():
        fail(
            f"missing {table_cpp.relative_to(REPO_ROOT)} "
            "(W33-34 — UPaldarkLootTable needs a .cpp for the "
            "`GetPrimaryAssetId` override + `RollEntries` weighted RNG)."
        )

    table_text = table_h.read_text(encoding="utf-8")
    if not re.search(
        r"class\s+PALDARKLAB_API\s+UPaldarkLootTable\s*:\s*public\s+UPrimaryDataAsset",
        table_text,
    ):
        fail(
            f"{table_h.relative_to(REPO_ROOT)} must declare "
            "`class PALDARKLAB_API UPaldarkLootTable : public "
            "UPrimaryDataAsset` (W33-34 — primary-asset scan rule relies "
            "on the UPrimaryDataAsset base)."
        )
    if not re.search(r"struct\s+PALDARKLAB_API\s+FPaldarkLootEntry\b", table_text):
        fail(
            f"{table_h.relative_to(REPO_ROOT)} must declare "
            "`struct PALDARKLAB_API FPaldarkLootEntry` (W33-34 — required "
            "row type for the Entries array)."
        )
    for label, pattern in (
        ("FGameplayTag LootTableTag",                                r"FGameplayTag\s+LootTableTag\b"),
        ("FText DisplayName",                                        r"FText\s+DisplayName\b"),
        ("int32 MaxRolls",                                           r"int32\s+MaxRolls\b"),
        ("TArray<FPaldarkLootEntry> Entries",                        r"TArray<\s*FPaldarkLootEntry\s*>\s*Entries\b"),
        ("TSoftObjectPtr<UPaldarkItemDefinition> ItemDefinition",    r"TSoftObjectPtr<\s*UPaldarkItemDefinition\s*>\s*ItemDefinition\b"),
        ("int32 Weight",                                             r"int32\s+Weight\b"),
        ("int32 MinCount",                                           r"int32\s+MinCount\b"),
        ("int32 MaxCount",                                           r"int32\s+MaxCount\b"),
        ("FGameplayTag GuaranteedWhenTag",                           r"FGameplayTag\s+GuaranteedWhenTag\b"),
    ):
        if not re.search(pattern, table_text):
            fail(
                f"{table_h.relative_to(REPO_ROOT)} is missing UPROPERTY "
                f"or struct field `{label}` (W33-34 — required for the "
                "weighted-roll contract)."
            )

    if table_cpp.is_file():
        table_cpp_text = table_cpp.read_text(encoding="utf-8")
        if not re.search(
            r"FPrimaryAssetType\s*\(\s*TEXT\s*\(\s*\"PaldarkLootTable\"\s*\)\s*\)",
            table_cpp_text,
        ):
            fail(
                f"{table_cpp.relative_to(REPO_ROOT)} must return a "
                "primary asset id with "
                "`FPrimaryAssetType(TEXT(\"PaldarkLootTable\"))` "
                "(W33-34 — the type string MUST match the DefaultGame.ini "
                "PrimaryAssetTypesToScan row)."
            )
        if "RollEntries" not in table_cpp_text:
            fail(
                f"{table_cpp.relative_to(REPO_ROOT)} must implement "
                "`RollEntries` (W33-34 — without it the loot drop component "
                "has no way to produce loot bag contents)."
            )

    # (2) UPaldarkLootDropComponent — file + shape.
    if not drop_h.is_file():
        fail(
            f"missing {drop_h.relative_to(REPO_ROOT)} "
            "(W33-34 — UPaldarkLootDropComponent listens to "
            "OnHealthZeroed and drives the loot bag spawn)."
        )
        return
    if not drop_cpp.is_file():
        fail(
            f"missing {drop_cpp.relative_to(REPO_ROOT)} "
            "(W33-34 — UPaldarkLootDropComponent needs a .cpp for the "
            "BeginPlay binding + async-load callback)."
        )

    drop_text = drop_h.read_text(encoding="utf-8")
    if not re.search(
        r"class\s+PALDARKLAB_API\s+UPaldarkLootDropComponent\s*:\s*public\s+UActorComponent",
        drop_text,
    ):
        fail(
            f"{drop_h.relative_to(REPO_ROOT)} must declare "
            "`class PALDARKLAB_API UPaldarkLootDropComponent : public "
            "UActorComponent` (W33-34 — component slot pattern)."
        )
    for label, pattern in (
        ("FPrimaryAssetId LootTableId",                  r"FPrimaryAssetId\s+LootTableId\b"),
        ("TSubclassOf<APaldarkLootBag> LootBagClass",    r"TSubclassOf<\s*APaldarkLootBag\s*>\s*LootBagClass\b"),
        ("FVector SpawnOffset",                          r"FVector\s+SpawnOffset\b"),
    ):
        if not re.search(pattern, drop_text):
            fail(
                f"{drop_h.relative_to(REPO_ROOT)} is missing UPROPERTY "
                f"`{label}` (W33-34 — required loot drop config)."
            )

    if drop_cpp.is_file():
        drop_cpp_text = drop_cpp.read_text(encoding="utf-8")
        if "OnHealthZeroed" not in drop_cpp_text:
            fail(
                f"{drop_cpp.relative_to(REPO_ROOT)} must bind to "
                "`UPaldarkAttributeSet::OnHealthZeroed` (W33-34 — that is "
                "the GAS broadcast that triggers the loot roll)."
            )
        if "LoadPrimaryAsset" not in drop_cpp_text:
            fail(
                f"{drop_cpp.relative_to(REPO_ROOT)} must async-load the "
                "loot table via `UAssetManager::LoadPrimaryAsset` (W33-34 "
                "— synchronous load on death blocks the kill pipeline)."
            )
        if "SpawnActor<APaldarkLootBag>" not in drop_cpp_text:
            fail(
                f"{drop_cpp.relative_to(REPO_ROOT)} must spawn the loot "
                "bag via `SpawnActor<APaldarkLootBag>` (W33-34 — required "
                "to surface drops to clients)."
            )

    # (3) APaldarkLootBag — file + shape.
    if not bag_h.is_file():
        fail(
            f"missing {bag_h.relative_to(REPO_ROOT)} "
            "(W33-34 — APaldarkLootBag is the replicated pickup actor)."
        )
        return
    if not bag_cpp.is_file():
        fail(
            f"missing {bag_cpp.relative_to(REPO_ROOT)} "
            "(W33-34 — APaldarkLootBag needs a .cpp for "
            "GetLifetimeReplicatedProps + RequestPickup)."
        )

    bag_text = bag_h.read_text(encoding="utf-8")
    if not re.search(
        r"class\s+PALDARKLAB_API\s+APaldarkLootBag\s*:\s*public\s+AActor",
        bag_text,
    ):
        fail(
            f"{bag_h.relative_to(REPO_ROOT)} must declare "
            "`class PALDARKLAB_API APaldarkLootBag : public AActor` "
            "(W33-34 — base AActor required for level spawn + replication)."
        )
    if not re.search(r"struct\s+PALDARKLAB_API\s+FPaldarkLootBagEntry\b", bag_text):
        fail(
            f"{bag_h.relative_to(REPO_ROOT)} must declare "
            "`struct PALDARKLAB_API FPaldarkLootBagEntry` (W33-34 — "
            "replicated row type for ReplicatedContents)."
        )
    for label, pattern in (
        ("TArray<FPaldarkLootBagEntry> ReplicatedContents",  r"TArray<\s*FPaldarkLootBagEntry\s*>\s*ReplicatedContents\b"),
        ("float AutoCleanupSeconds",                         r"float\s+AutoCleanupSeconds\b"),
    ):
        if not re.search(pattern, bag_text):
            fail(
                f"{bag_h.relative_to(REPO_ROOT)} is missing UPROPERTY "
                f"`{label}` (W33-34 — required loot bag state)."
            )

    if bag_cpp.is_file():
        bag_cpp_text = bag_cpp.read_text(encoding="utf-8")
        if "DOREPLIFETIME(APaldarkLootBag, ReplicatedContents)" not in bag_cpp_text:
            fail(
                f"{bag_cpp.relative_to(REPO_ROOT)} must register "
                "`DOREPLIFETIME(APaldarkLootBag, ReplicatedContents)` "
                "(W33-34 — without it clients see an empty bag)."
            )
        if "RequestPickup" not in bag_cpp_text:
            fail(
                f"{bag_cpp.relative_to(REPO_ROOT)} must implement "
                "`RequestPickup` (W33-34 — that is the server entry point "
                "for popping items into a player's inventory)."
            )

    # (4) APaldarkPalCharacter — LootDropSlot wired.
    if pal_char_h.is_file():
        pal_h_text = pal_char_h.read_text(encoding="utf-8")
        if not re.search(
            r"TObjectPtr<\s*UPaldarkLootDropComponent\s*>\s*LootDropSlot\b",
            pal_h_text,
        ):
            fail(
                f"{pal_char_h.relative_to(REPO_ROOT)} must declare "
                "`TObjectPtr<UPaldarkLootDropComponent> LootDropSlot` "
                "(W33-34 — 10th Pal component slot)."
            )
        if "GetLootDropSlot" not in pal_h_text:
            fail(
                f"{pal_char_h.relative_to(REPO_ROOT)} must expose a "
                "`GetLootDropSlot()` accessor (W33-34 — every other Pal "
                "slot exposes one; console commands rely on it)."
            )
    if pal_char_cpp.is_file():
        pal_cpp_text = pal_char_cpp.read_text(encoding="utf-8")
        if "CreateDefaultSubobject<UPaldarkLootDropComponent>" not in pal_cpp_text:
            fail(
                f"{pal_char_cpp.relative_to(REPO_ROOT)} must "
                "`CreateDefaultSubobject<UPaldarkLootDropComponent>` for "
                "LootDropSlot (W33-34 — without it hostile-Pal subclasses "
                "never get a listener)."
            )

    # (5) UPaldarkMapDefinition — bIsIndoor designer hint.
    if map_def_h.is_file():
        map_def_text = map_def_h.read_text(encoding="utf-8")
        if not re.search(r"bool\s+bIsIndoor\b", map_def_text):
            fail(
                f"{map_def_h.relative_to(REPO_ROOT)} must declare "
                "`bool bIsIndoor` (W33-34 — designer hint for Q4 AI "
                "Director density planner; Map 3 sets this true)."
            )

    # (6) DefaultGame.ini wiring for PaldarkLootTable.
    if default_game.is_file():
        ini_text = default_game.read_text(encoding="utf-8")
        if not re.search(
            r"PrimaryAssetType\s*=\s*\"PaldarkLootTable\"",
            ini_text,
        ):
            fail(
                f"{default_game.relative_to(REPO_ROOT)} is missing a "
                "`+PrimaryAssetTypesToScan=(PrimaryAssetType="
                "\"PaldarkLootTable\"...)` row (W33-34 — without it the "
                "editor's Asset Manager scanner skips /Game/Paldark/Loot)."
            )
        if "/Game/Paldark/Loot" not in ini_text:
            fail(
                f"{default_game.relative_to(REPO_ROOT)} must point the "
                "PaldarkLootTable row at `/Game/Paldark/Loot` "
                "(W33-34 — this is the convention the W33-34 README "
                "references)."
            )

    # (7) Console commands TU + 3 cmd registrations.
    if not loot_cmds_cpp.is_file():
        fail(
            f"missing {loot_cmds_cpp.relative_to(REPO_ROOT)} "
            "(W33-34 — console commands TU hosts Paldark.Loot.DumpTable + "
            "Paldark.Loot.ForceDrop + Paldark.Loot.PickupNearest)."
        )
    else:
        cmds_text = loot_cmds_cpp.read_text(encoding="utf-8")
        for cmd_name in (
            "Paldark.Loot.DumpTable",
            "Paldark.Loot.ForceDrop",
            "Paldark.Loot.PickupNearest",
        ):
            if cmd_name not in cmds_text:
                fail(
                    f"{loot_cmds_cpp.relative_to(REPO_ROOT)} must register "
                    f"the `{cmd_name}` console command (W33-34 — designer "
                    "needs this for loot wiring verification)."
                )

    # (8) Content/Paldark/Loot/ placeholder directory.
    if not content_dir.is_dir():
        fail(
            f"missing {content_dir.relative_to(REPO_ROOT)}/ directory "
            "(W33-34 — designer drops DA_LootTable_HostileStandard + "
            "DA_LootTable_Boss + DA_Item_Tech_* here; ship the directory "
            "with a README.txt placeholder so the AssetManager scan "
            "rule has a valid path to walk)."
        )


def check_w35_tame_minigame_shape() -> None:
    """W35-36 — Pal Bond + Capture (Tame minigame) scaffold.

    Seven cooperating pieces:

      1. `UPaldarkAttributeSet` gains `Stun` + `MaxStun` replicated
         FGameplayAttributeData with RepNotify callbacks (drives the
         capture-probability formula's StunPct contribution).
      2. `UPaldarkPalBondComponent` flips from W3-4 empty stub to a real
         component (`BondLevel` + `BondXP` replicated, `AddBondXP` server
         API, `OnBondLevelChanged` multicast delegate).
      3. `UPaldarkPalTameComponent` (server-only) holds `BeginTameAttempt`
         + `ForceTame` + the capture-probability formula. Designer flips
         `bIsTameable` on hostile-Pal subclasses.
      4. `UPaldarkPalDefinition` gains `TameDifficulty` +
         `BaseCaptureProbability` so per-species tuning lives on the
         W27-28 data asset (not hard-coded in C++).
      5. `APaldarkPalSphere` replicated projectile actor wraps a
         `UProjectileMovementComponent`; hit routes to
         `UPaldarkPalTameComponent::BeginTameAttempt`.
      6. `UPaldarkGameplayAbility_UsePalSphere` (server-side GA) consumes
         the Pal Sphere item from the player's inventory and spawns the
         projectile.
      7. `APaldarkCharacter` gains `RosterSlot` (13th player slot) +
         `GetRosterSlot()` accessor; `UPaldarkPlayerPalRosterComponent`
         hosts the per-player roster array (`FPaldarkRosterEntry`,
         COND_OwnerOnly replicated, `TamePal` server API, `DumpToLog`).
      8. Hostile-Pal subclasses (Direhound / Razorbird / Stoneclad /
         Vinewraith / Boltmane) flip `bIsTameable = true` on their
         TameSlot in ctor.

    Plus the 4 console commands and the Content/Paldark/Tame/ placeholder.
    """
    src_root = PROJECT_DIR / "Source" / "PaldarkLab"

    attr_h          = src_root / "Public"  / "Gas"            / "PaldarkAttributeSet.h"
    attr_cpp        = src_root / "Private" / "Gas"            / "PaldarkAttributeSet.cpp"
    bond_h          = src_root / "Public"  / "Pal" / "Components" / "PaldarkPalBondComponent.h"
    bond_cpp        = src_root / "Private" / "Pal" / "Components" / "PaldarkPalBondComponent.cpp"
    tame_h          = src_root / "Public"  / "Pal" / "Components" / "PaldarkPalTameComponent.h"
    tame_cpp        = src_root / "Private" / "Pal" / "Components" / "PaldarkPalTameComponent.cpp"
    pal_def_h       = src_root / "Public"  / "Pal"            / "PaldarkPalDefinition.h"
    sphere_h        = src_root / "Public"  / "Pal"            / "PaldarkPalSphere.h"
    sphere_cpp      = src_root / "Private" / "Pal"            / "PaldarkPalSphere.cpp"
    ga_h            = src_root / "Public"  / "Gas" / "Abilities" / "PaldarkGameplayAbility_UsePalSphere.h"
    ga_cpp          = src_root / "Private" / "Gas" / "Abilities" / "PaldarkGameplayAbility_UsePalSphere.cpp"
    roster_h        = src_root / "Public"  / "Pal"            / "PaldarkPlayerPalRosterComponent.h"
    roster_cpp      = src_root / "Private" / "Pal"            / "PaldarkPlayerPalRosterComponent.cpp"
    pal_char_h      = src_root / "Public"  / "Pal"            / "PaldarkPalCharacter.h"
    pal_char_cpp    = src_root / "Private" / "Pal"            / "PaldarkPalCharacter.cpp"
    player_char_h   = src_root / "Public"  / "Player"         / "PaldarkCharacter.h"
    player_char_cpp = src_root / "Private" / "Player"         / "PaldarkCharacter.cpp"
    tame_cmds_cpp   = src_root / "Private" / "Pal"            / "PaldarkTameConsoleCommands.cpp"
    content_dir     = PROJECT_DIR / "Content" / "Paldark" / "Tame"

    # (1) AttributeSet — Stun + MaxStun.
    if attr_h.is_file():
        attr_text = attr_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("FGameplayAttributeData Stun",     r"FGameplayAttributeData\s+Stun\b"),
            ("FGameplayAttributeData MaxStun",  r"FGameplayAttributeData\s+MaxStun\b"),
            ("ATTRIBUTE_ACCESSORS(... Stun)",   r"ATTRIBUTE_ACCESSORS\s*\(\s*UPaldarkAttributeSet\s*,\s*Stun\s*\)"),
            ("ATTRIBUTE_ACCESSORS(... MaxStun)",r"ATTRIBUTE_ACCESSORS\s*\(\s*UPaldarkAttributeSet\s*,\s*MaxStun\s*\)"),
            ("OnRep_Stun",                      r"void\s+OnRep_Stun\b"),
            ("OnRep_MaxStun",                   r"void\s+OnRep_MaxStun\b"),
        ):
            if not re.search(pattern, attr_text):
                fail(
                    f"{attr_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W35-36 — required so the capture-"
                    "probability formula can read the StunPct ratio)."
                )

    if attr_cpp.is_file():
        attr_cpp_text = attr_cpp.read_text(encoding="utf-8")
        for label in ("Stun", "MaxStun"):
            if f"DOREPLIFETIME_CONDITION_NOTIFY(UPaldarkAttributeSet, {label}" not in attr_cpp_text:
                fail(
                    f"{attr_cpp.relative_to(REPO_ROOT)} must register "
                    f"`DOREPLIFETIME_CONDITION_NOTIFY(UPaldarkAttributeSet, "
                    f"{label}, ...)` (W35-36 — without it the RepNotify "
                    "callback never fires on clients)."
                )

    # (2) UPaldarkPalBondComponent — real.
    if bond_h.is_file():
        bond_text = bond_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("AddBondXP server API",          r"void\s+AddBondXP\b"),
            ("SetBondLevel server API",       r"void\s+SetBondLevel\b"),
            ("int32 BondLevel",               r"int32\s+BondLevel\b"),
            ("float BondXP",                  r"float\s+BondXP\b"),
            ("OnBondLevelChanged delegate",   r"FOnPaldarkBondLevelChanged\s+OnBondLevelChanged\b"),
            ("FRuntimeFloatCurve BondXPCurve",r"FRuntimeFloatCurve\s+BondXPCurve\b"),
            ("OnRep_BondLevel callback",      r"void\s+OnRep_BondLevel\b"),
        ):
            if not re.search(pattern, bond_text):
                fail(
                    f"{bond_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W35-36 — bond component flips from "
                    "W3-4 stub to real this milestone)."
                )

    if bond_cpp.is_file():
        bond_cpp_text = bond_cpp.read_text(encoding="utf-8")
        for label in ("BondLevel", "BondXP"):
            if f"DOREPLIFETIME(UPaldarkPalBondComponent, {label})" not in bond_cpp_text:
                fail(
                    f"{bond_cpp.relative_to(REPO_ROOT)} must register "
                    f"`DOREPLIFETIME(UPaldarkPalBondComponent, {label})` "
                    "(W35-36 — bond state has to replicate to HUD)."
                )

    # (3) UPaldarkPalTameComponent — file + shape.
    if not tame_h.is_file():
        fail(
            f"missing {tame_h.relative_to(REPO_ROOT)} "
            "(W35-36 — UPaldarkPalTameComponent owns BeginTameAttempt + "
            "the capture-probability formula; the projectile can't "
            "resolve without it)."
        )
        return
    if not tame_cpp.is_file():
        fail(
            f"missing {tame_cpp.relative_to(REPO_ROOT)} "
            "(W35-36 — UPaldarkPalTameComponent needs a .cpp for the "
            "formula evaluation + RegisterAndDestroy tail)."
        )

    tame_text = tame_h.read_text(encoding="utf-8")
    if not re.search(
        r"class\s+PALDARKLAB_API\s+UPaldarkPalTameComponent\s*:\s*public\s+UActorComponent",
        tame_text,
    ):
        fail(
            f"{tame_h.relative_to(REPO_ROOT)} must declare "
            "`class PALDARKLAB_API UPaldarkPalTameComponent : public "
            "UActorComponent` (W35-36 — component slot pattern)."
        )
    for label, pattern in (
        ("bool bIsTameable",                          r"bool\s+bIsTameable\b"),
        ("float BaseCaptureProbability",              r"float\s+BaseCaptureProbability\b"),
        ("float TameDifficulty",                      r"float\s+TameDifficulty\b"),
        ("float MinCaptureProbability",               r"float\s+MinCaptureProbability\b"),
        ("float MaxCaptureProbability",               r"float\s+MaxCaptureProbability\b"),
        ("float FailStunDrainFraction",               r"float\s+FailStunDrainFraction\b"),
        ("BeginTameAttempt server API",               r"EPaldarkTameResult\s+BeginTameAttempt\b"),
        ("ForceTame server API",                      r"void\s+ForceTame\b"),
        ("OnTameAttemptResolved delegate",            r"FOnPaldarkTameAttemptResolved\s+OnTameAttemptResolved\b"),
    ):
        if not re.search(pattern, tame_text):
            fail(
                f"{tame_h.relative_to(REPO_ROOT)} is missing UPROPERTY "
                f"or function `{label}` (W35-36 — required for the "
                "capture-probability contract)."
            )

    if tame_cpp.is_file():
        tame_cpp_text = tame_cpp.read_text(encoding="utf-8")
        for needle, hint in (
            ("FMath::Clamp",      "formula clamps to [MinCaptureProbability, MaxCaptureProbability]"),
            ("FMath::FRand",      "formula rolls an RNG sample"),
            ("RegisterAndDestroy","success path registers to roster + destroys Pal"),
        ):
            if needle not in tame_cpp_text:
                fail(
                    f"{tame_cpp.relative_to(REPO_ROOT)} must invoke "
                    f"`{needle}` (W35-36 — {hint})."
                )

    # (4) UPaldarkPalDefinition — TameDifficulty + BaseCaptureProbability.
    if pal_def_h.is_file():
        pal_def_text = pal_def_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("float TameDifficulty",          r"float\s+TameDifficulty\b"),
            ("float BaseCaptureProbability",  r"float\s+BaseCaptureProbability\b"),
        ):
            if not re.search(pattern, pal_def_text):
                fail(
                    f"{pal_def_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W35-36 — per-species capture tuning "
                    "lives on the W27-28 data asset, not in C++)."
                )

    # (5) APaldarkPalSphere — projectile actor.
    if not sphere_h.is_file():
        fail(
            f"missing {sphere_h.relative_to(REPO_ROOT)} "
            "(W35-36 — APaldarkPalSphere is the replicated projectile "
            "that routes hits to the tame component)."
        )
    else:
        sphere_text = sphere_h.read_text(encoding="utf-8")
        if not re.search(
            r"class\s+PALDARKLAB_API\s+APaldarkPalSphere\s*:\s*public\s+AActor",
            sphere_text,
        ):
            fail(
                f"{sphere_h.relative_to(REPO_ROOT)} must declare "
                "`class PALDARKLAB_API APaldarkPalSphere : public AActor` "
                "(W35-36 — base AActor required for replication + "
                "level spawn)."
            )
        if "UProjectileMovementComponent" not in sphere_text:
            fail(
                f"{sphere_h.relative_to(REPO_ROOT)} must reference "
                "`UProjectileMovementComponent` (W35-36 — projectile "
                "movement is the standard sphere-in-flight contract)."
            )
        if not re.search(r"FGameplayTag\s+PalSphereTierTag\b", sphere_text):
            fail(
                f"{sphere_h.relative_to(REPO_ROOT)} must declare "
                "`FGameplayTag PalSphereTierTag` (W35-36 — projectile "
                "carries the tier so the tame formula's multiplier knows "
                "T1/T2/T3)."
            )

    if sphere_cpp.is_file():
        sphere_cpp_text = sphere_cpp.read_text(encoding="utf-8")
        if "BeginTameAttempt" not in sphere_cpp_text:
            fail(
                f"{sphere_cpp.relative_to(REPO_ROOT)} must call "
                "`BeginTameAttempt` on overlap/hit (W35-36 — that is the "
                "only entry point to the tame formula)."
            )

    # (6) UPaldarkGameplayAbility_UsePalSphere.
    if not ga_h.is_file():
        fail(
            f"missing {ga_h.relative_to(REPO_ROOT)} "
            "(W35-36 — Use Pal Sphere GA is the server-side ability "
            "that consumes inventory + spawns the projectile)."
        )
    else:
        ga_text = ga_h.read_text(encoding="utf-8")
        if not re.search(
            r"class\s+PALDARKLAB_API\s+UPaldarkGameplayAbility_UsePalSphere\s*:\s*public\s+UPaldarkGameplayAbility",
            ga_text,
        ):
            fail(
                f"{ga_h.relative_to(REPO_ROOT)} must declare "
                "`class PALDARKLAB_API UPaldarkGameplayAbility_UsePalSphere "
                ": public UPaldarkGameplayAbility` (W35-36 — Pal Sphere "
                "GA inherits the project's shared GA base)."
            )
        for label, pattern in (
            ("FGameplayTag PalSphereTierTag",       r"FGameplayTag\s+PalSphereTierTag\b"),
            ("TSubclassOf<APaldarkPalSphere>",      r"TSubclassOf<\s*APaldarkPalSphere\s*>"),
        ):
            if not re.search(pattern, ga_text):
                fail(
                    f"{ga_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W35-36 — required GA wiring)."
                )

    if ga_cpp.is_file():
        ga_cpp_text = ga_cpp.read_text(encoding="utf-8")
        if "SpawnActorDeferred<APaldarkPalSphere>" not in ga_cpp_text and "SpawnActor<APaldarkPalSphere>" not in ga_cpp_text:
            fail(
                f"{ga_cpp.relative_to(REPO_ROOT)} must spawn the "
                "`APaldarkPalSphere` projectile (W35-36 — GA's whole job "
                "is to instantiate the sphere on the authority)."
            )

    # (7) UPaldarkPlayerPalRosterComponent — roster.
    if not roster_h.is_file():
        fail(
            f"missing {roster_h.relative_to(REPO_ROOT)} "
            "(W35-36 — roster component hosts the player's tamed Pal "
            "list; without it TameComponent has nowhere to register)."
        )
    else:
        roster_text = roster_h.read_text(encoding="utf-8")
        if not re.search(
            r"class\s+PALDARKLAB_API\s+UPaldarkPlayerPalRosterComponent\s*:\s*public\s+UActorComponent",
            roster_text,
        ):
            fail(
                f"{roster_h.relative_to(REPO_ROOT)} must declare "
                "`class PALDARKLAB_API UPaldarkPlayerPalRosterComponent : "
                "public UActorComponent` (W35-36 — component slot "
                "pattern)."
            )
        if not re.search(r"struct\s+PALDARKLAB_API\s+FPaldarkRosterEntry\b", roster_text):
            fail(
                f"{roster_h.relative_to(REPO_ROOT)} must declare "
                "`struct PALDARKLAB_API FPaldarkRosterEntry` (W35-36 — "
                "replicated row type for the roster array)."
            )
        for label, pattern in (
            ("TArray<FPaldarkRosterEntry> Entries", r"TArray<\s*FPaldarkRosterEntry\s*>\s*Entries\b"),
            ("TamePal server API",                  r"void\s+TamePal\b"),
            ("DumpToLog helper",                    r"void\s+DumpToLog\b"),
        ):
            if not re.search(pattern, roster_text):
                fail(
                    f"{roster_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W35-36 — required roster contract)."
                )

    if roster_cpp.is_file():
        roster_cpp_text = roster_cpp.read_text(encoding="utf-8")
        if "DOREPLIFETIME_CONDITION(UPaldarkPlayerPalRosterComponent, Entries, COND_OwnerOnly)" not in roster_cpp_text:
            fail(
                f"{roster_cpp.relative_to(REPO_ROOT)} must register "
                "`DOREPLIFETIME_CONDITION(UPaldarkPlayerPalRosterComponent, "
                "Entries, COND_OwnerOnly)` (W35-36 — owner-only privacy "
                "gate prevents other players seeing your roster)."
            )

    # (8) Player + Pal character wiring.
    if pal_char_h.is_file():
        pal_h_text = pal_char_h.read_text(encoding="utf-8")
        if not re.search(
            r"TObjectPtr<\s*UPaldarkPalTameComponent\s*>\s*TameSlot\b",
            pal_h_text,
        ):
            fail(
                f"{pal_char_h.relative_to(REPO_ROOT)} must declare "
                "`TObjectPtr<UPaldarkPalTameComponent> TameSlot` "
                "(W35-36 — 11th Pal component slot)."
            )
        if "GetTameSlot" not in pal_h_text:
            fail(
                f"{pal_char_h.relative_to(REPO_ROOT)} must expose a "
                "`GetTameSlot()` accessor (W35-36 — every other Pal "
                "slot exposes one; console commands rely on it)."
            )
    if pal_char_cpp.is_file():
        pal_cpp_text = pal_char_cpp.read_text(encoding="utf-8")
        if "CreateDefaultSubobject<UPaldarkPalTameComponent>" not in pal_cpp_text:
            fail(
                f"{pal_char_cpp.relative_to(REPO_ROOT)} must "
                "`CreateDefaultSubobject<UPaldarkPalTameComponent>` for "
                "TameSlot (W35-36 — without it hostile-Pal subclasses "
                "have no tame listener)."
            )

    if player_char_h.is_file():
        player_h_text = player_char_h.read_text(encoding="utf-8")
        if not re.search(
            r"TObjectPtr<\s*UPaldarkPlayerPalRosterComponent\s*>\s*RosterSlot\b",
            player_h_text,
        ):
            fail(
                f"{player_char_h.relative_to(REPO_ROOT)} must declare "
                "`TObjectPtr<UPaldarkPlayerPalRosterComponent> RosterSlot` "
                "(W35-36 — 13th player component slot)."
            )
        if "GetRosterSlot" not in player_h_text:
            fail(
                f"{player_char_h.relative_to(REPO_ROOT)} must expose a "
                "`GetRosterSlot()` accessor (W35-36 — every other "
                "player slot exposes one; console commands rely on it)."
            )
    if player_char_cpp.is_file():
        player_cpp_text = player_char_cpp.read_text(encoding="utf-8")
        if "CreateDefaultSubobject<UPaldarkPlayerPalRosterComponent>" not in player_cpp_text:
            fail(
                f"{player_char_cpp.relative_to(REPO_ROOT)} must "
                "`CreateDefaultSubobject<UPaldarkPlayerPalRosterComponent>` "
                "for RosterSlot (W35-36 — without it the tame component "
                "has nowhere to register successful tames)."
            )

    # (9) Hostile-Pal subclasses flip bIsTameable = true.
    hostile_subclasses = (
        "PaldarkPalCharacter_Direhound.cpp",
        "PaldarkPalCharacter_Razorbird.cpp",
        "PaldarkPalCharacter_Stoneclad.cpp",
        "PaldarkPalCharacter_Vinewraith.cpp",
        "PaldarkPalCharacter_Boltmane.cpp",
    )
    for filename in hostile_subclasses:
        path = src_root / "Private" / "Pal" / filename
        if not path.is_file():
            fail(
                f"missing {path.relative_to(REPO_ROOT)} "
                "(W29-30 — hostile-Pal subclass expected by W35-36 "
                "tame check)."
            )
            continue
        text = path.read_text(encoding="utf-8")
        if "bIsTameable" not in text:
            fail(
                f"{path.relative_to(REPO_ROOT)} must flip "
                "`bIsTameable = true` on TameSlot in ctor (W35-36 — "
                "without it Pal Sphere hits are no-ops on this species)."
            )

    # (10) Console commands TU + 4 cmd registrations.
    if not tame_cmds_cpp.is_file():
        fail(
            f"missing {tame_cmds_cpp.relative_to(REPO_ROOT)} "
            "(W35-36 — console commands TU hosts Paldark.Tame.Force + "
            "Paldark.Tame.DumpRoster + Paldark.Tame.SetStun + "
            "Paldark.Pal.Bond.AddXP)."
        )
    else:
        cmds_text = tame_cmds_cpp.read_text(encoding="utf-8")
        for cmd_name in (
            "Paldark.Tame.Force",
            "Paldark.Tame.DumpRoster",
            "Paldark.Tame.SetStun",
            "Paldark.Pal.Bond.AddXP",
        ):
            if cmd_name not in cmds_text:
                fail(
                    f"{tame_cmds_cpp.relative_to(REPO_ROOT)} must "
                    f"register the `{cmd_name}` console command (W35-36 "
                    "— designer needs this for tame minigame "
                    "verification)."
                )

    # (11) Content/Paldark/Tame/ placeholder directory.
    if not content_dir.is_dir():
        fail(
            f"missing {content_dir.relative_to(REPO_ROOT)}/ directory "
            "(W35-36 — designer drops DA_Item_PalSphere_T1/T2/T3 + "
            "GE_StunOnHit + GE_StunDecay here; ship the directory with "
            "a README.txt placeholder so designer workflow is bootstrapped)."
        )


def check_w37_inventory_full_shape() -> None:
    """W37-38 — Inventory full scaffold.

    Ten cooperating pieces, all stuctural (header presence + symbol +
    a couple of canary regexes). Designer-authored DataAssets are NOT
    validated — content lives under /Content/Paldark/Inventory/{Backpacks,
    Containers} and the placeholder README is enough.

      1. UPaldarkItemFragment_Backpack {.h,.cpp} — backpack fragment with
         BackpackTierTag + MaxWeightBonusKg + ExtraSlots.
      2. UPaldarkItemFragment_Container {.h,.cpp} — container fragment
         with InnerMaxWeightKg + InnerMaxSlots (clamped 0-16).
      3. FPaldarkInventoryEntry gains TArray<FPaldarkInventoryEntry>
         InnerEntries so a row can carry a nested container.
      4. UPaldarkPlayerInventoryComponent:
           - BaseMaxWeightKg replicated (renamed from MaxWeightKg).
           - EquippedBackpack TSoftObjectPtr replicated + OnRep callback.
           - SetEquippedBackpack server API.
           - GetMaxWeightKg() definition (not inline).
           - GetExtraSlotsFromBackpack() helper.
           - DumpCompositeToLog() debug function.
      5. APaldarkCharacter:
           - TSubclassOf<APaldarkLootBag> PlayerDeathBagClass UPROPERTY.
           - SpawnDeathLootBagFromInventory() definition.
           - Death hook lambda invokes SpawnDeathLootBagFromInventory.
      6. PaldarkLab.cpp registers 4 new console commands:
           Paldark.Inventory.GiveAll / EquipBackpack /
           TestDeathDrop / DumpComposite.
      7. EXPECTED_GAMEPLAY_TAGS already enforced — 17 item tags + 3
         backpack tiers + 1 equip slot.
      8. Content/Paldark/Inventory/Backpacks + .../Containers
         placeholder directories.
    """
    src_root = PROJECT_DIR / "Source" / "PaldarkLab"

    bp_frag_h     = src_root / "Public"  / "Inventory" / "Fragments" / "PaldarkItemFragment_Backpack.h"
    bp_frag_cpp   = src_root / "Private" / "Inventory" / "Fragments" / "PaldarkItemFragment_Backpack.cpp"
    ct_frag_h     = src_root / "Public"  / "Inventory" / "Fragments" / "PaldarkItemFragment_Container.h"
    ct_frag_cpp   = src_root / "Private" / "Inventory" / "Fragments" / "PaldarkItemFragment_Container.cpp"
    inv_h         = src_root / "Public"  / "Player" / "Components" / "PaldarkPlayerInventoryComponent.h"
    inv_cpp       = src_root / "Private" / "Player" / "Components" / "PaldarkPlayerInventoryComponent.cpp"
    char_h        = src_root / "Public"  / "Player" / "PaldarkCharacter.h"
    char_cpp      = src_root / "Private" / "Player" / "PaldarkCharacter.cpp"
    module_cpp    = src_root / "PaldarkLab.cpp"
    backpack_dir  = PROJECT_DIR / "Content" / "Paldark" / "Inventory" / "Backpacks"
    container_dir = PROJECT_DIR / "Content" / "Paldark" / "Inventory" / "Containers"

    # (1) Backpack fragment files.
    if not bp_frag_h.is_file():
        fail(
            f"missing {bp_frag_h.relative_to(REPO_ROOT)} "
            "(W37-38 — UPaldarkItemFragment_Backpack supplies the "
            "MaxWeightBonusKg + ExtraSlots metadata used by "
            "UPaldarkPlayerInventoryComponent::GetMaxWeightKg)."
        )
    else:
        bp_text = bp_frag_h.read_text(encoding="utf-8")
        if not re.search(
            r"class\s+PALDARKLAB_API\s+UPaldarkItemFragment_Backpack\s*:\s*public\s+UPaldarkItemFragment",
            bp_text,
        ):
            fail(
                f"{bp_frag_h.relative_to(REPO_ROOT)} must declare "
                "`class PALDARKLAB_API UPaldarkItemFragment_Backpack : "
                "public UPaldarkItemFragment` (W37-38 — fragment "
                "composition pattern from W11-12)."
            )
        for label, pattern in (
            ("FGameplayTag BackpackTierTag", r"FGameplayTag\s+BackpackTierTag\b"),
            ("float MaxWeightBonusKg",       r"float\s+MaxWeightBonusKg\b"),
            ("int32 ExtraSlots",             r"int32\s+ExtraSlots\b"),
            ("GetMaxWeightBonusKg() accessor", r"float\s+GetMaxWeightBonusKg\s*\("),
            ("GetExtraSlots() accessor",     r"int32\s+GetExtraSlots\s*\("),
        ):
            if not re.search(pattern, bp_text):
                fail(
                    f"{bp_frag_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W37-38 — required Backpack fragment "
                    "field/accessor)."
                )

    if not bp_frag_cpp.is_file():
        fail(
            f"missing {bp_frag_cpp.relative_to(REPO_ROOT)} "
            "(W37-38 — Backpack fragment needs a .cpp for the debug "
            "description override)."
        )

    # (2) Container fragment files.
    if not ct_frag_h.is_file():
        fail(
            f"missing {ct_frag_h.relative_to(REPO_ROOT)} "
            "(W37-38 — UPaldarkItemFragment_Container marks an item as "
            "having a nested inner inventory, with InnerMaxWeightKg + "
            "InnerMaxSlots tuning)."
        )
    else:
        ct_text = ct_frag_h.read_text(encoding="utf-8")
        if not re.search(
            r"class\s+PALDARKLAB_API\s+UPaldarkItemFragment_Container\s*:\s*public\s+UPaldarkItemFragment",
            ct_text,
        ):
            fail(
                f"{ct_frag_h.relative_to(REPO_ROOT)} must declare "
                "`class PALDARKLAB_API UPaldarkItemFragment_Container : "
                "public UPaldarkItemFragment` (W37-38 — fragment "
                "composition pattern from W11-12)."
            )
        for label, pattern in (
            ("float InnerMaxWeightKg", r"float\s+InnerMaxWeightKg\b"),
            ("int32 InnerMaxSlots",    r"int32\s+InnerMaxSlots\b"),
            ("GetInnerMaxWeightKg()",  r"float\s+GetInnerMaxWeightKg\s*\("),
            ("GetInnerMaxSlots()",     r"int32\s+GetInnerMaxSlots\s*\("),
        ):
            if not re.search(pattern, ct_text):
                fail(
                    f"{ct_frag_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W37-38 — required Container fragment "
                    "field/accessor)."
                )
        # Cap InnerMaxSlots ≤ 16 to keep DOREPLIFETIME blob under MTU.
        # The header should advertise that ceiling either via comment or
        # a ClampMax meta. Accept either; the runtime clamp in .cpp is the
        # real defence.
        if "16" not in ct_text:
            fail(
                f"{ct_frag_h.relative_to(REPO_ROOT)} must reference the "
                "InnerMaxSlots ceiling `16` (W37-38 — caps DOREPLIFETIME "
                "blob below MTU; FastArraySerializer migration is W47)."
            )

    if not ct_frag_cpp.is_file():
        fail(
            f"missing {ct_frag_cpp.relative_to(REPO_ROOT)} "
            "(W37-38 — Container fragment needs a .cpp for the debug "
            "description override)."
        )

    # (3) FPaldarkInventoryEntry — InnerEntries.
    if inv_h.is_file():
        inv_text = inv_h.read_text(encoding="utf-8")
        if not re.search(
            r"TArray<\s*FPaldarkInventoryEntry\s*>\s*InnerEntries\b",
            inv_text,
        ):
            fail(
                f"{inv_h.relative_to(REPO_ROOT)} must declare "
                "`TArray<FPaldarkInventoryEntry> InnerEntries` inside "
                "FPaldarkInventoryEntry (W37-38 — nested rows are how a "
                "container item carries its sub-inventory inside the "
                "outer Entries replication)."
            )

        # (4) Component shape.
        for label, pattern in (
            ("BaseMaxWeightKg replicated",   r"float\s+BaseMaxWeightKg\b"),
            ("EquippedBackpack soft ptr",    r"TSoftObjectPtr<\s*UPaldarkItemDefinition\s*>\s*EquippedBackpack\b"),
            ("SetEquippedBackpack server API", r"bool\s+SetEquippedBackpack\b"),
            ("GetEquippedBackpack accessor", r"GetEquippedBackpack\s*\("),
            ("GetExtraSlotsFromBackpack",    r"int32\s+GetExtraSlotsFromBackpack\b"),
            ("OnRep_EquippedBackpack",       r"void\s+OnRep_EquippedBackpack\b"),
            ("DumpCompositeToLog",           r"void\s+DumpCompositeToLog\b"),
        ):
            if not re.search(pattern, inv_text):
                fail(
                    f"{inv_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W37-38 — required for backpack + "
                    "composite-container wiring)."
                )

    if inv_cpp.is_file():
        inv_cpp_text = inv_cpp.read_text(encoding="utf-8")
        for label, pattern in (
            ("BaseMaxWeightKg DOREPLIFETIME",
             r"DOREPLIFETIME\s*\(\s*UPaldarkPlayerInventoryComponent\s*,\s*BaseMaxWeightKg\s*\)"),
            ("EquippedBackpack DOREPLIFETIME",
             r"DOREPLIFETIME\s*\(\s*UPaldarkPlayerInventoryComponent\s*,\s*EquippedBackpack\s*\)"),
            ("GetMaxWeightKg sums backpack bonus",
             r"GetMaxWeightBonusKg\s*\("),
        ):
            if not re.search(pattern, inv_cpp_text):
                fail(
                    f"{inv_cpp.relative_to(REPO_ROOT)} must reference "
                    f"`{label}` (W37-38 — required replication / "
                    "weight-cap wiring)."
                )

    # (5) Player character drop-on-death.
    if char_h.is_file():
        char_h_text = char_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("PlayerDeathBagClass UPROPERTY",
             r"TSubclassOf<\s*APaldarkLootBag\s*>\s*PlayerDeathBagClass\b"),
            ("SpawnDeathLootBagFromInventory decl",
             r"void\s+SpawnDeathLootBagFromInventory\s*\("),
        ):
            if not re.search(pattern, char_h_text):
                fail(
                    f"{char_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W37-38 — drop-on-death wiring)."
                )

    if char_cpp.is_file():
        char_cpp_text = char_cpp.read_text(encoding="utf-8")
        if "PlayerDeathBagClass = APaldarkLootBag::StaticClass" not in char_cpp_text:
            fail(
                f"{char_cpp.relative_to(REPO_ROOT)} must default "
                "`PlayerDeathBagClass` to `APaldarkLootBag::StaticClass()` "
                "in the constructor (W37-38 — designers shouldn't have "
                "to wire it per-Blueprint)."
            )
        if "SpawnDeathLootBagFromInventory" not in char_cpp_text:
            fail(
                f"{char_cpp.relative_to(REPO_ROOT)} must define + invoke "
                "`SpawnDeathLootBagFromInventory()` (W37-38 — the W24-25 "
                "death hook is the only path that triggers the bag)."
            )
        # The death-hook lambda must invoke the spawn helper *before*
        # `RecordDeath` (the match subsystem may destroy the pawn, which
        # would invalidate the inventory).
        spawn_idx  = char_cpp_text.find("SpawnDeathLootBagFromInventory()")
        record_idx = char_cpp_text.find("MatchSub->RecordDeath")
        if spawn_idx > 0 and record_idx > 0 and spawn_idx > record_idx:
            fail(
                f"{char_cpp.relative_to(REPO_ROOT)} invokes "
                "`SpawnDeathLootBagFromInventory()` AFTER "
                "`MatchSub->RecordDeath` (W37-38 — must drop bag first "
                "so the inventory snapshot survives a potential pawn "
                "destroy in the match flow)."
            )

    # (6) PaldarkLab.cpp registers 4 new console commands.
    if module_cpp.is_file():
        module_text = module_cpp.read_text(encoding="utf-8")
        for cmd_name in (
            "Paldark.Inventory.GiveAll",
            "Paldark.Inventory.EquipBackpack",
            "Paldark.Inventory.TestDeathDrop",
            "Paldark.Inventory.DumpComposite",
        ):
            if cmd_name not in module_text:
                fail(
                    f"{module_cpp.relative_to(REPO_ROOT)} must register "
                    f"the `{cmd_name}` console command (W37-38 — "
                    "designer needs this for inventory verification)."
                )

    # (7) Content placeholder directories.
    if not backpack_dir.is_dir():
        fail(
            f"missing {backpack_dir.relative_to(REPO_ROOT)}/ directory "
            "(W37-38 — designer drops DA_Item_Backpack_T1/T2/T3 here; "
            "ship the directory with a README.txt placeholder so "
            "designer workflow is bootstrapped)."
        )
    if not container_dir.is_dir():
        fail(
            f"missing {container_dir.relative_to(REPO_ROOT)}/ directory "
            "(W37-38 — designer drops DA_Item_LootCrate / similar "
            "container DAs here; ship the directory with a README.txt "
            "placeholder so designer workflow is bootstrapped)."
        )

    # ---- W39 — L-32 scaffold (FastArraySerializer migration prep) ---------
    # Q3 Bug-Bash L-32 (P2) scaffold landed in W39. The migration itself
    # (swapping `Entries` to a FastArraySerializer-backed array) is still
    # deferred to W47, but the symbols below MUST stay in the codebase so
    # the W47 PR is a focused storage swap rather than a callsite hunt.
    #
    #   1. FPaldarkInventoryEntry declares `int32 MigrationReplicationKey`
    #      (NotReplicated + Transient) so the W47 PR can remove this and
    #      route callers through `FFastArraySerializerItem::ReplicationKey`.
    #   2. UPaldarkPlayerInventoryComponent declares the static
    #      `BuildNextReplicationKey()` counter and the
    #      `MarkInventoryDirty(int32 EntryIndex = INDEX_NONE)` funnel.
    #   3. The .cpp implements both helpers AND replaces every previous
    #      `OnInventoryChanged.Broadcast(this)` callsite with
    #      `MarkInventoryDirty(...)` so the W47 migration is a one-line
    #      body swap rather than a grep.
    #
    # The check is intentionally cheap (regex presence) — it just freezes
    # the scaffold contract so a future refactor can't silently un-do it.

    # (8) Header — MigrationReplicationKey field + scaffold helpers.
    if inv_h.is_file():
        inv_text = inv_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("MigrationReplicationKey UPROPERTY",
             r"int32\s+MigrationReplicationKey\b"),
            ("BuildNextReplicationKey static counter",
             r"static\s+int32\s+BuildNextReplicationKey\s*\("),
            ("MarkInventoryDirty funnel",
             r"void\s+MarkInventoryDirty\s*\(\s*int32\s+EntryIndex\s*=\s*INDEX_NONE\s*\)"),
        ):
            if not re.search(pattern, inv_text):
                fail(
                    f"{inv_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W39 — L-32 scaffold; FastArraySerializer "
                    "migration prep, see README W39 § L-32 closure)."
                )
        # MigrationReplicationKey UPROPERTY MUST carry NotReplicated +
        # Transient so the outer DOREPLIFETIME blob doesn't pay 4 bytes
        # per row for unused data, and save-game (W47) doesn't persist it.
        # Anchor the regex on the UPROPERTY block immediately preceding
        # the `int32 MigrationReplicationKey` field declaration so the
        # check ignores any earlier doc-comment references to the field.
        rep_key_match = re.search(
            r"UPROPERTY\s*\(([^)]*)\)\s*int32\s+MigrationReplicationKey\b",
            inv_text,
        )
        if rep_key_match is None:
            fail(
                f"{inv_h.relative_to(REPO_ROOT)} must declare the "
                "`MigrationReplicationKey` field with a UPROPERTY block "
                "directly above it (W39 — L-32 scaffold)."
            )
        else:
            rep_key_block = rep_key_match.group(1)
            for flag in ("NotReplicated", "Transient"):
                if flag not in rep_key_block:
                    fail(
                        f"{inv_h.relative_to(REPO_ROOT)} must mark "
                        f"`MigrationReplicationKey` UPROPERTY as `{flag}` "
                        "(W39 — L-32 scaffold; NotReplicated keeps the "
                        "field out of the DOREPLIFETIME blob, Transient "
                        "keeps it out of save-game)."
                    )

    # (9) .cpp — atomic counter, AddItem stamping, broadcast funnel.
    if inv_cpp.is_file():
        inv_cpp_text = inv_cpp.read_text(encoding="utf-8")
        for label, pattern in (
            ("PaldarkInventoryMigration namespace + GReplicationKeyCounter",
             r"namespace\s+PaldarkInventoryMigration\b[\s\S]*?GReplicationKeyCounter\b"),
            ("BuildNextReplicationKey definition",
             r"int32\s+UPaldarkPlayerInventoryComponent::BuildNextReplicationKey\s*\("),
            ("FPlatformAtomics::InterlockedIncrement on counter",
             r"FPlatformAtomics::InterlockedIncrement\s*\(\s*&\s*PaldarkInventoryMigration::GReplicationKeyCounter\s*\)"),
            ("AddItem stamps MigrationReplicationKey",
             r"NewRow\.MigrationReplicationKey\s*=\s*BuildNextReplicationKey\s*\("),
            ("MarkInventoryDirty definition",
             r"void\s+UPaldarkPlayerInventoryComponent::MarkInventoryDirty\s*\(\s*int32\s+EntryIndex\s*\)"),
            ("MarkInventoryDirty bounds-check log tag",
             r"\[L-32\]\[Inventory\.MarkDirty\]"),
        ):
            if not re.search(pattern, inv_cpp_text):
                fail(
                    f"{inv_cpp.relative_to(REPO_ROOT)} must contain "
                    f"`{label}` (W39 — L-32 scaffold; required for the "
                    "W47 FastArraySerializer body-swap to be a one-line "
                    "change)."
                )
        # The funnel-conversion contract: every previous broadcast
        # callsite MUST route through MarkInventoryDirty now. Allow at
        # most one residual `OnInventoryChanged.Broadcast(this)` inside
        # MarkInventoryDirty itself (the actual delegate fire).
        # Strip `//` line-comments before counting so commented-out
        # example calls in doc-blocks (e.g. the W47 migration plan inside
        # MarkInventoryDirty) don't tip the count over the threshold.
        stripped_cpp = re.sub(r"//[^\n]*", "", inv_cpp_text)
        residual_broadcasts = re.findall(
            r"OnInventoryChanged\.Broadcast\s*\(\s*this\s*\)",
            stripped_cpp,
        )
        if len(residual_broadcasts) != 1:
            fail(
                f"{inv_cpp.relative_to(REPO_ROOT)} should have exactly "
                f"ONE `OnInventoryChanged.Broadcast(this)` call (inside "
                "MarkInventoryDirty); found "
                f"{len(residual_broadcasts)}. (W39 — L-32 scaffold; "
                "Add/Remove/Drop/SetEquippedBackpack/OnRep_* must route "
                "through MarkInventoryDirty so the W47 body-swap is "
                "one line.)"
            )


def check_w40_hub_shape() -> None:
    """W40-41 — Hub Town scaffold.

    Eight cooperating pieces, all structural (header presence + symbol +
    canary regexes). Designer-authored DataAssets and the actual .umap
    are NOT validated — content lives under /Content/Paldark/Hub/ and the
    designer assembles `DA_HubBuilding_<Type>` + `DA_MapDef_HubTown` +
    `DA_ExperienceDef_Hub` + `Map_HubTown.umap` in the UE Editor.

      1. UPaldarkHubBuildingDefinition {.h,.cpp} — PrimaryDataAsset under
         Public/Hub/. Per-instance designer config (BuildingTag,
         DisplayName, InteractRadius, InteractInputTag, RosterSlotCount,
         MarketplaceCatalog, AllowedRaidMaps).
      2. APaldarkHubBuilding {.h,.cpp} — base actor under Public/Hub/.
         Holds the soft ref to UPaldarkHubBuildingDefinition + the
         interact sphere + BeginPlay/EndPlay register hooks.
      3. APaldarkPalStable {.h,.cpp} — subclass under Public/Hub/.
      4. APaldarkMarketplaceKiosk {.h,.cpp} — subclass under Public/Hub/.
      5. APaldarkBriefingRoom {.h,.cpp} — subclass under Public/Hub/.
         Issues the deferred ServerTravel via HostHubServer.
      6. UPaldarkHubSubsystem {.h,.cpp} — UWorldSubsystem under Public/Hub/.
         Server-only registry with Initialize sweep + OnWorldBeginPlay
         retry sweep. Registers 3 console commands.
      7. APaldarkGameMode_Hub {.h,.cpp} — under Public/Framework/. Bumps
         MaxPlayersPerSession to 8 in its ctor.
      8. UPaldarkExperienceDefinition gains `int32 MaxPlayers` UPROPERTY.
         APaldarkGameModeBase::OnExperienceAssetReady seeds
         MaxPlayersPerSession from this field.
      9. UPaldarkNetSubsystem gains HostHubServer wrapper.
     10. DefaultGame.ini AssetManager scan rule for PaldarkHubBuilding.
    """
    src_root = PROJECT_DIR / "Source" / "PaldarkLab"

    # File map: each entry is (path, expected class symbol regex).
    expected_files = [
        (src_root / "Public"  / "Hub" / "PaldarkHubBuildingDefinition.h",
         r"class\s+PALDARKLAB_API\s+UPaldarkHubBuildingDefinition\s*:\s*public\s+UPrimaryDataAsset"),
        (src_root / "Private" / "Hub" / "PaldarkHubBuildingDefinition.cpp", None),
        (src_root / "Public"  / "Hub" / "PaldarkHubBuilding.h",
         r"class\s+PALDARKLAB_API\s+APaldarkHubBuilding\s*:\s*public\s+AActor"),
        (src_root / "Private" / "Hub" / "PaldarkHubBuilding.cpp", None),
        (src_root / "Public"  / "Hub" / "PaldarkPalStable.h",
         r"class\s+PALDARKLAB_API\s+APaldarkPalStable\s*:\s*public\s+APaldarkHubBuilding"),
        (src_root / "Private" / "Hub" / "PaldarkPalStable.cpp", None),
        (src_root / "Public"  / "Hub" / "PaldarkMarketplaceKiosk.h",
         r"class\s+PALDARKLAB_API\s+APaldarkMarketplaceKiosk\s*:\s*public\s+APaldarkHubBuilding"),
        (src_root / "Private" / "Hub" / "PaldarkMarketplaceKiosk.cpp", None),
        (src_root / "Public"  / "Hub" / "PaldarkBriefingRoom.h",
         r"class\s+PALDARKLAB_API\s+APaldarkBriefingRoom\s*:\s*public\s+APaldarkHubBuilding"),
        (src_root / "Private" / "Hub" / "PaldarkBriefingRoom.cpp", None),
        (src_root / "Public"  / "Hub" / "PaldarkHubSubsystem.h",
         r"class\s+PALDARKLAB_API\s+UPaldarkHubSubsystem\s*:\s*public\s+UWorldSubsystem"),
        (src_root / "Private" / "Hub" / "PaldarkHubSubsystem.cpp", None),
        (src_root / "Public"  / "Framework" / "PaldarkGameMode_Hub.h",
         r"class\s+PALDARKLAB_API\s+APaldarkGameMode_Hub\s*:\s*public\s+APaldarkGameModeBase"),
        (src_root / "Private" / "Framework" / "PaldarkGameMode_Hub.cpp", None),
    ]
    for path, class_regex in expected_files:
        if not path.is_file():
            fail(
                f"missing {path.relative_to(REPO_ROOT)} "
                "(W40-41 — Hub Town scaffold; see PaldarkLab/README.md § W40-41)."
            )
            continue
        if class_regex is not None:
            text = path.read_text(encoding="utf-8")
            if not re.search(class_regex, text):
                fail(
                    f"{path.relative_to(REPO_ROOT)} must declare the "
                    f"expected UCLASS matching `{class_regex}` "
                    "(W40-41 — Hub Town scaffold)."
                )

    # (1) Hub building definition — per-instance config fields.
    bld_def_h = src_root / "Public" / "Hub" / "PaldarkHubBuildingDefinition.h"
    if bld_def_h.is_file():
        text = bld_def_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("FGameplayTag BuildingTag",            r"FGameplayTag\s+BuildingTag\b"),
            ("FText DisplayName",                   r"FText\s+DisplayName\b"),
            ("float InteractRadius",                r"float\s+InteractRadius\b"),
            ("FGameplayTag InteractInputTag",       r"FGameplayTag\s+InteractInputTag\b"),
            ("int32 RosterSlotCount",               r"int32\s+RosterSlotCount\b"),
            ("MarketplaceCatalog soft array",
             r"TArray<\s*TSoftObjectPtr<\s*UPaldarkItemDefinition\s*>\s*>\s*MarketplaceCatalog\b"),
            ("AllowedRaidMaps soft array",
             r"TArray<\s*TSoftObjectPtr<\s*UPaldarkMapDefinition\s*>\s*>\s*AllowedRaidMaps\b"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{bld_def_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W40-41 — required Hub Building "
                    "Definition field; see README W40-41 § field map)."
                )

    bld_def_cpp = src_root / "Private" / "Hub" / "PaldarkHubBuildingDefinition.cpp"
    if bld_def_cpp.is_file():
        text = bld_def_cpp.read_text(encoding="utf-8")
        if "PaldarkHubBuilding" not in text:
            fail(
                f"{bld_def_cpp.relative_to(REPO_ROOT)} must return the "
                "`PaldarkHubBuilding` PrimaryAssetType from "
                "GetPrimaryAssetId() (W40-41 — keys AssetManager scan "
                "rule in DefaultGame.ini)."
            )

    # (2) Hub building base actor — soft ref + sphere + register hooks.
    bld_h = src_root / "Public" / "Hub" / "PaldarkHubBuilding.h"
    if bld_h.is_file():
        text = bld_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("BuildingDefinition soft ptr",
             r"TSoftObjectPtr<\s*UPaldarkHubBuildingDefinition\s*>\s*BuildingDefinition\b"),
            ("BuildingTag UPROPERTY",         r"FGameplayTag\s+BuildingTag\b"),
            ("OnInteract virtual",            r"virtual\s+void\s+OnInteract\s*\("),
            ("InteractSphere object ptr",
             r"TObjectPtr<\s*USphereComponent\s*>\s*InteractSphere\b"),
            ("BeginPlay override",            r"virtual\s+void\s+BeginPlay\s*\(\s*\)\s+override"),
            ("EndPlay override",              r"virtual\s+void\s+EndPlay\s*\("),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{bld_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W40-41 — Hub Building base scaffold)."
                )

    bld_cpp = src_root / "Private" / "Hub" / "PaldarkHubBuilding.cpp"
    if bld_cpp.is_file():
        text = bld_cpp.read_text(encoding="utf-8")
        for label, pattern in (
            ("RegisterHubBuilding called in BeginPlay",
             r"Subsystem->RegisterHubBuilding\s*\(\s*this\s*\)"),
            ("UnregisterHubBuilding called in EndPlay",
             r"Subsystem->UnregisterHubBuilding\s*\(\s*this\s*\)"),
            ("HasAuthority guard in BeginPlay",
             r"if\s*\(\s*!HasAuthority\s*\(\s*\)\s*\)"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{bld_cpp.relative_to(REPO_ROOT)} must contain "
                    f"`{label}` (W40-41 — server-only registration + "
                    "BeginPlay/EndPlay hooks)."
                )

    # (3-5) Three subclasses stamp BuildingTag in their ctor.
    for subclass_cpp, expected_tag in (
        (src_root / "Private" / "Hub" / "PaldarkPalStable.cpp",        "TAG_Paldark_Hub_Building_PalStable"),
        (src_root / "Private" / "Hub" / "PaldarkMarketplaceKiosk.cpp", "TAG_Paldark_Hub_Building_Marketplace"),
        (src_root / "Private" / "Hub" / "PaldarkBriefingRoom.cpp",     "TAG_Paldark_Hub_Building_BriefingRoom"),
    ):
        if subclass_cpp.is_file():
            text = subclass_cpp.read_text(encoding="utf-8")
            if not re.search(
                rf"BuildingTag\s*=\s*PaldarkGameplayTags::{re.escape(expected_tag)}\b",
                text,
            ):
                fail(
                    f"{subclass_cpp.relative_to(REPO_ROOT)} must stamp "
                    f"`BuildingTag = PaldarkGameplayTags::{expected_tag}` "
                    "in its ctor (W40-41 — Hub Building type tag; the "
                    "subsystem's GetBuildingsByTag query keys on this)."
                )

    # Briefing room is the only subclass with real behaviour — must wire
    # the deferred HostHubServer ServerTravel.
    brief_cpp = src_root / "Private" / "Hub" / "PaldarkBriefingRoom.cpp"
    if brief_cpp.is_file():
        text = brief_cpp.read_text(encoding="utf-8")
        for label, pattern in (
            ("GetPendingRaidMapTag query",
             r"Hub->GetPendingRaidMapTag\s*\(\s*\)"),
            ("HostHubServer call",
             r"HostHubServer\s*\("),
            ("SetTimerForNextTick deferred travel",
             r"SetTimerForNextTick\s*\("),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{brief_cpp.relative_to(REPO_ROOT)} must contain "
                    f"`{label}` (W40-41 — briefing-room handoff into "
                    "raid map via HostHubServer)."
                )

    # (6) Hub subsystem — server-only gating + sweep + console commands.
    sub_h = src_root / "Public" / "Hub" / "PaldarkHubSubsystem.h"
    if sub_h.is_file():
        text = sub_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("ShouldCreateSubsystem override",
             r"virtual\s+bool\s+ShouldCreateSubsystem\s*\("),
            ("Initialize override",
             r"virtual\s+void\s+Initialize\s*\("),
            ("OnWorldBeginPlay override",
             r"virtual\s+void\s+OnWorldBeginPlay\s*\("),
            ("RegisterHubBuilding API",
             r"void\s+RegisterHubBuilding\s*\(\s*APaldarkHubBuilding\b"),
            ("UnregisterHubBuilding API",
             r"void\s+UnregisterHubBuilding\s*\(\s*APaldarkHubBuilding\b"),
            ("GetBuildingsByTag query",
             r"GetBuildingsByTag\s*\(\s*const\s+FGameplayTag\s*&"),
            ("GetPendingRaidMapTag accessor",
             r"GetPendingRaidMapTag\s*\("),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{sub_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W40-41 — Hub Subsystem shape; "
                    "mirrors W31-32 POI subsystem)."
                )

    sub_cpp = src_root / "Private" / "Hub" / "PaldarkHubSubsystem.cpp"
    if sub_cpp.is_file():
        text = sub_cpp.read_text(encoding="utf-8")
        for label, pattern in (
            ("Server-only NetMode gate (DedicatedServer)", r"NM_DedicatedServer"),
            ("Server-only NetMode gate (ListenServer)",    r"NM_ListenServer"),
            ("Server-only NetMode gate (Standalone)",      r"NM_Standalone"),
            ("Initialize sweep call",
             r"SweepWorldForBuildings\s*\("),
            ("TActorIterator world sweep",
             r"TActorIterator<\s*APaldarkHubBuilding\s*>"),
            ("Paldark.Hub.List console",
             r"\"Paldark\.Hub\.List\""),
            ("Paldark.Hub.QueueRaid console",
             r"\"Paldark\.Hub\.QueueRaid\""),
            ("Paldark.Hub.Status console",
             r"\"Paldark\.Hub\.Status\""),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{sub_cpp.relative_to(REPO_ROOT)} must contain "
                    f"`{label}` (W40-41 — required Hub Subsystem wiring; "
                    "see W31-32 POI subsystem for the template shape)."
                )

    # (7) Hub GameMode — ctor bumps MaxPlayersPerSession to 8.
    hub_gm_cpp = src_root / "Private" / "Framework" / "PaldarkGameMode_Hub.cpp"
    if hub_gm_cpp.is_file():
        text = hub_gm_cpp.read_text(encoding="utf-8")
        if not re.search(r"MaxPlayersPerSession\s*=\s*8\b", text):
            fail(
                f"{hub_gm_cpp.relative_to(REPO_ROOT)} must set "
                "`MaxPlayersPerSession = 8` in its ctor (W40-41 — "
                "roadmap Q4 § Tuần 40-41 \"8-player shard\" line item; "
                "the hub experience can still override per-session via "
                "UPaldarkExperienceDefinition::MaxPlayers)."
            )

    # (8) Experience definition gains MaxPlayers field.
    exp_h = src_root / "Public" / "Experience" / "PaldarkExperienceDefinition.h"
    if exp_h.is_file():
        text = exp_h.read_text(encoding="utf-8")
        if not re.search(r"int32\s+MaxPlayers\b", text):
            fail(
                f"{exp_h.relative_to(REPO_ROOT)} must declare "
                "`int32 MaxPlayers` UPROPERTY (W40-41 — per-experience "
                "shard size; raid keeps default 4, hub overrides to 8)."
            )

    # APaldarkGameModeBase::OnExperienceAssetReady must seed
    # MaxPlayersPerSession from the experience's MaxPlayers.
    gm_base_cpp = src_root / "Private" / "Framework" / "PaldarkGameModeBase.cpp"
    if gm_base_cpp.is_file():
        text = gm_base_cpp.read_text(encoding="utf-8")
        if not re.search(
            r"MaxPlayersPerSession\s*=\s*Experience->MaxPlayers\b",
            text,
        ):
            fail(
                f"{gm_base_cpp.relative_to(REPO_ROOT)} must assign "
                "`MaxPlayersPerSession = Experience->MaxPlayers` inside "
                "OnExperienceAssetReady (W40-41 — seeds shard cap from "
                "the loaded experience, mirrors the L-21 contract)."
            )

    # (9) NetSubsystem gains HostHubServer wrapper.
    net_h = src_root / "Public" / "Net" / "PaldarkNetSubsystem.h"
    if net_h.is_file():
        text = net_h.read_text(encoding="utf-8")
        if not re.search(
            r"bool\s+HostHubServer\s*\(\s*const\s+FString\s*&\s*MapName",
            text,
        ):
            fail(
                f"{net_h.relative_to(REPO_ROOT)} must declare "
                "`bool HostHubServer(const FString& MapName, const FString& ExperienceId = TEXT(\"\"))` "
                "(W40-41 — briefing-room handoff entry point + W42-43 "
                "AWS fleet picker seam)."
            )

    net_cpp = src_root / "Private" / "Net" / "PaldarkNetSubsystem.cpp"
    if net_cpp.is_file():
        text = net_cpp.read_text(encoding="utf-8")
        if not re.search(
            r"UPaldarkNetSubsystem::HostHubServer\b",
            text,
        ):
            fail(
                f"{net_cpp.relative_to(REPO_ROOT)} must define "
                "`UPaldarkNetSubsystem::HostHubServer` (W40-41 — "
                "wrapper around HostListenServer with `[W40-41][Hub]` "
                "log prefix; W42-43 AWS branches on this entry point)."
            )
        if "[W40-41][Hub]" not in text:
            fail(
                f"{net_cpp.relative_to(REPO_ROOT)} must log the "
                "`[W40-41][Hub]` prefix in HostHubServer (W40-41 — "
                "designers grep this prefix to distinguish hub vs raid "
                "host attempts)."
            )

    # (10) DefaultGame.ini AssetManager scan rule.
    default_game = PROJECT_DIR / "Config" / "DefaultGame.ini"
    if default_game.is_file():
        text = default_game.read_text(encoding="utf-8")
        if "PrimaryAssetType=\"PaldarkHubBuilding\"" not in text:
            fail(
                f"{default_game.relative_to(REPO_ROOT)} must include a "
                "`+PrimaryAssetTypesToScan` entry for "
                "`PaldarkHubBuilding` rooted at "
                "`/Game/Paldark/Hub` (W40-41 — AssetManager picks up "
                "DA_HubBuilding_<Type> data assets)."
            )
        if "PaldarkHubBuildingDefinition" not in text:
            fail(
                f"{default_game.relative_to(REPO_ROOT)} must reference "
                "`PaldarkHubBuildingDefinition` as the scan rule's "
                "AssetBaseClass (W40-41)."
            )


def check_w42_backend_shape() -> None:
    """W42-43 — AWS backend integration scaffold.

    Eight cooperating pieces, all structural (header presence + symbol +
    canary regexes). Actual Lambda deployment / Cognito User Pool
    provisioning is SRE work that lands later (W46+). The mocked fallback
    path (`bUseAWSBackend=False`) ships green in this PR so the C++ build
    + designer quick-test loop keep working without AWS.

      1. PaldarkBackendTypes.h — 3 USTRUCTs (FPaldarkSessionToken,
         FPaldarkFleetSpec, FPaldarkBackendError) wired BlueprintType for
         UMG binding.
      2. UPaldarkBackendSettings {.h,.cpp} — UDeveloperSettings with the
         AWS knobs (bUseAWSBackend, AWSRegion, LambdaApiBaseUrl, Cognito
         pool/client ids, request timeout, retries, backoff, token-refresh
         lead).
      3. UPaldarkBackendSubsystem {.h,.cpp} — UGameInstanceSubsystem with
         6 API entry points (Login / Logout / RequestHubFleet /
         RequestRaidFleet / RefreshToken / GetStatusReport), 4
         BlueprintAssignable delegates, 4 console commands.
      4. UPaldarkNetSubsystem extends with HostRaidServer +
         JoinFleetByFleetSpec + RequestFleetAndTravel.
      5. PaldarkLab.Build.cs adds HTTP / Json / JsonUtilities /
         DeveloperSettings as PrivateDependencyModuleNames.
      6. PaldarkLogCategories.h declares LogPaldarkBackend; the core
         module's StartupModule logs "6 log categories".
      7. DefaultGame.ini has `[/Script/PaldarkLab.PaldarkBackendSettings]`
         block with 9 default keys.
      8. 12 gameplay tags in `PaldarkGameplayTags.cpp` (validated by
         `check_gameplay_tags`).
    """
    src_root = PROJECT_DIR / "Source" / "PaldarkLab"

    # (1) PaldarkBackendTypes.h — 3 structs + IsValid()/MakeLoopback().
    types_h = src_root / "Public" / "Backend" / "PaldarkBackendTypes.h"
    if not types_h.is_file():
        fail(
            f"missing {types_h.relative_to(REPO_ROOT)} "
            "(W42-43 — session token / fleet spec / error structs; see "
            "PaldarkLab/README.md § W42-43)."
        )
    else:
        text = types_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("FPaldarkSessionToken struct",
             r"struct\s+PALDARKLAB_API\s+FPaldarkSessionToken\b"),
            ("FPaldarkFleetSpec struct",
             r"struct\s+PALDARKLAB_API\s+FPaldarkFleetSpec\b"),
            ("FPaldarkBackendError struct",
             r"struct\s+PALDARKLAB_API\s+FPaldarkBackendError\b"),
            ("FPaldarkFleetSpec::MakeLoopback",
             r"static\s+FPaldarkFleetSpec\s+MakeLoopback\s*\("),
            ("FPaldarkSessionToken::IsValid",
             r"bool\s+IsValid\s*\(\s*\)\s*const\b"),
            ("USTRUCT(BlueprintType) wiring",
             r"USTRUCT\s*\(\s*BlueprintType\s*\)"),
            ("FString AccessToken field",
             r"FString\s+AccessToken\b"),
            ("FString RefreshToken field",
             r"FString\s+RefreshToken\b"),
            ("FString FleetId field",
             r"FString\s+FleetId\b"),
            ("FString IP field",
             r"FString\s+IP\b"),
            ("int32 Port field",
             r"int32\s+Port\b"),
            ("FGameplayTag FleetTag field",
             r"FGameplayTag\s+FleetTag\b"),
            ("FGameplayTag ResultTag field",
             r"FGameplayTag\s+ResultTag\b"),
            ("int32 StatusCode field",
             r"int32\s+StatusCode\b"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{types_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W42-43 — backend types shape; see "
                    "README W42-43 § struct map)."
                )

    # (2) UPaldarkBackendSettings — UDeveloperSettings with 9 knobs.
    settings_h = src_root / "Public" / "Backend" / "PaldarkBackendSettings.h"
    if not settings_h.is_file():
        fail(
            f"missing {settings_h.relative_to(REPO_ROOT)} "
            "(W42-43 — UDeveloperSettings exposing AWS knobs)."
        )
    else:
        text = settings_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("UPaldarkBackendSettings UCLASS",
             r"class\s+PALDARKLAB_API\s+UPaldarkBackendSettings\s*:\s*public\s+UDeveloperSettings"),
            ("config=Game UCLASS specifier",
             r"UCLASS\s*\([^)]*[Cc]onfig\s*=\s*Game"),
            ("bUseAWSBackend knob",
             r"bool\s+bUseAWSBackend\b"),
            ("AWSRegion knob",
             r"FString\s+AWSRegion\b"),
            ("LambdaApiBaseUrl knob",
             r"FString\s+LambdaApiBaseUrl\b"),
            ("CognitoUserPoolId knob",
             r"FString\s+CognitoUserPoolId\b"),
            ("CognitoAppClientId knob",
             r"FString\s+CognitoAppClientId\b"),
            ("RequestTimeoutSeconds knob",
             r"float\s+RequestTimeoutSeconds\b"),
            ("MaxRetries knob",
             r"int32\s+MaxRetries\b"),
            ("RetryBackoffSeconds knob",
             r"float\s+RetryBackoffSeconds\b"),
            ("TokenRefreshLeadSeconds knob",
             r"int32\s+TokenRefreshLeadSeconds\b"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{settings_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W42-43 — UPaldarkBackendSettings knob)."
                )

    settings_cpp = src_root / "Private" / "Backend" / "PaldarkBackendSettings.cpp"
    if not settings_cpp.is_file():
        fail(
            f"missing {settings_cpp.relative_to(REPO_ROOT)} "
            "(W42-43 — UDeveloperSettings minimal ctor body)."
        )

    # (3) UPaldarkBackendSubsystem — 6 API methods + 4 delegates + 4 console.
    sub_h = src_root / "Public" / "Backend" / "PaldarkBackendSubsystem.h"
    if not sub_h.is_file():
        fail(
            f"missing {sub_h.relative_to(REPO_ROOT)} "
            "(W42-43 — UGameInstanceSubsystem wrapping AWS HTTP calls)."
        )
    else:
        text = sub_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("UPaldarkBackendSubsystem UCLASS",
             r"class\s+PALDARKLAB_API\s+UPaldarkBackendSubsystem\s*:\s*public\s+UGameInstanceSubsystem"),
            ("Login API",
             r"void\s+Login\s*\("),
            ("Logout API",
             r"void\s+Logout\s*\("),
            ("RequestHubFleet API",
             r"void\s+RequestHubFleet\s*\("),
            ("RequestRaidFleet API",
             r"void\s+RequestRaidFleet\s*\("),
            ("RefreshToken API",
             r"void\s+RefreshToken\s*\("),
            ("GetStatusReport API",
             r"FString\s+GetStatusReport\s*\("),
            ("OnLoginCompleted BlueprintAssignable",
             r"FPaldarkBackendLoginSignature\s+OnLoginCompleted\b"),
            ("OnFleetRequestCompleted BlueprintAssignable",
             r"FPaldarkBackendFleetRequestSignature\s+OnFleetRequestCompleted\b"),
            ("OnTokenRefreshed BlueprintAssignable",
             r"FPaldarkBackendTokenRefreshedSignature\s+OnTokenRefreshed\b"),
            ("OnBackendError BlueprintAssignable",
             r"FPaldarkBackendErrorSignature\s+OnBackendError\b"),
            ("BlueprintAssignable UPROPERTY",
             r"UPROPERTY\s*\(\s*BlueprintAssignable"),
            ("Initialize override",
             r"virtual\s+void\s+Initialize\s*\("),
            ("Deinitialize override",
             r"virtual\s+void\s+Deinitialize\s*\("),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{sub_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W42-43 — backend subsystem shape)."
                )

    sub_cpp = src_root / "Private" / "Backend" / "PaldarkBackendSubsystem.cpp"
    if not sub_cpp.is_file():
        fail(
            f"missing {sub_cpp.relative_to(REPO_ROOT)} "
            "(W42-43 — backend subsystem implementation)."
        )
    else:
        text = sub_cpp.read_text(encoding="utf-8")
        for label, pattern in (
            ("Paldark.Backend.Login console",
             r"\"Paldark\.Backend\.Login\""),
            ("Paldark.Backend.RequestHubFleet console",
             r"\"Paldark\.Backend\.RequestHubFleet\""),
            ("Paldark.Backend.RequestRaidFleet console",
             r"\"Paldark\.Backend\.RequestRaidFleet\""),
            ("Paldark.Backend.Status console",
             r"\"Paldark\.Backend\.Status\""),
            ("LogPaldarkBackend usage",
             r"LogPaldarkBackend"),
            ("Mocked fallback returns MakeLoopback",
             r"FPaldarkFleetSpec::MakeLoopback\s*\("),
            ("State Tag transition logged",
             r"TransitionState\s*\("),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{sub_cpp.relative_to(REPO_ROOT)} must contain "
                    f"`{label}` (W42-43 — backend subsystem impl)."
                )

    # (4) NetSubsystem extensions — HostRaidServer / JoinFleetByFleetSpec.
    net_h = src_root / "Public" / "Net" / "PaldarkNetSubsystem.h"
    if net_h.is_file():
        text = net_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("HostRaidServer declaration",
             r"bool\s+HostRaidServer\s*\(\s*const\s+FString\s*&\s*MapName"),
            ("JoinFleetByFleetSpec declaration",
             r"bool\s+JoinFleetByFleetSpec\s*\(\s*const\s+FPaldarkFleetSpec\s*&"),
            ("UPaldarkBackendSubsystem forward decl",
             r"class\s+UPaldarkBackendSubsystem\b"),
            ("PaldarkBackendTypes include",
             r"#include\s+\"Backend/PaldarkBackendTypes\.h\""),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{net_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W42-43 — NetSubsystem backend hook)."
                )

    net_cpp = src_root / "Private" / "Net" / "PaldarkNetSubsystem.cpp"
    if net_cpp.is_file():
        text = net_cpp.read_text(encoding="utf-8")
        for label, pattern in (
            ("PaldarkBackendSubsystem include",
             r"#include\s+\"Backend/PaldarkBackendSubsystem\.h\""),
            ("HostRaidServer definition",
             r"UPaldarkNetSubsystem::HostRaidServer\b"),
            ("JoinFleetByFleetSpec definition",
             r"UPaldarkNetSubsystem::JoinFleetByFleetSpec\b"),
            ("RequestFleetAndTravel helper",
             r"UPaldarkNetSubsystem::RequestFleetAndTravel\b"),
            ("Paldark.Net.HostHub console",
             r"\"Paldark\.Net\.HostHub\""),
            ("Paldark.Net.HostRaid console",
             r"\"Paldark\.Net\.HostRaid\""),
            ("Backend->RequestHubFleet usage",
             r"Backend->RequestHubFleet\s*\("),
            ("Backend->RequestRaidFleet usage",
             r"Backend->RequestRaidFleet\s*\("),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{net_cpp.relative_to(REPO_ROOT)} must contain "
                    f"`{label}` (W42-43 — NetSubsystem backend hook)."
                )

    # (5) Build.cs adds HTTP / Json / JsonUtilities / DeveloperSettings.
    build_cs = src_root / "PaldarkLab.Build.cs"
    if build_cs.is_file():
        text = build_cs.read_text(encoding="utf-8")
        for dep in ("HTTP", "Json", "JsonUtilities", "DeveloperSettings"):
            if f"\"{dep}\"" not in text:
                fail(
                    f"{build_cs.relative_to(REPO_ROOT)} must list `\"{dep}\"` "
                    "in PrivateDependencyModuleNames (W42-43 — required for "
                    "FHttpModule / FJsonObjectConverter / UDeveloperSettings)."
                )

    # (6) LogPaldarkBackend category declared + defined.
    log_h = PROJECT_DIR / "Source" / "PaldarkLabCore" / "Public" / "PaldarkLogCategories.h"
    if log_h.is_file():
        text = log_h.read_text(encoding="utf-8")
        if "LogPaldarkBackend" not in text:
            fail(
                f"{log_h.relative_to(REPO_ROOT)} must declare "
                "`LogPaldarkBackend` (W42-43 — backend log category)."
            )

    log_cpp = PROJECT_DIR / "Source" / "PaldarkLabCore" / "Private" / "PaldarkLabCore.cpp"
    if log_cpp.is_file():
        text = log_cpp.read_text(encoding="utf-8")
        if "DEFINE_LOG_CATEGORY(LogPaldarkBackend)" not in text:
            fail(
                f"{log_cpp.relative_to(REPO_ROOT)} must define "
                "`LogPaldarkBackend` via DEFINE_LOG_CATEGORY "
                "(W42-43 — backend log category)."
            )

    # (7) DefaultGame.ini contains the settings block with 9 default keys.
    default_game = PROJECT_DIR / "Config" / "DefaultGame.ini"
    if default_game.is_file():
        text = default_game.read_text(encoding="utf-8")
        if "[/Script/PaldarkLab.PaldarkBackendSettings]" not in text:
            fail(
                f"{default_game.relative_to(REPO_ROOT)} must include the "
                "`[/Script/PaldarkLab.PaldarkBackendSettings]` block "
                "(W42-43 — surfaced via Project Settings UI)."
            )
        for key in (
            "bUseAWSBackend",
            "AWSRegion",
            "LambdaApiBaseUrl",
            "CognitoUserPoolId",
            "CognitoAppClientId",
            "RequestTimeoutSeconds",
            "MaxRetries",
            "RetryBackoffSeconds",
            "TokenRefreshLeadSeconds",
        ):
            if not re.search(rf"^{key}\s*=", text, flags=re.MULTILINE):
                fail(
                    f"{default_game.relative_to(REPO_ROOT)} must include "
                    f"a `{key}=` default in the "
                    "`[/Script/PaldarkLab.PaldarkBackendSettings]` block "
                    "(W42-43 — backend settings default)."
                )
        if re.search(
            r"^bUseAWSBackend\s*=\s*True",
            text,
            flags=re.MULTILINE | re.IGNORECASE,
        ):
            fail(
                f"{default_game.relative_to(REPO_ROOT)} must keep "
                "`bUseAWSBackend=False` in the default config "
                "(W42-43 — never enable the real AWS path on cold-clone "
                "checkouts; staging/production override via per-config "
                "DefaultGame_<Config>.ini)."
            )


def check_w44_stable_marketplace_shape() -> None:
    """W44-45 — Stable + Marketplace UI scaffold.

    Eight cooperating pieces, all structural (header presence + symbol +
    canary regexes). Designer authors the actual UMG widgets
    (`WBP_PaldarkStableWidget`, `WBP_PaldarkMarketplaceWidget`) as
    Blueprint subclasses of the C++ stubs.

      1. UPaldarkItemFragment_MarketValue {.h,.cpp} — UPaldarkItemFragment
         subclass with BuyPriceCredits / SellPriceCredits / bAllowBuy /
         bAllowSell. Attached per-item via Fragments array.
      2. UPaldarkPalDepositComponent {.h,.cpp} — UActorComponent on
         PlayerState. Holds TArray<FPaldarkRosterEntry> DepositedEntries
         (replicated COND_OwnerOnly) + MaxDepositSlots cap.
      3. UPaldarkStableController {.h,.cpp} — UActorComponent on
         PlayerController. Server RPCs (Deposit/Withdraw/Heal) + Client
         result RPC + 3 BlueprintAssignable delegates + HealPriceCredits
         designer knob.
      4. UPaldarkMarketplaceController {.h,.cpp} — UActorComponent on
         PlayerController. Server RPCs (Buy/Sell) + Client result RPC + 3
         BlueprintAssignable delegates + async catalog load via
         FStreamableManager.
      5. UPaldarkStableWidget / UPaldarkMarketplaceWidget {.h,.cpp} —
         UUserWidget stubs with BlueprintImplementableEvent hooks the
         designer's Blueprint subclasses bind to.
      6. APaldarkPlayerController instantiates both controllers as
         CreateDefaultSubobject in its ctor; exposes accessors.
      7. APaldarkPlayerState instantiates the deposit component as
         CreateDefaultSubobject in its ctor; exposes accessor.
      8. APaldarkPalStable::OnInteract + APaldarkMarketplaceKiosk::OnInteract
         both route via the per-player controller component (no longer
         no-ops as in W40-41).
    """
    src_root = PROJECT_DIR / "Source" / "PaldarkLab"

    expected_files = [
        # (1) MarketValue fragment.
        (src_root / "Public"  / "Inventory" / "Fragments" / "PaldarkItemFragment_MarketValue.h",
         r"class\s+PALDARKLAB_API\s+UPaldarkItemFragment_MarketValue\s*:\s*public\s+UPaldarkItemFragment"),
        (src_root / "Private" / "Inventory" / "Fragments" / "PaldarkItemFragment_MarketValue.cpp", None),
        # (2) Pal deposit storage.
        (src_root / "Public"  / "Pal" / "PaldarkPalDepositComponent.h",
         r"class\s+PALDARKLAB_API\s+UPaldarkPalDepositComponent\s*:\s*public\s+UActorComponent"),
        (src_root / "Private" / "Pal" / "PaldarkPalDepositComponent.cpp", None),
        # (3) Stable controller.
        (src_root / "Public"  / "Hub" / "PaldarkStableController.h",
         r"class\s+PALDARKLAB_API\s+UPaldarkStableController\s*:\s*public\s+UActorComponent"),
        (src_root / "Private" / "Hub" / "PaldarkStableController.cpp", None),
        # (4) Marketplace controller.
        (src_root / "Public"  / "Hub" / "PaldarkMarketplaceController.h",
         r"class\s+PALDARKLAB_API\s+UPaldarkMarketplaceController\s*:\s*public\s+UActorComponent"),
        (src_root / "Private" / "Hub" / "PaldarkMarketplaceController.cpp", None),
        # (5) UMG widget stubs.
        (src_root / "Public"  / "Hub" / "PaldarkStableWidget.h",
         r"class\s+PALDARKLAB_API\s+UPaldarkStableWidget\s*:\s*public\s+UUserWidget"),
        (src_root / "Private" / "Hub" / "PaldarkStableWidget.cpp", None),
        (src_root / "Public"  / "Hub" / "PaldarkMarketplaceWidget.h",
         r"class\s+PALDARKLAB_API\s+UPaldarkMarketplaceWidget\s*:\s*public\s+UUserWidget"),
        (src_root / "Private" / "Hub" / "PaldarkMarketplaceWidget.cpp", None),
    ]
    for path, class_regex in expected_files:
        if not path.is_file():
            fail(
                f"missing {path.relative_to(REPO_ROOT)} "
                "(W44-45 — Stable + Marketplace UI scaffold; see "
                "PaldarkLab/README.md § W44-45)."
            )
            continue
        if class_regex is not None:
            text = path.read_text(encoding="utf-8")
            if not re.search(class_regex, text):
                fail(
                    f"{path.relative_to(REPO_ROOT)} must declare the "
                    f"expected UCLASS matching `{class_regex}` "
                    "(W44-45 — Stable + Marketplace UI scaffold)."
                )

    # (1) MarketValue fragment — required pricing fields.
    mv_h = src_root / "Public" / "Inventory" / "Fragments" / "PaldarkItemFragment_MarketValue.h"
    if mv_h.is_file():
        text = mv_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("int32 BuyPriceCredits",
             r"int32\s+BuyPriceCredits\b"),
            ("int32 SellPriceCredits",
             r"int32\s+SellPriceCredits\b"),
            ("bool bAllowBuy",
             r"bool\s+bAllowBuy\b"),
            ("bool bAllowSell",
             r"bool\s+bAllowSell\b"),
            ("ClampMin = 0 on BuyPriceCredits/SellPriceCredits",
             r"ClampMin\s*=\s*\"0\""),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{mv_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W44-45 — MarketValue fragment field; "
                    "marketplace controller reads these per item)."
                )

    # (2) Pal deposit component — required API + replicated array.
    dep_h = src_root / "Public" / "Pal" / "PaldarkPalDepositComponent.h"
    if dep_h.is_file():
        text = dep_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("DepositEntry server API",
             r"int32\s+DepositEntry\s*\(\s*const\s+FPaldarkRosterEntry\s*&"),
            ("WithdrawEntry server API",
             r"FPaldarkRosterEntry\s+WithdrawEntry\s*\(\s*int32"),
            ("GetDepositedEntries accessor",
             r"const\s+TArray<\s*FPaldarkRosterEntry\s*>\s*&\s*GetDepositedEntries\s*\("),
            ("MaxDepositSlots designer knob",
             r"int32\s+MaxDepositSlots\b"),
            ("DepositedEntries ReplicatedUsing",
             r"UPROPERTY\s*\(\s*ReplicatedUsing\s*=\s*OnRep_DepositedEntries\s*\)"),
            ("GetLifetimeReplicatedProps override",
             r"virtual\s+void\s+GetLifetimeReplicatedProps\s*\("),
            ("DumpToLog helper",
             r"void\s+DumpToLog\s*\(\s*\)\s*const"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{dep_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W44-45 — Pal deposit component shape)."
                )

    dep_cpp = src_root / "Private" / "Pal" / "PaldarkPalDepositComponent.cpp"
    if dep_cpp.is_file():
        text = dep_cpp.read_text(encoding="utf-8")
        for label, pattern in (
            ("COND_OwnerOnly replication condition",
             r"COND_OwnerOnly"),
            ("DOREPLIFETIME_CONDITION on DepositedEntries",
             r"DOREPLIFETIME_CONDITION\s*\(\s*UPaldarkPalDepositComponent\s*,\s*DepositedEntries"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{dep_cpp.relative_to(REPO_ROOT)} must contain "
                    f"`{label}` (W44-45 — deposit list is per-player; "
                    "other players don't observe each other's deposits)."
                )

    # (3) Stable controller — RPC + delegate + designer knob shape.
    stable_h = src_root / "Public" / "Hub" / "PaldarkStableController.h"
    if stable_h.is_file():
        text = stable_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("RequestOpenStable client API",
             r"void\s+RequestOpenStable\s*\(\s*APaldarkPalStable\s*\*"),
            ("RequestDeposit client API",
             r"void\s+RequestDeposit\s*\(\s*int32"),
            ("RequestWithdraw client API",
             r"void\s+RequestWithdraw\s*\(\s*int32"),
            ("RequestHeal client API",
             r"void\s+RequestHeal\s*\(\s*int32"),
            ("Server_RequestDeposit Server RPC",
             r"UFUNCTION\s*\(\s*Server[^)]*\)\s*\n?\s*void\s+Server_RequestDeposit\b"),
            ("Server_RequestWithdraw Server RPC",
             r"UFUNCTION\s*\(\s*Server[^)]*\)\s*\n?\s*void\s+Server_RequestWithdraw\b"),
            ("Server_RequestHeal Server RPC",
             r"UFUNCTION\s*\(\s*Server[^)]*\)\s*\n?\s*void\s+Server_RequestHeal\b"),
            ("Client_ReceiveActionResult Client RPC",
             r"UFUNCTION\s*\(\s*Client[^)]*\)\s*\n?\s*void\s+Client_ReceiveActionResult\b"),
            ("OnStableActionResult BlueprintAssignable",
             r"FPaldarkStableActionResult\s+OnStableActionResult\b"),
            ("OnStableOpened BlueprintAssignable",
             r"FPaldarkStableOpened\s+OnStableOpened\b"),
            ("OnStableClosed BlueprintAssignable",
             r"FPaldarkStableClosed\s+OnStableClosed\b"),
            ("BlueprintAssignable UPROPERTY",
             r"UPROPERTY\s*\(\s*BlueprintAssignable"),
            ("HealPriceCredits designer knob",
             r"int32\s+HealPriceCredits\b"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{stable_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W44-45 — Stable controller shape)."
                )

    stable_cpp = src_root / "Private" / "Hub" / "PaldarkStableController.cpp"
    if stable_cpp.is_file():
        text = stable_cpp.read_text(encoding="utf-8")
        for label, pattern in (
            ("bIsTransactionInFlight guard",
             r"bIsTransactionInFlight"),
            ("Hub.Stable.Action.Deposit tag wired",
             r"TAG_Paldark_Hub_Stable_Action_Deposit"),
            ("Hub.Stable.Action.Withdraw tag wired",
             r"TAG_Paldark_Hub_Stable_Action_Withdraw"),
            ("Hub.Stable.Action.Heal tag wired",
             r"TAG_Paldark_Hub_Stable_Action_Heal"),
            ("Hub.Result.Success tag wired",
             r"TAG_Paldark_Hub_Result_Success"),
            ("LogPaldarkUI usage",
             r"LogPaldarkUI"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{stable_cpp.relative_to(REPO_ROOT)} must contain "
                    f"`{label}` (W44-45 — Stable controller impl)."
                )

    # (4) Marketplace controller — RPC + async load + catalog membership.
    mkt_h = src_root / "Public" / "Hub" / "PaldarkMarketplaceController.h"
    if mkt_h.is_file():
        text = mkt_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("RequestOpenMarketplace client API",
             r"void\s+RequestOpenMarketplace\s*\(\s*APaldarkMarketplaceKiosk\s*\*"),
            ("RequestBuy client API",
             r"void\s+RequestBuy\s*\(\s*FGameplayTag"),
            ("RequestSell client API",
             r"void\s+RequestSell\s*\(\s*FGameplayTag"),
            ("Server_RequestBuy Server RPC",
             r"UFUNCTION\s*\(\s*Server[^)]*\)\s*\n?\s*void\s+Server_RequestBuy\b"),
            ("Server_RequestSell Server RPC",
             r"UFUNCTION\s*\(\s*Server[^)]*\)\s*\n?\s*void\s+Server_RequestSell\b"),
            ("Client_ReceiveTransactionResult Client RPC",
             r"UFUNCTION\s*\(\s*Client[^)]*\)\s*\n?\s*void\s+Client_ReceiveTransactionResult\b"),
            ("OnMarketplaceActionResult BlueprintAssignable",
             r"FPaldarkMarketplaceActionResult\s+OnMarketplaceActionResult\b"),
            ("OnMarketplaceOpened BlueprintAssignable",
             r"FPaldarkMarketplaceOpened\s+OnMarketplaceOpened\b"),
            ("BlueprintAssignable UPROPERTY",
             r"UPROPERTY\s*\(\s*BlueprintAssignable"),
            ("CachedCatalog TArray",
             r"TArray<\s*TObjectPtr<\s*UPaldarkItemDefinition\s*>\s*>\s*CachedCatalog\b"),
            ("CatalogLoadHandle FStreamableHandle",
             r"TSharedPtr<\s*FStreamableHandle\s*>\s*CatalogLoadHandle\b"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{mkt_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W44-45 — Marketplace controller shape)."
                )

    mkt_cpp = src_root / "Private" / "Hub" / "PaldarkMarketplaceController.cpp"
    if mkt_cpp.is_file():
        text = mkt_cpp.read_text(encoding="utf-8")
        for label, pattern in (
            ("FStreamableManager async catalog load",
             r"\bRequestAsyncLoad\s*\("),
            ("FindFragmentByClass<UPaldarkItemFragment_MarketValue>",
             r"FindFragmentByClass\s*<\s*UPaldarkItemFragment_MarketValue\s*>"),
            ("Hub.Market.Action.Buy tag wired",
             r"TAG_Paldark_Hub_Market_Action_Buy"),
            ("Hub.Market.Action.Sell tag wired",
             r"TAG_Paldark_Hub_Market_Action_Sell"),
            ("Hub.Result.Fail.NotAllowed tag wired",
             r"TAG_Paldark_Hub_Result_Fail_NotAllowed"),
            ("Hub.Result.Fail.Insufficient tag wired",
             r"TAG_Paldark_Hub_Result_Fail_Insufficient"),
            ("LogPaldarkUI usage",
             r"LogPaldarkUI"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{mkt_cpp.relative_to(REPO_ROOT)} must contain "
                    f"`{label}` (W44-45 — Marketplace controller impl)."
                )

    # (5) UMG widget stubs — BlueprintImplementableEvent hooks.
    sw_h = src_root / "Public" / "Hub" / "PaldarkStableWidget.h"
    if sw_h.is_file():
        text = sw_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("K2_OnStableOpened BlueprintImplementableEvent",
             r"BlueprintImplementableEvent[^}]*?K2_OnStableOpened\b"),
            ("K2_OnStableActionResult BlueprintImplementableEvent",
             r"BlueprintImplementableEvent[^}]*?K2_OnStableActionResult\b"),
            ("GetStableController BlueprintPure",
             r"UPaldarkStableController\*\s+GetStableController\s*\("),
        ):
            if not re.search(pattern, text, flags=re.DOTALL):
                fail(
                    f"{sw_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W44-45 — Stable UMG widget stub)."
                )

    mw_h = src_root / "Public" / "Hub" / "PaldarkMarketplaceWidget.h"
    if mw_h.is_file():
        text = mw_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("K2_OnMarketplaceOpened BlueprintImplementableEvent",
             r"BlueprintImplementableEvent[^}]*?K2_OnMarketplaceOpened\b"),
            ("K2_OnMarketplaceActionResult BlueprintImplementableEvent",
             r"BlueprintImplementableEvent[^}]*?K2_OnMarketplaceActionResult\b"),
            ("GetMarketplaceController BlueprintPure",
             r"UPaldarkMarketplaceController\*\s+GetMarketplaceController\s*\("),
        ):
            if not re.search(pattern, text, flags=re.DOTALL):
                fail(
                    f"{mw_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W44-45 — Marketplace UMG widget stub)."
                )

    # (6) PlayerController instantiates both controllers + accessors.
    pc_h = src_root / "Public" / "Player" / "PaldarkPlayerController.h"
    if pc_h.is_file():
        text = pc_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("GetStableController accessor",
             r"UPaldarkStableController\*\s+GetStableController\s*\("),
            ("GetMarketplaceController accessor",
             r"UPaldarkMarketplaceController\*\s+GetMarketplaceController\s*\("),
            ("StableController member",
             r"TObjectPtr<\s*UPaldarkStableController\s*>\s+StableController\b"),
            ("MarketplaceController member",
             r"TObjectPtr<\s*UPaldarkMarketplaceController\s*>\s+MarketplaceController\b"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{pc_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W44-45 — PlayerController hosts the "
                    "per-player Hub UI controllers as default subobjects)."
                )

    pc_cpp = src_root / "Private" / "Player" / "PaldarkPlayerController.cpp"
    if pc_cpp.is_file():
        text = pc_cpp.read_text(encoding="utf-8")
        for label, pattern in (
            ("StableController CreateDefaultSubobject",
             r"StableController\s*=\s*CreateDefaultSubobject<\s*UPaldarkStableController\s*>"),
            ("MarketplaceController CreateDefaultSubobject",
             r"MarketplaceController\s*=\s*CreateDefaultSubobject<\s*UPaldarkMarketplaceController\s*>"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{pc_cpp.relative_to(REPO_ROOT)} must call "
                    f"`{label}` in the ctor (W44-45 — both Hub UI "
                    "controllers must ship by default; designer doesn't "
                    "attach them by hand)."
                )

    # (7) PlayerState instantiates deposit component + accessor.
    ps_h = src_root / "Public" / "Player" / "PaldarkPlayerState.h"
    if ps_h.is_file():
        text = ps_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("GetPalDepositComponent accessor",
             r"UPaldarkPalDepositComponent\*\s+GetPalDepositComponent\s*\("),
            ("PalDepositComponent member",
             r"TObjectPtr<\s*UPaldarkPalDepositComponent\s*>\s+PalDepositComponent\b"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{ps_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W44-45 — PlayerState hosts the per-"
                    "player deposit storage so it survives respawn)."
                )

    ps_cpp = src_root / "Private" / "Player" / "PaldarkPlayerState.cpp"
    if ps_cpp.is_file():
        text = ps_cpp.read_text(encoding="utf-8")
        if not re.search(
            r"PalDepositComponent\s*=\s*CreateDefaultSubobject<\s*UPaldarkPalDepositComponent\s*>",
            text,
        ):
            fail(
                f"{ps_cpp.relative_to(REPO_ROOT)} must call "
                "`PalDepositComponent = CreateDefaultSubobject<"
                "UPaldarkPalDepositComponent>(...)` in the ctor "
                "(W44-45 — deposit storage default subobject)."
            )

    # (8) Stable + Marketplace OnInteract route via per-player controller.
    stable_actor_cpp = src_root / "Private" / "Hub" / "PaldarkPalStable.cpp"
    if stable_actor_cpp.is_file():
        text = stable_actor_cpp.read_text(encoding="utf-8")
        for label, pattern in (
            ("Forwards to StableController->RequestOpenStable",
             r"Controller->RequestOpenStable\s*\(\s*this\s*\)"),
            ("GetStableController resolution",
             r"GetStableController\s*\(\s*\)"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{stable_actor_cpp.relative_to(REPO_ROOT)} must "
                    f"contain `{label}` (W44-45 — Stable OnInteract "
                    "delegates to the per-player controller component)."
                )

    market_actor_cpp = src_root / "Private" / "Hub" / "PaldarkMarketplaceKiosk.cpp"
    if market_actor_cpp.is_file():
        text = market_actor_cpp.read_text(encoding="utf-8")
        for label, pattern in (
            ("Forwards to MarketplaceController->RequestOpenMarketplace",
             r"Controller->RequestOpenMarketplace\s*\(\s*this\s*\)"),
            ("GetMarketplaceController resolution",
             r"GetMarketplaceController\s*\(\s*\)"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{market_actor_cpp.relative_to(REPO_ROOT)} must "
                    f"contain `{label}` (W44-45 — Marketplace "
                    "OnInteract delegates to the per-player controller)."
                )

    # Build.cs must list UMG (UUserWidget base class requires it).
    build_cs = src_root / "PaldarkLab.Build.cs"
    if build_cs.is_file():
        text = build_cs.read_text(encoding="utf-8")
        if "\"UMG\"" not in text:
            fail(
                f"{build_cs.relative_to(REPO_ROOT)} must list `\"UMG\"` "
                "in PublicDependencyModuleNames or "
                "PrivateDependencyModuleNames (W44-45 — required for "
                "UUserWidget base class of the Stable/Marketplace widget "
                "stubs)."
            )

    # LogPaldarkUI category must be declared + defined.
    log_h = PROJECT_DIR / "Source" / "PaldarkLabCore" / "Public" / "PaldarkLogCategories.h"
    if log_h.is_file():
        text = log_h.read_text(encoding="utf-8")
        if "LogPaldarkUI" not in text:
            fail(
                f"{log_h.relative_to(REPO_ROOT)} must declare "
                "`LogPaldarkUI` (W44-45 — UI log category, used by both "
                "Stable + Marketplace controllers and the 8 console "
                "commands)."
            )

    log_cpp = PROJECT_DIR / "Source" / "PaldarkLabCore" / "Private" / "PaldarkLabCore.cpp"
    if log_cpp.is_file():
        text = log_cpp.read_text(encoding="utf-8")
        if "DEFINE_LOG_CATEGORY(LogPaldarkUI)" not in text:
            fail(
                f"{log_cpp.relative_to(REPO_ROOT)} must define "
                "`LogPaldarkUI` via DEFINE_LOG_CATEGORY (W44-45)."
            )

    # 8 W44-45 console commands registered in PaldarkLab module.
    mod_cpp = src_root / "PaldarkLab.cpp"
    if mod_cpp.is_file():
        text = mod_cpp.read_text(encoding="utf-8")
        for cmd in (
            "Paldark.Hub.Stable.List",
            "Paldark.Hub.Stable.Deposit",
            "Paldark.Hub.Stable.Withdraw",
            "Paldark.Hub.Stable.Heal",
            "Paldark.Hub.Market.Catalog",
            "Paldark.Hub.Market.Buy",
            "Paldark.Hub.Market.Sell",
            "Paldark.Hub.Market.Balance",
        ):
            if f"\"{cmd}\"" not in text:
                fail(
                    f"{mod_cpp.relative_to(REPO_ROOT)} must register "
                    f"the `{cmd}` console command (W44-45 — designer "
                    "test loop for Stable + Marketplace UI flow)."
                )


def check_w46_hub_qa_shape() -> None:
    """W46 — Briefing room widget + multi-player vote/ready handoff.

    Six cooperating pieces, all structural (header presence + symbol +
    canary regexes). Designer authors the actual UMG widget
    (`WBP_PaldarkBriefingWidget`) as a Blueprint subclass of the C++ stub.

      1. UPaldarkBriefingController {.h,.cpp} — UActorComponent on
         APaldarkPlayerController. Owns per-player client/server plumbing
         for vote/ready flow. Server RPCs (Vote/Unvote/Ready/Unready) +
         Client result RPC + 3 BlueprintAssignable delegates.
      2. UPaldarkBriefingSessionComponent {.h,.cpp} — UActorComponent on
         APaldarkGameStateBase. Owns shard-wide vote tallies, ready bitset,
         current phase (Idle/Countdown/Travelling), countdown timer.
         Replicates to every client (PhaseTag, VoterStates, CountdownRemaining
         all ReplicatedUsing OnRep_*).
      3. UPaldarkBriefingWidget {.h,.cpp} — UUserWidget stub with
         BlueprintImplementableEvent hooks (K2_OnBriefingOpened /
         K2_OnBriefingClosed / K2_OnBriefingActionResult / K2_OnSessionChanged).
      4. APaldarkPlayerController instantiates BriefingController as a
         CreateDefaultSubobject in its ctor; exposes accessor.
      5. APaldarkGameStateBase instantiates BriefingSessionComponent as
         a CreateDefaultSubobject in its ctor; exposes accessor.
      6. APaldarkBriefingRoom::OnInteract routes through the per-player
         BriefingController (no longer hard-codes the W40-41 console-driven
         path; that's the documented backward-compat fallback).
    """
    src_root = PROJECT_DIR / "Source" / "PaldarkLab"

    expected_files = [
        # (1) Briefing controller.
        (src_root / "Public"  / "Hub" / "PaldarkBriefingController.h",
         r"class\s+PALDARKLAB_API\s+UPaldarkBriefingController\s*:\s*public\s+UActorComponent"),
        (src_root / "Private" / "Hub" / "PaldarkBriefingController.cpp", None),
        # (2) Briefing session component.
        (src_root / "Public"  / "Hub" / "PaldarkBriefingSessionComponent.h",
         r"class\s+PALDARKLAB_API\s+UPaldarkBriefingSessionComponent\s*:\s*public\s+UActorComponent"),
        (src_root / "Private" / "Hub" / "PaldarkBriefingSessionComponent.cpp", None),
        # (3) Briefing widget stub.
        (src_root / "Public"  / "Hub" / "PaldarkBriefingWidget.h",
         r"class\s+PALDARKLAB_API\s+UPaldarkBriefingWidget\s*:\s*public\s+UUserWidget"),
        (src_root / "Private" / "Hub" / "PaldarkBriefingWidget.cpp", None),
    ]
    for path, class_regex in expected_files:
        if not path.is_file():
            fail(
                f"missing {path.relative_to(REPO_ROOT)} "
                "(W46 — Briefing room widget + vote/ready handoff; see "
                "PaldarkLab/README.md § W46)."
            )
            continue
        if class_regex is not None:
            text = path.read_text(encoding="utf-8")
            if not re.search(class_regex, text):
                fail(
                    f"{path.relative_to(REPO_ROOT)} must declare the "
                    f"expected UCLASS matching `{class_regex}` (W46)."
                )

    # (1) Briefing controller — RPC + delegate shape.
    brief_h = src_root / "Public" / "Hub" / "PaldarkBriefingController.h"
    if brief_h.is_file():
        text = brief_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("RequestOpenBriefing client API",
             r"void\s+RequestOpenBriefing\s*\(\s*APaldarkBriefingRoom\s*\*"),
            ("RequestCloseBriefing client API",
             r"void\s+RequestCloseBriefing\s*\(\s*\)"),
            ("RequestVote client API",
             r"void\s+RequestVote\s*\(\s*FGameplayTag"),
            ("RequestUnvote client API",
             r"void\s+RequestUnvote\s*\(\s*\)"),
            ("RequestReady client API",
             r"void\s+RequestReady\s*\(\s*\)"),
            ("RequestUnready client API",
             r"void\s+RequestUnready\s*\(\s*\)"),
            ("Server_RequestVote Server RPC",
             r"UFUNCTION\s*\(\s*Server[^)]*\)\s*\n?\s*void\s+Server_RequestVote\b"),
            ("Server_RequestUnvote Server RPC",
             r"UFUNCTION\s*\(\s*Server[^)]*\)\s*\n?\s*void\s+Server_RequestUnvote\b"),
            ("Server_RequestReady Server RPC",
             r"UFUNCTION\s*\(\s*Server[^)]*\)\s*\n?\s*void\s+Server_RequestReady\b"),
            ("Server_RequestUnready Server RPC",
             r"UFUNCTION\s*\(\s*Server[^)]*\)\s*\n?\s*void\s+Server_RequestUnready\b"),
            ("Client_ReceiveActionResult Client RPC",
             r"UFUNCTION\s*\(\s*Client[^)]*\)\s*\n?\s*void\s+Client_ReceiveActionResult\b"),
            ("OnBriefingOpened BlueprintAssignable",
             r"FPaldarkBriefingOpened\s+OnBriefingOpened\b"),
            ("OnBriefingClosed BlueprintAssignable",
             r"FPaldarkBriefingClosed\s+OnBriefingClosed\b"),
            ("OnBriefingActionResult BlueprintAssignable",
             r"FPaldarkBriefingActionResult\s+OnBriefingActionResult\b"),
            ("BlueprintAssignable UPROPERTY",
             r"UPROPERTY\s*\(\s*BlueprintAssignable"),
            ("bIsTransactionInFlight guard",
             r"bIsTransactionInFlight"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{brief_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W46 — Briefing controller shape)."
                )

    brief_cpp = src_root / "Private" / "Hub" / "PaldarkBriefingController.cpp"
    if brief_cpp.is_file():
        text = brief_cpp.read_text(encoding="utf-8")
        for label, pattern in (
            ("Hub.Brief.UI.Open tag wired",
             r"TAG_Paldark_Hub_Brief_UI_Open"),
            ("Hub.Brief.UI.Closed tag wired",
             r"TAG_Paldark_Hub_Brief_UI_Closed"),
            ("Hub.Brief.Action.Vote tag wired",
             r"TAG_Paldark_Hub_Brief_Action_Vote"),
            ("Hub.Brief.Action.Ready tag wired",
             r"TAG_Paldark_Hub_Brief_Action_Ready"),
            ("Hub.Result.Success tag wired",
             r"TAG_Paldark_Hub_Result_Success"),
            ("LogPaldarkUI usage",
             r"LogPaldarkUI"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{brief_cpp.relative_to(REPO_ROOT)} must contain "
                    f"`{label}` (W46 — Briefing controller impl)."
                )

    # (2) Briefing session component — replicated state + authority API.
    sess_h = src_root / "Public" / "Hub" / "PaldarkBriefingSessionComponent.h"
    if sess_h.is_file():
        text = sess_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("ServerCastVote authority API",
             r"bool\s+ServerCastVote\s*\(\s*APaldarkPlayerController\s*\*"),
            ("ServerClearVote authority API",
             r"bool\s+ServerClearVote\s*\(\s*APaldarkPlayerController\s*\*"),
            ("ServerSetReady authority API",
             r"bool\s+ServerSetReady\s*\(\s*APaldarkPlayerController\s*\*\s*\w+\s*,\s*bool"),
            ("ServerRemovePlayer authority API",
             r"bool\s+ServerRemovePlayer\s*\(\s*APaldarkPlayerController\s*\*"),
            ("ServerForceTravel authority API",
             r"void\s+ServerForceTravel\s*\(\s*const\s+FGameplayTag"),
            ("GetPhaseTag accessor",
             r"FGameplayTag\s+GetPhaseTag\s*\(\s*\)\s*const"),
            ("GetVoteTallies accessor",
             r"TArray<\s*FPaldarkBriefingVoteTally\s*>\s+GetVoteTallies\s*\("),
            ("GetVoterStates accessor",
             r"TArray<\s*FPaldarkBriefingVoterState\s*>\s+GetVoterStates\s*\("),
            ("GetReadyCount accessor",
             r"int32\s+GetReadyCount\s*\(\s*\)\s*const"),
            ("GetLeadingMapTag accessor",
             r"FGameplayTag\s+GetLeadingMapTag\s*\(\s*\)\s*const"),
            ("GetCountdownRemaining accessor",
             r"int32\s+GetCountdownRemaining\s*\(\s*\)\s*const"),
            ("MinReadyPlayers designer knob",
             r"int32\s+MinReadyPlayers\b"),
            ("CountdownSeconds designer knob",
             r"int32\s+CountdownSeconds\b"),
            ("PhaseTag ReplicatedUsing",
             r"UPROPERTY\s*\(\s*ReplicatedUsing\s*=\s*OnRep_PhaseTag"),
            ("VoterStates ReplicatedUsing",
             r"UPROPERTY\s*\(\s*ReplicatedUsing\s*=\s*OnRep_VoterStates"),
            ("CountdownRemaining ReplicatedUsing",
             r"UPROPERTY\s*\(\s*ReplicatedUsing\s*=\s*OnRep_CountdownRemaining"),
            ("OnRep_PhaseTag UFUNCTION",
             r"UFUNCTION\s*\(\s*\)\s*\n?\s*void\s+OnRep_PhaseTag\b"),
            ("OnRep_VoterStates UFUNCTION",
             r"UFUNCTION\s*\(\s*\)\s*\n?\s*void\s+OnRep_VoterStates\b"),
            ("OnRep_CountdownRemaining UFUNCTION",
             r"UFUNCTION\s*\(\s*\)\s*\n?\s*void\s+OnRep_CountdownRemaining\b"),
            ("GetLifetimeReplicatedProps override",
             r"virtual\s+void\s+GetLifetimeReplicatedProps\s*\("),
            ("OnSessionChanged BlueprintAssignable",
             r"FPaldarkBriefingSessionChanged\s+OnSessionChanged\b"),
            ("DumpToLog helper",
             r"void\s+DumpToLog\s*\(\s*\)\s*const"),
            ("FPaldarkBriefingVoteTally USTRUCT",
             r"struct\s+FPaldarkBriefingVoteTally\b"),
            ("FPaldarkBriefingVoterState USTRUCT",
             r"struct\s+FPaldarkBriefingVoterState\b"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{sess_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W46 — Briefing session component shape)."
                )

    sess_cpp = src_root / "Private" / "Hub" / "PaldarkBriefingSessionComponent.cpp"
    if sess_cpp.is_file():
        text = sess_cpp.read_text(encoding="utf-8")
        for label, pattern in (
            ("DOREPLIFETIME PhaseTag",
             r"DOREPLIFETIME\s*\(\s*UPaldarkBriefingSessionComponent\s*,\s*PhaseTag"),
            ("DOREPLIFETIME VoterStates",
             r"DOREPLIFETIME\s*\(\s*UPaldarkBriefingSessionComponent\s*,\s*VoterStates"),
            ("DOREPLIFETIME CountdownRemaining",
             r"DOREPLIFETIME\s*\(\s*UPaldarkBriefingSessionComponent\s*,\s*CountdownRemaining"),
            ("Hub.Brief.Phase.Idle tag wired",
             r"TAG_Paldark_Hub_Brief_Phase_Idle"),
            ("Hub.Brief.Phase.Countdown tag wired",
             r"TAG_Paldark_Hub_Brief_Phase_Countdown"),
            ("Hub.Brief.Phase.Travelling tag wired",
             r"TAG_Paldark_Hub_Brief_Phase_Travelling"),
            ("LogPaldarkUI usage",
             r"LogPaldarkUI"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{sess_cpp.relative_to(REPO_ROOT)} must contain "
                    f"`{label}` (W46 — Briefing session component impl)."
                )

    # (3) Briefing widget stub — BlueprintImplementableEvent hooks.
    bw_h = src_root / "Public" / "Hub" / "PaldarkBriefingWidget.h"
    if bw_h.is_file():
        text = bw_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("K2_OnBriefingOpened BlueprintImplementableEvent",
             r"BlueprintImplementableEvent[^}]*?K2_OnBriefingOpened\b"),
            ("K2_OnBriefingClosed BlueprintImplementableEvent",
             r"BlueprintImplementableEvent[^}]*?K2_OnBriefingClosed\b"),
            ("K2_OnBriefingActionResult BlueprintImplementableEvent",
             r"BlueprintImplementableEvent[^}]*?K2_OnBriefingActionResult\b"),
            ("K2_OnSessionChanged BlueprintImplementableEvent",
             r"BlueprintImplementableEvent[^}]*?K2_OnSessionChanged\b"),
            ("GetBriefingController BlueprintPure",
             r"UPaldarkBriefingController\*\s+GetBriefingController\s*\("),
            ("GetSessionComponent BlueprintPure",
             r"UPaldarkBriefingSessionComponent\*\s+GetSessionComponent\s*\("),
        ):
            if not re.search(pattern, text, flags=re.DOTALL):
                fail(
                    f"{bw_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W46 — Briefing UMG widget stub)."
                )

    # (4) PlayerController instantiates BriefingController + accessor.
    pc_h = src_root / "Public" / "Player" / "PaldarkPlayerController.h"
    if pc_h.is_file():
        text = pc_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("GetBriefingController accessor",
             r"UPaldarkBriefingController\*\s+GetBriefingController\s*\("),
            ("BriefingController member",
             r"TObjectPtr<\s*UPaldarkBriefingController\s*>\s+BriefingController\b"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{pc_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W46 — PlayerController hosts the "
                    "per-player briefing controller as a default subobject)."
                )

    pc_cpp = src_root / "Private" / "Player" / "PaldarkPlayerController.cpp"
    if pc_cpp.is_file():
        text = pc_cpp.read_text(encoding="utf-8")
        for label, pattern in (
            ("CreateDefaultSubobject<UPaldarkBriefingController>",
             r"CreateDefaultSubobject\s*<\s*UPaldarkBriefingController\s*>"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{pc_cpp.relative_to(REPO_ROOT)} must contain "
                    f"`{label}` (W46 — PlayerController must instantiate "
                    "BriefingController in its constructor)."
                )

    # (5) GameStateBase instantiates BriefingSessionComponent + accessor.
    gs_h = src_root / "Public" / "Framework" / "PaldarkGameStateBase.h"
    if gs_h.is_file():
        text = gs_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("GetBriefingSessionComponent accessor",
             r"UPaldarkBriefingSessionComponent\*\s+GetBriefingSessionComponent\s*\("),
            ("BriefingSessionComponent member",
             r"TObjectPtr<\s*UPaldarkBriefingSessionComponent\s*>\s+BriefingSessionComponent\b"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{gs_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W46 — GameStateBase hosts the shard-wide "
                    "briefing session component as a default subobject)."
                )

    gs_cpp = src_root / "Private" / "Framework" / "PaldarkGameStateBase.cpp"
    if gs_cpp.is_file():
        text = gs_cpp.read_text(encoding="utf-8")
        for label, pattern in (
            ("CreateDefaultSubobject<UPaldarkBriefingSessionComponent>",
             r"CreateDefaultSubobject\s*<\s*UPaldarkBriefingSessionComponent\s*>"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{gs_cpp.relative_to(REPO_ROOT)} must contain "
                    f"`{label}` (W46 — GameStateBase must instantiate "
                    "BriefingSessionComponent in its constructor)."
                )

    # (6) BriefingRoom::OnInteract routes through BriefingController
    # (with W40-41 console-driven fallback documented in the comment).
    br_cpp = src_root / "Private" / "Hub" / "PaldarkBriefingRoom.cpp"
    if br_cpp.is_file():
        text = br_cpp.read_text(encoding="utf-8")
        for label, pattern in (
            ("BriefingController include",
             r'#\s*include\s+"Hub/PaldarkBriefingController\.h"'),
            ("RequestOpenBriefing routed call",
             r"RequestOpenBriefing\s*\(\s*this\s*\)"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{br_cpp.relative_to(REPO_ROOT)} must contain "
                    f"`{label}` (W46 — BriefingRoom::OnInteract must route "
                    "through the per-player BriefingController)."
                )

    # (7) PaldarkLab module registers the 6 W46 console commands.
    mod_cpp = src_root / "PaldarkLab.cpp"
    if mod_cpp.is_file():
        text = mod_cpp.read_text(encoding="utf-8")
        for cmd in (
            "Paldark.Hub.Brief.Dump",
            "Paldark.Hub.Brief.Vote",
            "Paldark.Hub.Brief.Unvote",
            "Paldark.Hub.Brief.Ready",
            "Paldark.Hub.Brief.Unready",
            "Paldark.Hub.QA.ForceTravel",
        ):
            if f"\"{cmd}\"" not in text:
                fail(
                    f"{mod_cpp.relative_to(REPO_ROOT)} must register "
                    f"the `{cmd}` console command (W46 — designer "
                    "test loop for the briefing vote/ready/countdown flow)."
                )


def check_w47_save_game_shape() -> None:
    """W47 — Save game system (USaveGame for player progression).

    Five cooperating pieces, all structural (header presence + symbol +
    canary regexes). Designer authors the UMG slot picker (W48 polish)
    and the auto-save hook (W48 polish) as Blueprint subclasses / event
    bindings on top of the C++ subsystem.

      1. PaldarkSaveTypes.h — three USTRUCT snapshot blobs
         (FPaldarkRosterSnapshot, FPaldarkInventorySnapshot,
         FPaldarkProgressSnapshot) + two USTRUCT entry rows
         (FPaldarkRosterEntrySaved, FPaldarkInventoryEntrySaved).
         Each snapshot uses tags (FGameplayTag / FName) instead of soft
         object pointers so the .sav blob survives asset renames.
      2. UPaldarkPlayerSaveGame {.h,.cpp} — USaveGame subclass with
         SchemaVersion (kCurrentSchema = 1), SaveTime, PlayerName, plus
         four nested snapshots (ActiveRoster, DepositRoster, Inventory,
         Progress). DescribeForLog() helper for the console dump path.
      3. UPaldarkSaveSubsystem {.h,.cpp} — UGameInstanceSubsystem with
         RequestSaveSlot / RequestLoadSlot / RequestDumpSlot / ClearSlot
         / DoesSlotExist / ListSlotsToLog / WipeAllSlots APIs + two
         BlueprintAssignable delegates (OnSaveCompleted, OnLoadCompleted).
      4. CaptureSnapshot / ApplySnapshot helpers on
         UPaldarkPlayerPalRosterComponent, UPaldarkPalDepositComponent,
         UPaldarkPlayerInventoryComponent (authority-only writes; broadcast
         change delegates after apply).
      5. PaldarkLab module registers 6 console commands
         (Paldark.Save.{Save,Load,Dump,ClearSlot,ListSlots} +
         Paldark.QA.WipeAllSlots).
    """
    src_root = PROJECT_DIR / "Source" / "PaldarkLab"

    expected_files = [
        # (1) Save types header (no .cpp class to check; types live in header).
        (src_root / "Public"  / "Save" / "PaldarkSaveTypes.h", None),
        (src_root / "Private" / "Save" / "PaldarkSaveTypes.cpp", None),
        # (2) Save game asset.
        (src_root / "Public"  / "Save" / "PaldarkPlayerSaveGame.h",
         r"class\s+PALDARKLAB_API\s+UPaldarkPlayerSaveGame\s*:\s*public\s+USaveGame"),
        (src_root / "Private" / "Save" / "PaldarkPlayerSaveGame.cpp", None),
        # (3) Save subsystem.
        (src_root / "Public"  / "Save" / "PaldarkSaveSubsystem.h",
         r"class\s+PALDARKLAB_API\s+UPaldarkSaveSubsystem\s*:\s*public\s+UGameInstanceSubsystem"),
        (src_root / "Private" / "Save" / "PaldarkSaveSubsystem.cpp", None),
    ]
    for path, class_regex in expected_files:
        if not path.is_file():
            fail(
                f"missing {path.relative_to(REPO_ROOT)} "
                "(W47 — Save game system; see PaldarkLab/README.md § W47)."
            )
            continue
        if class_regex is not None:
            text = path.read_text(encoding="utf-8")
            if not re.search(class_regex, text):
                fail(
                    f"{path.relative_to(REPO_ROOT)} must declare the "
                    f"expected UCLASS matching `{class_regex}` (W47)."
                )

    # (1) Save types — USTRUCT presence + field shape.
    types_h = src_root / "Public" / "Save" / "PaldarkSaveTypes.h"
    if types_h.is_file():
        text = types_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("FPaldarkRosterEntrySaved USTRUCT",
             r"struct\s+(?:PALDARKLAB_API\s+)?FPaldarkRosterEntrySaved\b"),
            ("FPaldarkRosterSnapshot USTRUCT",
             r"struct\s+(?:PALDARKLAB_API\s+)?FPaldarkRosterSnapshot\b"),
            ("FPaldarkInventoryEntrySaved USTRUCT",
             r"struct\s+(?:PALDARKLAB_API\s+)?FPaldarkInventoryEntrySaved\b"),
            ("FPaldarkInventorySnapshot USTRUCT",
             r"struct\s+(?:PALDARKLAB_API\s+)?FPaldarkInventorySnapshot\b"),
            ("FPaldarkProgressSnapshot USTRUCT",
             r"struct\s+(?:PALDARKLAB_API\s+)?FPaldarkProgressSnapshot\b"),
            ("Roster entry uses SpeciesTag (FGameplayTag)",
             r"FGameplayTag\s+SpeciesTag\b"),
            ("Inventory entry uses ItemTag (FGameplayTag)",
             r"FGameplayTag\s+ItemTag\b"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{types_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W47 — save types contract)."
                )

    # (2) Save game asset — schema + nested snapshot members.
    save_h = src_root / "Public" / "Save" / "PaldarkPlayerSaveGame.h"
    if save_h.is_file():
        text = save_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("kCurrentSchema constant",
             r"static\s+constexpr\s+int32\s+kCurrentSchema\b"),
            ("SchemaVersion member",
             r"int32\s+SchemaVersion\b"),
            ("SaveTime member",
             r"FDateTime\s+SaveTime\b"),
            ("PlayerName member",
             r"FString\s+PlayerName\b"),
            ("ActiveRoster snapshot member",
             r"FPaldarkRosterSnapshot\s+ActiveRoster\b"),
            ("DepositRoster snapshot member",
             r"FPaldarkRosterSnapshot\s+DepositRoster\b"),
            ("Inventory snapshot member",
             r"FPaldarkInventorySnapshot\s+Inventory\b"),
            ("Progress snapshot member",
             r"FPaldarkProgressSnapshot\s+Progress\b"),
            ("DescribeForLog helper",
             r"FString\s+DescribeForLog\s*\(\s*\)\s*const"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{save_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W47 — save game asset shape)."
                )

    # (3) Save subsystem — public API + delegates + slot bookkeeping.
    sub_h = src_root / "Public" / "Save" / "PaldarkSaveSubsystem.h"
    if sub_h.is_file():
        text = sub_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("RequestSaveSlot API",
             r"bool\s+RequestSaveSlot\s*\(\s*APaldarkPlayerController\s*\*"),
            ("RequestLoadSlot API",
             r"bool\s+RequestLoadSlot\s*\(\s*APaldarkPlayerController\s*\*"),
            ("RequestDumpSlot API",
             r"bool\s+RequestDumpSlot\s*\(\s*FName"),
            ("ClearSlot API",
             r"bool\s+ClearSlot\s*\(\s*FName"),
            ("DoesSlotExist API",
             r"bool\s+DoesSlotExist\s*\(\s*FName"),
            ("ListSlotsToLog API",
             r"int32\s+ListSlotsToLog\s*\("),
            ("WipeAllSlots API",
             r"int32\s+WipeAllSlots\s*\("),
            ("OnSaveCompleted BlueprintAssignable",
             r"FOnPaldarkSaveCompleted\s+OnSaveCompleted\b"),
            ("OnLoadCompleted BlueprintAssignable",
             r"FOnPaldarkLoadCompleted\s+OnLoadCompleted\b"),
            ("BlueprintAssignable UPROPERTY",
             r"UPROPERTY\s*\(\s*BlueprintAssignable"),
            ("InFlightSaveSlots tracker member",
             r"TArray<\s*FName\s*>\s+InFlightSaveSlots\b"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{sub_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W47 — save subsystem shape)."
                )

    sub_cpp = src_root / "Private" / "Save" / "PaldarkSaveSubsystem.cpp"
    if sub_cpp.is_file():
        text = sub_cpp.read_text(encoding="utf-8")
        for label, pattern in (
            ("AsyncSaveGameToSlot wired",
             r"UGameplayStatics::AsyncSaveGameToSlot"),
            ("AsyncLoadGameFromSlot wired",
             r"UGameplayStatics::AsyncLoadGameFromSlot"),
            ("DeleteGameInSlot wired",
             r"UGameplayStatics::DeleteGameInSlot"),
            ("Save.Result.Success tag wired",
             r"TAG_Paldark_Save_Result_Success"),
            ("Save.Result.Fail.IOError tag wired",
             r"TAG_Paldark_Save_Result_Fail_IOError"),
            ("Save.Result.Fail.SchemaMismatch tag wired",
             r"TAG_Paldark_Save_Result_Fail_SchemaMismatch"),
            ("Save.Result.Fail.NoSlot tag wired",
             r"TAG_Paldark_Save_Result_Fail_NoSlot"),
            ("Save.Trigger.Manual tag wired",
             r"TAG_Paldark_Save_Trigger_Manual"),
            ("LogPaldarkLab usage",
             r"LogPaldarkLab"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{sub_cpp.relative_to(REPO_ROOT)} must contain "
                    f"`{label}` (W47 — save subsystem impl)."
                )

    # (4) Snapshot helpers on the three live state components.
    snapshot_components = [
        (src_root / "Public"  / "Pal" / "PaldarkPlayerPalRosterComponent.h",
         "FPaldarkRosterSnapshot",
         "UPaldarkPlayerPalRosterComponent"),
        (src_root / "Public"  / "Pal" / "PaldarkPalDepositComponent.h",
         "FPaldarkRosterSnapshot",
         "UPaldarkPalDepositComponent"),
        (src_root / "Public"  / "Player" / "Components" / "PaldarkPlayerInventoryComponent.h",
         "FPaldarkInventorySnapshot",
         "UPaldarkPlayerInventoryComponent"),
    ]
    for header, snapshot_type, class_name in snapshot_components:
        if not header.is_file():
            fail(
                f"missing {header.relative_to(REPO_ROOT)} "
                f"(W47 — expected snapshot helpers on {class_name})."
            )
            continue
        text = header.read_text(encoding="utf-8")
        capture_pattern = (
            rf"{snapshot_type}\s+CaptureSnapshot\s*\(\s*\)\s*const"
        )
        apply_pattern = (
            rf"void\s+ApplySnapshot\s*\(\s*const\s+{snapshot_type}\s*&"
        )
        if not re.search(capture_pattern, text):
            fail(
                f"{header.relative_to(REPO_ROOT)} must declare "
                f"`{snapshot_type} CaptureSnapshot() const` "
                f"(W47 — {class_name} snapshot helper)."
            )
        if not re.search(apply_pattern, text):
            fail(
                f"{header.relative_to(REPO_ROOT)} must declare "
                f"`void ApplySnapshot(const {snapshot_type}&)` "
                f"(W47 — {class_name} snapshot helper)."
            )

    # (5) PaldarkLab module registers the 6 W47 console commands.
    mod_cpp = src_root / "PaldarkLab.cpp"
    if mod_cpp.is_file():
        text = mod_cpp.read_text(encoding="utf-8")
        for cmd in (
            "Paldark.Save.Save",
            "Paldark.Save.Load",
            "Paldark.Save.Dump",
            "Paldark.Save.ClearSlot",
            "Paldark.Save.ListSlots",
            "Paldark.QA.WipeAllSlots",
        ):
            if f"\"{cmd}\"" not in text:
                fail(
                    f"{mod_cpp.relative_to(REPO_ROOT)} must register "
                    f"the `{cmd}` console command (W47 — designer "
                    "test loop for the save/load round-trip)."
                )


def check_w48_polish_shape() -> None:
    """W48 — Polish (auto-save hooks + UMG slot picker stub + hub→raid smoke).

    Builds on top of the W47 save game system. Four moving parts:

      1. Auto-save hooks on UPaldarkSaveSubsystem:
           * `HandlePostLoadMapWithWorld` — re-binds the world-scope match
             subsystem outcome delegate + (once) the GameInstance-scope
             net subsystem logout delegate.
           * `HandlePlayerOutcomeChanged(PlayerState, Outcome)` — filters
             for Extracted + local PC, fires `RequestSaveSlot(...,
             Paldark.Save.Trigger.HubReturn)`.
           * `HandlePlayerLogoutAutoSave(AController*)` — filters for
             local Paldark PC, fires `RequestSaveSlot(...,
             Paldark.Save.Trigger.Logout)`.
           * `RequestHubToRaidHandoffSmoke(PC, SlotName)` — manual smoke
             entry that wraps RequestSaveSlot with Trigger.Travel.
           * `bAutoSaveOnExtract`, `bAutoSaveOnLogout` UPROPERTYs so QA
             can toggle the hooks at runtime.

      2. UMG save slot picker stub (UPaldarkSaveSlotPickerWidget). C++
         base with BlueprintCallable wrappers (RequestSave / RequestLoad
         / RequestDump / RequestClear / RequestList) + two
         BlueprintImplementableEvent hooks (K2_OnSaveCompleted,
         K2_OnLoadCompleted). Designer authors WBP_* subclass.

      3. New gameplay tag `Paldark.Save.Trigger.Travel` (declared in
         PaldarkGameplayTags.h, defined in .cpp). Already covered by
         the global gameplay tag check; this validator only confirms
         the .cpp UE_DEFINE entry exists.

      4. New console command `Paldark.QA.HubToRaidHandoff` registered
         in PaldarkLab.cpp and matching handler.
    """
    src_root = PROJECT_DIR / "Source" / "PaldarkLab"

    # ---- (1) Auto-save plumbing on the save subsystem ---------------------

    sub_h = src_root / "Public" / "Save" / "PaldarkSaveSubsystem.h"
    if sub_h.is_file():
        text = sub_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("bAutoSaveOnExtract UPROPERTY",
             r"UPROPERTY\s*\([^)]*\)\s*bool\s+bAutoSaveOnExtract\b"),
            ("bAutoSaveOnLogout UPROPERTY",
             r"UPROPERTY\s*\([^)]*\)\s*bool\s+bAutoSaveOnLogout\b"),
            ("RequestHubToRaidHandoffSmoke API",
             r"bool\s+RequestHubToRaidHandoffSmoke\s*\(\s*APaldarkPlayerController\s*\*"),
            ("HandlePostLoadMapWithWorld helper",
             r"void\s+HandlePostLoadMapWithWorld\s*\(\s*UWorld\s*\*"),
            ("HandlePlayerOutcomeChanged helper",
             r"void\s+HandlePlayerOutcomeChanged\s*\(\s*APlayerState\s*\*"),
            ("HandlePlayerLogoutAutoSave helper (UFUNCTION)",
             r"UFUNCTION\s*\(\s*\)\s*void\s+HandlePlayerLogoutAutoSave\s*\(\s*AController\s*\*"),
            ("BoundMatchSubsystem weak ref",
             r"TWeakObjectPtr<\s*UPaldarkMatchSubsystem\s*>\s+BoundMatchSubsystem\b"),
            ("MatchOutcomeHandle delegate handle",
             r"FDelegateHandle\s+MatchOutcomeHandle\b"),
            ("bBoundNetLogout flag",
             r"bool\s+bBoundNetLogout\b"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{sub_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W48 — auto-save hook shape)."
                )

    sub_cpp = src_root / "Private" / "Save" / "PaldarkSaveSubsystem.cpp"
    if sub_cpp.is_file():
        text = sub_cpp.read_text(encoding="utf-8")
        for label, pattern in (
            ("PostLoadMapWithWorld delegate bind",
             r"FCoreUObjectDelegates::PostLoadMapWithWorld\.AddUObject\s*\(\s*this\s*,\s*&UPaldarkSaveSubsystem::HandlePostLoadMapWithWorld"),
            ("Match outcome delegate bind",
             r"OnPlayerOutcomeChanged\.AddUObject\s*\(\s*this\s*,\s*&UPaldarkSaveSubsystem::HandlePlayerOutcomeChanged"),
            ("Net logout delegate bind",
             r"OnPlayerLogout\.AddDynamic\s*\(\s*this\s*,\s*&UPaldarkSaveSubsystem::HandlePlayerLogoutAutoSave"),
            ("HubReturn trigger fires on extract",
             r"TAG_Paldark_Save_Trigger_HubReturn"),
            ("Logout trigger fires on logout",
             r"TAG_Paldark_Save_Trigger_Logout"),
            ("Travel trigger fires on smoke",
             r"TAG_Paldark_Save_Trigger_Travel"),
            ("Extracted outcome filter",
             r"EPaldarkPlayerOutcome::Extracted"),
            ("LogPaldarkLab W48 tag",
             r"\[Save\]\[W48\]"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{sub_cpp.relative_to(REPO_ROOT)} must contain "
                    f"`{label}` (W48 — auto-save hook impl)."
                )

    # ---- (2) UMG slot picker stub -----------------------------------------

    picker_h = src_root / "Public" / "Save" / "PaldarkSaveSlotPickerWidget.h"
    if not picker_h.is_file():
        fail(
            f"missing {picker_h.relative_to(REPO_ROOT)} "
            "(W48 — UMG save slot picker stub)."
        )
    else:
        text = picker_h.read_text(encoding="utf-8")
        for label, pattern in (
            ("UPaldarkSaveSlotPickerWidget UCLASS",
             r"class\s+PALDARKLAB_API\s+UPaldarkSaveSlotPickerWidget\s*:\s*public\s+UUserWidget"),
            ("Abstract Blueprintable UCLASS macro",
             r"UCLASS\s*\(\s*Abstract\s*,\s*Blueprintable\s*\)"),
            ("GetSaveSubsystem BP-pure accessor",
             r"UFUNCTION\s*\(\s*BlueprintPure[^)]*\)\s*UPaldarkSaveSubsystem\s*\*\s*GetSaveSubsystem"),
            ("GetOwningPaldarkPC BP-pure accessor",
             r"UFUNCTION\s*\(\s*BlueprintPure[^)]*\)\s*APaldarkPlayerController\s*\*\s*GetOwningPaldarkPC"),
            ("RequestSave BP-callable wrapper",
             r"UFUNCTION\s*\(\s*BlueprintCallable[^)]*\)\s*bool\s+RequestSave\b"),
            ("RequestLoad BP-callable wrapper",
             r"UFUNCTION\s*\(\s*BlueprintCallable[^)]*\)\s*bool\s+RequestLoad\b"),
            ("RequestDump BP-callable wrapper",
             r"UFUNCTION\s*\(\s*BlueprintCallable[^)]*\)\s*bool\s+RequestDump\b"),
            ("RequestClear BP-callable wrapper",
             r"UFUNCTION\s*\(\s*BlueprintCallable[^)]*\)\s*bool\s+RequestClear\b"),
            ("RequestList BP-callable wrapper",
             r"UFUNCTION\s*\(\s*BlueprintCallable[^)]*\)\s*int32\s+RequestList\b"),
            ("K2_OnSaveCompleted BP event",
             r"BlueprintImplementableEvent[^)]*\)[\s\S]*?K2_OnSaveCompleted"),
            ("K2_OnLoadCompleted BP event",
             r"BlueprintImplementableEvent[^)]*\)[\s\S]*?K2_OnLoadCompleted"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{picker_h.relative_to(REPO_ROOT)} must declare "
                    f"`{label}` (W48 — save slot picker shape)."
                )

    picker_cpp = src_root / "Private" / "Save" / "PaldarkSaveSlotPickerWidget.cpp"
    if not picker_cpp.is_file():
        fail(
            f"missing {picker_cpp.relative_to(REPO_ROOT)} "
            "(W48 — UMG save slot picker stub impl)."
        )
    else:
        text = picker_cpp.read_text(encoding="utf-8")
        for label, pattern in (
            ("NativeOnInitialized override",
             r"void\s+UPaldarkSaveSlotPickerWidget::NativeOnInitialized\s*\(\s*\)"),
            ("NativeDestruct override",
             r"void\s+UPaldarkSaveSlotPickerWidget::NativeDestruct\s*\(\s*\)"),
            ("OnSaveCompleted dynamic bind",
             r"OnSaveCompleted\.AddDynamic\s*\(\s*this\s*,\s*&UPaldarkSaveSlotPickerWidget::HandleSaveCompleted"),
            ("OnLoadCompleted dynamic bind",
             r"OnLoadCompleted\.AddDynamic\s*\(\s*this\s*,\s*&UPaldarkSaveSlotPickerWidget::HandleLoadCompleted"),
            ("Picker uses Trigger.Manual on direct save",
             r"TAG_Paldark_Save_Trigger_Manual"),
        ):
            if not re.search(pattern, text):
                fail(
                    f"{picker_cpp.relative_to(REPO_ROOT)} must contain "
                    f"`{label}` (W48 — picker impl)."
                )

    # ---- (3) Travel trigger tag defined in .cpp ---------------------------

    tag_cpp = (PROJECT_DIR / "Source" / "PaldarkLabCore" / "Private" /
               "PaldarkGameplayTags.cpp")
    if tag_cpp.is_file():
        text = tag_cpp.read_text(encoding="utf-8")
        if not re.search(
            r'UE_DEFINE_GAMEPLAY_TAG\s*\(\s*TAG_Paldark_Save_Trigger_Travel\s*,\s*"Paldark\.Save\.Trigger\.Travel"',
            text,
        ):
            fail(
                f"{tag_cpp.relative_to(REPO_ROOT)} must UE_DEFINE_GAMEPLAY_TAG "
                "`TAG_Paldark_Save_Trigger_Travel` = `Paldark.Save.Trigger.Travel` (W48)."
            )

    # ---- (4) Console command registration ---------------------------------

    mod_cpp = src_root / "PaldarkLab.cpp"
    if mod_cpp.is_file():
        text = mod_cpp.read_text(encoding="utf-8")
        if "\"Paldark.QA.HubToRaidHandoff\"" not in text:
            fail(
                f"{mod_cpp.relative_to(REPO_ROOT)} must register "
                "the `Paldark.QA.HubToRaidHandoff` console command "
                "(W48 — hub→raid handoff smoke test)."
            )
        if not re.search(
            r"HandleQAHubToRaidHandoffCommand\b", text
        ):
            fail(
                f"{mod_cpp.relative_to(REPO_ROOT)} must define "
                "`HandleQAHubToRaidHandoffCommand` (W48)."
            )

    mod_h = src_root / "PaldarkLab.h"
    if mod_h.is_file():
        text = mod_h.read_text(encoding="utf-8")
        if not re.search(
            r"IConsoleObject\s*\*\s*QAHubToRaidHandoffCommand\b", text
        ):
            fail(
                f"{mod_h.relative_to(REPO_ROOT)} must declare "
                "`IConsoleObject* QAHubToRaidHandoffCommand` (W48)."
            )


def main() -> int:
    if not PROJECT_DIR.is_dir():
        fail(f"PaldarkLab/ project directory not found at {PROJECT_DIR}")
    else:
        check_uproject()
        check_module_layout()
        check_module_impl()
        check_targets()
        check_log_categories()
        check_no_bom()
        check_framework_classes()
        check_ini_wiring()
        check_gameplay_tags()
        check_gameplay_tags_dependency()
        check_enhanced_input_dependency()
        check_input_config_shape()
        check_player_slot_classes()
        check_pal_slot_classes()
        check_player_character_slot_wiring()
        check_pal_character_slot_wiring()
        check_pal_pawn_data_shape()
        check_activity_fsm_shape()
        check_gas_dependency()
        check_gas_shape()
        check_combat_damage_shape()
        check_inventory_shape()
        check_net_subsystem_shape()
        check_lag_compensation_shape()
        check_pal_combat_shape()
        check_hostile_pal_shape()
        check_squad_shape()
        check_match_shape()
        check_extraction_gamemode_wired()
        check_required_game_features_wired()
        check_anim_instance_shape()
        check_pal_definition_shape()
        check_pal_spawn_subsystem()
        check_game_feature_plugin()
        check_w29_pal_roster_shape()
        check_w31_map_blockout_shape()
        check_w33_loot_drop_shape()
        check_w35_tame_minigame_shape()
        check_w37_inventory_full_shape()
        check_w40_hub_shape()
        check_w42_backend_shape()
        check_w44_stable_marketplace_shape()
        check_w46_hub_qa_shape()
        check_w47_save_game_shape()
        check_w48_polish_shape()

    if errors:
        print(f"validate_paldarklab.py: {len(errors)} error(s):", file=sys.stderr)
        for err in errors:
            print(f"  - {err}", file=sys.stderr)
        return 1

    print("validate_paldarklab.py: OK — PaldarkLab/ structure looks healthy.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
