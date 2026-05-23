# PALDARK — 12-Month Roadmap (52 tuần → Beta)

> Nguyên tắc: **0.1% mỗi ngày, 365 ngày = beta**.
>
> Mỗi tuần có **Outcome cụ thể có thể chạy/test được** — không "đang nghiên cứu". Nếu tuần X không show được build/screenshot/demo → flag risk.
>
> Áp [Donchitos Story Workflow](../Methodology/01-Donchitos_Applied.md) cho mỗi feature → US ticket.

---

## Tổng Quan 4 Quarter

| Q | Tuần | Theme | Outcome |
|---|------|-------|---------|
| **Q1** | 1–13 | **Foundation** | Build chạy được, 1 Pal companion follow player trên map test |
| **Q2** | 14–26 | **Combat & Network** | 4 player dedicated server, GAS combat, Pal combat AI |
| **Q3** | 27–39 | **Content & World** | 3 map, 10 Pal, full UI, inventory, save/load |
| **Q4** | 40–52 | **Polish & Beta** | Hub town, backend AWS, polish, closed beta |

Mỗi quarter có **Milestone Build** (binary playable end-of-Q).

---

## Q1 — Foundation (Week 1–13)

### Tuần 1–2: Setup & Lyra base
| Day | Task | Pillar | Source |
|-----|------|--------|--------|
| 1 | Clone Palworld + Lyra sample. Fork Lyra | P17 | [14] |
| 2 | Build server target Lyra | P1+P7 | [08] |
| 3 | Encoding BOM fix, log category | P1 | Refactor_Plan.md F-01 |
| 4 | Setup `Paldark` module hierarchy (7 module) | P1 | Refactor_Plan.md Phase 0 |
| 5 | Setup Github Actions CI (Build + lint) | P1 | — |
| 6–7 | Lyra Experience hello-world cho "Test Map" | P17 | [14] |
| 8–10 | Custom Lyra Experience cho "Raid_Sandbox" | P17 | [15] |
| 11–14 | Lyra Input Config + Enhanced Input | P4 | [15] |

**Outcome tuần 2:** Run dedicated server localhost, 1 player connect, Lyra character chạy được trên Raid_Sandbox.

### Tuần 3–4: Player & Pal Pawn skeleton
- Tách `APaldarkCharacter` từ Lyra base.
- 12 Component slot trên player (đặt empty Component class).
- `APaldarkPalCharacter` — Pawn cơ bản, 8 Component slot.
- `UPaldarkPalLocomotionComponent` — Pal đi sau player 5m.
- Anim BP placeholder.

**Outcome tuần 4:** Player + 1 Pal companion trên map test, Pal follow player với navmesh.

### Tuần 5–6: Activity FSM port từ RoN
- Đọc `04.ReadyOrNot/Source/.../Activities/BaseActivity.h+cpp`.
- Tạo `UPaldarkBaseActivity`.
- Tạo `UPaldarkPalActivityComponent` chạy Activity.
- 3 activity: Idle, Follow, Investigate.

**Outcome tuần 6:** Pal switch giữa Idle ↔ Follow ↔ Investigate dựa trên player distance + ping.

### Tuần 7–8: GAS basic + AttributeSet
- Setup GAS plugin.
- `UPaldarkAttributeSet` (Health, Stamina, MoveSpeed).
- Player + Pal có ASC.
- 1 ability: GA_Sprint.

**Outcome tuần 8:** Player sprint = stamina tiêu hao via GA.

### Tuần 9–10: Damage chain
- GE_Damage_Standard, GE_Damage_Headshot.
- MMC, Execution Calc port từ [11] Aura.
- 1 weapon (pistol hitscan) làm damage qua GE.

**Outcome tuần 10:** Player shoot dummy → dummy chết với damage chain (HP/Armor/Headshot).

### Tuần 11–12: Inventory fragment
- Setup fragment system theo [09].
- 3 fragment: Stackable, Equipable, Weight.
- 5 item: Pistol, Ammo_9mm, Bandage, Pal_Sphere, Energy_Drink.

**Outcome tuần 12:** Inventory UI hiện 5 item, pick up/drop, weight cap.

### Tuần 13: Q1 Milestone Build
- Internal playtest: 1 player, 1 Pal, 1 map, 1 weapon, inventory, sprint, shoot dummy.
- Bug bash 1 ngày.

