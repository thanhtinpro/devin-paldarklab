// PALDARK W22-23 — Pal "follow squad command" activity (P22).
//
// Listens to the world's `UPaldarkSquadSubsystem::OnSquadCommandIssued`
// delegate. When the issuing player belongs to the same squad as the Pal's
// owning player (the `APawn` referenced by the Pal's locomotion component's
// FollowedPawn), the Pal caches the command + location and runs the
// matching behaviour.
//
// Priority ladder (friendly companion):
//   Idle (10) -> Follow (20) -> FollowSquadCommand (22)
//   -> Investigate (30) -> Combat (40)
//
// Sitting between Follow and Investigate means:
//   - A Pal that is just leashing on the player can be pulled away by a
//     Follow command (re-form on a different squadmate) — useful behaviour.
//   - A Pal currently investigating a ping is NOT yanked by a Follow
//     command — investigation completes first (intentional: don't lose
//     an in-flight intent to a passive regroup command).
//   - A Pal in Combat (P40) ignores all squad commands — also intentional.
//
// Command behaviours (read from `UPaldarkSquadSubsystem::GetStandingCommand`):
//   - Squad.Command.Follow  — pull toward the issuer at full MaxWalkSpeed.
//   - Squad.Command.Stay    — clear movement input; sit at current location.
//   - Squad.Command.Regroup — like Follow but moves all squad Pals to issuer.
//   - Squad.Command.Attack  — orient toward `OptionalLocation` + close in.
//     Combat preempts naturally once perception fires.
//
// Designer add path:
//   Add `UPaldarkActivity_FollowSquadCommand` to the friendly Pal's
//   `CandidateActivities` list (BP subclass of `APaldarkPalCharacter`).
//   Hostile-Pal subclasses (Direhound / Razorbird) do NOT add this; their
//   CandidateActivities stays `[Idle, Patrol, Stalk, Combat]`.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Pal/Activities/PaldarkBaseActivity.h"
#include "PaldarkActivity_FollowSquadCommand.generated.h"

class APaldarkCharacter;
class UPaldarkSquadSubsystem;

UCLASS()
class PALDARKLAB_API UPaldarkActivity_FollowSquadCommand : public UPaldarkBaseActivity
{
	GENERATED_BODY()

public:
	UPaldarkActivity_FollowSquadCommand();

	virtual bool InitActivity(UPaldarkPalActivityComponent* InComponent) override;

	virtual bool CanRun_Implementation() const override;
	virtual bool ShouldContinue_Implementation() const override;
	virtual void EnterActivity_Implementation() override;
	virtual void TickActivity_Implementation(float DeltaSeconds) override;
	virtual void ExitActivity_Implementation() override;

protected:
	// Bound to `UPaldarkSquadSubsystem::OnSquadCommandIssued` in
	// `InitActivity`. Captures the most recent command so CanRun can
	// gate on "issuer is on the same squad as me + command not Stay
	// when already stationary".
	void HandleSquadCommandIssued(
		APaldarkCharacter* Issuer,
		FGameplayTag CommandTag,
		FVector OptionalLocation);

	// Resolve the squad subsystem cached on Init.
	UPaldarkSquadSubsystem* GetSquadSubsystem() const;

	// Returns the player this Pal currently follows (via the locomotion
	// component's `FollowedPawn`). Null for Pals with no owner — those
	// will never trigger CanRun.
	APaldarkCharacter* GetFollowedPlayer() const;

	// Designer knob — minimum delta (cm) between current Pal location and
	// the command target before the Pal bothers moving. Avoids jitter
	// when a Follow command lands on a Pal already on top of the issuer.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Activity")
	float ArrivalToleranceCm = 200.f;

	// Most recently received command + location. Reset on ExitActivity so
	// stale state doesn't carry over across a Combat->FollowSquadCommand
	// switch.
	UPROPERTY(Transient)
	FGameplayTag CurrentCommandTag;

	UPROPERTY(Transient)
	FVector CurrentCommandLocation = FVector::ZeroVector;

	// Delegate handle so ExitActivity can unbind cleanly.
	FDelegateHandle SquadCommandHandle;
};
