# P15 — Performance & Production-grade

> Tự soạn từ: [01] Tom Looman, [12] Pro UE Coding, [14] Lyra, [04] ReadyOrNot.
>
> **Differentiator giữa hobby vs shipped.** PALDARK 4-player extraction + Pal AI + open Dark Zone — Significance Manager bắt buộc từ tuần đầu.

---

## Pillar Identity

**Cái gì:** Profile, fix hitch, optimize tick + spawn + GPU.

**Scope:**
- ✅ Unreal Insights — capture + analyze.
- ✅ Significance Manager — distance-based detail reduction.
- ✅ Object Pooling — reuse spawned actor.
- ✅ Aggregate Ticking — batch tick.
- ✅ PSO precaching — pre-compile shader.
- ✅ Data-Oriented Design (DoD) — flat array, cache-friendly.
- ✅ LOD setup (mesh, shadow, anim).
- ✅ NetCullDistance + NetUpdateFrequency tune (P06).

**Out of scope:**
- ❌ GPU shader optimization (engine-level).
- ❌ Replication detail (P06).

---

## PALDARK cần gì từ pillar này?

| Need | Solution | Source |
|------|----------|--------|
| 100+ actor in Dark Zone (Pal + NPC + civilian + loot) | Significance Manager | [04] RoN |
| Projectile (bullet) tần suất cao | Object Pool | [01] Tom Looman |
| 12 Pal team tick AI | Aggregate Ticking | [01] Tom Looman |
| First-time spawn no hitch | PSO Precache | UE doc |
| Dynamic 1080p min 60fps target | LOD + Significance | [04] RoN |
| Memory budget 8GB | Bundle + soft ref | P14 |

---

## Sources Matrix

| Source | Coverage | Lesson lấy từ đó |
|--------|----------|------------------|
| [01] Tom Looman | 🟢 | L1 Insights basic, L2 Object Pool, L3 Aggregate Tick, L4 Data-Oriented projectile |
| [12] Pro UE | 🟡 | L5 Graphics/audio perf concepts |
| [14] Lyra | 🟢 | L6 Performance Optimization Section |
| [04] RoN GDD | 🟢 | L7 Significance Manager production, L8 Audio Propagation perf |

---

## Prerequisite

- P01 (basic C++).
- P03 (Component).
- P06 (NetUpdateFrequency).
- P14 (Async Asset).

---

## Lessons

### L1 — Unreal Insights basic 🧪

**Goal:** Capture frame trace, find expensive function.

**Concept:**
- `Build → Cook → run` with `-trace=...`.
- Or in-editor `Window → Developer Tools → Unreal Insights`.
- View: Timing (per-frame ms), Memory (alloc), Network (bandwidth).
- Mark code section: `TRACE_CPUPROFILER_EVENT_SCOPE(FName)`.

