// PALDARK W20-21 — Hostile-Pal pack subsystem.
//
// `UWorldSubsystem` that maintains a per-PackTag list of hostile Pals + a
// shared "nearest threat" broadcast channel. When any pack member's
// `UPaldarkPalCombatComponent::HandleThreatChanged` fires, it calls
// `BroadcastPackThreat(this, NewThreat)` on the subsystem; the subsystem
// then iterates every other pack member with the same PackTag, filters by
// `PackBroadcastRadius` (default 2000 cm) around the initiator, and calls
// `Perception->ForceThreat(NewThreat)` on each.
//
// W20-21 outcome (from Documents/PALDARK/03-Roadmap_1_Year.md § Tuần 20-21):
//   "Map test có 4 Direhound patrol, attack player khi gần."
//
// Why a subsystem (not per-Pal RPC):
//   - Subsystem is the single source of truth for "who is in which pack" so
//     the broadcast loop avoids the per-Pal `TActorIterator` cost that the
//     perception's W18-19 scan already pays once per Pal per scan tick.
//   - World subsystem lifetime matches the level's lifetime, so the pack
//     registry resets cleanly between PIE sessions / ServerTravel.
//   - Subsystem path is server-authoritative — clients see nothing here
//     since the perception itself is server-only (W18-19).
//
// Pack feedback loop guard:
//   `BroadcastPackThreat` first checks whether the receiving Pal already
//   considers `NewThreat` its current threat. If yes, skip — avoids the
//   "Pal A broadcasts to B, B broadcasts to A" infinite recursion when
//   both perceive the same actor.
//
// What this is NOT (deferred):
//   - No persistence between levels — pack composition is rebuilt every
//     PostLogin. The save-game integration lands W31-32.
//   - No "alpha / member" hierarchy. The roadmap deliverable is "4
//     Direhound patrol + attack" — a flat pack is enough. Alpha-driven
//     decision making lands W22-23 with the squad system.
//   - No flock cohesion (boids). Pack only shares threat sightings, not
//     formation. Cohesion lands W30+ with the Razorbird aerial polish.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "PaldarkHostilePackSubsystem.generated.h"

class AActor;
class APaldarkPalCharacter;

UCLASS()
class PALDARKLAB_API UPaldarkHostilePackSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPaldarkHostilePackSubsystem();

	// UWorldSubsystem — only initialise on hosts (server + standalone). The
	// pack registry is server-authoritative.
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Register a Pal into a named pack. Called from `BeginPlay` on every
	// hostile-Pal C++ subclass (Direhound / Razorbird). Idempotent — a
	// double-register collapses to one entry.
	void RegisterPalToPack(APaldarkPalCharacter* InPal, const FGameplayTag& InPackTag);

	// Remove a Pal from the registry. Called from `EndPlay` so dead Pals
	// don't keep ghost entries in the pack tag's array.
	void UnregisterPalFromPack(APaldarkPalCharacter* InPal);

	// Called by `UPaldarkPalCombatComponent::HandleThreatChanged` when a
	// Pal's perception locks onto a hostile. The subsystem iterates the
	// initiator's pack tag, filters by `PackBroadcastRadius` around the
	// initiator's location, and calls `Perception->ForceThreat(NewThreat)`
	// on each packmate that doesn't already have the same target.
	void BroadcastPackThreat(APaldarkPalCharacter* Initiator, AActor* NewThreat);

	// Debug accessor used by `Paldark.Pal.DumpPackState` + unit-test
	// scaffolding: count of currently-registered Pals under `InPackTag`.
	// Returns 0 for unknown pack tags. Filters out stale weak refs.
	int32 GetPackSize(const FGameplayTag& InPackTag) const;

	// Dump pack registry state to LogPaldarkPal. Used by `Paldark.Pal.DumpPackState`.
	void DumpToLog() const;

	// Designer knob — radius (cm) around the initiator within which
	// packmates receive the threat broadcast. Default 2000 cm (~20 m) keeps
	// the pack "tight" — a Direhound on the far side of the map doesn't
	// teleport into combat just because a packmate spotted the player.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Paldark|Pal|Pack", meta = (ClampMin = "0.0"))
	float PackBroadcastRadius = 2000.f;

private:
	// Per-PackTag list of registered Pals. Strong refs so a Pal being
	// destroyed mid-broadcast doesn't fault — `Unregister` removes the
	// entry on EndPlay, the IsValid check in Broadcast covers the gap.
	UPROPERTY(Transient)
	TMap<FGameplayTag, FPaldarkHostilePack> PackRegistry;
};

// Per-pack registry entry. Wrapped in a USTRUCT so `TMap<FGameplayTag, TArray<...>>`
// (which UPROPERTY doesn't support directly) becomes a `TMap<FGameplayTag, FPaldarkHostilePack>`.
USTRUCT()
struct FPaldarkHostilePack
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<APaldarkPalCharacter>> Members;
};
