# P06 — Multiplayer / Replication

> Tự soạn từ: [07] CPP MP Crash, [10] Blaster, [13] Crunch, [04] RoN.
>
> **Pillar khó nhất, debt cao nhất nếu sai.** Lag Compensation + Server-Side Rewind = sự khác biệt giữa hobby và shipped MP.

---

## Pillar Identity

**Cái gì:** Replicate variable + RPC + understand authority + lag compensation cho MP.

**Scope:**
- ✅ Variable replication (`UPROPERTY(Replicated, ReplicatedUsing=...)`).
- ✅ DOREPLIFETIME + condition (OwnerOnly, InitialOnly, SkipOwner).
- ✅ RPC: Server / Client / NetMulticast / Reliable / Unreliable.
- ✅ RepNotify (`OnRep_*` callback).
- ✅ NetUpdateFrequency tuning.
- ✅ Significance Manager integration (P15).
- ✅ Server-Side Rewind / Lag Compensation Frame Buffer.
- ✅ GAS replication mode (Full/Mixed/Minimal).
- ✅ Server-authoritative movement.

**Out of scope:**
- ❌ Backend matchmaking (P07).
- ❌ ASC replication detail (P08).

---

## PALDARK cần gì từ pillar này?

| Need | Solution |
|------|----------|
| TTK ngắn fair cho mọi ping | Server-Side Rewind 300ms buffer |
| Pal command từ client | Server RPC + RepNotify Command state |
| 4 player Raid replicate state | DOREPLIFETIME default + bandwidth budget |
| Civilian fear state | NetMulticast OnFear |
| Pal mid-air mount jump | Replicated MovementMode change |
| Extract beacon countdown | GameState replicated float countdown |

---

## Sources Matrix

| Source | Coverage | Lesson lấy từ đó |
|--------|----------|------------------|
| [07] CPP MP Crash | 🟢 | L1 Authority/Role, L2 Replication variable, L3 RPC, L4 RepNotify |
| [10] Blaster | 🟢 | L5 Server-Side Rewind / Lag Comp Frame Buffer |
| [13] Crunch | 🟢 | L6 GAS replication mode Mixed |
| [04] RoN GDD | 🟠 | L7 NetUpdateFrequency + Significance integration |

---

## Prerequisite

- P01 (UCLASS, UPROPERTY).
- P02 (Authority, NetRole).
- P03 (Component composition).

---

## Lessons

### L1 — Authority + NetRole đào sâu 📖🧪

**Goal:** Phân biệt server vs client logic, dùng đúng macro.

**Concept:**
- `HasAuthority()` — server hoặc standalone.
- `GetLocalRole()` 3 giá trị: Authority / AutonomousProxy / SimulatedProxy.
- `GetRemoteRole()` đối ứng.
- `IsLocallyControlled()` — owning client của Pawn này.
- `GetNetMode()`: Standalone / DedicatedServer / ListenServer / Client.

**Source learning path:**
- [07] CPP MP Crash § Authority & Role chapter.

**Practice exercise (2 hour):**
1. Tạo `APaldarkLabReplActor` log mỗi tick Role + RemoteRole.
2. PIE 2 player (1 listen server + 1 client).
3. So sánh log:
   - Server: Authority + AutonomousProxy on remote.
   - Client (owning): AutonomousProxy + Authority on remote.
   - Client (other player): SimulatedProxy.

**PALDARK take-away:**
- 🟢 Server-only logic wrap `if (HasAuthority())`.
- 🟢 Owner-only UI wrap `if (IsLocallyControlled())`.
- 🟢 Cosmetic-only logic chạy trên SimulatedProxy.

**Apply ở week:** Roadmap week 10.

---

### L2 — Variable Replication + DOREPLIFETIME 🧪

**Goal:** Replicate biến từ server → client với condition.

**Concept:**
- Constructor: `bReplicates = true`.
- Header: `UPROPERTY(Replicated) float Health;`.
- CPP: override `GetLifetimeReplicatedProps` + `DOREPLIFETIME(Class, Property)`.
- Condition variants:
  - `DOREPLIFETIME_CONDITION(Class, Prop, COND_OwnerOnly)` — chỉ owning client.
  - `COND_InitialOnly` — replicate 1 lần lúc spawn.
  - `COND_SkipOwner` — không gửi owner.
  - `COND_SimulatedOnly` — chỉ simulated (cosmetic).

**Source learning path:**
- [07] CPP MP § Replication chapter.

