// PALDARK W3-4 — Pal locomotion component (Pal follow player ~5m).
//
// Only Pal component in W3-4 with real behaviour. Ticks the owning Pal toward
// `FollowedPawn` whenever the planar distance exceeds `TargetFollowDistance`.
// The movement is fed through `ACharacter::AddMovementInput` so the existing
// `UCharacterMovementComponent` handles navmesh stepping, friction, falling.
//
// W3-4 deliberately does NOT use `UAIController` + Behavior Tree — that gets
// wired in W5-6 when the Activity FSM lands and W18-19 for combat / utility AI.
// For now the simple "steer toward target" loop is enough to satisfy the
// roadmap outcome: "Pal follow player với navmesh".
//
// W5-6 update — the Pal Activity FSM (UPaldarkPalActivityComponent) now owns
// the high-level state. Activities flip `bFollowEnabled` to pause the follow
// loop (Idle / Investigate) without losing the `FollowedPawn` cache, so a
// switch back to Follow resumes seamlessly. Activities can also drive their
// own movement input directly on the character (e.g. Investigate steers
// toward a ping location).
//
// Replication: state lives on the server's locomotion component; the
// follow tick only runs on authority so simulated clients see replicated
// position via ACharacter's default movement replication.
//
// W14-15 update: replicate `FollowedPawn` and `bFollowEnabled` (RepNotify)
// so simulated proxies + listen-server hosts can render debug HUD /
// per-Pal status without RPCing back to the server every frame. The
// follow tick itself still only runs on authority (the movement is
// already replicated by ACharacter).
//
//   FollowedPawn   — REPLICATED via OnRep_FollowedPawn. Designed for
//                    AlwaysRelevant Pals (until W27-28 significance work).
//   bFollowEnabled — REPLICATED via OnRep_FollowEnabled so HUD can read
//                    the activity state on clients.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PaldarkPalLocomotionComponent.generated.h"

class APawn;

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent))
class PALDARKLAB_API UPaldarkPalLocomotionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkPalLocomotionComponent();

	// UActorComponent — declare replicated properties.
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	// Set the pawn this Pal should follow. Call once on spawn; future weeks
	// will switch to a tag-keyed lookup (e.g. "follow team leader").
	void SetFollowedPawn(APawn* InFollowedPawn);

	// Returns the currently followed pawn (may be null before SetFollowedPawn
	// or after the leader despawns).
	APawn* GetFollowedPawn() const { return FollowedPawn.Get(); }

	// W5-6 — Activity FSM hook. When false, the follow tick skips the
	// `AddMovementInput` call but `FollowedPawn` is preserved, so flipping
	// back to true resumes the leash. Activities flip this on Enter/Exit;
	// gameplay code should NOT bypass the activity FSM.
	void SetFollowEnabled(bool bInEnabled);
	bool IsFollowEnabled() const { return bFollowEnabled; }

	// W5-6 — Read-only planar distance to the followed pawn (cm). Returns
	// `TNumericLimits<float>::Max()` when there is no followed pawn.
	// Used by activity selection predicates (Follow vs Idle hysteresis).
	float GetPlanarDistanceToFollowedPawn() const;

	virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Designer knobs — exposed so different Pal species can have different
	// follow distance / leash radius. Defaults are tuned for the 1-player /
	// 1-companion W3-4 sandbox.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Follow")
	float TargetFollowDistance = 500.0f; // cm — ~5m, roadmap target.

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Follow")
	float MaxLeashDistance = 3000.0f; // cm — beyond this we teleport the Pal in W18+.

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Follow")
	float StopDeadbandDistance = 80.0f; // cm — stop adding input when within this band.

protected:
	virtual void BeginPlay() override;

	// W14-15 — RepNotify hooks. Logged via LogPaldarkPal so a designer
	// watching a client can see the moment the leader handoff replicates.
	UFUNCTION()
	void OnRep_FollowedPawn();

	UFUNCTION()
	void OnRep_FollowEnabled();

private:
	// W14-15 — Now replicated. Hard pointer because a TWeakObjectPtr does
	// not survive the FArchive round-trip used by network replication; the
	// owning Pal's lifetime is tied to the server-spawned actor so a GC
	// strong ref here is safe.
	UPROPERTY(ReplicatedUsing = OnRep_FollowedPawn, Transient)
	TObjectPtr<APawn> FollowedPawn;

	// W5-6 — Activity FSM gate. true by default so existing W3-4 spawns
	// keep their follow behaviour even when no Activity FSM is configured.
	// W14-15 — Replicated so HUD / AnimBP on clients can react to the
	// activity-driven pause without server RPC chatter.
	UPROPERTY(ReplicatedUsing = OnRep_FollowEnabled, Transient)
	bool bFollowEnabled = true;
};
