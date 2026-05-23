# PALDARK — Q4 Playtest Checklist (Tuần 49-50)

> **Source:** [`Documents/PALDARK/03-Roadmap_1_Year.md`](../PALDARK/03-Roadmap_1_Year.md) § Tuần 49-50.
>
> **Q4 Outcome verification target:** "alpha-ready vertical slice 3 — hub-and-spoke loop (Hub Town → briefing → raid → extract → save → return) end-to-end với 1 listen-server + 4-player co-op + persistent player progression."

This document is the **runnable 4-client PIE walkthrough** that closes out Q4. Designer + 3 internal testers cộng dồn Q1-Q4 setup, mở 4 PIE client (1 listen-server + 3 client), chạy 12 mục dưới end-to-end. Mỗi mục có expected console log để designer verify mà không cần screenshot.

VM không chạy UE5 — đây là designer-side document. C++ console test loop đã cover sẵn trong từng PR README (xem [`PaldarkLab/README.md`](../../PaldarkLab/README.md)); checklist này gộp lại thành flow alpha demo + thêm step UMG sau khi designer author WBP_* subclass.

---

## 0. Q4 setup delta (≈ 5-6 ngày designer — bổ sung vào Q3 setup)

> Q3 setup vẫn cần (xem [`Q3-Playtest-Checklist.md`](Q3-Playtest-Checklist.md) §0). Q4 thêm 6 nhóm asset author dưới đây.

### 0.1 Map_HubTown.umap + Experience HubTown (W40-41)

1. Create `Map_HubTown.umap` (sublevel hoặc level streaming optional) — ngoại cảnh hub town + ground plane.
2. Place 3 `APaldarkHubBuilding` actor trong map, mỗi cái wire `BuildingTag`:
   - Tag = `Paldark.Hub.Building.Stable` (placement near west of plaza)
   - Tag = `Paldark.Hub.Building.Marketplace` (placement near east of plaza)
   - Tag = `Paldark.Hub.Building.Briefing` (placement near north of plaza)
3. Create `DA_PaldarkExperience_HubTown` (subclass `UPaldarkExperience_HubTown`). Wire:
   - `RequiredGameFeatures` → empty (hub không cần raid content plugin)
   - `DefaultPawnData` → `DA_PawnData_Player_Hub` (optional dedicated hub pawn nếu spectator-style; else reuse `DA_PawnData_Player_Raid`)
   - `ActionSets` → empty or `DA_ActionSet_Hub` (interact + sprint)
4. Wire `DefaultEngine.ini` `[/Script/Engine.GameMapsSettings]` `GameDefaultMap=/Game/Maps/Map_HubTown.Map_HubTown` (cho dev convenience).

### 0.2 WBP_PaldarkBriefingWidget (W46)

1. Create BP subclass of `UPaldarkBriefingWidget` named `WBP_PaldarkBriefingWidget`.
2. Layout vertical panel:
   - Top: map vote list (ListView bind sang `GetSessionState().AllowedMaps`).
   - Middle: ready button (toggle, bind sang `ToggleReady()` BP wrapper).
   - Bottom: countdown HUD (TextBlock bind sang `GetSessionState().CountdownSec`).
3. Override `K2_OnPhaseChanged(NewPhase)` để switch panel visibility theo `Paldark.Briefing.Phase.{Lobby, Voting, Countdown, Travel}`.
4. Wire `WBP_PaldarkBriefingWidget` vào `APaldarkBriefingTerminal::DefaultWidgetClass` UPROPERTY.

### 0.3 WBP_PaldarkStableWidget + WBP_PaldarkMarketplaceWidget (W44-45)

1. **Stable widget:** subclass `UPaldarkStableWidget` → `WBP_PaldarkStableWidget`. Two-column layout: Active Roster | Deposit Roster. Button per row: "Deposit" (Active→Deposit) hoặc "Withdraw" (Deposit→Active). Override `K2_OnDepositCompleted(Slot, ResultTag)` cho refresh + toast.
2. **Marketplace widget:** subclass `UPaldarkMarketplaceWidget` → `WBP_PaldarkMarketplaceWidget`. Catalog list view bind sang `GetCatalog()->Entries`. Each row: item icon + name + Buy price + Sell price + qty. Modal "Buy" / "Sell" confirm. Wallet display top-right bind sang `GetPlayerWallet()->Credits`. Override `K2_OnTransactionCompleted(ResultTag)` cho refresh + toast.
3. Wire `WBP_PaldarkStableWidget` vào `APaldarkHubBuilding`'s Stable variant; `WBP_PaldarkMarketplaceWidget` vào Marketplace variant.

### 0.4 DA_MarketplaceCatalog_Default (W44-45)

