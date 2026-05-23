# PALDARK — Q3 Playtest Checklist (Tuần 39)

> **Goal:** 1 file walkthrough chạy 1 lần PIE 4-client trong UE 5.4 Editor, chạm hết feature W27-38. Designer in trang này ra giấy → tick từng box → screenshot → ship video 90s 4-quadrant.
>
> **Prerequisites:**
> - UE 5.4 Editor (chưa có Toolchain trên VM — phải chạy trên máy designer / dev).
> - `git pull` về `main` (commit `3bb7f00` trở lên — sau PR #33).
> - Q1 + Q2 milestone đã đóng (designer đã author đầy đủ asset từ [`Q1-Playtest-Checklist.md`](Q1-Playtest-Checklist.md) § 0 + [`Q2-Playtest-Checklist.md`](Q2-Playtest-Checklist.md) § 0).
> - [`Documents/Devlog/Q3-Milestone-Build.md`](Q3-Milestone-Build.md) đọc qua một lần.

---

## 0. Q3 setup delta (≈ 5-7 ngày designer — bổ sung vào Q2 setup)

> Tất cả bước Blueprint authoring đã có chi tiết trong [`PaldarkLab/README.md`](../../PaldarkLab/README.md). Section này chỉ tóm ngắn để designer biết phải xếp đúng thứ tự cộng dồn từ Q2. Đây là content-heavy quarter — assume 5-7 ngày designer time tổng, chia 2 designer (1 character/AnimBP + 1 map) parallel.

### 0.1 AnimBP scaffold (W27 — carry from Q2 L-06)

- [ ] **`ABP_PaldarkPlayer.uasset`** subclass `UPaldarkAnimInstance`. State Machine: Idle/Walk/Run/Sprint/Fall/Combat. Blend Space: WalkBS (Speed + Direction). Implement 3 layer fn from `IPaldarkAnimLayerInterface`: `PlayLocomotion`, `PlayCombat`, `PlayMontage`.
- [ ] **`ABP_PaldarkPal.uasset`** subclass `UPaldarkPalAnimInstance`. Same State Machine + 3 extra branches based on `bIsHostile/bIsFlying/bIsInCombat` exposed properties.
- [ ] **`PD_RaidPlayer`** + `PD_PaldarkPal_*` → `DefaultAnimInstanceClass = ABP_PaldarkPlayer / ABP_PaldarkPal`. Note: setting on PawnData (not directly on Mesh) lets `APaldarkCharacter::SetPawnData` swap AnimInstance per experience.
- [ ] **Verify thread-safe pattern.** Open `UPaldarkAnimInstance::NativeThreadSafeUpdateAnimation` and ensure no raw `UObject*` is dereferenced — only `LocomotionData` USTRUCT snapshot. Designer should test in PIE: enable `Show Async Anim Updates` (`a.AnimInstance.Async 2`), verify no warning về "GameThread access".

### 0.2 AssetManager async + Game Feature plugin (W27-28)

- [ ] **Enable `PaldarkRaidContent` plugin.** `Edit → Plugins` → search "PaldarkRaidContent" → check Enabled → restart Editor.
- [ ] **Verify plugin loaded.** PIE → server console: `Paldark.GameFeature.Status` → `plugins=[PaldarkRaidContent: Activated]`.
- [ ] **Async warmup.** Server: `Paldark.Pal.WarmAll` → log `warming N PalDefinition assets, queued, succeeded` (N = number of `DA_PalDef_*` registered as `PrimaryAssetType=PaldarkPalDef`).
- [ ] **Spawn via subsystem.** Server: `Paldark.Pal.Spawn Direhound` → 1 Direhound spawn (async load if not pre-warmed, immediate if pre-warmed).

### 0.3 10-Pal roster (W29-30 + Q2 carry)

- [ ] **5 hostile-Pal BP subclass + AnimBP wire.** Existing Q2: `BP_PaldarkCharacter_Direhound` + `BP_PaldarkCharacter_Razorbird`. New Q3 (3 more):
  - `BP_PaldarkCharacter_Stoneclad` parent `APaldarkPalCharacter_Stoneclad`. CandidateActivities=[Patrol, Stalk, Combat]. TeamTag=Hostile. AnimBP `ABP_PaldarkPal_Stoneclad`.
  - `BP_PaldarkCharacter_Vinewraith` parent `APaldarkPalCharacter_Vinewraith`. Same. AnimBP placeholder ABP_Manny if rồ chuyên ABP chưa author.
  - `BP_PaldarkCharacter_Boltmane` parent `APaldarkPalCharacter_Boltmane`. BossFSM activity in CandidateActivities. Phase 0..2 transition tags = `Paldark.Pal.BossPhase.0/1/2`. AnimBP `ABP_PaldarkPal_Boltmane`.
- [ ] **2 companion-Pal BP subclass.** New Q3:
  - `BP_PaldarkPalCharacter_Foxparks` parent `APaldarkPalCharacter_Foxparks`. CandidateActivities=[Idle, Follow, Investigate, Combat, FollowSquadCommand]. TeamTag=Friendly. AnimBP `ABP_PaldarkPal_Foxparks`.
  - `BP_PaldarkPalCharacter_Tombat` parent `APaldarkPalCharacter_Tombat`. Same companion config. AnimBP `ABP_PaldarkPal_Tombat`.
- [ ] **5 ability BP subclass.** New Q3:
  - `BP_GA_Charge` parent `UPaldarkGameplayAbility_Charge`. Tag: `Paldark.Ability.Charge`. Damage SetByCaller default 20.
  - `BP_GA_VineWhip` parent `UPaldarkGameplayAbility_VineWhip`. Tag: `Paldark.Ability.VineWhip`. Damage SetByCaller default 15. Apply slow GE on hit (defer slow GE designer Q4).
  - `BP_GA_FireBreath` parent `UPaldarkGameplayAbility_FireBreath`. Tag: `Paldark.Ability.FireBreath`. Damage SetByCaller default 25. Multi-target cone.
  - `BP_GA_ChainLightning` parent `UPaldarkGameplayAbility_ChainLightning`. Tag: `Paldark.Ability.ChainLightning`. Damage SetByCaller default 30. Chain hop count 3.
  - `BP_GA_Bite` parent `UPaldarkGameplayAbility_Bite`. Tag: `Paldark.Ability.Bite`. Damage SetByCaller default 12. Melee range 200cm.
- [ ] **10 `DA_PalDef_*` per-species** (5 Q2 carry + 5 mới Q3):
  - `DA_PalDef_Direhound` — SpeciesTag=Direhound, PalClass=BP_PaldarkCharacter_Direhound, TameDifficulty=1.0, BaseCaptureProbability=0.35.
  - `DA_PalDef_Razorbird` — SpeciesTag=Razorbird, ..., TameDifficulty=1.2, BaseCaptureProbability=0.30.
  - `DA_PalDef_Stoneclad` — SpeciesTag=Stoneclad, ..., TameDifficulty=1.8, BaseCaptureProbability=0.20.
  - `DA_PalDef_Vinewraith` — SpeciesTag=Vinewraith, ..., TameDifficulty=1.4, BaseCaptureProbability=0.25.
  - `DA_PalDef_Boltmane` — SpeciesTag=Boltmane, ..., TameDifficulty=3.0, BaseCaptureProbability=0.10 (boss sub-1%).
  - `DA_PalDef_Foxparks` — SpeciesTag=Foxparks, PalClass=BP_PaldarkPalCharacter_Foxparks. (Companion-only — `bIsTameable=false` default but field tunable.)
  - `DA_PalDef_Tombat` — SpeciesTag=Tombat, PalClass=BP_PaldarkPalCharacter_Tombat.
  - (3 more if base companion classes use this pattern.)

### 0.4 Map 2 "Rừng Hỏng" (W31-32)

- [ ] **`Map_RungHong.umap`** new map under `/Game/Paldark/Maps/`. Designer blockout: 800×800m terrain + 4-6 POI placement (forest theme: ruined shrines, mossy clearings, fallen logs).
- [ ] **Place 4× `BP_PointOfInterest`** (subclass `APaldarkPointOfInterest`). Per-POI: `POITag = Paldark.Map.POI.Glade` or `RuinedShrine` or `MossyClearing` or `FallenLog`. `Label = "Glade"` etc.
- [ ] **Place ≥ 4× `BP_PalSpawnPoint`** (subclass `APaldarkPalSpawnPoint`). Each links to a POI tag (e.g., `LinkedPOITag=Paldark.Map.POI.Glade`). SpawnTable references `Direhound/Razorbird/etc.`
- [ ] **`DA_MapDef_RungHong.uasset`** subclass `UPaldarkMapDefinition`. PointsOfInterest array fills with 4-6 POI tags. WeatherTag=`Paldark.Map.Weather.Fog` or `Rain`. BiomeTag=`Paldark.Map.Biome.Forest`. `bIsIndoor = false`.
- [ ] **Register `DA_MapDef_RungHong` as PrimaryAsset.** Project Settings → AssetManager → PrimaryAssetType="PaldarkMapDef" (already auto from W31-32 if `DA_MapDef_*` lives in `/Game/Paldark/Maps/Definitions/`).
- [ ] **Verify POI discovery.** PIE on `Map_RungHong` → server console: `Paldark.Map.DumpPOIs` → log `POIs: 4-6 entries=[{POI_Glade=Paldark.Map.POI.Glade}, ...]`.
- [ ] **Verify spawn points.** Server: `Paldark.Map.DumpSpawnPoints` → log `SpawnPoints: N entries=[...]`.
- [ ] **Verify map definition.** Server: `Paldark.Map.Definition` → log `MapDef=DA_MapDef_RungHong WeatherTag=Fog BiomeTag=Forest bIsIndoor=false`.

### 0.5 Map 3 "Cơ Xưởng PalCorp" + loot drop (W33-34)

- [ ] **`Map_PalCorp.umap`** new map under `/Game/Paldark/Maps/`. Designer blockout: indoor facility theme, 4-6 POI (lobby, corridor, lab, server room, etc.).
- [ ] **Place 4× `BP_PointOfInterest`** with PalCorp tags (Paldark.Map.POI.PalCorpLobby/Corridor/Lab/ServerRoom).
- [ ] **Place ≥ 4× `BP_PalSpawnPoint`** indoor variation (Razorbird disabled — indoor; replace with Stoneclad/Vinewraith).
- [ ] **`DA_MapDef_PalCorp.uasset`** subclass `UPaldarkMapDefinition`. WeatherTag=`Paldark.Map.Weather.Indoor`. BiomeTag=`Paldark.Map.Biome.Industrial`. **`bIsIndoor = true`** (suppresses Razorbird spawn from spawner; flying mode loses purpose indoors).
- [ ] **Author 3 loot tier DA + 1 base loot table.**
  - `DA_LootTable_Common` — entries: `[(DA_Item_Resource_Wood, 0.5, 1..3, 0.8), (DA_Item_Resource_Stone, 0.5, 1..2, 0.5)]` (item, weight, count range, drop chance).
  - `DA_LootTable_Tech` — entries: `[(DA_Item_Resource_PalCore, 0.5, 1, 0.6), (DA_Item_Resource_Credits, 0.3, 10..50, 0.4)]`. Used by PalCorp.
  - `DA_LootTable_Rare` — entries: `[(DA_Item_Backpack_T2, 0.1, 1, 0.05), (DA_Item_Backpack_T3, 0.05, 1, 0.01)]`. Used by boss/elite Pal.
- [ ] **Wire `UPaldarkLootDropComponent`** to per-species `BP_PaldarkCharacter_*`. Set `LootTable=DA_LootTable_*` per species (Direhound → Common, Boltmane → Rare + Tech, etc.).
- [ ] **Verify loot bag.** PIE on `Map_PalCorp` → spawn Direhound, kill it. Loot bag actor spawns at death transform, walk close → interact prompt → pickup transfers items.

### 0.6 Pal Bond + Capture (W35-36)

- [ ] **Author 3 Pal Sphere item DA** under `/Game/Paldark/Items/`:
  - `DA_Item_PalSphere_T1` — ItemTag=Paldark.Item.PalSphere.T1, Stack(8), Weight(0.5).
  - `DA_Item_PalSphere_T2` — ItemTag=Paldark.Item.PalSphere.T2, Stack(6), Weight(0.6).
  - `DA_Item_PalSphere_T3` — ItemTag=Paldark.Item.PalSphere.T3, Stack(3), Weight(0.8).
- [ ] **Author `BP_GA_UsePalSphere`** subclass `UPaldarkGameplayAbility_UsePalSphere`. PalSphereTierTag=Paldark.Item.PalSphere.T1, PalSphereClass=`APaldarkPalSphere`, CooldownGE=`GE_Cooldown_UsePalSphere` (1s). (Or 3 subclass T1/T2/T3.)
- [ ] **Grant ability** via `UPaldarkAbilitySet_Player` so player auto-receives on possess.
- [ ] **Author 2 stun GE** under `/Game/Paldark/Tame/`:
  - `GE_StunOnHit` — instant, magnitude = `Paldark.SetByCaller.PalDamage * 0.5`. Wire vào `UPaldarkPalCombatComponent.AttackGE` (apply to target on hit).
  - `GE_StunDecay` — periodic (Period=0.25s, Magnitude=-5/sec). Apply on Pal BeginPlay so stun drains naturally.
- [ ] **(Optional) Author `BondXPCurve`** trên `BP_PaldarkPalCharacter_Foxparks/Tombat`. Keys: (1, 100), (5, 600), (10, 2000), (20, 10000).
- [ ] **Verify.** PIE → spawn Direhound + stun + low-HP + throw Pal Sphere → server log `BeginTameAttempt rolled=X.XX P=Y.YY result=Success/Fail`.

### 0.7 Inventory full (W37-38)

- [ ] **Author 17 mới item DA** (W37-38 README §"Authoring order" step 1 chi tiết schema):
  - Weapons: `DA_Item_Weapon_Rifle/Shotgun/SMG/Melee_Baton` (4).
  - Ammo: `DA_Item_Ammo_Rifle/Shotgun/SMG` (3).
  - Consumable: `DA_Item_Consumable_Medkit/Stimpack/PalFood` (3).
  - Throwable: `DA_Item_Throwable_Frag/Smoke/Flash` (3).
  - Resource: `DA_Item_Resource_Wood/Stone/PalCore/Credits` (4).
- [ ] **Author 3 backpack DA** + Backpack fragment + Equipable fragment:
  - `DA_Item_Backpack_T1` — Backpack(T1, MaxWeightBonusKg=10, ExtraSlots=2).
  - `DA_Item_Backpack_T2` — Backpack(T2, MaxWeightBonusKg=25, ExtraSlots=5).
  - `DA_Item_Backpack_T3` — Backpack(T3, MaxWeightBonusKg=45, ExtraSlots=10).
- [ ] **(Optional) Author 1 container DA** `DA_Item_LootCrate` with `UPaldarkItemFragment_Container(InnerMaxWeightKg=5, InnerMaxSlots=4)`.
- [ ] **Verify.** PIE → `Paldark.Inventory.GiveAll` → expect `granted=30` (Q1=5 + Q2=3 PalSphere + Q3=17 + Q3=3 backpack + Q3=1 container = ~29-30 tổng tùy designer).

✅ Sau bước này Content folder phải có thêm (so với Q2): ~10 hostile-Pal AnimBP + ~10 ABP layers + 5 ability BP + 10 PalDef DA + 3 Pal Sphere DA + 3 backpack DA + 17 item DA + 3 loot table DA + 2 stun GE + 1 cooldown GE + 2 mới map + 2 mới MapDef + 8-12 POI/SpawnPoint placement.

---

## 1. 4-player listen-server connect (P06 + P15)

> Carry từ Q2 § 1. Cùng setup PIE 4 viewport + listen server, nhưng giờ chọn map khác để test variety.

```
Editor PIE settings → Number of Players = 4 + Net Mode = Play As Listen Server
```

- [ ] PIE 4 viewport mở `Raid_Sandbox` không crash. 4 player spawn ở 4 PlayerStart.
- [ ] Server console: `Paldark.Net.DumpRoster` → `players=4 controllers=[PC_0..PC_3]`.
- [ ] Server: `Paldark.Match.Dump` → `Phase=Warmup Players=4 Alive=4`.
- [ ] **Switch to Map 2.** Server: `ServerTravel /Game/Paldark/Maps/Map_RungHong` (hoặc reload PIE với Default Map = `Map_RungHong`). All 4 client should re-spawn.
- [ ] **Switch to Map 3.** Same với `Map_PalCorp`. Verify indoor flag: server: `Paldark.Map.Definition` → `bIsIndoor=true`.

📸 Screenshot 1: 4-quadrant PIE viewport trên cả 3 map (chia ra 3 screenshot riêng nếu cần).

---

## 2. AnimBP scaffold (W27)

Goal: AnimBP thread-safe pattern không touch raw UObject + State Machine transitions clean.

- [ ] **Locomotion swap.** PIE on `Raid_Sandbox`. Hold W (forward) → Pal nên play `Walk` Anim. Sprint (LShift) → `Run`. Air → `Fall`.
- [ ] **Combat layer.** Companion Pal in Combat activity → `bIsInCombat=true` exposed property → `Combat` branch fires. Server: `Paldark.Pal.CurrentActivity` confirm Combat.
- [ ] **Thread-safe debug.** Server console: `a.AnimInstance.Async 2` → warning về raw `UObject*` access trong `NativeThreadSafeUpdateAnimation`? **Must be 0**. Nếu warning → designer mistake trong `ABP_PaldarkPlayer` graph.
- [ ] **Hostile-Pal AnimBP.** Spawn Direhound + Stoneclad + Vinewraith. Each must use đúng AnimBP per species (não T-pose).

📸 Screenshot 2: PIE log `a.AnimInstance.Async 2` không warning.

---

## 3. AssetManager async + Game Feature plugin (W27-28)

Goal: Async load Pal definition + Game Feature plugin activates without hitch.

- [ ] **Plugin status.** Server: `Paldark.GameFeature.Status` → `plugins=[PaldarkRaidContent: Activated]`. Nếu `Unloaded` → designer chưa enable plugin (Edit → Plugins).
- [ ] **Warm-up cache.** Server: `Paldark.Pal.WarmAll` → log `warming N PalDefinition assets, queued, succeeded` (N=10 với Q3 roster đầy đủ).
- [ ] **Coalesce check.** Server: `Paldark.Pal.WarmAll` ngay lập tức 2 lần. Log second call: `coalesced, all assets already loaded` (FStreamableManager lifecycle in-flight handle).
- [ ] **Spawn via subsystem.** Server: `Paldark.Pal.Spawn Direhound` (subsystem path) → 1 Direhound spawn, async load if not pre-warmed.
- [ ] **No hitch on first map load.** Open Profiler (stat unit) on `Map_RungHong` load — frame time spike < 50ms (assuming pre-warm was done). If > 100ms, designer needs Bundle config check.

📸 Screenshot 3: Server log với `WarmAll succeeded` + `stat unit` overlay.

---

## 4. 10-Pal roster + boss FSM (W29-30)

Goal: 5 hostile + 2 companion (Q3) + 3 carry Q2 species spawn + ability fire + boss phase transitions.

- [ ] **Spawn 1 of each Q3 hostile species.**
  - `Paldark.Pal.SpawnTestHostile Stoneclad 800` → 1 Stoneclad spawn 8m forward. Activity=Patrol/Stalk. Ability=Charge ready.
  - `Paldark.Pal.SpawnTestHostile Vinewraith 1000` → 1 Vinewraith spawn 10m. Ability=VineWhip.
  - `Paldark.Pal.SpawnTestHostile Boltmane 1200` → 1 Boltmane (boss) spawn 12m. Phase 0 activity ready.
- [ ] **Spawn 1 of each Q3 companion species.**
  - `Paldark.Pal.SpawnTestCompanion Foxparks` → 1 Foxparks spawn close. Activity=Follow.
  - `Paldark.Pal.SpawnTestCompanion Tombat` → 1 Tombat spawn close.
- [ ] **Ability fire — Stoneclad.** Walk towards Stoneclad → switch to Combat. Server log `UPaldarkGameplayAbility_Charge — target=PaldarkCharacter_0 damage=20`. Stoneclad rams into player.
- [ ] **Ability fire — Vinewraith.** Server log `UPaldarkGameplayAbility_VineWhip — target=... damage=15`. (Slow GE on hit defer Q4.)
- [ ] **Ability fire — Boltmane.** Server log `UPaldarkGameplayAbility_ChainLightning — target=... damage=30 chain_hops=3` (multi-hit chain). 
- [ ] **Boss FSM phase 1.** Damage Boltmane to 60% HP. Server log `UPaldarkActivity_BossFSM — phase_transition 0→1 health_pct=0.6`. Activity tag `Paldark.Pal.BossPhase.1` flag flips.
- [ ] **Boss FSM phase 2.** Damage Boltmane to 30% HP. Log phase 1→2 transition. Aggressive mode kicks in.
- [ ] **Foxparks/Tombat companion squad.** `Paldark.Squad.Command Follow` → 2 companion Pal route to player.
- [ ] **Verify roster size.** Server: `Paldark.Pal.DumpRoster` (or count spawned actor) → 5 Q3 hostile + 2 Q3 companion + 3 Q2 carry = 10 species visible in PIE.

📸 Screenshot 4: PIE viewport với boss Boltmane phase 2 + 2 companion + 4 player.

---

## 5. Map 2 "Rừng Hỏng" exploration (W31-32)

Goal: POI discovery + SpawnPoint links + weather/biome tag visible.

- [ ] **Load Map 2.** Server: `ServerTravel /Game/Paldark/Maps/Map_RungHong` (or reload PIE with `Map_RungHong` as default).
- [ ] **POI dump.** Server: `Paldark.Map.DumpPOIs` → `POIs: 4-6 entries=[...]`. Verify count match placement.
- [ ] **Spawn points dump.** Server: `Paldark.Map.DumpSpawnPoints` → `SpawnPoints: ≥4 entries=[...]`. Each entry should have `LinkedPOITag` matching a POI.
- [ ] **Map definition.** Server: `Paldark.Map.Definition` → log `MapDef=DA_MapDef_RungHong WeatherTag=Paldark.Map.Weather.Fog BiomeTag=Paldark.Map.Biome.Forest bIsIndoor=false`.
- [ ] **POI-driven spawn.** Walk near a POI → spawn point fires (per designer config) → Direhound/Razorbird spawn.
- [ ] **POI BeginPlay race.** Server log on map load: `UPaldarkPOISubsystem::Initialize — sweeping for existing POIs, found N` + `OnWorldBeginPlay — retrying late-registered POIs, found M`. No warning về unregistered POI.

📸 Screenshot 5: PIE on Map_RungHong với fog + forest visible + Pal spawned at POI.

---

## 6. Map 3 "Cơ Xưởng PalCorp" + loot drop (W33-34)

Goal: Indoor map + tech-themed loot bag from Pal kill + flatten nested inventory.

- [ ] **Load Map 3.** Server: `ServerTravel /Game/Paldark/Maps/Map_PalCorp`.
- [ ] **Indoor flag.** Server: `Paldark.Map.Definition` → `bIsIndoor=true`. (Razorbird spawn should be suppressed if spawner respects flag — verify if designer wired in `APaldarkPalSpawner::OnBeginPlay`.)
- [ ] **Spawn Direhound for loot.** Server: `Paldark.Pal.SpawnTestHostile Direhound 500`. Kill it (LMB hitscan).
- [ ] **Loot bag spawn.** Server log `UPaldarkLootDropComponent — async_load DA_LootTable_Common, succeeded` + `APaldarkLootBag::InitializeContents — entries=N`. Bag spawns at Direhound's death transform.
- [ ] **Pickup.** Walk close to bag (3m) → interact prompt → `E` → items transfer to inventory. Server log `APaldarkLootBag::OnPickupConfirmed — transferred=K items`. Bag destroys.
- [ ] **5-min auto-cleanup.** Spawn another bag, leave it. After 5 minutes (or `Paldark.Loot.CleanupAll` if available) → bag despawns automatically.
- [ ] **Boss loot.** Kill Boltmane (use `Paldark.Gas.Damage <label> 9999` if HP is high). Loot bag should reference `DA_LootTable_Rare` → contains 1× backpack T2/T3 (per designer drop chance).

📸 Screenshot 6: PIE on Map_PalCorp với loot bag visible at Pal corpse.

---

## 7. Tame minigame (W35-36)

Goal: Throw Pal Sphere → BeginTameAttempt formula → success/fail.

- [ ] **Get a Pal Sphere.** Server: `Paldark.Inventory.GiveAll` (gives 1 of every item incl. 3 PalSphere) OR designer pre-add via `Paldark.Inventory.Add DA_Item_PalSphere_T1`.
- [ ] **Set up target.** `Paldark.Pal.SpawnTestHostile Direhound 500`. Then `Paldark.Tame.SetStun Direhound_0 0.5` (stun 50%) + `Paldark.Gas.Damage Direhound_0 70` (HP 30%).
- [ ] **Throw via GA.** Hold IA_UsePalSphere input (designer bind in IMC_Default — defer). For now use `Paldark.Tame.Force Direhound_0` to bypass formula.
- [ ] **Success path.** Roll X < P → server log `BeginTameAttempt rolled=0.20 P=0.45 result=Success`. Pal destroys, roster entry appends. `Paldark.Tame.DumpRoster` → 1 entry.
- [ ] **Fail path.** Re-spawn Direhound, throw without stun → P low → likely Fail. Stun drains 30%. Pal stays.
- [ ] **Boss capture (sub-1%).** Spawn Boltmane + stun + low HP + T3 sphere. Roll P should be < 0.01. Designer dùng `Paldark.Tame.Force` cho video shot.
- [ ] **Bond level.** Add bond XP: `Paldark.Pal.Bond.AddXP Direhound_0 100` → bond level 0→1.

📸 Screenshot 7: PIE log với `BeginTameAttempt Result=Success` + roster dump.

---

## 8. Inventory full (W37-38)

Goal: 30 items + backpack tier swap + container nested + drop-on-death.

- [ ] **Give all items.** Server: `Paldark.Inventory.GiveAll` → log `registered=N granted=K weight=A/B kg`. Verify weight bar (HUD or log).
- [ ] **Equip backpack T1.** Server: `Paldark.Inventory.EquipBackpack T1` → log `MaxWeightKg 30→40 kg`. Verify cap.
- [ ] **Equip backpack T2.** `Paldark.Inventory.EquipBackpack T2` → 30→55 kg.
- [ ] **Equip backpack T3.** `Paldark.Inventory.EquipBackpack T3` → 30→75 kg.
- [ ] **Clear backpack.** `Paldark.Inventory.EquipBackpack clear` → 30 kg base. Log warning over-encumbered if still > 30.
- [ ] **Dump composite.** `Paldark.Inventory.DumpComposite` → top-level entries + nested `InnerEntries` (empty unless container placed).
- [ ] **(Optional) Add container.** Server: `Paldark.Inventory.Add DA_Item_LootCrate` + designer wires nested content. `DumpComposite` shows nested entries.
- [ ] **Test death drop.** `Paldark.Inventory.TestDeathDrop` → loot bag spawns at player feet, inventory empties. Walk + pickup → items return.
- [ ] **Actual death drop.** `Paldark.Gas.Damage <player_label> 9999` (or shoot self with `BP_PaldarkDummyTarget` reflect mode) → `SpawnDeathLootBagFromInventory` fires BEFORE `MatchSub->RecordDeath`. Bag at death transform, inventory cleared. Match phase routes KIA outcome (Q2 carry).

📸 Screenshot 8: PIE log với 4 EquipBackpack lines (T1/T2/T3/clear) + 1 TestDeathDrop bag spawn.

---

## 9. Carry-from-Q2 smoke tests (collapsed)

> Re-run Q2 § 1-8 abbreviated. Verify Q3 changes don't regress Q2 features.

- [ ] **Squad ping** (Q2 § 5): T to ping, ping marker visible across 4 client.
- [ ] **Extraction beacon** (Q2 § 6): Walk lên beacon → 5s ramp → extract. Match Ended.
- [ ] **Lag compensation** (Q2 § 7): 80ms ping → server log `RewindToServerTime delta=80ms rewound=Yes`.
- [ ] **Dedicated server** (Q2 § 8, optional): `PaldarkServer.exe` + 4 client → match runs end-to-end.

---

## 10. Bug bash (1 ngày — optional cho Q3)

Đọc [`Q3-Bug-Bash.md`](Q3-Bug-Bash.md), chọn ≤ 5 issue ưu tiên cao (`P0`/`P1`) đóng trong ngày. Issue còn lại defer sang Q4 task list (W40+ hub town).

---

## 11. Sign-off

- [ ] ✅ Tất cả 9 section trên (Connect / AnimBP / Async / 10-Pal / Map 2 / Map 3 / Tame / Inventory / Q2-carry) tick xong.
- [ ] ✅ 8+ screenshot saved vào `Documents/Devlog/Q3-Playtest-Screens/` (folder tự tạo nếu chưa có).
- [ ] ✅ Video 90s record (OBS hoặc UE Editor record button, 4-quadrant split-screen nếu support) → upload → link trong `Documents/Devlog/Q3-Playtest-Video.md`.
- [ ] ✅ [`Q3-Bug-Bash.md`](Q3-Bug-Bash.md) review, mark `P0`/`P1` đã fix.
- [ ] ✅ Commit [`Documents/Devlog/Q3-Milestone-Build.md`](Q3-Milestone-Build.md) cập nhật `Pillar coverage actual %` với số sau playtest thực.

**Q3 milestone đóng** khi 5 ✅ trên cả tick. Q4 bắt đầu W40 — Hub Town (8-player shard + Pal Stable + Marketplace UI + Briefing Room) (roadmap § Tuần 40-41).

---

## 12. References

- [`PaldarkLab/README.md`](../../PaldarkLab/README.md) — full feature authoring + test loop (~2,900 dòng).
- [`Documents/Devlog/Q3-Milestone-Build.md`](Q3-Milestone-Build.md) — Q3 status summary, pillar coverage, gap analysis.
- [`Documents/Devlog/Q3-Bug-Bash.md`](Q3-Bug-Bash.md) — 20-issue priority list cho 1-day bug bash.
- [`Documents/Devlog/Q2-Playtest-Checklist.md`](Q2-Playtest-Checklist.md) — Q2 baseline checklist (this Q3 file inherits + extends).
- [`Documents/Devlog/Q1-Playtest-Checklist.md`](Q1-Playtest-Checklist.md) — Q1 baseline.
- [`Documents/PALDARK/03-Roadmap_1_Year.md`](../PALDARK/03-Roadmap_1_Year.md) § Tuần 39 + Q3 outcome.
- Per-week build logs Q2: [W14-15](W14-15-Build.md), [W16-17](W16-17-Build.md), [W18-19](W18-19-Build.md), [W20-21](W20-21-Build.md), [W22-23](W22-23-Build.md), [W24-25](W24-25-Build.md).
- Q3 PR detail (Devin commit notes embedded trong each PR description on GitHub): [#27](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/27), [#28](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/28), [#29](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/29), [#30](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/30), [#31](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/31), [#32](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/32), [#33](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/33).
