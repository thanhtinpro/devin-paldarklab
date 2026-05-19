# So sánh 3 Dự Án Thực Tế: Palworld vs PUBG vs Ready or Not

> Tổng hợp 3 đánh giá ([Palworld](Projects/Palworld_Assessment.md), [PUBG](Projects/PUBG_Assessment.md), [ReadyOrNot](Projects/ReadyOrNot_Assessment.md)) thành 1 bảng so sánh duy nhất.
>
> Mục tiêu: hiểu vì sao 3 game khác nhau **chọn** kiến trúc khác nhau — và rút bài học cho các dự án mới.

---

## 1. Profile tóm lược

| | Palworld (skeleton) | PUBG-KI (fan-made) | Ready or Not (AAA shipped) |
|---|---|---|---|
| **Studio** | KYWorld (fan) | PUBG-KI (fan) | Void Interactive |
| **Genre** | Creature-collect + Survival + Co-op | Battle Royale 100p | Tactical FPS co-op + PvP |
| **Trạng thái** | Skeleton (5–10%) | Prototype (~55%) | Shipped (~95% relevant) |
| **Số file C++** | ~70 | ~263 | ~1.500 |
| **LOC ước tính** | ~5.000 | ~50.000 | ~298.000 |
| **GAS** | ✅ Setup chưa impl | ✅ Có Ability/Task | ❌ Không dùng |
| **Lyra** | ❌ | ❌ | ❌ |
| **Manager pattern** | (chưa) | 7 Manager (AActor) | 16 Manager (mix Actor + Subsystem) |
| **Composition** | Component skeleton | 6 Component, gọn | 66 Component nhưng vẫn god-object |
| **AI approach** | (chưa) | (không có — pure PvP) | Activity FSM + Utility AI (custom) |
| **Network** | `bReplicates` cờ | Lobby + Session UI | AdvancedSessions + HostMigration P2P |
| **Dedicated** | Chưa | Có nhưng chưa AWS | Không cần (co-op P2P) |
| **Save** | Không | Không (BR pure) | DataSingleton + Roster |
| **Open-world** | Có (planned) | Có (BR map 8x8km) | Không (mission map) |
| **Coverage tổng (theo 18 pillar)** | **~21%** | **~55%** | **~73%** |

---

## 2. Pillar Matrix so sánh

Trục: 18 pillar × 3 project. Số là % coverage so với target genre của project đó.

| Pillar | Palworld | PUBG | RoN |
|--------|---------|------|-----|
| **P1 C++/Build** | 60% | 75% | 90% |
| **P2 Core Framework** | 25% | 90% | 95% |
| **P3 Composition** | 30% | 85% | 60% (god-obj debt) |
| **P4 Enhanced Input** | 40% | 80% | 75% |
| **P5 Animation** | 20% | 60% | 85% |
| **P6 Replication** | 10% | 60% | 85% |
| **P7 Dedicated Server** | 5% | 40% | 50% (P2P thay vì dedicated) |
| **P8 GAS** | 30% | 70% | **0% (không dùng)** |
| **P9 AI** | 5% | 10% (không cần) | **100%** |
| **P10 UI** | 20% | 85% | 100% (god-widget debt) |
| **P11 Inventory** | 25% | 80% | 85% |
| **P12 Data-driven** | 50% | 70% | 85% |
| **P13 Save/Load** | 0% | 0% | 80% |
| **P14 AssetMgr/Async** | 10% | 30% | 70% |
| **P15 Performance** | 5% | 30% | **95%** |
| **P16 Math/Physics/Vehicle/Audio** | 30% | 60% | 90% |
| **P17 Lyra** | 0% | 0% | 0% |
| **P18 Backend / Live Ops** | 0% | 20% | 70% |
| **AVG (un-weighted)** | **20%** | **53%** | **73%** |

### Heatmap

```
P1   Palworld 🟠  PUBG 🟢  RoN 🟢
P2   Palworld 🔴  PUBG 🟢  RoN 🟢
P3   Palworld 🔴  PUBG 🟢  RoN 🟡
P4   Palworld 🟡  PUBG 🟢  RoN 🟢
P5   Palworld 🔴  PUBG 🟡  RoN 🟢
P6   Palworld 🔴  PUBG 🟡  RoN 🟢
P7   Palworld 🔴  PUBG 🟡  RoN 🟡
P8   Palworld 🟡  PUBG 🟢  RoN ⚫ (no use)
P9   Palworld 🔴  PUBG ⚫  RoN 🟢
P10  Palworld 🔴  PUBG 🟢  RoN 🟢
P11  Palworld 🟡  PUBG 🟢  RoN 🟢
P12  Palworld 🟡  PUBG 🟢  RoN 🟢
P13  Palworld 🔴  PUBG ⚫  RoN 🟢
P14  Palworld 🔴  PUBG 🔴  RoN 🟡
P15  Palworld 🔴  PUBG 🔴  RoN 🟢
P16  Palworld 🔴  PUBG 🟡  RoN 🟢
P17  ⚫ ⚫ ⚫ (no project use Lyra)
P18  Palworld 🔴  PUBG 🔴  RoN 🟡
```

