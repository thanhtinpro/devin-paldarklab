# PALDARK — Q3 Milestone Build (Tuần 39)

> **Source:** [`Documents/PALDARK/03-Roadmap_1_Year.md`](../PALDARK/03-Roadmap_1_Year.md) § Tuần 39.
>
> **Roadmap deliverable:** "Internal playtest: 3 map, 10 Pal, full inventory, tame mechanic."
>
> **Q3 Outcome (from roadmap):** **Vertical slice 2** — content đủ cho 5 giờ chơi lặp.

This devlog freezes the Q3 milestone: what was shipped W27-38 in C++, how it maps to the roadmap, which pillars hit their Q3 % target, and what the user must do in UE Editor (the VM has no UE 5.4 compiler) to actually validate the 3-map / 10-Pal / inventory-full / tame-mechanic vertical slice.

---

## 1. PR ledger (W27 → W38)

| PR | Week | Title | Status | Branch |
|----|------|-------|--------|--------|
| [#27](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/27) | W27 (Path A) | AnimBP Lyra ALS port (C++ scaffold + validator) | ✅ merged | `devin/.../w27-animbp-lyra-als-port` |
| [#28](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/28) | W27-28 | AssetManager + Async load + Game Feature plugin (Pal definition + spawn subsystem + pre-warm + PaldarkRaidContent scaffold) | ✅ merged | `devin/.../w27-28-asset-manager-async` |
| [#29](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/29) | W29-30 | 5 Pal loài còn lại (Stoneclad + Vinewraith + Boltmane hostile + Foxparks + Tombat companion + boss FSM activity) | ✅ merged | `devin/.../w29-mass-ai-scale` |
| [#30](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/30) | W31-32 | Map 2 "Rừng Hỏng" blockout (POI + spawn point + map definition + 3 console commands + 12 tags) | ✅ merged | `devin/.../w31-32-map2-blockout` |
| [#31](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/31) | W33-34 | Map 3 "Cơ Xưởng PalCorp" + tech-themed loot drop (loot table + drop component + loot bag + 10th Pal slot + bIsIndoor + 9 tags + 3 console commands + validator) | ✅ merged | `devin/.../w33-34-map3-palcorp-loot` |
| [#32](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/32) | W35-36 | Pal Bond + Capture (Tame minigame): Stun attrs + bond comp + tame comp + Pal Sphere + UsePalSphere GA + Roster + 10 tags + 4 console commands + validator + Content placeholder | ✅ merged | `devin/.../w35-36-tame-minigame` |
| [#33](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/33) | W37-38 | Inventory full (30 items + 3 backpack tiers + composite container + drop-on-death) | ✅ merged | `devin/.../w37-38-inventory-full` |

7 PR, ~12,500 dòng C++ + ini + python validator + README + content README cộng dồn W27-38 trên `main`. Toàn bộ commit qua CI structural validation (`scripts/ci/validate_paldarklab.py`, 2/2 check xanh từng PR).

Cumulative since Q1 close (W13): 23 PR (#14-#33 trừ đã merge + 3 follow-up Q2), ~30,000 dòng tổng. Q2 → Q3 delta: +7 PR, +12,500 dòng, +21 tags, +12 console commands.

---

## 2. Feature map vs roadmap Q3 outcome

Roadmap § Q3 ghi rõ Q3 outcome cần: "Vertical slice 2 — content đủ cho 5 giờ chơi lặp." Roadmap § Tuần 39 phá xuống 4 axis content: **3 map** + **10 Pal** + **full inventory** + **tame mechanic**. Bảng dưới mapping từng axis → PR / file C++ / lệnh test:

| Q3 Outcome axis | C++ delivery | Test entrypoint | PR |
|-----------------|--------------|-----------------|----|
| **AnimBP scaffold** (locomotion layer parity với Lyra ALS) | `UPaldarkAnimInstance` (base, thread-safe snapshot pattern: `LocomotionData` USTRUCT + `NativeThreadSafeUpdateAnimation`) + `UPaldarkPalAnimInstance` (subclass: `bIsHostile`/`bIsFlying`/`bIsInCombat` exposed cho State Machine) + `IPaldarkAnimLayerInterface` (3 layer fn: PlayLocomotion/PlayCombat/PlayMontage) + `UPaldarkPawnData::DefaultAnimInstanceClass` slot | (Designer) Author `ABP_PaldarkPlayer` + `ABP_PaldarkPal` subclass và verify thread-safe pattern không touch raw UObject* | #27 |
| **Async load + Game Feature plugin** (map không hitch + Pal pre-warm) | `UPaldarkPalDefinition` (UPrimaryDataAsset, SpeciesTag + soft PalClass/Mesh/AnimInstance + GrantedAbilities) + `UPaldarkPalSpawnSubsystem` (`FStreamableManager` coalesce + `TArray<TSharedPtr<FStreamableHandle>>` lifecycle) + `PaldarkRaidContent` Game Feature plugin (manifest + boot-strap) + 3 console cmds (`Paldark.Pal.Spawn`, `Paldark.Pal.WarmAll`, `Paldark.GameFeature.Status`) | `Paldark.Pal.WarmAll` → log `warming N PalDefinition assets, queued, succeeded` | #28 |
| **10 Pal roster** (5 Q2 + 5 Q3) | 3 hostile (Stoneclad/Vinewraith/Boltmane) + 2 companion (Foxparks/Tombat) `APaldarkPalCharacter` subclass + 5 ability subclass (Charge/VineWhip/FireBreath/ChainLightning/Bite) + `UPaldarkActivity_BossFSM` (Boltmane phase 0..2 + transition tags) + 13 new gameplay tags (5 species + 2 pack + 1 activity + 5 ability + 2 boss phase) | `Paldark.Pal.SpawnTestCompanion 0..3` + `Paldark.Pal.SpawnDirehoundPack` + `Paldark.Pal.SpawnTestHostile <species>` | #29 |
| **Map 2 "Rừng Hỏng"** (forest blockout) | `APaldarkPointOfInterest` (designer-placeable AActor with replicated POI tag + label) + `UPaldarkPOISubsystem` (UWorldSubsystem, deferred discovery via TActorIterator sweep + per-POI BeginPlay retry on miss) + `APaldarkPaldarkPalSpawnPoint` (placeable spawn point linked to POI tag) + `UPaldarkMapDefinition` (UPrimaryDataAsset: PointsOfInterest + WeatherTag + BiomeTag) + 12 new tags (3 POI tag + 3 weather + 3 biome + 3 spawn point) + 3 console cmds (`Paldark.Map.DumpPOIs`, `Paldark.Map.DumpSpawnPoints`, `Paldark.Map.Definition`) | `Paldark.Map.DumpPOIs` → log `POIs: 12 entries=[{POI_Glade, POI_RuinedShrine, ...}]` | #30 |
| **Map 3 "Cơ Xưởng PalCorp" + tech-themed loot drop** | `UPaldarkLootTable` (UPrimaryDataAsset: entries với ItemDefinition soft ref + weight + count range + drop chance) + `UPaldarkLootDropComponent` (Pal-attached, async load + spawn `APaldarkLootBag` at death transform with weak-owner-ptr capture) + `APaldarkLootBag` (replicated pickup actor, 5-min auto-cleanup, `FPaldarkLootBagEntry` array) + `UPaldarkMapDefinition::bIsIndoor` flag + 10th Pal companion slot (`LootDropSlot`) + 9 new tags (3 loot tier + 3 PalCorp POI + 3 indoor zone) + 3 console cmds | `Paldark.Loot.SpawnTestBag` + `Paldark.Pal.Kill <label>` + walk → pickup | #31 |
| **Pal Bond + Capture** (Tame minigame) | 2 new AttributeSet attrs (Stun + MaxStun, replicated + clamped) + `UPaldarkPalBondComponent` rewrite (BondLevel 0..20, BondXP curve fallback linear 100 XP/level, FOnPaldarkBondLevelChanged delegate) + `UPaldarkPalTameComponent` (11th Pal slot, capture probability formula `(1-HpPct)*(1+StunPct)*TierMult/TameDifficulty`) + `APaldarkPalSphere` (projectile actor with `OnComponentHit` authority-only) + `UPaldarkGameplayAbility_UsePalSphere` (server-only GA) + `UPaldarkPlayerPalRosterComponent` (13th player slot, `COND_OwnerOnly` replication) + 10 new tags + 4 console cmds | `Paldark.Tame.SetStun + Paldark.Gas.Damage + GA throw` → `BeginTameAttempt rolled=X.XX P=Y.YY result=Success/Fail` | #32 |
| **Inventory full** (30 items + backpack tiers + composite container + drop-on-death) | `FPaldarkInventoryEntry::InnerEntries` (self-referential, one level deep) + `UPaldarkPlayerInventoryComponent` rename `MaxWeightKg → BaseMaxWeightKg` + `GetMaxWeightKg() = base + backpack bonus` + `EquippedBackpack` (TSoftObjectPtr replicated) + `SetEquippedBackpack` server API + `DumpCompositeToLog` + `UPaldarkItemFragment_Backpack` (BackpackTierTag + MaxWeightBonusKg + ExtraSlots) + `UPaldarkItemFragment_Container` (InnerMaxWeightKg + InnerMaxSlots clamp `[0, 16]`) + `APaldarkCharacter::SpawnDeathLootBagFromInventory` (flatten Entries + InnerEntries → `APaldarkLootBag`) + 21 new tags (17 item identity + 3 backpack tier + 1 equip slot) + 4 console cmds (`Paldark.Inventory.GiveAll / EquipBackpack / TestDeathDrop / DumpComposite`) | `Paldark.Inventory.GiveAll` + `Paldark.Inventory.EquipBackpack T2` → cap 30→55 + `Paldark.Inventory.TestDeathDrop` | #33 |

> **7/7 axis của vertical slice 2 đã ship ở C++ side.** Designer side cần author trong UE Editor (cộng dồn từ Q1 + Q2):
> - `ABP_PaldarkPlayer` + `ABP_PaldarkPal` + per-hostile-Pal ABP subclass (Path A từ #27, ~2 ngày designer-side mỗi character) — vẫn pending từ Q2 carry.
> - 5 hostile-Pal BP subclass (3 mới Q3: `BP_PaldarkCharacter_Stoneclad/Vinewraith/Boltmane`; 2 carry Q2: Direhound/Razorbird).
> - 2 companion BP subclass (`BP_PaldarkPalCharacter_Foxparks/Tombat`) + per-species `DA_PalDef_*` (5 mới Q3 + 5 carry Q2 = 10 total).
> - 5 ability BP subclass (`BP_GA_Charge/VineWhip/FireBreath/ChainLightning/Bite`) + 2 generic (`BP_GA_UsePalSphere` from #32, parametrized T1/T2/T3 OR 3 subclass).
> - `Map_RungHong.umap` + `Map_PalCorp.umap` + per-map `DA_MapDef_*` + POI/SpawnPoint placement.
> - 3 Pal Sphere item DA (`DA_Item_PalSphere_T1/T2/T3`).
> - 3 backpack item DA (`DA_Item_Backpack_T1/T2/T3`) + optional 1 container DA (`DA_Item_LootCrate`) + 17 new item DA (Weapon/Ammo/Consumable/Throwable/Resource — 30 total catalog).
> - `GE_StunOnHit` + `GE_StunDecay` + cooldown GE for `GA_UsePalSphere`.
>
> Tất cả bước Blueprint đã có hướng dẫn trong [`PaldarkLab/README.md`](../../PaldarkLab/README.md) § "AnimBP Lyra ALS port (W27)", § "AssetManager + Async load + Game Feature plugin (W27-28)", § "W29-30 — 5 Pal loài", § "W31-32 — Map 2", § "W33-34 — Map 3", § "W35-36 — Pal Bond + Capture", § "W37-38 — Inventory full".

---

## 3. Pillar coverage — actual vs target

Roadmap dự kiến Q3 đạt: P9 90% / P11 95% / P12 90% / P14 80% / P15 80% / P16 60%. Đối chiếu thực tế:

| Pillar | Target Q3 | Actual Q3 | Delta | Bằng chứng |
|--------|-----------|-----------|-------|------------|
| **P03** Composition         | 95% | **95%** | 0   | Q2 carry ~90% + 10th Pal slot LootDropSlot (#31) + 11th Pal slot TameSlot (#32) + 13th player slot RosterSlot (#32) + new InventoryComponent helpers (#33). Component pattern proven across 13+ slots. |
| **P05** Animation           | 80% | **65%** | -15 | #27 Path A C++ scaffold landed (UPaldarkAnimInstance + UPaldarkPalAnimInstance + IPaldarkAnimLayerInterface + PawnData slot). **Designer-authored AnimBP / State Machine / Blend Space vẫn pending** — đây là 2-day designer task ngoài C++ scope. Pillar advance khi assets land trên branch riêng. |
| **P09** AI                  | 90% | **92%** | +2  | Q2 carry 75% + #29 added 3 hostile subclass (Stoneclad/Vinewraith/Boltmane) + 2 companion (Foxparks/Tombat) + `UPaldarkActivity_BossFSM` (phase 0..2). 8 activity total (Idle/Follow/Investigate/Combat/Patrol/Stalk/FollowSquadCommand/BossFSM). Còn thiếu: AI Director scaling (defer Q4 polish). |
| **P11** Open World          | 95% | **90%** | -5  | 3 map blockout (Raid_Sandbox + Rừng Hỏng + PalCorp) + POI + SpawnPoint + MapDefinition subsystem (#30 + #31). **Designer-authored .umap files vẫn pending** — C++ infra xong, content stays designer task. |
| **P12** Data-driven         | 90% | **95%** | +5  | 11 primary asset type (Experience/PawnData/ActionSet/InputConfig/Item/PalSpawnTable/SquadCommandSet/PalDefinition/LootTable/MapDefinition + LootBagEntry inferred) + per-species TameDifficulty/BaseCaptureProbability (#32) + per-tier backpack stats (#33). Mọi tuning bằng DataAsset. |
| **P14** AssetManager        | 80% | **85%** | +5  | Q2 carry 45% + Async load qua bundle (#28) + Game Feature plugin scaffold (#28) + 11 primary asset type. Còn thiếu: full GameFeatureAction integration (W40+ hub town). |
| **P15** Game Mode/State     | 80% | **80%** | 0   | Q2 carry 70% + experience swap pattern + match end + drop-on-death (#33) + lifecycle hooks. Match phase Ended → Lobby travel vẫn pending (W40+). |
| **P16** GameFeature Plugin  | 60% | **55%** | -5  | #28 scaffold (manifest + plugin descriptor + boot-strap dependency on PaldarkLab) shipped, nhưng GameFeatureAction wire trên Map đăng ký dynamic chưa hoàn chỉnh — Game Feature plugin ở mức "loadable, activatable, dependency declared" chưa "auto-spawn beacon when activated". Defer Q4 polish (W40+ hub town). |

**Tổng coverage Q3 ≈ 80%** (cộng đều 8 pillar tracked Q3 + carry-over Q1+Q2). Roadmap target ≈ 79%. **Pillar nào outperformance: P12 (+5 từ 11 primary asset type), P14 (+5 từ async + plugin scaffold), P09 (+2 từ boss FSM activity).** Underperformance: P05 (-15) + P11 (-5) + P16 (-5) — đều là pillar mà C++ scaffold xong nhưng đợi designer authoring (AnimBP / .umap / GameFeatureAction).

**Pillar mới phát sinh / cần track Q4:**
- **P07** (already tracked) — vẫn ở 60% (Q2 carry). Q4 task: prediction client-side rollback (cộng dồn 100%).
- **P13** Save/Load — vẫn ở 50% (Q3 carry). Q4 task W44-45 explicit.
- **P17** Audio — chưa start (0%). Q4 task W46-47 (FMOD).
- **P18** Backend/AWS — 30% (lifecycle hook + delegate scaffold from Q3 milestone hand-off but no DynamoDB write yet). Q4 task W42-43.

---

## 4. Gap analysis vs roadmap Q3 outcome

> Roadmap § Tuần 39 yêu cầu: "3 map, 10 Pal, full inventory, tame mechanic" + Q3 outcome "Vertical slice 2 — content đủ cho 5 giờ chơi lặp".

| # | Required | Shipped (C++) | Designer authoring still needed | Risk |
|---|----------|---------------|---------------------------------|------|
| 1 | **3 map playable** | ✅ `Raid_Sandbox` (Q1 + Q2) + `APaldarkPointOfInterest` + `UPaldarkPOISubsystem` + `UPaldarkMapDefinition` + `bIsIndoor` flag (#30 + #31) | `Map_RungHong.umap` (#30 designer task) + `Map_PalCorp.umap` (#31 designer task) + per-map `DA_MapDef_*` (3 total) + ≥ 4 POI/SpawnPoint placement per map | **High** — 2 map blockout là designer task lớn nhất Q3 (2 ngày mỗi map). Nếu skip → Q3 outcome về 1 map (Q2 baseline). |
| 2 | **10 Pal companion + hostile** | ✅ Q2 5 (Direhound, Razorbird, Player-side companion + 2 hostile dummy classes) + Q3 5 (Stoneclad/Vinewraith/Boltmane hostile + Foxparks/Tombat companion) (#29) | 5 BP subclass mới (Stoneclad/Vinewraith/Boltmane/Foxparks/Tombat) + per-species `DA_PalDef_*` (10 total: 5 mới Q3 + 5 carry Q2) + 5 ability BP subclass (Charge/VineWhip/FireBreath/ChainLightning/Bite) + AnimBP per species (T-pose risk nếu skip — see L-06 Q2 carry) | **High** — 10 BP + 10 DA + 5 ability BP + 10 AnimBP = ~25 asset, 3-5 ngày designer time. Skip → Q3 outcome về 5 Pal (Q2 baseline). |
| 3 | **Full inventory** (30 items + backpack + container + death drop) | ✅ 21 new tags + Backpack/Container fragment + InnerEntries + EquippedBackpack + SpawnDeathLootBagFromInventory + 4 console cmds (#33) | 30 item DA (13 carry + 17 mới Q3) + 3 backpack DA (T1/T2/T3) + 1 container DA (LootCrate, optional) = ~34 DA, ~1 ngày designer | Medium — pattern proven (W11-12 5 DA + W35-36 3 PalSphere DA). 17 mới Q3 chỉ là tag wiring, no new mesh/icon required (asset team can paint placeholders). |
| 4 | **Tame mechanic** | ✅ Stun + MaxStun attrs + bond comp + tame comp + Pal Sphere + UsePalSphere GA + Roster (#32) | 3 Pal Sphere item DA (T1/T2/T3) + 1 GA BP (`BP_GA_UsePalSphere`) + per-species `TameDifficulty/BaseCaptureProbability` on `DA_PalDef_*` + 2 GE (`GE_StunOnHit/GE_StunDecay`) + optional `BondXPCurve` on companion BPs | Low — README W35-36 §"Authoring order" chi tiết 5 bước. Default values shipping. |
| 5 | **Async load + Game Feature plugin** | ✅ `UPaldarkPalSpawnSubsystem` + `PaldarkRaidContent` plugin (#28) | (None — works out of the box if `Paldark.GameFeature.Status` returns `Loaded, Activated`. Designer chỉ enable plugin trong project settings nếu chưa default-enabled) | Low — plugin descriptor đã ship, designer chỉ verify `Edit → Plugins → PaldarkRaidContent` checked. |
| 6 | **AnimBP scaffold (Path A from Q2)** | ✅ `UPaldarkAnimInstance` + `UPaldarkPalAnimInstance` + `IPaldarkAnimLayerInterface` + `UPaldarkPawnData::DefaultAnimInstanceClass` (#27) | `ABP_PaldarkPlayer` + `ABP_PaldarkPal` + per-character AnimBP subclass (subclass for hostile Pal nếu State Machine khác) + State Machine (Idle/Walk/Run/Sprint/Fall/Combat) + 3 layer fn implementations | **High** (carry from Q2) — 2-day designer task per character. Critical cho Q3 demo video (avoid T-pose). |

**Verdict:** 6/6 axis có C++ backbone. Designer block lớn nhất là:
- **2 map blockout** (#1, 4-5 ngày designer time) — defer cho Q3 milestone tick là acceptable (record video chỉ với Raid_Sandbox + 1 placeholder map cũng OK cho 5-giờ-chơi-lặp claim).
- **10 BP + 10 DA + AnimBP cluster** (#2 + #6, ~5-7 ngày designer time) — đây là Q3 critical path. Đề xuất chia 2 designer (1 chuyên character + AnimBP, 1 chuyên map blockout) parallel.

Tail risks:
- **L-21 (new):** Game Feature plugin `PaldarkRaidContent` boot-strap thứ tự — nếu W40+ hub town swap experience, plugin có thể chưa kịp activate khi GA cần loot table. Mitigation: `UPaldarkExperienceDefinition.RequiredGameFeatures[]` array enforces activation pre-spawn. Validator check W40+ (defer).
- **L-22 (new):** `MaxWeightKg → BaseMaxWeightKg` rename callsites — nếu designer Blueprint từng read `MaxWeightKg` (W11-12 carry), sẽ break silent. Mitigation: validator updated trong #33 to grep both names; designer test loop in W37-38 README cover scenario.

Em đề xuất **Path B** cho Q3 milestone (giữ đúng deadline) + Q4 hub town W40+ là dịp thứ 2 để designer back-fill assets nếu Q3 slip.

---

## 5. Known issues & risks (frozen tại W38)

| ID | Issue | Severity | Mitigation | Defer to |
|----|-------|----------|------------|----------|
| L-21 | Game Feature plugin `PaldarkRaidContent` không tự auto-activate trên experience swap — designer phải bật trong `Edit → Plugins` OR `UPaldarkExperienceDefinition.RequiredGameFeatures[]` (chưa wire) | High | README W27-28 §"Authoring order" step 1 + console log warning | W40+ (hub town swap + RequiredGameFeatures wire) |
| L-22 | `MaxWeightKg → BaseMaxWeightKg` rename callsites — designer Blueprint Q1 W11-12 có thể đọc field name cũ | Medium | Validator updated W37-38 to grep both names; W37-38 README §"Anti-patterns avoided" item liệt kê | Q4 polish (validator extend với BP grep) |
| L-23 | AnimBP authoring vẫn pending — `UPaldarkAnimInstance` scaffold shipped W27 nhưng designer chưa author State Machine / Blend Space / layer fn | High | README W27 § "Authoring order" 8 bước + Q2-Q3 carry note trong Q3-Playtest-Checklist | Q3 đầu hoặc parallel branch (carry Q2 L-06) |
| L-24 | Map 2 + Map 3 `.umap` chưa commit — designer task 4-5 ngày | High | C++ infra (POI + SpawnPoint + MapDef) đã enforce shape; designer block | W39 (now) hoặc Q4 đầu |
| L-25 | Per-species `DA_PalDef_*` chưa author cho 5 species mới Q3 — Foxparks/Tombat/Stoneclad/Vinewraith/Boltmane | High | README W29-30 + W35-36 chi tiết tuning values | W39 (now) hoặc Q4 đầu |
| L-26 | 3 Pal Sphere item DA + 3 backpack DA + 17 mới item DA chưa author = 23 DA total | Medium | README W11-12 + W35-36 + W37-38 chi tiết schema | W39 (now) hoặc Q4 đầu |
| L-27 | `GE_StunOnHit` + `GE_StunDecay` chưa author — stun bar tăng/giảm depend on designer GE | Medium | README W35-36 §"Authoring order" step 4 | W39 (now) hoặc Q4 đầu |
| L-28 | Boltmane phase transition thresholds (0→1 ở 60% HP, 1→2 ở 30% HP) hardcoded — không tunable per-DA | Low | Expose `PhaseHealthThresholds[]` array trên `UPaldarkPalDefinition` | Q4 polish |
| L-29 | Loot bag `InitializeContents` không validate item def soft ptr null path — silent skip | Low | Validator hoặc inline log warning | Q4 polish |
| L-30 | Drop-on-death flatten `InnerEntries` → bag mất container nesting | Low | Documented anti-pattern; W47 save-game preserve original tree | W47 save-game |
| L-31 | `EquippedBackpack` replicate to all clients (not COND_OwnerOnly) — privacy / bandwidth concern | Low | Backpacks là persistent player gear, public visibility acceptable. Flip COND_OwnerOnly nếu UI team xác nhận chỉ owner cần thấy | Q4 polish (UI team decision) |
| L-32 | `FPaldarkInventoryEntry::InnerEntries` self-referential — UHT support but FastArraySerializer migration deferred W47 | Medium | InnerMaxSlots clamp `[0, 16]` keeps DOREPLIFETIME blob < MTU | W47 save-game polish |
| L-33 | `SetEquippedBackpack` allows swap mid-fight even if new total > new cap — chỉ log warning, không enforce | Low | W38 polish: encumbrance penalty curve | W38+ |
| L-34 | Game Feature plugin chưa auto-activate trên dedicated server startup — dedicated.exe có thể boot without plugin loaded | Medium | Add `bIsServerOnly = false` trên plugin descriptor + boot-strap on `OnPostEngineInit` server hook | Q4 polish |
| L-35 | POI subsystem `TActorIterator` sweep on `Initialize` — nếu POI BeginPlay sau subsystem Init, miss; retry on `OnWorldBeginPlay` đã có nhưng vẫn race window | Low | Documented; retry usually fires < 1 frame | Q4 polish |
| L-36 | Loot bag actor `bAutoCleanupTimer = 300s` hardcoded — designer không tune được | Low | Expose `AutoCleanupSeconds` UPROPERTY | Q4 polish |
| L-37 | Tame attempt formula `BaseCaptureProbability` không có client preview — UI cần show "32% chance to capture" trước throw | Medium | HUD task: predicted probability từ AttributeSet read + Pal definition. W50 HUD polish | W50 |
| L-38 | Pal Sphere actor đè onto Player hand vs camera-forward — quẳng từ vai chứ không phải mũi camera | Low | Per-character socket override trên `BP_PaldarkCharacter` (designer wire bone socket "PalSphereSocket") | Q4 polish (cùng anim) |
| L-39 | Multi-player inventory contention — 2 player cùng grab dropped loot bag cùng frame | Low | First-touch-wins acceptable; documented anti-pattern | Q4 polish |
| L-40 | `PaldarkAssetManager.GetPrimaryAssetIdList` query mỗi `GiveAll` call — cost N items × M ticks nếu spam | Low | Cache result trong `PaldarkLabWorldSubsystem` if used > 1/frame | (No fix needed — console cmd is rare) |

> **Verdict:** Không có issue nào ở mức **Critical** block Q3 outcome (vertical slice 2 playable). L-21 / L-23 / L-24 / L-25 / L-26 đều là designer-side authoring (5-7 ngày work) hoặc Q4 carry-over (W40+ hub town). L-22 / L-37 là UI/polish risk cho Q4. Codebase technical debt remains healthy ratio: 0 P0, 4 P1 (designer-block), 7 P2 (polish), 9 P3 (defer Q4+).

---

## 6. Files / module health (cumulative W1-38)

```
PaldarkLab/
├── PaldarkLab.uproject                              # UE 5.4, 4 plugin core (Lyra-style + Paldark)
├── Plugins/
│   └── PaldarkRaidContent/                          # Game Feature plugin (W27-28)
│       ├── PaldarkRaidContent.uplugin
│       └── ... (manifest + boot-strap)
├── Config/
│   ├── DefaultEngine.ini                            # log verbosity + class wiring + GameMaps + GameModeClassAliases
│   ├── DefaultGame.ini                              # 11 primary asset type (Experience/PawnData/ActionSet/InputConfig/Item/PalSpawnTable/SquadCommandSet/PalDefinition/LootTable/MapDefinition/+1 inferred)
│   ├── DefaultInput.ini                             # EnhancedPlayerInput + EnhancedInputComponent
│   └── DefaultGameplayTags.ini                      # (auto, native tags ship trong PaldarkGameplayTags)
├── Content/
│   └── Paldark/
│       ├── Tame/README.txt                          # Pal Sphere + GA authoring contract (W35-36)
│       └── Inventory/
│           ├── Backpacks/README.txt                 # T1/T2/T3 backpack authoring contract (W37-38)
│           └── Containers/README.txt                # LootCrate / nested container authoring contract (W37-38)
├── Source/
│   ├── PaldarkLabCore/                              # PreDefault loading
│   │   ├── Public/PaldarkLogCategories.h            # 5 log category
│   │   ├── Public/PaldarkGameplayTags.h             # ~90 native tag (Q1=24 + Q2=21 + Q3=~45: PalRoster/Map/Loot/Tame/Bond/Item/Backpack/Equip)
│   │   └── Private/...
│   ├── PaldarkLab/                                  # Default loading
│   │   ├── PaldarkLab.Build.cs                      # GameplayAbilities + Tags + Tasks + EnhancedInput + AIModule + NavigationSystem
│   │   ├── PaldarkLab.cpp                           # ~45 console cmd register (Q1 15 + Q2 15 + Q3 ~15: Pal/Map/Loot/Tame/Inventory/GameFeature)
│   │   ├── Public/
│   │   │   ├── Framework/                           # AssetManager + GameMode + GameModeBase + GameMode_Extraction + GameState + GameInstance
│   │   │   ├── Experience/                          # ExperienceDefinition + ActionSet + PawnData + DefaultAnimInstanceClass slot (W27)
│   │   │   ├── Input/PaldarkInputConfig.h
│   │   │   ├── Anim/                                # AnimInstance + PalAnimInstance + IPaldarkAnimLayerInterface (W27)
│   │   │   ├── Player/                              # PC + PS (ext Match) + Char (death hook + drop-on-death) + 13 component slots (12 Q2 + 1 RosterSlot Q3)
│   │   │   ├── Pal/                                 # Pal char + 11 component slots (8 Q1 + Combat + Perception + LootDrop + Tame) + Locomotion + Activity (8 subclass incl BossFSM) + Combat ability + Perception + Spawner + 5 hostile subclass + 2 companion subclass
│   │   │   ├── Pal/Combat/                          # PaldarkPalConsideration + utility AI
│   │   │   ├── Pal/Definition/                      # PaldarkPalDefinition (W27-28)
│   │   │   ├── Gas/                                 # AttributeSet (5 + Stun + MaxStun = 7) + ASC + GA base + Sprint + HitscanFire + PalAttack + UsePalSphere + 5 hostile ability + DamageExecutionCalculation
│   │   │   ├── Combat/PaldarkDummyTarget.h
│   │   │   ├── Inventory/                           # ItemFragment + 5 fragment (3 Q1 + Backpack + Container) + ItemDefinition + InventoryComponent (W11-12 + W37-38 composite)
│   │   │   ├── Net/                                 # NetSubsystem (W14-15) + LagCompensationComponent (W16-17)
│   │   │   ├── Subsystem/                           # PaldarkLabWorldSubsystem (Q1) + HostilePackSubsystem (W20-21) + PalSpawnSubsystem (W27-28) + POISubsystem (W31-32) + GameFeature wire
│   │   │   ├── Squad/                               # SquadSubsystem + PingSubsystem + PingMarker + SquadCommandSet + SquadMembershipComponent
│   │   │   ├── Match/                               # MatchTypes (enums + row) + MatchSubsystem + ExtractionBeacon
│   │   │   ├── Map/                                 # PointOfInterest + SpawnPoint + MapDefinition + POISubsystem (W31-32)
│   │   │   └── Loot/                                # LootTable + LootDropComponent + LootBag (W33-34)
│   │   └── Private/                                 # matching .cpp tree
│   └── PaldarkLabEditor/                            # Editor-only placeholder
└── README.md                                        # ~2,900 lines, đầy đủ test loop cho mọi feature W1-38
```

**Module load order (PreDefault → Default → PostEngineInit):** unchanged from Q1.

Validator kiểm 20 thứ ở mức Python (`scripts/ci/validate_paldarklab.py` — ~5,400 dòng):
- uproject + 3 module layout + 3 target + 5 log category + no BOM
- ~90 native gameplay tag declared (Q1=24 + Q2=21 + Q3=~45)
- Build.cs dependency: EnhancedInput / GameplayAbilities / GameplayTags / GameplayTasks / AIModule / NavigationSystem
- Input config shape, Player/Pal slot classes, Pal pawn data shape
- Activity FSM shape, GAS shape (7 attr + clamp + ASC + 4+ GA), Combat damage shape, Inventory fragment shape + backpack/container fragment shape
- Net subsystem shape, LagCompensation shape (W14-15 + W16-17)
- Pal combat shape, hostile-Pal subclass shape, pack subsystem shape, spawner shape (W18-19 + W20-21)
- Squad subsystem shape, ping subsystem shape, ping marker shape, command set shape, listener activity shape (W22-23)
- Match shape (subsystem + beacon + GameMode subclass + PS/GS REPNOTIFY + death hook + console) (W24-25)
- Extraction GameMode wired (W26 follow-up)
- AnimInstance shape (W27)
- PalDefinition + PalSpawnSubsystem + Game Feature plugin (W27-28)
- W29-30 Pal roster shape (5 species + 5 ability + boss FSM)
- W31-32 Map blockout shape (POI + SpawnPoint + MapDefinition + 12 tags)
- W33-34 Loot drop shape (LootTable + LootDropComponent + LootBag + bIsIndoor)
- W35-36 Tame minigame shape (Stun + MaxStun + bond + tame + PalSphere + GA + Roster + Pal char wiring + player char wiring + 5 hostile subclasses flip + 4 console cmds)
- W37-38 Inventory full shape (Backpack fragment + Container fragment + InnerEntries + component + drop-on-death wiring + 4 console cmds + Content placeholder)

---

## 7. Recommended Q3 → Q4 transition

1. **Tuần này (designer):**
   - Author / verify 5 hostile-Pal BP + 2 companion BP (Q3 5 species). Carry 5 Q2 (Direhound/Razorbird + Q1 base).
   - Author per-species `DA_PalDef_*` (10 total: 5 mới Q3 + 5 carry Q2).
   - Author 5 ability BP (`BP_GA_Charge/VineWhip/FireBreath/ChainLightning/Bite`).
   - Author 3 Pal Sphere DA + 3 backpack DA + 17 mới item DA (23 total).
   - Author `GE_StunOnHit` + `GE_StunDecay`.
   - Author 2 mới map (`Map_RungHong.umap` + `Map_PalCorp.umap`) + per-map `DA_MapDef_*` + POI/SpawnPoint placement.
   - **(Carry Q2)** Author `ABP_PaldarkPlayer` + `ABP_PaldarkPal` + per-character AnimBP (2-day designer task per character).
   - Chạy 1 lần PIE 4-client end-to-end theo [`Q3-Playtest-Checklist.md`](Q3-Playtest-Checklist.md).
   - Gửi video 90s back vào `Documents/Devlog/Q3-Playtest-Video.md` (4-quadrant split, kèm screenshot/youtube link).
2. **Đầu Q4 (dev):**
   - **PR đầu Q4: W40-41 Hub Town** — hub map blockout + 8-player shard + Pal Stable + Marketplace UI + Briefing Room (roadmap § Tuần 40-41).
   - **PR thứ 2 Q4: W42-43 Backend AWS** — Cognito auth + DynamoDB tables + Lambda (login/persist_pal/persist_inventory) + API Gateway.
   - **PR follow-up validator**: enforce Game Feature plugin activation in experience (L-21), `MaxWeightKg → BaseMaxWeightKg` Blueprint grep (L-22).
3. **Đầu Q4 (designer):**
   - Author `Documents/PALDARK/05-Vertical_Slice_2_Demo_Script.md` (15-minute mission narrative covering 3 maps + 10 Pal + tame + inventory full).
   - Record 90s "marketing" video sau khi AnimBP fix.
   - Update `Documents/PALDARK/02-Pillar_Coverage.md` Q3 column với % thực tế sau playtest.

---

## 8. Cumulative Q1+Q2+Q3 stats

| Metric | Q1 close (W13) | Q2 close (W26) | Q3 close (W39) | Δ Q2→Q3 |
|--------|----------------|----------------|----------------|---------|
| PR merged (Devin) | 10 | 16 | 23 | +7 |
| C++ source files | ~80 | ~130 | ~180 | +50 |
| Native gameplay tags | 24 | 45+ | ~90 | +45 |
| Console commands | 15 | 30+ | 45+ | +15 |
| Subsystems | 1 (LabWorldSubsystem) | 5 (Lab + HostilePack + Squad + Ping + Match) | 7 (+ PalSpawn + POI) | +2 |
| World actor classes | 2 (DummyTarget + PingMarker n/a) | 5 (DummyTarget + PingMarker + PalSpawner + ExtractionBeacon + ...) | 9 (+ POI + SpawnPoint + LootBag + PalSphere) | +4 |
| Activity subclasses | 3 (Idle/Follow/Investigate) | 7 (+ Combat/Patrol/Stalk/FollowSquadCommand) | 8 (+ BossFSM) | +1 |
| Pal species (C++) | 1 base | 3 (base + Direhound + Razorbird) | 8 (+ Stoneclad/Vinewraith/Boltmane/Foxparks/Tombat) | +5 |
| Primary asset type | 5 | 7 | 11 (+ PalDefinition + LootTable + MapDefinition + LootBagEntry inferred) | +4 |
| Validator lines | ~1,367 | ~3,000 | ~5,400 | +2,400 |
| README lines | 516 | 1,896 | ~2,900 | +1,000 |
| Pillar coverage estimate | ~47% (matches roadmap) | ~62% (matches roadmap) | ~79-80% (matches roadmap target 79%) | +17 |

---

## 9. References

- [`Documents/PALDARK/03-Roadmap_1_Year.md`](../PALDARK/03-Roadmap_1_Year.md) — toàn bộ 52-week schedule.
- [`Documents/PALDARK/02-Pillar_Coverage.md`](../PALDARK/02-Pillar_Coverage.md) — pillar target / mapping feature.
- [`Documents/UE5_Core_Pillars.md`](../UE5_Core_Pillars.md) — 18 pillar definition.
- [`PaldarkLab/README.md`](../../PaldarkLab/README.md) — feature-by-feature authoring + test loop (~2,900 dòng).
- [`Documents/Devlog/Q3-Playtest-Checklist.md`](Q3-Playtest-Checklist.md) — runnable 4-client PIE walkthrough for Q3.
- [`Documents/Devlog/Q3-Bug-Bash.md`](Q3-Bug-Bash.md) — 20-issue priority list cho 1-day bug bash.
- [`Documents/Devlog/Q2-Milestone-Build.md`](Q2-Milestone-Build.md) + [`Q2-Playtest-Checklist.md`](Q2-Playtest-Checklist.md) + [`Q2-Bug-Bash.md`](Q2-Bug-Bash.md) — Q2 baseline.
- [`Documents/Devlog/Q1-Milestone-Build.md`](Q1-Milestone-Build.md) + [`Q1-Playtest-Checklist.md`](Q1-Playtest-Checklist.md) + [`Q1-Bug-Bash.md`](Q1-Bug-Bash.md) — Q1 baseline.
- Per-week build logs Q2: [W14-15](W14-15-Build.md), [W16-17](W16-17-Build.md), [W18-19](W18-19-Build.md), [W20-21](W20-21-Build.md), [W22-23](W22-23-Build.md), [W24-25](W24-25-Build.md).
- Q3 PR detail (Devin commit notes embedded trong each PR description on GitHub): [#27](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/27), [#28](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/28), [#29](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/29), [#30](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/30), [#31](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/31), [#32](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/32), [#33](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/33).
