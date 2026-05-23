// PALDARK W22-23 — Pal "follow squad command" activity implementation.

#include "Pal/Activities/PaldarkActivity_FollowSquadCommand.h"

#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Pal/Components/PaldarkPalActivityComponent.h"
#include "Pal/Components/PaldarkPalLocomotionComponent.h"
#include "Pal/PaldarkPalCharacter.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"
#include "Player/PaldarkCharacter.h"
#include "Squad/PaldarkSquadSubsystem.h"

UPaldarkActivity_FollowSquadCommand::UPaldarkActivity_FollowSquadCommand()
{
	ActivityTag = PaldarkGameplayTags::TAG_Paldark_Pal_Activity_FollowSquadCommand;
	Priority = 22;
}

bool UPaldarkActivity_FollowSquadCommand::InitActivity(UPaldarkPalActivityComponent* InComponent)
{
	if (!Super::InitActivity(InComponent))
	{
		return false;
	}

	if (UPaldarkSquadSubsystem* SquadSub = GetSquadSubsystem())
	{
		SquadCommandHandle = SquadSub->OnSquadCommandIssued.AddUObject(
			this, &UPaldarkActivity_FollowSquadCommand::HandleSquadCommandIssued);
	}
	return true;
}

void UPaldarkActivity_FollowSquadCommand::HandleSquadCommandIssued(
	APaldarkCharacter* Issuer,
	FGameplayTag CommandTag,
	FVector OptionalLocation)
{
	if (Issuer == nullptr || !CommandTag.IsValid())
	{
		return;
	}

	const APaldarkCharacter* MyPlayer = GetFollowedPlayer();
	if (MyPlayer == nullptr)
	{
		return;
	}

	// Only react to commands from issuers on the same squad. The subsystem
	// already knows the squads — ask it.
	const UPaldarkSquadSubsystem* SquadSub = GetSquadSubsystem();
	if (SquadSub == nullptr)
	{
		return;
	}

	const FGameplayTag IssuerSquad = SquadSub->GetSquadTagForPlayer(Issuer);
	const FGameplayTag MySquad     = SquadSub->GetSquadTagForPlayer(MyPlayer);
	if (!IssuerSquad.IsValid() || IssuerSquad != MySquad)
	{
		return;
	}

	CurrentCommandTag      = CommandTag;
	CurrentCommandLocation = OptionalLocation;

	UE_LOG(LogPaldarkPal, Verbose,
		TEXT("[FollowSquadCommand] %s captured %s (loc=%s) from %s."),
		*GetNameSafe(GetPalOwner()),
		*CommandTag.ToString(),
		*OptionalLocation.ToCompactString(),
		*Issuer->GetName());
}

bool UPaldarkActivity_FollowSquadCommand::CanRun_Implementation() const
{
	// Only friendly Pals — hostile Pals don't have an owning player.
	if (GetFollowedPlayer() == nullptr)
	{
		return false;
	}
	return CurrentCommandTag.IsValid();
}

bool UPaldarkActivity_FollowSquadCommand::ShouldContinue_Implementation() const
{
	// Stay active until either:
	//   - The command tag clears (e.g. designer console-clears it).
	//   - The Pal has arrived (within ArrivalToleranceCm of the issuer /
	//     target location) — Follow / Regroup commands self-resolve.
	if (!CanRun_Implementation())
	{
		return false;
	}

	const APaldarkPalCharacter* Pal = GetPalOwner();
	if (Pal == nullptr)
	{
		return false;
	}

	if (CurrentCommandTag == PaldarkGameplayTags::TAG_Paldark_Squad_Command_Stay)
	{
		// Stay sticks indefinitely — Combat preempts naturally.
		return true;
	}

	if (CurrentCommandTag == PaldarkGameplayTags::TAG_Paldark_Squad_Command_Follow ||
		CurrentCommandTag == PaldarkGameplayTags::TAG_Paldark_Squad_Command_Regroup)
	{
		const APaldarkCharacter* Target = GetFollowedPlayer();
		if (Target == nullptr)
		{
			return false;
		}
		const float Distance = FVector::Dist2D(
			Pal->GetActorLocation(), Target->GetActorLocation());
		return Distance > ArrivalToleranceCm;
	}

	if (CurrentCommandTag == PaldarkGameplayTags::TAG_Paldark_Squad_Command_Attack)
	{
		// Attack sticks until Combat preempts or location is reached.
		const float Distance = FVector::Dist2D(
			Pal->GetActorLocation(), CurrentCommandLocation);
		return Distance > ArrivalToleranceCm;
	}

	return false;
}

