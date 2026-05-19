#include "Pal/PaldarkPalSpawnSubsystem.h"

#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

#include "PaldarkLogCategories.h"
#include "Anim/PaldarkAnimInstance.h"
#include "Pal/PaldarkPalCharacter.h"
#include "Pal/PaldarkPalDefinition.h"

bool UPaldarkPalSpawnSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// Inhibit on editor preview worlds where we'd never spawn server-side
	// Pals. The runtime check inside SpawnPalAsync / RequestPreWarmAsync
	// still guards against client-only worlds.
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}

	if (const UWorld* World = Cast<UWorld>(Outer))
	{
		const EWorldType::Type WT = World->WorldType;
		return WT == EWorldType::Game
			|| WT == EWorldType::PIE
			|| WT == EWorldType::GamePreview;
	}
	return false;
}

void UPaldarkPalSpawnSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkPalSpawnSubsystem::Initialize — world %s."),
		GetWorld() != nullptr ? *GetWorld()->GetName() : TEXT("<null>"));
}

void UPaldarkPalSpawnSubsystem::Deinitialize()
{
	ClearWarmCache();
	Super::Deinitialize();
}

void UPaldarkPalSpawnSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	const ENetMode NetMode = InWorld.GetNetMode();
	const bool bAuthoritative = NetMode != NM_Client;
	UE_LOG(LogPaldarkPal, Log,
		TEXT("UPaldarkPalSpawnSubsystem::OnWorldBeginPlay — NetMode=%d Authoritative=%d."),
		static_cast<int32>(NetMode), bAuthoritative ? 1 : 0);
}

UPaldarkPalSpawnSubsystem* UPaldarkPalSpawnSubsystem::Get(const UWorld* World)
{
	if (World == nullptr)
	{
		return nullptr;
	}
	return World->GetSubsystem<UPaldarkPalSpawnSubsystem>();
}

void UPaldarkPalSpawnSubsystem::RequestPreWarmAsync(
	const FPrimaryAssetId& DefId,
	FOnPalPreWarmComplete OnDone)
{
	if (!DefId.IsValid())
	{
		UE_LOG(LogPaldarkPal, Warning,
			TEXT("RequestPreWarmAsync — invalid DefId; firing OnDone(false)."));
		if (OnDone.IsBound())
		{
			OnDone.Execute(DefId, false);
		}
		return;
	}

	if (const UWorld* World = GetWorld())
	{
		if (World->GetNetMode() == NM_Client)
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("RequestPreWarmAsync %s — called on client; ignoring."),
				*DefId.ToString());
			if (OnDone.IsBound())
			{
				OnDone.Execute(DefId, false);
			}
			return;
		}
	}

	// Coalesce: if already in flight or resolved, queue the callback onto
	// the existing state instead of issuing a second async load.
	if (FPaldarkPalPreWarmState* Existing = WarmStates.Find(DefId))
	{
		if (Existing->bResolved)
		{
			// Already warm — invoke synchronously on the next tick so the
			// caller never sees a delegate fire from inside Request*.
			if (OnDone.IsBound() && GetWorld() != nullptr)
			{
				const bool bSuccess = Existing->Definition != nullptr;
				FOnPalPreWarmComplete Local = OnDone;
				GetWorld()->GetTimerManager().SetTimerForNextTick(
					[Local, DefId, bSuccess]()
					{
						Local.ExecuteIfBound(DefId, bSuccess);
					});
			}
			return;
		}

		if (OnDone.IsBound())
		{
			Existing->PendingCallbacks.Add(OnDone);
		}
		return;
	}

	FPaldarkPalPreWarmState NewState;
	if (OnDone.IsBound())
	{
		NewState.PendingCallbacks.Add(OnDone);
	}
	WarmStates.Add(DefId, MoveTemp(NewState));

	StartLoadInternal(DefId);
}

void UPaldarkPalSpawnSubsystem::StartLoadInternal(const FPrimaryAssetId& DefId)
{
	UAssetManager& Manager = UAssetManager::Get();

	// LoadPrimaryAsset with the "Spawn" bundle pulls every soft ref tagged
	// AssetBundles="Spawn" on UPaldarkPalDefinition. The returned handle is
	// kept alive by the subsystem so the loaded sub-objects don't drift to
	// GC after the callback fires.
	const TArray<FName> Bundles = { FName(TEXT("Spawn")) };
	const FStreamableDelegate OnLoaded = FStreamableDelegate::CreateUObject(
		this,
		&UPaldarkPalSpawnSubsystem::HandleLoadComplete,
		DefId);

	const TSharedPtr<FStreamableHandle> Handle =
		Manager.LoadPrimaryAsset(DefId, Bundles, OnLoaded);

	if (FPaldarkPalPreWarmState* State = WarmStates.Find(DefId))
	{
		State->Handle = Handle;
	}

	UE_LOG(LogPaldarkPal, Log,
		TEXT("StartLoadInternal — issued async load of %s (Spawn bundle, handle=%s)."),
		*DefId.ToString(),
		Handle.IsValid() ? TEXT("valid") : TEXT("null"));
}

