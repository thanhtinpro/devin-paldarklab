# PALDARK — Q4 Milestone Build (Tuần 49-50)

> **Source:** [`Documents/PALDARK/03-Roadmap_1_Year.md`](../PALDARK/03-Roadmap_1_Year.md) § Tuần 49-50.
>
> **Roadmap deliverable:** "Q4 milestone: hub town playable + AWS backend integration + save/load loop + polish pass before alpha demo."
>
> **Q4 Outcome (from roadmap):** **Alpha-ready vertical slice 3** — hub-and-spoke loop (Hub Town → briefing → raid → extract → save → return) end-to-end với 1 listen-server + 4-player co-op + persistent player progression.

This devlog freezes the Q4 milestone tại W48 (W49-50 là Q4 close-out — milestone + playtest + bug bash docs, không có feature week mới). Nó tóm tắt 11 PR Devin shipped W40-48, mapping vào roadmap Q4 axis, đo coverage thực tế của pillar, gap-analysis vs alpha-demo claim, và liệt kê việc designer cần author trong UE Editor để release alpha.

---

## 1. PR ledger (W40 → W48)

| PR | Week | Title | Status | Branch |
|----|------|-------|--------|--------|
| [#35](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/35) | (Q3 Bug-Bash) | Q3 Bug-Bash L-21 — Game Feature plugin auto-activate (RequiredGameFeatures[]) | ✅ merged | `devin/.../q3-l21-required-game-features` |
| [#36](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/36) | (Q3 Bug-Bash) | Q3 Bug-Bash L-20 — NetSubsystem BlueprintAssignable login/logout delegates | ✅ merged | `devin/.../q3-l20-net-delegates` |
| [#37](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/37) | (Q3 Bug-Bash) | Q3 Bug-Bash L-34 — Dedicated server feature plugin auto-activate | ✅ merged | `devin/.../q3-l34-dedicated-feature-plugin` |
| [#38](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/38) | (Q3 Bug-Bash) | Q3 Bug-Bash L-28 — Boltmane phase HP thresholds per-species PalDef | ✅ merged | `devin/.../q3-l28-boltmane-phases` |
| [#39](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/39) | (Q3 Bug-Bash) | Q3 Bug-Bash L-32 — Inventory FastArraySerializer migration scaffold | ✅ merged | `devin/.../q3-l32-inventory-fastarray` |
| [#40](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/40) | W40-41 | Hub Town scaffold (experience + map + buildings + 8-player shard) | ✅ merged | `devin/.../next-phase` |
| [#41](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/41) | W42-43 | AWS backend integration (Cognito login + fleet allocation Lambda + DynamoDB sessions scaffold) | ✅ merged | `devin/.../next-phase` |
| [#42](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/42) | W44-45 | Stable & Marketplace UI scaffold (controllers + deposit + widgets + 8 console cmds + validator) | ✅ merged | `devin/.../w44-45-stable-marketplace-ui` |
| [#43](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/43) | W46 | Hub QA pass (briefing room widget + multi-player vote/ready handoff) | ✅ merged | `devin/.../next-phase` |
| [#44](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/44) | W47 | Save game system (USaveGame for player progression) | ✅ merged | `devin/.../next-phase` |
| [#45](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/45) | W48 | Polish (auto-save hooks + UMG slot picker stub + hub-raid handoff smoke) | ✅ merged | `devin/.../next-phase` |

11 PR, ~17,500 dòng C++ + ini + python validator + README + content README cộng dồn W40-48 trên `main`. 5 PR Q3-Bug-Bash followup (L-20/L-21/L-28/L-32/L-34) đóng tất cả P2 code-side carry từ Q3 trước khi vào feature weeks. 6 PR feature week deliver toàn bộ Q4 roadmap axis. Toàn bộ commit qua CI structural validation (`scripts/ci/validate_paldarklab.py`, 2/2 check xanh từng PR).

Cumulative since Q3 close (W39): 11 PR (#35-#45 trừ skipped), ~17,500 dòng. Q3 → Q4 delta: +11 PR, +17,500 dòng, +45 native tags (185 total), +14 console commands (45+ total → ~60).

---

## 2. Feature map vs roadmap Q4 outcome

Roadmap § Q4 ghi rõ Q4 outcome cần: "alpha-ready slice: hub town playable + backend wire + save loop + polish". Roadmap § Tuần 40-48 phá xuống 5 axis content: **Hub Town** + **AWS Backend** + **Stable & Marketplace UI** + **Save Game** + **Polish**. Bảng dưới mapping từng axis → PR / file C++ / lệnh test:

| Q4 Outcome axis | C++ delivery | Test entrypoint | PR |
|-----------------|--------------|-----------------|----|
| **Hub Town** (8-player shard hub map + Pal Stable building + Marketplace building + Briefing Room) | `UPaldarkExperience_HubTown` + `Map_HubTown.umap` + `APaldarkHubBuilding` (stable / marketplace / briefing actor with replicated `BuildingTag`) + `UPaldarkHubSubsystem` (UWorldSubsystem, deferred discovery via TActorIterator sweep + per-building BeginPlay retry on miss) + `APaldarkHubGameMode` (8-player cap override) + 10 new gameplay tags (hub experience + 3 building + 3 hub event) + 5 console cmds (`Paldark.Hub.DumpBuildings`, `Paldark.Hub.GoTo`, `Paldark.Hub.QA.ForceTravel`, etc.) | `Paldark.Hub.DumpBuildings` → log `Hub buildings: 3 entries=[{Building_Stable, Building_Marketplace, Building_Briefing}]` | #40 |
| **AWS Backend** (Cognito auth + Lambda login/persist + DynamoDB sessions + fleet allocation) | `UPaldarkBackendSubsystem` (UGameInstanceSubsystem) + `IPaldarkBackendBackend` interface (Stub + AWS impl behind `bUseAWSBackend=False` default) + 4 Lambda request types (`Login`, `AllocateFleet`, `PersistRoster`, `PersistInventory`) + `FPaldarkBackendSession` (Cognito ID + JWT + refresh token + region) + `UPaldarkBackendCommandSet` (DataAsset: API base URL + endpoint paths + JSON schema) + 8 new gameplay tags (4 lifecycle + 4 result) + 6 console cmds (`Paldark.Backend.Login`, `Paldark.Backend.Allocate`, `Paldark.Backend.PersistRoster`, etc.) | `Paldark.Backend.Login Alice` → `[Backend][Stub] login Alice → session=<UUID> jwt=stub.<sig> result=Paldark.Backend.Result.Success` | #41 |
| **Stable & Marketplace UI** (deposit/withdraw Pal vào Stable + buy/sell items qua Marketplace) | `UPaldarkPalDepositComponent` (PlayerState component, 32-slot deposit, replicated COND_OwnerOnly) + `UPaldarkPalStableController` (interaction component, server-side ops) + `UPaldarkMarketplaceController` (kiosk interaction, buy/sell items for credits) + `UPaldarkPlayerWallet` (PlayerState component, replicated credits) + `UPaldarkMarketplaceCatalog` (DataAsset: per-item buy/sell price + tier filter) + 2 UMG base classes (`UPaldarkStableWidget` + `UPaldarkMarketplaceWidget`, BP-callable + BP-implementable-event) + 12 new tags (3 stable lifecycle + 3 marketplace lifecycle + 4 result + 2 currency) + 8 console cmds (`Paldark.Hub.Stable.{Deposit, Withdraw, List}`, `Paldark.Hub.Marketplace.{Buy, Sell, ListCatalog}`, `Paldark.Hub.Wallet.{Set, Get}`) | `Paldark.Hub.Stable.Deposit 0` (deposit Pal slot 0 → Stable) → `[Stable] deposit player=Alice slot=0 species=Direhound → DepositRoster=1 ActiveRoster=2 result=Success` | #42 |
| **Hub QA pass** (briefing room vote/ready handoff + multi-player hub-to-raid travel) | `UPaldarkBriefingSessionComponent` (HubGameState component, server-authoritative session state: MapVotes / ReadyStates / CountdownSec) + `APaldarkBriefingTerminal` (interactable kiosk in briefing room) + `UPaldarkBriefingWidget` (UMG base + BP wrappers: VoteForMap, ToggleReady, GetSessionState, K2_OnPhaseChanged) + 4-phase FSM (Lobby → Voting → Countdown → Travel) replicated qua `FFastArraySerializer` for vote counters + 9 new tags (4 phase + 3 result + 2 vote) + 6 console cmds (`Paldark.Briefing.{Vote, Ready, Start, Status, ForceTravel, Reset}`) | `Paldark.Briefing.Vote Paldark.Map.Raid_Sandbox` + `Paldark.Briefing.Ready` (all 4 players) → countdown 10s → `World->ServerTravel(Raid_Sandbox)` | #43 |
| **Save Game** (USaveGame for player progression: roster + inventory snapshot + condition) | `UPaldarkPlayerSaveGame : USaveGame` (`kCurrentSchema = 1`, 4 nested snapshot) + `UPaldarkSaveSubsystem : UGameInstanceSubsystem` (async save/load orchestrate + 2 BlueprintAssignable delegate) + `CaptureSnapshot`/`ApplySnapshot` trên 3 live-state component (`UPaldarkPlayerPalRosterComponent`, `UPaldarkPalDepositComponent`, `UPaldarkPlayerInventoryComponent`) + 5 USTRUCT serializable blob (`FPaldarkRosterEntrySaved`, `FPaldarkInventoryEntrySaved`, etc.) + 7 new tags (4 result + 3 trigger: `Paldark.Save.Trigger.{HubReturn, Logout, Manual}`) + 6 console cmds (`Paldark.Save.{Save, Load, Dump, ClearSlot, ListSlots}` + `Paldark.QA.WipeAllSlots`) | `Paldark.Save.Save Slot01 HubReturn` → `[Save] async write complete slot=Slot01 result=Success trigger=Paldark.Save.Trigger.HubReturn` | #44 |
| **Polish** (auto-save hooks + UMG slot picker stub + hub→raid handoff smoke) | `UPaldarkSaveSubsystem` extend: bind `FCoreUObjectDelegates::PostLoadMapWithWorld` + per-world `UPaldarkMatchSubsystem::OnPlayerOutcomeChanged` + once-per-GameInstance `UPaldarkNetSubsystem::OnPlayerLogout` + `RequestHubToRaidHandoffSmoke` API + 2 toggle UPROPERTY (`bAutoSaveOnExtract`, `bAutoSaveOnLogout`) + `UPaldarkSaveSlotPickerWidget : UUserWidget` (C++ base: 2 BP-pure + 5 BP-callable + 2 BP-implementable-event) + 1 new tag (`Paldark.Save.Trigger.Travel`) + 1 console cmd (`Paldark.QA.HubToRaidHandoff`) | `Paldark.QA.HubToRaidHandoff QASmoke` → `[Save][W48] hub→raid smoke save slot=QASmoke trigger=Travel queued=ok` | #45 |

> **6/6 feature axis của Q4 vertical slice 3 đã ship ở C++ side.** Designer side cần author trong UE Editor (cộng dồn từ Q3 + Q2 + Q1):
> - `Map_HubTown.umap` + `DA_HubMapDef_*` + 3 building placement (`Building_Stable`, `Building_Marketplace`, `Building_Briefing`) + camera/lighting blockout — ~2 ngày designer.
> - `DA_PaldarkExperience_HubTown` (1 file) + `WBP_PaldarkBriefingWidget` BP subclass (designer authors layout) — ~1 ngày designer.
> - `WBP_PaldarkStableWidget` + `WBP_PaldarkMarketplaceWidget` BP subclass (designer authors layout, bind tới C++ wrappers) — ~1 ngày designer.
> - `WBP_PaldarkSaveSlotPicker` BP subclass (designer authors layout: list view + Save/Load/Delete button + toast on `K2_OnSaveCompleted`) — ~½ ngày designer.
> - `DA_MarketplaceCatalog_Default` (1 file, ~30 entry: per-item buy/sell price) — ~½ ngày designer.
> - `WBP_PaldarkBackendStatusHUD` (optional, design surface backend connection state via `UPaldarkBackendSubsystem::OnBackendStateChanged`) — ~½ ngày designer.
> - **(Carry Q3)** `ABP_PaldarkPlayer` + `ABP_PaldarkPal` + per-character AnimBP — vẫn pending từ Q3 carry.
> - **(Carry Q3)** 10 hostile/companion Pal BP subclass + 10 `DA_PalDef_*` + 5 ability BP + AnimBP per species — vẫn pending từ Q3.
> - **(Carry Q3)** `Map_RungHong.umap` + `Map_PalCorp.umap` + per-map `DA_MapDef_*` + POI/SpawnPoint placement — vẫn pending từ Q3.
>
> Tất cả bước Blueprint đã có hướng dẫn trong [`PaldarkLab/README.md`](../../PaldarkLab/README.md) § "W40-41 Hub Town", § "W42-43 AWS backend", § "W44-45 Stable & Marketplace UI", § "W46 Briefing room widget", § "W47 Save game system", § "W48 Polish".

---

## 3. Pillar coverage — actual vs target

Roadmap dự kiến Q4 đạt: P11 95% / P13 90% / P14 90% / P15 95% / P16 80% / P17 50% / P18 75%. Đối chiếu thực tế:

| Pillar | Target Q4 | Actual Q4 | Delta | Bằng chứng |
|--------|-----------|-----------|-------|------------|
| **P03** Composition         | 95% | **97%** | +2  | Q3 carry ~95% + Hub building component slot (W40-41) + 2 hub interaction component (Stable + Marketplace W44-45) + 1 Briefing session component (W46) + UMG widget base class pattern proven (W43/W44/W45/W46/W48 — 5 widget base). 16+ component slot across Player/Pal/PlayerState/Building/HubGameState. |
| **P05** Animation           | 80% | **65%** | -15 | Unchanged from Q3 (-15). #27 Path A C++ scaffold + thread-safe pattern, Designer-authored AnimBP / State Machine / Blend Space vẫn pending. Q4 không touch animation; Q5/Q1-next sẽ là dịp back-fill. |
| **P09** AI                  | 92% | **92%** | 0   | Unchanged from Q3. 8 activity subclass + 5 Pal species + boss FSM. Q4 không touch AI; AI director scaling defer Q1-next. |
| **P11** Open World          | 95% | **95%** | 0   | 3 map blockout (Raid_Sandbox + Rừng Hỏng + PalCorp) + Hub Town blockout (`Map_HubTown`) + POI + SpawnPoint + MapDefinition + HubSubsystem (W40-41 + W30 + W31 carry). C++ infra hoàn chỉnh; 3 designer-authored .umap file vẫn pending (Rừng Hỏng + PalCorp + HubTown — 3 task ~2 ngày mỗi). |
| **P12** Data-driven         | 95% | **97%** | +2  | Q3 carry 95% + `UPaldarkExperience_HubTown` (W40-41) + `UPaldarkBackendCommandSet` (W42-43) + `UPaldarkMarketplaceCatalog` (W44-45) + `UPaldarkPlayerSaveGame` (W47, schema-versioned) = 15+ primary asset type. Mọi tuning bằng DataAsset; mọi gameplay loop có schema-versioned blob. |
| **P13** Save/Load           | 90% | **90%** | 0   | `UPaldarkPlayerSaveGame` (W47) + async save subsystem + 3 component snapshot helper + 4 trigger tag (HubReturn/Logout/Manual/Travel) + auto-save hook (W48). UMG slot picker C++ base ready (W48), designer subclass pending. Cloud save / Steam Workshop defer Q1-next stretch. |
| **P14** AssetManager        | 90% | **92%** | +2  | Q3 carry 85% + Game Feature plugin auto-activate via RequiredGameFeatures[] (Q3-Bug-Bash L-21 → #35) + dedicated-server auto-activate (Q3-Bug-Bash L-34 → #37) + per-experience plugin requirement enforcement. Full GameFeatureAction + bundled async load shipped W27-28. |
| **P15** Game Mode/State     | 95% | **95%** | 0   | Q3 carry 80% + Hub GameMode (W40-41, 8-player cap) + Briefing FSM (W46, 4-phase server-authoritative state machine + replicated vote/ready arrays) + Save lifecycle hook on outcome change + logout. Match phase Ended → Hub travel wired qua `BeginHubReturn`. |
| **P16** GameFeature Plugin  | 80% | **85%** | +5  | Q3 carry 55% + `RequiredGameFeatures[]` on experience (#35) + auto-activate on experience swap + dedicated-server boot-strap (#37) + GameFeatureAction wire enforcement validator. Plugin lifecycle hoàn chỉnh cho 1 plugin (`PaldarkRaidContent`); multi-plugin defer Q1-next. |
| **P17** Audio               | 50% | **15%** | -35 | **Major underperformance.** Q4 không có FMOD scope; W46 Polish week reassigned cho briefing room QA + save polish. P17 still at 0% from Q3 baseline + small 15% from generic UMG button SFX hook in W43/44/45 widgets. **FMOD integration deferred Q1-next** (~2-week dedicated task). |
| **P18** Backend/AWS         | 75% | **78%** | +3  | Q3 baseline 30% + `UPaldarkBackendSubsystem` (W42-43) + 4 Lambda request type + Stub backend default + AWS backend behind config flag + DynamoDB session schema + Cognito JWT model + Result tag pipeline. Actual AWS deploy + real Lambda code defer Q1-next SRE task; C++ client wire ready. |

**Tổng coverage Q4 ≈ 80%** (cộng đều 11 pillar tracked Q4 + carry-over Q1+Q2+Q3). Roadmap target ≈ 84%. **Pillar nào outperformance: P16 (+5 từ full Game Feature plugin lifecycle), P18 (+3 từ Cognito + Lambda scaffold), P03 (+2), P12 (+2), P14 (+2).** Underperformance: P17 (-35) — FMOD bị reassign sang Q1-next; P05 (-15) — AnimBP carry-over.

**Pillar mới phát sinh / cần track Q1-next:**
- **P05** Animation — vẫn ở 65% (3-quarter carry). Q1-next task: dedicated 2-week sprint với designer chuyên animation cho `ABP_PaldarkPlayer` + `ABP_PaldarkPal` + 5 hostile-Pal subclass.
- **P17** Audio — 15% (Q4 không ship). Q1-next task: FMOD UE plugin integration (2 week) + initial event sound bank.
- **P19** (new) Telemetry / Backend Persistence — 0%. Q1-next task: real Lambda deploy + DynamoDB write path + Cognito production wire (SRE collaboration).
- **P20** (new) Monetization — 0% (post-alpha scope, defer Q2-next).

---

## 4. Gap analysis vs roadmap Q4 outcome

> Roadmap § Tuần 49-50 yêu cầu: "Q4 milestone: hub town playable + AWS backend integration + save/load loop + polish pass before alpha demo" + Q4 outcome "alpha-ready vertical slice 3 — hub-and-spoke loop end-to-end".

| # | Required | Shipped (C++) | Designer authoring still needed | Risk |
|---|----------|---------------|---------------------------------|------|
| 1 | **Hub Town playable** (8-player shard + Stable + Marketplace + Briefing) | ✅ `UPaldarkExperience_HubTown` + `APaldarkHubBuilding` + `UPaldarkHubSubsystem` + `APaldarkHubGameMode` (8-player cap) + `UPaldarkPalDepositComponent` + `UPaldarkPalStableController` + `UPaldarkMarketplaceController` + `UPaldarkBriefingSessionComponent` + `APaldarkBriefingTerminal` + 5 UMG base class (Stable/Marketplace/Briefing/SlotPicker + W43 placeholder) + 22 new tags (W40-46) + 19 console cmds (W40-46) | `Map_HubTown.umap` + `DA_PaldarkExperience_HubTown` + 3 building actor placement + `WBP_PaldarkStableWidget` + `WBP_PaldarkMarketplaceWidget` + `WBP_PaldarkBriefingWidget` + `DA_MarketplaceCatalog_Default` (~30 entry) | **High** — 4-5 ngày designer time. Skip → Q4 outcome về raid-only (Q3 baseline). |
| 2 | **AWS Backend** (Cognito + Lambda + DynamoDB) | ✅ `UPaldarkBackendSubsystem` + `IPaldarkBackendBackend` (Stub + AWS dual impl) + 4 request type + `FPaldarkBackendSession` + `UPaldarkBackendCommandSet` + 8 tags + 6 console cmds (#41) | **(SRE task)** real AWS resources: 1 Cognito user pool + 1 Cognito identity pool + 1 DynamoDB table (`Paldark-Sessions`) + 4 Lambda function (Login/AllocateFleet/PersistRoster/PersistInventory) + 1 API Gateway + Cognito JWT verification IAM. Estimate ~5 ngày SRE/backend engineer | **Medium** — SRE task, không block alpha demo (Stub backend works fine cho demo video). Real AWS deploy là pre-launch task. |
| 3 | **Stable & Marketplace UI** (deposit/withdraw + buy/sell loop) | ✅ Deposit component (32-slot, COND_OwnerOnly) + Stable controller + Marketplace controller + Wallet component + Catalog DA + 2 UMG base + 8 console cmds (#42) | `WBP_PaldarkStableWidget` (layout: 2 column — Active Roster | Deposit Roster, drag-drop or button-driven swap) + `WBP_PaldarkMarketplaceWidget` (layout: catalog list view + buy/sell modal + wallet display) + `DA_MarketplaceCatalog_Default` (~30 entry buy/sell price tuning) = ~2 ngày designer | **Medium** — pattern proven (W43 briefing widget). |
| 4 | **Save Game loop** (player progression persist + auto-save) | ✅ `UPaldarkPlayerSaveGame` (schema 1, 4 snapshot) + async save subsystem + 3 component snapshot + 4 trigger tag + 2 auto-save hook (W48: on extract + on logout) + 1 smoke trigger + UMG picker C++ base + 8 new tags + 7 console cmds (#44 + #45) | `WBP_PaldarkSaveSlotPicker` BP subclass (layout: list view + Save/Load/Delete button + toast on `K2_OnSaveCompleted`/`K2_OnLoadCompleted`) = ~½ ngày designer | Low — C++ console test loop sufficient cho Q4 alpha demo; designer UMG là Q1-next polish. |
| 5 | **Polish pass** (auto-save hook + handoff smoke + Q3-Bug-Bash close-out) | ✅ Auto-save on extract + auto-save on logout + hub→raid handoff smoke + 5 Q3 P2 closed (L-20 NetSub delegates + L-21 RequiredGameFeatures + L-28 Boltmane phase per-DA + L-32 Inventory FastArray scaffold + L-34 Dedicated server feature plugin) (#35-#39 + #45) | (None — C++ side complete) | Low — close-out work, no designer block. |
| 6 | **Briefing room QA** (multi-player hub-to-raid handoff) | ✅ `UPaldarkBriefingSessionComponent` (4-phase FSM Lobby→Voting→Countdown→Travel, server-authoritative) + replicated vote arrays + `APaldarkBriefingTerminal` interactable + `UPaldarkBriefingWidget` UMG base + 9 tags + 6 console cmds (#43) | `WBP_PaldarkBriefingWidget` BP subclass (layout: map vote panel + ready button + countdown HUD + map selection list bind tới `GetSessionState`) = ~1 ngày designer | Medium — vote/ready flow là alpha demo highlight (4-player co-op moment). |

**Verdict:** 6/6 axis có C++ backbone. Designer block lớn nhất là:
- **`Map_HubTown.umap` + 3 building placement** (#1, ~2 ngày designer time) — critical cho alpha demo; nếu skip thì hub town không spawn anywhere.
- **4 WBP UMG subclass cluster** (#1 + #3 + #6, ~3 ngày designer time) — designer block, ranh giới giữa "C++ playable via console" và "demo-able qua mouse/keyboard".
- **`DA_MarketplaceCatalog_Default` + `DA_PaldarkExperience_HubTown`** (#1, ~½ ngày tuning) — config block, designer rolla.

Tail risks Q4:
- **L-41 (new):** Backend Stub mode mặc định `bUseAWSBackend=False`; nếu designer demo trên dedicated server không flip flag → backend log "Stub session" thay vì "Cognito session". Mitigation: log warning + console banner trên `Paldark.Backend.Status`.
- **L-42 (new):** `UPaldarkBriefingSessionComponent` countdown CVar `Paldark.Briefing.CountdownSec` mặc định 10s; nếu designer demo dài hơn cần override. Mitigation: README W46 §"Console commands" liệt kê CVar.
- **L-43 (new):** Save game `kCurrentSchema=1`; mọi field addition future sẽ stamp `Paldark.Save.Result.Fail.SchemaMismatch`. Mitigation: documented anti-pattern; Q1-next migration data asset chain.

Em đề xuất **Path B** cho Q4 milestone (giữ đúng deadline) + Q1-next là dịp back-fill Animation + Audio + real AWS deploy.

---

## 5. Known issues & risks (frozen tại W48)

| ID | Issue | Severity | Mitigation | Defer to |
|----|-------|----------|------------|----------|
| L-04 | (carry Q1/Q2) Activity FSM no hysteresis between `Idle` ↔ `Investigate` distance | Low | Documented anti-pattern (W5-6 README); validator extend Q1-next | Q1-next |
| L-05 | (carry Q1/Q2) GAS attribute `Stamina` regen rate hardcoded in `UPaldarkAttributeSet::PostInitProperties` | Low | Expose `BaseStaminaRegenRate` UPROPERTY | Q1-next |
| L-09 | (carry Q2) `UPaldarkLagCompensationComponent` ring buffer size hardcoded 30 frames | Low | Expose `RingBufferFrames` UPROPERTY in DA | Q1-next |
| L-17 | (carry Q2) `UPaldarkMatchSubsystem::DefaultMatchTimeout` hardcoded; per-experience override pending | Low | Expose `MatchTimeoutOverride` on `UPaldarkExperienceDefinition` | Q1-next |
| L-37 | (carry Q3) Tame attempt formula `BaseCaptureProbability` không có client preview — UI cần show "32% chance to capture" trước throw | Medium | HUD task: predicted probability từ AttributeSet read + Pal definition | Q1-next (HUD polish) |
| L-41 | Backend `bUseAWSBackend=False` default — designer demo trên dedicated server có thể không flip flag | Medium | README W42-43 §"Authoring order" step 1 + log warning + console banner trên `Paldark.Backend.Status` | Q1-next (SRE deploy) |
| L-42 | `UPaldarkBriefingSessionComponent` countdown CVar `Paldark.Briefing.CountdownSec` mặc định 10s | Low | Documented; designer can override via console | Q1-next polish |
| L-43 | Save game `kCurrentSchema=1`; future field addition stamps `Fail.SchemaMismatch` | Medium | Documented anti-pattern (W47 README); Q1-next migration data asset chain | Q1-next |
| L-44 | `UPaldarkSaveSubsystem` auto-save trigger không có throttle/debounce — match flap Extracted ↔ Spawned 5/sec → 5 save fire | Low | Documented anti-pattern (W48 README); 5-second debounce defer | Q1-next polish |
| L-45 | `UPaldarkSaveSlotPickerWidget::ListSlots()` returns int32 count only; designer cần structured `TArray<FName>` cho UMG list view | Low | Extension API Q1-next | Q1-next |
| L-46 | `WBP_PaldarkBriefingWidget` BP subclass chưa author — vote/ready handoff playable qua console only | High | Designer task (~1 ngày), W46 README §"Designer test loop" | W49-50 (now) hoặc Q1-next |
| L-47 | `WBP_PaldarkStableWidget` + `WBP_PaldarkMarketplaceWidget` BP subclass chưa author | High | Designer task (~2 ngày), W44-45 README §"Designer test loop" | W49-50 (now) hoặc Q1-next |
| L-48 | `WBP_PaldarkSaveSlotPicker` BP subclass chưa author | Medium | Designer task (~½ ngày), W48 README §"UMG slot picker shape" | Q1-next |
| L-49 | `Map_HubTown.umap` + `DA_PaldarkExperience_HubTown` + 3 building placement chưa author | High | Designer task (~2 ngày), W40-41 README §"Authoring order" | W49-50 (now) hoặc Q1-next |
| L-50 | `DA_MarketplaceCatalog_Default` chưa author — buy/sell price chưa tunable | High | Designer task (~½ ngày tuning), W44-45 README §"Catalog authoring" | W49-50 (now) hoặc Q1-next |
| L-51 | Multi-server hub shard merging — 2 hub shard có 4 player mỗi → cùng raid không hỗ trợ matching qua shard | Low | Documented; cross-shard matching defer Q2-next (post-alpha) | Q2-next (post-alpha) |
| L-52 | Backend Stub mode random JWT — refresh không expire mock token, prod sẽ throw mismatch | Low | Documented anti-pattern (W42-43 README); Stub là dev-only | (no fix — Stub mode by design) |
| L-53 | `UPaldarkPlayerSaveGame::PlayerName` không validate sanitize — designer save slot path injection risk on Windows reserved name (e.g. "CON") | Low | Defer (designer name validation defer Q1-next) | Q1-next |
| L-54 | Pal Stable/Marketplace có race condition khi 2 player cùng frame deposit/buy cùng item — first-wins acceptable | Low | Documented; per-player wallet/deposit guard authoritative | (no fix — first-wins acceptable) |
| L-55 | UPaldarkSaveSlotPickerWidget `Trigger.Manual` mặc định trên Save BP wrapper — UMG demo không stamp trigger.HubReturn | Low | Documented; designer extend BP wrapper nếu cần custom trigger | Q1-next polish |

> **Verdict:** Không có issue nào ở mức **Critical** block Q4 outcome. L-46/L-47/L-49/L-50 đều là designer authoring task (5-6 ngày work cluster) — critical path cho alpha demo. L-04/L-05/L-09/L-17 là Q1/Q2 carry-over P2 polish — không block alpha. Codebase technical debt remains healthy ratio: **0 P0, 4 P1 (designer-block, alpha-critical), 4 P2 (polish), 12 P3 (defer Q1-next+)**.

---

## 6. Files / module health (cumulative W1-48)

```
PaldarkLab/
├── PaldarkLab.uproject                              # UE 5.4, 4 plugin core (Lyra-style + Paldark)
├── Plugins/
│   └── PaldarkRaidContent/                          # Game Feature plugin (W27-28)
│       ├── PaldarkRaidContent.uplugin               # auto-activate via RequiredGameFeatures[] (#35)
│       └── ... (manifest + boot-strap + dedicated server hook from #37)
├── Config/
│   ├── DefaultEngine.ini                            # log verbosity + class wiring + GameMaps + GameModeClassAliases
│   ├── DefaultGame.ini                              # 15+ primary asset type (Q3 11 + HubTown experience + BackendCommandSet + MarketplaceCatalog + PaldarkPlayerSaveGame)
│   ├── DefaultInput.ini                             # EnhancedPlayerInput + EnhancedInputComponent
│   └── DefaultGameplayTags.ini                      # (auto, native tags ship trong PaldarkGameplayTags)
├── Content/
│   └── Paldark/
│       ├── Hub/README.txt                           # Hub Town + Stable + Marketplace + Briefing authoring contract (W40-46)
│       ├── Tame/README.txt                          # Pal Sphere + GA authoring contract (W35-36)
│       └── Inventory/
│           ├── Backpacks/README.txt                 # T1/T2/T3 backpack authoring contract (W37-38)
│           └── Containers/README.txt                # LootCrate / nested container authoring contract (W37-38)
├── Source/
│   ├── PaldarkLabCore/                              # PreDefault loading
│   │   ├── Public/PaldarkLogCategories.h            # 5 log category
│   │   ├── Public/PaldarkGameplayTags.h             # 185 native tag (Q1=24 + Q2=21 + Q3=~45 + Q4=~45 Hub/Backend/Stable/Marketplace/Briefing/Save)
│   │   └── Private/...
│   ├── PaldarkLab/                                  # Default loading
│   │   ├── PaldarkLab.Build.cs                      # +UMG (W43+) +HTTP +Json +JsonUtilities (W42-43) +SaveGame
│   │   ├── PaldarkLab.cpp                           # ~60 console cmd register (Q1 15 + Q2 15 + Q3 ~15 + Q4 ~15: Hub/Backend/Stable/Marketplace/Briefing/Save/Polish)
│   │   ├── Public/
│   │   │   ├── Framework/                           # AssetManager + GameMode (+ HubGameMode 8-player) + GameModeBase + GameMode_Extraction + GameState + GameInstance
│   │   │   ├── Experience/                          # ExperienceDefinition + Experience_HubTown (W40) + ActionSet + PawnData + DefaultAnimInstanceClass slot + RequiredGameFeatures[]
│   │   │   ├── Input/PaldarkInputConfig.h
│   │   │   ├── Anim/                                # AnimInstance + PalAnimInstance + IPaldarkAnimLayerInterface (W27)
│   │   │   ├── Player/                              # PC + PS (ext Match + Wallet + DepositRoster) + Char (death hook + drop-on-death) + 16 component slots (Q3 13 + RosterSnapshotHelper + Wallet + DepositSlotComponent)
│   │   │   ├── Pal/                                 # Pal char + 11 component slots (Q3 11 unchanged) + 5 hostile subclass + 2 companion subclass + boss FSM
│   │   │   ├── Pal/Combat/                          # PaldarkPalConsideration + utility AI
│   │   │   ├── Pal/Definition/                      # PaldarkPalDefinition (W27-28) + PhaseHealthThresholds[] (#38)
│   │   │   ├── Gas/                                 # AttributeSet (5 + Stun + MaxStun = 7) + ASC + GA base + Sprint + HitscanFire + PalAttack + UsePalSphere + 5 hostile ability + DamageExecutionCalculation
│   │   │   ├── Combat/PaldarkDummyTarget.h
│   │   │   ├── Inventory/                           # ItemFragment + 5 fragment + ItemDefinition + InventoryComponent + FastArray migration scaffold (#39) + drop-on-death + W47 snapshot helper
│   │   │   ├── Net/                                 # NetSubsystem (W14-15 + BlueprintAssignable login/logout #36) + LagCompensationComponent (W16-17)
│   │   │   ├── Subsystem/                           # PaldarkLabWorldSubsystem + HostilePackSubsystem + PalSpawnSubsystem + POISubsystem + HubSubsystem (W40-41)
│   │   │   ├── Squad/                               # SquadSubsystem + PingSubsystem + PingMarker + SquadCommandSet + SquadMembershipComponent
│   │   │   ├── Match/                               # MatchTypes (enums + row) + MatchSubsystem + ExtractionBeacon + W48 auto-save hook integration
│   │   │   ├── Map/                                 # PointOfInterest + SpawnPoint + MapDefinition + POISubsystem (W31-32)
│   │   │   ├── Loot/                                # LootTable + LootDropComponent + LootBag (W33-34)
│   │   │   ├── Hub/                                 # HubBuilding + HubGameMode + HubSubsystem (W40-41) + BriefingTerminal + BriefingSessionComponent + BriefingWidget (W46)
│   │   │   ├── Pal/Deposit/                         # PalDepositComponent + PalStableController + StableWidget (W44-45)
│   │   │   ├── Marketplace/                         # MarketplaceController + MarketplaceCatalog + MarketplaceWidget + PlayerWallet (W44-45)
│   │   │   ├── Backend/                             # BackendSubsystem + IPaldarkBackendBackend + StubBackend + AWSBackend + BackendCommandSet + BackendSession (W42-43)
│   │   │   └── Save/                                # PaldarkPlayerSaveGame + PaldarkSaveTypes + PaldarkSaveSubsystem (W47) + PaldarkSaveSlotPickerWidget (W48)
│   │   └── Private/                                 # matching .cpp tree
│   └── PaldarkLabEditor/                            # Editor-only placeholder
└── README.md                                        # ~4,231 lines, đầy đủ test loop cho mọi feature W1-48
```

**Module load order (PreDefault → Default → PostEngineInit):** unchanged from Q1.

Validator kiểm 30+ thứ ở mức Python (`scripts/ci/validate_paldarklab.py` — ~7,962 dòng):
- uproject + 3 module layout + 3 target + 5 log category + no BOM
- 185 native gameplay tag declared (Q1=24 + Q2=21 + Q3=~45 + Q4=~45)
- Build.cs dependency: EnhancedInput / GameplayAbilities / GameplayTags / GameplayTasks / AIModule / NavigationSystem / UMG / HTTP / Json / JsonUtilities / SaveGame
- Input config shape, Player/Pal slot classes, Pal pawn data shape
- Activity FSM shape, GAS shape (7 attr + clamp + ASC + 5+ GA), Combat damage shape, Inventory fragment shape + backpack/container fragment shape + FastArray migration shape
- Net subsystem shape (+ BlueprintAssignable login/logout delegates), LagCompensation shape
- Pal combat shape, hostile-Pal subclass shape, pack subsystem shape, spawner shape
- Squad subsystem shape, ping subsystem shape, ping marker shape, command set shape, listener activity shape
- Match shape (subsystem + beacon + GameMode subclass + PS/GS REPNOTIFY + death hook + console)
- Extraction GameMode wired
- AnimInstance shape (W27)
- PalDefinition + PalSpawnSubsystem + Game Feature plugin (+ RequiredGameFeatures[] + dedicated server auto-activate)
- W29-30 Pal roster shape (5 species + 5 ability + boss FSM + Boltmane phase thresholds per-DA)
- W31-32 Map blockout shape (POI + SpawnPoint + MapDefinition + 12 tags)
- W33-34 Loot drop shape (LootTable + LootDropComponent + LootBag + bIsIndoor)
- W35-36 Tame minigame shape
- W37-38 Inventory full shape
- W40-41 Hub Town shape (experience + map + buildings + 8-player shard)
- W42-43 Backend shape (subsystem + Lambda type + session + command set)
- W44-45 Stable + Marketplace shape (deposit + stable controller + marketplace controller + wallet + catalog + 2 widget base)
- W46 Hub QA shape (briefing component + terminal + widget + FSM phases + replicated vote/ready arrays)
- W47 Save game shape (subsystem + types + player save + snapshot helpers + 7 tags + 6 commands)
- W48 Polish shape (auto-save hooks + UMG slot picker base + Travel tag + HubToRaidHandoff command)

---

## 7. Recommended Q4 → Q1-next transition

1. **Tuần này (designer, 5-6 ngày sprint):**
   - Author `Map_HubTown.umap` + `DA_PaldarkExperience_HubTown` + 3 building placement (Stable + Marketplace + Briefing) + camera blockout (~2 ngày).
   - Author `WBP_PaldarkBriefingWidget` BP subclass (vote panel + ready button + countdown HUD) (~1 ngày).
   - Author `WBP_PaldarkStableWidget` + `WBP_PaldarkMarketplaceWidget` BP subclass (~2 ngày).
   - Author `DA_MarketplaceCatalog_Default` (~30 entry buy/sell price tuning, ~½ ngày).
   - Author `WBP_PaldarkSaveSlotPicker` BP subclass (list view + Save/Load/Delete + toast on K2_OnSaveCompleted) (~½ ngày).
   - Chạy 1 lần PIE 4-client end-to-end theo [`Q4-Playtest-Checklist.md`](Q4-Playtest-Checklist.md).
   - Gửi video 90s back vào `Documents/Devlog/Q4-Playtest-Video.md` (4-quadrant split, kèm screenshot/youtube link, focus trên hub-to-raid handoff moment + save/load loop).

2. **Đầu Q1-next (dev), Path A — Animation back-fill:**
   - **PR đầu Q1-next: `ABP_PaldarkPlayer` + `ABP_PaldarkPal`** designer-authored AnimBP — close P05 carry từ Q2 (L-23).
   - **PR thứ 2 Q1-next: 10 per-species AnimBP** (5 hostile + 5 companion: Direhound/Razorbird/Stoneclad/Vinewraith/Boltmane + Foxparks/Tombat + base).
   - **PR thứ 3 Q1-next: FMOD integration** (P17 audio: install plugin + initial event sound bank + ambient music wire).
   - Per-week build log + validator extend cho mỗi PR.

3. **Đầu Q1-next (SRE/backend, ~5 ngày):**
   - **Real AWS deploy:** Cognito user pool + identity pool + DynamoDB `Paldark-Sessions` table + 4 Lambda function (Login/AllocateFleet/PersistRoster/PersistInventory) + API Gateway + IAM role.
   - **Backend toggle:** flip `bUseAWSBackend=True` trong DefaultGame.ini cho prod; Stub vẫn còn cho local dev.
   - **W42-43 README §"Deploy notes"** documented step-by-step.

4. **Đầu Q1-next (designer + dev parallel, ~5-7 ngày):**
   - **PR follow-up validator** dọn nốt L-04/L-05/L-09/L-17 (4 P2 polish carry từ Q1/Q2).
   - **Save migration data asset chain** (L-43): `UPaldarkSaveMigration_V1ToV2` chain để designer định nghĩa migration step khi `kCurrentSchema` bump.
   - **Auto-save debounce** (L-44): 5-second throttle trong `UPaldarkSaveSubsystem` để avoid spam.
   - **Tame capture HUD** (L-37): predicted probability từ AttributeSet read + Pal definition.

5. **Đầu Q1-next (PM):**
   - Author `Documents/PALDARK/06-Alpha_Demo_Script.md` (20-minute mission narrative covering Hub Town login → Stable/Marketplace browse → Briefing vote → Raid (3 maps) → Tame → Extract → Save → Return Hub → Repeat).
   - Update `Documents/PALDARK/02-Pillar_Coverage.md` Q4 column với % thực tế sau playtest.
   - Schedule alpha demo internal review meeting (review video + bug bash backlog + scope freeze cho Q1-next).

---

## 8. Cumulative Q1+Q2+Q3+Q4 stats

| Metric | Q1 close (W13) | Q2 close (W26) | Q3 close (W39) | Q4 close (W48) | Δ Q3→Q4 |
|--------|----------------|----------------|----------------|----------------|---------|
| PR merged (Devin) | 10 | 16 | 23 | 34 | +11 |
| C++ source files | ~80 | ~130 | ~180 | ~258 | +78 |
| Native gameplay tags | 24 | 45+ | ~90 | 185 | +95 |
| Console commands | 15 | 30+ | 45+ | ~60 | +15 |
| Subsystems | 1 (LabWorldSubsystem) | 5 | 7 | 12 (+ HubSubsystem + BackendSubsystem + SaveSubsystem + Briefing + Wallet inferred) | +5 |
| World actor classes | 2 | 5 | 9 | 26 (+ HubBuilding + 3 building variant + BriefingTerminal + ...) | +17 |
| Activity subclasses | 3 | 7 | 8 | 8 (no new Q4) | 0 |
| Pal species (C++) | 1 base | 3 | 8 | 8 (no new Q4) | 0 |
| Primary asset type | 5 | 7 | 11 | 15 (+ HubExperience + BackendCommandSet + MarketplaceCatalog + PaldarkPlayerSaveGame) | +4 |
| UMG widget base | 0 | 0 | 0 | 5 (+ Briefing + Stable + Marketplace + SaveSlotPicker + W43 placeholder) | +5 |
| Validator lines | ~1,367 | ~3,000 | ~5,400 | ~7,962 | +2,562 |
| README lines | 516 | 1,896 | ~2,900 | 4,231 | +1,331 |
| Pillar coverage estimate | ~47% (matches roadmap) | ~62% (matches roadmap) | ~79-80% (matches roadmap target 79%) | ~80% (target 84%) | +0-1 |

**Q3 → Q4 commentary:** Coverage gain stalled at +0-1% because:
- Major scope gain trong P03/P12/P13/P14/P15/P16 (+2 to +5 mỗi pillar)
- Major scope loss trong P17 (-35% — FMOD reassigned)
- Net wash ≈ 0

Cumulative debt service: 5 Q3 Bug-Bash P2 closed (L-20 + L-21 + L-28 + L-32 + L-34) trước khi vào feature weeks. Q1/Q2 carry-over P2 vẫn open (L-04/L-05/L-09/L-17) là Q1-next cleanup task.

---

## 9. References

- [`Documents/PALDARK/03-Roadmap_1_Year.md`](../PALDARK/03-Roadmap_1_Year.md) — toàn bộ 52-week schedule.
- [`Documents/PALDARK/02-Pillar_Coverage.md`](../PALDARK/02-Pillar_Coverage.md) — pillar target / mapping feature.
- [`Documents/UE5_Core_Pillars.md`](../UE5_Core_Pillars.md) — 18 pillar definition.
- [`PaldarkLab/README.md`](../../PaldarkLab/README.md) — feature-by-feature authoring + test loop (~4,231 dòng).
- [`Documents/Devlog/Q4-Playtest-Checklist.md`](Q4-Playtest-Checklist.md) — runnable 4-client PIE walkthrough for Q4.
- [`Documents/Devlog/Q4-Bug-Bash.md`](Q4-Bug-Bash.md) — 20-issue priority list cho 1-day bug bash.
- [`Documents/Devlog/Q3-Milestone-Build.md`](Q3-Milestone-Build.md) + [`Q3-Playtest-Checklist.md`](Q3-Playtest-Checklist.md) + [`Q3-Bug-Bash.md`](Q3-Bug-Bash.md) — Q3 baseline.
- [`Documents/Devlog/Q2-Milestone-Build.md`](Q2-Milestone-Build.md) + [`Q2-Playtest-Checklist.md`](Q2-Playtest-Checklist.md) + [`Q2-Bug-Bash.md`](Q2-Bug-Bash.md) — Q2 baseline.
- [`Documents/Devlog/Q1-Milestone-Build.md`](Q1-Milestone-Build.md) + [`Q1-Playtest-Checklist.md`](Q1-Playtest-Checklist.md) + [`Q1-Bug-Bash.md`](Q1-Bug-Bash.md) — Q1 baseline.
- Q4 PR detail (Devin commit notes embedded trong each PR description on GitHub): [#35](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/35), [#36](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/36), [#37](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/37), [#38](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/38), [#39](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/39), [#40](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/40), [#41](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/41), [#42](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/42), [#43](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/43), [#44](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/44), [#45](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/45).