**Source learning path:**
- [01] Tom Looman § Insights chapter.
- [Epic Insights doc](https://docs.unrealengine.com/5.3/en-US/unreal-insights-in-unreal-engine/).

**Practice exercise (2 hour):**
1. Run PIE → start trace.
2. View Timing → find function > 1ms.
3. Add `TRACE_CPUPROFILER_EVENT_SCOPE(MyExpensiveFunc)` → re-trace → see in trace.

**PALDARK take-away:**
- 🟢 Mỗi week ≥ 1 Insights capture → catch regression.
- 🟢 Trace scope tag toàn bộ component tick.

**Apply ở week:** Roadmap week 24 + every week from W36+.

---

### L2 — Object Pooling 🧪

**Goal:** Reuse projectile actor thay vì spawn/destroy hằng frame.

**Concept:**
- Pool: pre-spawn N actor, hide.
- Request → unhide + reset state + return.
- Done → hide + back to pool.
- Avoid `SpawnActor` + GC churn.

**Source learning path:**
- [01] Tom Looman § Object Pool experimental chapter.

**API / Class chính:**
- `UObjectPoolSubsystem` (custom).
- `AcquireActor(Class) → AActor*`.
- `ReleaseActor(AActor*)`.

**Practice exercise (3 hour):**
1. `UPaldarkLabProjectilePoolSubsystem` pre-spawn 100 projectile.
2. Replace `SpawnActor<AProjectile>` với `Pool->Acquire()`.
3. Insights compare before/after — `SpawnActor` time drop to ~0.

**PALDARK take-away:**
- 🟢 Bullet projectile pool 200.
- 🟢 Particle effect pool 50.
- 🟢 Audio cue actor pool 30.

**Apply ở week:** Roadmap week 24.

---

### L3 — Aggregate Ticking 🧪

**Goal:** N actor có cùng Tick() → batch thành 1 tick lớn.

**Concept:**
- Default: mỗi actor tick → overhead per-actor.
- Aggregate: 1 manager `Tick` iterate array → process all instance.
- Cache-friendly: data flat array.

**Source learning path:**
- [01] Tom Looman § Aggregate Tick chapter.

**Practice exercise:**
1. `UPaldarkLabAggregateTicker` Subsystem.
2. `Register(IThing*)` add to flat array.
3. Subsystem Tick → loop → call each.
4. Compare 1000 individual actor tick vs 1 aggregated tick.

**PALDARK take-away:**
- 🟢 Pal AI tick (Activity FSM) → aggregate via Subsystem.
- 🟢 Civilian tick → aggregate.
- 🟢 Loot pickup detection → aggregate (instead of overlap event).

**Apply ở week:** Roadmap week 24.

---

### L4 — Data-Oriented Projectile (experimental) 📖

**Goal:** Projectile = struct in flat array, không phải Actor.

**Concept:**
- Traditional: 1 projectile = 1 AActor with Tick + Mesh + Collision.
- DoD: 1 projectile = `FProjectileData` struct.
- All projectile in `TArray<FProjectileData>` — flat memory.
- 1 Manager Tick: SIMD-friendly loop.
- Render: instanced mesh component.

**Source learning path:**
- [01] Tom Looman § DoD experimental chapter.

**Trade-off:**
- Pros: 10x performance for >500 projectile.
- Cons: lose Blueprint extensibility, complex.

**PALDARK take-away:**
- 🟡 Beta: Object Pool (L2) đủ rồi.
- 🟡 Post-beta if 4-player + 200 projectile/sec → consider DoD.

**Apply ở week:** Defer post-beta.

---

### L5 — Graphics/Audio basics (Pro UE) 📖

**Goal:** Hiểu LOD + Texture stream + Audio compression.

**Concept:**
- LOD: 4 level (LOD0 high, LOD3 low). Auto-switch by screen size.
- Texture streaming: pool texture by viewer distance.
- Audio: compress + bus + reverb zone.

**Source learning path:**
- [12] Pro UE § Graphics chapter + Audio chapter.

**PALDARK take-away:**
- 🟢 Pal mesh 4 LOD (cute → recognizable silhouette → simple).
- 🟢 Texture pool 4GB target.
- 🟢 FMOD compress + bus (mix in P16).

**Apply ở week:** Roadmap week 24.

---

### L6 — Lyra Performance Optimization 📖

**Goal:** Lyra has dedicated optimization section.

**Concept (Lyra):**
- PSO precaching — pre-compile shader so first-time spawn no hitch.
- Async asset bundle.
- Significance integration.
- Async physics tick.

**Source learning path:**
- [14] Exploring Lyra § Performance chapter.

**PALDARK take-away:**
- 🟢 PSO precache cho Player + Pal + 5 most-spawned enemy.
- 🟢 Audit bundle config (P14).

**Apply ở week:** Roadmap week 38.

---

### L7 — Significance Manager (RoN CORE) 🧪

**Goal:** Actor far away → reduce tick rate + LOD + AI.

**Concept:**
- `USignificanceManager::RegisterObject(Object, Tag, CalcSignificanceFunc, PostSigUpdateFunc)`.
- Each frame: calc significance score (e.g., 1 / distance) → call `PostSigUpdate` with new score.
- Actor adjust: tick interval, LOD, AI rate based on score.

**Source learning path:**
- [04] RoN GDD § Phase 5 ADR-4 (Significance + ThreatOctree).
- [Epic Significance Manager doc].

**API / Class chính:**
- `USignificanceManager`.
- `Significance` (float 0..1).

**Practice exercise (4 hour):**
1. `UPaldarkLabSignificanceManager` GameInstance Subsystem.
2. Pal/NPC Register with calc func: `Score = 1.0 - clamp(DistanceToPlayer/100m, 0, 1)`.
3. PostSigUpdate: 
   - Score > 0.7 → TickInterval = 0.0 (every frame).
   - Score 0.3-0.7 → TickInterval = 0.1.
   - Score < 0.3 → TickInterval = 0.5 + LOD3.

**PALDARK take-away:**
- 🟢 **CORE** — Significance Manager mọi NPC + Pal + civilian từ tuần đầu.
- 🟢 Tích hợp với NetUpdateFrequency (P06).

**Apply ở week:** Roadmap week 24.

---

### L8 — Audio Propagation perf (RoN) 📖

**Goal:** Audio occlusion + reverb không tick mọi frame.

**Concept:**
- RoN audio: trace from source to listener → check wall/door → adjust LP filter.
- Trace expensive → batch: 1 trace per 100ms, smooth interpolate between samples.

**Source learning path:**
- [04] RoN GDD § Phase 2 audio.

**PALDARK take-away:**
- 🟢 Pal voice + Dark Zone ambience → trace 100ms, smooth.

**Apply ở week:** Roadmap week 29.

---

## ⚡ Capstone Exercise

**Goal:** PaldarkLab profile + optimize 100-actor scene.

**Yêu cầu:**
1. Spawn 100 dummy AI + 50 loot pickup.
2. Baseline Insights capture.
3. Apply: Significance Manager + Object Pool + Aggregate Tick.
4. Post-optimization Insights capture.
5. Frame time before vs after.

**Acceptance:** Frame time drop ≥ 30% sau optimize.

**Effort:** 1 tuần.

---

## Khi apply vào PALDARK roadmap

| Week | Pillar P15 usage |
|------|------------------|
| W24 | Significance Manager + Object Pool + Aggregate Tick — first pass |
| W29 | Audio propagation perf |
| W36 | GAS perf optimize |
| W38 | PSO precache + Lyra perf integration |
| W40+ | Continuous Insights every week |

---

## Anti-patterns (cảnh báo)

- 🔴 Tick every frame for far actor (no Significance).
- 🔴 `SpawnActor` hằng frame (no Object Pool).
- 🔴 Hard ref loading entire content on level start.
- 🔴 Texture 4096 cho item icon 64px display — waste VRAM.
- 🔴 No LOD on mesh > 1m view distance.
- 🔴 Insights capture không trace scope → fragmented data.
- 🔴 Optimize without profiling first → wasted effort.

---

## Tham chiếu

- [`Documents/UE5_Core_Pillars.md`](../UE5_Core_Pillars.md) § P15.
- [`Documents/GameDesign/01-ActionRoguelike_GDD.md`](../GameDesign/01-ActionRoguelike_GDD.md) — Object Pool, Aggregate Tick, DoD.
- [`Documents/GameDesign/04-ReadyOrNot_GDD.md`](../GameDesign/04-ReadyOrNot_GDD.md) — Significance.
- [`Documents/GameDesign/14-Exploring_Lyra_GDD.md`](../GameDesign/14-Exploring_Lyra_GDD.md) — Lyra perf.
