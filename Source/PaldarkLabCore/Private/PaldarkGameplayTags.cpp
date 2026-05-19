#include "PaldarkGameplayTags.h"

namespace PaldarkGameplayTags
{
	// Definition of the tags declared in PaldarkGameplayTags.h.
	// UE_DEFINE_GAMEPLAY_TAG registers the tag at module load via static init —
	// because PaldarkLabCore is PreDefault, these tags are guaranteed visible
	// before any gameplay/editor module looks them up.
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Experience_Default,     "Paldark.Experience.Default");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Experience_RaidSandbox, "Paldark.Experience.RaidSandbox");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Pawn_Player,            "Paldark.Pawn.Player");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Pawn_Pal,               "Paldark.Pawn.Pal");

	// W1 day 11-14 — InputTag.* hierarchy. Strings deliberately mirror Lyra so
	// designers familiar with that namespace can read PaldarkInputConfig assets
	// without re-learning.
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_InputTag_Move, "Paldark.InputTag.Move");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_InputTag_Look, "Paldark.InputTag.Look");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_InputTag_Jump, "Paldark.InputTag.Jump");

	// W7-8 — Sprint hierarchy (Input + Ability + State). InputTag.Sprint is the
	// `AbilityInputActions` lookup key, Ability.Sprint is the AbilityTag on the
	// granted UPaldarkGameplayAbility_Sprint (used for cancel-by-tag), and
	// State.Sprinting is a loose tag added on the owner ASC while the ability
	// is active.
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_InputTag_Sprint,  "Paldark.InputTag.Sprint");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Ability_Sprint,   "Paldark.Ability.Sprint");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_State_Sprinting,  "Paldark.State.Sprinting");

	// W9-10 — Hitscan Fire ability chain + damage SetByCaller magnitudes +
	// hit/state tags. Mirrors the Aura / Crunch namespace convention so the
	// course material ports cleanly.
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_InputTag_Fire,                  "Paldark.InputTag.Fire");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Ability_Fire,                   "Paldark.Ability.Fire");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hit_Headshot,                   "Paldark.Hit.Headshot");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_State_IsDead,                   "Paldark.State.IsDead");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_SetByCaller_BaseDamage,         "Paldark.SetByCaller.BaseDamage");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_SetByCaller_HeadshotMultiplier, "Paldark.SetByCaller.HeadshotMultiplier");

	// W5-6 — Pal Activity FSM. Mirror RoN naming (Idle/Follow/Investigate)
	// under our Paldark.Pal.Activity.* namespace. Console commands like
	// `Paldark.Pal.SetActivity <Idle|Follow|Investigate>` resolve to one of
	// these tags before forwarding to UPaldarkPalActivityComponent.
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Pal_Activity_Idle,        "Paldark.Pal.Activity.Idle");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Pal_Activity_Follow,      "Paldark.Pal.Activity.Follow");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Pal_Activity_Investigate, "Paldark.Pal.Activity.Investigate");

	// W11-12 — Inventory item identity tags. Strings under Paldark.Item.* mirror
	// the asset suffixes (`DA_Item_<x>`) so a designer can scan the namespace in
	// Project Settings → GameplayTags and find one asset per row.
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Pistol,        "Paldark.Item.Pistol");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Ammo_9mm,      "Paldark.Item.Ammo_9mm");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Bandage,       "Paldark.Item.Bandage");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Pal_Sphere,    "Paldark.Item.Pal_Sphere");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Energy_Drink,  "Paldark.Item.Energy_Drink");

	// W11-12 — Equipment slot tags. Paldark.Equip.Slot.<x> kept under a
	// dedicated namespace so the inventory UI can filter rows by tag prefix.
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Equip_Slot_Primary,    "Paldark.Equip.Slot.Primary");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Equip_Slot_Consumable, "Paldark.Equip.Slot.Consumable");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Equip_Slot_Throwable,  "Paldark.Equip.Slot.Throwable");

	// W14-15 — Network topology tags. RefreshTopology() reads GetNetMode() once
	// and exposes the result as tags so downstream HUD / activity / spawn code
	// stays mode-agnostic. Strings deliberately mirror the engine ENetMode
	// enum for grep-ability.
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Net_Role_Host,             "Paldark.Net.Role.Host");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Net_Role_Client,           "Paldark.Net.Role.Client");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Net_Mode_Standalone,       "Paldark.Net.Mode.Standalone");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Net_Mode_ListenServer,     "Paldark.Net.Mode.ListenServer");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Net_Mode_DedicatedServer,  "Paldark.Net.Mode.DedicatedServer");

	// W16-17 — Server-side rewind result tags. HitConfirmed / HitRejected are
	// returned in the ServerSideRewindResult so callers (HitscanFire ability,
	// anti-cheat heuristics, debug commands) can branch without re-running the
	// trace. Hit.Bodyshot is the companion to Hit.Headshot so the
	// DamageExecutionCalculation can treat "no zone tag" as a bug rather than
	// a normal-damage default.
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_LagComp_HitConfirmed, "Paldark.LagComp.HitConfirmed");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_LagComp_HitRejected,  "Paldark.LagComp.HitRejected");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hit_Bodyshot,         "Paldark.Hit.Bodyshot");

	// W18-19 — Pal Combat activity + Pal attack ability + Team identity +
	// Pal-side SetByCaller magnitude. Strings deliberately mirror the existing
	// Paldark.Pal.Activity.* / Paldark.Ability.* / Paldark.Team.* / Paldark.SetByCaller.*
	// namespaces so designer-side Project Settings → GameplayTags stays one
	// tree per concept.
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Pal_Activity_Combat,    "Paldark.Pal.Activity.Combat");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Ability_PalAttack,      "Paldark.Ability.PalAttack");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Team_Player,            "Paldark.Team.Player");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Team_Hostile,           "Paldark.Team.Hostile");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_SetByCaller_PalDamage,  "Paldark.SetByCaller.PalDamage");

	// W20-21 — Hostile-Pal species + activity + pack identity tags. Strings
	// extend the same Paldark.Pal.{Activity,Species,Pack}.* namespaces so the
	// Project Settings → GameplayTags tree stays one branch per concept.
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Pal_Species_Direhound,  "Paldark.Pal.Species.Direhound");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Pal_Species_Razorbird,  "Paldark.Pal.Species.Razorbird");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Pal_Activity_Patrol,    "Paldark.Pal.Activity.Patrol");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Pal_Activity_Stalk,     "Paldark.Pal.Activity.Stalk");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Pal_Pack_Direhound,     "Paldark.Pal.Pack.Direhound");

	// W22-23 — Squad system + radial command + ping + input tags. Strings
	// land under new Paldark.Squad.* / Paldark.Ping.* sub-namespaces; the
	// W22-23 input tags extend the existing Paldark.InputTag.* family so
	// they show up next to Sprint / Fire in Project Settings.
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Squad_Default,                "Paldark.Squad.Default");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Squad_Command_Attack,         "Paldark.Squad.Command.Attack");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Squad_Command_Follow,         "Paldark.Squad.Command.Follow");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Squad_Command_Stay,           "Paldark.Squad.Command.Stay");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Squad_Command_Regroup,        "Paldark.Squad.Command.Regroup");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Ping_Type_Spot,               "Paldark.Ping.Type.Spot");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Ping_Type_Enemy,              "Paldark.Ping.Type.Enemy");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Ping_Type_Loot,               "Paldark.Ping.Type.Loot");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Ping_Type_Danger,             "Paldark.Ping.Type.Danger");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_InputTag_Ping,                "Paldark.InputTag.Ping");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_InputTag_MarkEnemy,           "Paldark.InputTag.MarkEnemy");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_InputTag_RadialOpen,          "Paldark.InputTag.RadialOpen");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Pal_Activity_FollowSquadCommand,
		"Paldark.Pal.Activity.FollowSquadCommand");

	// W24-25 — Match phase + per-player outcome + extraction input tags.
	// Strings land under new Paldark.Match.* sub-namespaces; the
	// InteractExtraction input tag extends the existing
	// Paldark.InputTag.* family so it shows up next to Ping / Sprint
	// in Project Settings → GameplayTags.
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Match_Phase_Warmup,           "Paldark.Match.Phase.Warmup");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Match_Phase_Active,           "Paldark.Match.Phase.Active");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Match_Phase_Extracting,       "Paldark.Match.Phase.Extracting");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Match_Phase_Ended,            "Paldark.Match.Phase.Ended");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Match_Outcome_Extracted,      "Paldark.Match.Outcome.Extracted");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Match_Outcome_KIA,            "Paldark.Match.Outcome.KIA");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Match_Outcome_Disconnected,   "Paldark.Match.Outcome.Disconnected");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_InputTag_InteractExtraction,  "Paldark.InputTag.InteractExtraction");

	// W29-30 — 5 new species + 2 new pack + 1 new activity + 5 new
	// ability tags. Strings extend the existing
	// Paldark.Pal.{Species,Pack,Activity}.* / Paldark.Ability.* families
	// so the Project Settings → GameplayTags tree stays one branch per
	// concept.
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Pal_Species_Stoneclad,        "Paldark.Pal.Species.Stoneclad");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Pal_Species_Vinewraith,       "Paldark.Pal.Species.Vinewraith");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Pal_Species_Foxparks,         "Paldark.Pal.Species.Foxparks");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Pal_Species_Boltmane,         "Paldark.Pal.Species.Boltmane");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Pal_Species_Tombat,           "Paldark.Pal.Species.Tombat");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Pal_Pack_Stoneclad,           "Paldark.Pal.Pack.Stoneclad");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Pal_Pack_Vinewraith,          "Paldark.Pal.Pack.Vinewraith");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Pal_Activity_BossPhase,       "Paldark.Pal.Activity.BossPhase");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Ability_PalAttack_Bite,           "Paldark.Ability.PalAttack.Bite");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Ability_PalAttack_VineWhip,       "Paldark.Ability.PalAttack.VineWhip");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Ability_PalAttack_Charge,         "Paldark.Ability.PalAttack.Charge");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Ability_PalAttack_FireBreath,     "Paldark.Ability.PalAttack.FireBreath");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Ability_PalAttack_ChainLightning, "Paldark.Ability.PalAttack.ChainLightning");

	// W31-32 — Map 2 "Rừng Hỏng" blockout. POI type + danger tier + map
	// id + spawn point listener tags. Strings land under new
	// Paldark.POI.* / Paldark.Map.* sub-namespaces so designers can grep
	// them as one branch each in Project Settings → GameplayTags.
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_POI_Type_Outpost,         "Paldark.POI.Type.Outpost");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_POI_Type_Cache,           "Paldark.POI.Type.Cache");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_POI_Type_RuinedTower,     "Paldark.POI.Type.RuinedTower");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_POI_Type_MiniBoss,        "Paldark.POI.Type.MiniBoss");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_POI_Type_ExtractionPad,   "Paldark.POI.Type.ExtractionPad");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_POI_Type_SafeZone,        "Paldark.POI.Type.SafeZone");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_POI_DangerTier_Low,       "Paldark.POI.DangerTier.Low");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_POI_DangerTier_Medium,    "Paldark.POI.DangerTier.Medium");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_POI_DangerTier_High,      "Paldark.POI.DangerTier.High");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Map_RaidSandbox,          "Paldark.Map.RaidSandbox");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Map_RungHong,             "Paldark.Map.RungHong");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Pal_Activity_SpawnPoint,  "Paldark.Pal.Activity.SpawnPoint");

	// W33-34 — Map 3 "Cơ Xưởng PalCorp" + tech-themed loot drop. Strings
	// extend Paldark.Map.* / Paldark.POI.Type.* / Paldark.Item.* /
	// Paldark.LootTable.* families so Project Settings → GameplayTags
	// stays one branch per concept.
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Map_PalCorp,              "Paldark.Map.PalCorp");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_POI_Type_IndoorRoom,      "Paldark.POI.Type.IndoorRoom");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Tech_Battery,        "Paldark.Item.Tech.Battery");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Tech_Circuit,        "Paldark.Item.Tech.Circuit");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Tech_Coolant,        "Paldark.Item.Tech.Coolant");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Tech_Polymer,        "Paldark.Item.Tech.Polymer");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Tech_Microchip,      "Paldark.Item.Tech.Microchip");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_LootTable_HostileStandard,"Paldark.LootTable.HostileStandard");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_LootTable_Boss,           "Paldark.LootTable.Boss");

	// W35-36 — Pal Bond + capture (Tame minigame). One ability tag, three
	// Pal Sphere item identity tags, three tame result tags, three bond
	// event tags. Strings land under Paldark.Ability.* / Paldark.Item.* /
	// Paldark.Tame.* / Paldark.Bond.* so designer-side Project Settings →
	// GameplayTags stays one branch per concept.
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Ability_UsePalSphere,         "Paldark.Ability.UsePalSphere");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_PalSphere_T1,            "Paldark.Item.PalSphere.T1");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_PalSphere_T2,            "Paldark.Item.PalSphere.T2");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_PalSphere_T3,            "Paldark.Item.PalSphere.T3");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Tame_Result_Success,          "Paldark.Tame.Result.Success");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Tame_Result_Fail_HpTooHigh,   "Paldark.Tame.Result.Fail.HpTooHigh");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Tame_Result_Fail_RngMiss,     "Paldark.Tame.Result.Fail.RngMiss");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Bond_Event_Damage,            "Paldark.Bond.Event.Damage");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Bond_Event_Tame,              "Paldark.Bond.Event.Tame");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Bond_Event_ShareFood,         "Paldark.Bond.Event.ShareFood");

	// W37-38 — Inventory full. 17 item identity tags + 3 backpack tier tags
	// + 1 Equip.Slot.Backpack. Tag strings stay aligned with the asset names
	// designer authors in /Content/Paldark/Inventory/ so console commands +
	// future save-game blobs reference the same lookup key.
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Weapon_Rifle,             "Paldark.Item.Weapon.Rifle");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Weapon_Shotgun,           "Paldark.Item.Weapon.Shotgun");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Weapon_SMG,               "Paldark.Item.Weapon.SMG");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Weapon_Melee_Baton,       "Paldark.Item.Weapon.Melee.Baton");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Ammo_Rifle,               "Paldark.Item.Ammo.Rifle");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Ammo_Shotgun,             "Paldark.Item.Ammo.Shotgun");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Ammo_SMG,                 "Paldark.Item.Ammo.SMG");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Consumable_Medkit,        "Paldark.Item.Consumable.Medkit");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Consumable_Stimpack,      "Paldark.Item.Consumable.Stimpack");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Consumable_PalFood,       "Paldark.Item.Consumable.PalFood");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Throwable_Frag,           "Paldark.Item.Throwable.Frag");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Throwable_Smoke,          "Paldark.Item.Throwable.Smoke");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Throwable_Flash,          "Paldark.Item.Throwable.Flash");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Resource_Wood,            "Paldark.Item.Resource.Wood");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Resource_Stone,           "Paldark.Item.Resource.Stone");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Resource_PalCore,         "Paldark.Item.Resource.PalCore");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Resource_Currency_Credits,"Paldark.Item.Resource.Currency.Credits");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Backpack_T1,              "Paldark.Item.Backpack.T1");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Backpack_T2,              "Paldark.Item.Backpack.T2");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Item_Backpack_T3,              "Paldark.Item.Backpack.T3");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Equip_Slot_Backpack,           "Paldark.Equip.Slot.Backpack");

	// W40-41 — Hub Town (Q4 feature week #1). Map identity / experience identity /
	// hub building type / interact input / mode kind tag families. Strings land
	// under Paldark.Map.* / Paldark.Experience.* / Paldark.Hub.Building.* /
	// Paldark.InputTag.Interact.* / Paldark.Mode.* so designer-side Project
	// Settings → GameplayTags stays one branch per concept.
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Map_HubTown,                   "Paldark.Map.HubTown");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Experience_HubTown,            "Paldark.Experience.HubTown");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hub_Building_PalStable,        "Paldark.Hub.Building.PalStable");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hub_Building_Marketplace,      "Paldark.Hub.Building.Marketplace");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hub_Building_BriefingRoom,     "Paldark.Hub.Building.BriefingRoom");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_InputTag_Interact_Stable,      "Paldark.InputTag.Interact.Stable");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_InputTag_Interact_Marketplace, "Paldark.InputTag.Interact.Marketplace");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_InputTag_Interact_Briefing,    "Paldark.InputTag.Interact.Briefing");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Mode_Hub,                      "Paldark.Mode.Hub");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Mode_Raid,                     "Paldark.Mode.Raid");

	// W42-43 — AWS backend integration. 12 new tags total (4 state + 5
	// result + 3 fleet). State + Result strings deliberately mirror the
	// shape of `Paldark.Net.Mode.*` / `Paldark.Match.Outcome.*` from
	// earlier weeks so designers can grep the Project Settings tree by
	// one consistent convention.
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Backend_State_LoggedOut,        "Paldark.Backend.State.LoggedOut");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Backend_State_Authenticating,   "Paldark.Backend.State.Authenticating");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Backend_State_LoggedIn,         "Paldark.Backend.State.LoggedIn");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Backend_State_Refreshing,       "Paldark.Backend.State.Refreshing");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Backend_Result_Success,         "Paldark.Backend.Result.Success");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Backend_Result_Fail_Network,    "Paldark.Backend.Result.Fail.Network");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Backend_Result_Fail_Auth,       "Paldark.Backend.Result.Fail.Auth");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Backend_Result_Fail_Timeout,    "Paldark.Backend.Result.Fail.Timeout");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Backend_Result_Fail_NoFleet,    "Paldark.Backend.Result.Fail.NoFleet");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Backend_Fleet_Hub,              "Paldark.Backend.Fleet.Hub");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Backend_Fleet_Raid,             "Paldark.Backend.Fleet.Raid");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Backend_Fleet_Reserved,         "Paldark.Backend.Fleet.Reserved");

	// W44-45 — Hub UI (Pal Stable + Marketplace controller). 14 tags total
	// (4 UI state + 3 stable action + 2 market action + 5 shared result).
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hub_UI_Closed,                  "Paldark.Hub.UI.Closed");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hub_UI_OpeningStable,           "Paldark.Hub.UI.OpeningStable");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hub_UI_OpeningMarket,           "Paldark.Hub.UI.OpeningMarket");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hub_UI_Open,                    "Paldark.Hub.UI.Open");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hub_Stable_Action_Deposit,      "Paldark.Hub.Stable.Action.Deposit");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hub_Stable_Action_Withdraw,     "Paldark.Hub.Stable.Action.Withdraw");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hub_Stable_Action_Heal,         "Paldark.Hub.Stable.Action.Heal");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hub_Market_Action_Buy,          "Paldark.Hub.Market.Action.Buy");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hub_Market_Action_Sell,         "Paldark.Hub.Market.Action.Sell");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hub_Result_Success,             "Paldark.Hub.Result.Success");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hub_Result_Fail_Empty,          "Paldark.Hub.Result.Fail.Empty");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hub_Result_Fail_Full,           "Paldark.Hub.Result.Fail.Full");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hub_Result_Fail_Insufficient,   "Paldark.Hub.Result.Fail.Insufficient");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hub_Result_Fail_NotAllowed,     "Paldark.Hub.Result.Fail.NotAllowed");

	// W46 — Hub QA pass: briefing-room widget + multi-player vote/ready
	// handoff. 10 tags total (3 UI state + 4 action + 3 phase).
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hub_Brief_UI_Closed,            "Paldark.Hub.Brief.UI.Closed");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hub_Brief_UI_Opening,           "Paldark.Hub.Brief.UI.Opening");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hub_Brief_UI_Open,              "Paldark.Hub.Brief.UI.Open");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hub_Brief_Action_Vote,          "Paldark.Hub.Brief.Action.Vote");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hub_Brief_Action_Unvote,        "Paldark.Hub.Brief.Action.Unvote");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hub_Brief_Action_Ready,         "Paldark.Hub.Brief.Action.Ready");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hub_Brief_Action_Unready,       "Paldark.Hub.Brief.Action.Unready");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hub_Brief_Phase_Idle,           "Paldark.Hub.Brief.Phase.Idle");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hub_Brief_Phase_Countdown,      "Paldark.Hub.Brief.Phase.Countdown");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Hub_Brief_Phase_Travelling,     "Paldark.Hub.Brief.Phase.Travelling");

	// W47 — Save game system. 7 tags (4 result + 3 trigger). Strings land
	// under Paldark.Save.* so designer-side Project Settings → GameplayTags
	// stays one branch per concept. See the W47 README section for the
	// full result/trigger semantics.
	//
	// W48 — Adds the 4th trigger `Paldark.Save.Trigger.Travel` for the
	// hub→raid handoff smoke-save (and any future automatic save fired
	// before a ServerTravel boundary).
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Save_Result_Success,              "Paldark.Save.Result.Success");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Save_Result_Fail_IOError,         "Paldark.Save.Result.Fail.IOError");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Save_Result_Fail_SchemaMismatch,  "Paldark.Save.Result.Fail.SchemaMismatch");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Save_Result_Fail_NoSlot,          "Paldark.Save.Result.Fail.NoSlot");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Save_Trigger_HubReturn,           "Paldark.Save.Trigger.HubReturn");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Save_Trigger_Logout,              "Paldark.Save.Trigger.Logout");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Save_Trigger_Manual,              "Paldark.Save.Trigger.Manual");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Paldark_Save_Trigger_Travel,              "Paldark.Save.Trigger.Travel");
}
