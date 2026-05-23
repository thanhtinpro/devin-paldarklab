# PALDARK — Q1 Bug Bash (Tuần 13, 1 ngày)

> **Source:** Roadmap § Tuần 13 "Bug bash 1 ngày." Audit toàn bộ C++ + ini + validator + README ở freeze point W12 → ưu tiên hóa cho 1 ngày bug bash trước khi vào Q2.
>
> **Method:** Grep `TODO|FIXME|HACK` trong `PaldarkLab/Source/` (kết quả: 0 — codebase sạch ở mức comment). Audit thủ công 10 PR description + README để rút known-issue. Cross-reference với pillar gap trong [`Q1-Milestone-Build.md`](Q1-Milestone-Build.md) § 5.

---

## 0. Priority legend

| P0 | Block Q1 outcome — phải fix trước khi tick milestone. |
|----|--------------------------------------------------------|
| P1 | Block hoặc xấu UX nhưng có workaround — fix trong bug-bash day. |
| P2 | Cosmetic / improvement — defer Q2. |
| P3 | Future-proofing — defer Q3+. |

---

## 1. Issue table (15 issue, ưu tiên hóa)

| ID | Priority | Issue | Component | Roadmap defer | Fix sketch |
|----|----------|-------|-----------|---------------|------------|
| K-01 | **P0** | Không có AnimBP cho Player/Pal — mannequin T-pose, không có Idle/Walk/Run blend, locomotion `AddMovementInput`-driven nhưng visual T-pose | Animation | W13.5 hoặc W14 đầu Q2 | Port Lyra ALS minimal: State Machine 3 state + Blend Space + Distance Matching. Estimate: 2 ngày dev. **Status (W27):** Code-side closed by W27 Path A (`UPaldarkAnimInstance` + `UPaldarkPalAnimInstance` + `IPaldarkAnimLayerInterface` + PawnData `DefaultAnimInstanceClass` wiring; xem `PaldarkLab/README.md` § "AnimBP Lyra ALS port (W27)"). Designer task: 2-day AnimBP authoring (State Machine + Blend Space + 3 layer functions) → assets land trên branch riêng. |
| K-13 | **P1** | Sprint không cancel khi Stamina = 0 — `AttributeSet` clamp Stamina ở `[0, MaxStamina]` nhưng `GE_Sprint_Cost` (Infinite Duration Period 0.1s) vẫn tick + drain. Player chạy nhanh vĩnh viễn với Stamina 0 | GAS | W14-15 | Add native tag `Paldark.State.IsExhausted`. Trigger trong `AttributeSet::PostGameplayEffectExecute` khi `Stamina → 0`. `GA_Sprint.AbilityTags` block by `Paldark.State.IsExhausted`. Estimate: 4 giờ. |
| K-02 | **P1** | `Raid_Sandbox.umap` chưa commit (binary asset) — designer mỗi máy tự re-tạo, không reproducible | Map | W13 designer task | Designer commit map + git-lfs setup. Estimate: 1 giờ + 5 phút mỗi `git pull`. |
| K-07 | **P1** | `GA_HitscanFire` dùng `LineTraceSingleByChannel` không có lag compensation — server thấy không khớp client khi ping > 50ms | Network | W16-17 (explicit) | Server-side rewind port từ [10] MP Shooter (`UCompensateLagComponent` + frame buffer 1s). Estimate: 5 ngày. Đây là W16-17 scope, không phải Q1 expectation. |
| K-04 | **P2** | `Paldark.Inventory.Drop` chỉ clear `Entries`, không spawn world pickup actor — player drop xong item biến mất | Inventory | Q2 | `AItemPickupActor` (replicated, mesh + `UInteractionComponent`). `DropAllItems` loop spawn 1 actor cho mỗi entry. Estimate: 2 ngày. |
| K-05 | **P2** | UMG inventory widget chưa tồn tại — chỉ test qua console | UI | Q2 (W13-14 follow-up) | `WBP_InventoryGrid` listen `OnInventoryChanged` multicast. Estimate: 3 ngày (Aura WidgetController pattern). |
| K-06 | **P2** | Encumbrance chưa gate locomotion — `IsOverEncumbered()` true nhưng player chạy bình thường | Inventory | Q2 | Bind `OnInventoryChanged` → recompute `MoveSpeed` modifier (apply / remove `GE_Encumbered`). Estimate: 1 ngày. |
| K-14 | **P2** | Replication client→server chưa wire — chỉ console cmd authority-side mới Add/Remove. Pickup actor sẽ cần `Server_RequestPickup` RPC | Network/Inventory | Q2 | `UFUNCTION(Server, Reliable, WithValidation) void Server_AddItemFromPickup(...)`. Estimate: 4 giờ. |
| K-08 | **P3** | Pal Activity FSM không reload từ savegame — `BeginPlay` mỗi lần init Idle | Save/Load | Q3 | `Serialize` override → save `CurrentActivityTag` + restore qua `SetCurrentActivityByTag`. Estimate: 4 giờ. |
| K-09 | **P3** | InventoryComponent dùng `LoadSynchronous` — fresh client resolve sync khi `OnRep_Entries` first time | Asset/Perf | Q3 (P14 polish) | `FStreamableManager::RequestAsyncLoad` qua AssetManager Bundle. Estimate: 1 ngày. |
| K-10 | **P3** | FastArraySerializer chưa wire — Entries replicate full payload mỗi mutate (Pistol stack 1 mutate → resend toàn bộ inventory) | Network/Perf | W21+ | `FFastArraySerializer` subclass khi inventory > 50 entries. Estimate: 2 ngày. Không phải Q1 expectation (5 item ceiling). |
| K-11 | **P3** | Game Feature plugin chưa setup — chưa modular gameplay | Lyra | Q3 | Tách module `PaldarkInventory` / `PaldarkCombat` ra Game Feature plugin. Estimate: 1 tuần. |
| K-12 | **P3** | `Paldark.Pal.Ping` không clamp Z — ping ngoài navmesh khiến Pal stuck giữa air walk | AI | W18+ | `UNavigationSystemV1::ProjectPointToNavigation` trong `RequestInvestigate`. Estimate: 2 giờ. |
| K-03 | **P3** | Weight cap **advisory** only — `AddItem` không reject khi over capacity | Inventory | Q2 (cùng K-06) | Cùng PR K-06. |
| K-15 | **P3** | Console cmd `Paldark.Inventory.Add` dùng path-based scan (`/Game/Paldark/Items`) — fail silently nếu designer đặt sai folder | Validator | W14 | Validator extend kiểm folder path khớp `DefaultGame.ini` `PrimaryAssetTypesToScan.Directories`. Estimate: 2 giờ. |

