// PALDARK W33-34 — Loot bag implementation.

#include "Loot/PaldarkLootBag.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Inventory/PaldarkItemDefinition.h"
#include "Net/UnrealNetwork.h"
#include "PaldarkLogCategories.h"
#include "Player/Components/PaldarkPlayerInventoryComponent.h"
#include "Player/PaldarkCharacter.h"
#include "TimerManager.h"

APaldarkLootBag::APaldarkLootBag()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->InitSphereRadius(50.f);
	PickupSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	PickupSphere->SetGenerateOverlapEvents(true);
	RootComponent = PickupSphere;

	BagMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BagMesh"));
	BagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BagMesh->SetupAttachment(RootComponent);
}

void APaldarkLootBag::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APaldarkLootBag, ReplicatedContents);
}

void APaldarkLootBag::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		if (PickupSphere)
		{
			PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &APaldarkLootBag::OnPickupSphereOverlap);
		}
		if (AutoCleanupSeconds > 0.f)
		{
			GetWorldTimerManager().SetTimer(AutoCleanupHandle, this,
				&APaldarkLootBag::HandleAutoCleanup, AutoCleanupSeconds, false);
		}
	}
}

void APaldarkLootBag::InitializeContents(const TArray<FPaldarkLootBagEntry>& InEntries)
{
	if (!HasAuthority())
	{
		UE_LOG(LogPaldarkInventory, Warning,
			TEXT("LootBag %s — InitializeContents called on non-authority. Ignored."),
			*GetName());
		return;
	}

	ReplicatedContents = InEntries;
	OnBagChanged.Broadcast(this);

	UE_LOG(LogPaldarkInventory, Log,
		TEXT("LootBag %s — populated with %d entries."),
		*GetName(), ReplicatedContents.Num());
}

void APaldarkLootBag::RequestPickup(APaldarkCharacter* Requester)
{
	if (!HasAuthority()) { return; }
	if (!Requester) { return; }
	if (ReplicatedContents.Num() == 0) { return; }

	UPaldarkPlayerInventoryComponent* Inventory =
		Requester->FindComponentByClass<UPaldarkPlayerInventoryComponent>();
	if (!Inventory)
	{
		UE_LOG(LogPaldarkInventory, Warning,
			TEXT("LootBag %s — requester %s has no inventory component, skipping pickup."),
			*GetName(), *Requester->GetName());
		return;
	}

	int32 TransferredEntries = 0;
	int32 TotalTransferred = 0;
	for (int32 EntryIdx = ReplicatedContents.Num() - 1; EntryIdx >= 0; --EntryIdx)
	{
		FPaldarkLootBagEntry& Entry = ReplicatedContents[EntryIdx];
		if (Entry.ItemDefinition.IsNull() || Entry.Count <= 0)
		{
			ReplicatedContents.RemoveAt(EntryIdx);
			continue;
		}

		const int32 Placed = Inventory->AddItem(Entry.ItemDefinition, Entry.Count);
		if (Placed > 0)
		{
			Entry.Count -= Placed;
			TransferredEntries++;
			TotalTransferred += Placed;
		}
		if (Entry.Count <= 0)
		{
			ReplicatedContents.RemoveAt(EntryIdx);
		}
	}

	OnBagChanged.Broadcast(this);

	UE_LOG(LogPaldarkInventory, Log,
		TEXT("LootBag %s — %s picked up %d items across %d entries (remaining %d)."),
		*GetName(), *Requester->GetName(),
		TotalTransferred, TransferredEntries, ReplicatedContents.Num());

	if (ReplicatedContents.Num() == 0)
	{
		GetWorldTimerManager().ClearTimer(AutoCleanupHandle);
		Destroy();
	}
}

void APaldarkLootBag::OnRep_ReplicatedContents()
{
	OnBagChanged.Broadcast(this);
}

void APaldarkLootBag::OnPickupSphereOverlap(UPrimitiveComponent* /*OverlappedComp*/,
	AActor* OtherActor, UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
	if (!HasAuthority()) { return; }
	if (APaldarkCharacter* Character = Cast<APaldarkCharacter>(OtherActor))
	{
		RequestPickup(Character);
	}
}

void APaldarkLootBag::HandleAutoCleanup()
{
	if (!HasAuthority()) { return; }
	UE_LOG(LogPaldarkInventory, Log,
		TEXT("LootBag %s — auto-cleanup fired (%.0fs elapsed, %d entries unclaimed)."),
		*GetName(), AutoCleanupSeconds, ReplicatedContents.Num());
	Destroy();
}

APaldarkLootBag* APaldarkLootBag::FindNearestBag(const UObject* WorldContext,
	const FVector& Location, float MaxRadius)
{
	if (!WorldContext) { return nullptr; }
	UWorld* World = WorldContext->GetWorld();
	if (!World) { return nullptr; }

	APaldarkLootBag* Best = nullptr;
	float BestDistSq = MaxRadius > 0.f ? (MaxRadius * MaxRadius) : TNumericLimits<float>::Max();
	for (TActorIterator<APaldarkLootBag> It(World); It; ++It)
	{
		APaldarkLootBag* Bag = *It;
		if (!Bag || Bag->IsEmpty()) { continue; }
		const float DistSq = FVector::DistSquared(Bag->GetActorLocation(), Location);
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Bag;
		}
	}
	return Best;
}
