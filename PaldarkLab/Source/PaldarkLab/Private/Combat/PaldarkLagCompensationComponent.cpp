#include "Combat/PaldarkLagCompensationComponent.h"

#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"
#include "Player/PaldarkCharacter.h"

UPaldarkLagCompensationComponent::UPaldarkLagCompensationComponent()
{
    // Tick on the server only — the rewind buffer is server-authoritative.
    // SetActive(false) on the client to avoid burning frames on clients who
    // can't make use of the data. BeginPlay tightens this further by calling
    // SetComponentTickEnabled(false) when the owner isn't authority.
    PrimaryComponentTick.bCanEverTick          = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    PrimaryComponentTick.TickGroup             = TG_PostPhysics;

    // Component is intentionally non-replicated — the only replicated surface
    // is the ServerScoreRequest_Hitscan UFUNCTION(Server, Reliable) RPC, and
    // SetIsReplicatedByDefault must be true for the RPC to route via the
    // owner connection.
    SetIsReplicatedByDefault(true);
}

void UPaldarkLagCompensationComponent::BeginPlay()
{
    Super::BeginPlay();

    CachedCharacter = Cast<APaldarkCharacter>(GetOwner());
    if (CachedCharacter == nullptr)
    {
        UE_LOG(LogPaldarkNet, Warning,
            TEXT("UPaldarkLagCompensationComponent::BeginPlay — owner %s is not APaldarkCharacter; disabling tick."),
            *GetNameSafe(GetOwner()));
        SetComponentTickEnabled(false);
        return;
    }

    // Server-only — clients hold the component for RPC routing only, never
    // record frames.
    if (CachedCharacter->GetLocalRole() != ROLE_Authority)
    {
        SetComponentTickEnabled(false);
    }
}

void UPaldarkLagCompensationComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (CachedCharacter == nullptr || CachedCharacter->GetLocalRole() != ROLE_Authority)
    {
        return;
    }

    SaveFramePackage();

    // Prune the tail. Iterate from oldest forward — TDoubleLinkedList allows
    // O(1) removal from either end.
    const UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }
    const float Now = World->GetTimeSeconds();
    while (auto* TailNode = FrameHistory.GetTail())
    {
        const FPaldarkFramePackage& Oldest = TailNode->GetValue();
        if (Now - Oldest.Time < MaxRecordTime)
        {
            break;
        }
        FrameHistory.RemoveNode(TailNode);
    }
}

void UPaldarkLagCompensationComponent::SaveFramePackage()
{
    if (CachedCharacter == nullptr)
    {
        return;
    }

    FPaldarkFramePackage Package;
    Package.Time  = GetWorld()->GetTimeSeconds();
    Package.Owner = CachedCharacter;
    CacheBoxPositions(CachedCharacter, Package);

    // Insert at head — newest sample is always front.
    FrameHistory.AddHead(MoveTemp(Package));
}

void UPaldarkLagCompensationComponent::CacheBoxPositions(
    APaldarkCharacter*    Character,
    FPaldarkFramePackage& OutPackage)
{
    if (Character == nullptr)
    {
        return;
    }

    const TMap<FName, TObjectPtr<UBoxComponent>>& Boxes = Character->GetHitCollisionBoxes();
    OutPackage.HitBoxes.Reserve(Boxes.Num());

    for (const TPair<FName, TObjectPtr<UBoxComponent>>& Row : Boxes)
    {
        if (UBoxComponent* Box = Row.Value)
        {
            FPaldarkBoxInformation Info;
            Info.Location  = Box->GetComponentLocation();
            Info.Rotation  = Box->GetComponentRotation();
            Info.BoxExtent = Box->GetScaledBoxExtent();
            OutPackage.HitBoxes.Emplace(Row.Key, MoveTemp(Info));
        }
    }
}

void UPaldarkLagCompensationComponent::MoveBoxes(
    APaldarkCharacter*          Character,
    const FPaldarkFramePackage& Package)
{
    if (Character == nullptr)
    {
        return;
    }
    const TMap<FName, TObjectPtr<UBoxComponent>>& Boxes = Character->GetHitCollisionBoxes();
    for (const TPair<FName, FPaldarkBoxInformation>& Row : Package.HitBoxes)
    {
        const TObjectPtr<UBoxComponent>* Found = Boxes.Find(Row.Key);
        if (Found == nullptr)
        {
            continue;
        }
        if (UBoxComponent* Box = *Found)
        {
            Box->SetWorldLocation(Row.Value.Location);
            Box->SetWorldRotation(Row.Value.Rotation);
            Box->SetBoxExtent(Row.Value.BoxExtent);
        }
    }
}

