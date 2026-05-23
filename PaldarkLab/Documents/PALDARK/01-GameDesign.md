# PALDARK — Game Design Document (Donchitos 7-phase)

> Áp dụng [Donchitos Framework](../Donchitos_GameStudios_Framework.md) cho PALDARK.
> Vision: [00-VISION.md](00-VISION.md).
>
> Đây là **document game design**, không phải tech doc. Tech sẽ ở [02-Pillar_Coverage.md](02-Pillar_Coverage.md).

---

## Phase 1 — `/brainstorm`

### Câu hỏi gốc
> *"Game gì kết hợp Palworld + Tarkov + Ready or Not mà bán được vé sống?"*

### High-Concept
**Tactical 4-player extraction co-op** trong thế giới Pal hậu-tận-thế. Người chơi mang **1 Pal companion** vào **Dark Zone** ngắn (60–90 phút). Mục tiêu: săn Pal hiếm, loot tech, sống sót để extract. Chết = mất Pal + gear chuyến đó.

### Cảm xúc cốt lõi
- **Căng thẳng** — như Tarkov, mỗi viên đạn quan trọng.
- **Yêu thương** — như Palworld, Pal là bạn đồng hành.
- **Hối hận** — như Lost Light, mất Pal là vết sẹo.
- **Vinh quang** — Pal huyền thoại = bằng chứng kỹ năng.

### Câu chuyện tóm tắt (Lore)
2031. Một tổ chức tên *PalCorp* phát hiện *Neural Resonance* — sóng tâm lý chung Pal–người. Họ build máy khuyếch đại để "control mass Pal". Tai nạn → sóng đảo cực → Pal cổ đại trong vùng nhiễm hoá điên (gọi là *Echo Pal*). Vùng này = Dark Zone. Bạn là *Tamer* — một tay chuyên đi vào tìm Pal sạch + công nghệ PalCorp còn lại.

---

## Phase 2 — `/decompose`

### Hệ thống lớn (Top-level systems)

| Hệ thống | Verb cốt lõi | Tham chiếu pillar |
|----------|--------------|--------------------|
| **Raid Loop** | Drop → Loot → Tame → Combat → Extract | P2, P6, P7 |
| **Hub & Persistence** | Store → Breed → Train → Equip → Plan | P13, P17, P18 |
| **Pal Companion AI** | Follow → Scout → Combat → Bond | P9 |
| **Combat System** | Aim → Shoot → Pal Ability → Tactical | P5, P6, P8 |
| **Inventory & Looting** | Pickup → Stack → Weight → Containerize | P11 |
| **Squad Communication** | Ping → Radial → Voice → Pal command | P10 |
| **World & Spatial** | Navigate → Significance → Threat | P15, P16 |
| **Backend & Live Ops** | Match → Lobby → Persist → Telemetry | P7, P18 |

### Diagram (text)

```
                    ┌─────────────────┐
                    │   Hub Town      │  Persistence layer
                    │  (8-shard)      │  (P13 + P17 + P18)
                    └────────┬────────┘
                             │ Lobby → Match
                             ▼
                    ┌─────────────────┐
                    │  Matchmaker     │  AWS GameLift (P7 + P18)
                    │  (Coordinator)  │  
                    └────────┬────────┘
                             │ Spawn dedicated
                             ▼
                    ┌─────────────────┐
                    │   Dark Zone     │  Per-match server (P6 + P7)
                    │   (1km × 1km)   │
                    │                 │
                    │  Pal AI         │  Activity FSM (P9)
                    │  Threat Octree  │  Spatial (P15)
                    │  Significance   │  Performance (P15)
                    │  Audio Propag.  │  FMOD (P16)
                    │  Loot & Combat  │  Inventory + GAS (P8 + P11)
                    │  Squad UI       │  CommonUI (P10)
                    └────────┬────────┘
                             │ Extract → Persist
                             ▼
                    [Pal/gear → DB → Hub on next session]
```

---

## Phase 3 — `/define` (GDD chi tiết)

### 3.1 Game Loop

#### Macro Loop (1 ngày người chơi)
1. Đăng nhập hub → shard 8 người.
2. Browse market, chat, plan squad.
3. Vào Briefing → chọn map + Pal + gear.
4. Drop vào Dark Zone (raid 45–90 phút).
5. Loot / tame / fight / extract.
6. Return hub → upgrade base, breed Pal, sell loot.
7. Lặp.

