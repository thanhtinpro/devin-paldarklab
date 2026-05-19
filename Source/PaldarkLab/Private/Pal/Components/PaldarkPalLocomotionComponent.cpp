#include "Pal/Components/PaldarkPalLocomotionComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

#include "PaldarkLogCategories.h"

UPaldarkPalLocomotionComponent::UPaldarkPalLocomotionComponent()
{
	// This component drives Pal follow movement — it MUST tick.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup    = TG_PrePhysics;

	// W14-15 — Replicate FollowedPawn + bFollowEnabled. The follow movement
	// itself still piggy-backs on ACharacter's replicated CharacterMovement;
	// the only reason this component needs replication is so HUD / AnimBP
	// code on clients can answer "who am I following / am I paused".
	SetIsReplicatedByDefault(true);
}

void UPaldarkPalLocomotionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UPaldarkPalLocomotionComponent, FollowedPawn);
	DOREPLIFETIME(UPaldarkPalLocomotionComponent, bFollowEnabled);
}

void UPaldarkPalLocomotionComponent::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogPaldarkPal, Log, TEXT("UPaldarkPalLocomotionComponent::BeginPlay — owner=%s target=%.1f cm"),
		*GetNameSafe(GetOwner()), TargetFollowDistance);
}

void UPaldarkPalLocomotionComponent::SetFollowedPawn(APawn* InFollowedPawn)
{
	FollowedPawn = InFollowedPawn;
	UE_LOG(LogPaldarkPal, Log, TEXT("UPaldarkPalLocomotionComponent::SetFollowedPawn — pal=%s leader=%s"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(InFollowedPawn));
}

void UPaldarkPalLocomotionComponent::OnRep_FollowedPawn()
{
	UE_LOG(LogPaldarkPal, Verbose,
		TEXT("OnRep_FollowedPawn — pal=%s new-leader=%s"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(FollowedPawn));
}

void UPaldarkPalLocomotionComponent::OnRep_FollowEnabled()
{
	UE_LOG(LogPaldarkPal, Verbose,
		TEXT("OnRep_FollowEnabled — pal=%s enabled=%d"),
		*GetNameSafe(GetOwner()),
		bFollowEnabled ? 1 : 0);
}

void UPaldarkPalLocomotionComponent::SetFollowEnabled(bool bInEnabled)
{
	if (bFollowEnabled == bInEnabled)
	{
		return;
	}
	bFollowEnabled = bInEnabled;
	UE_LOG(LogPaldarkPal, Verbose, TEXT("UPaldarkPalLocomotionComponent::SetFollowEnabled — pal=%s enabled=%d"),
		*GetNameSafe(GetOwner()),
		bFollowEnabled ? 1 : 0);
}

float UPaldarkPalLocomotionComponent::GetPlanarDistanceToFollowedPawn() const
{
	const APawn* Leader = FollowedPawn;
	const AActor* OwnerActor = GetOwner();
	if (Leader == nullptr || OwnerActor == nullptr)
	{
		return TNumericLimits<float>::Max();
	}
	FVector Delta = Leader->GetActorLocation() - OwnerActor->GetActorLocation();
	Delta.Z = 0.0f;
	return Delta.Size();
}

void UPaldarkPalLocomotionComponent::TickComponent(
	float DeltaSeconds,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);

	ACharacter* PalChar = Cast<ACharacter>(GetOwner());
	if (PalChar == nullptr || !PalChar->HasAuthority())
	{
		// Follow loop runs on authority only; clients see replicated movement.
		return;
	}

	if (!bFollowEnabled)
	{
		// W5-6 — Activity FSM owns the high-level state; it has paused us.
		return;
	}

	APawn* Leader = FollowedPawn;
	if (Leader == nullptr)
	{
		return;
	}

	const FVector PalPos    = PalChar->GetActorLocation();
	const FVector LeaderPos = Leader->GetActorLocation();
	FVector Delta = LeaderPos - PalPos;
	Delta.Z = 0.0f; // planar follow — let CharacterMovement handle gravity.
	const float Distance = Delta.Size();

	if (Distance <= TargetFollowDistance + StopDeadbandDistance)
	{
		return;
	}

	// AddMovementInput direction. Magnitude is normalised; the character movement
	// component scales by MaxWalkSpeed. W18+ will replace this with a steering
	// behaviour that respects activity state (e.g. flank, hold position).
	const FVector InputDir = Delta.GetSafeNormal();
	PalChar->AddMovementInput(InputDir, 1.0f);
}
