// PALDARK W18-19 + W20-21 — Pal combat / hostile-Pal console commands.
//
// Six console commands registered as `FAutoConsoleCommandWithWorldAndArgs`:
//
//   Paldark.Pal.DumpThreat
//     Dumps every Pal's perception + combat + activity state to
//     `LogPaldarkPal`. Locates Pals by `TActorIterator<APaldarkPalCharacter>`
//     in the local world (server only — combat state isn't replicated).
//
//   Paldark.Pal.SpawnHostileDummy [DistanceCm=600]
//     Spawns one `APaldarkDummyTarget` in front of the local player at the
//     given distance. The dummy's constructor sets `TeamTag = Paldark.Team.Hostile`
//     so the player's nearest Pal aggros on it almost immediately (next
//     perception scan tick). Server / standalone only.
//
//   Paldark.Pal.ForceCombat [PalName] [TargetName]
//     Bypasses perception entirely — calls `ForceThreat` on the named Pal
//     with the named target. Useful for replaying a regression where the
//     scan logic was the only thing that prevented combat. When called
//     without args, picks the first Pal + closest hostile in range.
//
//   Paldark.Pal.SpawnDirehoundPack [Count=4] [DistanceCm=1200]
//     (W20-21) Spawns N Direhound hostiles in a ring `DistanceCm` cm in
//     front of the local player. Uses `APaldarkHostilePalSpawner::SpawnHostilePackAt`
//     so the spawned Pals are registered with the pack subsystem and inherit
//     the `Paldark.Pal.Pack.Direhound` pack tag. Requires a non-abstract
//     C++ class (APaldarkPalCharacter_Direhound) — designer-side BP subclass
//     `BP_Direhound_Default` recommended for mesh / anim, but the C++ class
//     spawns by itself for smoke testing.
//
//   Paldark.Pal.SpawnRazorbird [DistanceCm=1500]
//     (W20-21) Spawns one Razorbird hostile in front of the local player.
//     Lone Pal — no pack tag. The activity FSM defaults to Patrol when
//     waypoints exist or Idle when none.
//
//   Paldark.Pal.DumpPackState
//     (W20-21) Dumps `UPaldarkHostilePackSubsystem` registry + every
//     hostile Pal's species/pack/activity state to LogPaldarkPal.
//
// All commands live in their own TU (not the combat component) so we can
// include the perception / combat / activity / dummy / spawner headers
// without pulling them into every Pal compile.
//
// Roadmap reference: Documents/PALDARK/03-Roadmap_1_Year.md § Tuần 18–21.

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

#include "Combat/PaldarkDummyTarget.h"
#include "Pal/Components/PaldarkPalActivityComponent.h"
#include "Pal/Components/PaldarkPalCombatComponent.h"
#include "Pal/Components/PaldarkPalPatrolComponent.h"
#include "Pal/Components/PaldarkPalPerceptionComponent.h"
#include "Pal/PaldarkHostilePackSubsystem.h"
#include "Pal/PaldarkHostilePalSpawner.h"
#include "Pal/PaldarkPalCharacter.h"
#include "Pal/PaldarkPalCharacter_Direhound.h"
#include "Pal/PaldarkPalCharacter_Razorbird.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"

