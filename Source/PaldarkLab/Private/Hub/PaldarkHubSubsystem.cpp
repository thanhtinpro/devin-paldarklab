// PALDARK W40-41 — Hub town subsystem implementation.

#include "Hub/PaldarkHubSubsystem.h"

#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Framework/PaldarkGameModeBase.h"
#include "GameFramework/GameModeBase.h"
#include "GameplayTagsManager.h"
#include "HAL/IConsoleManager.h"
#include "Hub/PaldarkHubBuilding.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"

UPaldarkHubSubsystem::UPaldarkHubSubsystem() = default;

bool UPaldarkHubSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}
	if (const UWorld* World = Cast<UWorld>(Outer))
	{
		// Server-only registry. Same gating shape as W31-32 POI subsystem.
		const ENetMode NetMode = World->GetNetMode();
		return NetMode == NM_DedicatedServer
			|| NetMode == NM_ListenServer
			|| NetMode == NM_Standalone;
	}
	return false;
}

void UPaldarkHubSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	RegisteredBuildings.Reset();

	// First sweep — picks up buildings whose BeginPlay fired before our
	// Initialize. OnWorldBeginPlay runs a second sweep as belt-and-suspenders.
	SweepWorldForBuildings();

	IConsoleManager& Console = IConsoleManager::Get();
	CmdList = Console.RegisterConsoleCommand(
		TEXT("Paldark.Hub.List"),
		TEXT("List every registered hub building (Pal Stable / Marketplace / Briefing Room)."),
		FConsoleCommandWithArgsDelegate::CreateUObject(this, &UPaldarkHubSubsystem::HandleConsoleList),
		ECVF_Default);

	CmdQueueRaid = Console.RegisterConsoleCommand(
		TEXT("Paldark.Hub.QueueRaid"),
		TEXT("Queue a raid map for briefing-room handoff. Usage: Paldark.Hub.QueueRaid <Paldark.Map.*>"),
		FConsoleCommandWithArgsDelegate::CreateUObject(this, &UPaldarkHubSubsystem::HandleConsoleQueueRaid),
		ECVF_Default);

	CmdStatus = Console.RegisterConsoleCommand(
		TEXT("Paldark.Hub.Status"),
		TEXT("Dump hub subsystem state — registered count, pending raid tag, current GameMode MaxPlayers."),
		FConsoleCommandWithArgsDelegate::CreateUObject(this, &UPaldarkHubSubsystem::HandleConsoleStatus),
		ECVF_Default);
}

void UPaldarkHubSubsystem::Deinitialize()
{
	IConsoleManager& Console = IConsoleManager::Get();
	if (CmdList      != nullptr) { Console.UnregisterConsoleObject(CmdList);      CmdList      = nullptr; }
	if (CmdQueueRaid != nullptr) { Console.UnregisterConsoleObject(CmdQueueRaid); CmdQueueRaid = nullptr; }
	if (CmdStatus    != nullptr) { Console.UnregisterConsoleObject(CmdStatus);    CmdStatus    = nullptr; }

	RegisteredBuildings.Reset();
	PendingRaidMapTag = FGameplayTag();
	Super::Deinitialize();
}

void UPaldarkHubSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// Second sweep — catches buildings streamed in via sublevel during the
	// same frame as Initialize.
	SweepWorldForBuildings();
}

UPaldarkHubSubsystem* UPaldarkHubSubsystem::Get(const UWorld* World)
{
	if (World == nullptr)
	{
		return nullptr;
	}
	return World->GetSubsystem<UPaldarkHubSubsystem>();
}

void UPaldarkHubSubsystem::RegisterHubBuilding(APaldarkHubBuilding* InBuilding)
{
	if (InBuilding == nullptr || !InBuilding->IsValidLowLevelFast())
	{
		return;
	}
	if (RegisteredBuildings.Contains(InBuilding))
	{
		// Idempotent — Initialize sweep + building BeginPlay can both
		// register the same actor. Second call is a no-op.
		return;
	}
	RegisteredBuildings.Add(InBuilding);
	UE_LOG(LogPaldarkLab, Log,
		TEXT("[HubSubsystem] Registered building %s (Tag=%s)"),
		*InBuilding->GetDebugLabel(),
		*InBuilding->BuildingTag.ToString());
}

void UPaldarkHubSubsystem::UnregisterHubBuilding(APaldarkHubBuilding* InBuilding)
{
	if (InBuilding == nullptr)
	{
		return;
	}
	const int32 Removed = RegisteredBuildings.Remove(InBuilding);
	if (Removed > 0)
	{
		UE_LOG(LogPaldarkLab, Log,
			TEXT("[HubSubsystem] Unregistered building %s"),
			*InBuilding->GetDebugLabel());
	}
}

TArray<APaldarkHubBuilding*> UPaldarkHubSubsystem::GetAllBuildings() const
{
	TArray<APaldarkHubBuilding*> Result;
	Result.Reserve(RegisteredBuildings.Num());
	for (const TObjectPtr<APaldarkHubBuilding>& Ptr : RegisteredBuildings)
	{
		if (APaldarkHubBuilding* Building = Ptr.Get())
		{
			Result.Add(Building);
		}
	}
	Result.Sort([](const APaldarkHubBuilding& A, const APaldarkHubBuilding& B)
	{
		return A.GetName() < B.GetName();
	});
	return Result;
}

TArray<APaldarkHubBuilding*> UPaldarkHubSubsystem::GetBuildingsByTag(const FGameplayTag& InTag) const
{
	TArray<APaldarkHubBuilding*> Result;
	if (!InTag.IsValid())
	{
		return Result;
	}
	for (const TObjectPtr<APaldarkHubBuilding>& Ptr : RegisteredBuildings)
	{
		if (APaldarkHubBuilding* Building = Ptr.Get())
		{
			if (Building->BuildingTag == InTag)
			{
				Result.Add(Building);
			}
		}
	}
	return Result;
}