1. Create `UPaldarkMarketplaceCatalog` DataAsset named `DA_MarketplaceCatalog_Default`.
2. Wire ~30 entry, mỗi entry:
   - `ItemDefinition` → soft pointer to existing DA (Pistol/Bandage/9mm/etc. — reuse W11-12 + W37-38 catalog).
   - `BuyPriceCredits` (e.g. Pistol = 250, Bandage = 50, 9mm = 5/round).
   - `SellPriceCredits` (typically 50-70% of buy).
   - `MinTierTag` (optional — reuse `Paldark.Item.Tier.{T1, T2, T3}` từ W37-38).
3. Default starting wallet credits 1000 (tunable trên `UPaldarkPlayerWallet::StartingCredits`).

### 0.5 WBP_PaldarkSaveSlotPicker (W48)

1. Subclass `UPaldarkSaveSlotPickerWidget` → `WBP_PaldarkSaveSlotPicker`.
2. Layout: ListView (bind tới slots — future iteration sẽ extend C++ wrapper; tạm bind tới hard-coded `Slot01..Slot10` cho alpha demo). Per row: slot name + Last save time + 3 button (Save / Load / Delete).
3. Button events: bind sang `RequestSave/Load/Clear` BP wrapper (W48).
4. Override `K2_OnSaveCompleted(Slot, ResultTag, TriggerTag)` cho refresh + toast (e.g. xanh nếu Success, đỏ nếu Fail).
5. Override `K2_OnLoadCompleted(Slot, ResultTag)` cho HUD refresh + close picker.

### 0.6 (Carry Q3) AnimBP + map blockout + Pal BP cluster

Vẫn pending từ Q3 (xem [`Q3-Playtest-Checklist.md`](Q3-Playtest-Checklist.md) §0). Q4 không yêu cầu must-have cho alpha demo nếu hub + raid_sandbox đủ — designer có thể opt to skip Rừng Hỏng + PalCorp cho alpha demo 1.0 và back-fill Q1-next.

---

## 1. Hub Town spawn (P11 + P15) [W40-41]

1. PIE 1-client (listen server) khởi tạo với `Map_HubTown` làm default map (DefaultEngine.ini wire xong từ §0.1).
2. Player spawn trong hub plaza, 3 building visible (Stable/Marketplace/Briefing).
3. Console `Paldark.Hub.DumpBuildings` → log line `Hub buildings: 3 entries=[{Building_Stable, Building_Marketplace, Building_Briefing}]`.
4. Console `Paldark.Hub.GoTo Paldark.Hub.Building.Stable` → player teleport gần Stable building.

> **Pass criteria:** 3 building được register qua `UPaldarkHubSubsystem`, không có "register on Initialize timing issue" warning.

---

## 2. Pal Stable deposit/withdraw (P03 + P12) [W44-45]

1. PIE 1-client với hub map. Player có 3 Pal trong active roster (from prior save hoặc `Paldark.Pal.SpawnTestCompanion 0..2`).
2. Walk to Stable building. Trigger interaction (E key hoặc `Paldark.Hub.Stable.OpenWidget`).
3. `WBP_PaldarkStableWidget` mở. Click "Deposit" trên Pal slot 0.
4. Expected log line: `[Stable] deposit player=Alice slot=0 species=Direhound → DepositRoster=1 ActiveRoster=2 result=Success`.
5. Pal disappear khỏi Active Roster, appear trong Deposit Roster.
6. Click "Withdraw" trên slot Deposit Roster 0.
7. Expected log line: `[Stable] withdraw player=Alice slot=0 species=Direhound → DepositRoster=0 ActiveRoster=3 result=Success`.

> **Pass criteria:** 32-slot deposit replicate đúng tới Owner (COND_OwnerOnly), other client không thấy slot data. Console `Paldark.Hub.Stable.List` dump cả 2 roster.

---

## 3. Marketplace buy/sell + Wallet (P03 + P12) [W44-45]

1. Walk to Marketplace building. Trigger interaction.
2. `WBP_PaldarkMarketplaceWidget` mở. Catalog hiển thị ~30 entry. Wallet display "Credits: 1000".
3. Click "Buy" trên Pistol (250 credits).
4. Expected log line: `[Marketplace] buy player=Alice item=Pistol → wallet=750 result=Success`.
5. Pistol appear trong inventory (`Paldark.Inventory.List` verify).
6. Click "Sell" trên Pistol.
7. Expected log line: `[Marketplace] sell player=Alice item=Pistol → wallet=1000+150=1150 result=Success` (assuming 60% sell rate).

> **Pass criteria:** Transaction atomic (wallet + inventory cùng update qua server RPC). Race condition: console 2 player cùng frame `Paldark.Hub.Marketplace.Buy` cùng item → first-touch-wins (second nhận `Paldark.Hub.Result.Fail.InsufficientStock` if catalog stock-limited, otherwise both succeed).