void UPaldarkPalSpawnSubsystem::HandleLoadComplete(FPrimaryAssetId DefId)
{
	FPaldarkPalPreWarmState* State = WarmStates.Find(DefId);
	if (State == nullptr)
	{
		UE_LOG(LogPaldarkPal, Warning,
			TEXT("HandleLoadComplete %s — state missing (cache cleared mid-flight?). Dropping."),
			*DefId.ToString());
		return;
	}

	UAssetManager& Manager = UAssetManager::Get();
	const FSoftObjectPath Path = Manager.GetPrimaryAssetPath(DefId);
	UObject* LoadedObject = Path.IsNull() ? nullptr : Path.ResolveObject();
	const UPaldarkPalDefinition* Def = Cast<UPaldarkPalDefinition>(LoadedObject);

	State->Definition = Def;
	State->bResolved = true;

	const bool bSuccess = Def != nullptr;
	UE_LOG(LogPaldarkPal, Log,
		TEXT("HandleLoadComplete — DefId=%s Success=%d (resolved to %s)."),
		*DefId.ToString(),
		bSuccess ? 1 : 0,
		Def != nullptr ? *Def->GetName() : TEXT("<null>"));

	// Fire-and-clear pending callbacks. Copy out first so we don't iterate
	// over a TArray while a callback could re-enter RequestPreWarmAsync.
	TArray<FOnPalPreWarmComplete> Callbacks = MoveTemp(State->PendingCallbacks);
	State->PendingCallbacks.Reset();
	for (const FOnPalPreWarmComplete& CB : Callbacks)
	{
		CB.ExecuteIfBound(DefId, bSuccess);
	}
}

bool UPaldarkPalSpawnSubsystem::IsPreWarmed(const FPrimaryAssetId& DefId) const
{
	const FPaldarkPalPreWarmState* State = WarmStates.Find(DefId);
	return State != nullptr && State->bResolved && State->Definition != nullptr;
}

const UPaldarkPalDefinition* UPaldarkPalSpawnSubsystem::FindWarmedDefinition(const FPrimaryAssetId& DefId) const
{
	const FPaldarkPalPreWarmState* State = WarmStates.Find(DefId);
	return (State != nullptr && State->bResolved) ? State->Definition.Get() : nullptr;
}

TArray<FPrimaryAssetId> UPaldarkPalSpawnSubsystem::GetTrackedDefinitionIds() const
{
	TArray<FPrimaryAssetId> Out;
	WarmStates.GetKeys(Out);
	return Out;
}

void UPaldarkPalSpawnSubsystem::ClearWarmCache()
{
	UE_LOG(LogPaldarkPal, Log,
		TEXT("ClearWarmCache — releasing %d handle(s)."), WarmStates.Num());
	WarmStates.Reset();
}

void UPaldarkPalSpawnSubsystem::SpawnPalAsync(
	const FPrimaryAssetId& DefId,
	const FTransform& Transform,
	FOnPalSpawnedAsync OnSpawned)
{
	if (const UWorld* World = GetWorld())
	{
		if (World->GetNetMode() == NM_Client)
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("SpawnPalAsync %s — called on client; ignoring."),
				*DefId.ToString());
			if (OnSpawned.IsBound())
			{
				OnSpawned.Execute(DefId, nullptr);
			}
			return;
		}
	}

	// Fast path: definition is already warm — spawn synchronously after a
	// next-tick hop so the call signature stays "async always".
	if (const UPaldarkPalDefinition* Warm = FindWarmedDefinition(DefId))
	{
		APaldarkPalCharacter* Spawned = SpawnFromWarmedDefinition(*Warm, Transform);
		if (OnSpawned.IsBound())
		{
			OnSpawned.Execute(DefId, Spawned);
		}
		return;
	}

	// Slow path: pre-warm first, then spawn from the completion callback.
	// The lambda captures `Transform` by value + `OnSpawned` by value.
	FOnPalPreWarmComplete Chain = FOnPalPreWarmComplete::CreateWeakLambda(
		this,
		[this, Transform, OnSpawned](FPrimaryAssetId InDefId, bool bSuccess) mutable
		{
			if (!bSuccess)
			{
				if (OnSpawned.IsBound())
				{
					OnSpawned.Execute(InDefId, nullptr);
				}
				return;
			}
			const UPaldarkPalDefinition* Warm = FindWarmedDefinition(InDefId);
			APaldarkPalCharacter* Spawned = (Warm != nullptr)
				? SpawnFromWarmedDefinition(*Warm, Transform)
				: nullptr;
			if (OnSpawned.IsBound())
			{
				OnSpawned.Execute(InDefId, Spawned);
			}
		});
	RequestPreWarmAsync(DefId, Chain);
}

