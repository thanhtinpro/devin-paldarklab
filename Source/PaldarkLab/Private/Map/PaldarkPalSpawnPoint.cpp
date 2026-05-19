// PALDARK W31-32 — Designer-placeable Pal spawn point.

#include "Map/PaldarkPalSpawnPoint.h"

#include "Components/BillboardComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/World.h"
#include "Pal/PaldarkHostilePalSpawner.h"
#include "Pal/PaldarkPalCharacter.h"
#include "Pal/PaldarkPalDefinition.h"
#include "Pal/PaldarkPalSpawnSubsystem.h"
#include "PaldarkLogCategories.h"

APaldarkPalSpawnPoint::APaldarkPalSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	Billboard->bIsScreenSizeScaled = true;
	RootComponent = Billboard;

	bReplicates = false;
}

void APaldarkPalSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		// Spawn point is server-authoritative — clients see the spawned
		// Pal through replication only.
		return;
	}

	if (!bSpawnOnBeginPlay)
	{
		UE_LOG(LogPaldarkLab, Verbose,
			TEXT("[SpawnPoint %s] bSpawnOnBeginPlay=false; skipping initial spawn."),
			*GetName());
		return;
	}

	if (!SpeciesTag.IsValid() && !PalDefinitionId.IsValid() && !FallbackPalClass)
	{
		UE_LOG(LogPaldarkLab, Warning,
			TEXT("[SpawnPoint %s] Neither SpeciesTag, PalDefinitionId, nor FallbackPalClass set; skipping spawn."),
			*GetName());
		return;
	}

	UPaldarkPalSpawnSubsystem* SpawnSubsystem = UPaldarkPalSpawnSubsystem::Get(GetWorld());

	bool bDispatched = false;
	if (SpawnSubsystem && (PalDefinitionId.IsValid() || SpeciesTag.IsValid()))
	{
		bDispatched = TryDefinitionPathSpawn(SpawnSubsystem);
	}

	if (!bDispatched)
	{
		bDispatched = TryClassPathSpawn();
	}

	if (!bDispatched)
	{
		UE_LOG(LogPaldarkLab, Warning,
			TEXT("[SpawnPoint %s] Failed to resolve a spawn path (Species=%s, DefId=%s, FallbackClass=%s)."),
			*GetName(),
			*SpeciesTag.ToString(),
			*PalDefinitionId.ToString(),
			FallbackPalClass ? *FallbackPalClass->GetName() : TEXT("None"));
	}
}

bool APaldarkPalSpawnPoint::TryDefinitionPathSpawn(UPaldarkPalSpawnSubsystem* Subsystem)
{
	if (!Subsystem)
	{
		return false;
	}

	FPrimaryAssetId DefId = PalDefinitionId;
	if (!DefId.IsValid() && SpeciesTag.IsValid())
	{
		// Lyra convention — primary asset row name mirrors the species
		// leaf (`Paldark.Pal.Species.Foxparks` → `DA_PalDef_Foxparks`).
		// We can't actually verify the asset exists without scanning
		// AssetRegistry; the subsystem's async load returns failure if
		// the row is missing, which we log there.
		const FString TagStr = SpeciesTag.ToString();
		int32 LastDot = INDEX_NONE;
		FString Leaf = TagStr;
		if (TagStr.FindLastChar(TEXT('.'), LastDot) && LastDot < TagStr.Len() - 1)
		{
			Leaf = TagStr.RightChop(LastDot + 1);
		}
		const FName RowName(*FString::Printf(TEXT("DA_PalDef_%s"), *Leaf));
		DefId = FPrimaryAssetId(FPrimaryAssetType(TEXT("PaldarkPalDefinition")), RowName);
	}

	if (!DefId.IsValid())
	{
		return false;
	}

	const FTransform Transform = GetActorTransform();
	TWeakObjectPtr<APaldarkPalSpawnPoint> WeakSelf(this);
	FOnPalSpawnedAsync OnDone;
	OnDone.BindLambda([WeakSelf, DefId](FPrimaryAssetId, APaldarkPalCharacter* Spawned)
	{
		if (APaldarkPalSpawnPoint* Self = WeakSelf.Get())
		{
			if (Spawned)
			{
				Self->RegisterSpawnedInstance(Spawned);
				UE_LOG(LogPaldarkLab, Log,
					TEXT("[SpawnPoint %s] Definition-path spawn succeeded (DefId=%s)."),
					*Self->GetName(),
					*DefId.ToString());
			}
			else
			{
				UE_LOG(LogPaldarkLab, Warning,
					TEXT("[SpawnPoint %s] Definition-path spawn returned null Pal (DefId=%s); designer-side asset likely missing."),
					*Self->GetName(),
					*DefId.ToString());
			}
		}
	});
	Subsystem->SpawnPalAsync(DefId, Transform, OnDone);
	bHasFiredSpawn = true;
	UE_LOG(LogPaldarkLab, Log,
		TEXT("[SpawnPoint %s] Dispatched definition-path spawn (DefId=%s)."),
		*GetName(),
		*DefId.ToString());
	return true;
}

bool APaldarkPalSpawnPoint::TryClassPathSpawn()
{
	if (!FallbackPalClass)
	{
		return false;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector Origin = GetActorLocation();
	const int32 Spawned = APaldarkHostilePalSpawner::SpawnHostilePackAt(
		World,
		FallbackPalClass,
		FGameplayTag(), // empty pack tag — single-spawn point, no pack stamp
		Origin,
		/*InCount=*/ 1,
		/*InSpreadRadius=*/ 0.f,
		/*InWaypoints=*/ TArray<FVector>());
	if (Spawned <= 0)
	{
		UE_LOG(LogPaldarkLab, Warning,
			TEXT("[SpawnPoint %s] Class-path spawn returned 0 (Class=%s)."),
			*GetName(),
			*FallbackPalClass->GetName());
		return false;
	}

	bHasFiredSpawn = true;
	UE_LOG(LogPaldarkLab, Log,
		TEXT("[SpawnPoint %s] Class-path spawn succeeded (Class=%s)."),
		*GetName(),
		*FallbackPalClass->GetName());
	// Class-path spawn doesn't surface the spawned Pal pointer back to us
	// (SpawnHostilePackAt returns count only), so the concurrent-instance
	// budget falls back to "one outstanding fan-out per BeginPlay" without
	// per-Pal EndPlay tracking. That's fine for the W31-32 blockout —
	// per-Pal budget tracking lands with the respawn timer in W35-36.
	return true;
}

void APaldarkPalSpawnPoint::RegisterSpawnedInstance(APaldarkPalCharacter* InSpawned)
{
	if (!InSpawned)
	{
		return;
	}
	ActiveInstances.Add(InSpawned);
	InSpawned->OnEndPlay.AddDynamic(this, &APaldarkPalSpawnPoint::OnSpawnedInstanceEndPlay);
}

void APaldarkPalSpawnPoint::OnSpawnedInstanceEndPlay(AActor* Actor, EEndPlayReason::Type /*Reason*/)
{
	APaldarkPalCharacter* AsPal = Cast<APaldarkPalCharacter>(Actor);
	if (!AsPal)
	{
		return;
	}
	ActiveInstances.RemoveAll([AsPal](const TWeakObjectPtr<APaldarkPalCharacter>& Ptr)
	{
		return Ptr.Get() == AsPal || !Ptr.IsValid();
	});
}
