// PALDARK W16-17 — Server-side rewind component.
//
// Outcome guarded by `Documents/PALDARK/03-Roadmap_1_Year.md` § Tuần 16–17:
//   "Hitscan accurate với 100ms ping artificial."
//
// What this component does (server-only behaviour):
//   - Every server frame, snapshot the owning character's per-bone hitbox
//     transforms into FPaldarkFramePackage and prepend it to FrameHistory.
//   - Prune the tail when the oldest entry falls outside MaxRecordTime
//     (default 4 s — generous so 250 ms ping + 100 ms artificial lag still
//     fits inside the window).
//   - On `ServerScoreRequest_Hitscan` RPC (client → server), interpolate
//     between the two FrameHistory samples bracketing the client-reported
//     HitTime, cache the live box transforms, snap the boxes back to the
//     rewound state, re-trace, capture verdict, restore the live state.
//   - Return FPaldarkServerSideRewindResult to the caller so the calling
//     ability (W18+ predicted HitscanFire) can apply damage with confidence.
//
// What this component intentionally does NOT do:
//   - Predict on the client (no client-side authority — the client only
//     reports HitTime; the server is the source of truth for hit verdict).
//   - Apply damage. The component returns a result struct; the calling
//     ability decides what GE to apply. Keeps the rewind logic
//     ability-agnostic (future melee / projectile abilities can reuse it).
//   - Validate the client's reported HitTime against anti-cheat heuristics
//     beyond MaxRecordTime — that's a W42-43 backend AWS week task.
//
// Design notes:
//   - Lives on APaldarkCharacter (player) via the LagCompSlot UPROPERTY.
//     The W3-4 LocomotionExtSlot pattern wasn't reused because lag comp
//     needs to tick on server while the existing slots are gameplay
//     behaviour holders.
//   - Frame history uses TDoubleLinkedList<FPaldarkFramePackage> so insert
//     at head + prune from tail is O(1). TArray would force a O(n) shift.
//   - Hitbox bones are TMap-keyed by FName so designer-renamed sockets
//     don't break rewind (lookup is by string, not by pointer identity).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/PaldarkLagCompensationTypes.h"
#include "Containers/List.h"
#include "PaldarkLagCompensationComponent.generated.h"

class APaldarkCharacter;
class UBoxComponent;

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent))
class PALDARKLAB_API UPaldarkLagCompensationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPaldarkLagCompensationComponent();

    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    /**
     * Top-level rewind entry point. Looks up FrameHistory for the bracketing
     * pair around HitTime, interpolates a synthetic frame, restores hitboxes
     * on HitCharacter to that frame, and traces TraceStart→HitLocation against
     * the rewound boxes. Restores live box state before returning.
     *
     * Server-only. Returns a zeroed result with bHitConfirmed=false and
     * ResultTag=HitRejected if called on a client.
     *
     * @param HitCharacter   Target the client claims to have hit. Must own a
     *                       UPaldarkLagCompensationComponent (this function
     *                       reads HitCharacter's component, not this one).
     * @param TraceStart     World-space origin of the trace (camera / muzzle).
     * @param HitLocation    World-space impact point the client reported.
     * @param HitTime        Server time at which the client claims the hit
     *                       happened. Sourced from
     *                       APaldarkPlayerController::GetServerTime() -
     *                       SingleTripTime on the firing client.
     */
    FPaldarkServerSideRewindResult ServerSideRewind(
        APaldarkCharacter* HitCharacter,
        const FVector&     TraceStart,
        const FVector&     HitLocation,
        float              HitTime);

    /**
     * Client → server RPC. The W16-17 predicted-fire path will call this
     * after detecting a hit locally. ServerSideRewind runs the verdict and
     * the calling ability applies the damage GE based on the result.
     *
     * This RPC lives on the lag comp component (not the ability) so that the
     * server-side reliable channel is bound to the shooter's
     * UPaldarkLagCompensationComponent — which is owned by APaldarkCharacter
     * and therefore goes through the OwnerConnection without spilling onto
     * the ASC channel.
     */
    UFUNCTION(Server, Reliable)
    void ServerScoreRequest_Hitscan(
        APaldarkCharacter* HitCharacter,
        const FVector&     TraceStart,
        const FVector&     HitLocation,
        float              HitTime);

    /** Number of frame packages currently retained. Read by
     *  `Paldark.LagComp.DumpHistory` debug command. */
    int32 GetHistoryCount() const { return FrameHistory.Num(); }

    /** Newest sample time (server-time) in FrameHistory, or 0 if empty. */
    float GetNewestSampleTime() const;

    /** Oldest sample time (server-time) in FrameHistory, or 0 if empty. */
    float GetOldestSampleTime() const;

    /** Dump the entire FrameHistory to LogPaldarkNet at Log verbosity.
     *  Used by `Paldark.LagComp.DumpHistory` to verify the buffer is
     *  populating correctly. Bounded by current history size — never spams
     *  more than ~120 lines (4 s × 30 Hz). */
    void DumpHistoryToLog() const;

    /** Returns the bone name the rewind logic treats as a headshot zone.
     *  Read by tests / debug commands to validate the lookup matches the
     *  HitscanFire ability's HeadBoneName. */
    const FName& GetHeadBoneName() const { return HeadBoneName; }