void UPaldarkActivity_FollowSquadCommand::EnterActivity_Implementation()
{
	Super::EnterActivity_Implementation();

	UE_LOG(LogPaldarkPal, Log,
		TEXT("[FollowSquadCommand] %s entered — command=%s loc=%s."),
		*GetNameSafe(GetPalOwner()),
		*CurrentCommandTag.ToString(),
		*CurrentCommandLocation.ToCompactString());
}

void UPaldarkActivity_FollowSquadCommand::TickActivity_Implementation(float /*DeltaSeconds*/)
{
	APaldarkPalCharacter* Pal = GetPalOwner();
	if (Pal == nullptr)
	{
		return;
	}

	// Stay: clear any pending movement input so the Pal doesn't drift.
	if (CurrentCommandTag == PaldarkGameplayTags::TAG_Paldark_Squad_Command_Stay)
	{
		if (UCharacterMovementComponent* Movement = Pal->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
		}
		return;
	}

	// Resolve target location.
	FVector TargetLocation = FVector::ZeroVector;
	if (CurrentCommandTag == PaldarkGameplayTags::TAG_Paldark_Squad_Command_Follow ||
		CurrentCommandTag == PaldarkGameplayTags::TAG_Paldark_Squad_Command_Regroup)
	{
		const APaldarkCharacter* Target = GetFollowedPlayer();
		if (Target == nullptr)
		{
			return;
		}
		TargetLocation = Target->GetActorLocation();
	}
	else if (CurrentCommandTag == PaldarkGameplayTags::TAG_Paldark_Squad_Command_Attack)
	{
		TargetLocation = CurrentCommandLocation;
	}
	else
	{
		return;
	}

	const FVector PalLocation = Pal->GetActorLocation();
	const FVector Delta = TargetLocation - PalLocation;
	const FVector DeltaPlanar(Delta.X, Delta.Y, 0.f);
	if (DeltaPlanar.IsNearlyZero())
	{
		return;
	}

	Pal->AddMovementInput(DeltaPlanar.GetSafeNormal(), 1.0f);
}

void UPaldarkActivity_FollowSquadCommand::ExitActivity_Implementation()
{
	Super::ExitActivity_Implementation();

	UE_LOG(LogPaldarkPal, Log,
		TEXT("[FollowSquadCommand] %s exited (command=%s)."),
		*GetNameSafe(GetPalOwner()),
		*CurrentCommandTag.ToString());

	// Clear the cached command so a stale tag doesn't immediately re-trigger
	// on the next selection tick. The standing command on the subsystem
	// persists — designers can re-issue from the console to re-arm.
	CurrentCommandTag = FGameplayTag::EmptyTag;
	CurrentCommandLocation = FVector::ZeroVector;
}

UPaldarkSquadSubsystem* UPaldarkActivity_FollowSquadCommand::GetSquadSubsystem() const
{
	const UWorld* World = GetWorld();
	return World != nullptr ? World->GetSubsystem<UPaldarkSquadSubsystem>() : nullptr;
}

APaldarkCharacter* UPaldarkActivity_FollowSquadCommand::GetFollowedPlayer() const
{
	const APawn* Followed = GetFollowedPawn();
	return const_cast<APaldarkCharacter*>(Cast<APaldarkCharacter>(Followed));
}
