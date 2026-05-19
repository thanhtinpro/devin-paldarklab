// PALDARK W31-32 — POI + Map console commands.
//
// Three console commands registered as `FAutoConsoleCommandWithWorldAndArgs`:
//
//   Paldark.POI.Dump
//     Dumps every registered `APaldarkPointOfInterest` to LogPaldarkLab via
//     `UPaldarkPOISubsystem::DumpToLog`. Server / standalone only — clients
//     have no POI registry (server-authoritative).
//
//   Paldark.POI.HighlightTier <Low|Medium|High>
//     Draws debug spheres around every POI at the given danger tier for 5 s.
//     Useful for spotting "I added a MiniBoss POI but forgot to mark it
//     High" mistakes during Map 2 blockout playtest. Standalone / listen
//     server only (uses local `UDebugDrawService`).
//
//   Paldark.Map.LoadDefinition <PaldarkMapDefinition.RowName>
//     Async-loads a `UPaldarkMapDefinition` by primary asset row name and
//     logs the resolved MapTag / DisplayName / Recommended counts. Verifies
//     the AssetManager scan rule picked up a freshly-authored
//     `DA_MapDef_<X>` without needing to fire up the experience pipeline.
//
// Roadmap reference: Documents/PALDARK/03-Roadmap_1_Year.md § Tuần 31-32.

#include "DrawDebugHelpers.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "UObject/PrimaryAssetId.h"

#include "Map/PaldarkMapDefinition.h"
#include "Map/PaldarkPOISubsystem.h"
#include "Map/PaldarkPointOfInterest.h"
#include "PaldarkLogCategories.h"

namespace PaldarkMapConsole
{
	static void DumpPOIs(const TArray<FString>& /*Args*/, UWorld* World)
	{
		if (!World)
		{
			UE_LOG(LogPaldarkLab, Warning, TEXT("Paldark.POI.Dump — no world."));
			return;
		}
		UPaldarkPOISubsystem* Subsystem = UPaldarkPOISubsystem::Get(World);
		if (!Subsystem)
		{
			UE_LOG(LogPaldarkLab, Warning,
				TEXT("Paldark.POI.Dump — POI subsystem unavailable (client world?)."));
			return;
		}
		Subsystem->DumpToLog();
	}

	static void HighlightTier(const TArray<FString>& Args, UWorld* World)
	{
		if (!World)
		{
			UE_LOG(LogPaldarkLab, Warning, TEXT("Paldark.POI.HighlightTier — no world."));
			return;
		}
		UPaldarkPOISubsystem* Subsystem = UPaldarkPOISubsystem::Get(World);
		if (!Subsystem)
		{
			UE_LOG(LogPaldarkLab, Warning,
				TEXT("Paldark.POI.HighlightTier — POI subsystem unavailable (client world?)."));
			return;
		}

		EPaldarkPOIDangerTier Tier = EPaldarkPOIDangerTier::Low;
		FColor SphereColor = FColor::Green;
		if (Args.Num() > 0)
		{
			const FString& A = Args[0];
			if (A.Equals(TEXT("High"), ESearchCase::IgnoreCase))
			{
				Tier = EPaldarkPOIDangerTier::High;
				SphereColor = FColor::Red;
			}
			else if (A.Equals(TEXT("Medium"), ESearchCase::IgnoreCase))
			{
				Tier = EPaldarkPOIDangerTier::Medium;
				SphereColor = FColor::Yellow;
			}
			else if (A.Equals(TEXT("Low"), ESearchCase::IgnoreCase))
			{
				Tier = EPaldarkPOIDangerTier::Low;
				SphereColor = FColor::Green;
			}
			else
			{
				UE_LOG(LogPaldarkLab, Warning,
					TEXT("Paldark.POI.HighlightTier — unknown tier '%s'. Use Low|Medium|High."),
					*A);
				return;
			}
		}

		const TArray<APaldarkPointOfInterest*> POIs = Subsystem->GetPOIsByDangerTier(Tier);
		UE_LOG(LogPaldarkLab, Log,
			TEXT("Paldark.POI.HighlightTier — drawing %d POI(s) at tier %d."),
			POIs.Num(),
			static_cast<int32>(Tier));
		for (APaldarkPointOfInterest* POI : POIs)
		{
			if (!POI)
			{
				continue;
			}
			DrawDebugSphere(
				World,
				POI->GetActorLocation(),
				POI->RegistrationRadius,
				/*Segments=*/ 24,
				SphereColor,
				/*bPersistent=*/ false,
				/*LifeTime=*/ 5.f,
				/*DepthPriority=*/ 0,
				/*Thickness=*/ 4.f);
		}
	}