Legend: 🟢 ≥75% • 🟡 50–74% • 🟠 25–49% • 🔴 <25% • ⚫ Not applicable.

---

## 3. Architectural choice — bài học chéo

### Composition (P3)

| Project | Cách dùng | Bài học |
|---------|-----------|---------|
| **Palworld** | Component skeleton có sẵn (`PawnExtension`, `PawnEquipment`, `InventoryComponentBase`) nhưng rỗng | Đúng hướng — phải fill |
| **PUBG** | 6 Component (Equipped/Input/Inventory/ItemData/Movement/NearArea) đủ vừa phải | **Gold standard** cho game mid-size |
| **RoN** | 66 Component **nhưng** logic vẫn nằm trong PlayerCharacter | **Anti-pattern**: tạo component nhưng vẫn để character to. Component phải **own** logic, không phải mượn |

> **Take-away cho Palworld:** học từ PUBG (composition mid-size đúng mức), tránh trap của RoN (component giả tạo).

### AI (P9)

| Project | Cách dùng | Bài học |
|---------|-----------|---------|
| **Palworld** | Chưa có | Cần BT hoặc StateTree cho Pal — học từ [01] Tom + ReadyOrNot |
| **PUBG** | Không cần (pure PvP) | OK |
| **RoN** | **Activity FSM + Utility AI + Custom Sense + ThreatOctree** | **Top tier**. Pal AI nên học pattern này (Pal có nhiều activity: Work/Combat/Idle/Follow) |

> **Take-away cho Palworld:** Pal AI **phải** học từ RoN, không phải [11] Aura BT đơn giản.

### Manager pattern

| Project | Cách dùng | Bài học |
|---------|-----------|---------|
| **Palworld** | Chưa | Setup ngay từ đầu thành `UWorldSubsystem` |
| **PUBG** | 7 Manager (AActor singleton) | Debt — convert sang `UWorldSubsystem` |
| **RoN** | 16 Manager (mix Actor + Subsystem) | Mature nhưng vẫn còn debt |

### GAS (P8)

| Project | Cách dùng | Bài học |
|---------|-----------|---------|
| **Palworld** | Skeleton ASC + AS + Ability có sẵn | Đúng hướng — phải impl (theo [05] + [11]) |
| **PUBG** | Có GA cho Airplane_Fall/Look + weapon GA | Mid-tier GAS sử dụng |
| **RoN** | **Không dùng** — game ship trước thời GAS mature | Bài học: GAS không bắt buộc cho game AAA shipped — đúng tooling hay đúng game design quan trọng hơn |

### Performance (P15)

| Project | Cách dùng | Bài học |
|---------|-----------|---------|
| **Palworld** | Chưa | Open-world game **bắt buộc** SignificanceManager. Đầu tư sớm |
| **PUBG** | Chưa thấy | 100-player BR cần SignificanceManager + spatial partition |
| **RoN** | Significance + ThreatOctree + Audio Propagation + Reflex | Production-grade |

### UI (P10)

| Project | Cách dùng | Bài học |
|---------|-----------|---------|
| **Palworld** | Skeleton | Cần widget hierarchy đầy đủ |
| **PUBG** | 8 sub-folder widget gọn | **Mid-size gold standard** |
| **RoN** | 180 widget, SwatCommand 5K LOC | God-widget anti-pattern. Cần MVVM |

---

## 4. Genre × Pillar — match analysis

| Genre | Pillar bắt buộc | Pillar nice-to-have | Pillar không cần |
|-------|-----------------|----------------------|------------------|
| **Creature-collect Co-op Survival** (Palworld) | P2, P3, P6, P8, P9, P11, P12, P13, P15, P7 | P5, P10, P14, P18 | P17 |
| **Battle Royale** (PUBG) | P2, P3, P6, P7, P10, P15 (lag comp + significance), P12 | P5, P11, P18 | P9, P13, P17 |
| **Tactical FPS Co-op** (RoN) | P2, P3, P5, P6, P9, P10, P11, P15 | P12, P13, P14, P18 | P7 (P2P OK), P8, P17 |

**Đối chiếu thực tế:**

| | Bắt buộc hoàn thành? |
|---|----------------------|
| Palworld | ❌ — thiếu **P6/P7/P8/P9/P13/P15** (toàn pillar bắt buộc) |
| PUBG | ❌ — thiếu **P15 (Significance) + P7 (AWS)** |
| RoN | ✅ — hoàn thành tất cả pillar bắt buộc |

---

## 5. Donchitos Framework — apply chéo

