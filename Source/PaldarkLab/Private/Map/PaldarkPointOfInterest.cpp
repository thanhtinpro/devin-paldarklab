// PALDARK W31-32 — Designer-placeable POI actor.

#include "Map/PaldarkPointOfInterest.h"

#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameplayTagContainer.h"
#include "Map/PaldarkPOISubsystem.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"

APaldarkPointOfInterest::APaldarkPointOfInterest()
{
	PrimaryActorTick.bCanEverTick = false;

	DebugSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DebugSphere"));
	DebugSphere->InitSphereRadius(RegistrationRadius);
	DebugSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DebugSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	DebugSphere->SetHiddenInGame(true);
	DebugSphere->SetVisibility(true);
	RootComponent = DebugSphere;

	// POIs are placed by designers and never destroyed mid-game in W31-32
	// scope — no need to replicate (the subsystem is server-only anyway).
	bReplicates = false;
}

void APaldarkPointOfInterest::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		// Server-only registry — client POIs (none today, but future-proof)
		// silently skip register.
		return;
	}

	if (!POITag.IsValid())
	{
		UE_LOG(LogPaldarkLab, Warning,
			TEXT("[POI %s] BeginPlay — POITag is invalid; skipping register."),
			*GetName());
		return;
	}

	if (UPaldarkPOISubsystem* Subsystem = UPaldarkPOISubsystem::Get(GetWorld()))
	{
		Subsystem->RegisterPOI(this);
	}
	// If the subsystem isn't initialised yet (early BeginPlay race), the
	// subsystem's Initialize sweep will pick this POI up via
	// TActorIterator. No retry needed here.
}

void APaldarkPointOfInterest::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		if (UPaldarkPOISubsystem* Subsystem = UPaldarkPOISubsystem::Get(GetWorld()))
		{
			Subsystem->UnregisterPOI(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

FGameplayTag APaldarkPointOfInterest::GetDangerTierTag() const
{
	using namespace PaldarkGameplayTags;
	switch (DangerTier)
	{
		case EPaldarkPOIDangerTier::High:   return TAG_Paldark_POI_DangerTier_High;
		case EPaldarkPOIDangerTier::Medium: return TAG_Paldark_POI_DangerTier_Medium;
		case EPaldarkPOIDangerTier::Low:
		default:                            return TAG_Paldark_POI_DangerTier_Low;
	}
}

FString APaldarkPointOfInterest::GetDebugLabel() const
{
	if (!DisplayName.IsEmpty())
	{
		return DisplayName.ToString();
	}
	if (POITag.IsValid())
	{
		// `Paldark.POI.Type.RuinedTower` → "RuinedTower".
		const FString TagStr = POITag.ToString();
		int32 LastDot = INDEX_NONE;
		if (TagStr.FindLastChar(TEXT('.'), LastDot) && LastDot < TagStr.Len() - 1)
		{
			return TagStr.RightChop(LastDot + 1);
		}
		return TagStr;
	}
#if WITH_EDITOR
	return GetActorLabel();
#else
	return GetName();
#endif
}