**API / Class chính:**
- `DOREPLIFETIME(Class, Property)`
- `DOREPLIFETIME_CONDITION(Class, Property, Condition)`

**Practice exercise (2 hour):**
1. `APaldarkLabReplActor` có `Health` + `MaxHealth` replicated.
2. `Score` replicated `COND_OwnerOnly`.
3. PIE 2 player: thay Health server → client thấy. Score server set cho A → B không thấy.

**PALDARK take-away:**
- 🟢 PlayerState: Score + Kills + Extracts replicated default.
- 🟢 Pal Bond level chỉ replicate `COND_OwnerOnly` (private info).

**Apply ở week:** Roadmap week 10.

---

### L3 — RPC: Server / Client / Multicast 🧪

**Goal:** Gọi function server hoặc client từ remote.

**Concept:**
- `UFUNCTION(Server, Reliable, WithValidation)` — client → server.
- `UFUNCTION(Client, Reliable)` — server → owning client.
- `UFUNCTION(NetMulticast, Reliable/Unreliable)` — server → all clients.
- Reliable = guaranteed (TCP-like, expensive).
- Unreliable = fire-and-forget (UDP-like, cosmetic OK).
- `_Implementation` suffix cho impl func; `_Validate` cho server validation.

**Source learning path:**
- [07] CPP MP § RPC chapter.

**Practice exercise (3 hour):**
1. Tạo `Server_RequestSpawnPal(FName PalType)` Server + Reliable + Validate.
2. Validate check `PalType != NAME_None`.
3. Implementation spawn Pal actor.
4. Multicast `Multicast_PlayBondVFX()` Unreliable.

**PALDARK take-away:**
- 🟢 Pal command via Server RPC: `Server_PalCommand(EPalCommand Cmd)` Reliable.
- 🟢 Bond VFX qua Multicast Unreliable.
- 🟢 Damage feedback (hit marker) qua Client RPC tới owning shooter.

**Apply ở week:** Roadmap week 10-11.

---

### L4 — RepNotify (OnRep_) callback 🧪

**Goal:** Trigger logic khi variable replicate change.

**Concept:**
- `UPROPERTY(ReplicatedUsing=OnRep_Health) float Health;`.
- Method: `UFUNCTION() void OnRep_Health(float OldValue);`.
- Server CHỈ change value → server không gọi OnRep_; client tự gọi khi replicate đến.
- Để server cũng trigger logic, call manually trong setter.

**Source learning path:**
- [07] CPP MP § RepNotify chapter.
- [10] Blaster `BlasterCharacter::OnRep_OverlappingWeapon`.

**Practice exercise:**
1. `OnRep_Health(float OldHealth)` log "Health change OldHealth → Health".
2. PIE: server damage → client thấy log + UI update.

**PALDARK take-away:**
- 🟢 PlayerState `Score` + `OnRep_Score` → UI auto-update không cần manual broadcast.
- 🟢 Pal state `OnRep_CurrentActivity` → AnimBP swap state.

**Apply ở week:** Roadmap week 10.

---

### L5 — Server-Side Rewind / Lag Compensation (Blaster CORE) 🧪

**Goal:** Bắn fair cho mọi ping — server rewind hitbox theo client timestamp.

**Concept:**
- **Problem:** client thấy enemy ở vị trí A (300ms ago lag), bắn, server thấy enemy ở B → miss.
- **Solution:** server lưu ring buffer 4s hitbox history mỗi character (60 frames @ 15Hz).
- Client fire → packs `FireTimestamp` (server time client thấy enemy lúc đó).
- Server receive Server_Fire(...) → rewind tất cả character hitbox to that timestamp → trace → if hit → confirm damage.
- Server tự reset hitbox về current sau check.

**Source learning path:**
- [10] Blaster § Server-Side Rewind chapter — entire section dedicated.
- Cross-ref: `Documents/GameDesign/10-MP_Shooter_GDD.md` § Phase 5 ADR-1.

**API / Class chính:**
- `ULagCompensationComponent`
- `FFramePackage` struct (timestamp + hitbox transforms).
- `FServerSideRewindResult ServerSideRewind(Character, TraceStart, HitLocation, HitTime)`.

**Practice exercise (1 ngày):**
1. Copy `ULagCompensationComponent` pattern từ Blaster.
2. Mỗi Tick (15Hz) snapshot all bone xform → FrameHistory.
3. `Server_ScoreRequest(HitChar, HitTime, ...)` rewind history → trace → confirm.

**PALDARK take-away:**
- 🟢 **CORE** — `UPaldarkLagCompComp` 100% copy Blaster.
- 🟢 Buffer 4s đủ cho ping ≤ 200ms.