#### Micro Loop (trong raid)
1. Drop → scout 5 phút.
2. Combat encounter (2–4 lần) → tactical.
3. Tame attempt (nếu thấy Pal phù hợp).
4. Extract beacon timer (5 phút).
5. Hold extract → bay về.

### 3.2 Player Character

**Class:** Không phân class cố định. Player customization:
- 4 stat: Endurance, Aim, Stealth, Bond Affinity.
- 12 perk slot (mở dần qua level).
- Loadout 3 weapon slot + 1 Pal slot + 3 consumable.

**Movement:** Lyra Locomotion (chạy/đi/lean/crouch/prone). Stamina giới hạn (RoN-style).

### 3.3 Pal Companion

**Bond Level 0–20** quyết định:

| Bond | Khả năng mở |
|------|-------------|
| 0–4 | Follow, basic attack |
| 5–8 | Scout (ra trước 30m) |
| 9–12 | Mark enemy (qua AI Sense) |
| 13–16 | Revive player (1 lần/raid) |
| 17–20 | Special signature ability (mỗi loài 1 cái — Foxparks: distract bomb; Boltmane: lightning dash) |

**Gen system:**
- 4 gen stat: Speed / Strength / Stealth / Smarts (0–100 mỗi cái).
- Breed: parent gen averaged + random mutation ±10.
- Gen ảnh hưởng combat + Activity AI preference.

**Activity FSM của Pal** (port từ ReadyOrNot pattern):
- Idle, Follow, Scout, Cower, Combat, Investigate, Revive.
- Mỗi Activity có **Considerations** chấm điểm → AI tự chọn.

### 3.4 Pal Hostile (Echo Pal)

**Loài cơ bản trong beta:**
| Loài | Hành vi | Threat tier |
|------|---------|-------------|
| Direhound | Pack hunter, ambush | T1 |
| Razorbird | Aerial, hit-and-run | T1 |
| Stoneclad | Tank, charge | T2 |
| Vinewraith | Ranged, root | T2 |
| Boltmane Alpha | Boss, stalk, multi-phase | T3 |

Sử dụng **Activity FSM + custom AISense + ThreatOctree** (port RoN).

### 3.5 Combat

**Súng (P6 + P10):**
- Hitscan + projectile mix.
- Lag compensation server-side rewind (từ [10] MP Shooter).
- 5 loại: Pistol, SMG, Rifle, Sniper, Shotgun. 3 tier mỗi loại.
- Recoil pattern data-driven (DataAsset).

**Pal ability (P8):**
- GAS-based.
- ASC mounted lên Pal Pawn.
- Player có thể trigger ability của Pal qua radial wheel.

**Cận chiến:**
- Melee đơn giản (Lyra base).
- Pal có thể tank đòn melee giúp player.

### 3.6 Inventory

**Theo course [09]:**
- Composite pattern: Container item chứa item.
- Fragment composition: mỗi item là tập hợp fragment (Stackable, Equipable, Consumable, Statful, Weight).
- Weight system: súng nặng giảm stamina.
- Stash unlimited (ở hub), inventory hạn chế (trong raid).

### 3.7 UI

**Trong raid:**
- HUD tối giản: ammo, stamina, compass, Pal status.
- Radial wheel: 8 slot — 4 Pal command + 4 squad command.
- No minimap.

**Hub:**
- Lyra-style menu (FrontEnd activatable widget).
- Pal Manager: list Pal đã thuần.
- Stash, Marketplace, Briefing room.

### 3.8 Audio

- FMOD Studio (như RoN).
- Audio Propagation Component cho occlusion qua wall.
- Pal có voice samples (mỗi loài 20–40 sample).

### 3.9 World

**3 Dark Zone Map (beta):**
| Map | Theme | Khả năng đặc biệt |
|-----|-------|--------------------|
| **Cảng Bỏ Hoang** | Port + container yard | Cover dày, vertical |
| **Rừng Hỏng** | Rừng cháy + glow tree | Vision limited, Pal nhiều |
| **Cơ Xưởng PalCorp** | Indoor lab | Closed quarter, tech loot |

Mỗi map 1×1km, 30–40 POI, 4–6 extract beacon.

### 3.10 Hub Town

- 8-player shard (WorldSubsystem-managed).
- 1 hub map (small town).
- Buildings: Pal Stable, Marketplace, Briefing Room, Crafting Bench, Pal Hall of Fame.
- Persistence: AWS DynamoDB store player + Pal + inventory state.

