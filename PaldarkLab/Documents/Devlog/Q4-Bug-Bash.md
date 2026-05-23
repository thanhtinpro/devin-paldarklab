# PALDARK — Q4 Bug Bash (Tuần 49-50)

> **Source:** [`Documents/PALDARK/03-Roadmap_1_Year.md`](../PALDARK/03-Roadmap_1_Year.md) § Tuần 49-50.
>
> **Purpose:** consolidate Q4 known issue + Q1-Q3 carry-over, prioritize cho 1-day bug bash trước alpha demo, define close-out criteria cho Q4 milestone.

This document complements [`Q4-Milestone-Build.md`](Q4-Milestone-Build.md) §5 (Known issues & risks). Issue list dưới đây gom tất cả L-XX từ Q4 (W40-48) cộng carry-over từ Q1/Q2/Q3 còn open. Slot plan §2 phá thành 4 morning/afternoon slot cho 1-day designer + dev bug bash.

---

## 0. Priority legend

- **P0 (critical, alpha-block)** — alpha demo không record được nếu không fix. Must fix tuần này.
- **P1 (designer-block, alpha-critical)** — C++ scaffold xong, designer cần author asset để playable end-to-end. Critical path cho alpha demo. 5-6 ngày authoring (xem [`Q4-Playtest-Checklist.md`](Q4-Playtest-Checklist.md) §0).
- **P2 (polish, post-alpha)** — bug có log warning, không crash, không break demo. Defer Q1-next cleanup.
- **P3 (defer/wontfix)** — known limitation, documented as anti-pattern, không fix trong roadmap window.

---

## 1. Issue table (20 issue, ưu tiên hóa)

### Carry-over từ Q1/Q2 (chưa đóng)

| ID | Issue | Severity | Origin | Mitigation | Target |
|----|-------|----------|--------|------------|--------|
| L-04 | Activity FSM no hysteresis between `Idle` ↔ `Investigate` distance | P2 | Q1 W5-6 | Documented anti-pattern; designer tune hysteresis range trong DA | Q1-next |
| L-05 | GAS attribute `Stamina` regen rate hardcoded in `UPaldarkAttributeSet::PostInitProperties` | P2 | Q1 W7-8 | Expose `BaseStaminaRegenRate` UPROPERTY trong AttributeSet | Q1-next |
| L-09 | `UPaldarkLagCompensationComponent` ring buffer size hardcoded 30 frames | P2 | Q2 W16-17 | Expose `RingBufferFrames` UPROPERTY in DA | Q1-next |
| L-17 | `UPaldarkMatchSubsystem::DefaultMatchTimeout` hardcoded; per-experience override pending | P2 | Q2 W24-25 | Expose `MatchTimeoutOverride` on `UPaldarkExperienceDefinition` | Q1-next |
| L-37 | Tame attempt formula `BaseCaptureProbability` không có client preview — UI cần show "32% chance to capture" trước throw | P2 | Q3 W35-36 | HUD task: predicted probability từ AttributeSet read + Pal definition | Q1-next (HUD polish) |

### Q4 mới phát sinh (W40-48)

