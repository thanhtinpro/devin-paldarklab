# PALDARK — Q2 Bug Bash (Tuần 26, 1 ngày)

> **Source:** Roadmap § Tuần 26 "Internal playtest: 4 player, 4 Pal, 1 map, 4 enemy, full loop." Audit toàn bộ C++ + ini + validator + README ở freeze point W25 → ưu tiên hóa cho 1 ngày bug bash trước khi vào Q3.
>
> **Method:** Grep `TODO|FIXME|HACK` trong `PaldarkLab/Source/` (kết quả: 0 — codebase sạch ở mức comment). Audit thủ công 6 PR description W14-25 + README để rút known-issue Q2 + carry-over Q1 chưa đóng. Cross-reference với pillar gap trong [`Q2-Milestone-Build.md`](Q2-Milestone-Build.md) § 5.

---

## 0. Priority legend

| P0 | Block Q2 outcome — phải fix trước khi tick milestone. |
|----|--------------------------------------------------------|
| P1 | Block hoặc xấu UX nhưng có workaround — fix trong bug-bash day. |
| P2 | Cosmetic / improvement — defer Q3. |
| P3 | Future-proofing — defer Q4+. |

---

## 1. Issue table (20 issue, ưu tiên hóa)

### Carry-over từ Q1 (chưa đóng)