**Apply ở week:** Roadmap week 17-18.

---

### L6 — GAS Replication Mode Mixed (Crunch) 📖🧪

**Goal:** Optimize bandwidth cho GAS với mode Mixed.

**Concept:**
- 3 mode:
  - **Full** — replicate to all clients (default).
  - **Mixed** — owner-only for Tag/GE/Cooldown; minimal for others.
  - **Minimal** — nobody knows (cho AI).
- `ASC->SetReplicationMode(EGameplayEffectReplicationMode::Mixed)` ở `BeginPlay`.
- Trade-off: bandwidth ↓ nhưng spectator-mode loses info.

**Source learning path:**
- [13] Crunch § GAS Optimization chapter.

**Practice exercise:**
1. `UPaldarkLabASC::PostInitProperties` set Mixed.
2. Compare network usage trước/sau.

**PALDARK take-away:**
- 🟢 Player ASC: Mixed (owner full + others minimal).
- 🟢 Pal ASC: Mixed (owner thấy đầy đủ cooldown).
- 🟢 AI NPC ASC: Minimal.

**Apply ở week:** Roadmap week 13.

---

### L7 — NetUpdateFrequency tuning + Significance integration 📖

**Goal:** Tăng/giảm replication rate theo importance.

**Concept:**
- `NetUpdateFrequency` (default 100Hz) — max replicate rate.
- `MinNetUpdateFrequency` (default 2Hz) — min rate khi không có thay đổi.
- `NetCullDistanceSquared` — beyond this không replicate.
- Combine với Significance Manager (P15): far actor → reduce frequency.

**Source learning path:**
- [04] RoN GDD § Phase 5 ADR-4 (Significance).
- [Epic Replication Optimization doc].

**Practice exercise:**
1. `APaldarkLabReplActor::SetSignificance(float Value)` adjust NetUpdateFrequency.
2. PIE 2 player: far → log shows < 10Hz; near → 60Hz.

**PALDARK take-away:**
- 🟢 PALDARK Significance Subsystem manage NetUpdateFreq:
  - Near (< 30m): 30Hz.
  - Mid (30-100m): 10Hz.
  - Far (> 100m): 2Hz hoặc dormant.

**Apply ở week:** Roadmap week 24.

---

## ⚡ Capstone Exercise

**Goal:** PaldarkLab MP với LagComp + full RPC + replication.

**Yêu cầu:**
1. `APaldarkLabCharacter` replicated Health + Score (OwnerOnly).
2. `Server_Fire(HitLoc, HitTime)` + `Multicast_PlayFireVFX()`.
3. `OnRep_Health` → UI update.
4. `ULagCompensationComponent` 4s buffer.
5. Mock test: 2 PIE client, 1 với artificial 200ms ping → hit register fair.

**Acceptance:** Network insights show < 5KB/s per player ở idle, < 20KB/s combat.

**Effort:** 1 tuần.

---

## Khi apply vào PALDARK roadmap

| Week | Pillar P06 usage |
|------|------------------|
| W10 | Replicate Health + Score + Pal state |
| W10-11 | Server RPC cho Pal command + interact |
| W13 | GAS Mixed replication |
| W17-18 | Server-Side Rewind / Lag Comp |
| W24 | NetUpdateFrequency + Significance |

---

## Anti-patterns (cảnh báo)

- 🔴 Replicate mọi biến không cần — bandwidth bloat.
- 🔴 Reliable RPC cho cosmetic (footstep, muzzle flash) → buffer overflow.
- 🔴 Client-authoritative damage → cheat exploit.
- 🔴 Quên `bReplicates = true` constructor → silent fail.
- 🔴 Multicast Reliable cho event tần suất cao (mỗi frame).
- 🔴 RepNotify gọi GAS RPC inside → infinite loop nguy cơ.
- 🔴 Forget Server-Side Rewind cho hitscan → high-ping player cheat-feel.

---

## Tham chiếu

- [`Documents/UE5_Core_Pillars.md`](../UE5_Core_Pillars.md) § P6.
- [`Documents/GameDesign/07-CPP_MP_Crash_GDD.md`](../GameDesign/07-CPP_MP_Crash_GDD.md).
- [`Documents/GameDesign/10-MP_Shooter_GDD.md`](../GameDesign/10-MP_Shooter_GDD.md).
- [`Documents/PALDARK/01-GameDesign.md`](../PALDARK/01-GameDesign.md) ADR-008 Lag Comp.