void UPaldarkLagCompensationComponent::ResetBoxes(
    APaldarkCharacter*          Character,
    const FPaldarkFramePackage& Package)
{
    // ResetBoxes is structurally identical to MoveBoxes — kept as a separate
    // function so callers + log lines distinguish the rewind step from the
    // live-state restore step. If MoveBoxes ever grows side effects (e.g.
    // attaching to a parent), this twin function lets us avoid them on
    // restore.
    MoveBoxes(Character, Package);
}

void UPaldarkLagCompensationComponent::EnableCharacterMeshCollision(
    APaldarkCharacter*      Character,
    ECollisionEnabled::Type CollisionType)
{
    if (Character == nullptr)
    {
        return;
    }
    const TMap<FName, TObjectPtr<UBoxComponent>>& Boxes = Character->GetHitCollisionBoxes();
    for (const TPair<FName, TObjectPtr<UBoxComponent>>& Row : Boxes)
    {
        if (UBoxComponent* Box = Row.Value)
        {
            Box->SetCollisionEnabled(CollisionType);
        }
    }
}

FPaldarkFramePackage UPaldarkLagCompensationComponent::InterpBetweenFrames(
    const FPaldarkFramePackage& Older,
    const FPaldarkFramePackage& Younger,
    float                       HitTime)
{
    FPaldarkFramePackage Interp;
    Interp.Owner = Younger.Owner;
    Interp.Time  = HitTime;

    const float Distance = Younger.Time - Older.Time;
    // Guard against degenerate intervals (two samples on the same tick).
    const float Alpha = Distance > KINDA_SMALL_NUMBER
        ? FMath::Clamp((HitTime - Older.Time) / Distance, 0.f, 1.f)
        : 1.f;

    Interp.HitBoxes.Reserve(Younger.HitBoxes.Num());
    for (const TPair<FName, FPaldarkBoxInformation>& Row : Younger.HitBoxes)
    {
        const FPaldarkBoxInformation* OlderInfo = Older.HitBoxes.Find(Row.Key);
        if (OlderInfo == nullptr)
        {
            // Bone went missing between samples — fall back to younger.
            Interp.HitBoxes.Emplace(Row.Key, Row.Value);
            continue;
        }

        FPaldarkBoxInformation Info;
        Info.Location  = FMath::VInterpTo(OlderInfo->Location, Row.Value.Location, 1.f, Alpha);
        Info.Rotation  = FMath::RInterpTo(OlderInfo->Rotation, Row.Value.Rotation, 1.f, Alpha);
        // Box extent should not actually change frame-to-frame, but lerp
        // anyway so a designer who edits the extent mid-PIE doesn't blow up.
        Info.BoxExtent = FMath::VInterpTo(OlderInfo->BoxExtent, Row.Value.BoxExtent, 1.f, Alpha);
        Interp.HitBoxes.Emplace(Row.Key, MoveTemp(Info));
    }
    return Interp;
}

