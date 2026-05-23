# Ready or Not (Void Interactive) — Đánh giá theo UE5 Core Pillars

> Source: `04.ReadyOrNot/` — game AAA đã shipped, ~1.500 file .h/.cpp, ~298.000 LOC.
> Tài liệu kiến trúc 612 dòng đã có sẵn: [kien_truc_codebase_readyornot_vi.md](../../04.ReadyOrNot/Documents/kien_truc_codebase_readyornot_vi.md).
> GDD .docx + 2 sơ đồ Gameplay Flow / AI Flow trong cùng folder.
>
> **Tính chất codebase:** Đây là game đã ship — phản ánh **đầy đủ tech debt từ scale**. Không phải "best practice" — học **vì sao** quyết định kiến trúc ra như vậy, không copy mù.
>
> Đánh giá apply [Donchitos framework](../Donchitos_GameStudios_Framework.md) và [UE5 Core Pillars](../UE5_Core_Pillars.md).

---

## 1. Concept

| Khía cạnh | Mục tiêu | Hiện trạng code |
|-----------|----------|-----------------|
| **Game type** | Tactical FPS (SWAT-style room clearing) | Đầy đủ và đã shipped |
| **Target platform** | PC + PS4/PS5 + Xbox One/Series X | UE5 cross-platform, có console subsystems |
| **Multiplayer scope** | Co-op 5 player + nhiều PvP mode | AdvancedSessions, HostMigrationManager |
| **Project maturity** | Production (đã release) | ~100% gameplay shipped, nhiều mode |

### Game Pillars (suy ra từ gameplay + GDD)
1. **Tactical realism** — Lethality cao, mỗi viên đạn quan trọng, lean/peek/cover quan trọng.
2. **Command + control** — Player ra lệnh đội SWAT AI (radial menu).
3. **Mission-based, replayable** — Map có replay value qua loadout + AI placement.
4. **Co-op trên hết** — PvE Coop là mode chính, PvP là bonus.

---

## 2. Systems Map (đối chiếu với 14 module phân tích sẵn)

| Module | Status | Pillar map |
|--------|--------|-----------|
| Core Framework | 🟢 | P2 |
| Characters | 🟢 (god-object) | P2 + P3 (debt) |
| Actors (Items, Weapons, Doors) | 🟢 | P2 + P11 |
| Components | 🟢 | P3 |
| AI System (Activity + Utility) | 🟢 (custom — NOT BT/StateTree) | P9 |
| Game Modes (15+) | 🟢 | P2 |
| Managers (Info/) | 🟢 (16 Manager classes — debt nhưng functional) | P2 |
| UI/HUD (180 widget) | 🟢 | P10 |
| Data Assets (42) | 🟢 | P12 |
| Animation | 🟢 | P5 |
| Libraries (lib/) | 🟢 (god-libs) | P1 (debt) |
| Navigation (NavMesh + ThreatOctree) | 🟢 | P9 + P16 |
| Commander / Metagame | 🟢 | P18 |
| Support (Debug + Audio + Localization) | 🟢 | P15 + P10 |

---

## 3. Đánh giá theo Pillars

| Pillar | % | Trạng thái | Bằng chứng | Debt / lưu ý |
|--------|---|-----------|------------|--------------|
| **P1 C++/Build** | 90% | 🟢 | Module size, plugin list (FMODStudio, mod.io, AdvancedSessions, DynamicCoverSystem, CustomAnimNode, Reflex, VivoxCore, AimAssistSystem, AMRagdoll) | Build dài; thiếu module split |
| **P2 Core Framework** | 95% | 🟢 | 15+ GameMode, mỗi mode GM+GS pair | OK |
| **P3 Composition** | 60% | 🟡 | 66 component | PlayerCharacter.cpp 10k LOC là **god-object** — composition đáng lẽ phải phẳng hơn |
| **P4 Enhanced Input** | 75% | 🟢 | Input layer trong PlayerController 5k LOC | Nên tách thành IA + Component |
| **P5 Animation** | 85% | 🟢 | `Animation/`, `AnimInputs/`, `CustomAnimNode` plugin, motion warping | Sophisticated |
| **P6 Replication** | 85% | 🟢 | AdvancedSessions, HostMigrationManager, GS replicated | Có host migration P2P |
| **P7 Dedicated Server / Sessions** | 50% | 🟡 | Mainly P2P + AdvancedSessions, không phải GameLift fleet | Phù hợp scope co-op 5 người (không cần fleet) |
| **P8 GAS** | 0% | 🔴 | **Không dùng GAS** | Custom ability path through PlayerCharacter — debt |
| **P9 AI** | 100% | 🟢 | **Custom AI:** Activity FSM + Utility AI (AIAction/Consideration/Gate) + Custom Sense + ThreatOctree | **Top tier**. Đáng học pattern Activity |
| **P10 UI** | 100% | 🟢 | 180 widget, SwatCommand 170KB radial, PreMissionPlanning 94KB | Sophisticated, nhưng widget size debt |
| **P11 Inventory** | 85% | 🟢 | InventoryComponent 50KB + LoadoutManager | Functional, không có Item Fragment |
| **P12 Data-driven** | 85% | 🟢 | 42 UDataAsset (item, level, AI archetype, customization) | Tốt |
| **P13 Save/Load** | 80% | 🟢 | DataSingleton + GameUserSettings + Roster/Profile | Profile-based |
| **P14 AssetMgr/Async** | 70% | 🟡 | AsyncLoader trong lib/ | Có thể không dùng PrimaryAssetManager |
| **P15 Performance** | 95% | 🟢 | **ReadyOrNotSignificanceManager**, **ThreatOctree**, **FMODAudioPropagationComponent**, Reflex (low latency) | Production-grade |
| **P16 Math/Physics** | 90% | 🟢 | DynamicCoverSystem plugin, AMRagdoll, Custom AnimNode | Tactical movement |
| **P17 Lyra** | 0% | 🔴 | Không dùng Lyra (game predates Lyra production-ready) | OK |
| **P18 Backend** | 70% | 🟢 | ReadyOrNotBackend 31KB, ModioManager 21KB, MapStatisticsSystem | Mod.io + telemetry |

