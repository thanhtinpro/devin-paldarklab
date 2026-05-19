# W14-15 — 4-Player Dedicated Server (Net Subsystem + Pal Replication)

**Status:** in-flight (this PR adds the C++ + ini wiring; designer runs the
4-player listen / dedicated test in UE Editor afterwards).
**Window:** roadmap § Tuần 14–15 (Q2 opener — networking infrastructure week,
*not* a feature week).
**Roadmap outcome:** *"4 player + 4 Pal trên server dedicated test."*
**Pillars touched:** P06 (replication), P07 (dedicated server), P02 (game
instance subsystem), P09 (Pal AI replication).

---

## 1. Why W14-15 looks like this

The 1-year roadmap freezes W1-12 at the end of W13 (Q1 Milestone Build, PR #17)
and uses W14-15 as the first networking checkpoint. The roadmap explicitly
states:

> *Login flow basic (no auth). Listen server → switch dedicated. 4 player
> connect. Replicate Pal position/anim.*

That is a deliberately small scope: **no Lobby UMG, no auth, no matchmaking,
no server-side rewind.** Those land in:

| Deferred to | Why                                                                 |
|-------------|---------------------------------------------------------------------|
| W16-17      | Server-side rewind / lag compensation (P06 deep dive)               |
| W23-25      | Lobby UMG widget (P10 — CommonUI Activatable Widget pattern)        |
| W26-28      | Matchmaking / mod.io session browser (P10 + P18)                    |
| W42-43      | Cognito / Steam / EOS auth (P18 — backend AWS week)                 |
| W47-48      | GameLift Fleet / EC2 dedicated hosting (P18 — backend AWS week)     |

So W14-15 is **the smallest possible net surface** that lets a designer hit
`Paldark.Net.Host` in one PIE window, `Paldark.Net.Join 127.0.0.1:7777` in the
other three, and watch 4 Pals follow 4 players on the wire.

---

## 2. PR ledger

| PR  | Title                                                             | Branch                                     | Status |
|-----|-------------------------------------------------------------------|--------------------------------------------|--------|
| #18 | W14-15 — 4-player dedicated server (net subsystem + Pal replication) | `devin/1779005933-w14-15-dedicated-server` | This PR (open) |

Predecessors:

- W1-12 — 9 merged PRs (PR #7 → PR #16) — see `Documents/Devlog/Q1-Milestone-Build.md`.
- W13 — Q1 Milestone Build (PR #17, merged) — playtest + bug-bash docs only.

---

## 3. What shipped (feature map)

### 3.1 New native gameplay tags (5)

`PaldarkLab/Source/PaldarkLabCore/Public/PaldarkGameplayTags.h` + `.cpp`:

| Tag                                       | Meaning                                              |
|-------------------------------------------|------------------------------------------------------|
| `Paldark.Net.Role.Host`                   | This client *is* (or is co-located with) the server. |
| `Paldark.Net.Role.Client`                 | This client is talking to a remote server.           |
| `Paldark.Net.Mode.Standalone`             | `ENetMode::NM_Standalone` (single-player PIE).       |
| `Paldark.Net.Mode.ListenServer`           | `ENetMode::NM_ListenServer` (host has local player). |
| `Paldark.Net.Mode.DedicatedServer`        | `ENetMode::NM_DedicatedServer` (headless).           |

The tags are populated into `UPaldarkNetSubsystem::CachedTopologyTags` on every
`PostWorldInitialization`, so downstream HUD / activity / spawn code can ask
*"are we on a dedicated server?"* via a tag query instead of scattering
`GetNetMode()` switches across the codebase.

### 3.2 `UPaldarkNetSubsystem` (UGameInstanceSubsystem)

Files:

- `PaldarkLab/Source/PaldarkLab/Public/Net/PaldarkNetSubsystem.h`
- `PaldarkLab/Source/PaldarkLab/Private/Net/PaldarkNetSubsystem.cpp`

5 C++ entry points + 4 console commands:

| Console command                | C++ method                            | Topology       |
|--------------------------------|---------------------------------------|----------------|
| `Paldark.Net.Host [Map] [Exp]` | `HostListenServer(Map, ExperienceId)` | Server-only    |
| `Paldark.Net.Join <Address>`   | `JoinServer(Address)`                 | Client-only    |
| `Paldark.Net.Disconnect`       | `DisconnectFromServer()`              | Client-only    |
| `Paldark.Net.Status`           | `GetNetSnapshot()` → log              | Anywhere       |

Subsystem lifecycle:

- `Initialize` — registers the 4 console commands + binds `OnNetworkFailure` /
  `OnTravelFailure` / `OnPostWorldInitialization`.
- `Deinitialize` — unregisters everything (no zombie delegates after PIE
  shutdown).

The subsystem is a **Game Instance** subsystem (not World) on purpose: the
`UWorld` is destroyed when `ServerTravel` swaps maps, so a World subsystem
would lose its registered console commands + cached topology every time the
party leaves the Hub. Game Instance is the correct lifetime tier here.

`FPaldarkNetSnapshot` (returned by `GetNetSnapshot`):

```cpp
struct FPaldarkNetSnapshot
{
    FGameplayTag Role;            // Paldark.Net.Role.*
    FGameplayTag Mode;            // Paldark.Net.Mode.*
    int32        ConnectedPeerCount = 0;
    FString      ServerAddress;
    FString      MapName;
};
```

### 3.3 GameMode net hooks

`APaldarkGameModeBase` now overrides the three connection-lifecycle hooks:

- `PreLogin` — rejects the (N+1)th client with
  `"Server full (4/4). Please try again later."`. Reads
  `ConnectedPlayerCount` against `MaxPlayersPerSession` (default = 4,
  `UPROPERTY(EditDefaultsOnly)` so designers can lower the cap for 2-player
  co-op tests).
- `PostLogin` — increments `ConnectedPlayerCount`, logs
  `[PostLogin] PC_0 joined — 1/4 player(s) connected.` to `LogPaldarkNet`.
- `Logout` — decrements `ConnectedPlayerCount`, logs
  `[Logout] PC_0 left — 0/4 player(s) remaining.`.

`GetMaxPlayers()` inline accessor returns `MaxPlayersPerSession`, so
`Paldark.Net.Status` can print a clean `Peers=N/Cap` line.

`InitGame` also logs the resolved peer cap so the very first log line a
designer sees in a fresh PIE / dedicated launch tells them the topology limit.

### 3.4 Pal replication tuning

`APaldarkPalCharacter` is now bandwidth-budgeted for the 4-player target:

```cpp
NetUpdateFrequency     = 30.0f;   // 30 Hz position updates (matches server tick)
MinNetUpdateFrequency  = 5.0f;    // far-from-player floor
NetCullDistanceSquared = 2.5e9f;  // cm^2 ≈ 500 m cull distance
bReplicates            = true;
bReplicateMovement     = true;
```

`UPaldarkPalLocomotionComponent` replicates the follow target + enable flag so
non-host clients see the same Pal AI behaviour as the host:

```cpp
UPROPERTY(Replicated) TObjectPtr<APawn> FollowedPawn;
UPROPERTY(Replicated) bool bFollowEnabled = false;

void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>&) const override;
// DOREPLIFETIME(UPaldarkPalLocomotionComponent, FollowedPawn);
// DOREPLIFETIME(UPaldarkPalLocomotionComponent, bFollowEnabled);
```

Both fields were `Transient TWeakObjectPtr<APawn>` (server-only) in W3-4 and
are now promoted to a strong `TObjectPtr<APawn>` so the network serialiser can
push them down to clients.

The ASC on the Pal stays in `EGameplayEffectReplicationMode::Minimal`
(unchanged from W7-8) — server-only GE prediction is the right trade-off for
4 Pals × 4 clients.

### 3.5 Config wiring

`PaldarkLab/Config/DefaultEngine.ini`:

```ini
[OnlineSubsystem]
DefaultPlatformService=NULL                 ; LAN-only for W14-15 (no Steam/EOS yet)

[/Script/Engine.GameSession]
MaxPlayers=4                                 ; base-class cap fires before our PreLogin

[/Script/OnlineSubsystemUtils.IpNetDriver]
NetServerMaxTickRate=30                      ; matches 30 Hz NetUpdateFrequency on Pal
MaxClientRate=15000
MaxInternetClientRate=15000
```

`PaldarkLab/Config/DefaultGame.ini` already had `MaxPlayers=4` under the
`GameMapsSettings` section from earlier weeks — left untouched (defence in
depth).

### 3.6 Validator extension

`scripts/ci/validate_paldarklab.py` gained `check_net_subsystem_shape()`. It
fails CI if any of the following drift:

- `PaldarkNetSubsystem.{h,cpp}` missing under `Source/PaldarkLab/{Public,Private}/Net/`.
- Header doesn't declare `class PALDARKLAB_API UPaldarkNetSubsystem : public UGameInstanceSubsystem`.
- Header missing any of `FPaldarkNetSnapshot / HostListenServer / JoinServer / DisconnectFromServer / GetNetSnapshot / RefreshTopology`.
- Cpp missing any of the 4 console command names (`Paldark.Net.Host`,
  `Paldark.Net.Join`, `Paldark.Net.Disconnect`, `Paldark.Net.Status`) or
  `ServerTravel` / `ClientTravel` / `OnPostWorldInitialization` symbols.
- GameMode header missing the 4 net overrides.
- GameMode cpp missing `ConnectedPlayerCount` / `MaxPlayersPerSession` /
  `LogPaldarkNet`.
- Pal locomotion component missing `Replicated` UPROPERTY + matching
  `DOREPLIFETIME` calls.
- Pal character missing `NetUpdateFrequency` / `MinNetUpdateFrequency` /
  `NetCullDistanceSquared` tuning.
- DefaultEngine.ini missing `OnlineSubsystem`, `GameSession` MaxPlayers, or
  `IpNetDriver` blocks.

The 5 net tags (`Paldark.Net.Role.{Host,Client}` +
`Paldark.Net.Mode.{Standalone,ListenServer,DedicatedServer}`) are added to the
existing `EXPECTED_GAMEPLAY_TAGS` set, so a missing `UE_DEFINE_GAMEPLAY_TAG`
in `PaldarkGameplayTags.cpp` also fails CI.

---

## 4. Test plan (designer side — Devin cannot run this on the VM)

The VM has no UE 5.4 compiler, so all 4 of the following must be run by a
designer on a machine with UE 5.4 installed:

1. **Compile** — Generate Project Files + build `PaldarkLab.uproject`,
   `PaldarkLabEditor.Target`, and `PaldarkLabServer.Target`. Expect 0 errors,
   0 warnings.
2. **1-player listen smoke**:
   - PIE in **Standalone** with 1 client.
   - Console: `Paldark.Net.Status` → expect `Role=Host Mode=Standalone Peers=0`.
3. **2-player listen test**:
   - PIE in **Play As Listen Server** with 2 clients.
   - On Client 2: `Paldark.Net.Join 127.0.0.1:7777`.
   - On host: `Paldark.Net.Status` → expect `Role=Host Mode=ListenServer Peers=1`.
4. **4-player listen test**:
   - PIE in **Play As Listen Server** with 4 clients.
   - On host: `Paldark.Net.Status` → expect `Role=Host Mode=ListenServer Peers=3`.
   - Spawn 4 Pals (one per player), confirm `FollowedPawn` replicates correctly
     across all 4 windows.
5. **4-player dedicated test** (Q2 milestone artefact):
   - Launch `PaldarkLabServer.Target` headless.
   - 4 clients run from PIE / standalone instances. Each:
     `Paldark.Net.Join <server-ip>:7777`.
   - On any client: `Paldark.Net.Status` → expect
     `Role=Client Mode=DedicatedServer Peers=0` (clients can't read the peer
     list; only the server can).
   - Spawn 4 Pals, record 30 s video showing players + Pals replicating
     correctly. This is the Q2 milestone artefact.
6. **MaxPlayers cap**:
   - 5th client attempts to join after 4 are connected.
   - Expect: `"Server full (4/4). Please try again later."` returned to client,
     `LogPaldarkNet: [PreLogin] Rejected <addr>: server full 4/4.` on server.

If steps 2–5 all pass, W14-15 is closed.

---

## 5. Known issues / risks (carried into W16-17)

| ID  | Risk                                                                                | Mitigation                                                                  |
|-----|-------------------------------------------------------------------------------------|-----------------------------------------------------------------------------|
| N01 | `UPROPERTY(Replicated) TObjectPtr<APawn>` may need `COND_None` vs `COND_OwnerOnly`. | Currently `COND_None` (everyone sees follow target). Revisit in W16-17.     |
| N02 | `NetCullDistanceSquared = 2.5e9` is ~500 m; designer may want 250 m for indoor maps.| Made `UPROPERTY(EditDefaultsOnly)`-style on Pal BP; tweak per-experience.   |
| N03 | `Paldark.Net.Join` blocks on resolve; LAN with no DNS may stall ~3 s.               | LAN-only test — IP literals (`127.0.0.1:7777`) skip DNS. Document in README.|
| N04 | No lag compensation yet — hitscan fire on Pal will favour shooter at 100+ ms ping.  | W16-17 — server-side rewind PR (next).                                      |
| N05 | No `bAutoReceiveInput` on Pal — co-op `possess` not yet possible.                   | W19+ (P&P play loop). Not on critical path for W14-15.                      |
| N06 | Pal `ASC` Minimal mode skips GE prediction.                                         | Intentional. Re-evaluate after W16-17 lag comp lands.                       |
| N07 | `?Experience=` URL option may be eaten by `ServerTravel` if both maps are absolute. | Tested in W1 day 6-7 path; should still work. Re-test in step 5.            |
| N08 | NAT punch-through not configured — only LAN / IPv4 reachable hosts work.            | Deferred to W42-43 (Steam P2P) or W47-48 (GameLift dedicated).              |

---

## 6. Feature map (file → behaviour)

| File                                                                              | Δ behaviour                                                                                                |
|-----------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------------------|
| `Source/PaldarkLab/Public/Net/PaldarkNetSubsystem.h`                              | New `UGameInstanceSubsystem` with 5 entry points + `FPaldarkNetSnapshot` + `CachedTopologyTags`.            |
| `Source/PaldarkLab/Private/Net/PaldarkNetSubsystem.cpp`                           | 4 console commands + `ServerTravel` / `ClientTravel` body + failure relay.                                  |
| `Source/PaldarkLab/Public/Framework/PaldarkGameModeBase.h`                        | 4 new overrides (`PreLogin`, `PostLogin`, `Logout`, `GetMaxPlayers`) + `MaxPlayersPerSession`.              |
| `Source/PaldarkLab/Private/Framework/PaldarkGameModeBase.cpp`                     | 4-player cap enforcement + log lines on every connection event.                                              |
| `Source/PaldarkLab/Public/Pal/Components/PaldarkPalLocomotionComponent.h`         | `FollowedPawn` + `bFollowEnabled` promoted to `UPROPERTY(Replicated) TObjectPtr<APawn>`.                    |
| `Source/PaldarkLab/Private/Pal/Components/PaldarkPalLocomotionComponent.cpp`      | `GetLifetimeReplicatedProps` + `SetIsReplicatedByDefault(true)`.                                            |
| `Source/PaldarkLab/Private/Pal/PaldarkPalCharacter.cpp`                           | 4-player net tuning: 30 Hz update, 5 Hz floor, 500 m cull.                                                  |
| `Source/PaldarkLabCore/Public/PaldarkGameplayTags.h`                              | 5 new `UE_DECLARE_GAMEPLAY_TAG_EXTERN` (`Paldark.Net.Role.*` + `Paldark.Net.Mode.*`).                       |
| `Source/PaldarkLabCore/Private/PaldarkGameplayTags.cpp`                           | Matching 5 `UE_DEFINE_GAMEPLAY_TAG`.                                                                        |
| `Config/DefaultEngine.ini`                                                        | `[OnlineSubsystem] DefaultPlatformService=NULL` + `[GameSession] MaxPlayers=4` + `[IpNetDriver]` block.    |
| `scripts/ci/validate_paldarklab.py`                                               | New `check_net_subsystem_shape()` + 3 new ini regex patterns + 5 new gameplay tag names.                    |
| `PaldarkLab/README.md`                                                            | New "Dedicated server + 4-player test loop (W14-15)" section with authoring + test order + anti-patterns.  |

---

## 7. Anti-patterns avoided

- **World subsystem for net entry points** — would lose registered console
  commands on every `ServerTravel`. Used Game Instance subsystem instead.
- **`GetNetMode()` scattered everywhere** — used tag-cache pattern
  (`CachedTopologyTags`) so callers do a tag query, not a `switch`.
- **Replicating `TWeakObjectPtr<APawn>`** — weak pointers don't survive
  serialisation; promoted to strong `TObjectPtr`.
- **Default `NetUpdateFrequency = 100.0f` on `ACharacter`** — 4 × 4 × 100 Hz
  is 1600 actor ticks/s. Capped at 30 Hz.
- **`UPROPERTY(Replicated)` without `GetLifetimeReplicatedProps`** — UE
  silently drops replication. Validator catches this now.
- **Hard-coded server IP** — `Paldark.Net.Join` reads the address from console
  arg, so LAN / Tailscale / localhost all work without recompile.
- **Calling `ServerTravel` from client** — subsystem checks
  `World->GetNetMode()` before issuing the travel call.
- **Skipping the `PreLogin` 5-client check** — relying on `AGameSession`'s
  default would fire after `Super::PreLogin` had already initialised the
  connection, wasting one round-trip. Our check fires before any state mutates.

---

## 8. Next steps

| Week  | Task                                                                                       | Why now                                                                |
|-------|--------------------------------------------------------------------------------------------|------------------------------------------------------------------------|
| W16-17| Server-side rewind / lag compensation (`UPaldarkLagCompensationComponent` + history ring). | First feature week of Q2; addresses N04 directly.                       |
| W18-19| Activity FSM extensions — combat-aware Pal activity (Aggro / Engage / Flee).               | Carries on the P09 work from W5-6; replication now in place.           |
| W20-21| Inventory drop / pickup actor + UMG widget.                                                | Closes the W11-12 inventory loop with UI surface. Q2 mid-quarter.       |
| W22   | Q2 Milestone Build (mirrors W13 — internal playtest + bug bash).                            | Q2 closure. Generates `Q2-Milestone-Build.md` + 4-player video.         |

---

## 9. Acceptance criteria (designer signs off when all 4 pass)

- [ ] PaldarkLab compiles clean on UE 5.4 (Editor + Server targets, 0 errors / 0 warnings).
- [ ] `Paldark.Net.Status` returns the expected topology in Standalone / Listen / Dedicated PIE.
- [ ] 4 clients connect to a dedicated server, each sees 4 Pals correctly following 4 different players.
- [ ] 5th client gets the `"Server full"` rejection (server log shows `[PreLogin] Rejected`).

Once those 4 items are checked, W14-15 closes and W16-17 (lag compensation)
begins.

---

## 10. References

- `Documents/PALDARK/03-Roadmap_1_Year.md` § Tuần 14–15
- `Documents/UE5_Course/06-P06-Replication.md` (DOREPLIFETIME, RPC, RepNotify)
- `Documents/UE5_Course/07-P07-Dedicated_Server.md` (ServerTravel / ClientTravel flow)
- `PaldarkLab/README.md` § "Dedicated server + 4-player test loop (W14-15)"
- `scripts/ci/validate_paldarklab.py` § `check_net_subsystem_shape()`
