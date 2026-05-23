# PALDARK — W16-17 Build: Lag Compensation Server-Side Rewind

> **Roadmap reference:** `Documents/PALDARK/03-Roadmap_1_Year.md` § Tuần 16–17.
> **Status:** ✅ C++ infrastructure shipped — designer playtest pending (VM has no UE5.4 compiler).
> **Date:** 2026-05-16.

## 1. Goal

Port the [10] Udemy MP Shooter ("Blaster") server-side rewind pattern onto
PaldarkLab so the W18+ predicted Hitscan ability can resolve client-reported
hits against historical geometry. Outcome the roadmap pins:

> **Hitscan accurate với 100 ms ping artificial.**

W16-17 is a **networking infrastructure week** (like W14-15). No new
gameplay; the deliverable is the rewind machinery plus debug commands that
let a designer verify hitscan accuracy in PIE under `Net PktLag=100`.

## 2. What shipped

### Data model (`Public/Combat/PaldarkLagCompensationTypes.h`)

| Struct | Purpose |
|--------|---------|
| `FPaldarkBoxInformation`           | Single bone snapshot — `Location / Rotation / BoxExtent` |
| `FPaldarkFramePackage`             | Server-tick snapshot — `Time / TMap<FName, FPaldarkBoxInformation> / Owner` |
| `FPaldarkServerSideRewindResult`   | Verdict — `bHitConfirmed / bHeadShot / ImpactPoint / RewindTime / ResultTag` |

### Component (`Public+Private/Combat/PaldarkLagCompensationComponent.{h,cpp}`)

- `UPaldarkLagCompensationComponent : UActorComponent` — server-only tick.
- Owns `TDoubleLinkedList<FPaldarkFramePackage> FrameHistory` (newest at
  head, oldest at tail).
- Tick: `SaveFramePackage()` → push newest, prune oldest beyond
  `MaxRecordTime` (default 4 s).
- `ServerSideRewind(HitCharacter, TraceStart, HitLocation, HitTime)` →
  walks `FrameHistory` to find bracketing pair, calls `InterpBetweenFrames`,
  then `ConfirmHit` which moves the target's hitboxes to the synthetic
  frame, traces head-first then body, restores live state, and returns the
  verdict struct.
- `ServerScoreRequest_Hitscan` (Server, Reliable RPC) — entry point that
  W18+ predicted-fire ability will call.
- Public diagnostics: `GetHistoryCount`, `GetNewestSampleTime`,
  `GetOldestSampleTime`, `DumpHistoryToLog`.

### Per-bone hitboxes (`APaldarkCharacter`)

Constructor adds 16 `UBoxComponent` instances attached to default UE5
mannequin sockets: `head`, `pelvis`, `spine_02/03`, `upperarm_{l,r}`,
`lowerarm_{l,r}`, `hand_{l,r}`, `thigh_{l,r}`, `calf_{l,r}`, `foot_{l,r}`.
All default to `NoCollision`. The rewind flow flips them to `QueryOnly`
just for the trace window and back. Exposed via `GetHitCollisionBoxes()`
accessor — the lag comp component reads this on the target character
during rewind.

Also wired `LagCompSlot` as the 13th slot UPROPERTY so blueprint
subclasses can swap the component (matches the existing W3-4 slot
pattern).

### Time sync handshake (`APaldarkPlayerController`)

| Method | Domain | What |
|--------|--------|------|
| `ServerRequestServerTime(ClientTime)` | Client → Server (Reliable) | Client sends its local time. |
| `ClientReportServerTime(ClientTime, ServerReceiveTime)` | Server → Client (Reliable) | Server echoes its receive time so client computes RTT + delta. |
| `GetServerTime()` | Public                       | Server: `WorldTime`. Client: `WorldTime + ClientServerDelta`. |
| `GetSingleTripTime()` | Public                    | Half RTT estimate. Shooter subtracts this to back-date HitTime. |

`PlayerTick` re-issues `ServerRequestServerTime` every `TimeSyncFrequency`
seconds (default 5 s) so clock drift over a long match stays
sub-millisecond. `ReceivedPlayer` seeds the handshake on join so the
first input has a working clock.

### Gameplay tags (`PaldarkLabCore`)

Three new tags registered:

- `Paldark.LagComp.HitConfirmed` — `FPaldarkServerSideRewindResult.ResultTag` on a confirmed hit.
- `Paldark.LagComp.HitRejected`  — `FPaldarkServerSideRewindResult.ResultTag` on rejection.
- `Paldark.Hit.Bodyshot`         — companion to existing `Paldark.Hit.Headshot` so damage exec can distinguish "no hit" from "body hit".

### Console commands (3)

| Console command                              | What it does                                                                                                                                       |
|----------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| `Paldark.LagComp.DumpHistory`                | Dump local pawn's `FrameHistory` (count + oldest + newest sample time) + `GetServerTime()` + `GetSingleTripTime()` to `LogPaldarkNet`.                |
| `Paldark.LagComp.SimulateHit [BackdateMs]`   | Camera-trace from local pawn → call `ServerScoreRequest_Hitscan` with `HitTime = GetServerTime() - BackdateMs/1000`. Default 100 ms back-date.       |
| `Paldark.LagComp.ArtificialPing [Ms]`        | Wrapper around `Net PktLag=N`. Default 100 ms; `0` clears.                                                                                          |

### Validator (`scripts/ci/validate_paldarklab.py`)

Added `check_lag_compensation_shape()` (~170 LOC) — validates:

- 3 USTRUCT types in `PaldarkLagCompensationTypes.h`.
- Lag comp component shape (UActorComponent inheritance, `ServerSideRewind`, `ServerScoreRequest_Hitscan`, `TDoubleLinkedList`, `MaxRecordTime`, `HeadBoneName`, `GetHistoryCount`, `DumpHistoryToLog`).
- Lag comp component body (Cache/Move/Reset/Enable functions, `InterpBetweenFrames`, `ConfirmHit`, `ServerScoreRequest_Hitscan_Implementation`, both result tags, `LogPaldarkNet`).
- `APaldarkCharacter` wiring (`LagCompSlot`, `HitCollisionBoxes`, `GetHitCollisionBoxes`, 5+ bone names — `head`, `pelvis`, `spine_02`, `upperarm_l`, `calf_r`).
- `APaldarkPlayerController` time sync RPCs (`ServerRequestServerTime`, `ClientReportServerTime`, `GetServerTime`, `GetSingleTripTime`, `ClientServerDelta`, `TimeSyncFrequency`).
- 3 console commands by name (`Paldark.LagComp.DumpHistory`, `Paldark.LagComp.SimulateHit`, `Paldark.LagComp.ArtificialPing`).
- 3 new gameplay tags listed in `EXPECTED_GAMEPLAY_TAGS`.

Validator passes on the current branch (0 errors).

## 3. What's NOT in W16-17 (deferred)

| Item | Lands in | Why |
|------|----------|-----|
| GA_HitscanFire LocalPredicted refactor | W18-19 (Pal Combat Activity) | W16-17 stays focused on the rewind machinery. The W9-10 ability stays `ServerOnly`; the RPC + result struct are ready, only the ability glue defers. |
| Headshot magnitude through GE | W18-19 | Result struct carries `bHeadShot`; the GE SetByCaller bump lands with the LocalPredicted refactor. |
| Client-side prediction (full LCP loop) | W22+ | W16-17 is *server-side* rewind only. |
| Anti-cheat heuristics | W42-43 (live ops) | Variance bounds, cone-of-fire validation, drop heuristics — backend AWS week. |
| Lag comp for Pal abilities (melee + ranged) | W18-19 | Lag comp component is on the player character; Pal AI gets its own rewind once Pal combat lands. |

## 4. Compile + test plan (designer side)

VM that built this PR has **no UE5.4 compiler**, so the structural CI
validator is the only Devin-side gate. Compile + PIE testing must run on
the designer's machine.

### Compile
1. `Right-click PaldarkLab.uproject → Generate Visual Studio project files`.
2. Open `PaldarkLab.sln` in Visual Studio 2022.
3. Build `Development Editor | Win64` — expect 0 errors.

Likely risk areas (call-sites added in this PR — if any fail to compile,
the error message localises directly to one file):