**Tổng %:**

| Group | % | Weight | Score |
|-------|---|--------|-------|
| Foundation | 80% | 25 | 20.0 |
| Combat (P5, P6, P8, P9) | 67% (no GAS) | 25 | 16.8 |
| Content (P10, P11, P12, P13) | 88% | 20 | 17.5 |
| Production (P14, P15, P7, P18) | 71% | 20 | 14.2 |
| Specialty (P16, P17) | 45% | 10 | 4.5 |
| **TOTAL** | | 100 | **~73%** |

> **ReadyOrNot ~73% coverage** (so với target tổng hợp; nếu loại GAS+Lyra vì không relevant cho game, sẽ ~85%).

---

## 4. Đã làm được gì (positive — đáng học)

1. **Activity System cho AI:** thay vì BT thuần (vốn flat và khó tăng độ phức tạp), Void dùng Utility + Activity FSM. **Pattern đáng nghiên cứu cho AI sandbox hành xử mở** (Pal AI trong Palworld có thể học pattern này).
2. **Custom Sense (`ReadyOrNotAISense_Sight`):** override UE Perception mặc định vì nhu cầu specific (cone vision, occluder). Pattern: kế thừa `UAISense_Sight` và override `Update()`.
3. **ThreatAwarenessSubsystem + ThreatOctree:** spatial query nhanh cho "threat trong khu vực" — bài học pattern Octree cho game open-area.
4. **SignificanceManager dùng đúng cách:** không AAA UE5 nào không có. Tham khảo class `ReadyOrNotSignificanceManager`.
5. **Manager pattern phân lớp đúng:** mỗi domain 1 Manager (SWAT, Scoring, Sound, Backend, Modio, CSGas…) — concern separation tốt **nếu** Manager đó là Subsystem. Trong ReadyOrNot nhiều Manager là AActor — debt nhỏ.
6. **Per-mode GameMode + GameState pair:** 15+ mode mà không loạn vì pattern nhất quán.
7. **Door system như 1 sub-system riêng** (269KB) — vì door là cốt lõi tactical FPS (breach, lockpick, C2, slap, mirror, dùng AI scan). Concept đáng học: **chọn 1 verb cốt lõi và đầu tư nó hoàn chỉnh.**
8. **FMODAudioPropagationComponent:** âm thanh truyền qua phòng (sound occlusion). Khó cài, AAA mới có.
9. **Cross-platform subsystems:** ConsoleMultiplayerSubsystem + PS5ActivitiesSubsystem + AchievementSubsystem — gọn từng GameInstanceSubsystem cho từng platform feature.

---

## 5. Tech debt nổi bật (không nên copy)

| Vấn đề | Debt | Tại sao |
|--------|------|---------|
| **PlayerCharacter.cpp 10.500 LOC** | Critical | God-object, không test được, merge conflict liên tục. Đáng lẽ phải tách thành 10 component (Movement, Lean, Cover, Interaction, Weapon, Camera, Input, Network, Voice, Sandbox). |
| **ReadyOrNotCharacter.cpp 8.000 LOC** | Critical | Base class lẽ ra phải mỏng |
| **PlayerController.cpp 5.100 LOC** | High | Input chia thành Component được |
| **SwatCommandWidget.cpp 5.000 LOC** | High | 1 widget 5K LOC = không UX iterate được. Cần MVVM |
| **CommandFunctionLibrary 5.000 LOC** | High | "God library" |
| **BpGameplayHelperLib 4.300 LOC** | High | Dump bag for Blueprint helper |
| **WorldDataGenerator 7.900 LOC** | Medium | Off-line tool generator nên là editor module, không trong runtime |
| **SWATManager 4.600 LOC** | Medium | Manager mà to thì nên tách Coordinator + State + Executor |
| **Door 7.700 LOC** | Acceptable | Door là cốt lõi game — đầu tư là đúng |
| **Manager AActor thay vì Subsystem** | Low | Một số Manager spawn vào level, hơi chậm và spawn-then-find |
| **Không có GAS** | N/A | Quyết định trước thời GAS production-ready. Nếu làm lại 2025, GAS sẽ dùng cho ability damage chain |
| **Không có Lyra** | N/A | Game predates Lyra |

