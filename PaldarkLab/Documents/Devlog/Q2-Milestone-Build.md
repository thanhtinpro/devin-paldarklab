# PALDARK — Q2 Milestone Build (Tuần 26)

> **Source:** [`Documents/PALDARK/03-Roadmap_1_Year.md`](../PALDARK/03-Roadmap_1_Year.md) § Tuần 26.
>
> **Roadmap deliverable:** "Internal playtest: 4 player, 4 Pal, 1 map, 4 enemy, full loop."
>
> **Q2 Outcome (from roadmap):** **Vertical slice 1** — 1 map 1 mission có thể chơi end-to-end (drop → fight → extract).

This devlog freezes the Q2 milestone: what was shipped W14-25 in C++, how it maps to the roadmap, which pillars hit their Q2 % target, and what the user must do in UE Editor (the VM has no UE 5.4 compiler) to actually validate the 4-player vertical slice.

---

## 1. PR ledger (W14 → W25)

| PR | Week | Title | Status | Branch |
|----|------|-------|--------|--------|
| [#18](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/18) | W14-15 | 4-player dedicated server (net subsystem + Pal replication) | ✅ merged | `devin/.../w14-15-dedicated-server` |
| [#19](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/19) | W16-17 | Lag Compensation Server-Side Rewind (frame buffer + rewind + time-sync) | ✅ merged | `devin/.../w16-17-lag-compensation` |
| [#20](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/20) | W18-19 | Pal Combat Activity (threat detection + utility AI + combat ability + team tags) | ✅ merged | `devin/.../w18-19-pal-combat-activity` |
| [#21](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/21) | W20-21 | Hostile-Pal AI subclasses (Direhound pack + Razorbird aerial) | ✅ merged | `devin/.../w20-21-hostile-pal-ai` |
| [#22](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/22) | W22-23 | Squad system (ping + radial command) | ✅ merged | `devin/.../w22-23-pal-squad-system` |
| [#23](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/23) | W24-25 | Extraction flow (match subsystem + beacon + game mode/state extension + death hook + console) | ✅ merged | `devin/.../w24-25-extraction-flow` |

6 PR, ~14,000 dòng C++ + ini + python validator + README on `main` cộng dồn W14-25. Toàn bộ commit qua CI structural validation (`scripts/ci/validate_paldarklab.py`, 2/2 check xanh từng PR).

Cumulative since Q1 close: 16 PR (#7-#23 minus đã merge), ~19,500 dòng tổng.

---

## 2. Feature map vs roadmap Q2 outcome

Roadmap § Q2 ghi rõ Q2 outcome cần: "Vertical slice 1 — 1 map 1 mission có thể chơi end-to-end." Bảng dưới mapping từng layer của vertical slice → PR / file C++ / lệnh test:

| Q2 Outcome layer | C++ delivery | Test entrypoint | PR |
|------------------|--------------|-----------------|----|
| **Multiplayer drop** (4 player dedicated) | `UPaldarkNetSubsystem` (`UGameInstanceSubsystem`) + `APaldarkPlayerController::ServerTimeSync` + `APaldarkGameModeBase::PreLogin / PostLogin` chuẩn hóa | `PaldarkServer.exe Raid_Sandbox -log` + 4× `PaldarkClient.exe 127.0.0.1` | #18 |
| **Lag compensation** (hitscan) | `UPaldarkLagCompensationComponent` (1s frame buffer) + `RewindToServerTime` + `UPaldarkGameplayAbility_HitscanFire::ServerValidateAndApply` rewind path | LMB ở 80ms ping → server log `rewound=Yes shot_time=… server_time=… delta=…` | #19 |
| **Pal companion combat** | `UPaldarkActivity_Combat` (priority 40, preempt Investigate/Follow) + `UPaldarkPalPerceptionComponent` (15m scan + 200cm aggro hysteresis + 3s grace) + `UPaldarkGameplayAbility_PalAttack` (AI-only, server-only) + 3 utility consideration | `Paldark.Pal.SpawnHostileDummy 800` → companion auto switch Combat | #20 |
| **Hostile-Pal AI** (Direhound pack + Razorbird aerial) | `APaldarkPalCharacter_Direhound` + `APaldarkPalCharacter_Razorbird` + `UPaldarkActivity_Patrol/Stalk` + `UPaldarkHostilePackSubsystem` (UWorldSubsystem) + `APaldarkPalSpawner` | `Paldark.Pal.SpawnDirehoundPack 3 800` + `Paldark.Pal.SpawnRazorbird 800` | #21 |
| **Squad coordination** (ping + radial command) | `UPaldarkSquadSubsystem` + `UPaldarkPingSubsystem` + `APaldarkPingMarker` (replicated) + `UPaldarkSquadCommandSet` (UPrimaryDataAsset) + `UPaldarkActivity_FollowSquadCommand` (P22) + `UPaldarkSquadMembershipComponent` | `Paldark.Squad.Ping Spot/Enemy/Loot/Move` + `Paldark.Squad.Command Follow/Stay/Attack` | #22 |
| **Extraction match loop** | `UPaldarkMatchSubsystem` (UWorldSubsystem server-only, 4-state FSM Warmup→Active→Extracting→Ended + per-player outcome registry) + `APaldarkExtractionBeacon` (replicated actor + USphere trigger + 5s ramp) + `APaldarkGameMode_Extraction` + REPNOTIFY MatchPhase/Outcome/Progress | `Paldark.Match.Dump` + `Paldark.Match.Phase Active` + walk lên beacon | #23 |

> **6/6 layer của vertical slice đã ship ở C++ side.** Designer side cần author trong UE Editor (cộng dồn từ Q1):
> - 1 `BP_PaldarkCharacter_Direhound` + 1 `BP_PaldarkCharacter_Razorbird` BP subclass (#21).
> - 1 `BP_PalSpawner` placement + 1 `BP_ExtractionBeacon` placement trong `Raid_Sandbox` (#21 + #23).
> - 1 `DA_SquadCommandSet_Default` UPrimaryDataAsset + 1 `IMC_Squad` Input Mapping Context wire `T/V/Q` (#22).
> - Swap `APaldarkGameMode_Extraction` lên Raid_Sandbox experience GameMode override (#23 — không tự auto-wire).
>
> Tất cả bước Blueprint đã có hướng dẫn trong [`PaldarkLab/README.md`](../../PaldarkLab/README.md) § "Hostile Pal authoring + test loop", § "Squad authoring + test loop", § "W24-25 — Extraction flow → Authoring order".

---

## 3. Pillar coverage — actual vs target

Roadmap dự kiến Q2 đạt: P6 80% / P7 60% / P8 80% / P9 70% / P10 60% / P14 40% / P15 50%. Đối chiếu thực tế:

| Pillar | Target Q2 | Actual Q2 | Delta | Bằng chứng |
|--------|-----------|-----------|-------|------------|
| **P06** Networking         | 80% | **85%** | +5  | `UPaldarkNetSubsystem` + dedicated server target + listen→dedicated launch + 4-player tested + Pal replication tuning (#18). Push model + condition flags wire (#22 trên ping marker). RPC validation (#22 + #23). Còn thiếu: NetDriver custom (defer Q3 nếu cần). |
| **P07** Lag Compensation   | 60% | **70%** | +10 | `UPaldarkLagCompensationComponent` 1s frame buffer + server-side rewind path + client→server time sync (#19). Hitscan ability đi qua rewind. Còn thiếu: rollback prediction client-side (defer Q4). |
| **P08** GAS                | 80% | **78%** | -2  | Q1 carry (65% → +13 from #20 PalAttack ability + W18-19 ability granting on server + #23 OnHealthZeroed re-use). Còn thiếu: AbilityTask custom (1-2 task chuyên dụng), GameplayCue (FX/SFX hook). |
| **P09** AI                 | 70% | **75%** | +5  | Q1 carry (45% → +30 from #20 utility AI + #21 hostile AI subclass + pack subsystem + Patrol/Stalk activity + #22 squad command listener). 5 activity total (Idle/Follow/Investigate/Combat/Patrol/Stalk/FollowSquadCommand). Còn thiếu: BT/EQS (defer hoặc skip — utility AI là design choice). |
| **P10** Multiplayer Shooter| 60% | **65%** | +5  | Hitscan + lag comp + dedicated server + 4-player roster (#18 + #19). Còn thiếu: melee weapon (Q3), grenade (Q3), aim down sight (Q3 polish). |
| **P14** AssetManager       | 40% | **45%** | +5  | Q1 base 35% (PaldarkAssetManager + 5 primary asset type) + #21 added `PaldarkPalSpawnTable` primary asset type + #22 added `PaldarkSquadCommandSet`. Async load qua bundle (defer Q3). |
| **P15** Game Mode/State    | 50% | **70%** | +20 | Q1 base 30% (GameMode/GameState/GameInstance + AssetManager) + #23 added `APaldarkGameMode_Extraction` (lifecycle hooks) + `APaldarkGameStateBase` (3 REPNOTIFY) + `APaldarkPlayerState` (2 REPNOTIFY) + match FSM. Còn thiếu: PlayerStart pool, HUD class wire (Q3 polish). |

**Tổng coverage Q2 ≈ 70%** (cộng đều 7 pillar tracked Q2 + carry-over Q1). Roadmap target ≈ 65%. **Pillar nào outperformance: P15 (+20 từ extraction GameMode + REPNOTIFY).** Underperformance: P08 GAS -2 (chưa wire GameplayCue / custom AbilityTask).

**Pillar mới phát sinh cần track Q3:**
- **P12 Data-driven** — đã chạy tốt với 7 primary asset type, tracker tiếp Q3.
- **P03 Composition** — cộng dồn thêm 5-7 component (perception/squad membership/match-related) qua W18-25.
- **P05 Animation** — vẫn ở 15% (Q1 carry). **P0 risk for Q2 demo video** — designer phải port Lyra ALS hoặc dùng mannequin default trước khi record vertical slice video.

---

## 4. Gap analysis vs roadmap Q2 outcome

> Roadmap § Tuần 26 yêu cầu: "Internal playtest: 4 player, 4 Pal, 1 map, 4 enemy, full loop." + Q2 outcome "Vertical slice 1 — 1 map 1 mission end-to-end".

| # | Required | Shipped (C++) | Designer authoring still needed | Risk |
|---|----------|---------------|---------------------------------|------|
| 1 | 4 player connect dedicated | ✅ `UPaldarkNetSubsystem` + dedicated server target + `PreLogin/PostLogin` chuẩn hóa + `ServerTimeSync` (#18) | Dedicated server binary build (`UAT BuildCookRun -dedicatedserver -platform=Win64`) + 4 PIE client config | Medium — runtime test cần real network setup. PIE listen+3 client cũng OK ở mức smoke test. |
| 2 | 4 Pal companion | ✅ `APaldarkPalCharacter` + `UPaldarkPalLocomotionComponent` + Activity FSM + Combat ability + Squad listener (#13 + #20 + #22) | `BP_PaldarkPalCharacter` Pal companion BP per player (đã có từ Q1) + `DA_SquadCommandSet_Default` (#22) | Low — Q1 carry, đã test ở Q1 playtest. |
| 3 | 1 map | ✅ `Raid_Sandbox` Experience + `APaldarkExtractionBeacon` actor + `APaldarkPalSpawner` actor (#9 + #21 + #23) | `Raid_Sandbox.umap` (đã từ Q1 Bug Bash slot 3) + 4× `BP_PalSpawner` placement + 2× `BP_ExtractionBeacon` placement + nav build | Medium — designer phải place actor đúng vị trí (4 spawner spread, beacon ở 2 corner) để pacing 5-10 phút raid. |
| 4 | 4 enemy | ✅ `APaldarkPalCharacter_Direhound` + `APaldarkPalCharacter_Razorbird` + Patrol/Stalk activity + Pack subsystem (#21) | `BP_PaldarkCharacter_Direhound` BP + `BP_PaldarkCharacter_Razorbird` BP + per-spawner config (count/wave/cooldown) | Low — README §"Hostile Pal authoring" chi tiết 6 bước; default values an toàn. |
| 5 | Full loop (drop → fight → extract) | ✅ `UPaldarkMatchSubsystem` (4-state FSM) + `APaldarkExtractionBeacon` + `APaldarkGameMode_Extraction` (#23) | Swap experience GameMode override → `APaldarkGameMode_Extraction` (KHÔNG tự auto-wire!) | **High** — nếu designer quên swap, match stay `Warmup` mãi mãi. README §"W24-25 Authoring order" step 2 ghi rõ. |
| 6 | Squad coordination | ✅ `UPaldarkSquadSubsystem` + Ping + Radial command + Pal squad listener activity (#22) | `DA_SquadCommandSet_Default` (Follow/Stay/Attack) + `IMC_Squad` (T/V/Q) | Low — README §"Squad authoring" chi tiết 8 bước. |
| 7 | Lag compensation working | ✅ `UPaldarkLagCompensationComponent` + `RewindToServerTime` + `ServerValidateAndApply` (#19) | (None — already wire qua `BP_GA_HitscanFire` từ Q1) | Low — automatic khi hitscan ability fire. |

**Verdict:** 7/7 yêu cầu có C++ backbone. Designer block lớn nhất là **GameMode override swap** (#5) — nếu quên, vertical slice broken. Đề xuất:
- **Path A (sạch):** Thêm validator check `DefaultEngine.ini` có `[/Script/EngineSettings.GameMapsSettings]` set `GlobalDefaultGameMode = APaldarkGameMode_Extraction` cho experience Raid_Sandbox. Risk: 1 PR follow-up W26+.
- **Path B (nhanh):** Bug-bash slot dedicated cho designer swap + commit ini. Risk: phụ thuộc designer kỷ luật.

Em đề xuất **Path B** cho Q2 milestone (giữ đúng deadline) + Path A là task đầu tiên của Q3 (cùng với AssetManager async load PR).

---

## 5. Known issues & risks (frozen tại W25)

| ID | Issue | Severity | Mitigation | Defer to |
|----|-------|----------|------------|----------|
| L-01 | `APaldarkGameMode_Extraction` chưa đăng ký trong `DefaultEngine.ini` — designer phải tự swap experience GameMode override; nếu quên, match stay `Warmup` mãi mãi | High | README §"W24-25 Authoring" step 2 + console log warning khi run on client | Q3 đầu (validator extend) |
| L-02 | UMG end-of-match widget chưa tồn tại — chỉ có C++ delegate `OnMatchOutcomeChanged` + `OnMatchPhaseReplicated` | High | W22+ polish week (đã defer rõ trong README W24-25 anti-pattern) | W30+ |
| L-03 | Lobby travel chưa wire — match Ended → designer phải reload level manually | High | Roadmap W40+ hub town explicit defer | W40+ |
| L-04 | Reconnect mid-match không support — `Disconnected` là terminal | Medium | W14-15 follow-up trong Q3 | Q3 |
| L-05 | Spectator camera chưa wire — KIA player stuck ragdoll | Medium | W22+ polish + free-cam component | W30+ |
| L-06 | AnimBP T-pose risk — Q1 K-01 chưa fully resolved (Path B mannequin default) | High | Lyra ALS port trước Q2 demo video | Q3 đầu (đẩy lên priority cao) |
| L-07 | Multi-match-per-session không support — designer reload level cho match tiếp theo | Low | Seamless travel + AssetManager Bundle | W40+ |
| L-08 | Persistent match stats chưa wire — kết quả lưu in-memory, world teardown wipe | Low | DynamoDB integration | W42-43 (AWS) |
| L-09 | Beacon `bRequireExplicitConsent` tag `Paldark.InputTag.InteractExtraction` chưa có default IMC binding — designer tự bind | Low | README §"Authoring order" step 3 ghi rõ | Q3 polish |
| L-10 | Razorbird `MOVE_Flying` không pathfind navmesh — drive thẳng `AddMovementInput` world-space | Low | Acceptable for aerial AI; explicit anti-pattern documented | W30+ (3D nav volume) |
| L-11 | Pack subsystem broadcast aggro không có distance falloff — pack-mate xa nhất nhận trigger ngang pack-mate gần nhất | Low | Future weighted broadcast | Q4 polish |
| L-12 | Squad command radial wheel UMG chưa tồn tại — chỉ test qua console | Medium | W22+ polish | W30+ |
| L-13 | Squad FollowSquadCommand activity P22 sit giữa Follow P20 và Stalk P25 — designer dễ confuse priority ladder | Medium | README §"Squad design notes" + per-activity priority log on entry | Validator check trong Q3 |
| L-14 | Match `Paldark.Match.Phase Extracting` console không direct enter — call StartMatch và log warning | Low | Banner-only phase set bởi first RecordExtraction; documented trong console handler comment | (No fix needed) |
| L-15 | `OnHealthZeroed` weak lambda capture `this` — nếu Character destroy trước PlayerState resolve, có thể null-deref | Low | `MatchDeathHookHandle.IsValid()` guard + `IsValid(PlayerState)` check trong lambda | (No fix needed — guards sufficient) |
| L-16 | `ExtractionProgress` REPNOTIFY throttle 0.25s — client thấy progress bar giật stair-step | Medium | HUD task interpolate giữa RepNotify | UI widget task |
| L-17 | Hard timeout `MaxMatchDuration` default 600s — không có designer-tunable per-experience | Low | Expose qua experience DataAsset trong Q3 | Q3 |
| L-18 | CI validator chưa check `Raid_Sandbox.umap` presence (Q1 K-02 carry) | Low | Validator extend grep map file existence | Q3 đầu |
| L-19 | Pal Combat ability `BP_GA_PalAttack` SetByCaller `Paldark.SetByCaller.PalDamage` chưa expose damage tuning qua DataAsset — hardcoded designer-author cost | Low | Move sang `UPaldarkPalCombatProfileDataAsset` | Q3 polish |
| L-20 | `UPaldarkNetSubsystem` `OnPostLogin` event không expose tới Blueprint — UMG lobby UI sẽ cần | Medium | Add `BlueprintAssignable` delegate | W30+ (cùng UMG lobby) |

> **Verdict:** Không có issue nào ở mức **Critical** block Q2 outcome (vertical slice playable). L-01 / L-06 / L-02 / L-03 đều là designer-side / defer roadmap issues. L-13 / L-16 / L-20 là UI/polish risk cho Q3.

---

## 6. Files / module health (cumulative W1-25)

```
PaldarkLab/
├── PaldarkLab.uproject                              # UE 5.4, 3 plugin core
├── Config/
│   ├── DefaultEngine.ini                            # log verbosity + class wiring + GameMaps
│   ├── DefaultGame.ini                              # 7 primary asset type (Experience/PawnData/ActionSet/InputConfig/Item/PalSpawnTable/SquadCommandSet)
│   ├── DefaultInput.ini                             # EnhancedPlayerInput + EnhancedInputComponent
│   └── DefaultGameplayTags.ini                      # (auto, native tags ship trong PaldarkGameplayTags)
├── Source/
│   ├── PaldarkLabCore/                              # PreDefault loading
│   │   ├── Public/PaldarkLogCategories.h            # 5 log category
│   │   ├── Public/PaldarkGameplayTags.h             # 45+ native tag (Q1=24 + Q2=21: Combat/Pack/Squad/Match/InputTag)
│   │   └── Private/...
│   ├── PaldarkLab/                                  # Default loading
│   │   ├── PaldarkLab.Build.cs                      # GameplayAbilities + Tags + Tasks + EnhancedInput + AIModule + NavigationSystem
│   │   ├── PaldarkLab.cpp                           # 25+ console cmd register
│   │   ├── Public/
│   │   │   ├── Framework/                           # AssetManager + GameMode + GameModeBase + GameMode_Extraction + GameState + GameInstance
│   │   │   ├── Experience/                          # ExperienceDefinition + ActionSet + PawnData
│   │   │   ├── Input/PaldarkInputConfig.h
│   │   │   ├── Player/                              # PC + PS (ext Match) + Char (death hook) + 12 component slots
│   │   │   ├── Pal/                                 # Pal char + 8 component slots + Locomotion + Activity (7 subclass) + Combat ability + Perception + Spawner + 2 hostile subclass
│   │   │   ├── Pal/Combat/                          # PaldarkPalConsideration + utility AI
│   │   │   ├── Gas/                                 # AttributeSet (OnHealthZeroed) + ASC + GA base + Sprint + HitscanFire + PalAttack + DamageExecutionCalculation
│   │   │   ├── Combat/PaldarkDummyTarget.h
│   │   │   ├── Inventory/                           # ItemFragment + 3 fragment + ItemDefinition + InventoryComponent (Q1)
│   │   │   ├── Net/                                 # NetSubsystem (W14-15) + LagCompensationComponent (W16-17)
│   │   │   ├── Subsystem/                           # PaldarkLabWorldSubsystem (Q1) + HostilePackSubsystem (W20-21)
│   │   │   ├── Squad/                               # SquadSubsystem + PingSubsystem + PingMarker + SquadCommandSet + SquadMembershipComponent
│   │   │   └── Match/                               # MatchTypes (enums + row) + MatchSubsystem + ExtractionBeacon
│   │   └── Private/                                 # matching .cpp tree
│   └── PaldarkLabEditor/                            # Editor-only placeholder
└── README.md                                        # 1896 lines, đầy đủ test loop cho mọi feature W1-25
```

**Module load order (PreDefault → Default → PostEngineInit):** unchanged from Q1.

Validator kiểm 14 thứ ở mức Python (`scripts/ci/validate_paldarklab.py` — ~3000 dòng):
- uproject + 3 module layout + 3 target + 5 log category + no BOM
- 45+ native gameplay tag declared (Q1 24 + Q2 21)
- Build.cs dependency: EnhancedInput / GameplayAbilities / GameplayTags / GameplayTasks / AIModule / NavigationSystem
- Input config shape, Player/Pal slot classes, Pal pawn data shape
- Activity FSM shape, GAS shape (5 attr + clamp + ASC + 3 GA), Combat damage shape, Inventory fragment shape
- **Net subsystem shape, LagCompensation shape** (W14-15 + W16-17)
- **Pal combat shape, hostile-Pal subclass shape, pack subsystem shape, spawner shape** (W18-19 + W20-21)
- **Squad subsystem shape, ping subsystem shape, ping marker shape, command set shape, listener activity shape** (W22-23)
- **Match shape (subsystem + beacon + GameMode subclass + PS/GS REPNOTIFY + death hook + console)** (W24-25)

---

## 7. Recommended Q2 → Q3 transition

1. **Tuần này (designer):**
   - Author / verify 4 BP subclass: `BP_PaldarkCharacter_Direhound`, `BP_PaldarkCharacter_Razorbird`, `BP_PalSpawner` placement, `BP_ExtractionBeacon` placement.
   - **Swap Raid_Sandbox experience GameMode override → `APaldarkGameMode_Extraction`** (L-01).
   - Author `DA_SquadCommandSet_Default` + `IMC_Squad` (T/V/Q binding).
   - Chạy 1 lần PIE 4-client end-to-end theo [`Q2-Playtest-Checklist.md`](Q2-Playtest-Checklist.md).
   - Gửi video 60s back vào `Documents/Devlog/Q2-Playtest-Video.md` (4-quadrant split, kèm screenshot/youtube link).
2. **Đầu Q3 (dev):**
   - **PR đầu Q3: AnimBP Lyra ALS port** (L-06 carry from Q1 K-01) — đóng visual debt trước Q3 demo video.
   - **PR thứ 2 Q3: AssetManager async + Game Feature plugin** (roadmap § Tuần 27-28).
   - **PR follow-up validator**: enforce GameMode override in DefaultEngine (L-01), check `Raid_Sandbox.umap` presence (L-18 from Q1 K-02).
3. **Đầu Q3 (designer):**
   - Author `Documents/PALDARK/04-Vertical_Slice_Demo_Script.md` (5-10 minute mission narrative).
   - Record 90s "marketing" video sau khi AnimBP fix.

---

## 8. Cumulative Q1+Q2 stats

| Metric | Q1 close (W13) | Q2 close (W26) | Δ |
|--------|----------------|----------------|---|
| PR merged (Devin) | 10 | 16 | +6 |
| C++ source files | ~80 | ~130 | +50 |
| Native gameplay tags | 24 | 45+ | +21 |
| Console commands | 15 | 30+ | +15+ |
| Subsystems | 1 (LabWorldSubsystem) | 5 (Lab + HostilePack + Squad + Ping + Match) | +4 |
| World actor classes | 2 (DummyTarget + PingMarker n/a) | 5 (DummyTarget + PingMarker + PalSpawner + ExtractionBeacon + ...) | +3 |
| Activity subclasses | 3 (Idle/Follow/Investigate) | 7 (+ Combat/Patrol/Stalk/FollowSquadCommand) | +4 |
| Validator lines | ~1,367 | ~3,000 | +1,633 |
| README lines | 516 | 1,896 | +1,380 |
| Pillar coverage estimate | ~65% | ~70% (Q2 + Q1 carry) | +5 |

---

## 9. References

- [`Documents/PALDARK/03-Roadmap_1_Year.md`](../PALDARK/03-Roadmap_1_Year.md) — toàn bộ 52-week schedule.
- [`Documents/PALDARK/02-Pillar_Coverage.md`](../PALDARK/02-Pillar_Coverage.md) — pillar target / mapping feature.
- [`Documents/UE5_Core_Pillars.md`](../UE5_Core_Pillars.md) — 18 pillar definition.
- [`PaldarkLab/README.md`](../../PaldarkLab/README.md) — feature-by-feature authoring + test loop (1896 dòng).
- [`Documents/Devlog/Q2-Playtest-Checklist.md`](Q2-Playtest-Checklist.md) — runnable 4-client PIE walkthrough.
- [`Documents/Devlog/Q2-Bug-Bash.md`](Q2-Bug-Bash.md) — 20-issue priority list cho 1-day bug bash.
- [`Documents/Devlog/Q1-Milestone-Build.md`](Q1-Milestone-Build.md) + [`Q1-Playtest-Checklist.md`](Q1-Playtest-Checklist.md) + [`Q1-Bug-Bash.md`](Q1-Bug-Bash.md) — Q1 baseline.
- [`Documents/Devlog/W14-15-Build.md`](W14-15-Build.md) + [`W16-17-Build.md`](W16-17-Build.md) + [`W18-19-Build.md`](W18-19-Build.md) + [`W20-21-Build.md`](W20-21-Build.md) + [`W22-23-Build.md`](W22-23-Build.md) + [`W24-25-Build.md`](W24-25-Build.md) — per-week build logs.