| ID | Issue | Severity | Origin | Mitigation | Target |
|----|-------|----------|--------|------------|--------|
| L-41 | Backend `bUseAWSBackend=False` default — designer demo trên dedicated server có thể quên flip flag → log "Stub session" thay vì "Cognito session" | P2 | W42-43 #41 | README W42-43 §"Authoring order" step 1 + log warning + console banner trên `Paldark.Backend.Status` | Q1-next (SRE deploy) |
| L-42 | `UPaldarkBriefingSessionComponent` countdown CVar `Paldark.Briefing.CountdownSec` mặc định 10s — designer cần custom cho long demo | P2 | W46 #43 | Documented; designer can override via console | Q1-next polish |
| L-43 | Save game `kCurrentSchema=1`; future field addition stamps `Fail.SchemaMismatch` — không có migration chain | P2 | W47 #44 | Documented anti-pattern (W47 README); Q1-next migration data asset chain | Q1-next |
| L-44 | `UPaldarkSaveSubsystem` auto-save trigger không có throttle/debounce — match flap Extracted ↔ Spawned 5/sec → 5 save fire | P2 | W48 #45 | Documented anti-pattern (W48 README); 5-second debounce defer | Q1-next polish |
| L-45 | `UPaldarkSaveSlotPickerWidget::ListSlots()` returns int32 count only; designer cần structured `TArray<FName>` cho UMG list view | P2 | W48 #45 | Extension API Q1-next | Q1-next |
| L-46 | `WBP_PaldarkBriefingWidget` BP subclass chưa author — vote/ready handoff playable qua console only | **P1** | W46 #43 | Designer task ~1 ngày, W46 README §"Designer test loop" | W49-50 (now) |
| L-47 | `WBP_PaldarkStableWidget` + `WBP_PaldarkMarketplaceWidget` BP subclass chưa author | **P1** | W44-45 #42 | Designer task ~2 ngày, W44-45 README §"Designer test loop" | W49-50 (now) |
| L-48 | `WBP_PaldarkSaveSlotPicker` BP subclass chưa author | P2 | W48 #45 | Designer task ~½ ngày, W48 README §"UMG slot picker shape" | Q1-next |
| L-49 | `Map_HubTown.umap` + `DA_PaldarkExperience_HubTown` + 3 building placement chưa author | **P1** | W40-41 #40 | Designer task ~2 ngày, W40-41 README §"Authoring order" | W49-50 (now) |
| L-50 | `DA_MarketplaceCatalog_Default` chưa author — buy/sell price chưa tunable | **P1** | W44-45 #42 | Designer task ~½ ngày tuning, W44-45 README §"Catalog authoring" | W49-50 (now) |
| L-51 | Multi-server hub shard merging — 2 hub shard có 4 player mỗi → cùng raid không hỗ trợ matching qua shard | P3 | W40-41 #40 | Documented; cross-shard matching defer Q2-next (post-alpha) | Q2-next |
| L-52 | Backend Stub mode random JWT — refresh không expire mock token, prod sẽ throw mismatch | P3 | W42-43 #41 | Documented anti-pattern (W42-43 README); Stub là dev-only | (no fix — Stub mode by design) |
| L-53 | `UPaldarkPlayerSaveGame::PlayerName` không validate sanitize — designer save slot path injection risk on Windows reserved name (e.g. "CON") | P3 | W47 #44 | Defer (designer name validation defer Q1-next) | Q1-next |
| L-54 | Pal Stable/Marketplace có race condition khi 2 player cùng frame deposit/buy cùng item — first-wins acceptable | P3 | W44-45 #42 | Documented; per-player wallet/deposit guard authoritative | (no fix — first-wins acceptable) |
| L-55 | UPaldarkSaveSlotPickerWidget `Trigger.Manual` mặc định trên Save BP wrapper — UMG demo không stamp trigger.HubReturn | P3 | W48 #45 | Documented; designer extend BP wrapper nếu cần custom trigger | Q1-next polish |

> **Distribution Q4:**
> - 0 P0
> - 5 P1 (designer-block, alpha-critical: L-46/L-47/L-49/L-50 + 1 carry from Q3 implied)
> - 8 P2 (polish: L-04/L-05/L-09/L-17/L-37 + L-41/L-42/L-43/L-44/L-45/L-48)
> - 5 P3 (defer/wontfix: L-51/L-52/L-53/L-54/L-55)
>
> Cumulative since session start: 55 L-XX tracked, 30 closed, 25 carry/open. Q4 close-out plan defers 17 to Q1-next (no codebase debt blocker before alpha demo).

---

## 2. P0 / P1 fix plan (1 ngày bug bash)

> Q4 không có P0. P1 toàn bộ là designer authoring task. Slot plan dưới phân chia designer time cho 1-day bug bash (~5-6 ngày total nhưng concentrate vào critical 1-day slot trước alpha demo).

### Slot 1 — Sáng (P1 L-49: Map_HubTown blockout, ~3 giờ)

- Designer A author `Map_HubTown.umap` (sublevel hoặc full level): ngoại cảnh hub town + ground plane + camera placement + lighting blockout.
- Place 3 `APaldarkHubBuilding` actor:
  - West of plaza → tag `Paldark.Hub.Building.Stable`
  - East of plaza → tag `Paldark.Hub.Building.Marketplace`
  - North of plaza → tag `Paldark.Hub.Building.Briefing`