FPaldarkServerSideRewindResult UPaldarkLagCompensationComponent::ServerSideRewind(
    APaldarkCharacter* HitCharacter,
    const FVector&     TraceStart,
    const FVector&     HitLocation,
    float              HitTime)
{
    FPaldarkServerSideRewindResult Result;
    Result.ImpactPoint = HitLocation;
    Result.RewindTime  = HitTime;
    Result.ResultTag   = PaldarkGameplayTags::TAG_Paldark_LagComp_HitRejected;

    if (HitCharacter == nullptr)
    {
        UE_LOG(LogPaldarkNet, Verbose,
            TEXT("ServerSideRewind — HitCharacter null; rejecting."));
        return Result;
    }
    UPaldarkLagCompensationComponent* TargetLagComp = HitCharacter->FindComponentByClass<UPaldarkLagCompensationComponent>();
    if (TargetLagComp == nullptr)
    {
        UE_LOG(LogPaldarkNet, Verbose,
            TEXT("ServerSideRewind — target %s has no LagCompensation; rejecting."),
            *HitCharacter->GetName());
        return Result;
    }
    if (TargetLagComp->FrameHistory.GetHead() == nullptr || TargetLagComp->FrameHistory.GetTail() == nullptr)
    {
        UE_LOG(LogPaldarkNet, Verbose,
            TEXT("ServerSideRewind — target %s history empty; rejecting."),
            *HitCharacter->GetName());
        return Result;
    }

    const FPaldarkFramePackage& Newest = TargetLagComp->FrameHistory.GetHead()->GetValue();
    const FPaldarkFramePackage& Oldest = TargetLagComp->FrameHistory.GetTail()->GetValue();

    // Three cases:
    //   1. HitTime newer than newest → use newest sample (we just ticked).
    //   2. HitTime older than oldest → reject (latency exceeded MaxRecordTime).
    //   3. HitTime between two samples → InterpBetweenFrames.
    if (HitTime > Newest.Time)
    {
        return TargetLagComp->ConfirmHit(Newest, HitCharacter, TraceStart, HitLocation);
    }
    if (HitTime < Oldest.Time)
    {
        UE_LOG(LogPaldarkNet, Verbose,
            TEXT("ServerSideRewind — HitTime %.3f older than oldest sample %.3f; rejecting."),
            HitTime, Oldest.Time);
        return Result;
    }

    // Walk newest → oldest to find the pair that brackets HitTime. Cap the
    // walk at FrameHistory.Num() so a degenerate list (out-of-order times
    // from a clock skew) doesn't loop forever.
    auto* YoungerNode = TargetLagComp->FrameHistory.GetHead();
    auto* OlderNode   = YoungerNode != nullptr ? YoungerNode->GetNextNode() : nullptr;
    while (OlderNode != nullptr)
    {
        if (OlderNode->GetValue().Time <= HitTime)
        {
            const FPaldarkFramePackage Synthetic = InterpBetweenFrames(
                OlderNode->GetValue(),
                YoungerNode->GetValue(),
                HitTime);
            return TargetLagComp->ConfirmHit(Synthetic, HitCharacter, TraceStart, HitLocation);
        }
        YoungerNode = OlderNode;
        OlderNode   = OlderNode->GetNextNode();
    }

    // Shouldn't reach here — the bracketing should have succeeded since
    // we already passed the Oldest.Time check above. Defensive log.
    UE_LOG(LogPaldarkNet, Warning,
        TEXT("ServerSideRewind — fell off bracketing walk on %s (HitTime=%.3f). Rejecting."),
        *HitCharacter->GetName(), HitTime);
    return Result;
}

FPaldarkServerSideRewindResult UPaldarkLagCompensationComponent::ConfirmHit(
    const FPaldarkFramePackage& Package,
    APaldarkCharacter*          HitCharacter,
    const FVector&              TraceStart,
    const FVector&              HitLocation)
{
    FPaldarkServerSideRewindResult Result;
    Result.RewindTime  = Package.Time;
    Result.ImpactPoint = HitLocation;
    Result.ResultTag   = PaldarkGameplayTags::TAG_Paldark_LagComp_HitRejected;

    if (HitCharacter == nullptr)
    {
        return Result;
    }
    UWorld* World = HitCharacter->GetWorld();
    if (World == nullptr)
    {
        return Result;
    }

    // 1) Cache live box state so we can restore after the rewind trace.
    FPaldarkFramePackage LiveCache;
    LiveCache.Owner = HitCharacter;
    LiveCache.Time  = World->GetTimeSeconds();
    CacheBoxPositions(HitCharacter, LiveCache);

    // 2) Snap boxes to the synthetic rewound frame.
    MoveBoxes(HitCharacter, Package);

    // 3) Toggle collision on (NoCollision → QueryOnly) so the trace can hit
    //    the rewound boxes. The boxes start in NoCollision so they don't
    //    interfere with live combat / movement.
    EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryOnly);

    // 4) Head-first trace. Headshot zone is checked separately so we can
    //    return the headshot bit even when the trace also intersects another
    //    box later down the line.
    FCollisionQueryParams Params(SCENE_QUERY_STAT(PaldarkLagCompHead), /*bTraceComplex=*/false);
    Params.AddIgnoredActor(HitCharacter); // never hit our own root
    FHitResult HeadHit;
    const FVector TraceEnd = HitLocation + (HitLocation - TraceStart).GetSafeNormal() * 50.f; // small overshoot
    bool bHitHead = false;
    {
        const TMap<FName, TObjectPtr<UBoxComponent>>& Boxes = HitCharacter->GetHitCollisionBoxes();
        if (const TObjectPtr<UBoxComponent>* HeadBox = Boxes.Find(HeadBoneName))
        {
            // Temporarily isolate the head box: turn everything else off,
            // do the head trace, then turn the rest back on for the body
            // trace below.
            for (const TPair<FName, TObjectPtr<UBoxComponent>>& Row : Boxes)
            {
                if (Row.Value != *HeadBox && Row.Value != nullptr)
                {
                    Row.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                }
            }
            bHitHead = World->LineTraceSingleByChannel(HeadHit, TraceStart, TraceEnd, RewindTraceChannel, Params);
            // Restore the rest of the boxes for the body trace.
            for (const TPair<FName, TObjectPtr<UBoxComponent>>& Row : Boxes)
            {
                if (Row.Value != nullptr)
                {
                    Row.Value->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
                }
            }
        }
    }

    // 5) If head trace didn't connect, fall back to body trace.
    FHitResult BodyHit;
    bool bHitBody = false;
    if (!bHitHead)
    {
        bHitBody = World->LineTraceSingleByChannel(BodyHit, TraceStart, TraceEnd, RewindTraceChannel, Params);
    }

    const FHitResult& WinningHit = bHitHead ? HeadHit : BodyHit;
    const bool        bAnyHit    = bHitHead || bHitBody;

    if (bAnyHit && WinningHit.GetActor() == HitCharacter)
    {
        Result.bHitConfirmed = true;
        Result.bHeadShot     = bHitHead;
        Result.ImpactPoint   = WinningHit.ImpactPoint;
        Result.ResultTag     = PaldarkGameplayTags::TAG_Paldark_LagComp_HitConfirmed;
    }

    // 6) Restore live state regardless of outcome.
    EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);
    ResetBoxes(HitCharacter, LiveCache);

    UE_LOG(LogPaldarkNet, Log,
        TEXT("LagComp::ConfirmHit on %s — rewind=%.3f live=%.3f hit=%d head=%d impact=%s"),
        *HitCharacter->GetName(),
        Package.Time, LiveCache.Time,
        Result.bHitConfirmed ? 1 : 0,
        Result.bHeadShot ? 1 : 0,
        *Result.ImpactPoint.ToCompactString());

    return Result;
}

