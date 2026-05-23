#include "Pal/PaldarkPalSphere.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"

#include "Pal/Components/PaldarkPalTameComponent.h"
#include "Pal/PaldarkPalCharacter.h"
#include "Player/PaldarkCharacter.h"
#include "PaldarkLogCategories.h"

APaldarkPalSphere::APaldarkPalSphere()
{
	PrimaryActorTick.bCanEverTick = false;

	// Replicated so simulated proxies see the arc + trail.
	bReplicates = true;
	SetReplicateMovement(true);
	// 10 s server-side cap — a sphere that lands outside any collision
	// volume self-destructs so we don't leak actors.
	InitialLifeSpan = 10.f;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(15.0f);
	// Generic overlap so the projectile routes Pal-vs-sphere hits to the
	// HandleOverlap callback. Designer-side BP subclass can flip this to
	// block-all if they want a satisfying bounce-off-wall feel.
	CollisionComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	CollisionComponent->SetGenerateOverlapEvents(true);
	RootComponent = CollisionComponent;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(CollisionComponent);
	ProjectileMovement->InitialSpeed = 1500.f;
	ProjectileMovement->MaxSpeed = 2500.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 1.0f;
}

void APaldarkPalSphere::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APaldarkPalSphere, OwnerPlayer);
	DOREPLIFETIME(APaldarkPalSphere, PalSphereTierTag);
}

void APaldarkPalSphere::InitForThrow(APaldarkCharacter* InOwnerPlayer, FGameplayTag InPalSphereTierTag)
{
	// Authority-only init — the GA calls this between SpawnActorDeferred
	// and FinishSpawning so the replicated values are set before clients
	// hear about the actor.
	OwnerPlayer = InOwnerPlayer;
	PalSphereTierTag = InPalSphereTierTag;
}

void APaldarkPalSphere::BeginPlay()
{
	Super::BeginPlay();

	// Overlap binding is server-only — the tame resolution path is
	// authoritative. Simulated proxies still see the arc + trail but never
	// drive BeginTameAttempt.
	if (HasAuthority() && CollisionComponent)
	{
		CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &APaldarkPalSphere::HandleOverlap);
	}

	UE_LOG(LogPaldarkPal, Verbose,
		TEXT("APaldarkPalSphere::BeginPlay — sphere=%s authority=%d tier=%s owner=%s"),
		*GetName(), HasAuthority() ? 1 : 0,
		*PalSphereTierTag.ToString(),
		OwnerPlayer ? *OwnerPlayer->GetName() : TEXT("<none>"));
}

void APaldarkPalSphere::HandleOverlap(UPrimitiveComponent* /*OverlappedComponent*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	if (!HasAuthority() || bHasResolved || !OtherActor)
	{
		return;
	}

	// Ignore self-overlap with the throwing player on the first frame after
	// spawn — the sphere starts inside the player capsule and would
	// otherwise instantly self-resolve as a "no Pal" event.
	if (OtherActor == OwnerPlayer)
	{
		return;
	}

	APaldarkPalCharacter* HitPal = Cast<APaldarkPalCharacter>(OtherActor);
	if (!HitPal)
	{
		// Hit world geometry / a player — sphere is wasted; destroy without
		// invoking a tame attempt.
		bHasResolved = true;
		UE_LOG(LogPaldarkPal, Verbose,
			TEXT("APaldarkPalSphere::HandleOverlap — sphere=%s hit non-Pal actor=%s, destroying"),
			*GetName(), *OtherActor->GetName());
		Destroy();
		return;
	}

	bHasResolved = true;

	UPaldarkPalTameComponent* Tame =
		HitPal->FindComponentByClass<UPaldarkPalTameComponent>();
	if (Tame)
	{
		Tame->BeginTameAttempt(OwnerPlayer, PalSphereTierTag);
	}
	else
	{
		UE_LOG(LogPaldarkPal, Warning,
			TEXT("APaldarkPalSphere::HandleOverlap — sphere=%s hit Pal=%s but no UPaldarkPalTameComponent"),
			*GetName(), *HitPal->GetName());
	}

	Destroy();
}
