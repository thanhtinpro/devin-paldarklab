// PALDARK W22-23 — Squad command component (per-player radial-wheel emitter).
//
// Per-player `UActorComponent` that owns:
//   - A soft-pointer to the `UPaldarkSquadCommandSet` data asset (designer-
//     authored, holds the radial-wheel rows).
//   - The Server RPC entry point `Server_IssueCommand(FGameplayTag, FVector)`
//     that client UI calls when the player picks a wheel slice.
//
// On the autonomous client, the radial-wheel widget calls
// `IssueCommand(Tag)` which validates against the command set, then fires
// the Server RPC; the server resolves the squad via
// `UPaldarkSquadSubsystem::BroadcastSquadCommand`.
//
// Why a separate component (not on the controller):
//   - Composition (P03) — the controller is already overloaded with input +
//     pawn-data + GAS-grant + time-sync responsibilities.
//   - Symmetric with the W22-23 ping subsystem (also a per-pawn entry-point
//     into a world subsystem).
//   - Cleaner unit-test surface — drop this component on any actor + give
//     it a fake squad subsystem and the IssueCommand path is testable.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PaldarkSquadCommandComponent.generated.h"

class APaldarkCharacter;
class UPaldarkSquadCommandSet;
class UPaldarkSquadSubsystem;

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent))
class PALDARKLAB_API UPaldarkSquadCommandComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkSquadCommandComponent();

	// Issue a radial-wheel command. Locally validates against the command
	// set (rejects unknown tags) and forwards to the Server RPC. Safe to
	// call from autonomous client + standalone + listen-server host.
	//
	// `OptionalLocation` is the most recent enemy-ping location for Attack
	// commands; pass `FVector::ZeroVector` for non-targeted commands
	// (Follow / Stay / Regroup). The squad subsystem stores the location
	// per squad so Pal activities can read it.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Squad")
	void IssueCommand(FGameplayTag InCommandTag, FVector OptionalLocation);

	// Read the active command set asset (designer-authored). Returns null
	// if the soft pointer never loaded.
	UFUNCTION(BlueprintPure, Category = "Paldark|Squad")
	const UPaldarkSquadCommandSet* GetCommandSet() const;

	// Designer override — soft pointer so the asset can be loaded on demand
	// (PrimaryDataAsset, registered via DefaultGame.ini).
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Paldark|Squad")
	TSoftObjectPtr<UPaldarkSquadCommandSet> CommandSetSoft;

protected:
	// Server RPC — receives the validated command from the autonomous client.
	// Re-validates against the command set (a malicious client may have
	// stripped the local check) and forwards to the squad subsystem.
	UFUNCTION(Server, Reliable)
	void Server_IssueCommand(FGameplayTag InCommandTag, FVector OptionalLocation);

	// Cached strong pointer resolved on demand from the soft pointer. Held
	// here so repeated `IssueCommand` calls don't re-load.
	UPROPERTY(Transient)
	mutable TObjectPtr<const UPaldarkSquadCommandSet> CachedCommandSet;

	APaldarkCharacter* GetOwnerCharacter() const;
	UPaldarkSquadSubsystem* GetSquadSubsystem() const;
};