**Q1 Outcome:** Có **gameplay loop nguyên thuỷ** — drop, di chuyển, có Pal follow, có súng bắn được, có inventory.

**Pillar covered Q1:** P1 100%, P2 70%, P3 70%, P4 80%, P5 60%, P8 60%, P9 40%, P11 60%, P17 80%.

---

## Q2 — Combat & Network (Week 14–26)

### Tuần 14–15: 4-player dedicated test
- Login flow basic (no auth).
- Listen server → switch dedicated.
- 4 player connect.
- Replicate Pal position/anim.

**Outcome tuần 15:** 4 player + 4 Pal trên server dedicated test.

### Tuần 16–17: Lag Compensation Server-Side Rewind
- Port pattern từ [10] MP Shooter.
- Frame package buffer trên hitbox.
- Server rewind theo client timestamp.

**Outcome tuần 17:** Hitscan accurate với 100ms ping artificial.

### Tuần 18–19: Pal Combat Activity
- Activity_PalCombat với consideration (threat distance, health, ammo).
- Pal attack enemy với 1 ability.
- Pal aggro switch khi enemy chuyển.

**Outcome tuần 19:** Pal companion auto-defend player khi enemy approach.

### Tuần 20–21: Echo Pal hostile (basic)
- 2 loài: Direhound (pack), Razorbird (aerial).
- Activity hostile: Patrol, Stalk, Attack.
- Spawn manager cơ bản.

**Outcome tuần 21:** Map test có 4 Direhound patrol, attack player khi gần.

### Tuần 22–23: Squad system
- VOIP via Vivox.
- Ping system (T key spot ping).
- Radial wheel (Pal + Squad command).
- Mark enemy.

**Outcome tuần 23:** 4-player team coordinate qua ping + radial command.

### Tuần 24–25: Extraction flow
- Extract beacon actor + timer.
- Extract success → return to lobby.
- Extract fail (die) → loss screen.
- Match end logic.

**Outcome tuần 25:** Full raid loop functional (drop → fight → extract).

### Tuần 26: Q2 Milestone Build
- Internal playtest: 4 player, 4 Pal, 1 map, 4 enemy, full loop.

**Q2 Outcome:** **Vertical slice 1** — 1 map 1 mission có thể chơi end-to-end.

**Pillar covered Q2:** P6 80%, P7 60%, P8 80%, P9 70%, P10 60%, P14 40%, P15 50%.

---

## Q3 — Content & World (Week 27–39)

### Tuần 27–28: AssetManager + Async load
- PrimaryDataAsset cho Pal definition.
- Async load Pal asset on raid start.
- GameFeature plugin cho map.

**Outcome:** Map load không hitch, Pal async pre-warm.

### Tuần 29–30: 5 Pal loài còn lại
- Stoneclad, Vinewraith, Foxparks (companion), Boltmane (boss), Tombat (companion).
- AnimBP per loài.
- Ability per loài.

**Outcome:** 10 loài Pal trong roster.

### Tuần 31–32: Map 2 — "Rừng Hỏng"
- Block out level 1×1km.
- POI placement.
- Spawn point per Pal loài.

**Outcome:** Map 2 playable.

### Tuần 33–34: Map 3 — "Cơ Xưởng PalCorp"
- Indoor close-quarter.
- Loot drop tech-themed.

**Outcome:** 3 map full.

### Tuần 35–36: Pal Bond + capture
- Bond Level 0–20 system.
- Pal Sphere usage → tame minigame.
- Tame success → Pal vào roster.

**Outcome:** Tame được Pal trong raid.

### Tuần 37–38: Inventory full
- 30 item.
- Composite container.
- Backpack tier 1–3.
- Drop on death.

**Outcome:** Loot economy hoạt động.

### Tuần 39: Q3 Milestone Build
- Internal playtest: 3 map, 10 Pal, full inventory, tame mechanic.

**Q3 Outcome:** **Vertical slice 2** — content đủ cho 5 giờ chơi lặp.

**Pillar covered Q3:** P9 90%, P11 95%, P12 90%, P14 80%, P15 80%, P16 60%.

---

## Q4 — Polish & Beta (Week 40–52)

### Tuần 40–41: Hub Town
- Hub map blockout.
- 8-player shard.
- Pal Stable, Marketplace UI, Briefing Room.

