// PALDARK W1 — Primary game module umbrella header. Re-exports core log categories so
// game-side files only need `#include "PaldarkLab.h"`.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "PaldarkLogCategories.h"

class IConsoleObject;

class FPaldarkLabModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	// Console command(s) owned by this module. Held as raw pointer because
	// IConsoleManager owns the lifetime; we just need to unregister on shutdown.
	IConsoleObject* HelloWorldCommand = nullptr;
	IConsoleObject* ExperienceCurrentCommand = nullptr;
	IConsoleObject* ExperienceHelloCommand = nullptr;
	IConsoleObject* ExperienceListExtensionsCommand = nullptr;
	// W1 day 11-14 — Inspect the local player's UEnhancedInputLocalPlayerSubsystem
	// + active PawnData InputConfig.
	IConsoleObject* InputListBindingsCommand = nullptr;
	// W3-4 — Spawn one Pal companion behind the local player (debug helper).
	// Args: optional `index` (which row of `DefaultPalCompanions` to spawn,
	// default 0) and optional `class_path` (override the PawnData class).
	IConsoleObject* PalSpawnTestCompanionCommand = nullptr;

	// W5-6 — Activity FSM debug helpers.
	// `Paldark.Pal.CurrentActivity` logs the active activity tag for every
	// Pal in the world. `Paldark.Pal.SetActivity <Idle|Follow|Investigate>`
	// force-switches every Pal to the requested state. `Paldark.Pal.Ping
	// [<X> <Y> <Z>]` files an Investigate request at the requested world
	// location (defaults to the local player's location), preempting Follow.
	IConsoleObject* PalCurrentActivityCommand = nullptr;
	IConsoleObject* PalSetActivityCommand     = nullptr;
	IConsoleObject* PalPingCommand            = nullptr;

	// W7-8 — GAS debug helpers.
	// `Paldark.Gas.DumpAttributes` prints the live attribute snapshot
	// (Health / Stamina / MoveSpeed / Armor) for every actor in the world
	// that implements IAbilitySystemInterface. `Paldark.Gas.Damage <Amount>`
	// applies a one-shot damage GE (Health -= Amount) to the local player —
	// useful for testing the W9-10 damage chain before the GE asset exists,
	// using a SetByCaller magnitude on a designer-authored
	// GE_Damage_Standard.
	IConsoleObject* GasDumpAttributesCommand = nullptr;
	IConsoleObject* GasDamageCommand         = nullptr;

	// W9-10 — Combat / damage chain debug helpers.
	// `Paldark.Combat.SpawnDummy [Distance]` spawns one APaldarkDummyTarget
	// in front of the local player so designers can validate the GE +
	// execution calc path without needing a full level layout.
	// `Paldark.Combat.Fire` activates the Fire ability on the local player
	// from the console (skipping the input binding) — useful when iterating
	// on the trace / damage formula without unbinding keyboard input.
	IConsoleObject* CombatSpawnDummyCommand = nullptr;
	IConsoleObject* CombatFireCommand       = nullptr;

	// W11-12 — Inventory fragment debug helpers. These four commands let a
	// designer exercise the inventory loop end-to-end before the UMG widget
	// exists. All operate on the local player's first
	// UPaldarkPlayerInventoryComponent (the only one in W11-12).
	//   `Paldark.Inventory.List`                          — DumpToLog snapshot.
	//   `Paldark.Inventory.Add    <ItemTag> [Count]`      — server-add by tag.
	//   `Paldark.Inventory.Remove <ItemTag> [Count]`      — server-remove by tag.
	//   `Paldark.Inventory.Drop`                          — DropAllItems on server.
	IConsoleObject* InventoryListCommand   = nullptr;
	IConsoleObject* InventoryAddCommand    = nullptr;
	IConsoleObject* InventoryRemoveCommand = nullptr;
	IConsoleObject* InventoryDropCommand   = nullptr;

	// W37-38 — Inventory full debug helpers. Four console commands that
	// exercise the 30-item catalog + backpack tiers + composite container
	// + drop-on-death pipeline end-to-end before the UMG inventory grid
	// lands.
	//   `Paldark.Inventory.GiveAll`                        — server-add 1 of every PaldarkItem asset registered with the AssetManager.
	//   `Paldark.Inventory.EquipBackpack <T1|T2|T3>`      — looks up the matching backpack ItemDef by Paldark.Item.Backpack.<tier> and equips it.
	//   `Paldark.Inventory.TestDeathDrop`                  — invokes APaldarkCharacter::SpawnDeathLootBagFromInventory() without applying damage.
	//   `Paldark.Inventory.DumpComposite`                  — logs entries plus any nested container InnerEntries (one level deep).
	IConsoleObject* InventoryGiveAllCommand       = nullptr;
	IConsoleObject* InventoryEquipBackpackCommand = nullptr;
	IConsoleObject* InventoryTestDeathDropCommand = nullptr;
	IConsoleObject* InventoryDumpCompositeCommand = nullptr;

	// W27-28 — AssetManager async-load + spawn pipeline debug helpers.
	//   `Paldark.Pal.SpawnFromDefinition <DefId> [X Y Z]` — kicks off
	//      `UPaldarkPalSpawnSubsystem::SpawnPalAsync` for the named
	//      PaldarkPalDefinition primary asset id. Resolves the Pal class +
	//      mesh + anim instance via the "Spawn" asset bundle. Pre-warms on
	//      miss; subsequent calls hit the warm cache. Defaults the spawn
	//      transform to 4m in front of the local player.
	//   `Paldark.Pal.DumpDefinitionRegistry` — prints every primary asset id
	//      currently tracked by the spawn subsystem along with its pre-warm
	//      state (Resolved / InFlight / Failed) so a designer can verify
	//      that the experience's `PreWarmPalDefinitions` row resolved.
	IConsoleObject* PalSpawnFromDefinitionCommand   = nullptr;
	IConsoleObject* PalDumpDefinitionRegistryCommand = nullptr;

	// W44-45 — Stable + Marketplace UI debug helpers. Eight console commands
	// that exercise the W44-45 plumbing without authoring the UMG layout.
	// All operate on the local player's PaldarkPlayerController, which is
	// where the W44-45 controller components live, plus its PlayerState
	// (which holds the deposit component).
	//   `Paldark.Hub.Stable.List`                          — dump active roster + deposit list.
	//   `Paldark.Hub.Stable.Deposit  <RosterIndex>`        — controller->RequestDeposit.
	//   `Paldark.Hub.Stable.Withdraw <DepositIndex>`       — controller->RequestWithdraw.
	//   `Paldark.Hub.Stable.Heal     <RosterIndex>`        — controller->RequestHeal (debit credits, refill HP).
	//   `Paldark.Hub.Market.Catalog [KioskName]`           — dump kiosk catalog (resolved item def + buy/sell prices).
	//   `Paldark.Hub.Market.Buy  <ItemTag> [Count=1]`      — controller->RequestBuy.
	//   `Paldark.Hub.Market.Sell <ItemTag> [Count=1]`      — controller->RequestSell.
	//   `Paldark.Hub.Market.Balance`                       — log local player's credits balance.
	IConsoleObject* StableListCommand     = nullptr;
	IConsoleObject* StableDepositCommand  = nullptr;
	IConsoleObject* StableWithdrawCommand = nullptr;
	IConsoleObject* StableHealCommand     = nullptr;
	IConsoleObject* MarketCatalogCommand  = nullptr;
	IConsoleObject* MarketBuyCommand      = nullptr;
	IConsoleObject* MarketSellCommand     = nullptr;
	IConsoleObject* MarketBalanceCommand  = nullptr;

	// W46 — Briefing room (hub QA pass) debug helpers. Six console commands
	// that exercise the W46 vote/ready/countdown loop without authoring the
	// UMG layout. All operate on the local player's PaldarkPlayerController +
	// the shard-wide UPaldarkBriefingSessionComponent (on GameState).
	//   `Paldark.Hub.Brief.Dump`            — dump session phase + tallies + voters.
	//   `Paldark.Hub.Brief.Vote <MapTag>`   — controller->RequestVote(MapTag).
	//   `Paldark.Hub.Brief.Unvote`          — controller->RequestUnvote.
	//   `Paldark.Hub.Brief.Ready`           — controller->RequestReady.
	//   `Paldark.Hub.Brief.Unready`         — controller->RequestUnready.
	//   `Paldark.Hub.QA.ForceTravel <MapTag> [MapName]`
	//                                       — server force-travel bypass; jumps the
	//                                         session straight to Travelling phase
	//                                         and issues HostHubServer. Authority
	//                                         only. Designer / QA only.
	IConsoleObject* BriefDumpCommand    = nullptr;
	IConsoleObject* BriefVoteCommand    = nullptr;
	IConsoleObject* BriefUnvoteCommand  = nullptr;
	IConsoleObject* BriefReadyCommand   = nullptr;
	IConsoleObject* BriefUnreadyCommand = nullptr;
	IConsoleObject* QAForceTravelCommand = nullptr;

	// W47 — Save game (USaveGame for player progression). Six console
	// commands exercise the W47 save subsystem (UPaldarkSaveSubsystem)
	// without an authored UMG slot picker; the picker + auto-save hook
	// ship in W48 polish.
	//   `Paldark.Save.Save  [SlotName] [TriggerTag]`     — capture snapshot + AsyncSaveGameToSlot.
	//   `Paldark.Save.Load  [SlotName]`                  — AsyncLoadGameFromSlot + apply to live PC.
	//   `Paldark.Save.Dump  [SlotName]`                  — AsyncLoadGameFromSlot + log DescribeForLog (no apply).
	//   `Paldark.Save.ClearSlot [SlotName]`              — DeleteSlot from disk.
	//   `Paldark.Save.ListSlots`                         — log every slot we can see + schema version.
	//   `Paldark.QA.WipeAllSlots`                        — nuke every Paldark.Save.* slot we know about (designer / QA only).
	IConsoleObject* SaveSaveCommand        = nullptr;
	IConsoleObject* SaveLoadCommand        = nullptr;
	IConsoleObject* SaveDumpCommand        = nullptr;
	IConsoleObject* SaveClearSlotCommand   = nullptr;
	IConsoleObject* SaveListSlotsCommand   = nullptr;
	IConsoleObject* QAWipeAllSlotsCommand  = nullptr;

	// W48 — Polish (auto-save hooks + UMG slot picker stub + hub→raid
	// handoff smoke). One extra QA console command:
	//   `Paldark.QA.HubToRaidHandoff [SlotName]` — fires the W47 save
	//                                              pipeline tagged with
	//                                              `Paldark.Save.Trigger.Travel`
	//                                              so a designer can grep
	//                                              the log for the hub→raid
	//                                              boundary save without
	//                                              actually invoking
	//                                              ServerTravel. Authority
	//                                              not required (the save
	//                                              path is local-PC-scope).
	IConsoleObject* QAHubToRaidHandoffCommand = nullptr;
};