- `Components/BoxComponent.h` + `Components/SkeletalMeshComponent.h` includes in `PaldarkCharacter.cpp` — usually present in `Engine.Build.cs` deps but if missing, add `BoxComponent` to module deps.
- `TDoubleLinkedList<FPaldarkFramePackage>` — UE 5.4 lives in `Containers/List.h`. If a build-version mismatch ever moves it, swap to `Containers/DoubleLinkedList.h`.
- `ServerRequestServerTime_Implementation` + `ClientReportServerTime_Implementation` — these are generated header-side, the cpp definitions must match the UFUNCTION declarations.
- `FAutoConsoleCommandWithWorldAndArgs` — requires `HAL/IConsoleManager.h` include; added in `PaldarkPlayerController.cpp`.

### PIE test loop

1. **Smoke (no lag)** — Standalone, 1 player.
   - `Paldark.Combat.SpawnDummy 500` → dummy 5 m away.
   - Aim at dummy. `Paldark.LagComp.SimulateHit 0` → expect
     `LagComp::ConfirmHit … hit=1` in `LogPaldarkNet`.
   - `Paldark.LagComp.DumpHistory` → expect `count=~120 oldest=… newest=…`.

2. **100 ms ping (single-client)** — same as above, prefix with
   `Paldark.LagComp.ArtificialPing 100` then `Paldark.LagComp.SimulateHit 100`.
   Expect `hit=1` — this is the W16-17 milestone artefact.

3. **2-player listen** — PIE Listen-Server, 2 clients.
   - Client 2: `Paldark.LagComp.ArtificialPing 100`.
   - Client 2 aims at Client 1's pawn, `Paldark.LagComp.SimulateHit 100`.
   - Expect Client 2 log: `Paldark.LagComp.SimulateHit … hit_time=X target=PC_1`.
   - Expect Client 1 (server) log: `LagComp::ConfirmHit on PC_1 … hit=1`.

4. **Failure case (latency > MaxRecordTime)** — bump artificial ping to
   `5000` (5 s), simulate. Expect `LagComp::ServerSideRewind — HitTime …
   older than oldest sample … rejecting.` This validates the rejection
   path is working (anti-cheat hook for W42-43).

### Recording

W16-17 milestone artefact (per roadmap): 30 s PIE video showing
`SimulateHit 100` returning `hit=1` against a dummy / a remote pawn with
`Net PktLag=100` active. Save as `Documents/Devlog/Q2-W16-Rewind-Demo.mp4`.

## 5. Next

- **W18-19** — Pal Combat Activity: spec the predicted-fire ability that
  consumes `ServerScoreRequest_Hitscan`. Per roadmap § Tuần 18–19.
- **W17.5 (optional polish)** — Inventory UMG widget + pickup actor (was
  the alternative branch I offered post-W14-15). Could slip before W18 if
  designer wants visible inventory before predicted fire.
- **W22+ (Q3 Multiplayer Replication)** — Client-side prediction loop on
  top of W16-17 rewind.

## 6. Files touched

```
PaldarkLab/Source/PaldarkLab/Public/Combat/PaldarkLagCompensationTypes.h         (new)
PaldarkLab/Source/PaldarkLab/Public/Combat/PaldarkLagCompensationComponent.h     (new)
PaldarkLab/Source/PaldarkLab/Private/Combat/PaldarkLagCompensationComponent.cpp  (new)
PaldarkLab/Source/PaldarkLab/Public/Player/PaldarkCharacter.h                    (+ LagCompSlot, HitCollisionBoxes, GetHitCollisionBoxes)
PaldarkLab/Source/PaldarkLab/Private/Player/PaldarkCharacter.cpp                 (+ 16 hitboxes in ctor)
PaldarkLab/Source/PaldarkLab/Public/Player/PaldarkPlayerController.h             (+ time sync RPCs + GetServerTime + GetSingleTripTime)
PaldarkLab/Source/PaldarkLab/Private/Player/PaldarkPlayerController.cpp          (+ handshake bodies + 3 console commands)
PaldarkLab/Source/PaldarkLabCore/Public/PaldarkGameplayTags.h                    (+ 3 tag decls)
PaldarkLab/Source/PaldarkLabCore/Private/PaldarkGameplayTags.cpp                 (+ 3 tag defs)
PaldarkLab/README.md                                                             (+ §"Lag compensation server-side rewind (W16-17)")
scripts/ci/validate_paldarklab.py                                                (+ check_lag_compensation_shape + 3 tags in EXPECTED_GAMEPLAY_TAGS)
Documents/Devlog/W16-17-Build.md                                                 (new — this file)
```

11 files touched: 4 new, 7 modified.