**Outcome:** Hub functional.

### Tuần 42–43: Backend AWS
- Cognito auth flow.
- DynamoDB table creation.
- Lambda functions: login, persist_pal, persist_inventory.
- API Gateway.

**Outcome:** Player + Pal persist across session.

### Tuần 44–45: Save/Load + Pal breeding
- Pal gen storage.
- Breeding bench logic.
- Save settings local.

**Outcome:** Breed 2 Pal → con kế thừa gen.

### Tuần 46–47: Audio Propagation FMOD
- FMOD Studio integration.
- Audio Propagation Component cho occlusion.
- Pal voice samples.

**Outcome:** Audio tactical — nghe enemy qua tường.

### Tuần 48–49: Performance pass
- SignificanceManager setup.
- Pal LOD cycle.
- ThreatOctree integration.
- Actor pooling cho bullet.
- Profile + fix top 10 hitch.

**Outcome:** 60 fps trên target hardware (RTX 3060) với 16 Pal in scene.

### Tuần 50: UI polish
- HUD final.
- Indicator Manager polish.
- Menu Lyra style.
- Settings menu.

### Tuần 51: Closed Beta Build
- Internal alpha → friends-and-family.
- Bug triage.
- Telemetry pipeline live (CloudWatch).

### Tuần 52: Beta Launch
- 100 invite key.
- Discord setup.
- Feedback form.
- Day-1 patch ready.

**Q4 Outcome:** **PALDARK Closed Beta** live cho 100 player.

**Pillar covered Q4:** P5 90%, P7 100%, P13 90%, P15 100%, P16 90%, P18 100%.

---

## Cumulative Pillar Progress

```
Pillar     | Q1   Q2   Q3   Q4
P1         | 100% 100% 100% 100%
P2         | 70%  90%  100% 100%
P3         | 70%  85%  95%  95%
P4         | 80%  90%  100% 100%
P5         | 60%  70%  80%  90%
P6         | 30%  80%  100% 100%
P7         | 20%  60%  60%  100%
P8         | 60%  80%  100% 100%
P9         | 40%  70%  90%  100%
P10        | 30%  60%  80%  90%
P11        | 60%  70%  95%  95%
P12        | 50%  70%  90%  100%
P13        | 20%  30%  50%  90%
P14        | 30%  40%  80%  100%
P15        | 20%  50%  80%  100%
P16        | 30%  40%  60%  90%
P17        | 80%  90%  95%  100%
P18        | 0%   10%  30%  100%
-----------+--------------------
Total %    | 47%  62%  79%  97%
```

> **End-of-Q1: 47%** (đủ để cảm vibe game).
> **End-of-Q2: 62%** (vertical slice 1).
> **End-of-Q3: 79%** (content đủ).
> **End-of-Q4: 97%** (beta-ready).

---

## Risk Register

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| FMOD license $$$ | Medium | Medium | Plan B: UE native MetaSound, mất audio propagation, đẩy v1.0 |
| AWS cost beta vượt $500/tháng | Low | Medium | AWS Free Tier + spot instance + scale to zero |
| Lyra Activity FSM impossible to port | Low | High | Plan B: viết Activity FSM custom từ đầu — 3 tuần phụ |
| GAS performance Pal mass spawn | Medium | High | Significance Manager + GAS net update tweak ([13] technique) |
| 1 dev không kịp 52 tuần | High | Critical | (a) cắt scope thêm; (b) outsource asset; (c) push beta sang 14 tháng |

---

## Daily Routine (Mẫu)

Mỗi ngày:
1. **Sáng (1 giờ):** Đọc subtitle/code 1 chương relevant tuần này.
2. **Trưa (3 giờ):** Code task tuần.
3. **Chiều (1 giờ):** Test + screenshot + ghi `Documents/Devlog/YYYY-MM-DD.md`.
4. **Tối (30 phút):** Update PALDARK roadmap status + plan ngày mai.

**Cuối tuần:** Build playtest. Ghi `Documents/Devlog/WeekN-Build.md` + 1 video 30 giây.

---

## Tiếp theo

- [04-Resource_Map.md](04-Resource_Map.md) — học gì từ source nào theo tuần.
- [../Methodology/00-Vision_Reverse_Engineering.md](../Methodology/00-Vision_Reverse_Engineering.md) — cách tự kiểm tra từng ngày có align với vision không.
