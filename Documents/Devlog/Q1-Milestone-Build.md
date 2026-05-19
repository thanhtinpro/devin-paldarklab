# PALDARK — Q1 Milestone Build (Tuần 13)

> **Source:** [`Documents/PALDARK/03-Roadmap_1_Year.md`](../PALDARK/03-Roadmap_1_Year.md) § Tuần 13.
>
> **Roadmap deliverable:** "Internal playtest: 1 player, 1 Pal, 1 map, 1 weapon, inventory, sprint, shoot dummy. Bug bash 1 ngày."
>
> **Q1 Outcome (from roadmap):** "Có **gameplay loop nguyên thuỷ** — drop, di chuyển, có Pal follow, có súng bắn được, có inventory."

This devlog freezes the Q1 milestone: what was shipped W1-12 in C++, how it maps to the roadmap, which pillars hit their Q1 % target, and what the user must do in UE Editor (the VM has no UE 5.4 compiler) to actually validate the playtest loop.

---

## 1. PR ledger (W1 → W12)

| PR | Week | Title | Status | Branch |
|----|------|-------|--------|--------|
| [#7](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/7)  | W1 day 1-4   | PaldarkLab — P01 capstone scaffold (3 module / 3 target / 5 log / console cmd / actor / subsystem)            | ✅ merged | `devin/.../w1-paldarklab-skeleton` |
| [#8](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/8)  | W1 day 5     | GitHub Actions CI (PaldarkLab structural validation + markdown link check)                                    | ✅ merged | `devin/.../w1-day5-github-actions` |
| [#9](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/9)  | W1 day 6-7   | Paldark Experience hello-world (vendor-neutral Lyra pattern)                                                  | ✅ merged | `devin/.../w1-day6-7-experience` |
| [#10](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/10) | W1 day 8-10  | Paldark Experience action sets + player framework (Raid_Sandbox)                                              | ✅ merged | `devin/.../w1-day8-10-raid-sandbox-experience` |
| [#11](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/11) | W1 day 11-14 | Paldark Enhanced Input + tag-keyed Input Config (mapping contexts via PawnData)                               | ✅ merged | `devin/.../w1-day11-14-enhanced-input` |
| [#12](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/12) | W3-4         | Paldark Player & Pal Pawn skeleton (12 + 8 component slots + follow locomotion)                               | ✅ merged | `devin/.../w3-4-player-pal-pawn` |
| [#13](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/13) | W5-6         | Paldark Activity FSM skeleton (Idle / Follow / Investigate + ping)                                            | ✅ merged | `devin/.../w5-6-activity-fsm` |
| [#14](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/14) | W7-8         | Paldark GAS basic (AttributeSet + ASC + GA_Sprint)                                                            | ✅ merged | `devin/.../w7-8-gas-attributeset` |
| [#15](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/15) | W9-10        | Paldark Damage chain (GE + DamageExecutionCalculation + HitscanFire + DummyTarget)                            | ✅ merged | `devin/.../w9-10-damage-chain` |
| [#16](https://github.com/SlimeVRX/Soliz-Devin-Palworld/pull/16) | W11-12       | Paldark Inventory fragment (Item Fragment + ItemDefinition + InventoryComponent)                              | ✅ merged | `devin/.../w11-12-inventory-fragment` |

10 PR, ~5,500 dòng C++ + ini + python validator + README on `main`. Toàn bộ commit qua CI structural validation (`scripts/ci/validate_paldarklab.py`).

---

## 2. Feature map vs roadmap outcome

Roadmap § Tuần 13 ghi rõ Q1 outcome cần 5 thứ — bảng dưới mapping từng thứ → PR / file C++ / lệnh test:

| Q1 Outcome | C++ delivery | Test entrypoint | PR |
|------------|--------------|-----------------|----|
| **Drop** (vào map từ menu) | `APaldarkGameModeBase` parse `?Experience=PaldarkExperience.PX_RaidSandbox` + async-load via `UPaldarkAssetManager` → `APaldarkPlayerController::OnPossess` chạy `BeginPlay` chain | `open Raid_Sandbox?Experience=PaldarkExperience.PX_RaidSandbox` | #9 #10 |
| **Di chuyển** (player) | `APaldarkCharacter` + Enhanced Input + `UPaldarkInputConfig` (tag-keyed) + IMC qua `PawnData.DefaultMappingContexts` | WASD + chuột | #11 #12 |
| **Pal follow** | `APaldarkPalCharacter` (8 component slots) + `UPaldarkPalLocomotionComponent` + `UPaldarkPalActivityComponent` (Activity FSM Idle/Follow/Investigate) | `Paldark.Pal.SpawnTestCompanion` + `Paldark.Pal.Ping` | #12 #13 |
| **Súng bắn** (1 weapon) | `UPaldarkAttributeSet` + `UPaldarkAbilitySystemComponent` + `UPaldarkGameplayAbility_HitscanFire` + `UPaldarkDamageExecutionCalculation` + `APaldarkDummyTarget` | `Paldark.Combat.SpawnDummy` → click chuột (`IA_Fire` → `Paldark.Ability.Fire`) hoặc `Paldark.Combat.Fire` | #14 #15 |
| **Sprint stamina** | `UPaldarkGameplayAbility_Sprint` (apply `GE_Sprint_Cost` periodic + `GE_Sprint_MoveSpeed` additive + `State.Sprinting` tag) | LShift + `Paldark.Gas.DumpAttributes` | #14 |
| **Inventory** (5 item, weight cap) | `UPaldarkItemFragment` (Stackable/Equipable/Weight) + `UPaldarkItemDefinition` + `UPaldarkPlayerInventoryComponent` (replicated Entries + DOREPLIFETIME + 2-phase stack split) | `Paldark.Inventory.Add Pistol` / `Add Ammo_9mm 200` / `List` / `Remove` / `Drop` | #16 |

> **5/5 outcome items đã ship ở C++ side.** Designer side cần author trong UE Editor: 1 GE_Sprint_Cost + 1 GE_Sprint_MoveSpeed (#14), 1 GE_Damage_Standard + 1 GE_DummyInitAttributes + 1 BP_GA_HitscanFire + 1 BP_DummyTarget_Default (#15), 5 DA_Item_* DataAsset (#16). Tất cả bước Blueprint đã có hướng dẫn trong [`PaldarkLab/README.md`](../../PaldarkLab/README.md) (§ "Sprint test loop", § "Damage chain authoring + test loop", § "Inventory fragment authoring + test loop").

---

## 3. Pillar coverage — actual vs target

Roadmap dự kiến Q1 đạt: P1 100% / P2 70% / P3 70% / P4 80% / P5 60% / P8 60% / P9 40% / P11 60% / P17 80%. Đối chiếu thực tế:

| Pillar | Target Q1 | Actual Q1 | Delta | Bằng chứng |
|--------|-----------|-----------|-------|------------|
| **P01** C++ & Build         | 100% | **100%** | 0    | 3 module + 3 target + 5 log category + Build.cs dep graph (PR #7) |
| **P02** Core Framework      | 70%  | **75%**  | +5   | GameMode/GameState/GameInstance + PaldarkAssetManager (UAssetManager) + 5 primary asset types registered. PlayerStart/Pause/HUD chưa wire (defer Q2). |
| **P03** Composition         | 70%  | **72%**  | +2   | 12 player slots + 8 Pal slots + 1 hot component (LocomotionComponent) implemented. Còn 19 slot rỗng chờ feature week. |
| **P04** Enhanced Input      | 80%  | **80%**  | 0    | `UPaldarkInputConfig` tag-keyed + IMC stack qua PawnData + 4 input tag (Move/Look/Jump/Sprint/Fire). Modifier/Trigger custom defer. |
| **P05** Animation           | 60%  | **15%**  | **-45** | ❌ Chưa có AnimBP — chỉ có `ACharacter` skeleton. Locomotion `AddMovementInput`-driven, không có Lyra ALS. Q2 catch-up bắt buộc trước khi 4-player playtest. |
| **P08** GAS                 | 60%  | **65%**  | +5   | AttributeSet (6 attr) + ASC subclass + GA_Sprint + GA_HitscanFire + DamageExecutionCalculation. Còn thiếu: AbilityTask custom, GameplayCue, cooldown UI hook. |
| **P09** AI                  | 40%  | **45%**  | +5   | Activity FSM (Idle/Follow/Investigate) + ping cmd + planar leash. BT/EQS chưa wire (defer W18+). |
| **P11** Inventory           | 60%  | **65%**  | +5   | Item Fragment composition + 3 concrete fragment + ItemDefinition + replicated InventoryComponent. UMG widget + pickup actor defer (W13+). |
| **P17** Lyra                | 80%  | **75%**  | -5   | Vendor-neutral Experience system + PawnData + ActionSet + AssetManager. Game Feature plugin chưa setup (defer Q3). |

**Tổng coverage Q1 ≈ 65%** (cộng đều 9 pillar). Roadmap target ≈ 70%. **Gap chính: P05 Animation -45 điểm.** Đây là risk lớn nhất phải đóng trong Q2 — không có AnimBP thì playtest "drop, di chuyển, Pal follow" không có visual feedback tin cậy. Đề xuất chen 1 PR W13.5 (giữa Q1 và Q2) port Lyra ALS minimal trước khi vào "Tuần 14-15 — 4-player dedicated test".

---

## 4. Gap analysis vs roadmap "5 thứ Q1 phải có"

> Roadmap § Tuần 13 yêu cầu: "Internal playtest: 1 player, 1 Pal, 1 map, 1 weapon, inventory, sprint, shoot dummy."

| # | Required | Shipped (C++) | Designer authoring still needed | Risk |
|---|----------|---------------|---------------------------------|------|
| 1 | 1 player    | ✅ APaldarkCharacter + spring arm + camera + Enhanced Input    | BP_PaldarkCharacter subclass (mesh + AnimBP) | High — chưa có AnimBP, mannequin sẽ T-pose. |
| 2 | 1 Pal       | ✅ APaldarkPalCharacter + Activity FSM + Locomotion + 8 slots  | BP_Fox_Companion subclass (mesh + AnimBP) | High — same as #1. |
| 3 | 1 map       | ✅ `Raid_Sandbox` level placeholder name + Experience `PX_RaidSandbox` | UE level author + navmesh | Medium — `Raid_Sandbox.umap` chưa được commit (binary asset, designer phải tạo). |
| 4 | 1 weapon    | ✅ GA_HitscanFire (C++ class) + DummyTarget actor              | BP_GA_HitscanFire (BP subclass) + GE_Damage_Standard (BP) + BP_DummyTarget_Default (BP) | Low — README §"Damage chain authoring" có 6 bước rõ. |
| 5 | Inventory   | ✅ UPaldarkPlayerInventoryComponent + 5 item tag + 4 console cmd | 5 DA_Item_* DataAsset (Pistol/Ammo_9mm/Bandage/Pal_Sphere/Energy_Drink) | Low — README §"Inventory fragment authoring" có bảng item full. |
| 6 | Sprint      | ✅ GA_Sprint + AttributeSet Stamina drain                       | BP_GA_Sprint + GE_Sprint_Cost + GE_Sprint_MoveSpeed (BP) | Low — README §"Sprint test loop" có 4 bước. |
| 7 | Shoot dummy | ✅ Toàn bộ damage chain end-to-end                              | BP_GA_HitscanFire + BP_DummyTarget_Default (BP) | Low — same as #4. |

**Verdict:** 7/7 yêu cầu có C++ backbone. Designer block lớn nhất là **AnimBP** (#1 #2) — không có thì playtest vẫn chạy được code path nhưng visual không tin cậy. Đề xuất:
- **Path A (sạch):** Đặt 1 PR W13.5 chen vào trước W14 để port Lyra ALS minimal (Idle/Walk/Run + StateMachine + Distance Matching). Risk: thêm 1-2 tuần delay.
- **Path B (nhanh):** Dùng UE5 mannequin default AnimBP cho Q1 milestone, ghi nhận "T-pose Pal" là known-issue, đi tiếp W14. Risk: playtest video nhìn xấu, nhưng đủ để verify code path.

Em đề xuất **Path B** cho Q1 milestone (giữ đúng deadline) + Path A là task đầu tiên của Q2 (trước W14 dedicated test).

---

## 5. Known issues & risks (frozen tại W13)

| ID | Issue | Severity | Mitigation | Defer to |
|----|-------|----------|------------|----------|
| K-01 | Không có AnimBP cho Player/Pal — mannequin T-pose | High | Path A/B § 4 | W13.5 hoặc W14 |
| K-02 | `Raid_Sandbox.umap` chưa commit (binary asset) | Medium | Designer tạo trong UE Editor, commit lfs | W13 designer task |
| K-03 | Weight cap **advisory** — `AddItem` không reject khi over capacity | Low | Movement penalty gate trong W13-14 / Q2 | Q2 |
| K-04 | Pickup actor chưa tồn tại — `Paldark.Inventory.Drop` chỉ clear array, không spawn world actor | Medium | `AItemPickupActor` + OnUse RPC | Q2 (W13-14 inventory follow-up) |
| K-05 | UMG inventory widget chưa tồn tại — chỉ test qua console | Medium | `WBP_InventoryGrid` + `OnInventoryChanged` listener | Q2 |
| K-06 | Encumbrance chưa gate locomotion — `IsOverEncumbered()` true nhưng player chạy bình thường | Low | `UPaldarkPlayerInventoryComponent::ApplyEncumbranceModifier` hook trên CharMovement | Q2 |
| K-07 | GA_HitscanFire dùng `LineTraceSingleByChannel` — không có lag compensation | High | Server-side rewind port từ [10] MP Shooter | W16-17 |
| K-08 | Pal Activity FSM dùng `BeginPlay` instantiation — không reload Activity từ savegame | Low | `Serialize` override + save current activity tag | Q3 |
| K-09 | Async asset load defer — InventoryComponent dùng `LoadSynchronous` | Low | `FStreamableManager::RequestAsyncLoad` qua AssetManager Bundle | Q3 (P14 polish) |
| K-10 | FastArraySerializer chưa wire — Entries replicate full payload mỗi mutate | Low | `FFastArraySerializer` subclass cho W21+ khi inventory > 50 entries | W21+ |
| K-11 | Game Feature plugin chưa setup — chưa modular gameplay | Low | Lyra-style Game Feature plugin "PaldarkInventory" / "PaldarkCombat" | Q3 |
| K-12 | `PaldarkLab.Pal.Ping` không clamp `Z` — ping ngoài navmesh khiến Pal stuck | Low | `UNavigationSystemV1::ProjectPointToNavigation` trong RequestInvestigate | W18+ |
| K-13 | Sprint không cancel khi Stamina = 0 — Stamina clamp ở `MaxStamina/0` nhưng GE_Sprint_Cost tiếp tục tick | Medium | Add `Paldark.State.IsExhausted` tag → cancel GA_Sprint khi tag apply | W14-15 |
| K-14 | Replication client→server one-way only — chưa có `Server_*` RPC trên InventoryComponent | Medium | Add `Server_AddItem` UFUNCTION với validation | Q2 |
| K-15 | Console cmd `Paldark.Inventory.Add` dùng path-based scan (`/Game/Paldark/Items`) — fail nếu designer đặt sai folder | Low | Validator extend kiểm folder path khớp ini | W14 |

> **Verdict:** Không có issue nào ở mức **Critical** block Q1 outcome. K-01 (AnimBP T-pose) là risk visual lớn nhất nhưng không block gameplay loop. K-07 (lag comp) explicitly là W16-17 scope, không phải Q1 expectation.

---

## 6. Files / module health

```
PaldarkLab/
├── PaldarkLab.uproject                  # UE 5.4, 3 plugin (EnhancedInput, GameplayAbilities, ModelingToolsEditorMode)
├── Config/
│   ├── DefaultEngine.ini                # log verbosity + class wiring (GameInstance/GameMode)
│   ├── DefaultGame.ini                  # 5 primary asset type (Experience/PawnData/ActionSet/InputConfig/Item)
│   ├── DefaultInput.ini                 # EnhancedPlayerInput + EnhancedInputComponent (P04)
│   └── DefaultGameplayTags.ini          # (auto-generated, native tags ship trong PaldarkGameplayTags)
├── Source/
│   ├── PaldarkLabCore/                  # PreDefault loading, log categories + native gameplay tags
│   │   ├── PaldarkLabCore.Build.cs
│   │   ├── Public/PaldarkLogCategories.h          # 5 log category
│   │   ├── Public/PaldarkGameplayTags.h           # 24 native tag (Input/Pawn/Experience/Pal.Activity/Equip/Item/InputTag/Hit/State/Ability/SetByCaller)
│   │   ├── Private/PaldarkGameplayTags.cpp
│   │   └── Private/PaldarkLabCore.cpp
│   ├── PaldarkLab/                      # Default loading, main game module
│   │   ├── PaldarkLab.Build.cs          # GameplayAbilities + GameplayTags + GameplayTasks + EnhancedInput
│   │   ├── PaldarkLab.cpp               # 8 console cmd register
│   │   ├── Public/
│   │   │   ├── Framework/               # AssetManager + GameMode + GameState + GameInstance
│   │   │   ├── Experience/              # ExperienceDefinition + ActionSet + PawnData
│   │   │   ├── Input/PaldarkInputConfig.h
│   │   │   ├── Player/                  # PlayerController + PlayerState + Character + 12 component slots
│   │   │   ├── Pal/                     # PalCharacter + 8 component slots + LocomotionComponent + ActivityComponent + 3 Activity subclass
│   │   │   ├── Gas/                     # AttributeSet + ASC + GameplayAbility base + Sprint + HitscanFire + DamageExecutionCalculation
│   │   │   ├── Combat/PaldarkDummyTarget.h
│   │   │   ├── Inventory/               # ItemFragment + 3 fragment + ItemDefinition
│   │   │   └── Subsystem/PaldarkLabWorldSubsystem.h
│   │   └── Private/                     # matching .cpp tree
│   └── PaldarkLabEditor/                # Editor-only placeholder
└── README.md                            # 516 lines, đầy đủ test loop cho mọi feature W1-12
```

**Module load order (PreDefault → Default → PostEngineInit):**
1. `PaldarkLabCore` (PreDefault) — log + native tags available trước Engine init.
2. `PaldarkLab` (Default) — sample actor + subsystem + console cmd register.
3. `PaldarkLabEditor` (PostEngineInit, Editor-only) — placeholder.

Validator kiểm 6 thứ ở mức Python (`scripts/ci/validate_paldarklab.py` — 1367 dòng):
- uproject + 3 module layout + 3 target + 5 log category + no BOM
- 24 native gameplay tag declared
- Build.cs dependency: EnhancedInput / GameplayAbilities / GameplayTags / GameplayTasks
- Input config shape, Player/Pal slot classes, Pal pawn data shape
- Activity FSM shape, GAS shape (5 attr + clamp + ASC + 2 GA), Combat damage shape, Inventory fragment shape

---

## 7. Recommended Q1 → Q2 transition

1. **Trong tuần này (designer):**
   - Author `Raid_Sandbox.umap` (1×1 km flat plane + 1 NavMeshBoundsVolume + 1 PlayerStart đặt z=200).
   - Author 11 Blueprint subclass (1 Player BP + 1 Pal BP + 4 GE + 1 GA Fire + 1 Dummy + 5 DA Item).
   - Chạy 1 lần PIE end-to-end theo [`Q1-Playtest-Checklist.md`](Q1-Playtest-Checklist.md).
   - Gửi video 30s back vào `Documents/Devlog/Q1-Playtest-Video.md` (kèm screenshot/youtube link).
2. **Đầu Q2 (dev):**
   - PR đầu Q2: Lyra ALS minimal port (Path A § 4) — đóng K-01.
   - Tiếp theo: W14-15 dedicated server (login flow basic + listen→dedicated + 4 player connect + replicate Pal pos/anim).

---

## 8. References

- [`Documents/PALDARK/03-Roadmap_1_Year.md`](../PALDARK/03-Roadmap_1_Year.md) — toàn bộ 52-week schedule.
- [`Documents/PALDARK/02-Pillar_Coverage.md`](../PALDARK/02-Pillar_Coverage.md) — pillar target / mapping feature.
- [`Documents/UE5_Core_Pillars.md`](../UE5_Core_Pillars.md) — 18 pillar definition.
- [`PaldarkLab/README.md`](../../PaldarkLab/README.md) — feature-by-feature authoring + test loop (516 dòng).
- [`Documents/Devlog/Q1-Playtest-Checklist.md`](Q1-Playtest-Checklist.md) — runnable PIE walkthrough.
- [`Documents/Devlog/Q1-Bug-Bash.md`](Q1-Bug-Bash.md) — 15-issue priority list cho 1-day bug bash.