---

## 6. Cấu trúc — đáp ứng không?

### Đáp ứng tốt

| Pattern | Đánh giá |
|---------|----------|
| Module decomposition (14 area) | ✅ |
| Per-mode GameMode + GameState | ✅ |
| 16 Manager separation | ✅ (mỗi cái 1 concern) |
| Subsystem cho platform feature | ✅ |
| Interface pattern (20 interface) | ✅ |
| DataAsset cho content (42) | ✅ |
| Plugin separation cho cross-cutting (FMOD, mod.io, Reflex) | ✅ |
| Activity-based AI | ✅ (innovate) |

### Sai / debt

| Vấn đề | Khuyến nghị (nếu refactor) |
|--------|---------------------------|
| God Character (10K LOC) | Composition: tách thành Components |
| God Widget (SwatCommand 5K LOC) | MVVM pattern |
| God Lib (CommandFunctionLib 5K LOC) | Split by domain |
| Manager là AActor | Convert sang `UWorldSubsystem` |
| Editor tool ở runtime (WorldDataGenerator) | Convert sang `UEditorSubsystem` |
| Không có GAS | (Refactor lớn, có thể không justify) |
| Không có test module | Add `Source/ReadyOrNotTests/` |

---

## 7. Patterns đáng tách thành "playbook" cho Palworld/PUBG

| Pattern | Source | Apply cho |
|---------|--------|-----------|
| **Activity FSM + Utility AI** | `Info/Activities/` + `AI/AIAction*` | **Palworld Pal AI** (Work/Combat/Idle/Follow) |
| **SignificanceManager** | `ReadyOrNotSignificanceManager` | **Palworld + PUBG** open-world LOD |
| **ThreatAwarenessSubsystem (Octree)** | `Subsystems/ThreatAwarenessSubsystem` + `Octree/` | **PUBG** zone awareness |
| **Custom AISense** | `Senses/ReadyOrNotAISense_Sight` | Bất kỳ game cần vision tùy chỉnh |
| **AudioPropagation FMOD** | `FMODAudioPropagationComponent` | Tactical FPS / horror |
| **HostMigrationManager** | `Info/HostMigrationManager` | Co-op P2P games |
| **Manager + Subsystem hỗn hợp** | `Info/*` | Bất kỳ game lớn |
| **Per-mode GM/GS pair** | `GameModes/*` | Multi-mode game |
| **DataSingleton lib pattern** | `lib/DataSingleton` | Persist data nhẹ |
| **Per-domain RadialWidget** | `RadialWidgetBase` + `SwatCommandWidget` | Vehicle/Inventory/Pal command radial UI |

---

## 8. Ánh xạ tới khóa học

| Pattern | Course gần nhất | Khác biệt |
|---------|-----------------|-----------|
| Activity AI | Không có course — học từ ReadyOrNot study + [01] BT/EQS basis | StateTree [15] cũng gần |
| Significance | [14] Lyra chương 4 | Tương đồng |
| FMOD Audio Propagation | Không có | Cần FMOD docs |
| Host Migration | Không có | AdvancedSessions plugin docs |
| AdvancedSessions vs OnlineSubsystem | Không có | Plugin-specific |
| Custom AISense | Không có chính thức | UE docs + ReadyOrNot code |
| GAS damage chain (nếu retrofit) | [11] Aura | Áp cho weapon system của RoN |

---

## 9. Kết luận

- **ReadyOrNot ~73% coverage**, nếu chỉ tính pillar **relevant** cho tactical FPS thì ~85%.
- **Đã shipped, gameplay đầy đủ** — không thiếu, chỉ có **tech debt**.
- **Pattern đáng học nhất:** Activity AI + Significance + ThreatOctree + Custom Sense + Audio Propagation. Đây là 5 món AAA mà không khóa nào dạy hết.
- **Pattern không nên copy:** God Character / God Widget / Manager-as-Actor / Editor-tool-in-runtime. Đây là debt từ scale.
- **Đặc biệt:** *không* dùng GAS, *không* dùng Lyra — chứng minh game AAA vẫn ship được mà không cần 2 framework đó. Nhưng nếu khởi tạo lại 2025, sẽ có lý do để cân nhắc GAS cho weapon/ability damage chain.
