// PALDARK W1 day 8-10 — Native gameplay tag singleton.
//
// Centralises the gameplay tag handles we reference from C++. Designers can still
// add Blueprint-only tags via Project Settings → GameplayTags; this file is for
// the ones that runtime code needs a compile-time reference to.
//
// Lives in PaldarkLabCore (PreDefault) so PaldarkLab (Default) and future
// PaldarkLabEditor (Editor) modules can both see the same tags without circular
// dependencies. Tag registration happens via `UE_DEFINE_GAMEPLAY_TAG_STATIC` in
// `PaldarkGameplayTags.cpp`; the engine's tag manager loads them at boot.
//
// Naming convention (mirrors Lyra):
//   Paldark.Experience.<...>     — gates the experience system itself.
//   Paldark.Pawn.<...>           — pawn-level capabilities.
//   Paldark.UI.<...>             — HUD / widget routing.
// Add new tags here as we need them; the W1 set is intentionally tiny.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace PaldarkGameplayTags
{
	// Marker tag granted by every experience action set so we can verify the
	// "action set applied" path end-to-end in W1 day 8-10.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Experience_Default);

	// Marker tag granted by the Raid_Sandbox action set bundle. Used by the
	// gameplay code path that distinguishes sandbox from hello-world.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Experience_RaidSandbox);

	// Pawn classification tags — first two cover the W1 → W3 player/Pal split.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Pawn_Player);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Pawn_Pal);

	// W1 day 11-14 — Input action tags. These are the gameplay-tag side of the
	// UPaldarkInputConfig mapping: designers point a tag at a UInputAction in
	// the data asset, and C++ binds the handler at SetupPlayerInputComponent
	// time using the tag here as the lookup key. Mirrors Lyra's InputTag.*
	// hierarchy so day 11-14 lessons port cleanly.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_InputTag_Move);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_InputTag_Look);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_InputTag_Jump);

	// W7-8 — Sprint ability input/ability/state hierarchy. The input tag is the
	// `AbilityInputActions` lookup key in UPaldarkInputConfig; the ability tag
	// is the AbilityTag on UPaldarkGameplayAbility_Sprint (used for cancel-by-
	// tag); the state tag is added/removed on the owner ASC while the ability
	// is active so HUD / AnimBP / activity selection can react.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_InputTag_Sprint);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Ability_Sprint);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_State_Sprinting);

	// W9-10 — Hitscan Fire ability tag chain. InputTag.Fire is the
	// `AbilityInputActions` lookup key, Ability.Fire is the AbilityTag on
	// UPaldarkGameplayAbility_HitscanFire (used for cancel-by-tag). Hit.Headshot
	// is added to the GameplayEffect spec when the hit hit a head bone — the
	// DamageExecutionCalculation reads it to apply the headshot multiplier.
	// State.IsDead is broadcast onto a target ASC when Health reaches zero.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_InputTag_Fire);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Ability_Fire);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hit_Headshot);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_State_IsDead);

	// W9-10 — SetByCaller magnitude tags. Authored damage GEs read these via
	// `FGameplayEffectSpec::SetSetByCallerMagnitude(tag, value)`; the
	// DamageExecutionCalculation queries them at execute time to compose the
	// final outgoing damage. Keeping them under SetByCaller.* makes the namespace
	// auditable in Project Settings → GameplayTags.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_SetByCaller_BaseDamage);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_SetByCaller_HeadshotMultiplier);

	// W5-6 — Pal Activity FSM identity tags. The activity component matches
	// a candidate activity by tag (for console-driven force-switch + future
	// data-driven activity sets) so designers can author Blueprint subclasses
	// of UPaldarkBaseActivity and bind them under the corresponding tag.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Pal_Activity_Idle);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Pal_Activity_Follow);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Pal_Activity_Investigate);

	// W11-12 — Inventory item identity tags. Each tag corresponds to one
	// authored UPaldarkItemDefinition asset (DA_Item_Pistol, DA_Item_Ammo_9mm,
	// DA_Item_Bandage, DA_Item_Pal_Sphere, DA_Item_Energy_Drink). The
	// inventory component looks items up by ItemTag (resolved from the
	// definition) so designer-renamed asset paths don't break console commands
	// or save data — only the tag string is the stable contract.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Pistol);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Ammo_9mm);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Bandage);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Pal_Sphere);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Energy_Drink);

	// W11-12 — Equipment slot tags. Used by UPaldarkItemFragment_Equipable's
	// EquipSlot field to declare which body slot the item targets. The full
	// equip-to-mesh / animation pose binding lands in W17-18 weapon polish;
	// W11-12 just records the slot so the inventory UI can filter equipables.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Equip_Slot_Primary);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Equip_Slot_Consumable);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Equip_Slot_Throwable);

	// W14-15 — Network role + topology tags. Read by UPaldarkNetSubsystem +
	// HUD code so we can distinguish "I am the host running listen server" vs
	// "I am a client on a dedicated server" without sprinkling
	// GetNetMode/GetLocalRole conditionals across the codebase. Tags are
	// authoritative once UPaldarkNetSubsystem::RefreshTopology() runs (called
	// on PostLogin and on every Open/Travel + after ServerTravel).
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Net_Role_Host);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Net_Role_Client);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Net_Mode_Standalone);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Net_Mode_ListenServer);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Net_Mode_DedicatedServer);

	// W16-17 — Server-side rewind / lag compensation result tags. The lag
	// compensation component returns one of these on every score request so
	// observers (HUD damage numbers, anti-cheat heuristics, debug command
	// `Paldark.LagComp.DumpHistory`) can branch on the outcome without
	// re-running the trace. HitConfirmed = rewind found the hitbox under the
	// client's HitLocation; HitRejected = rewound geometry no longer
	// contained the requested impact point (latency too large, target moved
	// out of trace cone, client lied). Hit.Bodyshot is the companion to the
	// existing Hit.Headshot tag — added so damage execution / VFX code can
	// distinguish "hit a hitbox other than head" from "no hit at all".
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_LagComp_HitConfirmed);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_LagComp_HitRejected);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hit_Bodyshot);

	// W18-19 — Pal Combat activity. Completes the Idle/Follow/Investigate/Combat
	// quartet so the Activity FSM has a top-priority preempt path when the
	// Pal's perception locks onto a hostile actor. The combat activity
	// instance ticks `UPaldarkPalCombatComponent::TryFireAttack` while the tag
	// is active; designers force the state via `Paldark.Pal.SetActivity Combat`
	// for debugging.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Pal_Activity_Combat);

	// W18-19 — Pal attack ability. AbilityTag on UPaldarkGameplayAbility_PalAttack;
	// the combat component activates it via `TryActivateAbilitiesByTag(this)`.
	// No corresponding InputTag — Pal attacks are AI-driven, not input-driven.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Ability_PalAttack);

	// W18-19 — Team identity tags. Pal perception scans actors within radius
	// and skips anything whose TeamTag matches its owner's; everything else
	// whose TeamTag matches HostileTeamTag (default Paldark.Team.Hostile)
	// counts as a threat. Player team is the default on APaldarkCharacter,
	// hostile is the default on APaldarkDummyTarget. Designer-authored Pal
	// species + future hostile mob subclasses can override per-Blueprint.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Team_Player);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Team_Hostile);

	// W18-19 — SetByCaller magnitude key for the Pal attack damage GE.
	// Authored separately from the player's `Paldark.SetByCaller.BaseDamage`
	// so designers can tune Pal damage vs player damage independently in the
	// damage execution. The execution falls back to BaseDamage when this tag
	// is absent on the spec so existing player-side GEs keep working.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_SetByCaller_PalDamage);

	// W20-21 — Hostile-Pal species identity tags. Stamped onto a hostile Pal's
	// SpeciesTag in its C++ subclass constructor. The pack subsystem uses
	// these (combined with a per-pack PackTag) to broadcast threat sightings
	// among same-species packmates without spilling across species (a
	// Direhound aggro doesn't pull in a Razorbird).
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Pal_Species_Direhound);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Pal_Species_Razorbird);

	// W20-21 — Hostile activity identity tags. Patrol (P15) is the always-on
	// fallback when a hostile Pal has waypoints but no threat. Stalk (P25)
	// preempts Patrol when perception sees a target but distance is still
	// outside `Combat.MinEngageRange` — the Pal closes in at reduced speed.
	// Combat (P40, W18-19) preempts Stalk once inside engage range.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Pal_Activity_Patrol);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Pal_Activity_Stalk);

	// W20-21 — Pack identity tag. Designer authors one pack tag per
	// Direhound spawner instance (e.g. `Paldark.Pal.Pack.Direhound`,
	// `Paldark.Pal.Pack.Direhound.North`) and the spawner stamps every
	// spawned Pal with the same tag. The pack subsystem broadcasts a
	// threat sighting from any pack member to every other member within
	// `PackBroadcastRadius` (default 2000 cm).
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Pal_Pack_Direhound);

	// W22-23 — Squad system tags. `Paldark.Squad.Default` is the membership
	// tag every freshly-spawned `APaldarkCharacter` registers under so a
	// 4-player raid lobby acts as one squad without designer setup. Per-pawn
	// override is supported via `UPaldarkSquadMembershipComponent::SquadTag`
	// (e.g. PvP teams as `Paldark.Squad.Red` / `Paldark.Squad.Blue` in W40+).
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Squad_Default);

	// W22-23 — Squad radial command tags. Stamped onto the
	// `UPaldarkSquadCommandSet` rows the designer authors, and matched by
	// `UPaldarkActivity_FollowSquadCommand::Tick` to pick which behaviour
	// to drive on Pal companions. Attack moves toward + auto-aggros at
	// the most recent enemy ping; Follow regroups on the issuing player;
	// Stay holds position; Regroup pulls every Pal back to the issuer.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Squad_Command_Attack);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Squad_Command_Follow);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Squad_Command_Stay);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Squad_Command_Regroup);

	// W22-23 — Ping type tags. The ping subsystem stamps the type onto the
	// spawned `APaldarkPingMarker` so the HUD widget (W22+ UMG task) can
	// pick the icon + sound. Enemy pings additionally cache the marked
	// actor so the marker follows it for the duration of its lifetime.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Ping_Type_Spot);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Ping_Type_Enemy);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Ping_Type_Loot);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Ping_Type_Danger);

	// W22-23 — Input tag for the squad radial wheel + ping + mark-enemy
	// hotkeys. Designer binds IMC rows to T (ping), V (mark enemy under
	// crosshair), Q (open radial wheel). C++ handlers route through these
	// tags so the binding stays designer-side.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_InputTag_Ping);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_InputTag_MarkEnemy);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_InputTag_RadialOpen);

	// W22-23 — Activity tag for the Pal "follow squad command" listener
	// (P22). Priority sits between Follow (P20) and Investigate (P30) so
	// active investigations and combat take precedence, but a sitting
	// idle Pal will respond to a Follow command. Only on the friendly
	// companion's CandidateActivities list — hostile-Pal subclasses
	// (W20-21 Direhound / Razorbird) intentionally skip it.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Pal_Activity_FollowSquadCommand);

	// W24-25 — Match phase identity tags. The match subsystem's
	// `OnMatchPhaseChanged` delegate ships the new phase tag so HUD
	// widgets, console commands, and AI activities can branch off a
	// single tag enum instead of duplicating a state-machine read.
	//   Warmup     — match created, lobby loaded, players are still
	//                joining; no extraction allowed.
	//   Active     — full raid: players fight, loot, head toward a
	//                beacon. Extraction beacons accept overlap.
	//   Extracting — at least one beacon timer is in flight. Bookkeeping
	//                tag — does not actually gate logic, but useful for
	//                HUD "Extraction in progress" banner.
	//   Ended      — match concluded. PlayerOutcome is final, no further
	//                damage/extraction processed; the GameMode is awaiting
	//                a server travel back to lobby.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Match_Phase_Warmup);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Match_Phase_Active);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Match_Phase_Extracting);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Match_Phase_Ended);

	// W24-25 — Per-player outcome tags. Stamped onto APaldarkPlayerState
	// via OutcomeTag once the player either extracts successfully, dies,
	// or disconnects. Read by the (designer-authored) end-screen widget
	// in W22+ polish and by `Paldark.Match.Dump` for verification.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Match_Outcome_Extracted);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Match_Outcome_KIA);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Match_Outcome_Disconnected);

	// W24-25 — Input tag for the "consent to extract" hotkey. Designer
	// binds E (or any IA) in IMC_Default to the InteractExtraction tag;
	// the player controller routes the press to the beacon that the
	// player is currently overlapping. Without this hotkey the beacon
	// still ticks on passive overlap, but designers may flip the beacon
	// to require explicit consent (`bRequireExplicitConsent` on beacon).
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_InputTag_InteractExtraction);

	// W29-30 — 5 new Pal species tags. Stoneclad / Vinewraith / Boltmane
	// are hostile-Pal C++ subclasses (`APaldarkPalCharacter_<Species>`)
	// stamped in their constructor. Foxparks / Tombat are companion
	// species authored as `DA_PalDef_*` data assets via the W27-28
	// `UPaldarkPalDefinition` path (no per-species C++ subclass).
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Pal_Species_Stoneclad);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Pal_Species_Vinewraith);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Pal_Species_Foxparks);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Pal_Species_Boltmane);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Pal_Species_Tombat);

	// W29-30 — Per-species pack tags for the hostile species that come
	// in packs (Stoneclad + Vinewraith). Boltmane is a solo boss — no
	// pack tag. Foxparks / Tombat are companions — no pack tag.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Pal_Pack_Stoneclad);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Pal_Pack_Vinewraith);

	// W29-30 — Boltmane boss phase activity tag (P50, replaces Combat P40
	// on the Boltmane subclass's CandidateActivities). Set via
	// `UPaldarkActivity_BossPhase::ActivityTag` in the ctor. The activity's
	// phase enum (`EPaldarkBossPhase`) is a separate enum value — not a
	// gameplay tag — since phases are mutually exclusive ordinals.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Pal_Activity_BossPhase);

	// W29-30 — 5 per-species Pal attack ability tags. Each tag is added
	// to the matching `UPaldarkGameplayAbility_PalAttack_<X>` subclass's
	// AbilityTags container in its ctor, and the base
	// `Paldark.Ability.PalAttack` tag is preserved so tag-driven
	// activation (`TryActivateAbilitiesByTag`) still matches generically.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Ability_PalAttack_Bite);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Ability_PalAttack_VineWhip);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Ability_PalAttack_Charge);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Ability_PalAttack_FireBreath);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Ability_PalAttack_ChainLightning);

	// W31-32 — Map 2 "Rừng Hỏng" blockout. Six POI type tags identify
	// the kinds of points-of-interest a designer can drop onto a level
	// (Outpost / Cache / RuinedTower / MiniBoss / ExtractionPad /
	// SafeZone). Three danger-tier tags coarsely bin POIs by how dangerous
	// the surrounding encounter is (Low / Medium / High). Two map-ID tags
	// identify the playable maps shipped so far (RaidSandbox retro-fit
	// from W1 + RungHong introduced this PR). One activity tag is reserved
	// for the spawn-point listener so designers can debug per-point spawn
	// timing without colliding with the existing Patrol / Stalk / Combat
	// / BossPhase activity tags.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_POI_Type_Outpost);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_POI_Type_Cache);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_POI_Type_RuinedTower);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_POI_Type_MiniBoss);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_POI_Type_ExtractionPad);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_POI_Type_SafeZone);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_POI_DangerTier_Low);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_POI_DangerTier_Medium);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_POI_DangerTier_High);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Map_RaidSandbox);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Map_RungHong);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Pal_Activity_SpawnPoint);

	// W33-34 — Map 3 "Cơ Xưởng PalCorp" + tech-themed loot drop. One map
	// identity tag for the new map. One POI type tag (`IndoorRoom`) so the
	// designer can drop close-quarter POIs without conflating them with the
	// outdoor types from W31-32. Five tech-themed item identity tags — one
	// per authored `DA_Item_Tech_<X>` data asset. Two loot table identity
	// tags — one for the standard hostile-Pal drop pool, one for the boss
	// (Boltmane) drop pool — stamped onto authored `DA_LootTable_*` data
	// assets so console commands and validator can resolve tables by tag
	// without depending on asset paths.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Map_PalCorp);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_POI_Type_IndoorRoom);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Tech_Battery);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Tech_Circuit);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Tech_Coolant);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Tech_Polymer);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Tech_Microchip);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_LootTable_HostileStandard);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_LootTable_Boss);

	// W35-36 — Pal Bond + capture (Tame minigame). Ten new tags split
	// across four sub-namespaces:
	//   Paldark.Ability.UsePalSphere       — AbilityTag for the server-side
	//                                        GA that consumes a Pal Sphere
	//                                        from inventory and spawns the
	//                                        replicated APaldarkPalSphere
	//                                        projectile.
	//   Paldark.Item.PalSphere.{T1,T2,T3}  — Item identity tags for the
	//                                        three Pal Sphere tiers. Each
	//                                        tier maps to a multiplier on
	//                                        the capture probability formula
	//                                        (T1 baseline 1.0, T2 1.5, T3 2.0).
	//   Paldark.Tame.Result.{Success,Fail.HpTooHigh,Fail.RngMiss}
	//                                      — Outcome tags broadcast after
	//                                        `UPaldarkPalTameComponent::BeginTameAttempt`
	//                                        resolves. HUD widget reads
	//                                        which subtag fired to pick the
	//                                        feedback line.
	//   Paldark.Bond.Event.{Damage,Tame,ShareFood}
	//                                      — Reason tags passed to
	//                                        `UPaldarkPalBondComponent::AddBondXP`
	//                                        so designers can configure
	//                                        per-event XP rewards from a
	//                                        single curve table.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Ability_UsePalSphere);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_PalSphere_T1);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_PalSphere_T2);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_PalSphere_T3);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Tame_Result_Success);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Tame_Result_Fail_HpTooHigh);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Tame_Result_Fail_RngMiss);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Bond_Event_Damage);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Bond_Event_Tame);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Bond_Event_ShareFood);

	// W37-38 — Inventory full. Brings the authored item catalog from 13 to 30.
	// Adds 17 item identity tags spread across Weapon / Ammo / Consumable /
	// Throwable / Resource categories so the W47 save-game can blob an entire
	// loadout without a string-fallback. Three backpack tier tags map to the
	// three authored DA_Item_Backpack_T{1,2,3} assets that boost
	// `UPaldarkPlayerInventoryComponent::GetMaxWeightKg`. One new equip slot
	// tag (`Paldark.Equip.Slot.Backpack`) selects the backpack slot;
	// designers wire it onto the backpack item's Equipable fragment.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Weapon_Rifle);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Weapon_Shotgun);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Weapon_SMG);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Weapon_Melee_Baton);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Ammo_Rifle);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Ammo_Shotgun);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Ammo_SMG);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Consumable_Medkit);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Consumable_Stimpack);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Consumable_PalFood);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Throwable_Frag);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Throwable_Smoke);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Throwable_Flash);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Resource_Wood);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Resource_Stone);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Resource_PalCore);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Resource_Currency_Credits);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Backpack_T1);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Backpack_T2);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Item_Backpack_T3);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Equip_Slot_Backpack);

	// W40-41 — Hub Town (Q4 feature week #1). Ten new tags split across four
	// sub-namespaces:
	//   Paldark.Map.HubTown                  — Map identity tag for the
	//                                          designer-authored `Map_HubTown.umap`.
	//                                          Carried on the `DA_MapDef_HubTown`
	//                                          PrimaryDataAsset and asserted by
	//                                          `check_w40_hub_shape` so a
	//                                          renamed asset does not break
	//                                          briefing-room handoff.
	//   Paldark.Experience.HubTown           — Experience identity tag for
	//                                          `PX_HubTown` PrimaryAsset. Lets
	//                                          the briefing room route raid
	//                                          handoff back to the hub by tag
	//                                          rather than by hard-coded asset
	//                                          path.
	//   Paldark.Hub.Building.{PalStable,     — Hub building type tags stamped
	//   Marketplace,BriefingRoom}              onto the per-instance
	//                                          `BuildingTag` UPROPERTY by the
	//                                          C++ ctor of each subclass.
	//                                          `UPaldarkHubSubsystem::GetBuildingsByTag`
	//                                          queries by tag so future UMG
	//                                          / interact code looks up a
	//                                          stable contract instead of
	//                                          UClass.
	//   Paldark.InputTag.Interact.{Stable,   — Input action tags for the three
	//   Marketplace,Briefing}                  hub-specific interact prompts.
	//                                          Designer binds the prompt key
	//                                          (E by default) in the hub IMC,
	//                                          C++ dispatches to the matching
	//                                          `OnInteract` override on the
	//                                          building under the cursor.
	//   Paldark.Mode.{Hub,Raid}              — Kind-of-mode tags so the
	//                                          GameMode can self-describe to
	//                                          HUD / debug code without
	//                                          string-matching on UClass
	//                                          names.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Map_HubTown);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Experience_HubTown);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hub_Building_PalStable);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hub_Building_Marketplace);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hub_Building_BriefingRoom);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_InputTag_Interact_Stable);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_InputTag_Interact_Marketplace);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_InputTag_Interact_Briefing);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Mode_Hub);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Mode_Raid);

	// W42-43 — AWS backend integration. 13 new tags split across three
	// sub-namespaces:
	//   Paldark.Backend.State.{LoggedOut,Authenticating,LoggedIn,Refreshing}
	//                 — Mirrors UPaldarkBackendSubsystem::StateTag. UMG
	//                   widgets bind to OnLoginCompleted / OnTokenRefreshed
	//                   and read StateTag for "show login spinner" /
	//                   "show error toast" branching.
	//   Paldark.Backend.Result.{Success,Fail.Network,Fail.Auth,Fail.Timeout,Fail.NoFleet}
	//                 — Carried on FPaldarkBackendError::ResultTag. UMG can
	//                   `Switch on GameplayTag` to pick the right error
	//                   string + retry policy without scraping the HTTP
	//                   status code.
	//   Paldark.Backend.Fleet.{Hub,Raid,Reserved}
	//                 — Carried on FPaldarkFleetSpec::FleetTag. The
	//                   NetSubsystem reads this to decide which pre-travel
	//                   hook fires (clear inventory on raid, restore
	//                   roster on hub). Reserved is a placeholder for the
	//                   W44+ private hub feature (paid tier).
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Backend_State_LoggedOut);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Backend_State_Authenticating);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Backend_State_LoggedIn);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Backend_State_Refreshing);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Backend_Result_Success);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Backend_Result_Fail_Network);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Backend_Result_Fail_Auth);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Backend_Result_Fail_Timeout);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Backend_Result_Fail_NoFleet);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Backend_Fleet_Hub);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Backend_Fleet_Raid);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Backend_Fleet_Reserved);

	// W44-45 — Hub UI (Pal Stable + Marketplace controller). 14 new tags
	// split across four sub-namespaces:
	//   Paldark.Hub.UI.{Closed,OpeningStable,OpeningMarket,Open}
	//                 — UMG state machine. The controller component on the
	//                   PlayerController flips between these as the
	//                   building's `OnInteract` request flows through the
	//                   server snapshot RPC and back. Closed is the rest
	//                   state; OpeningStable/OpeningMarket are the
	//                   "snapshot RPC in flight" intermediate; Open is
	//                   the steady state while the widget is on screen.
	//   Paldark.Hub.Stable.Action.{Deposit,Withdraw,Heal}
	//                 — Stamped onto the FOnStableActionResult delegate
	//                   payload so UMG / debug code can branch off a
	//                   tag instead of an enum. Heal is the W44-45 hook
	//                   that converts credits into HealthAtTame on a
	//                   deposited Pal entry.
	//   Paldark.Hub.Market.Action.{Buy,Sell}
	//                 — Same shape, but for marketplace transactions.
	//                   Carried on FOnMarketActionResult.
	//   Paldark.Hub.Result.{Success,Fail.Empty,Fail.Full,Fail.Insufficient,Fail.NotAllowed}
	//                 — Shared between the stable + marketplace result
	//                   delegates. Empty = source roster slot empty;
	//                   Full = destination roster / inventory at cap;
	//                   Insufficient = not enough credits / stack;
	//                   NotAllowed = item not in catalog, not authority,
	//                   or generic permission failure.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hub_UI_Closed);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hub_UI_OpeningStable);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hub_UI_OpeningMarket);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hub_UI_Open);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hub_Stable_Action_Deposit);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hub_Stable_Action_Withdraw);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hub_Stable_Action_Heal);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hub_Market_Action_Buy);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hub_Market_Action_Sell);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hub_Result_Success);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hub_Result_Fail_Empty);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hub_Result_Fail_Full);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hub_Result_Fail_Insufficient);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hub_Result_Fail_NotAllowed);

	// W46 — Hub QA pass: briefing-room widget + multi-player vote/ready
	// handoff. The W40-41 briefing room was a single-player console-driven
	// path; W46 adds the actual lobby-style flow where every player in the
	// hub votes for a raid map and signals ready, and the server runs the
	// countdown + ServerTravels the whole shard together.
	//
	//   Paldark.Hub.Brief.UI.{Closed,Opening,Open}
	//                 — Per-player briefing widget UI state. Closed = no
	//                   widget mounted; Opening = OnInteract fired,
	//                   waiting for snapshot RPC; Open = snapshot received,
	//                   widget showing map picker + ready button.
	//   Paldark.Hub.Brief.Action.{Vote,Unvote,Ready,Unready}
	//                 — Stamped onto FOnBriefingActionResult delegate
	//                   payload so UMG / debug code can branch off a
	//                   tag instead of an enum.
	//   Paldark.Hub.Brief.Phase.{Idle,Countdown,Travelling}
	//                 — Replicated session-wide phase on
	//                   UPaldarkBriefingSessionComponent (lives on
	//                   GameState). Idle = waiting for votes; Countdown =
	//                   majority vote + ready threshold reached, N-second
	//                   timer counting down; Travelling = countdown
	//                   completed, ServerTravel issued.
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hub_Brief_UI_Closed);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hub_Brief_UI_Opening);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hub_Brief_UI_Open);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hub_Brief_Action_Vote);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hub_Brief_Action_Unvote);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hub_Brief_Action_Ready);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hub_Brief_Action_Unready);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hub_Brief_Phase_Idle);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hub_Brief_Phase_Countdown);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Hub_Brief_Phase_Travelling);

	// W47 — Save game system. The save subsystem is console-driven (W47
	// scope: auto-save / UMG save slot picker are W48 polish). Seven tags
	// across two sub-namespaces:
	//
	//   Paldark.Save.Result.{Success,Fail.IOError,Fail.SchemaMismatch,Fail.NoSlot}
	//                  — Result tag stamped on the `OnSaveCompleted` /
	//                    `OnLoadCompleted` delegate payload. UMG load-game
	//                    picker can `Switch on GameplayTag` to surface the
	//                    right error toast.
	//                      * Success           — async op finished, blob on
	//                                            disk / applied to live PC.
	//                      * Fail.IOError      — `AsyncSaveGameToSlot` /
	//                                            `AsyncLoadGameFromSlot`
	//                                            returned false. Disk full,
	//                                            file locked, etc.
	//                      * Fail.SchemaMismatch — load found a slot but its
	//                                            `SchemaVersion` doesn't
	//                                            match `kCurrentSchema`.
	//                                            Reserved for future schema
	//                                            bumps.
	//                      * Fail.NoSlot       — load requested but the slot
	//                                            doesn't exist on disk
	//                                            (designer mistyped the
	//                                            slot name).
	//   Paldark.Save.Trigger.{HubReturn,Logout,Manual,Travel}
	//                  — Trigger tag the *caller* passes to RequestSaveSlot
	//                    so the OnSaveCompleted listener can branch off
	//                    "why did we save?".
	//                      * HubReturn — post-raid auto-save on extraction
	//                                    (wired W48; UPaldarkSaveSubsystem
	//                                    listens to OnPlayerOutcomeChanged
	//                                    Extracted).
	//                      * Logout    — pre-disconnect auto-save (wired
	//                                    W48; UPaldarkSaveSubsystem listens
	//                                    to UPaldarkNetSubsystem
	//                                    OnPlayerLogout).
	//                      * Manual    — console / explicit pause-menu
	//                                    save (W47 ships this path; UMG
	//                                    save slot picker stub W48).
	//                      * Travel    — ServerTravel / map handoff smoke
	//                                    save (W48 hub-raid handoff console
	//                                    command; future automatic on
	//                                    extraction-room → raid travel).
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Save_Result_Success);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Save_Result_Fail_IOError);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Save_Result_Fail_SchemaMismatch);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Save_Result_Fail_NoSlot);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Save_Trigger_HubReturn);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Save_Trigger_Logout);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Save_Trigger_Manual);
	PALDARKLABCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Paldark_Save_Trigger_Travel);
}