---

## 4. Backend Stub login + persist (P18) [W42-43]

1. Console `Paldark.Backend.Status` → expected `[Backend] mode=Stub bUseAWSBackend=false session=null`.
2. Console `Paldark.Backend.Login Alice` → expected `[Backend][Stub] login Alice → session=<UUID> jwt=stub.<sig> result=Paldark.Backend.Result.Success`.
3. Console `Paldark.Backend.PersistRoster` → expected `[Backend][Stub] persist roster player=Alice slots=3 result=Success`.
4. Console `Paldark.Backend.AllocateFleet 2` → expected `[Backend][Stub] allocate fleet players=2 → fleet=<UUID> server=stub-fleet-1.local:7777 result=Success`.

> **Pass criteria:** Stub backend respond < 100ms. Real AWS deploy là SRE task Q1-next; verify Stub mode đầy đủ trước khi flip `bUseAWSBackend=True`. Nếu `bUseAWSBackend=True` trong DefaultGame.ini → Cognito SDK call (cần real AWS resource).

---

## 5. Briefing room vote + ready handoff (P03 + P15) [W46]

1. PIE 4-client (1 listen-server + 3 client), tất cả spawn vào `Map_HubTown`.
2. 4 player walk to Briefing building. Mỗi player trigger interaction → `WBP_PaldarkBriefingWidget` mở.
3. Phase = `Paldark.Briefing.Phase.Lobby`. Click "Start Voting" trên server hoặc console `Paldark.Briefing.Start` → phase flip → `Voting`.
4. 4 player mỗi người click 1 map từ vote list (3 vote `Paldark.Map.Raid_Sandbox` + 1 vote `Paldark.Map.Map_RungHong`).
5. Expected: `[Briefing] vote player=Alice map=Paldark.Map.Raid_Sandbox → Tally{Raid_Sandbox=1, Map_RungHong=0}`.
6. 4 player click "Ready" toggle. Last player ready → phase flip → `Countdown` với 10s timer.
7. Expected: `[Briefing] all 4 players ready → CountdownSec=10 phase=Paldark.Briefing.Phase.Countdown`.
8. 10s pass → server initiate `World->ServerTravel(/Game/Maps/Raid_Sandbox)`.
9. Expected: `[Briefing] countdown=0 → server travel to Paldark.Map.Raid_Sandbox phase=Travel`.

> **Pass criteria:** 4-player vote tally replicate đúng qua `FFastArraySerializer`. Ready state replicate per-player. Countdown server-authoritative (client chỉ display CountdownSec). ServerTravel chỉ fire trên authority.

---

## 6. Raid (Q3 carry) [W24-25 + W18-19 + W11-12 + W35-36 + W37-38]

1. 4 player land trong `Raid_Sandbox`. Match phase = `Lobby` → `InProgress` sau countdown ngắn.
2. 4 player spread out, engage hostile Pal qua `Paldark.Pal.SpawnDirehoundPack` (or designer placed).
3. Combat: kill 1-2 hostile, tame 1 hostile qua Pal Sphere (Q3 W35-36).
4. Walk to extraction beacon (`Paldark.Match.Extract` console hoặc step beacon volume).

> **Pass criteria:** All Q3 flow (kill / tame / loot / inventory drop-on-death) playable. Match outcome = `Extracted` cho player vào beacon.

---

## 7. Auto-save on extract (P13) [W48]

1. Player A `Paldark.Match.Extract` → outcome `Extracted`.
2. Expected log: `[Save][W48] auto-save on extract slot=Paldark_<NetID> trigger=Paldark.Save.Trigger.HubReturn`.
3. Expected: `[Save] async write begin slot=Paldark_<NetID> trigger=Paldark.Save.Trigger.HubReturn schema=1 active=N deposit=M inventory=K`.
4. Expected: `[Save] async write complete slot=Paldark_<NetID> result=Paldark.Save.Result.Success trigger=Paldark.Save.Trigger.HubReturn`.

> **Pass criteria:** Save fire chỉ trên local PC (filter `IsLocalController()`). Dedicated server không fire (Q1-next per-player save). Toggle `bAutoSaveOnExtract=false` → no auto-save log (verify CVar / DefaultGame.ini override).

---

## 8. Hub return + load (P13 + P15) [W47 + W48]

1. After Player A's extraction, ServerTravel hoặc match end trigger return to `Map_HubTown` (via `BeginHubReturn` hoặc designer trigger).
2. Player A spawn vào hub. Console `Paldark.Save.Load Paldark_<NetID>` → expected log line.
3. `[Save] ApplySnapshot complete: Schema=1 Active=N Deposit=M Inv=K Map='Map_HubTown'`.
4. Player A roster + deposit + inventory match snapshot at extraction time.

