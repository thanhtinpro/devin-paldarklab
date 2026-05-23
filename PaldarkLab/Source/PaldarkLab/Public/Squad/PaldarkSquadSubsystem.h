// PALDARK W22-23 — Squad subsystem.
//
// `UWorldSubsystem` that maintains a per-SquadTag list of `APaldarkCharacter`
// players + a broadcast channel for radial-wheel commands. Mirrors the W20-21
// `UPaldarkHostilePackSubsystem` shape so reading both side-by-side is easy:
//   - Pack subsystem groups hostile Pals by their `PackTag` and broadcasts
//     threat sightings between packmates.
//   - Squad subsystem groups player characters by their `SquadTag` and
//     broadcasts radial-wheel commands between squadmates. Friendly Pals
//     listen on the subsystem's delegate and steer accordingly.
//
// W22-23 outcome (Documents/PALDARK/03-Roadmap_1_Year.md § Tuần 22-23):
//   "4-player team coordinate qua ping + radial command."
//
// Server-authoritative: registration happens in
// `UPaldarkSquadMembershipComponent::BeginPlay` on authority only; the
// broadcast delegate fires on the server, which then walks every squadmate
// and (for Pal companion listeners) calls into the existing activity FSM
// (no client RPCs added — Pal movement already replicates via ACharacter).
//
// What this is NOT (deferred):
//   - No VOIP. Vivox SDK integration lives in a follow-up week with proper
//     license + plugin install. The radial wheel + ping channel cover the
//     roadmap deliverable "coordinate qua ping + radial command".
//   - No persistence between matches. Squad composition is rebuilt every
//     PostLogin (same lifetime as the pack subsystem).
//   - No multi-squad arbitration (PvP red vs blue). Single squad per match
//     for W22-23; multi-squad lands W40+ with hub town.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "PaldarkSquadSubsystem.generated.h"

class APaldarkCharacter;

// Per-squad registry entry. USTRUCT wrap so TMap<FGameplayTag, TArray<...>>
// (UPROPERTY-incompatible) becomes TMap<FGameplayTag, FPaldarkSquadRoster>.
USTRUCT()
struct FPaldarkSquadRoster
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<APaldarkCharacter>> Members;
};

// Multicast delegate fired when any squad member issues a radial command.
// `Issuer` is the player who pressed the radial wheel (server-validated);
// `CommandTag` is one of `Paldark.Squad.Command.*`; `OptionalLocation` is
// the most recent enemy-ping location for Attack commands, or
// `FVector::ZeroVector` if no ping is in flight. Friendly Pals'
// `UPaldarkActivity_FollowSquadCommand` binds to this delegate on Init.
DECLARE_MULTICAST_DELEGATE_ThreeParams(
	FPaldarkOnSquadCommandIssued,
	APaldarkCharacter* /* Issuer */,
	FGameplayTag /* CommandTag */,
	FVector /* OptionalLocation */);

UCLASS()
class PALDARKLAB_API UPaldarkSquadSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPaldarkSquadSubsystem();

	// UWorldSubsystem — only initialise on hosts (server + standalone).
	// The squad registry is server-authoritative.
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Register a player character into a named squad. Called from the
	// W22-23 `UPaldarkSquadMembershipComponent::BeginPlay` on authority.
	// Idempotent — a double-register collapses to one entry. If
	// `InSquadTag` is invalid, the player registers under the default
	// `Paldark.Squad.Default` tag.
	void RegisterPlayerToSquad(APaldarkCharacter* InPlayer, const FGameplayTag& InSquadTag);

	// Remove a player from the registry. Called from `EndPlay` so logged-out
	// players don't keep ghost entries.
	void UnregisterPlayerFromSquad(APaldarkCharacter* InPlayer);

	// Issue a radial-wheel command to every squadmate. Called from
	// `UPaldarkSquadCommandComponent::Server_IssueCommand` on the server
	// after RPC validation. The subsystem broadcasts on
	// `OnSquadCommandIssued` so friendly-Pal `UPaldarkActivity_FollowSquadCommand`
	// listeners can react.
	void BroadcastSquadCommand(
		APaldarkCharacter* Issuer,
		const FGameplayTag& CommandTag,
		const FVector& OptionalLocation);

	// Debug accessor used by `Paldark.Squad.Dump`. Returns 0 for unknown
	// squad tags. Filters out stale weak refs.
	int32 GetSquadSize(const FGameplayTag& InSquadTag) const;

	// Read the squad tag a given player is currently registered under, or
	// an invalid tag if the player is not registered. Used by the command
	// component so `IssueCommand(Tag)` routes through the player's actual
	// squad tag without the caller having to know it.
	FGameplayTag GetSquadTagForPlayer(const APaldarkCharacter* InPlayer) const;

	// Read every squadmate of a given player (including the player itself).
	// Used by ping broadcast + future HUD code. Filters stale weak refs.
	void GetSquadMembersForPlayer(
		const APaldarkCharacter* InPlayer,
		TArray<APaldarkCharacter*>& OutMembers) const;

	// Dump squad registry state to LogPaldarkPlayer. Used by `Paldark.Squad.Dump`.
	void DumpToLog() const;

	// Delegate fired on the server every time a radial command is broadcast.
	// Friendly Pals' Activity_FollowSquadCommand binds in Init.
	FPaldarkOnSquadCommandIssued OnSquadCommandIssued;

	// Designer knob — most-recent command per squad. The Pal activity reads
	// this so a freshly-spawned Pal can adopt the standing command without
	// having missed the original broadcast. Cleared on Deinitialize.
	UFUNCTION(BlueprintPure, Category = "Paldark|Squad")
	FGameplayTag GetStandingCommand(const FGameplayTag& InSquadTag) const;

	UFUNCTION(BlueprintPure, Category = "Paldark|Squad")
	FVector GetStandingCommandLocation(const FGameplayTag& InSquadTag) const;

private:
	UPROPERTY(Transient)
	TMap<FGameplayTag, FPaldarkSquadRoster> SquadRegistry;

	// Per-squad standing command — overwritten on every BroadcastSquadCommand
	// call. New Pals query this on activity init so they fall into formation
	// with the rest of the pack.
	UPROPERTY(Transient)
	TMap<FGameplayTag, FGameplayTag> StandingCommandPerSquad;

	UPROPERTY(Transient)
	TMap<FGameplayTag, FVector> StandingCommandLocationPerSquad;
};
