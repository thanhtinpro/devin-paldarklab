// PALDARK W40-41 — Designer-placeable hub building base actor.

#include "Hub/PaldarkHubBuilding.h"

#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "Hub/PaldarkHubBuildingDefinition.h"
#include "Hub/PaldarkHubSubsystem.h"
#include "PaldarkLogCategories.h"
#include "Player/PaldarkPlayerController.h"

APaldarkHubBuilding::APaldarkHubBuilding()
{
	PrimaryActorTick.bCanEverTick = false;

	InteractSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractSphere"));
	InteractSphere->InitSphereRadius(400.0f);
	InteractSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InteractSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractSphere->SetHiddenInGame(true);
	InteractSphere->SetVisibility(true);
	RootComponent = InteractSphere;

	// Buildings are placed by designers, never destroyed mid-game in W40-41
	// scope. Hub subsystem is server-only, so clients don't need a registry.
	bReplicates = false;
}

void APaldarkHubBuilding::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		// Hub subsystem is server-only — client buildings silently skip
		// register. Designers should still place the actor server-side
		// (the default for placed actors in a single .umap).
		return;
	}

	if (!BuildingTag.IsValid())
	{
		UE_LOG(LogPaldarkLab, Warning,
			TEXT("[HubBuilding %s] BeginPlay — BuildingTag invalid (subclass must stamp in ctor); skipping register."),
			*GetName());
		return;
	}

	// Resize sphere from the optional definition asset. Synchronous load is
	// fine — the asset is small (just config knobs, no mesh / audio) and
	// the building is placed at design time so the cost is paid at hub map
	// load, not mid-fight. Designers can skip the asset entirely; the
	// CDO-default 400 cm radius applies.
	if (!BuildingDefinition.IsNull())
	{
		if (const UPaldarkHubBuildingDefinition* Def = BuildingDefinition.LoadSynchronous())
		{
			if (Def->InteractRadius > 0.0f && InteractSphere != nullptr)
			{
				InteractSphere->SetSphereRadius(Def->InteractRadius);
			}
		}
	}

	if (UPaldarkHubSubsystem* Subsystem = UPaldarkHubSubsystem::Get(GetWorld()))
	{
		Subsystem->RegisterHubBuilding(this);
	}
	// Subsystem-race guard: if the subsystem isn't initialised yet (early
	// BeginPlay), Initialize()'s sweep + OnWorldBeginPlay's retry sweep pick
	// up this actor via TActorIterator.
}

void APaldarkHubBuilding::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		if (UPaldarkHubSubsystem* Subsystem = UPaldarkHubSubsystem::Get(GetWorld()))
		{
			Subsystem->UnregisterHubBuilding(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void APaldarkHubBuilding::OnInteract(APaldarkPlayerController* Interactor)
{
	// Base path — subclasses override. Log only so designers placing a
	// vanilla `APaldarkHubBuilding` (no subclass) get visible feedback
	// rather than a silent no-op.
	UE_LOG(LogPaldarkLab, Log,
		TEXT("[HubBuilding %s] OnInteract — base path (no subclass override). Interactor=%s."),
		*GetDebugLabel(),
		Interactor != nullptr ? *Interactor->GetName() : TEXT("<null>"));
}

FString APaldarkHubBuilding::GetDebugLabel() const
{
	// 1) Definition's DisplayName (designer-authored).
	if (const UPaldarkHubBuildingDefinition* Def = BuildingDefinition.Get())
	{
		if (!Def->DisplayName.IsEmpty())
		{
			return Def->DisplayName.ToString();
		}
	}
	// 2) BuildingTag leaf (`Paldark.Hub.Building.PalStable` → "PalStable").
	if (BuildingTag.IsValid())
	{
		const FString TagStr = BuildingTag.ToString();
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