protected:
    virtual void BeginPlay() override;

    /** Snapshot the current per-bone hitbox transforms on the owner. Pushed
     *  to the front of FrameHistory each server tick. */
    void SaveFramePackage();

    /** Static helper: read every box in the owner's HitCollisionBoxes map
     *  and emit a FPaldarkFramePackage with the current server time. Lives
     *  as static because the score-request flow needs to cache *another*
     *  character's live state during rewind (not just self). */
    static void CacheBoxPositions(
        APaldarkCharacter*    Character,
        FPaldarkFramePackage& OutPackage);

    /** Snap the boxes on Character to the transforms encoded in Package. */
    static void MoveBoxes(
        APaldarkCharacter*          Character,
        const FPaldarkFramePackage& Package);

    /** Re-apply the cached live state from Package onto Character. Symmetric
     *  to MoveBoxes — the rewind flow calls Move(rewound) → trace →
     *  Move(live cache). */
    static void ResetBoxes(
        APaldarkCharacter*          Character,
        const FPaldarkFramePackage& Package);

    /** Toggle collision on all of Character's HitCollisionBoxes between
     *  NoCollision (default, live game) and QueryOnly + a specific channel
     *  (during the rewind trace). Restored on exit. */
    static void EnableCharacterMeshCollision(
        APaldarkCharacter*       Character,
        ECollisionEnabled::Type  CollisionType);

    /** Interpolate the box transforms in Older / Younger by Alpha (0..1
     *  resolved off HitTime). Returns a synthetic FPaldarkFramePackage that
     *  is *not* added to FrameHistory — just used for the trace. */
    static FPaldarkFramePackage InterpBetweenFrames(
        const FPaldarkFramePackage& Older,
        const FPaldarkFramePackage& Younger,
        float                       HitTime);

    /** Final hit confirmation step. Moves HitCharacter's boxes to the
     *  synthetic Package, traces TraceStart→HitLocation against ECC_Visibility
     *  on the head box first (for headshot priority), then against the full
     *  box set, restores live state, and returns the verdict. */
    FPaldarkServerSideRewindResult ConfirmHit(
        const FPaldarkFramePackage& Package,
        APaldarkCharacter*          HitCharacter,
        const FVector&              TraceStart,
        const FVector&              HitLocation);

    /** Maximum age of any entry in FrameHistory in seconds. Older frames
     *  are pruned each tick. 4 s default covers ~250 ms LAN ping +
     *  100 ms artificial lag + headroom. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|LagComp")
    float MaxRecordTime = 4.0f;

    /** Bone name used to flag a confirmed hit as a headshot. Matches the
     *  default UE5 mannequin head bone — designer can override per
     *  character class via UPROPERTY edit. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|LagComp")
    FName HeadBoneName = TEXT("head");

    /** Distance (cm) allowed between the rewound box intersection point
     *  and the client-reported HitLocation before we reject the hit. Wide
     *  default (5 cm) so floating-point + animation pose drift doesn't
     *  flag legitimate hits. Anti-cheat polish lowers this in W42-43. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|LagComp")
    float RewindTolerance = 5.0f;

    /** Trace channel used during rewind. Visibility = matches what the
     *  W9-10 HitscanFire ability uses on the live geometry. Future
     *  WeaponTrace channel (W42+) will swap this out. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|LagComp")
    TEnumAsByte<ECollisionChannel> RewindTraceChannel = ECC_Visibility;

private:
    /** Owner character cache. Resolved in BeginPlay; used by SaveFramePackage
     *  to know which hitboxes to snapshot. */
    UPROPERTY(Transient)
    TObjectPtr<APaldarkCharacter> CachedCharacter;

    /** Frame history. Newest at head, oldest at tail. Prune happens off the
     *  tail each tick. */
    TDoubleLinkedList<FPaldarkFramePackage> FrameHistory;
};