---

## 2. P0 / P1 fix plan (1 ngày bug bash)

### Slot 1 — Sáng (P0 K-01: AnimBP basic)

> **Compromise (Path B § Milestone Build § 4):** Dùng UE5 Mannequin default AnimBP cho Q1, ghi nhận T-pose-nếu-có là known issue. Không port Lyra ALS đầy đủ trong bug bash day (đó là 2-ngày task → đẩy đầu Q2).

- [ ] `BP_PaldarkCharacter` Mesh → set `Anim Class = ABP_Manny` (UE5 Quinn/Manny ship default).
- [ ] `BP_PaldarkPalCharacter` Mesh → set `Anim Class = ABP_Manny`.
- [ ] Verify PIE: player chạy WASD thấy mannequin walk anim đúng direction. Pal follow thấy walk anim.
- [ ] Commit `BP_PaldarkCharacter.uasset` + `BP_PaldarkPalCharacter.uasset` (assume git-lfs).

**Estimate: 1 giờ** (chủ yếu fix path BP).

### Slot 2 — Trưa (P1 K-13: Sprint stamina cancel)

Đây là code-side fix, không cần designer.

- [ ] Add native tag `Paldark.State.IsExhausted` trong `PaldarkGameplayTags.h/cpp`.
- [ ] Validator: bổ sung tag vào `EXPECTED_TAGS` set.
- [ ] `UPaldarkAttributeSet::PostGameplayEffectExecute` — sau khi clamp Stamina, nếu `NewStamina <= 0` → apply tag (qua transient GE hoặc `ASC->AddLooseGameplayTag`); nếu `NewStamina > 0.1 * MaxStamina` → remove tag (hysteresis 10%).
- [ ] `UPaldarkGameplayAbility_Sprint::CanActivateAbility` — return false nếu `ASC->HasMatchingGameplayTag(Paldark.State.IsExhausted)`.
- [ ] Update README §"Sprint test loop" để thêm test case "Stamina = 0 → sprint cancel".

