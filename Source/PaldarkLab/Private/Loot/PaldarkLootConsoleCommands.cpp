// PALDARK W33-34 — Loot console commands.
//
// Three console commands registered as `FAutoConsoleCommandWithWorldAndArgs`:
//
//   Paldark.Loot.DumpTable <PaldarkLootTable.RowName>
//     Async-loads a `UPaldarkLootTable` by primary asset row name and logs
//     the resolved LootTableTag / MaxRolls / Entries breakdown. Verifies
//     the AssetManager scan rule picked up a freshly-authored
//     `DA_LootTable_<X>` without firing OnHealthZeroed on a real Pal.
//
//   Paldark.Loot.ForceDrop <PalLabelOrName>
//     Looks up an `APaldarkPalCharacter` by partial name / label match in
//     the active world, then calls
//     `UPaldarkLootDropComponent::ForceDropAt(GetActorTransform())` on its
//     LootDropSlot. Bypasses the OnHealthZeroed pipeline so testers can
//     verify loot wiring without first killing the Pal.
//
//   Paldark.Loot.PickupNearest
//     Finds the nearest non-empty `APaldarkLootBag` to the local player
//     pawn (radius unlimited) and calls `RequestPickup(LocalPlayer)`.
//     Server / standalone only — clients route picks through overlap.
//
// Roadmap reference: Documents/PALDARK/03-Roadmap_1_Year.md § Tuần 33-34.

#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Inventory/PaldarkItemDefinition.h"
#include "Loot/PaldarkLootBag.h"
#include "Loot/PaldarkLootDropComponent.h"
#include "Loot/PaldarkLootTable.h"
#include "Pal/PaldarkPalCharacter.h"
#include "PaldarkLogCategories.h"
#include "Player/PaldarkCharacter.h"
#include "UObject/PrimaryAssetId.h"

namespace PaldarkLootConsole
{
	static void DumpTable(const TArray<FString>& Args, UWorld* World)
	{
		if (!World)
		{
			UE_LOG(LogPaldarkInventory, Warning, TEXT("Paldark.Loot.DumpTable — no world."));
			return;
		}
		if (Args.Num() == 0)
		{
			UE_LOG(LogPaldarkInventory, Warning,
				TEXT("Paldark.Loot.DumpTable — usage: Paldark.Loot.DumpTable <RowName>"));
			return;
		}
		UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
		if (!AssetManager)
		{
			UE_LOG(LogPaldarkInventory, Warning,
				TEXT("Paldark.Loot.DumpTable — AssetManager not initialised."));
			return;
		}

		const FName RowName(*Args[0]);
		const FPrimaryAssetId TableId(FPrimaryAssetType(TEXT("PaldarkLootTable")), RowName);
		UE_LOG(LogPaldarkInventory, Log,
			TEXT("Paldark.Loot.DumpTable — async-loading %s."), *TableId.ToString());

		AssetManager->LoadPrimaryAsset(TableId, TArray<FName>(), FStreamableDelegate::CreateLambda(
			[TableId]()
			{
				UAssetManager& Mgr = UAssetManager::Get();
				const UObject* Loaded = Mgr.GetPrimaryAssetObject(TableId);
				const UPaldarkLootTable* Table = Cast<UPaldarkLootTable>(Loaded);
				if (!Table)
				{
					UE_LOG(LogPaldarkInventory, Warning,
						TEXT("Paldark.Loot.DumpTable — failed to resolve %s. Check DA_LootTable_<X> under /Game/Paldark/Loot/."),
						*TableId.ToString());
					return;
				}
				UE_LOG(LogPaldarkInventory, Log,
					TEXT("Paldark.Loot.DumpTable — %s: Tag=%s, MaxRolls=%d, Entries=%d."),
					*Table->GetDebugLabel(),
					*Table->LootTableTag.ToString(),
					Table->MaxRolls,
					Table->Entries.Num());
				int32 TotalWeight = 0;
				for (const FPaldarkLootEntry& E : Table->Entries)
				{
					if (E.IsValid()) { TotalWeight += E.Weight; }
				}
				for (int32 Idx = 0; Idx < Table->Entries.Num(); ++Idx)
				{
					const FPaldarkLootEntry& E = Table->Entries[Idx];
					const float Pct = (TotalWeight > 0 && E.IsValid())
						? (100.f * E.Weight / TotalWeight) : 0.f;
					UE_LOG(LogPaldarkInventory, Log,
						TEXT("  [%d] item=%s weight=%d (%.1f%%) count=[%d..%d] guaranteed=%s"),
						Idx,
						*E.ItemDefinition.ToString(),
						E.Weight,
						Pct,
						E.MinCount, E.MaxCount,
						E.GuaranteedWhenTag.IsValid() ? *E.GuaranteedWhenTag.ToString() : TEXT("<none>"));
				}
			}));
	}