- Create `DA_PaldarkExperience_HubTown`, wire `DefaultPawnData` + `RequiredGameFeatures` (empty).
- Wire `DefaultEngine.ini` `GameDefaultMap=/Game/Maps/Map_HubTown.Map_HubTown` cho dev convenience.

**Acceptance:** PIE start → spawn vào hub plaza. `Paldark.Hub.DumpBuildings` log line `Hub buildings: 3 entries=[Building_Stable, Building_Marketplace, Building_Briefing]`.

### Slot 2 — Trưa (P1 L-46: WBP_PaldarkBriefingWidget, ~3 giờ)

- Designer B subclass `UPaldarkBriefingWidget` → `WBP_PaldarkBriefingWidget`.
- Layout vertical panel:
  - Top: map vote list (ListView bind sang `GetSessionState().AllowedMaps`)
  - Middle: ready button (toggle, bind sang `ToggleReady()` BP wrapper)
  - Bottom: countdown HUD (TextBlock bind sang `GetSessionState().CountdownSec`)
- Override `K2_OnPhaseChanged(NewPhase)` switch panel visibility theo `Paldark.Briefing.Phase.{Lobby, Voting, Countdown, Travel}`.
- Wire `WBP_PaldarkBriefingWidget` vào `APaldarkBriefingTerminal::DefaultWidgetClass`.

**Acceptance:** PIE 4-client. Mỗi player walk to Briefing building → open widget → vote map → ready. Phase chuyển Lobby → Voting → Countdown → Travel (10s). ServerTravel fire.

### Slot 3 — Chiều (P1 L-47 + L-50: Stable + Marketplace UI + Catalog, ~5 giờ)

- Designer C subclass:
  - `UPaldarkStableWidget` → `WBP_PaldarkStableWidget` (2-column layout: Active Roster | Deposit Roster).
  - `UPaldarkMarketplaceWidget` → `WBP_PaldarkMarketplaceWidget` (catalog list + buy/sell modal + wallet display).
- Designer D author `DA_MarketplaceCatalog_Default`: ~30 entry với buy/sell price (Pistol = 250, Bandage = 50, 9mm = 5/round, etc.). Sell rate ~60-70% of buy.
- Wire `WBP_PaldarkStableWidget` + `WBP_PaldarkMarketplaceWidget` vào tương ứng `APaldarkHubBuilding`'s widget slot.

**Acceptance:** PIE 1-client trong hub. Walk to Stable → deposit/withdraw Pal qua UI. Walk to Marketplace → buy Pistol (250 credit) → wallet 1000→750. Sell back → wallet 750→900 (60% rate). All transaction stamp `Paldark.Hub.Result.Success` log.

### Slot 4 — Cuối ngày (P2 L-48: WBP_PaldarkSaveSlotPicker, ~2 giờ)

- Designer A (xong slot 1 chiều) subclass `UPaldarkSaveSlotPickerWidget` → `WBP_PaldarkSaveSlotPicker`.
- Layout: ListView (10 hard-coded slot Slot01..Slot10). Per row: slot name + 3 button (Save / Load / Delete).
- Button events bind sang `RequestSave/Load/Clear` BP wrapper.
- Override `K2_OnSaveCompleted` + `K2_OnLoadCompleted` cho toast + HUD refresh.

**Acceptance:** Pause menu → "Save / Load" → picker mở. Click Save trên Slot03 → toast green "Saved!". Click Load trên Slot01 → load snapshot.

---

## 3. Open questions trước khi đóng Q4

1. **Alpha demo scope freeze:** Hub-only (1 map) hay Hub + 3 raid map (4 map total)?
   - **Recommend Hub + 1 raid map (Raid_Sandbox)** cho alpha demo 1.0; Rừng Hỏng + PalCorp authoring defer Q1-next nếu Q3 designer task chưa close.
2. **Backend Stub vs AWS deploy:** Demo trên local Stub backend hay deploy thật?
   - **Recommend Stub backend** cho alpha demo (SRE deploy là post-alpha task). Demo video chú thích "backend mocked locally" trong descrription.
3. **Animation back-fill timing:** Q1-next sprint hay parallel với alpha demo polish?
   - **Recommend Q1-next dedicated sprint** (2-week) — alpha demo accept T-pose nếu animation chưa land (đây là PaldarkLab 5-month carry-over từ Q1).
4. **Save schema version bump policy:** Auto-bump on field add hay manual?
   - **Recommend manual bump + migration data asset chain** Q1-next (L-43). Trong alpha demo schema=1, no migration cần.