	static void LoadMapDefinition(const TArray<FString>& Args, UWorld* World)
	{
		if (!World)
		{
			UE_LOG(LogPaldarkLab, Warning,
				TEXT("Paldark.Map.LoadDefinition — no world."));
			return;
		}
		if (Args.Num() == 0)
		{
			UE_LOG(LogPaldarkLab, Warning,
				TEXT("Paldark.Map.LoadDefinition — usage: Paldark.Map.LoadDefinition <RowName>"));
			return;
		}
		UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
		if (!AssetManager)
		{
			UE_LOG(LogPaldarkLab, Warning,
				TEXT("Paldark.Map.LoadDefinition — AssetManager not initialised."));
			return;
		}

		const FName RowName(*Args[0]);
		const FPrimaryAssetId DefId(
			FPrimaryAssetType(TEXT("PaldarkMapDefinition")),
			RowName);
		UE_LOG(LogPaldarkLab, Log,
			TEXT("Paldark.Map.LoadDefinition — async-loading %s."),
			*DefId.ToString());

		AssetManager->LoadPrimaryAsset(DefId, TArray<FName>(), FStreamableDelegate::CreateLambda(
			[DefId]()
			{
				UAssetManager& Mgr = UAssetManager::Get();
				const UObject* Loaded = Mgr.GetPrimaryAssetObject(DefId);
				const UPaldarkMapDefinition* MapDef = Cast<UPaldarkMapDefinition>(Loaded);
				if (!MapDef)
				{
					UE_LOG(LogPaldarkLab, Warning,
						TEXT("Paldark.Map.LoadDefinition — failed to resolve %s. Check DA_MapDef_<X> exists under /Game/Paldark/Maps/."),
						*DefId.ToString());
					return;
				}
				UE_LOG(LogPaldarkLab, Log,
					TEXT("Paldark.Map.LoadDefinition — resolved %s: Tag=%s, DisplayName='%s', Players=%d, POIs=%d, SpawnPoints=%d, IntrinsicTags=%d."),
					*MapDef->GetDebugLabel(),
					*MapDef->MapTag.ToString(),
					*MapDef->DisplayName.ToString(),
					MapDef->RecommendedPlayerCount,
					MapDef->RecommendedPOICount,
					MapDef->RecommendedSpawnPointCount,
					MapDef->IntrinsicTagsToAssert.Num());
			}));
	}
}

static FAutoConsoleCommandWithWorldAndArgs GPaldarkPOIDumpCmd(
	TEXT("Paldark.POI.Dump"),
	TEXT("(W31-32) Dump every registered POI to LogPaldarkLab. Server / standalone only."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PaldarkMapConsole::DumpPOIs));

static FAutoConsoleCommandWithWorldAndArgs GPaldarkPOIHighlightTierCmd(
	TEXT("Paldark.POI.HighlightTier"),
	TEXT("(W31-32) Draw debug spheres around POIs at the given danger tier. Args: <Low|Medium|High>."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PaldarkMapConsole::HighlightTier));

static FAutoConsoleCommandWithWorldAndArgs GPaldarkMapLoadDefinitionCmd(
	TEXT("Paldark.Map.LoadDefinition"),
	TEXT("(W31-32) Async-load a UPaldarkMapDefinition by RowName and dump resolved fields."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PaldarkMapConsole::LoadMapDefinition));
