# PALDARK — Q1 Playtest Checklist (Tuần 13)

> **Goal:** 1 file walkthrough chạy 1 lần PIE trong UE 5.4 Editor, chạm hết feature W1-12. Designer in trang này ra giấy → tick từng box → screenshot → ship video 30s.
>
> **Prerequisites:**
> - UE 5.4 Editor (chưa có Toolchain trên VM — phải chạy trên máy designer / dev).
> - `git pull` về `main` (commit `d301930` trở lên — sau PR #16).
> - `Documents/Devlog/Q1-Milestone-Build.md` đọc qua một lần.

---

## 0. One-time setup (≈ 30 phút — chỉ làm lần đầu)

> Tất cả bước Blueprint authoring đã có chi tiết trong [`PaldarkLab/README.md`](../../PaldarkLab/README.md). Section này chỉ tóm ngắn để designer biết phải xếp đúng thứ tự.

- [ ] **GPF + Compile.** Open `PaldarkLab/PaldarkLab.uproject` → "Generate Project Files" → mở `.sln` → Build `PaldarkLab Development Editor`. Editor mở phải thấy `LogPaldark`, `LogPaldarkPal`, `LogPaldarkInventory`, `LogPaldarkNet`, `LogPaldarkGAS` xuất hiện trong Output Log.
- [ ] **Map `Raid_Sandbox`.** Content Browser → `/Game/Paldark/Maps/` → New → "Basic" → đặt tên `Raid_Sandbox`. Drop 1 `Floor` (5000×5000), 1 `NavMeshBoundsVolume` (bao toàn floor), 1 `PlayerStart` (z=200). Build navmesh (`P` để hiện debug).
- [ ] **Experience asset `PX_RaidSandbox`.** Content Browser → `/Game/Paldark/Experience/` → New → Data Asset → `PaldarkExperienceDefinition` → đặt tên `PX_RaidSandbox`. Wire:
  - `DefaultPawnData` → `PD_RaidPlayer` (xem bước sau).
  - `PlayerControllerClass` → `APaldarkPlayerController`.
  - `PlayerStateClass` → `APaldarkPlayerState`.
  - `IntrinsicTags` → `Paldark.Experience.RaidSandbox`.
- [ ] **PawnData `PD_RaidPlayer`.** Content Browser → `/Game/Paldark/PawnData/` → New → `PaldarkPawnData`. Wire:
  - `PawnClass` → `BP_PaldarkCharacter` (BP subclass `APaldarkCharacter` + mesh Mannequin Quinn).
  - `DefaultMappingContexts` → 1 row: `IMC_Default`, priority 0, `bRegisterWithSettings=true`.
  - `InputConfig` → `InputConfig_Default`.
  - `GrantedAbilities` → 2 row: `BP_GA_Sprint`, `BP_GA_HitscanFire`.
  - `StartupEffects` → (empty cho Q1).
  - `DefaultPalCompanions` → 1 row: `BP_PaldarkPalCharacter` (BP subclass `APaldarkPalCharacter` + mesh Mannequin Manny), offset `{-300, 0, 0}`, `FollowDistanceOverride=500`.
- [ ] **InputConfig `InputConfig_Default`.** Content Browser → `/Game/Paldark/Input/` → New → `PaldarkInputConfig`. Wire:
  - `NativeInputActions[]` → 3 row: `(Paldark.InputTag.Move, IA_Move)`, `(Paldark.InputTag.Look, IA_Look)`, `(Paldark.InputTag.Jump, IA_Jump)`.
  - `AbilityInputActions[]` → 2 row: `(Paldark.InputTag.Sprint, IA_Sprint)`, `(Paldark.InputTag.Fire, IA_Fire)`.
- [ ] **IMC `IMC_Default`.** Content Browser → `/Game/Paldark/Input/` → New → Input Mapping Context. Bind:
  - `IA_Move` → WASD (2D Axis modifier).
  - `IA_Look` → Mouse XY (2D Axis modifier, negate Y).
  - `IA_Jump` → Space (Digital Bool).
  - `IA_Sprint` → LeftShift (Digital Bool).
  - `IA_Fire` → LeftMouseButton + GamepadRightTrigger (Digital Bool).
- [ ] **GAS asset bundle (8 BP):**
  - `GE_Sprint_Cost` (Instant? No — Duration `Infinite`, Period `0.1s`, Modifiers: `Stamina` Add `-2`). § PaldarkLab/README.md "Sprint test loop".
  - `GE_Sprint_MoveSpeed` (Duration `Infinite`, Modifiers: `MoveSpeed` Add `+300`).
  - `BP_GA_Sprint` (subclass `UPaldarkGameplayAbility_Sprint`, set `SprintCost=GE_Sprint_Cost`, `SprintMoveSpeed=GE_Sprint_MoveSpeed`, `SprintingStateTag=Paldark.State.Sprinting`).
  - `GE_Damage_Standard` (Instant, Executions = `PaldarkDamageExecutionCalculation`).
  - `GE_DummyInitAttributes` (Instant, Modifiers: Health Override 100, MaxHealth Override 100, Armor Override 0).
  - `BP_GA_HitscanFire` (subclass `UPaldarkGameplayAbility_HitscanFire`, set `DamageEffectClass=GE_Damage_Standard`, `FireRange=10000`, `BaseDamage=25`, `HeadshotMultiplier=2.0`, `HeadBoneName=head`).
  - `BP_DummyTarget_Default` (subclass `APaldarkDummyTarget`, set `InitAttributesEffect=GE_DummyInitAttributes`, `DestroyDelaySeconds=1.5`, Mesh = Mannequin Manny).
- [ ] **Inventory DataAsset (5 file):** `/Game/Paldark/Items/DA_Item_*` per bảng trong [`PaldarkLab/README.md`](../../PaldarkLab/README.md#L408-L416) (Pistol / Ammo_9mm / Bandage / Pal_Sphere / Energy_Drink).
- [ ] **Player BP `BP_PaldarkCharacter`.** Subclass `APaldarkCharacter` + Mannequin Quinn mesh + ABP UE5 mannequin default. Drop component `UPaldarkPlayerInventoryComponent` lên actor, set `MaxWeightKg=30`.
- [ ] **Pal BP `BP_PaldarkPalCharacter`.** Subclass `APaldarkPalCharacter` + Mannequin Manny mesh + ABP UE5 mannequin default.
- [ ] **World Settings.** Map `Raid_Sandbox` → World Settings → set `GameMode Override = APaldarkGameModeBase` + `DefaultPawn Class = BP_PaldarkCharacter`.

✅ Sau bước này repo Content folder phải có: `PX_RaidSandbox`, `PD_RaidPlayer`, `InputConfig_Default`, `IMC_Default`, 5 `IA_*`, 8 GAS BP, 5 `DA_Item_*`, 2 BP Character, 1 map `Raid_Sandbox.umap`.

---

## 1. Drop & basic locomotion (P02 + P03 + P04 + P12 + P14)

Goal: vào map từ menu + di chuyển bằng WASD + nhìn chuột + nhảy Space.

```
open Raid_Sandbox?Experience=PaldarkExperience.PX_RaidSandbox
```

- [ ] Editor PIE Listen Server (1 client) mở `Raid_Sandbox` không crash.
- [ ] `Paldark.Experience.Current` → log `experience=PaldarkExperience:PX_RaidSandbox`.
- [ ] `Paldark.Experience.Hello` → log custom hello message từ `PX_RaidSandbox.HelloWorldMessage`.
- [ ] `Paldark.Experience.ListExtensions` → log class overrides (PlayerControllerClass / PlayerStateClass) + IntrinsicTags `Paldark.Experience.RaidSandbox`.
- [ ] `Paldark.Input.ListBindings` → log 1 controller + 1 IMC (`IMC_Default`, priority 0) + 3 NativeInputAction (Move/Look/Jump) + 2 AbilityInputAction (Sprint/Fire).
- [ ] **WASD** → player di chuyển (forward/back/strafe).
- [ ] **Chuột** → camera quay (spring arm + camera composition).
- [ ] **Space** → player nhảy.

📸 Screenshot 1: PIE viewport với player ở giữa map, output log hiển thị 4 lệnh trên.

---

## 2. Pal companion follow + Activity FSM (P03 + P09 + P12)

Goal: spawn 1 Pal đi theo player + ping FSM chuyển Investigate.

- [ ] `Paldark.Pal.SpawnTestCompanion` (no arg = index 0 = `BP_PaldarkPalCharacter`) → 1 Pal spawn cách player `-300 cm`, log `pal spawned at ... followed=BP_PaldarkCharacter_C_0 distance≈300cm`.
- [ ] Pal **đứng yên** khi player đứng yên (distance ≤ 500cm, Activity=Idle).
- [ ] Player **chạy lên trước 10m** → Pal switch Follow, đuổi theo (`AddMovementInput` driven), giữ khoảng cách ≈ 500cm khi player dừng.
- [ ] `Paldark.Pal.CurrentActivity` → log `pal=BP_PaldarkPalCharacter_C_0 activity=UPaldarkActivity_Follow tag=Paldark.Pal.Activity.Follow candidates=3`.
- [ ] `Paldark.Pal.Ping` (no arg = 6m trước player) → Pal switch Investigate (priority 30 > Follow 20), steer về ping location, magenta debug message hiện trên màn hình.
- [ ] `Paldark.Pal.CurrentActivity` → log `activity=UPaldarkActivity_Investigate`.
- [ ] **Đợi 1.5s** sau khi Pal tới gần ping point (arrival radius 150cm) → FSM về Follow/Idle.
- [ ] `Paldark.Pal.SetActivity Idle` → force Idle, Pal đứng im không leash.
- [ ] `Paldark.Pal.SetActivity Follow` → leash bật lại.
- [ ] **Hysteresis check.** Đứng cách Pal đúng 500cm → `Paldark.Pal.CurrentActivity` chạy 3 lần liên tiếp → phải ổn định 1 state (Idle hoặc Follow), không chatter.

📸 Screenshot 2: PIE viewport với 1 Pal đang follow player từ phía sau + on-screen ping magenta visible.

---

## 3. Sprint stamina drain (P04 + P08)

Goal: LShift → MoveSpeed bump 300 + Stamina drain 2/0.1s.

- [ ] `Paldark.Gas.DumpAttributes` → log `BP_PaldarkCharacter_C_0 — Health=100 MaxHealth=100 Stamina=100 MaxStamina=100 MoveSpeed=600 Armor=0`.
- [ ] **Hold LShift** + WASD forward → player chạy nhanh hơn rõ rệt (300→900 cm/s).
- [ ] `Paldark.Gas.DumpAttributes` (giữ LShift) → `Stamina` giảm liên tục (đợi 2s → còn ≈60).
- [ ] **Release LShift** → MoveSpeed về 600, Stamina dừng giảm (regen chưa wire ở Q1, đó là known issue K-13).
- [ ] Hold LShift đến khi Stamina = 0 → player **vẫn chạy nhanh** (K-13: sprint không cancel khi stamina cạn — Q2 sẽ wire `IsExhausted` tag).

📸 Screenshot 3: PIE log hiển thị Stamina giảm dần 100 → 80 → 60 → 40 khi hold LShift.

---

## 4. Hitscan fire + damage chain (P08 + P12)

Goal: bắn dummy chết theo 4 phát body shot hoặc 2 phát headshot.

- [ ] `Paldark.Combat.SpawnDummy 500` → 1 dummy spawn cách player 5m, mesh Manny, facing player.
- [ ] `Paldark.Gas.DumpAttributes` → 2 GAS actor: player + `BP_DummyTarget_Default_C_0 — Health=100 MaxHealth=100 Armor=0`.
- [ ] **Aim torso + LMB** → dummy Health giảm. Output Log đầy đủ 3 dòng:
  - `UPaldarkGameplayAbility_HitscanFire — hit=BP_DummyTarget_Default_C_0 bone=spine_01 headshot=0 distance=502.3 target_asc=...`
  - `UPaldarkDamageExecutionCalculation — base=25.00 headshot=1.00 armor=0.00 mitigation=1.000 final=25.00`
  - `UPaldarkAttributeSet::PostGameplayEffectExecute — IncomingDamage=25.00 -> Health 100.00 → 75.00`
- [ ] Bắn body shot **3 lần nữa** → Health 25 → 0 → log `UPaldarkAttributeSet — Health hit 0, broadcasting OnHealthZeroed.` → log `APaldarkDummyTarget::HandleHealthZeroed — instigator=...`.
- [ ] Sau **≈1.5s**, dummy `Destroy()` (visible despawn).
- [ ] `Paldark.Combat.SpawnDummy 500` lần nữa, đổi sang **aim head + LMB** → log `headshot=1 final=50.00`. 2 phát đủ kill.
- [ ] `Paldark.Combat.Fire` (no aim) → tương đương click chuột, dùng để test damage formula khi không tiện rebind.
- [ ] **Armor test.** Author 1 dummy variant `BP_DummyTarget_Tank` với `GE_DummyInitAttributes` mod `Armor Override 100`. Spawn → bắn body shot → `final ≈ 12.5` (25 × 1.0 × 100/(100+100)). Kill cần 8 phát.

📸 Screenshot 4: PIE viewport với dummy ragdoll fall sau headshot kill + log hiển thị 3 dòng damage trace.

---

## 5. Inventory fragment + weight cap (P11)

Goal: add 5 item + verify stack splitting + weight cap advisory + remove + drop.

- [ ] `Paldark.Inventory.List` → `entries=0 weight=0.00/30.00 kg over=no`.
- [ ] `Paldark.Inventory.Add Pistol` → log `requested=1 added=1 item=DA_Item_Pistol`.
- [ ] `Paldark.Inventory.Add Ammo_9mm 200` → log `requested=200 added=200`. List shows 4 ammo entries (60/60/60/20) + 1 Pistol entry.
- [ ] `Paldark.Inventory.Add Bandage 5` + `Paldark.Inventory.Add Pal_Sphere 2` + `Paldark.Inventory.Add Energy_Drink 3` → list shows 5 item types.
- [ ] `Paldark.Inventory.List` → weight ≈ `1.2 (pistol) + 0.012*200 (ammo) + 0.05*5 (bandage) + 0.3*2 (sphere) + 0.4*3 (drink) = 5.95 kg` over `no`.
- [ ] `Paldark.Inventory.Remove Ammo_9mm 75` → 4 ammo row → 3 row (60/60/5) hoặc tương đương (backwards iterate). Weight ≈ 4.45 kg.
- [ ] **Weight cap advisory.** Edit `BP_PaldarkCharacter` → `UPaldarkPlayerInventoryComponent` instance → `MaxWeightKg = 2`. Compile + run lại PIE → `Paldark.Inventory.Add Ammo_9mm 500` → log `added=500`, list `weight≈6 kg over=yes`. **Không reject** (K-03: advisory only). Restore `MaxWeightKg = 30`.
- [ ] `Paldark.Inventory.Drop` → list `entries=0 weight=0.00/30.00 kg`. (K-04: chưa spawn world pickup — Q2.)
- [ ] **Short form vs full tag.** `Paldark.Inventory.Add Paldark.Item.Pistol` (full) tương đương `Paldark.Inventory.Add Pistol` (short).

📸 Screenshot 5: PIE log hiển thị `Paldark.Inventory.List` với 5 item type sau bước add full.

---

## 6. Replication smoke test (2-player PIE)

Goal: server-side mutate `Entries` → client thấy entry qua `OnRep_Entries`.

- [ ] Editor → PIE settings → **Number of Players = 2** + Net Mode = Play as Listen Server.
- [ ] Run PIE → 2 viewport: server (window 1) + client (window 2).
- [ ] **Server console** (` ~ ` trong window 1): `Paldark.Inventory.Add Pistol` → log on server.
- [ ] **Client console** (window 2): `Paldark.Inventory.List` → entry `DA_Item_Pistol StackCount=1` phải hiện ngay (≤1 frame), confirming `DOREPLIFETIME(Entries)` + `OnRep_Entries` broadcast.
- [ ] **Authority check.** Client console: `Paldark.Inventory.Add Bandage` → log warning `local player has no authority — server-side only`. Server không nhận item.
- [ ] **Pal replicate.** Server: `Paldark.Pal.SpawnTestCompanion` → client viewport phải thấy Pal spawn + follow player (replicated movement chạy qua `SetReplicateMovement(true)`).

📸 Screenshot 6: 2 PIE viewport side-by-side với client log `Paldark.Inventory.List` ngay sau server `Add Pistol`.

---

## 7. Bug bash (1 ngày — optional cho Q1)

Đọc [`Q1-Bug-Bash.md`](Q1-Bug-Bash.md), chọn ≤ 3 issue ưu tiên cao (`P0`/`P1`) đóng trong ngày. Issue còn lại defer sang Q2 task list.

---

## 8. Sign-off

- [ ] ✅ Tất cả 6 section trên (Drop / Pal / Sprint / Fire / Inventory / Replication) tick xong.
- [ ] ✅ 6 screenshot saved vào `Documents/Devlog/Q1-Playtest-Screens/` (folder tự tạo nếu chưa có).
- [ ] ✅ Video 30s record (OBS hoặc UE Editor record button) → upload → link trong `Documents/Devlog/Q1-Playtest-Video.md`.
- [ ] ✅ `Q1-Bug-Bash.md` review, mark `P0`/`P1` đã fix.
- [ ] ✅ Commit `Documents/Devlog/Q1-Milestone-Build.md` cập nhật `Pillar coverage actual %` với số sau playtest thực.

**Q1 milestone đóng** khi 4 ✅ trên cả tick. Q2 bắt đầu W14 — Lyra ALS port (Path A § 4 của Milestone Build).

---

## 9. References

- [`PaldarkLab/README.md`](../../PaldarkLab/README.md) — full feature authoring + test loop.
- [`Documents/Devlog/Q1-Milestone-Build.md`](Q1-Milestone-Build.md) — Q1 status summary, pillar coverage, gap analysis.
- [`Documents/Devlog/Q1-Bug-Bash.md`](Q1-Bug-Bash.md) — 15-issue priority list.
- [`Documents/PALDARK/03-Roadmap_1_Year.md`](../PALDARK/03-Roadmap_1_Year.md) § Tuần 13.