**Estimate: 4 giờ** (code + validator + README + PR).

### Slot 3 — Chiều (P1 K-02: Commit Raid_Sandbox.umap)

- [ ] Designer setup git-lfs (`git lfs install` + `.gitattributes` cho `*.umap`, `*.uasset`).
- [ ] Author `Raid_Sandbox.umap` cuối cùng (đã có sketch trong [`Q1-Playtest-Checklist.md`](Q1-Playtest-Checklist.md) § 0).
- [ ] Commit + push.
- [ ] CI validator extend: kiểm `/Game/Paldark/Maps/Raid_Sandbox.umap` exist trong content folder.

**Estimate: 2 giờ** (chủ yếu git-lfs setup + level lighting build).

### Slot 4 — Cuối ngày (optional P2 K-04: pickup actor stub)

Nếu còn thời gian:

- [ ] `AItemPickupActor` class skeleton: replicated, `TSoftObjectPtr<UPaldarkItemDefinition> ItemDef`, `int32 StackCount`, `OnInteract` (chưa wire — chỉ stub).
- [ ] `DropAllItems` loop spawn 1 `AItemPickupActor` cho mỗi entry (chưa wire mesh; spawn raw cube actor cho debug).
- [ ] Validator: kiểm `Public/Inventory/PaldarkItemPickupActor.h` exist.

**Estimate: 4 giờ** (skeleton only — full wire defer Q2 W13-14 follow-up).

---

## 3. Open questions trước khi đóng Q1

Decision cần user (designer + tech lead) trả lời trong bug bash day:

- [ ] **Q-01:** Path A (port Lyra ALS đầy đủ — 2 ngày, delay W14) hay Path B (mannequin default ABP, đi tiếp W14)? Em đề xuất **Path B** trong Milestone Build § 4.
- [ ] **Q-02:** Pickup actor implement Q2 đầu (PR follow-up W13-14) hay defer cuối Q2 (cùng UMG widget)? Em đề xuất **đầu Q2** vì K-04 block "drop" outcome.
- [ ] **Q-03:** UMG inventory widget bắt buộc cho Q2 vertical slice không? Roadmap § Q2 outcome ghi "vertical slice 1, end-to-end" — em hiểu là YES, cần UMG trước W26.
- [ ] **Q-04:** Lag compensation (K-07) start W16-17 như roadmap hay slip sang W18+? Roadmap đã ghi rõ W16-17 → giữ nguyên schedule.

---

## 4. Closure criteria

Q1 milestone đóng khi:

- [ ] ✅ Tất cả P0 fix (K-01).
- [ ] ✅ Ít nhất 2/3 P1 fix (K-13 + K-02; K-07 explicit defer W16-17).
- [ ] ✅ `Documents/Devlog/Q1-Playtest-Checklist.md` tick xong 6/6 section.
- [ ] ✅ Video 30s shipped vào `Q1-Playtest-Video.md`.
- [ ] ✅ Q-01..Q-04 decision logged trong devlog hoặc `02-Pillar_Coverage.md` update.

Nếu P0 K-01 không kịp (path A delay): defer Q1 milestone sang W13.5, push back W14 dedicated test 1 tuần. Đây là decision tech lead, không phải Devin.

**W27 update:** Quyết định cuối là **Path A** (full Lyra ALS port) — code-side
scaffold đã land W27. K-01 code-side **closed**; tick conditional on
designer AnimBP authoring (2 days, blueprint-only). Tag mục P0 trên dashboard
"code-closed / designer-pending" thay vì "open".

---

## 5. References

- [`Documents/Devlog/Q1-Milestone-Build.md`](Q1-Milestone-Build.md) § 5 (Known issues table).
- [`Documents/Devlog/Q1-Playtest-Checklist.md`](Q1-Playtest-Checklist.md) § 7.
- [`Documents/PALDARK/03-Roadmap_1_Year.md`](../PALDARK/03-Roadmap_1_Year.md) § Tuần 13 + § Q2.
- [`PaldarkLab/README.md`](../../PaldarkLab/README.md) — anti-pattern sections per pillar.