> **Pass criteria:** Snapshot apply preserve composite Inventory tree (backpack tier + nested container). Roster snap-back preserve all 13 player slots + 11 Pal slots. Outcome reset.

---

## 9. Hub→raid handoff smoke (P13) [W48]

1. Console `Paldark.QA.HubToRaidHandoff QASmokeSlot` → expected `[Paldark.QA.HubToRaidHandoff] slot=QASmokeSlot trigger=Paldark.Save.Trigger.Travel queued=ok`.
2. Expected: `[Save][W48] hub→raid smoke save slot=QASmokeSlot`.
3. Expected: `[Save] async write complete slot=QASmokeSlot result=Paldark.Save.Result.Success trigger=Paldark.Save.Trigger.Travel`.

> **Pass criteria:** Smoke command stamp Travel tag without invoking ServerTravel. Slot xuất hiện qua `Paldark.Save.ListSlots`. Tag echo back trên `OnSaveCompleted` delegate.

---

## 10. UMG save slot picker (P13) [W48]

1. Open `WBP_PaldarkSaveSlotPicker` qua designer-wired key (e.g. F8) hoặc Pause menu.
2. ListView hiển thị 10 slot. Click "Save" trên Slot03.
3. Expected: `[Save] async write complete slot=Slot03 result=Success trigger=Paldark.Save.Trigger.Manual` (picker uses Manual trigger).
4. Toast UI display "Saved!" green.
5. Click "Load" trên Slot01 (assuming preexisting save).
6. Expected: `[Save] ApplySnapshot complete: ...` + HUD refresh.

> **Pass criteria:** UMG wrapper around C++ console commands works end-to-end. K2_OnSave/LoadCompleted fire correctly. Toast + HUD refresh logic in BP.

---

## 11. Auto-save on logout (P13) [W48]

1. Player C disconnect (close client window hoặc `disconnect` console).
2. Expected log on listen-server: `[Net] OnPlayerLogout broadcast for PaldarkPlayerController_<id>`.
3. Expected log on listen-server (if local Paldark PC): `[Save][W48] auto-save on logout slot=Paldark_<NetID> trigger=Paldark.Save.Trigger.Logout`.
4. Expected: `[Save] async write complete slot=Paldark_<NetID> result=Success trigger=Paldark.Save.Trigger.Logout`.

> **Pass criteria:** Save fire trước khi PC destroy. Remote client logout không fire auto-save trên listen-server (filter `IsLocalController()`). Dedicated server: per-player save defer Q1-next.

---

## 12. Bug bash (1 ngày — optional cho Q4)

1. Run all 11 steps trên với 4-client PIE. Report bug into [`Q4-Bug-Bash.md`](Q4-Bug-Bash.md).
2. Designer wrap up 5-6 ngày authoring task (§0).
3. Record 90-second video (4-quadrant split): hub spawn → stable/marketplace → briefing vote → raid extract → save → return hub → UMG slot picker.
4. Embed video into `Documents/Devlog/Q4-Playtest-Video.md` (placeholder file — author cùng tuần khi designer ship asset cluster).

---

## 13. Sign-off

| Role | Name | Date | Notes |
|------|------|------|-------|
| Lead designer | (author) | (W49-50) | hub + briefing + stable + marketplace authored |
| Lead engineer | (Devin) | W48 | C++ scaffold complete; CI 2/2 xanh từng PR |
| Lead PM | (author) | (W49-50) | alpha demo scope freeze + Q1-next roadmap kickoff |
| QA | (author) | (W49-50) | 12 step playtest pass; bug bash report attached |

---

## 14. References

- [`Documents/Devlog/Q4-Milestone-Build.md`](Q4-Milestone-Build.md) — Q4 PR ledger + pillar coverage + gap analysis.
- [`Documents/Devlog/Q4-Bug-Bash.md`](Q4-Bug-Bash.md) — known issue + 1-day priority fix plan.
- [`PaldarkLab/README.md`](../../PaldarkLab/README.md) §"W40-41 Hub Town", §"W42-43 AWS backend", §"W44-45 Stable & Marketplace UI", §"W46 Briefing room widget", §"W47 Save game system", §"W48 Polish" — feature-by-feature authoring + console test loop.
- [`Documents/Devlog/Q3-Playtest-Checklist.md`](Q3-Playtest-Checklist.md) — Q3 baseline (10-Pal + 3-map + tame + inventory full).
- [`Documents/Devlog/Q2-Playtest-Checklist.md`](Q2-Playtest-Checklist.md) — Q2 baseline (4-player co-op + match + extraction).
- [`Documents/Devlog/Q1-Playtest-Checklist.md`](Q1-Playtest-Checklist.md) — Q1 baseline (single-player + GAS + inventory + activity FSM).