APaldarkHubBuilding* UPaldarkHubSubsystem::GetNearestBuilding(const FVector& InLocation, float MaxRadius) const
{
	APaldarkHubBuilding* Best = nullptr;
	float BestDistSq = MaxRadius > 0.0f ? (MaxRadius * MaxRadius) : TNumericLimits<float>::Max();
	for (const TObjectPtr<APaldarkHubBuilding>& Ptr : RegisteredBuildings)
	{
		APaldarkHubBuilding* Building = Ptr.Get();
		if (Building == nullptr)
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(InLocation, Building->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Building;
		}
	}
	return Best;
}

void UPaldarkHubSubsystem::SetPendingRaidMapTag(const FGameplayTag& InTag)
{
	const FGameplayTag OldTag = PendingRaidMapTag;
	PendingRaidMapTag = InTag;
	UE_LOG(LogPaldarkLab, Log,
		TEXT("[HubSubsystem] PendingRaidMapTag %s → %s."),
		OldTag.IsValid() ? *OldTag.ToString() : TEXT("<none>"),
		InTag.IsValid() ? *InTag.ToString() : TEXT("<none>"));
}

void UPaldarkHubSubsystem::DumpToLog() const
{
	UE_LOG(LogPaldarkLab, Log,
		TEXT("[HubSubsystem] Dump — %d building(s) registered, PendingRaidMapTag=%s."),
		RegisteredBuildings.Num(),
		PendingRaidMapTag.IsValid() ? *PendingRaidMapTag.ToString() : TEXT("<none>"));
	const TArray<APaldarkHubBuilding*> Sorted = GetAllBuildings();
	for (int32 Idx = 0; Idx < Sorted.Num(); ++Idx)
	{
		APaldarkHubBuilding* Building = Sorted[Idx];
		const FVector Loc = Building->GetActorLocation();
		UE_LOG(LogPaldarkLab, Log,
			TEXT("  [%d] %s  tag=%s  loc=(%.0f,%.0f,%.0f)"),
			Idx,
			*Building->GetDebugLabel(),
			*Building->BuildingTag.ToString(),
			Loc.X, Loc.Y, Loc.Z);
	}
}

void UPaldarkHubSubsystem::SweepWorldForBuildings()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}
	int32 PickedUp = 0;
	for (TActorIterator<APaldarkHubBuilding> It(World); It; ++It)
	{
		APaldarkHubBuilding* Building = *It;
		if (Building == nullptr || RegisteredBuildings.Contains(Building))
		{
			continue;
		}
		if (!Building->BuildingTag.IsValid())
		{
			continue;
		}
		RegisteredBuildings.Add(Building);
		++PickedUp;
	}
	if (PickedUp > 0)
	{
		UE_LOG(LogPaldarkLab, Log,
			TEXT("[HubSubsystem] SweepWorldForBuildings — picked up %d building(s) (total=%d)."),
			PickedUp,
			RegisteredBuildings.Num());
	}
}

void UPaldarkHubSubsystem::HandleConsoleList(const TArray<FString>& /*Args*/)
{
	DumpToLog();
	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(
			-1, 6.0f, FColor::Cyan,
			FString::Printf(TEXT("Paldark.Hub: %d building(s) registered."),
				RegisteredBuildings.Num()));
	}
}

void UPaldarkHubSubsystem::HandleConsoleQueueRaid(const TArray<FString>& Args)
{
	if (Args.Num() < 1)
	{
		UE_LOG(LogPaldarkLab, Warning,
			TEXT("Paldark.Hub.QueueRaid: missing <MapTag> argument. Usage: Paldark.Hub.QueueRaid <Paldark.Map.*>"));
		return;
	}
	const FString TagStr = Args[0];
	const FGameplayTag Tag = UGameplayTagsManager::Get().RequestGameplayTag(FName(*TagStr), /*ErrorIfNotFound=*/false);
	if (!Tag.IsValid())
	{
		UE_LOG(LogPaldarkLab, Warning,
			TEXT("Paldark.Hub.QueueRaid: tag %s not registered. Check Project Settings → GameplayTags."),
			*TagStr);
		return;
	}
	SetPendingRaidMapTag(Tag);
	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(
			-1, 4.0f, FColor::Yellow,
			FString::Printf(TEXT("Paldark.Hub: queued raid map %s"), *Tag.ToString()));
	}
}

void UPaldarkHubSubsystem::HandleConsoleStatus(const TArray<FString>& /*Args*/)
{
	int32 GameModeMaxPlayers = -1;
	UWorld* World = GetWorld();
	if (World != nullptr)
	{
		if (const APaldarkGameModeBase* GM = World->GetAuthGameMode<APaldarkGameModeBase>())
		{
			GameModeMaxPlayers = GM->GetMaxPlayers();
		}
	}
	UE_LOG(LogPaldarkLab, Log,
		TEXT("[Hub.Status] Buildings=%d PendingRaidMapTag=%s GameMode.MaxPlayers=%d"),
		RegisteredBuildings.Num(),
		PendingRaidMapTag.IsValid() ? *PendingRaidMapTag.ToString() : TEXT("<none>"),
		GameModeMaxPlayers);
	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(
			-1, 4.0f, FColor::Cyan,
			FString::Printf(TEXT("Paldark.Hub: %d bld, pending=%s, max=%d"),
				RegisteredBuildings.Num(),
				PendingRaidMapTag.IsValid() ? *PendingRaidMapTag.ToString() : TEXT("<none>"),
				GameModeMaxPlayers));
	}
}