APaldarkPalCharacter* UPaldarkPalSpawnSubsystem::SpawnFromWarmedDefinition(
	const UPaldarkPalDefinition& Definition,
	const FTransform& Transform)
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	UClass* PalClass = Definition.PalClass.IsNull()
		? nullptr
		: Definition.PalClass.LoadSynchronous(); // warm — synchronous resolve hits cache.
	if (PalClass == nullptr || !PalClass->IsChildOf(APaldarkPalCharacter::StaticClass()))
	{
		UE_LOG(LogPaldarkPal, Warning,
			TEXT("SpawnFromWarmedDefinition %s — PalClass null or not a APaldarkPalCharacter (got %s)."),
			*Definition.GetName(),
			PalClass != nullptr ? *PalClass->GetName() : TEXT("nullptr"));
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Params.Owner = nullptr;

	APaldarkPalCharacter* Spawned = World->SpawnActor<APaldarkPalCharacter>(
		PalClass, Transform, Params);
	if (Spawned == nullptr)
	{
		UE_LOG(LogPaldarkPal, Warning,
			TEXT("SpawnFromWarmedDefinition %s — SpawnActor returned null."),
			*Definition.GetName());
		return nullptr;
	}

	// Apply mesh override before the anim instance class so the new skeleton
	// (if it changed) is in place when SetAnimInstanceClass re-instances.
	if (USkeletalMeshComponent* Mesh = Spawned->GetMesh())
	{
		if (!Definition.SkeletalMesh.IsNull())
		{
			if (USkeletalMesh* MeshAsset = Definition.SkeletalMesh.LoadSynchronous())
			{
				Mesh->SetSkeletalMesh(MeshAsset);
			}
		}

		if (!Definition.AnimInstanceClass.IsNull())
		{
			if (UClass* AnimClass = Definition.AnimInstanceClass.LoadSynchronous())
			{
				if (AnimClass->IsChildOf(UAnimInstance::StaticClass())
					&& Mesh->GetAnimClass() != AnimClass)
				{
					Mesh->SetAnimInstanceClass(AnimClass);
				}
			}
		}
	}

	// Movement tuning from the definition's eager stats.
	if (UCharacterMovementComponent* Move = Spawned->GetCharacterMovement())
	{
		Move->MaxWalkSpeed = Definition.MoveSpeedBase;
	}

	// Grant abilities server-side. ASC is owned by the Pal (W7-8) so we go
	// straight through the actor's GAS interface.
	if (UAbilitySystemComponent* ASC = Spawned->GetAbilitySystemComponent())
	{
		for (const TSoftClassPtr<UGameplayAbility>& SoftAbility : Definition.GrantedAbilities)
		{
			if (SoftAbility.IsNull())
			{
				continue;
			}
			UClass* AbilityClass = SoftAbility.LoadSynchronous();
			if (AbilityClass == nullptr)
			{
				continue;
			}
			if (UGameplayAbility* CDO = AbilityClass->GetDefaultObject<UGameplayAbility>())
			{
				const FGameplayAbilitySpec Spec(CDO, 1, INDEX_NONE, Spawned);
				ASC->GiveAbility(Spec);
			}
		}
	}

	UE_LOG(LogPaldarkPal, Log,
		TEXT("SpawnFromWarmedDefinition — spawned %s from %s at %s (mesh=%s anim=%s)."),
		*Spawned->GetName(),
		*Definition.GetName(),
		*Transform.GetLocation().ToCompactString(),
		Definition.SkeletalMesh.IsNull() ? TEXT("<bp default>") : *Definition.SkeletalMesh.ToString(),
		Definition.AnimInstanceClass.IsNull() ? TEXT("<bp default>") : *Definition.AnimInstanceClass.ToString());

	return Spawned;
}
