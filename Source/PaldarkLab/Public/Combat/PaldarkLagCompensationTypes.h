// PALDARK W16-17 — Server-side rewind data model.
//
// Three USTRUCT types that the lag compensation pipeline passes around. They
// live in their own header (instead of inside the component) so that the
// gameplay ability code, the score-request RPC payload, and any future
// telemetry layer can reference them without dragging the component header
// (with its tick + delegate surface) into a translation unit.
//
//  - FPaldarkBoxInformation     : one per-bone snapshot (location/rotation/extent).
//  - FPaldarkFramePackage       : one server-tick snapshot of every bone box on a
//                                  given character + the server timestamp.
//  - FPaldarkServerSideRewindResult : the verdict the lag comp returns to a hit
//                                  scoring request. HitConfirmed + headshot bit
//                                  + the rewound impact point that the damage
//                                  GE should use as the source-of-truth location.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PaldarkLagCompensationTypes.generated.h"

/**
 * Per-bone hitbox snapshot. The lag compensation pipeline interpolates between
 * two FPaldarkBoxInformation samples at the same bone key to reconstruct the
 * box's state at the client's reported HitTime.
 */
USTRUCT(BlueprintType)
struct PALDARKLAB_API FPaldarkBoxInformation
{
    GENERATED_BODY()

    /** World-space location of the box at the sample time. */
    UPROPERTY()
    FVector Location = FVector::ZeroVector;

    /** World-space rotation of the box at the sample time. */
    UPROPERTY()
    FRotator Rotation = FRotator::ZeroRotator;

    /** Half-extents of the box (cm). Pulled from UBoxComponent::GetUnscaledBoxExtent. */
    UPROPERTY()
    FVector BoxExtent = FVector::ZeroVector;
};

/**
 * One server-tick snapshot of every per-bone hitbox on a character. The lag
 * comp component holds a TDoubleLinkedList of these so the oldest entries can
 * be pruned cheaply once they fall outside MaxRecordTime.
 */
USTRUCT(BlueprintType)
struct PALDARKLAB_API FPaldarkFramePackage
{
    GENERATED_BODY()

    /** Server time at which this package was saved. Same clock domain as
     *  APaldarkPlayerController::GetServerTime(). Compared against the
     *  client-reported HitTime by the rewind algorithm. */
    UPROPERTY()
    float Time = 0.f;

    /** Map keyed by bone name → snapshot. We use the FName the designer typed
     *  into APaldarkCharacter::HitCollisionBoxes (which matches the skeletal
     *  socket name) so the rewind code can look up the corresponding live
     *  UBoxComponent on the same character by the same key. */
    UPROPERTY()
    TMap<FName, FPaldarkBoxInformation> HitBoxes;

    /** Owner-character pointer at sample time. Weak so a dead character
     *  doesn't keep memory alive longer than the history window — the rewind
     *  code re-resolves it and skips the package if the character was torn
     *  off the world. */
    UPROPERTY()
    TWeakObjectPtr<class APaldarkCharacter> Owner;
};

/**
 * Result of a score request. Returned synchronously from
 * UPaldarkLagCompensationComponent::ServerSideRewind. Damage application
 * happens *after* the caller inspects this struct so the calling ability can
 * choose to reject (anti-cheat heuristic), or proceed with HitConfirmed +
 * the rewound ImpactPoint as the source of truth.
 */
USTRUCT(BlueprintType)
struct PALDARKLAB_API FPaldarkServerSideRewindResult
{
    GENERATED_BODY()

    /** True if the rewound geometry contained the client's HitLocation
     *  within RewindTolerance cm. False if the trace missed (target moved
     *  out of trace cone, latency exceeded MaxRecordTime, or the client
     *  reported a location that never made sense). */
    UPROPERTY()
    bool bHitConfirmed = false;

    /** True if the confirmed hit landed on the box keyed under
     *  UPaldarkLagCompensationComponent::HeadBoneName. Mirrors what the
     *  W9-10 hitscan ability already does locally — moved here so the
     *  authoritative answer is computed against rewound geometry, not
     *  client-reported BoneName. */
    UPROPERTY()
    bool bHeadShot = false;

    /** Rewound impact point. When bHitConfirmed is true, this is the
     *  point the damage GE should reference (e.g. for HUD damage numbers
     *  + VFX). When bHitConfirmed is false, this is the client-reported
     *  HitLocation, untouched. */
    UPROPERTY()
    FVector ImpactPoint = FVector::ZeroVector;

    /** Server time that was rewound to (the interpolated sample time).
     *  Read by the debug command `Paldark.LagComp.DumpHistory` so a
     *  designer can compare against the live character position. */
    UPROPERTY()
    float RewindTime = 0.f;

    /** Categorisation tag — exactly one of Paldark.LagComp.HitConfirmed /
     *  Paldark.LagComp.HitRejected. Callers can build a tag container off
     *  this + the bHeadShot flag → Paldark.Hit.{Headshot|Bodyshot} to
     *  pass into the GE spec. */
    UPROPERTY()
    FGameplayTag ResultTag;
};