namespace PaldarkPalConsole
{
	static APlayerController* FindLocalPlayerController(UWorld* World)
	{
		if (World == nullptr)
		{
			return nullptr;
		}
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* PC = It->Get())
			{
				if (PC->IsLocalController())
				{
					return PC;
				}
			}
		}
		return nullptr;
	}

	static void DumpThreat(const TArray<FString>& /*Args*/, UWorld* World)
	{
		if (World == nullptr)
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("Paldark.Pal.DumpThreat — no world."));
			return;
		}
		int32 PalCount = 0;
		for (TActorIterator<APaldarkPalCharacter> It(World); It; ++It)
		{
			APaldarkPalCharacter* Pal = *It;
			if (Pal == nullptr)
			{
				continue;
			}
			++PalCount;
			UE_LOG(LogPaldarkPal, Log,
				TEXT("Paldark.Pal.DumpThreat — pal=%s team=%s"),
				*Pal->GetName(),
				*Pal->GetTeamTag().ToString());
			if (UPaldarkPalPerceptionComponent* Perception = Pal->GetPerceptionSlot())
			{
				Perception->DumpToLog();
			}
			if (UPaldarkPalCombatComponent* Combat = Pal->GetCombatSlot())
			{
				Combat->DumpToLog();
			}
			if (const UPaldarkPalActivityComponent* Activity = Pal->GetActivitySlot())
			{
				UE_LOG(LogPaldarkPal, Log,
					TEXT("Paldark.Pal.DumpThreat — pal=%s activity=%s"),
					*Pal->GetName(),
					*Activity->GetCurrentActivityTag().ToString());
			}
		}
		if (PalCount == 0)
		{
			UE_LOG(LogPaldarkPal, Log,
				TEXT("Paldark.Pal.DumpThreat — no APaldarkPalCharacter found in world."));
		}
	}

	static void SpawnHostileDummy(const TArray<FString>& Args, UWorld* World)
	{
		const float Distance = (Args.Num() > 0) ? FCString::Atof(*Args[0]) : 600.f;
		if (World == nullptr)
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("Paldark.Pal.SpawnHostileDummy — no world."));
			return;
		}
		APlayerController* PC = FindLocalPlayerController(World);
		if (PC == nullptr || PC->GetPawn() == nullptr)
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("Paldark.Pal.SpawnHostileDummy — no local player pawn."));
			return;
		}
		APawn* PlayerPawn = PC->GetPawn();
		const FVector  Forward  = PlayerPawn->GetActorForwardVector();
		const FVector  Location = PlayerPawn->GetActorLocation() + Forward * Distance;
		const FRotator Rotation = (-Forward).Rotation();

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		APaldarkDummyTarget* Dummy = World->SpawnActor<APaldarkDummyTarget>(
			APaldarkDummyTarget::StaticClass(), Location, Rotation, SpawnParams);
		if (Dummy == nullptr)
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("Paldark.Pal.SpawnHostileDummy — SpawnActor returned null."));
			return;
		}
		UE_LOG(LogPaldarkPal, Log,
			TEXT("Paldark.Pal.SpawnHostileDummy — spawned %s at %s (distance=%.1f from %s, team=%s)"),
			*Dummy->GetName(),
			*Location.ToCompactString(),
			Distance,
			*PlayerPawn->GetName(),
			*Dummy->GetTeamTag().ToString());
	}

	static APaldarkPalCharacter* FindPalByName(UWorld* World, const FString& Name)
	{
		for (TActorIterator<APaldarkPalCharacter> It(World); It; ++It)
		{
			APaldarkPalCharacter* Pal = *It;
			if (Pal != nullptr && Pal->GetName().Contains(Name, ESearchCase::IgnoreCase))
			{
				return Pal;
			}
		}
		return nullptr;
	}

	static AActor* FindActorByName(UWorld* World, const FString& Name)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Candidate = *It;
			if (Candidate != nullptr && Candidate->GetName().Contains(Name, ESearchCase::IgnoreCase))
			{
				return Candidate;
			}
		}
		return nullptr;
	}

	static void ForceCombat(const TArray<FString>& Args, UWorld* World)
	{
		if (World == nullptr)
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("Paldark.Pal.ForceCombat — no world."));
			return;
		}

		APaldarkPalCharacter* Pal    = nullptr;
		AActor*               Target = nullptr;

		if (Args.Num() >= 1)
		{
			Pal = FindPalByName(World, Args[0]);
		}
		else
		{
			// Pick the first Pal in the world.
			for (TActorIterator<APaldarkPalCharacter> It(World); It; ++It)
			{
				Pal = *It;
				break;
			}
		}
		if (Pal == nullptr)
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("Paldark.Pal.ForceCombat — no Pal found (args=%d)."),
				Args.Num());
			return;
		}

		if (Args.Num() >= 2)
		{
			Target = FindActorByName(World, Args[1]);
		}
		else
		{
			// Pick the closest hostile dummy. Walk dummies only so the
			// no-arg path is deterministic.
			float BestDistSq = TNumericLimits<float>::Max();
			const FVector PalLoc = Pal->GetActorLocation();
			for (TActorIterator<APaldarkDummyTarget> It(World); It; ++It)
			{
				APaldarkDummyTarget* Dummy = *It;
				if (Dummy == nullptr || !IsValid(Dummy))
				{
					continue;
				}
				const float DistSq = FVector::DistSquared(Dummy->GetActorLocation(), PalLoc);
				if (DistSq < BestDistSq)
				{
					BestDistSq = DistSq;
					Target = Dummy;
				}
			}
		}
		if (Target == nullptr)
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("Paldark.Pal.ForceCombat — no target found for pal=%s (args=%d)."),
				*Pal->GetName(), Args.Num());
			return;
		}

		UPaldarkPalPerceptionComponent* Perception = Pal->GetPerceptionSlot();
		if (Perception == nullptr)
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("Paldark.Pal.ForceCombat — pal=%s has no perception slot."),
				*Pal->GetName());
			return;
		}
		Perception->ForceThreat(Target);
		UE_LOG(LogPaldarkPal, Log,
			TEXT("Paldark.Pal.ForceCombat — pal=%s target=%s (forced)"),
			*Pal->GetName(),
			*Target->GetName());
	}

	// W20-21 — Helper: resolve "in front of the local player" anchor.
	static bool ResolveSpawnAnchor(UWorld* World, float Distance, FVector& OutLocation, FRotator& OutRotation)
	{
		if (World == nullptr)
		{
			return false;
		}
		APlayerController* PC = FindLocalPlayerController(World);
		if (PC == nullptr || PC->GetPawn() == nullptr)
		{
			return false;
		}
		APawn* PlayerPawn = PC->GetPawn();
		const FVector  Forward  = PlayerPawn->GetActorForwardVector();
		OutLocation = PlayerPawn->GetActorLocation() + Forward * Distance;
		OutRotation = (-Forward).Rotation();
		return true;
	}

	static void SpawnDirehoundPack(const TArray<FString>& Args, UWorld* World)
	{
		const int32 Count    = (Args.Num() > 0) ? FMath::Max(1, FCString::Atoi(*Args[0])) : 4;
		const float Distance = (Args.Num() > 1) ? FCString::Atof(*Args[1])               : 1200.f;

		FVector  Origin;
		FRotator Rotation;
		if (!ResolveSpawnAnchor(World, Distance, Origin, Rotation))
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("Paldark.Pal.SpawnDirehoundPack — no local player pawn."));
			return;
		}

		// Default waypoint ring around the spawn origin so the pack
		// immediately has a patrol path even without designer waypoints in
		// the level. Designers using the spawner actor in-level get their
		// authored waypoints — this helper is only for console smoke tests.
		TArray<FVector> RingWaypoints;
		const int32 WaypointCount = 4;
		const float WaypointRadius = 800.f;
		RingWaypoints.Reserve(WaypointCount);
		for (int32 I = 0; I < WaypointCount; ++I)
		{
			const float Angle = 2.f * PI * static_cast<float>(I) / static_cast<float>(WaypointCount);
			RingWaypoints.Emplace(
				Origin.X + FMath::Cos(Angle) * WaypointRadius,
				Origin.Y + FMath::Sin(Angle) * WaypointRadius,
				Origin.Z);
		}

		const int32 Spawned = APaldarkHostilePalSpawner::SpawnHostilePackAt(
			World,
			APaldarkPalCharacter_Direhound::StaticClass(),
			PaldarkGameplayTags::TAG_Paldark_Pal_Pack_Direhound,
			Origin,
			Count,
			/*SpreadRadius=*/300.f,
			RingWaypoints);

		UE_LOG(LogPaldarkPal, Log,
			TEXT("Paldark.Pal.SpawnDirehoundPack — origin=%s count=%d/%d waypoints=%d"),
			*Origin.ToCompactString(),
			Spawned,
			Count,
			RingWaypoints.Num());
	}

	static void SpawnRazorbird(const TArray<FString>& Args, UWorld* World)
	{
		const float Distance = (Args.Num() > 0) ? FCString::Atof(*Args[0]) : 1500.f;

		FVector  Origin;
		FRotator Rotation;
		if (!ResolveSpawnAnchor(World, Distance, Origin, Rotation))
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("Paldark.Pal.SpawnRazorbird — no local player pawn."));
			return;
		}

		// Razorbirds fly — bump the spawn Z so the bird appears in the air
		// rather than at the player's feet. 400 cm ~= 4 m above the player
		// gives a clear silhouette.
		Origin.Z += 400.f;

		// Empty waypoints — Razorbird Stalk activity drives toward the
		// player on perception; without waypoints Patrol falls back to Idle.
		const TArray<FVector> NoWaypoints;
		const int32 Spawned = APaldarkHostilePalSpawner::SpawnHostilePackAt(
			World,
			APaldarkPalCharacter_Razorbird::StaticClass(),
			/*PackTag=*/FGameplayTag(),
			Origin,
			/*Count=*/1,
			/*SpreadRadius=*/0.f,
			NoWaypoints);

		UE_LOG(LogPaldarkPal, Log,
			TEXT("Paldark.Pal.SpawnRazorbird — origin=%s spawned=%d"),
			*Origin.ToCompactString(),
			Spawned);
	}

	static void DumpPackState(const TArray<FString>& /*Args*/, UWorld* World)
	{
		if (World == nullptr)
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("Paldark.Pal.DumpPackState — no world."));
			return;
		}

		if (UPaldarkHostilePackSubsystem* PackSub = World->GetSubsystem<UPaldarkHostilePackSubsystem>())
		{
			PackSub->DumpToLog();
		}
		else
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("Paldark.Pal.DumpPackState — no UPaldarkHostilePackSubsystem on world."));
		}

		int32 HostileCount = 0;
		for (TActorIterator<APaldarkPalCharacter> It(World); It; ++It)
		{
			APaldarkPalCharacter* Pal = *It;
			if (Pal == nullptr || !Pal->GetSpeciesTag().IsValid())
			{
				continue;
			}
			++HostileCount;

			const UPaldarkPalActivityComponent* Activity = Pal->GetActivitySlot();
			const UPaldarkPalPatrolComponent*   Patrol   = Pal->GetPatrolSlot();
			UE_LOG(LogPaldarkPal, Log,
				TEXT("Paldark.Pal.DumpPackState — pal=%s species=%s pack=%s activity=%s waypoints=%d"),
				*Pal->GetName(),
				*Pal->GetSpeciesTag().ToString(),
				*Pal->GetPackTag().ToString(),
				Activity != nullptr ? *Activity->GetCurrentActivityTag().ToString() : TEXT("<no activity>"),
				Patrol != nullptr ? Patrol->Waypoints.Num() : -1);
			if (Patrol != nullptr)
			{
				Patrol->DumpToLog();
			}
		}
		if (HostileCount == 0)
		{
			UE_LOG(LogPaldarkPal, Log,
				TEXT("Paldark.Pal.DumpPackState — no hostile Pal (SpeciesTag set) found in world."));
		}
	}
}

