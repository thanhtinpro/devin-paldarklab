// PALDARK W31-32 — POI subsystem implementation.

#include "Map/PaldarkPOISubsystem.h"

#include "EngineUtils.h"
#include "Engine/World.h"
#include "Map/PaldarkPointOfInterest.h"
#include "PaldarkLogCategories.h"

UPaldarkPOISubsystem::UPaldarkPOISubsystem() = default;

bool UPaldarkPOISubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}
	if (const UWorld* World = Cast<UWorld>(Outer))
	{
		// Only spin up the registry on hosts (server + standalone). Clients
		// don't need it — POI logic is server-authoritative.
		const ENetMode NetMode = World->GetNetMode();
		return NetMode == NM_DedicatedServer
			|| NetMode == NM_ListenServer
			|| NetMode == NM_Standalone;
	}
	return false;
}

void UPaldarkPOISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	RegisteredPOIs.Reset();
	// First sweep — picks up any POIs whose BeginPlay fired before our
	// Initialize. OnWorldBeginPlay runs a second sweep as belt-and-suspenders.
	SweepWorldForPOIs();
}

void UPaldarkPOISubsystem::Deinitialize()
{
	RegisteredPOIs.Reset();
	Super::Deinitialize();
}

void UPaldarkPOISubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// Second sweep after world BeginPlay completes — catches POIs streamed
	// in via sublevel during the same frame as Initialize.
	SweepWorldForPOIs();
}

UPaldarkPOISubsystem* UPaldarkPOISubsystem::Get(const UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	return World->GetSubsystem<UPaldarkPOISubsystem>();
}

void UPaldarkPOISubsystem::RegisterPOI(APaldarkPointOfInterest* InPOI)
{
	if (!InPOI || !InPOI->IsValidLowLevelFast())
	{
		return;
	}
	if (RegisteredPOIs.Contains(InPOI))
	{
		// Idempotent — Initialize sweep + POI BeginPlay can both register
		// the same actor. Second call is a no-op.
		return;
	}
	RegisteredPOIs.Add(InPOI);
	UE_LOG(LogPaldarkLab, Log,
		TEXT("[POISubsystem] Registered POI %s (Tag=%s, Tier=%s)"),
		*InPOI->GetDebugLabel(),
		*InPOI->POITag.ToString(),
		*InPOI->GetDangerTierTag().ToString());
}

void UPaldarkPOISubsystem::UnregisterPOI(APaldarkPointOfInterest* InPOI)
{
	if (!InPOI)
	{
		return;
	}
	const int32 Removed = RegisteredPOIs.Remove(InPOI);
	if (Removed > 0)
	{
		UE_LOG(LogPaldarkLab, Log,
			TEXT("[POISubsystem] Unregistered POI %s"),
			*InPOI->GetDebugLabel());
	}
}

TArray<APaldarkPointOfInterest*> UPaldarkPOISubsystem::GetAllPOIs() const
{
	TArray<APaldarkPointOfInterest*> Result;
	Result.Reserve(RegisteredPOIs.Num());
	for (const TObjectPtr<APaldarkPointOfInterest>& Ptr : RegisteredPOIs)
	{
		if (APaldarkPointOfInterest* POI = Ptr.Get())
		{
			Result.Add(POI);
		}
	}
	Result.Sort([](const APaldarkPointOfInterest& A, const APaldarkPointOfInterest& B)
	{
		return A.GetName() < B.GetName();
	});
	return Result;
}

TArray<APaldarkPointOfInterest*> UPaldarkPOISubsystem::GetPOIsByTag(const FGameplayTag& InTag) const
{
	TArray<APaldarkPointOfInterest*> Result;
	if (!InTag.IsValid())
	{
		return Result;
	}
	for (const TObjectPtr<APaldarkPointOfInterest>& Ptr : RegisteredPOIs)
	{
		if (APaldarkPointOfInterest* POI = Ptr.Get())
		{
			if (POI->POITag == InTag)
			{
				Result.Add(POI);
			}
		}
	}
	return Result;
}

TArray<APaldarkPointOfInterest*> UPaldarkPOISubsystem::GetPOIsByDangerTier(EPaldarkPOIDangerTier InTier) const
{
	TArray<APaldarkPointOfInterest*> Result;
	for (const TObjectPtr<APaldarkPointOfInterest>& Ptr : RegisteredPOIs)
	{
		if (APaldarkPointOfInterest* POI = Ptr.Get())
		{
			if (POI->DangerTier == InTier)
			{
				Result.Add(POI);
			}
		}
	}
	return Result;
}

APaldarkPointOfInterest* UPaldarkPOISubsystem::GetNearestPOI(const FVector& InLocation, float MaxRadius) const
{
	APaldarkPointOfInterest* Best = nullptr;
	float BestDistSq = MaxRadius > 0.f ? (MaxRadius * MaxRadius) : TNumericLimits<float>::Max();
	for (const TObjectPtr<APaldarkPointOfInterest>& Ptr : RegisteredPOIs)
	{
		APaldarkPointOfInterest* POI = Ptr.Get();
		if (!POI)
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(InLocation, POI->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = POI;
		}
	}
	return Best;
}

void UPaldarkPOISubsystem::DumpToLog() const
{
	UE_LOG(LogPaldarkLab, Log,
		TEXT("[POISubsystem] Dump — %d POI(s) registered."),
		RegisteredPOIs.Num());
	const TArray<APaldarkPointOfInterest*> Sorted = GetAllPOIs();
	for (int32 Idx = 0; Idx < Sorted.Num(); ++Idx)
	{
		APaldarkPointOfInterest* POI = Sorted[Idx];
		const FVector Loc = POI->GetActorLocation();
		UE_LOG(LogPaldarkLab, Log,
			TEXT("  [%d] %s  type=%s  tier=%s  loc=(%.0f,%.0f,%.0f)"),
			Idx,
			*POI->GetDebugLabel(),
			*POI->POITag.ToString(),
			*POI->GetDangerTierTag().ToString(),
			Loc.X, Loc.Y, Loc.Z);
	}
}

void UPaldarkPOISubsystem::SweepWorldForPOIs()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	int32 PickedUp = 0;
	for (TActorIterator<APaldarkPointOfInterest> It(World); It; ++It)
	{
		APaldarkPointOfInterest* POI = *It;
		if (!POI || RegisteredPOIs.Contains(POI))
		{
			continue;
		}
		if (!POI->POITag.IsValid())
		{
			continue;
		}
		RegisteredPOIs.Add(POI);
		++PickedUp;
	}
	if (PickedUp > 0)
	{
		UE_LOG(LogPaldarkLab, Log,
			TEXT("[POISubsystem] SweepWorldForPOIs — picked up %d POI(s) (total=%d)."),
			PickedUp,
			RegisteredPOIs.Num());
	}
}