---

## Phase 4 — `/define-pillars`

### 3 Game Design Pillars (đã định ở Vision)

1. **Pal Bond Trumps Gun** — Pal là bạn, không phải vũ khí.
2. **Information is Survival** — Tín hiệu (âm/vision/scent) quyết định sống.
3. **Tame Once, Trust Forever** — Pal lưu vĩnh viễn, có legacy.

### Mọi feature đối chiếu lại pillar

| Feature đề xuất | Đối chiếu pillar | Quyết định |
|----------------|------------------|------------|
| Pal có thể chết vĩnh viễn | Pillar 1, 3 | ✅ Giữ — tạo đau |
| Permadeath gear | Pillar 2 | ✅ Giữ — tạo căng |
| Quick respawn không mất gì | — | ❌ Cắt — vi phạm pillar 1+3 |
| Auto-aim assist | Pillar 2 | ❌ Cắt — vi phạm pillar 2 |
| Pal lai gen | Pillar 3 | ✅ Giữ |
| MMO 100 player open world | — | ❌ Cắt — scope creep, không ép pillar nào |
| Audio propagation FMOD | Pillar 2 | ✅ Giữ — core |
| PvP raid | Pillar 1, 3 | ❌ Đẩy v1.0 — beta scope |

---

## Phase 5 — `/architecture-decision` (ADR)

> Mỗi quyết định kiến trúc lớn ghi 1 ADR. Lưu ở `Documents/PALDARK/ADR/`.

ADR đầu tiên cần ghi (đặt ở PR sau, không trong PR này):
- ADR-001: Lyra Framework hay không?
- ADR-002: Dedicated Server AWS GameLift hay custom EC2?
- ADR-003: GAS cho Pal ability — full hay subset?
- ADR-004: Activity FSM — port từ RoN hay viết mới?
- ADR-005: Inventory — Fragment ([09]) hay built-in Lyra Inventory ([15])?
- ADR-006: Audio — FMOD ($$$ license) hay UE native?
- ADR-007: Backend — DynamoDB hay relational?
- ADR-008: 4-player vs 6-player squad?

---

## Phase 6 — `/map-systems` (Mỗi system → 1 file)

(Sẽ làm trong PR sau. Sketch:)

```
Documents/PALDARK/Systems/
├── 01-RaidLoop.md
├── 02-HubPersistence.md
├── 03-PalAI.md
├── 04-Combat.md
├── 05-Inventory.md
├── 06-Squad.md
├── 07-World.md
├── 08-Backend.md
```

Mỗi file: Concept → Tech mapping → Class diagram sketch → Course nguồn → Order of dev.

---

## Phase 7 — `/create-story` (Story-driven dev tickets)

(Sau khi system map done. Sketch:)

```
US-001: "Là Tamer, tôi muốn drop xuống Dark Zone cùng 3 đồng đội để bắt đầu raid"
  → DoD: Lobby UI + Matchmaker + Dedicated Server spawn + 4 player connect + spawn loadout
  → Pillar: P2, P6, P7
  → Estimate: 2 tuần

US-002: "Là Tamer, Pal companion phải theo tôi đúng khoảng cách"
  → DoD: Activity Follow + NavMesh + Threat avoidance
  → Pillar: P9
  → Estimate: 1 tuần
```

---

## Glossary

| Thuật ngữ | Định nghĩa |
|-----------|------------|
| **Tamer** | Người chơi |
| **Pal** | Sinh vật bạn có thể bắt + train |
| **Echo Pal** | Pal hoang dại nhiễm xạ trong Dark Zone |
| **Dark Zone** | Map raid (1×1km × 3) |
| **Bond Level** | 0–20, đo độ thân của Pal với player |
| **Extract** | Lên helicopter rời Dark Zone |
| **Hub Town** | Map social shard 8 người |
| **Briefing Room** | Pre-match lobby trong hub |
| **Gen** | 4 stat di truyền của Pal |
| **Pal Hall of Fame** | Bảng vinh danh Pal huyền thoại của server |

---

## Tiếp theo

- [02-Pillar_Coverage.md](02-Pillar_Coverage.md) — kỹ thuật map từng feature vào pillar
- [03-Roadmap_1_Year.md](03-Roadmap_1_Year.md) — 52 tuần
- [04-Resource_Map.md](04-Resource_Map.md) — pillar × 15 source