static FAutoConsoleCommandWithWorldAndArgs GPaldarkPalDumpThreatCmd(
	TEXT("Paldark.Pal.DumpThreat"),
	TEXT("Dumps every Pal's perception + combat + activity state to LogPaldarkPal."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PaldarkPalConsole::DumpThreat));

static FAutoConsoleCommandWithWorldAndArgs GPaldarkPalSpawnHostileDummyCmd(
	TEXT("Paldark.Pal.SpawnHostileDummy"),
	TEXT("Spawn an APaldarkDummyTarget in front of the local player. Optional arg = distance in cm (default 600)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PaldarkPalConsole::SpawnHostileDummy));

static FAutoConsoleCommandWithWorldAndArgs GPaldarkPalForceCombatCmd(
	TEXT("Paldark.Pal.ForceCombat"),
	TEXT("Force a Pal into combat with a target. Args: [PalName] [TargetName]. With no args picks the first Pal + closest hostile."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PaldarkPalConsole::ForceCombat));

static FAutoConsoleCommandWithWorldAndArgs GPaldarkPalSpawnDirehoundPackCmd(
	TEXT("Paldark.Pal.SpawnDirehoundPack"),
	TEXT("(W20-21) Spawn a pack of Direhound hostiles. Args: [Count=4] [DistanceCm=1200]. Server / standalone."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PaldarkPalConsole::SpawnDirehoundPack));

static FAutoConsoleCommandWithWorldAndArgs GPaldarkPalSpawnRazorbirdCmd(
	TEXT("Paldark.Pal.SpawnRazorbird"),
	TEXT("(W20-21) Spawn one Razorbird hostile in the air in front of the local player. Args: [DistanceCm=1500]."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PaldarkPalConsole::SpawnRazorbird));

static FAutoConsoleCommandWithWorldAndArgs GPaldarkPalDumpPackStateCmd(
	TEXT("Paldark.Pal.DumpPackState"),
	TEXT("(W20-21) Dump pack subsystem registry + every hostile Pal's species/pack/activity state to LogPaldarkPal."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PaldarkPalConsole::DumpPackState));