	static APaldarkPalCharacter* FindPalByLabel(UWorld* World, const FString& NeedleLower)
	{
		if (!World) { return nullptr; }
		for (TActorIterator<APaldarkPalCharacter> It(World); It; ++It)
		{
			APaldarkPalCharacter* Pal = *It;
			if (!Pal) { continue; }
			const FString Name = Pal->GetName().ToLower();
			const FString Label = Pal->GetActorLabel().ToLower();
			if (Name.Contains(NeedleLower) || Label.Contains(NeedleLower))
			{
				return Pal;
			}
		}
		return nullptr;
	}

	static void ForceDrop(const TArray<FString>& Args, UWorld* World)
	{
		if (!World)
		{
			UE_LOG(LogPaldarkInventory, Warning, TEXT("Paldark.Loot.ForceDrop — no world."));
			return;
		}
		if (Args.Num() == 0)
		{
			UE_LOG(LogPaldarkInventory, Warning,
				TEXT("Paldark.Loot.ForceDrop — usage: Paldark.Loot.ForceDrop <PalLabelOrName>"));
			return;
		}

		const FString Needle = Args[0].ToLower();
		APaldarkPalCharacter* Pal = FindPalByLabel(World, Needle);
		if (!Pal)
		{
			UE_LOG(LogPaldarkInventory, Warning,
				TEXT("Paldark.Loot.ForceDrop — no Pal matching '%s' in world."), *Needle);
			return;
		}

		UPaldarkLootDropComponent* LootDrop = Pal->GetLootDropSlot();
		if (!LootDrop)
		{
			UE_LOG(LogPaldarkInventory, Warning,
				TEXT("Paldark.Loot.ForceDrop — Pal %s has no LootDropSlot."), *Pal->GetName());
			return;
		}
		if (!Pal->HasAuthority())
		{
			UE_LOG(LogPaldarkInventory, Warning,
				TEXT("Paldark.Loot.ForceDrop — Pal %s is not authoritative on this client."),
				*Pal->GetName());
			return;
		}

		const FTransform SpawnTransform = Pal->GetActorTransform();
		UE_LOG(LogPaldarkInventory, Log,
			TEXT("Paldark.Loot.ForceDrop — forcing drop on Pal %s at %s."),
			*Pal->GetName(), *SpawnTransform.GetLocation().ToCompactString());
		LootDrop->ForceDropAt(SpawnTransform);
	}

	static void PickupNearest(const TArray<FString>& /*Args*/, UWorld* World)
	{
		if (!World)
		{
			UE_LOG(LogPaldarkInventory, Warning, TEXT("Paldark.Loot.PickupNearest — no world."));
			return;
		}
		APlayerController* PC = GEngine ? GEngine->GetFirstLocalPlayerController(World) : nullptr;
		APaldarkCharacter* Player = PC ? Cast<APaldarkCharacter>(PC->GetPawn()) : nullptr;
		if (!Player)
		{
			UE_LOG(LogPaldarkInventory, Warning,
				TEXT("Paldark.Loot.PickupNearest — no local PaldarkCharacter pawn."));
			return;
		}
		if (!Player->HasAuthority())
		{
			UE_LOG(LogPaldarkInventory, Warning,
				TEXT("Paldark.Loot.PickupNearest — local pawn is not authoritative (run on server / standalone)."));
			return;
		}

		APaldarkLootBag* Bag = APaldarkLootBag::FindNearestBag(World, Player->GetActorLocation());
		if (!Bag)
		{
			UE_LOG(LogPaldarkInventory, Log,
				TEXT("Paldark.Loot.PickupNearest — no non-empty bag in world."));
			return;
		}
		UE_LOG(LogPaldarkInventory, Log,
			TEXT("Paldark.Loot.PickupNearest — picking up %s for player %s."),
			*Bag->GetName(), *Player->GetName());
		Bag->RequestPickup(Player);
	}
}

static FAutoConsoleCommandWithWorldAndArgs GPaldarkLootDumpTableCmd(
	TEXT("Paldark.Loot.DumpTable"),
	TEXT("(W33-34) Async-load a UPaldarkLootTable by RowName and dump resolved fields."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PaldarkLootConsole::DumpTable));

static FAutoConsoleCommandWithWorldAndArgs GPaldarkLootForceDropCmd(
	TEXT("Paldark.Loot.ForceDrop"),
	TEXT("(W33-34) Force-roll the configured LootTable on a Pal (bypass OnHealthZeroed). Args: <PalLabelOrName>."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PaldarkLootConsole::ForceDrop));

static FAutoConsoleCommandWithWorldAndArgs GPaldarkLootPickupNearestCmd(
	TEXT("Paldark.Loot.PickupNearest"),
	TEXT("(W33-34) Pick up the nearest non-empty loot bag into the local player's inventory."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PaldarkLootConsole::PickupNearest));