void UPaldarkLagCompensationComponent::ServerScoreRequest_Hitscan_Implementation(
    APaldarkCharacter* HitCharacter,
    const FVector&     TraceStart,
    const FVector&     HitLocation,
    float              HitTime)
{
    // This RPC is owned by the shooter. The verdict applies to HitCharacter,
    // so we route through that character's lag comp via ServerSideRewind.
    // The calling ability is responsible for applying damage based on the
    // result — this component never applies GE itself.
    const FPaldarkServerSideRewindResult Verdict = ServerSideRewind(
        HitCharacter, TraceStart, HitLocation, HitTime);

    UE_LOG(LogPaldarkNet, Log,
        TEXT("ServerScoreRequest_Hitscan — shooter=%s target=%s hit=%d head=%d"),
        *GetNameSafe(GetOwner()),
        *GetNameSafe(HitCharacter),
        Verdict.bHitConfirmed ? 1 : 0,
        Verdict.bHeadShot ? 1 : 0);
}

float UPaldarkLagCompensationComponent::GetNewestSampleTime() const
{
    if (const auto* Head = FrameHistory.GetHead())
    {
        return Head->GetValue().Time;
    }
    return 0.f;
}

float UPaldarkLagCompensationComponent::GetOldestSampleTime() const
{
    if (const auto* Tail = FrameHistory.GetTail())
    {
        return Tail->GetValue().Time;
    }
    return 0.f;
}

void UPaldarkLagCompensationComponent::DumpHistoryToLog() const
{
    UE_LOG(LogPaldarkNet, Log,
        TEXT("LagComp::DumpHistory owner=%s count=%d oldest=%.3f newest=%.3f head_bone=%s"),
        *GetNameSafe(GetOwner()),
        FrameHistory.Num(),
        GetOldestSampleTime(),
        GetNewestSampleTime(),
        *HeadBoneName.ToString());

    int32 Index = 0;
    for (auto* Node = FrameHistory.GetHead(); Node != nullptr; Node = Node->GetNextNode())
    {
        const FPaldarkFramePackage& Pkg = Node->GetValue();
        UE_LOG(LogPaldarkNet, Verbose,
            TEXT("LagComp::DumpHistory  [%03d] time=%.3f boxes=%d"),
            Index, Pkg.Time, Pkg.HitBoxes.Num());
        ++Index;
    }
}
