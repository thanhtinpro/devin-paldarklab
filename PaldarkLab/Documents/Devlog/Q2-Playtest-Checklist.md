# PALDARK — Q2 Playtest Checklist (Tuần 26)

> **Goal:** 1 file walkthrough chạy 1 lần PIE 4-client trong UE 5.4 Editor, chạm hết feature W14-25. Designer in trang này ra giấy → tick từng box → screenshot → ship video 60s 4-quadrant.
>
> **Prerequisites:**
> - UE 5.4 Editor (chưa có Toolchain trên VM — phải chạy trên máy designer / dev).
> - `git pull` về `main` (commit `2e8a210` trở lên — sau PR #23).
> - Q1 milestone đã đóng (designer đã author đầy đủ asset từ [`Q1-Playtest-Checklist.md`](Q1-Playtest-Checklist.md) § 0).
> - [`Documents/Devlog/Q2-Milestone-Build.md`](Q2-Milestone-Build.md) đọc qua một lần.

---

## 0. Q2 setup delta (≈ 90 phút — bổ sung vào Q1 setup)

> Tất cả bước Blueprint authoring đã có chi tiết trong [`PaldarkLab/README.md`](../../PaldarkLab/README.md). Section này chỉ tóm ngắn để designer biết phải xếp đúng thứ tự cộng dồn từ Q1.

### 0.1 Net + lag comp (W14-15 + W16-17)

- [ ] **Dedicated server target build.** `RunUAT.bat BuildCookRun -project=PaldarkLab.uproject -platform=Win64 -targetplatform=Win64 -server -serverconfig=Development -noclient -build -cook -stage -pak`. Output `PaldarkServer.exe` trong `Binaries/Win64/`. Verify launch: `PaldarkServer.exe Raid_Sandbox -log`. Log phải có `LogPaldarkNet — NetSubsystem initialized`.
- [ ] **Client → server time sync.** Trong PIE listen+client: `Paldark.Net.DumpTimeSync` (server console) → log `clients=N rtt_avg=… clock_skew_ms=…`. Skew phải < 50ms sau 5s.
- [ ] **Lag comp frame buffer.** PIE listen+1 client với 80ms emulated ping (Editor Preferences → Network Emulation). Client LMB → server log `UPaldarkLagCompensationComponent::RewindToServerTime rewound=Yes shot_time=… server_time=… delta=…` + damage applied.

### 0.2 Pal combat (W18-19)

- [ ] **Companion combat shape.** `BP_PaldarkPalCharacter` (Q1) phải có components: `UPaldarkPalCombatComponent` + `UPaldarkPalPerceptionComponent`. Verify trong BP class default → Components panel.
- [ ] **GA `BP_GA_PalAttack`.** Subclass `UPaldarkGameplayAbility_PalAttack`, set `DamageEffectClass=GE_Damage_Standard` (Q1 carry), `AttackRange=1500`, `AttackCooldownSeconds=1.5`, `BaseDamage=15`. Granted ở `BeginPlay` server-side qua `UPaldarkPalCombatComponent::GrantAbilities`.
- [ ] **Team tag wire.** `BP_PaldarkPalCharacter` → `UPaldarkPalCombatComponent.TeamTag=Paldark.Team.Friendly`. Hostile sẽ flip sang `Paldark.Team.Hostile`.

### 0.3 Hostile-Pal AI (W20-21)

- [ ] **2 hostile subclass BP.**
  - `BP_PaldarkCharacter_Direhound` subclass `APaldarkPalCharacter_Direhound` + mesh wolf-like + ABP_Manny (placeholder). `CandidateActivities` override (KHÔNG inherit FollowSquadCommand!): `[Patrol, Stalk, Combat]`. `UPaldarkPalCombatComponent.TeamTag=Paldark.Team.Hostile`.
  - `BP_PaldarkCharacter_Razorbird` subclass `APaldarkPalCharacter_Razorbird` + mesh bird-like. Movement mode `Flying` (set trong ctor → verify ở BP). Same `CandidateActivities` + `TeamTag=Hostile`.
- [ ] **Pal Spawner placement.** Drop 4× `BP_PalSpawner` (subclass `APaldarkPalSpawner`) vào `Raid_Sandbox`. Per-spawner config:
  - Spawner #1: `SpawnTable={Direhound: 3}`, `WaypointPoints=[3 PointTargets]`, `RespawnCooldown=120s`, `MaxActiveCount=3`.
  - Spawner #2: `SpawnTable={Razorbird: 2}`, `WaypointPoints=[5 high-altitude PointTargets]`, `RespawnCooldown=180s`.
  - Spawner #3: `SpawnTable={Direhound: 5}` (pack test).
  - Spawner #4: `SpawnTable={Razorbird: 3}` (aerial threat at extraction).
- [ ] **Pack subsystem health.** Server console: `Paldark.Pal.DumpPackState` → log `packs=N entries={...}`.

### 0.4 Squad system (W22-23)

- [ ] **DA_SquadCommandSet_Default.** Content Browser → `/Game/Paldark/Squad/` → New → `PaldarkSquadCommandSet`. Wire 3 row:
  - `Paldark.Squad.Command.Follow` → label "Follow", icon Manny portrait, color green.
  - `Paldark.Squad.Command.Stay` → label "Stay", icon stop sign, color yellow.
  - `Paldark.Squad.Command.Attack` → label "Attack", icon crosshair, color red.
- [ ] **IMC_Squad.** New IMC. Bind:
  - `IA_PingSpot` → T (Digital Bool).
  - `IA_PingEnemy` → V (Digital Bool).
  - `IA_SquadCommandRadial` → Q (Digital Bool, hold).
- [ ] **Append IMC.** `PD_RaidPlayer` (Q1) → `DefaultMappingContexts[]` += row `(IMC_Squad, priority 0, bRegisterWithSettings=true)`.
- [ ] **SquadMembership component.** `BP_PaldarkCharacter` → drop `UPaldarkSquadMembershipComponent` + `USquadCommandSlotComponent`. Set `SquadTag=Paldark.Squad.Default` + `CommandSetSoft=DA_SquadCommandSet_Default`.
- [ ] **Pal squad listener.** `BP_PaldarkPalCharacter` `CandidateActivities[]` += `UPaldarkActivity_FollowSquadCommand` (priority 22, between Follow 20 and Stalk 25).

### 0.5 Match extraction (W24-25)

- [ ] **Swap experience GameMode override (CRITICAL).** Open `PX_RaidSandbox` (Experience DA from Q1). Set `GameModeClass = APaldarkGameMode_Extraction` (NOT `APaldarkGameModeBase`). Without this swap, match stays in Warmup forever.
- [ ] **BP_ExtractionBeacon.** Content Browser → `/Game/Paldark/Match/` → New → Blueprint Class → parent `APaldarkExtractionBeacon`. Defaults:
  - `TriggerRadius=400`, `ExtractionDuration=5`, `bRequireExplicitConsent=false`, `bDecayOnLeave=true`.
  - Add static mesh component (e.g. radio pylon) bên trên USphereComponent.
- [ ] **Place 2 beacon** trong `Raid_Sandbox` (2 corner đối diện) for designer pacing.
- [ ] **Optional: explicit consent test beacon.** Drop 1 thêm beacon `bRequireExplicitConsent=true` + bind IA_InteractExtraction (E) trong IMC_Default → `Paldark.InputTag.InteractExtraction`.

✅ Sau bước này Content folder phải có thêm (so với Q1): `BP_PaldarkCharacter_Direhound`, `BP_PaldarkCharacter_Razorbird`, `BP_PalSpawner` (4× placed), `DA_SquadCommandSet_Default`, `IMC_Squad`, 2-3× `BP_ExtractionBeacon` (placed), `BP_GA_PalAttack`. Plus experience `PX_RaidSandbox` đã swap GameMode override.

---

## 1. 4-player listen-server connect (P06 + P15)

Goal: 4 client connect listen server + 4 spawn + roster broadcast.

```
Editor PIE settings → Number of Players = 4 + Net Mode = Play As Listen Server
```

- [ ] PIE 4 viewport mở `Raid_Sandbox` không crash. 4 player spawn ở 4 PlayerStart (designer phải drop ≥ 4 PlayerStart trong map).
- [ ] Server console: `Paldark.Net.DumpRoster` → log `players=4 controllers=[PC_0, PC_1, PC_2, PC_3]`.
- [ ] Server: `Paldark.Match.Dump` → `Phase=Warmup, Players=4, Alive=4, Extracted=0, KIA=0`.
- [ ] **Time sync.** Server: `Paldark.Net.DumpTimeSync` → `clients=4 rtt_avg=<10ms (PIE local) clock_skew_ms<50`.
- [ ] **HUD log smoke.** Mỗi client console: `Paldark.Experience.Current` → `experience=PaldarkExperience:PX_RaidSandbox`.

📸 Screenshot 1: 4-quadrant PIE viewport với 4 player spread.

---

## 2. Warmup → Active (W14-15 + W24-25)

Goal: warmup timer expire 30s → auto Active phase.

- [ ] Đợi 30s (default `WarmupDuration`). Server: `Paldark.Match.Dump` → `Phase=Active`.
- [ ] **Skip warmup (optional).** Server: `Paldark.Match.Phase Active` → instant transition. `Paldark.Match.Dump` → `Phase=Active`.
- [ ] **Warmup re-entry rejected.** Server: `Paldark.Match.Phase Warmup` → log warning `Paldark.Match.Phase — Warmup re-entry not supported (FSM is monotonic)`.

📸 Screenshot 2: Server console với 2 `Match.Dump` (Warmup → Active) + warmup warning.

---

## 3. Pal companion combat (W18-19)

Goal: 1 companion auto-defend khi hostile dummy approach + ability fire + perception hysteresis.

> Trên client 0 (host PC_0).

- [ ] Server: `Paldark.Pal.SpawnTestCompanion` → 1 friendly Pal spawn cách player `-300 cm`. Activity=Idle/Follow.
- [ ] Server: `Paldark.Pal.SpawnHostileDummy 800` → 1 hostile Pal spawn cách player 8m, team `Hostile`.
- [ ] **Threat detection.** Server: `Paldark.Pal.DumpThreat` → log `pal=BP_PaldarkPalCharacter_C_0 perception_radius=1500 threats=[BP_HostileDummy_C_0 distance=800] aggro_state=Engaged`.
- [ ] **Activity switch.** Server: `Paldark.Pal.CurrentActivity` → `activity=UPaldarkActivity_Combat tag=Paldark.Pal.Activity.Combat candidates=4 utility_score=0.78`.
- [ ] **Combat fire.** Pal nên rotate facing hostile + apply `GA_PalAttack` mỗi 1.5s. Server log `UPaldarkGameplayAbility_PalAttack — target=BP_HostileDummy_C_0 damage=15`.
- [ ] **Dummy down.** Sau ~9 hits, dummy Health=0, despawn. Pal grace 3s → back về Follow.
- [ ] **Aggro hysteresis check.** Spawn 2 hostile: `Paldark.Pal.SpawnHostileDummy 600` + `Paldark.Pal.SpawnHostileDummy 900`. Walk slow để delta < 200cm — Pal **không** switch target. Walk để delta ≥ 250cm — Pal switch.
- [ ] **Force combat (manual override).** `Paldark.Pal.ForceCombat <pal_index> <target_index>` → Pal target chỉ định.

📸 Screenshot 3: PIE viewport với 1 friendly Pal đang fire vào 1 hostile dummy + on-screen debug magenta line.

---

## 4. Hostile-Pal AI (W20-21)

Goal: Direhound pack aggros + broadcast tới pack member + Razorbird aerial chase.

- [ ] Server: `Paldark.Pal.SpawnDirehoundPack 3 1500` → 3 Direhound spawn cách player 15m, formation pack, link với pack subsystem.
- [ ] Server: `Paldark.Pal.DumpPackState` → log `packs=1 entries=[DirehoundPack_0=[D0,D1,D2]]`.
- [ ] Walk lên D0 đến trong perception radius (1500cm). D0 switch Combat → pack subsystem broadcast → D1 + D2 cũng switch Combat (2000cm aggro broadcast radius).
- [ ] **Pal Patrol activity.** Trước khi aggro, server: `Paldark.Pal.CurrentActivity` ở từng Direhound → `activity=UPaldarkActivity_Patrol candidates=3` (Patrol P15 < Stalk P25 < Combat P40).
- [ ] **Razorbird aerial.** Server: `Paldark.Pal.SpawnRazorbird 1000` → 1 Razorbird spawn z=500 (high altitude). Movement mode `Flying` — không pathfind navmesh, drive thẳng `AddMovementInput`.
- [ ] **Razorbird chase.** Walk lên — Razorbird dive xuống attack từ trên (z giảm dần). `Paldark.Pal.CurrentActivity` → `Combat`.
- [ ] **Pack despawn after kill.** Player kill 3 Direhound. Pack subsystem prune entry. `Paldark.Pal.DumpPackState` → `packs=1 entries=[DirehoundPack_0=[]]`.

📸 Screenshot 4: PIE viewport với 3 Direhound formation + 1 Razorbird overhead.

---

## 5. Squad coordination (W22-23)

Goal: 4-player squad + ping replicate + radial command + Pal squad listener.

- [ ] **Squad roster.** Server: `Paldark.Squad.Dump` → log `squads=[Default={PC_0,PC_1,PC_2,PC_3}] standing_commands={Default=None} active_pings=0`.
- [ ] **Ping Spot.** Client 0: `Paldark.Squad.Ping Spot` hoặc T → all 4 client viewport thấy ping marker (3s lifetime) ở world position 6m forward.
- [ ] **Ping Enemy (mark under crosshair).** Client 1: aim vào Direhound + V (IA_PingEnemy) hoặc `Paldark.Squad.Ping Enemy` → marker attach tới Direhound (follow movement). 3s expire.
- [ ] **Standing command — Stay.** Client 0: `Paldark.Squad.Command Stay` → companion Pal của tất cả player switch FollowSquadCommand (P22) override Follow (P20). Pal đứng nguyên.
- [ ] **Standing command — Follow.** Client 0: `Paldark.Squad.Command Follow` → cancel Stay, companion re-leash.
- [ ] **Squad command — Attack.** Client 0: ping enemy + `Paldark.Squad.Command Attack` → companion Pal route Attack ping target. Combat activity (P40) preempt FollowSquadCommand (P22).
- [ ] **Per-squad delivery filter.** (Manual test) 2 client cùng squad nhận ping, 2 client ngoài squad không nhận. (Q2 chỉ có 1 squad default, defer multi-squad sang Q3.)
- [ ] **Ping rate limit.** Client 0: spam T 10× nhanh → log warning `Paldark.Squad.Ping — rate limited (0.5s per issuer)`.

📸 Screenshot 5: 4-quadrant PIE viewport với ping marker visible across all 4.

---

## 6. Extraction flow + match end (W24-25)

Goal: Walk lên beacon → 5s ramp → all extract → match Ended với AllExtracted.

- [ ] **Beacon idle check.** Server: `Paldark.Match.Dump` → `Phase=Active`. Walk 1 player lên `BP_ExtractionBeacon` #1. Wait 5s (default `ExtractionDuration`). 
- [ ] **Ramp progress visible.** Server: `Paldark.Match.Dump` mid-ramp (tại ~3s) → `Players=4 ExtractionProgress={PC_0=0.6}`.
- [ ] **Extraction land.** Sau 5s: server log `UPaldarkMatchSubsystem::RecordExtraction — player=PC_0 outcome=Extracted`. `Paldark.Match.Dump` → `Phase=Extracting Outcome={PC_0=Extracted}`.
- [ ] **Death routes KIA.** Player 1 walk vào Direhound pack, bị kill (Health → 0). Death hook fire: `UPaldarkAttributeSet — Health hit 0, broadcasting OnHealthZeroed` → `UPaldarkMatchSubsystem::RecordDeath — player=PC_1 outcome=KIA`.
- [ ] **Decay test.** Player 2 lên beacon → ramp tới progress=0.5 → walk ra. Wait 2.5s — progress decay về 0 (default `bDecayOnLeave=true`).
- [ ] **All extract.** Player 2 + Player 3 cùng walk lên beacon, hold 5s. Cả 2 extract. Server log `UPaldarkMatchSubsystem::EvaluateEndCondition — reason=AllExtracted` → `SetPhase Ended`.
- [ ] **Match end broadcast.** All 4 client GameState `OnMatchPhaseReplicated(Ended)` + PlayerState `OnMatchOutcomeChanged(Extracted/KIA)` fire. (UI defer — chỉ log).
- [ ] **Post-end dump.** Server: `Paldark.Match.Dump` → `Phase=Ended Reason=AllExtracted Players=4 Extracted=3 KIA=1`.
- [ ] **Re-record (force end variant).** Reload level. Repeat steps 1-2 to Active. Server: `Paldark.Match.ForceEnd TeamWipe` → match Ended với reason `TeamWipe`. Verify all 4 player Outcome flips terminal (Alive → KIA force).

📸 Screenshot 6: 4-quadrant PIE viewport với 3 Extracted + 1 KIA + server log `Phase=Ended`.

---

## 7. Lag compensation in combat (W16-17)

Goal: Verify rewind path không lagless-shoot khi ping 80ms.

- [ ] Editor → Network Emulation → Latency 80ms (RTT 160ms).
- [ ] Restart PIE listen+1 client. Spawn dummy: `Paldark.Combat.SpawnDummy 1500`.
- [ ] Client 1: aim torso + LMB. Server log:
  - `UPaldarkGameplayAbility_HitscanFire — fire requested, server validating`.
  - `UPaldarkLagCompensationComponent::RewindToServerTime rewound=Yes shot_time=… server_time=… delta=80ms`.
  - `UPaldarkGameplayAbility_HitscanFire — hit=BP_DummyTarget_Default_C_0 bone=spine_01 damage=25`.
- [ ] **Visual check.** Đối thủ moving target test: spawn 1 friendly Pal patrol — client shoot. Rewound hit phải land trên *vị trí cũ* (client-time), không phải vị trí hiện tại (server-time).
- [ ] **No rewind needed (host).** Server LMB → log `rewound=No (local authority)`.

📸 Screenshot 7: PIE log hiển thị `RewindToServerTime delta=80ms rewound=Yes`.

---

## 8. Dedicated server smoke test (optional — W14-15)

Goal: PaldarkServer.exe + 4 PaldarkClient.exe end-to-end ngoài Editor.

- [ ] Terminal 1: `PaldarkServer.exe Raid_Sandbox -log -port=7777 -nullrhi`. Log `LogPaldarkNet — NetSubsystem initialized` + `Match phase=Warmup players=0`.
- [ ] Terminal 2-5: `PaldarkClient.exe 127.0.0.1:7777` (mỗi terminal 1 client). Server log mỗi client `LogPaldarkNet — PostLogin player=… roster=[…]`.
- [ ] Server console (`PaldarkServer.exe` terminal): `Paldark.Match.Dump` → `Phase=Warmup Players=4`.
- [ ] Sau 30s warmup → `Phase=Active`. Walk + ping + extract loop tương tự bước 1-6.
- [ ] **Disconnect mid-match.** Terminal 5 Ctrl+C → server log `PostLogout player=… outcome=Disconnected`.

> Skip nếu chưa có dedicated server build — listen+3 PIE đủ smoke.

📸 Screenshot 8: PaldarkServer.exe terminal với 4 client connected + `Match.Dump`.

---

## 9. Bug bash (1 ngày — optional cho Q2)

Đọc [`Q2-Bug-Bash.md`](Q2-Bug-Bash.md), chọn ≤ 5 issue ưu tiên cao (`P0`/`P1`) đóng trong ngày. Issue còn lại defer sang Q3 task list.

---

## 10. Sign-off

- [ ] ✅ Tất cả 8 section trên (Connect / Warmup→Active / Pal Combat / Hostile AI / Squad / Extraction / Lag Comp / Dedicated [optional]) tick xong.
- [ ] ✅ 8 screenshot saved vào `Documents/Devlog/Q2-Playtest-Screens/` (folder tự tạo nếu chưa có).
- [ ] ✅ Video 60s record (OBS hoặc UE Editor record button, 4-quadrant nếu split-screen support) → upload → link trong `Documents/Devlog/Q2-Playtest-Video.md`.
- [ ] ✅ `Q2-Bug-Bash.md` review, mark `P0`/`P1` đã fix.
- [ ] ✅ Commit `Documents/Devlog/Q2-Milestone-Build.md` cập nhật `Pillar coverage actual %` với số sau playtest thực.

**Q2 milestone đóng** khi 5 ✅ trên cả tick. Q3 bắt đầu W27 — AssetManager async load + Game Feature plugin (roadmap § Tuần 27-28).

---

## 11. References

- [`PaldarkLab/README.md`](../../PaldarkLab/README.md) — full feature authoring + test loop (1896 dòng).
- [`Documents/Devlog/Q2-Milestone-Build.md`](Q2-Milestone-Build.md) — Q2 status summary, pillar coverage, gap analysis.
- [`Documents/Devlog/Q2-Bug-Bash.md`](Q2-Bug-Bash.md) — 20-issue priority list cho 1-day bug bash.
- [`Documents/Devlog/Q1-Playtest-Checklist.md`](Q1-Playtest-Checklist.md) — Q1 baseline checklist (this Q2 file inherits + extends).
- [`Documents/PALDARK/03-Roadmap_1_Year.md`](../PALDARK/03-Roadmap_1_Year.md) § Tuần 26 + Q2 outcome.
- Per-week build logs: [W14-15](W14-15-Build.md), [W16-17](W16-17-Build.md), [W18-19](W18-19-Build.md), [W20-21](W20-21-Build.md), [W22-23](W22-23-Build.md), [W24-25](W24-25-Build.md).