5. **Marketplace catalog authoring ownership:** Designer-team hay GD-team?
   - **Recommend GD-team** wire initial price; designer-team handle layout + tier filter.

---

## 4. Closure criteria

Q4 sẽ được consider "closed" khi:

1. ✅ All Q4 PR (#35-#45) merged vào main — DONE.
2. ✅ CI 2/2 xanh cho mỗi PR — DONE.
3. ✅ `PaldarkLab/README.md` cập nhật cho mỗi feature week (W40-W48) — DONE.
4. ✅ Validator cumulative coverage ~7,962 dòng (Q3=5,400 → Q4=7,962, +2,562) — DONE.
5. ⏳ Designer author 5 critical asset cluster (L-46/L-47/L-49/L-50 + L-48 nice-to-have):
   - `Map_HubTown.umap` + `DA_PaldarkExperience_HubTown` + 3 building placement
   - `WBP_PaldarkBriefingWidget` BP subclass
   - `WBP_PaldarkStableWidget` + `WBP_PaldarkMarketplaceWidget` BP subclass
   - `DA_MarketplaceCatalog_Default` (~30 entry)
   - `WBP_PaldarkSaveSlotPicker` BP subclass (P2, nice-to-have)
6. ⏳ QA chạy 12-step Q4 playtest checklist 4-client PIE — pass criteria mỗi step.
7. ⏳ 90-second alpha demo video recorded + embed vào `Documents/Devlog/Q4-Playtest-Video.md`.
8. ⏳ Internal alpha demo review meeting scheduled.
9. ⏳ Q1-next sprint plan locked (Animation back-fill + FMOD + real AWS deploy + bug bash carry-over).

**Once all 9 criteria pass:** Q4 milestone closed. Begin Q1-next sprint (Tuần 1-13 of year 2).

---

## 5. Cumulative Q1+Q2+Q3+Q4 issue stats

| Metric | Q1 close (W13) | Q2 close (W26) | Q3 close (W39) | Q4 close (W48) | Δ Q3→Q4 |
|--------|----------------|----------------|----------------|----------------|---------|
| L-XX issues tracked total | 8 | 20 | 40 | 55 | +15 |
| P0 open | 0 | 0 | 0 | 0 | 0 |
| P1 open | 2 | 3 | 4 | 5 | +1 |
| P2 open | 4 | 8 | 11 | 8 | -3 (5 Q3 P2 closed Q4 + 6 new) |
| P3 open | 2 | 9 | 17 | 22 | +5 |
| L-XX issues closed Q-over-Q | n/a | 5 | 7 | 5 | -2 |
| Validator lines added | n/a | +1,633 | +2,400 | +2,562 | +162 |

**Q4 close-out story:**
- Q3 → Q4 transition: 5 P2 carry from Q3 closed (L-20 + L-21 + L-28 + L-32 + L-34) trong Q4 bug-bash followup PR (#35-#39) trước khi vào feature weeks. Resulting clean state cho W40-48 feature delivery.
- Q4 spawn: 15 new L-XX (L-41 → L-55). 5 P1 (designer authoring), 6 P2 (polish), 5 P3 (defer). No P0.
- Verdict: Codebase technical debt remains healthy. Designer authoring backlog (L-46/L-47/L-49/L-50) là critical path cho alpha demo nhưng không block code merge.

---

## 6. References

- [`Documents/Devlog/Q4-Milestone-Build.md`](Q4-Milestone-Build.md) — Q4 PR ledger + pillar coverage + gap analysis (§5 Known issues mirror).
- [`Documents/Devlog/Q4-Playtest-Checklist.md`](Q4-Playtest-Checklist.md) — runnable 4-client PIE walkthrough §0 setup + §1-13 step.
- [`PaldarkLab/README.md`](../../PaldarkLab/README.md) §"W40-41" → §"W48" — feature-by-feature authoring + console test loop.
- [`Documents/Devlog/Q3-Bug-Bash.md`](Q3-Bug-Bash.md) — Q3 baseline 20-issue list.
- [`Documents/Devlog/Q2-Bug-Bash.md`](Q2-Bug-Bash.md) + [`Q1-Bug-Bash.md`](Q1-Bug-Bash.md) — Q2/Q1 baseline.