| Phase Donchitos | Palworld | PUBG | RoN |
|-----------------|---------|------|-----|
| `/brainstorm` | PDF spec có | PDF spec có | GDD có |
| `/decompose` | Spec → 11 module trong PDF | Code-implicit | Code-implicit |
| `/define` (GDD) | Có PDF + .docx GDD Palworld | Có PDF | GDD .docx + 2 sơ đồ |
| `/define-pillars` | **Chưa có** | **Chưa có** | Implicit từ gameplay |
| `/architecture-decision` | **Chưa có ADR nào** | **Chưa có ADR** | **Chưa có ADR** (nhưng quyết định nằm trong tech doc) |
| `/map-systems` | Đã có ở Palworld Assessment §2 | Đã có ở PUBG Assessment §2 | Đã có 14 module trong kien_truc_codebase |
| `/create-story` | Chưa | Chưa | Chưa (game đã ship) |
| `/review` | Chưa CI gate | Chưa CI gate | Studio internal |
| `/release` | Chưa | Chưa | Đã ship |

> **Take-away:** cả 3 project đều thiếu **ADR pattern**. Apply Donchitos để bắt đầu ghi ADR cho Palworld trước (vì còn early).

---

## 6. Khuyến nghị áp dụng

### Cho Palworld (skeleton)
1. **Học pattern Activity AI từ RoN** cho Pal behavior.
2. **Học composition mid-size từ PUBG** (đừng rơi vào trap god-object).
3. **Học SignificanceManager từ RoN** cho open-world.
4. Impl GAS full theo [05] + [11].
5. Setup dedicated server theo [08] + [13] vì target 4-32 co-op.
6. Save system theo [01] Tom Looman.
7. Bắt đầu ghi ADR ngay (chỉ cần 6-8 cái lớn).

### Cho PUBG (prototype → production)
1. **Convert Manager AActor sang `UWorldSubsystem`.**
2. **Thêm SignificanceManager** (học từ RoN).
3. **Lag compensation** theo [10].
4. **Dedicated server + GameLift** theo [08] + [13].
5. Replay subsystem cho death cam.
6. Bot AI nếu cần fill.

### Cho RoN (shipped → maintain)
1. Refactor god-object character (low priority — game đã ship).
2. Refactor SwatCommandWidget thành MVVM.
3. Editor tool (WorldDataGenerator) tách khỏi runtime.
4. Nếu retrofit GAS — chỉ cho weapon damage chain.

---

## 7. Cross-project pattern playbook

3 project, **5 pattern đáng tách ra thành playbook** dùng chéo cho dự án mới:

| Playbook | Source | Áp dụng cho |
|----------|--------|-------------|
| **Activity FSM + Utility AI** | RoN `Info/Activities/` | Bất kỳ game có AI sandbox phức tạp (Palworld Pal, NPC RPG) |
| **Manager-as-Subsystem** | PUBG nâng cấp + RoN partial | Mọi game (thay AActor singleton) |
| **Per-mode GM+GS pair** | RoN `GameModes/` | Multi-mode game (PUBG đã có pattern này) |
| **Component composition mid-size** | PUBG `Component/` | Game mid-team (không god-object như RoN, không skeleton như Palworld) |
| **BR-specific actor toolkit** (Airplane/Parachute/Zone/Drop) | PUBG `GameplayActor/` | Bất kỳ game BR/Survival/RPG có zone-based gameplay |

---

## 8. Câu hỏi mở (cho Decision)

Khi user muốn build game mới, 5 câu hỏi cốt lõi (lấy cảm hứng từ Donchitos `/define-pillars`):

1. **Genre cốt lõi là gì?** → Quyết định pillar bắt buộc (xem §4).
2. **Multiplayer scope?** → Quyết định P6/P7/P18.
3. **Open-world hay map-based?** → Quyết định P15 (Significance).
4. **Single ability simple hay damage chain phức tạp?** → Quyết định GAS adoption.
5. **Team size + budget?** → Quyết định Lyra (over-engineered cho team < 10 người).

> Trả lời 5 câu này = pillars target = % coverage cần đạt = roadmap rõ.

---

## 9. Kết luận

| Project | Đánh giá tổng |
|---------|---------------|
| **Palworld** | Skeleton có hướng đi đúng (composition + GAS) nhưng còn rỗng. Cần đầu tư 5x. Học pattern AI từ RoN, composition từ PUBG. |
| **PUBG** | Prototype tốt, gần production. Cần fill 3 pillar P7/P10/P15 và refactor Manager. |
| **RoN** | Shipped, gameplay đầy đủ, có tech debt scale. Đáng học các pattern AAA (Significance, Activity AI, Audio Propagation) — không copy god-object. |

3 game không thay thế nhau — chúng **bổ sung lẫn nhau** thành 1 case study đầy đủ:
- **Palworld** dạy "đầu tư đúng vào skeleton có composition + GAS từ ngày 1".
- **PUBG** dạy "composition + Manager mid-size đúng mức cho team nhỏ-trung bình".
- **RoN** dạy "pattern AAA + warning về tech debt khi scale lên 300K LOC".
