# PALDARK — Q3 Bug Bash (Tuần 39, 1 ngày)

> **Source:** Roadmap § Tuần 39 "Internal playtest: 3 map, 10 Pal, full inventory, tame mechanic." Audit toàn bộ C++ + ini + validator + README ở freeze point W38 → ưu tiên hóa cho 1 ngày bug bash trước khi vào Q4.
>
> **Method:** Grep `TODO|FIXME|HACK` trong `PaldarkLab/Source/` (kết quả: 0 — codebase sạch ở mức comment). Audit thủ công 7 PR description W27-38 + README để rút known-issue Q3 + carry-over Q2 chưa đóng. Cross-reference với pillar gap trong [`Q3-Milestone-Build.md`](Q3-Milestone-Build.md) § 5.

---

## 0. Priority legend

| P0 | Block Q3 outcome — phải fix trước khi tick milestone. |
|----|--------------------------------------------------------|
| P1 | Block hoặc xấu UX nhưng có workaround — fix trong bug-bash day. |
| P2 | Cosmetic / improvement — defer Q4. |
| P3 | Future-proofing — defer Q4+. |

---

## 1. Issue table (20 issue, ưu tiên hóa)

### Carry-over từ Q2 (chưa đóng)

| ID | Priority | Issue | Component | Roadmap defer | Fix sketch |
|----|----------|-------|-----------|---------------|------------|
| L-02 | **P1** | UMG end-of-match widget chưa tồn tại — chỉ có C++ delegate `OnMatchOutcomeChanged` + `OnMatchPhaseReplicated`. Demo video sẽ thiếu end screen | UI | W30+ polish (carry from Q2) | `WBP_EndOfMatchScreen` listen 2 delegate + show win/loss + per-player outcome table. Estimate: 3 ngày (Aura WidgetController pattern). |
| L-04 | **P2** | Reconnect mid-match không support — `Disconnected` là terminal. Player Alt+F4 → re-join = new PlayerState, không restore outcome | Network/Match | Q4 (W40+ hub town hoặc trước khi matchmaking) | Map `LoginID` → restore `Outcome` từ subsystem registry. Estimate: 1 ngày. |
| L-05 | **P2** | Spectator camera chưa wire — KIA player stuck ragdoll, không observe teammate | UI/Camera | W30+ polish (carry from Q2) | `USpectatorCameraComponent` + free-cam input + KIA → switch camera transition. Estimate: 2 ngày. |
| L-06 | (code-closed W27, designer authoring pending) | ~~AnimBP T-pose risk — designer chưa author `ABP_PaldarkPlayer` / `ABP_PaldarkPal`~~ | ~~Animation~~ | ~~Q3 đầu~~ | ✅ Code-side closed by W27 Path A PR. **Designer authoring vẫn pending Q3 carry — see L-23 new.** |
| L-12 | **P1** | Squad command radial wheel UMG chưa tồn tại — chỉ test qua console; designer demo "feel" rất ốm | UI | W30+ polish (carry from Q2) | `WBP_SquadRadial` UMG widget bind tới `USquadCommandSlotComponent`. Estimate: 2 ngày. |
| L-16 | **P1** | `ExtractionProgress` REPNOTIFY throttle 0.25s → client thấy progress bar giật stair-step (nếu UMG widget exist) | UI/Net | UI widget task (carry from Q2) | HUD task interpolate giữa RepNotify với `FMath::Lerp`. Estimate: 2 giờ trong UI PR. |
| L-17 | (closed by #28 W27-28) | ~~Hard timeout `MaxMatchDuration` default 600s — không có designer-tunable per-experience~~ | ~~Match~~ | ~~Q3~~ | Defer — không close trong Q3 vì W27-28 chỉ focus async load. Carry to Q4. |
| L-19 | (closed by #29 W29-30) | ~~Pal Combat ability SetByCaller `Paldark.SetByCaller.PalDamage` chưa expose damage tuning qua DataAsset — hardcoded designer-author cost~~ | ~~GAS~~ | ~~Q3 polish~~ | ✅ Đóng cùng W29-30 PR (`UPaldarkPalCombatProfileDataAsset` per-Pal-species + 5 ability subclass có damage SetByCaller default). |
| L-20 | (closed by #36 W39 follow-up) | ~~`UPaldarkNetSubsystem` `OnPostLogin` event không expose tới Blueprint — UMG lobby UI sẽ cần~~ | ~~Network~~ | ~~W30+~~ | ✅ Code-side đóng trong W39 follow-up PR: 2 `DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam` (`FPaldarkPlayerLoginSignature(APlayerController*)` + `FPaldarkPlayerLogoutSignature(AController*)`) + 2 `BlueprintAssignable` UPROPERTY (`OnPlayerLogin` / `OnPlayerLogout`) + 2 `BroadcastPlayerLogin/Logout` UFUNCTION + `APaldarkGameModeBase::PostLogin/Logout` resolves subsystem qua `GetSubsystem<UPaldarkNetSubsystem>()` và call `BroadcastPlayerLogin(NewPlayer)/BroadcastPlayerLogout(Exiting)` (logout broadcast trước `Super::Logout` để giữ valid `PlayerState`). Validator extend `check_net_subsystem_shape` enforce header + cpp + GameMode wiring. README W14-15 § "W39 — L-20 closure (BlueprintAssignable login/logout delegates)". |

### Q3 mới phát sinh (W27-38)

| ID | Priority | Issue | Component | Roadmap defer | Fix sketch |
|----|----------|-------|-----------|---------------|------------|
| L-21 | (closed by #35 W39 follow-up) | ~~Game Feature plugin `PaldarkRaidContent` không tự auto-activate trên experience swap — designer phải bật trong `Edit → Plugins` OR `UPaldarkExperienceDefinition.RequiredGameFeatures[]` (chưa wire)~~ | ~~GameFeature~~ | ~~W40+~~ | ✅ Code-side đóng trong W39 follow-up PR: `UPaldarkExperienceDefinition::RequiredGameFeatures[]` UPROPERTY (`TArray<FString>`, EditDefaultsOnly) + `APaldarkGameModeBase::OnExperienceAssetReady` 3-layer check (IPluginManager::FindPlugin → GetPluginURLByName → GetPluginState == Active) logging `LogPaldark Error` + `AddOnScreenDebugMessage(FColor::Red, 15s)` per miss + `check_required_game_features_wired` validator. Designer task `PX_RaidSandbox.uasset` field wiring is per-asset (uasset binary, defer to designer branch). README W39 § "L-21 closure (RequiredGameFeatures assertion)". |
| L-22 | **P1** | `MaxWeightKg → BaseMaxWeightKg` rename callsites — designer Blueprint Q1 W11-12 có thể đọc field name cũ. Silent break nếu BP cũ exists | Inventory | Q4 polish (validator extend với BP grep) | Validator updated W37-38 to grep `MaxWeightKg` in `.cpp` — designer BP grep wait Q4 (need uasset diff parse). Workaround: README W37-38 §"Anti-patterns avoided" item liệt kê + `Paldark.Inventory.Dump` console for verify. Estimate: 1 ngày (validator BP grep). |
| L-23 | **P0** | AnimBP authoring vẫn pending — `UPaldarkAnimInstance` scaffold shipped W27 nhưng designer chưa author State Machine / Blend Space / layer fn (carry Q2 L-06 escalated to P0 cho Q3 demo video) | Animation | Q3 đầu hoặc parallel branch | Designer task: 2-day per character. `ABP_PaldarkPlayer` + `ABP_PaldarkPal` + per-species hostile-Pal AnimBP. README W27 § 8-step authoring order. Estimate: 5-7 ngày (10 character total). |
| L-24 | **P0** | Map 2 + Map 3 `.umap` chưa commit — designer task 4-5 ngày. Q3 outcome "3 map playable" depends on this | Map | W39 (now) hoặc Q4 đầu | Designer task: 2 ngày mỗi map blockout + per-map `DA_MapDef_*` + POI/SpawnPoint placement. README W31-32 + W33-34 chi tiết. Estimate: 4-5 ngày. |
| L-25 | **P0** | Per-species `DA_PalDef_*` chưa author cho 5 species mới Q3 (Foxparks/Tombat/Stoneclad/Vinewraith/Boltmane) | Data | W39 (now) hoặc Q4 đầu | Designer task: ~30 phút mỗi DA + AnimBP wire. Q3 demo video sẽ T-pose nếu skip. Estimate: 1 ngày (10 total: 5 mới Q3 + 5 carry Q2). |
| L-26 | **P1** | 3 Pal Sphere item DA + 3 backpack DA + 17 mới item DA chưa author = 23 DA total | Data | W39 (now) hoặc Q4 đầu | Designer task: ~15 phút mỗi DA (placeholder mesh + icon + tag wire). Estimate: 1 ngày. Critical: GiveAll console returns 0 grants nếu skip. |
| L-27 | **P1** | `GE_StunOnHit` + `GE_StunDecay` chưa author — stun bar tăng/giảm depend on designer GE | GAS | W39 (now) hoặc Q4 đầu | Designer task: 30 phút mỗi GE (curve table). README W35-36 §"Authoring order" step 4. Tame minigame won't work end-to-end without these. Estimate: 1-2 giờ. |
| L-28 | **P2** | Boltmane phase transition thresholds (0→1 ở 60% HP, 1→2 ở 30% HP) hardcoded — không tunable per-DA | AI | **Closed W39 follow-up (PR #38) — code-side** | Added `TArray<float> PhaseHealthThresholds` (`EditDefaultsOnly`, ClampMin=0.05, ClampMax=0.95, descending) UPROPERTY trên `UPaldarkPalDefinition`. `UPaldarkActivity_BossPhase::HydrateThresholdsFromPalDefinition` (called from `EnterActivity_Implementation`) reads `PhaseHealthThresholds[0..1]` qua `UAssetManager::GetPrimaryAssetObject(FPrimaryAssetId("PaldarkPalDefinition", <species leaf>))` và override `EnragedThresholdPct` / `TelegraphThresholdPct`. Lookup pattern mirror W35-36 `UPaldarkPalTameComponent::HydrateFromPalDefinition`. Validator `check_pal_definition_shape` thêm step (7) cho field presence + `EditDefaultsOnly` enforcement; `check_w29_pal_roster_shape` (step 8) thêm 6 assertion: header declare `HydrateThresholdsFromPalDefinition`, `.cpp` include PalDefinition + AssetManager header, helper body present, called từ EnterActivity, reference `PhaseHealthThresholds` field, log lines tagged `[L-28]`. Fallback: malformed input (empty array, <2 entries, non-descending, value outside [0,1]) → `LogPaldarkPal Warning` + boss uses W29-30 CDO defaults (0.5/0.25) — encounter remains playable. README W39 § L-28 closure documents two-layer fix + designer test loop + 4 regression cases. Estimate: 2 giờ (delivered as 1 PR). |
| L-29 | **P3** | Loot bag `InitializeContents` không validate item def soft ptr null path — silent skip | Loot | Q4 polish | Validator hoặc inline log warning. Estimate: 1 giờ. |
| L-30 | **P3** | Drop-on-death flatten `InnerEntries` → bag mất container nesting | Inventory | W47 save-game preserve original tree | Documented anti-pattern; W47 save-game preserve original tree via FArchive. Estimate: defer. |
| L-31 | **P3** | `EquippedBackpack` replicate to all clients (not COND_OwnerOnly) — privacy / bandwidth concern | Inventory/Net | Q4 polish (UI team decision) | Backpacks là persistent player gear, public visibility acceptable. Flip COND_OwnerOnly nếu UI team xác nhận chỉ owner cần thấy. Estimate: 30 phút. |
| L-32 | **P2** | `FPaldarkInventoryEntry::InnerEntries` self-referential — UHT support but FastArraySerializer migration deferred W47 | Inventory/Net | **Scaffold landed W39 follow-up (this PR) — code-side; W47 still owns the storage swap** | Added `FPaldarkInventoryEntry::MigrationReplicationKey` int32 UPROPERTY (`NotReplicated, Transient`) — per-row monotonic id stamped at append time; name divergence intentional so W47's `FFastArraySerializerItem` inheritance brings its own `ReplicationKey` without merge conflict. `UPaldarkPlayerInventoryComponent::BuildNextReplicationKey()` static uses `FPlatformAtomics::InterlockedIncrement` on a process-wide `PaldarkInventoryMigration::GReplicationKeyCounter`. `MarkInventoryDirty(int32 EntryIndex = INDEX_NONE)` is the single broadcast funnel — every previous `OnInventoryChanged.Broadcast(this)` callsite (`AddItem`, `RemoveItemByTag`, `DropAllItems`, `SetEquippedBackpack`, `OnRep_Entries`, `OnRep_EquippedBackpack`) now routes through it; validator caps residual broadcasts at 1 (the one inside `MarkInventoryDirty` body). `AddItem` Phase 2 stamps `NewRow.MigrationReplicationKey = BuildNextReplicationKey()` on freshly-appended rows only (top-off-existing-stack keeps the row's previous id, mirroring FastArraySerializer's `MarkItemDirty` semantics). Validator `check_w37_inventory_full_shape` extended with sections (8) header + (9) .cpp: 3 header symbols (field UPROPERTY + counter decl + funnel decl), `NotReplicated`+`Transient` flag enforcement on the field's UPROPERTY block (anchored regex skips the doc-comment mention), 6 .cpp symbols (namespace + counter, atomic increment, AddItem stamping, funnel def, `[L-32][Inventory.MarkDirty]` log tag), exactly-1 residual `OnInventoryChanged.Broadcast(this)` count (comment-stripped before counting so the W47 migration plan example block inside `MarkInventoryDirty` doesn't trip the count). README W39 § L-32 closure documents why-scaffold rationale, NotReplicated/Transient rationale, **W47 four-step migration plan** (1: inherit `FFastArraySerializerItem`, 2: wrap `Entries` in `FFastArraySerializer`-backed struct + `TStructOpsTypeTraits<…>::WithNetDeltaSerializer`, 3: rewrite `MarkInventoryDirty` body to call `MarkArrayDirty`/`MarkItemDirty`, 4: delete scaffold counter + `OnRep_Entries`), 7 anti-patterns avoided (name collision, `Replicated` flag, stamp-on-remove, per-tick mark, inlining broadcast, persisting counter, stamping on bag entries), 4 out-of-scope items (actual storage swap, `InnerEntries` per-row delta, per-row `MarkItemDirty` in `RemoveItemByTag`, removing `OnRep_Entries`). **W39 wire format unchanged** — W37-38 plain `DOREPLIFETIME(TArray<FPaldarkInventoryEntry>)` blob still ships per-mutation; only the callsite plumbing was migrated. Estimate: ~4 giờ (delivered as 1 PR). |
| L-33 | **P3** | `SetEquippedBackpack` allows swap mid-fight even if new total > new cap — chỉ log warning, không enforce | Inventory | W38+ polish | W38 polish: encumbrance penalty curve (slow movement when over-encumbered). Estimate: 4 giờ. |
| L-34 | **P2** | Game Feature plugin chưa auto-activate trên dedicated server startup — dedicated.exe có thể boot without plugin loaded | GameFeature/Net | **Closed W39 follow-up (PR #37) — code-side** | `.uplugin` set `EnabledByDefault=true` + `BuiltInInitialFeatureState=Active`; `.uproject` flip `PaldarkRaidContent.Enabled=true`; `UPaldarkGameInstance::Init` server-only hook iterates `IPluginManager::Get().GetDiscoveredPlugins()` filter `bIsGameFeaturePlugin`, calls `UGameFeaturesSubsystem::LoadAndActivateGameFeaturePlugin` for any below `Active`. Validator `check_game_feature_plugin` extended with 5 L-34 assertions. README W39 § L-34 closure documents the boot path + recovery + test loop. Estimate: 2 giờ (delivered as 1 PR). |
| L-35 | **P3** | POI subsystem `TActorIterator` sweep on `Initialize` — nếu POI BeginPlay sau subsystem Init, miss; retry on `OnWorldBeginPlay` đã có nhưng vẫn race window | Map | Q4 polish | Documented; retry usually fires < 1 frame. Add `RegisterDeferred` queue for POI BeginPlay before subsystem init. Estimate: 2 giờ. |
| L-36 | **P3** | Loot bag actor `bAutoCleanupTimer = 300s` hardcoded — designer không tune được | Loot | Q4 polish | Expose `AutoCleanupSeconds` UPROPERTY. Estimate: 30 phút. |
| L-37 | **P2** | Tame attempt formula `BaseCaptureProbability` không có client preview — UI cần show "32% chance to capture" trước throw | UI/Tame | W50 HUD polish | HUD task: predicted probability từ AttributeSet read + Pal definition. Estimate: 4 giờ trong UI PR. |
| L-38 | **P3** | Pal Sphere actor đè onto Player hand vs camera-forward — quẳng từ vai chứ không phải mũi camera | Tame/Anim | Q4 polish (cùng anim) | Per-character socket override trên `BP_PaldarkCharacter` (designer wire bone socket "PalSphereSocket"). Estimate: 30 phút. |
| L-39 | **P3** | Multi-player inventory contention — 2 player cùng grab dropped loot bag cùng frame | Inventory/Net | Q4 polish | First-touch-wins acceptable; documented anti-pattern. Race rare in practice. Estimate: defer. |
| L-40 | **P3** | `PaldarkAssetManager.GetPrimaryAssetIdList` query mỗi `GiveAll` call — cost N items × M ticks nếu spam | Performance | (No fix needed — console cmd is rare) | Cache result trong `PaldarkLabWorldSubsystem` if used > 1/frame. Estimate: 1 giờ. |

---

## 2. P0 / P1 fix plan (1 ngày bug bash)

### Slot 1 — Sáng (P0 L-25 + L-27: Designer DA + GE author)

> Critical path. Without `DA_PalDef_*` for 5 mới Q3 + 2 stun GE, tame minigame + boss AI won't work end-to-end. Demo video không có 10 Pal.

- [ ] Designer: author 5 mới `DA_PalDef_*` (Foxparks/Tombat/Stoneclad/Vinewraith/Boltmane) → tuning values per README W29-30 § Authoring order.
- [ ] Designer: author 5 carry Q2 `DA_PalDef_*` nếu chưa (Direhound/Razorbird + 3 generic dummy).
- [ ] Designer: author `GE_StunOnHit` (instant, magnitude = `Paldark.SetByCaller.PalDamage * 0.5`) + `GE_StunDecay` (periodic, -5/sec).
- [ ] Devin (parallel): write tutorial pass in README + log warning if any `DA_PalDef_*` missing on `Paldark.Pal.WarmAll`.
- [ ] Smoke test: `Paldark.Pal.WarmAll` → log `N=10 succeeded`. `Paldark.Pal.SpawnTestHostile Boltmane 1500` → spawns with phase 0 + ability ready.

**Estimate: 4-5 giờ (designer-side, mostly DA tuning + 2 GE author).**

### Slot 2 — Trưa (P0 L-21: Game Feature plugin auto-activate)

Code-side fix, no designer.

- [ ] Add `UPaldarkExperienceDefinition::RequiredGameFeatures[]` UPROPERTY (`TArray<FString>` plugin names).
- [ ] `APaldarkGameModeBase::OnExperienceAssetReady` iterates RequiredGameFeatures; if any `UGameFeaturesSubsystem::GetPluginState(name) != Activated`, log `LogError` + `AddOnScreenDebugMessage` red banner.
- [ ] Wire `PX_RaidSandbox.RequiredGameFeatures = ["PaldarkRaidContent"]` (default value in CDO).
- [ ] Extend `validate_paldarklab.py` `check_required_game_features_wired()` → grep header field + DefaultEngine.ini PrimaryAssetType for plugin asset registration.
- [ ] Smoke test: PIE without plugin enabled → red banner appears. PIE with plugin → no banner.

**Estimate: 2 giờ (Devin code + validator).**

### Slot 3 — Chiều (P0 L-23 + L-24: Designer AnimBP + 2 maps)

Designer-side, parallel with Slot 1 + Slot 2 if 2 designers.

- [ ] Designer 1: `ABP_PaldarkPlayer` + `ABP_PaldarkPal` (Q2 L-06 carry, escalated P0 cho Q3 video). State Machine + Blend Space + 3 layer fn. Estimate: 2 ngày.
- [ ] Designer 2: `Map_RungHong.umap` blockout + 4-6 POI/SpawnPoint placement + `DA_MapDef_RungHong`. Estimate: 2 ngày.
- [ ] Designer 2 (continuation): `Map_PalCorp.umap` blockout + tech-themed POI + `DA_MapDef_PalCorp` with `bIsIndoor=true`. Estimate: 2 ngày.

**Estimate: 4-6 ngày designer time** (parallel work distributes 5-7 ngày to ~3 ngày elapsed if 2 designers).

### Slot 4 — Cuối ngày (P1 L-22 + L-26: Validator + DA cleanup)

If time remains:

- [ ] Devin: validator BP grep for `MaxWeightKg` references — write asset reader (uasset binary diff parse via UE pak tool, complex; or fallback to text grep on `.uasset` plain XML if `bGenerateBPLibrariesForBlueprintCompilation=true`). **Defer Q4** — too complex for bug-bash slot.
- [ ] Designer: author 23 mới item DA (3 PalSphere + 3 backpack + 17 catalog). 15 phút mỗi DA = ~5-6 giờ total. Can parallel with Slot 1+3.
- [ ] **DO NOT** fully wire mesh / icon / VFX — chỉ stub đủ tag + weight + stack count cho `GiveAll` console.

**Estimate: 1 ngày designer time (parallel with main work).**

---

## 3. Open questions trước khi đóng Q3

Decision cần user (designer + tech lead) trả lời trong bug bash day:

- [ ] **Q-Q3-01:** Game Feature plugin auto-activation (L-21) — implement now (Slot 2) or defer Q4 hub town W40+? Em đề xuất **now** vì designer block khi quên enable plugin → silent fail trong PIE.
- [ ] **Q-Q3-02:** AnimBP authoring path — 5 species shared Manny AnimBP (Path C, "placeholder forever, accept T-pose for non-companion") OR full per-species AnimBP (Path A, $300-500 marketplace animset cost)? Em đề xuất **Path A cho 2 companion (Foxparks/Tombat) + Path C cho 3 hostile boss (Stoneclad/Vinewraith/Boltmane)** vì companion mới là phần player tương tác lâu nhất, hostile thường chết nhanh.
- [ ] **Q-Q3-03:** Map polish — Map 2 + Map 3 blockout-only hay marketplace landscape mua? Em đề xuất **blockout-only cho Q3 milestone + marketplace landscape Q4 hub town**. Blockout đủ chứng tỏ outcome "3 map playable" và pillar P11 90% target.
- [ ] **Q-Q3-04:** Inventory full 17 mới item DA — designer batch author hôm nay placeholder hay split across W40-50 (4-5 DA / week)? Em đề xuất **batch hôm nay placeholder** vì `GiveAll` console + tame loop depends. Mesh/icon polish có thể defer.
- [ ] **Q-Q3-05:** Tame minigame demo flow — designer record video với P=0.45 success case (visual feedback rõ) hay sub-1% boss capture case (more dramatic)? Em đề xuất **cả hai** (1 normal + 1 boss force capture qua `Paldark.Tame.Force`).

---

## 4. Closure criteria

Q3 milestone đóng khi:

- [ ] ✅ Tất cả P0 fix (L-21 GameFeature, L-23 AnimBP, L-24 maps, L-25 DA_PalDef, L-27 stun GE) hoặc accepted defer cho Q4 đầu với note risk.
- [ ] ✅ Ít nhất 2/3 P1 fix (L-02/L-12 UMG stub là nice-to-have; L-22 validator BP grep defer Q4; L-26 item DA priority).
- [ ] ✅ [`Documents/Devlog/Q3-Playtest-Checklist.md`](Q3-Playtest-Checklist.md) tick xong 9/9 section (skip Dedicated optional).
- [ ] ✅ Video 90s shipped vào `Q3-Playtest-Video.md`.
- [ ] ✅ Q-Q3-01..Q-Q3-05 decision logged trong devlog hoặc `02-Pillar_Coverage.md` update.

Nếu P0 L-21 không close (Game Feature plugin auto-activate code-side fix): defer Q3 milestone 4-6 giờ; KHÔNG defer cả tuần. Đây là 1 PR Devin work.

Nếu P0 L-23 / L-24 / L-25 chưa close (designer authoring): video record với caption "Visual placeholder, AnimBP polish + map blockout Q4 đầu". Q3 milestone vẫn đóng với note risk, tương tự Q2 fallback pattern.

---

## 5. Cumulative Q1+Q2+Q3 issue stats

| Category | Q1 close (15 issue) | Q2 close (20 new) | Q3 close (20 new) | Δ closed Q2→Q3 |
|----------|---------------------|-------------------|-------------------|-----------------|
| P0 still open | K-01 (1) | L-01 + L-06 + K-01 (3) | L-21 + L-23 + L-24 + L-25 + L-27 (5) | -3 carry, +5 mới |
| P1 still open | K-02 + K-07 + K-13 (3) | K-02 + L-02 + L-12 + L-13 + L-16 (5) | L-02 + L-12 + L-16 + L-22 + L-26 (5) | -1 K-13 + L-13 closed (W26 follow-up), +2 mới |
| P2 still open | K-04 + K-05 + K-06 + K-14 (4) | L-04 + L-05 + L-09 + L-17 + L-20 (5) | L-04 + L-05 + L-20 + L-28 + L-32 + L-34 + L-37 (7) | -2 carry, +4 mới |
| P3 still open | K-08-K-12 + K-15 (6) | L-03 + L-07 + L-08 + L-10 + L-11 + L-14 + L-15 + L-18 + L-19 (9) | L-29 + L-30 + L-31 + L-33 + L-35 + L-36 + L-38 + L-39 + L-40 (9) | -8 carry, +9 mới |

**Closed Q2→Q3 (4):**
- L-01 (W26 follow-up PR #25 — validator extend Extraction GameMode).
- L-13 (W26 follow-up PR #26 — validator extend hostile-Pal CandidateActivities REPLACE).
- L-18 (W26 follow-up PR #26 — validator extend `Raid_Sandbox.umap` warn).
- L-19 (W29-30 PR #29 — PalCombatProfile per-species damage).

**Closed inline Q2→Q3 (3 carry):**
- L-06 / K-01 (W27 Path A PR #27 — AnimInstance scaffold; designer authoring remains as L-23).

**Re-classification Q2→Q3:**
- L-06 escalated từ Q2 P0 (defer W27 đầu) → Q3 P0 (must close cho Q3 video) as L-23.
- L-17 demoted P2 → defer Q4 (chưa fit Q3 axis).

> **Verdict:** Q1+Q2+Q3 cộng dồn ~26 issue Q3-relevant + 6 carry P3. Healthy ratio — codebase technical debt remains low. P0 cluster (5 issue) all designer-side authoring (4) + 1 code-side fix (L-21 Game Feature) = 1 Devin PR + ~5-7 ngày designer time. Q4 mở đầu với W40-41 hub town (8-player shard + Pal Stable + Marketplace UI) + W42-43 backend AWS (Cognito + DynamoDB + Lambda).

---

## 6. References

- [`Documents/Devlog/Q3-Milestone-Build.md`](Q3-Milestone-Build.md) § 5 (Known issues table).
- [`Documents/Devlog/Q3-Playtest-Checklist.md`](Q3-Playtest-Checklist.md) § 11.
- [`Documents/Devlog/Q2-Bug-Bash.md`](Q2-Bug-Bash.md) — Q2 issue table (L-01 → L-20).
- [`Documents/Devlog/Q1-Bug-Bash.md`](Q1-Bug-Bash.md) — Q1 issue table (K-01 → K-15).
- [`Documents/PALDARK/03-Roadmap_1_Year.md`](../PALDARK/03-Roadmap_1_Year.md) § Tuần 39 + § Q3 outcome + § Tuần 40-41.
- [`PaldarkLab/README.md`](../../PaldarkLab/README.md) — anti-pattern sections per W27-38 feature.