| ID | Priority | Issue | Component | Roadmap defer | Fix sketch |
|----|----------|-------|-----------|---------------|------------|
| K-01 | (code-closed W27, designer authoring pending) | ~~Không có AnimBP cho Player/Pal/hostile-Pal — mannequin T-pose~~ | ~~Animation~~ | ~~W27 đầu Q3~~ | ✅ Code-side closed by W27 Path A PR: `UPaldarkAnimInstance` (base, thread-safe snapshot pattern) + `UPaldarkPalAnimInstance` (subclass với bIsHostile/bIsFlying/bIsInCombat) + `IPaldarkAnimLayerInterface` (3 layer functions) + `UPaldarkPawnData::DefaultAnimInstanceClass` slot + `APaldarkCharacter::SetPawnData` wires `Mesh->SetAnimInstanceClass(...)` + `check_anim_instance_shape()` validator. Designer task remains: 2-day AnimBP authoring (State Machine + Blend Space + layer function implementations) → assets land trên branch riêng. Xem `PaldarkLab/README.md` § "AnimBP Lyra ALS port (W27)". |
| K-02 | **P1** | `Raid_Sandbox.umap` chưa commit nếu Q1 bug bash slot 3 chưa close | Map | Q2 designer task (carry) | Designer setup git-lfs + commit map + 2-4 PlayerStart (≥ 4 cho Q2). Estimate: 1 giờ. |
| K-07 | (closed by #19 W16-17) | ~~Hitscan không lag comp~~ | ~~Network~~ | ~~W16-17~~ | ✅ Đóng bởi PR #19 (`UPaldarkLagCompensationComponent` 1s frame buffer + rewind). |

### Q2 mới phát sinh (W14-25)

| ID | Priority | Issue | Component | Roadmap defer | Fix sketch |
|----|----------|-------|-----------|---------------|------------|
| L-01 | (closed by W26 follow-up PR — validator extend) | ~~`APaldarkGameMode_Extraction` chưa đăng ký trong `DefaultEngine.ini`~~ | ~~Match/Framework~~ | ~~Q3 đầu~~ | ✅ Đóng bởi W26 validator-extend PR: (a) `UPaldarkExperienceDefinition::RequiredGameModeClass` field + runtime assertion in `APaldarkGameModeBase::OnExperienceAssetReady` (LogError + red `AddOnScreenDebugMessage`); (b) `+GameModeClassAliases=(Name="Extraction",...)` in `DefaultEngine.ini` so `?game=Extraction` URL flag works; (c) `check_extraction_gamemode_wired()` in `validate_paldarklab.py` enforces all three layers. Designer still must set the field on `PX_RaidSandbox.uasset` (1-tap) or use World Override. |
| L-06 | (code-closed W27, designer authoring pending) | ~~AnimBP T-pose risk — Q1 K-01 chưa fully resolved (Path B mannequin default chỉ duct-tape). Designer record video sẽ thấy ALS rough~~ | ~~Animation~~ | ~~Q3 đầu (cùng K-01)~~ | ✅ Đóng cùng K-01 by W27 Path A PR. Code-side scaffold (AnimInstance + layer interface + PawnData slot) shipped; designer authoring (Blend Space + State Machine + 3 layer functions per character) is the remaining step. |
| L-13 | **P1** | Squad FollowSquadCommand activity P22 sit giữa Follow P20 và Stalk P25 — designer dễ confuse priority ladder; hostile-Pal subclass có thể accidentally inherit | AI | Validator extend Q3 | Validator check: hostile-Pal `CandidateActivities` MUST `Replace` not `Append`. Estimate: 2 giờ. |
| L-02 | **P1** | UMG end-of-match widget chưa tồn tại — chỉ có C++ delegate `OnMatchOutcomeChanged` + `OnMatchPhaseReplicated`. Demo video sẽ thiếu end screen | UI | W30+ polish | `WBP_EndOfMatchScreen` listen 2 delegate + show win/loss + per-player outcome table. Estimate: 3 ngày (Aura WidgetController pattern). |
| L-12 | **P1** | Squad command radial wheel UMG chưa tồn tại — chỉ test qua console; designer demo "feel" rất ốm | UI | W30+ polish | `WBP_SquadRadial` UMG widget bind tới `USquadCommandSlotComponent`. Estimate: 2 ngày. |
| L-16 | **P1** | `ExtractionProgress` REPNOTIFY throttle 0.25s → client thấy progress bar giật stair-step (nếu UMG widget exist) | UI/Net | UI widget task | HUD task interpolate giữa RepNotify với `FMath::Lerp`. Estimate: 2 giờ trong UI PR. |
| L-04 | **P2** | Reconnect mid-match không support — `Disconnected` là terminal. Player Alt+F4 → re-join = new PlayerState, không restore outcome | Network/Match | Q3 (W14-15 follow-up) | Map `LoginID` → restore `Outcome` từ subsystem registry. Estimate: 1 ngày. |
| L-05 | **P2** | Spectator camera chưa wire — KIA player stuck ragdoll, không observe teammate | UI/Camera | W30+ polish | `USpectatorCameraComponent` + free-cam input + KIA → switch camera transition. Estimate: 2 ngày. |
| L-09 | **P2** | Beacon `bRequireExplicitConsent` tag `Paldark.InputTag.InteractExtraction` chưa có default IMC binding — designer tự bind E mỗi project | Input | Q3 polish | Add default `IMC_Default` row → `IA_InteractExtraction` (E). Estimate: 30 phút. |
| L-17 | **P2** | Hard timeout `MaxMatchDuration` default 600s — không có designer-tunable per-experience | Match | Q3 | Expose qua `UPaldarkExperienceDefinition.MatchTimeoutOverride`. Estimate: 2 giờ. |
| L-20 | **P2** | `UPaldarkNetSubsystem` `OnPostLogin` event không expose tới Blueprint — UMG lobby UI sẽ cần | Network | W30+ (cùng UMG lobby) | Add `BlueprintAssignable` delegate `OnPlayerLogin / OnPlayerLogout`. Estimate: 1 giờ. |
| L-10 | **P3** | Razorbird `MOVE_Flying` không pathfind navmesh — drive thẳng `AddMovementInput` world-space | AI | W30+ (3D nav volume) | UE 3D NavMesh Volume + flight envelope constraints. Estimate: 3 ngày. |
| L-11 | **P3** | Pack subsystem broadcast aggro không có distance falloff — pack-mate xa nhất nhận trigger ngang pack-mate gần nhất | AI | Q4 polish | Weighted broadcast với cubic falloff. Estimate: 4 giờ. |
| L-18 | **P3** | CI validator chưa check `Raid_Sandbox.umap` presence (Q1 K-02 carry) | Validator | Q3 đầu | `validate_paldarklab.py` grep `Content/Paldark/Maps/Raid_Sandbox.umap` exist. Estimate: 1 giờ. |
| L-19 | **P3** | Pal Combat ability `BP_GA_PalAttack` SetByCaller `Paldark.SetByCaller.PalDamage` chưa expose damage tuning qua DataAsset — hardcoded designer-author cost | GAS | Q3 polish | `UPaldarkPalCombatProfileDataAsset` per-Pal-species. Estimate: 1 ngày. |
| L-15 | **P3** | `OnHealthZeroed` weak lambda capture `this` — nếu Character destroy trước PlayerState resolve, có thể null-deref | Match | (No fix needed — guards sufficient) | `MatchDeathHookHandle.IsValid()` + `IsValid(PlayerState)` guards đã có. |
| L-14 | **P3** | Match `Paldark.Match.Phase Extracting` console không direct enter — call StartMatch và log warning | Match/Console | (No fix needed — banner-only phase) | Banner-only phase set bởi first RecordExtraction; documented trong handler comment. |
| L-08 | **P3** | Persistent match stats chưa wire — kết quả lưu in-memory, world teardown wipe | Backend | W42-43 (AWS) | DynamoDB write-back qua `URaidResultUploadSubsystem`. Estimate: 1 tuần. |
| L-03 | **P3** | Lobby travel chưa wire — match Ended → designer phải reload level manually | Framework | W40+ hub town | `ServerTravel("/Game/Maps/Lobby")` + lobby map + matchmaking glue. Estimate: 2 tuần. |
| L-07 | **P3** | Multi-match-per-session không support — designer reload level cho match tiếp theo | Framework | W40+ seamless travel | Cùng L-03. |

---

## 2. P0 / P1 fix plan (1 ngày bug bash)

### Slot 1 — Sáng (P0 L-01: ✅ closed by W26 validator-extend PR — designer authoring step remains)

> **Status update:** Validator extend đã ship sớm (W26 follow-up PR, ngoài bug-bash day). Codeside 3-layer defense xong; designer chỉ còn 1-tap authoring + smoke test.

- [x] Devin: `UPaldarkExperienceDefinition::RequiredGameModeClass` field added.
- [x] Devin: `APaldarkGameModeBase::OnExperienceAssetReady` asserts + logs Error + on-screen red banner if mismatched.
- [x] Devin: `+GameModeClassAliases=(Name="Extraction",GameMode="/Script/PaldarkLab.PaldarkGameMode_Extraction")` added to `DefaultEngine.ini`.
- [x] Devin: `check_extraction_gamemode_wired()` validator covers all three layers.
- [x] Devin: README W24-25 "Authoring order" rewritten with 3-layer pattern.
- [ ] Designer: open `PX_RaidSandbox` (Experience DataAsset). Set `RequiredGameModeClass = APaldarkGameMode_Extraction` (one slot, no GameMode override needed).
- [ ] Designer (alt): set `Raid_Sandbox → World Settings → GameMode Override = APaldarkGameMode_Extraction`. Either path (a) OR (c) is enough; both is safer.
- [ ] Commit `PX_RaidSandbox.uasset` (git-lfs).
- [ ] Smoke test: PIE listen+1 client → `Paldark.Match.Dump` → `Phase=Warmup`. Đợi 30s → `Phase=Active` (chứng tỏ subsystem chạy). Verify NO red on-screen banner.
- [ ] Negative test: temporarily un-set `RequiredGameModeClass` OR boot map without override → PIE should show RED on-screen `Experience PaldarkExperience.PX_RaidSandbox expects GameMode ... (got ...). Match features disabled.`

**Estimate: 30 phút (designer side, was 1 giờ).**

### Slot 2 — Trưa (P0 L-06 + K-01: AnimBP basic)

> **Path B continuation (Q1 bug bash compromise).** Same approach: dùng UE5 Mannequin default ABP cho Q2 milestone video. Full Lyra ALS port là W27 đầu Q3 (Path A).

- [ ] `BP_PaldarkCharacter` Mesh → set `Anim Class = ABP_Manny` (Q1 carry).
- [ ] `BP_PaldarkPalCharacter` Mesh → set `Anim Class = ABP_Manny`.
- [ ] **Q2 mới:** `BP_PaldarkCharacter_Direhound` Mesh → set `Anim Class = ABP_Manny` (placeholder wolf 4-leg sẽ T-pose, acceptable).
- [ ] **Q2 mới:** `BP_PaldarkCharacter_Razorbird` Mesh → set `Anim Class = ABP_Manny` (placeholder, sẽ "fly" T-pose).
- [ ] Verify PIE: 4 hostile + 4 companion + 4 player chạy walk anim đúng direction. Razorbird floating in air with `MOVE_Flying` (visual broken, ghi nhận known issue defer Q3).
- [ ] Commit 4 `BP_PaldarkCharacter*.uasset` (git-lfs).

**Estimate: 2 giờ (chủ yếu fix BP path + recommit).**

### Slot 3 — Chiều (P1 L-13: Validator check hostile-Pal `CandidateActivities`)

Đây là code-side fix, không cần designer.

- [ ] Extend `scripts/ci/validate_paldarklab.py` → `check_hostile_pal_activity_list`. Read `Source/PaldarkLab/Private/Pal/PaldarkPalCharacter_Direhound.cpp` + `Razorbird.cpp` → assert constructor sets `CandidateActivities = ...` (NOT `.Add(...)` or `.Append(...)`). Phải là full replace.
- [ ] Verify locally: `python3 scripts/ci/validate_paldarklab.py` → OK.
- [ ] Commit + push PR follow-up.

**Estimate: 2 giờ (validator + test + PR).**

### Slot 4 — Cuối ngày (optional P1 L-02 / L-12: UMG widget stub)

Nếu còn thời gian:

- [ ] `WBP_EndOfMatchScreen` UMG class skeleton: 1 TextBlock "Match Ended" + 1 ListView player outcomes. Bind: `BeginPlay` → cast PlayerState + GameState + AddDelegate `OnMatchPhaseReplicated` / `OnMatchOutcomeChanged`. Show/hide based on phase.
- [ ] Add tới `BP_PaldarkCharacter.AddToViewport` ở client side (PossessedBy hook).
- [ ] **DO NOT** fully wire styling, animation, etc. — chỉ stub đủ visual feedback cho video.
- [ ] Optional: `WBP_SquadRadial` cùng pattern.

**Estimate: 3 giờ (skeleton only — full polish defer W30+ UI polish week).**

---

## 3. Open questions trước khi đóng Q2

Decision cần user (designer + tech lead) trả lời trong bug bash day:

- [x] ~~**Q-Q2-01:** Validator extend cho L-01 (enforce GameMode override)~~ — **Decided:** shipped early as a stand-alone W26 follow-up PR (3-layer defense). Slot 4 không cần dùng cho việc này.
- [x] ~~**Q-Q2-02:** Lyra ALS port (K-01 / L-06) — Path A hay Path B?~~ — **Decided W27: Path A** (C++ scaffold landed by W27 PR; designer AnimBP authoring runs in parallel on separate branch, 2 days designer-side). Q3 schedule giữ nguyên — W27-28 AssetManager async load chen ngay sau W27 PR merge.
- [ ] **Q-Q2-03:** UMG end-of-match + squad radial widget — bắt buộc cho Q2 demo video không? Roadmap § Q2 outcome ghi "vertical slice 1, end-to-end" — em hiểu là core gameplay loop, UI có thể defer. Đề xuất **stub-only trong Slot 4**, full UMG W30+.
- [ ] **Q-Q2-04:** Reconnect mid-match (L-04) — Q3 đầu hay cuối Q3 W36+? Em đề xuất **W36+** (sau lobby/matchmaking để có nơi mà reconnect *vào*).
- [ ] **Q-Q2-05:** Hostile-Pal AnimBP — designer commit hôm nay placeholder hay mua/import animset proper (wolf + bird) trong Q3? Em đề xuất **placeholder Q2, real Q3 đầu** (cost: $100-300 marketplace animset).

---

## 4. Closure criteria

Q2 milestone đóng khi:

- [x] ✅ Tất cả P0 fix (L-01 closed by W26 follow-up PR #25; L-06/K-01 **code-side closed by W27 Path A** — designer AnimBP authoring deferred to parallel branch).
- [ ] ✅ Ít nhất 1/2 P1 fix (L-13 validator extend; L-02/L-12 UMG stub là nice-to-have).
- [ ] ✅ [`Documents/Devlog/Q2-Playtest-Checklist.md`](Q2-Playtest-Checklist.md) tick xong 8/8 section (skip Dedicated optional).
- [ ] ✅ Video 60s shipped vào `Q2-Playtest-Video.md`.
- [x] ✅ Q-Q2-01..Q-Q2-05 decision logged trong devlog hoặc `02-Pillar_Coverage.md` update (Q-Q2-01 W26 PR #25, Q-Q2-02 W27 Path A PR).

Nếu P0 L-01 không close (designer chưa swap GameMode): defer Q2 milestone 1-2 ngày; KHÔNG defer cả tuần. Đây là 1-tap config change.

Nếu P0 L-06/K-01 chưa close (mannequin T-pose vẫn xấu): video record với caption "Visual placeholder, AnimBP polish W27". Q2 milestone vẫn đóng, ghi note risk cho Q3. **W27 update:** Path A C++ scaffold shipped → video caption đổi thành "Designer-authored AnimBP pending; locomotion data shape parity với Lyra ALS already in place." Risk note xuống P1.

---

## 5. Cumulative Q1+Q2 issue stats

| Category | Q1 close (15 issue) | Q2 close (20 new) | Δ closed Q1→Q2 |
|----------|---------------------|-------------------|-----------------|
| P0 still open | K-01 (1) | L-01 + L-06 + K-01 (3) | -1 carry, +2 mới |
| P1 still open | K-02 + K-07 + K-13 (3) | K-02 + L-02 + L-12 + L-13 + L-16 (5) | -2 K-07/K-13 closed, +4 mới |
| P2 still open | K-04 + K-05 + K-06 + K-14 (4) | L-04 + L-05 + L-09 + L-17 + L-20 (5) | -4 carry, +5 mới (W14-15 follow-up needed) |
| P3 still open | K-08-K-12 + K-15 (6) | L-03 + L-07 + L-08 + L-10 + L-11 + L-14 + L-15 + L-18 + L-19 (9) | -6 carry, +9 mới |

**Closed Q1→Q2 (3):** K-07 (lag comp by #19), K-13 (sprint stamina cancel by W14-15 follow-up — *check if actually closed*), K-14 (RPC by #18).

**Re-classification Q1→Q2:**
- K-01 escalated từ Q1 P0 (defer W13.5) → Q2 P0 (must close W27 đầu).
- K-15 demoted P3 → not blocking, defer Q3.

> **Verdict:** Q1+Q2 cộng dồn ~20 issue Q2-relevant + 6 carry P3. Healthy ratio — codebase chưa có technical debt critical. P0 cluster (L-01 + L-06 + K-01) là 1-2 ngày work, không phải tuần. Q3 mở đầu với 1 PR Lyra ALS + 1 PR validator extend → đóng được 80% P0/P1 cluster trong 1 tuần.

---

## 6. References

- [`Documents/Devlog/Q2-Milestone-Build.md`](Q2-Milestone-Build.md) § 5 (Known issues table).
- [`Documents/Devlog/Q2-Playtest-Checklist.md`](Q2-Playtest-Checklist.md) § 10.
- [`Documents/Devlog/Q1-Bug-Bash.md`](Q1-Bug-Bash.md) — Q1 issue table (K-01 → K-15).
- [`Documents/PALDARK/03-Roadmap_1_Year.md`](../PALDARK/03-Roadmap_1_Year.md) § Tuần 26 + § Q2 outcome + § Q3.
- [`PaldarkLab/README.md`](../../PaldarkLab/README.md) — anti-pattern sections per W14-25 feature.
