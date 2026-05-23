# PUBG (PUBG-KI/PUBG-KI) — Đánh giá theo UE5 Core Pillars

> Source: `03.PUBG/Source/PUBG/` (263 file .h/.cpp).
> Spec: 1 PDF tại `03.PUBG/Documents/PUBG_Development_Specifications_compressed.pdf` (chưa OCR vì spec không phải sản phẩm chính, repo là fan-made).
>
> **Lưu ý:** Đây là **fan-made / community implementation**, không phải code chính thức Krafton. Phân tích là về cấu trúc và pattern, không phải reverse-engineer code thật của PUBG Corp.
>
> Đánh giá apply [Donchitos framework](../Donchitos_GameStudios_Framework.md) và [UE5 Core Pillars](../UE5_Core_Pillars.md).

---

## 1. Concept

| Khía cạnh | Mục tiêu | Hiện trạng code |
|-----------|----------|-----------------|
| **Game type** | Battle Royale (PUBG clone) | Khá đầy đủ — có Airplane, RedZone, Zone, SupplyDrop, ItemSpawn, Lobby |
| **Target platform** | PC | UE5 C++ + dedicated session lobby |
| **Multiplayer scope** | 100 player BR (target gốc), thực tế repo có vẻ scaled-down | Có Session Browser, Lobby, Game Session widgets |
| **Spec progress** | Có 1 PDF (chưa OCR) | ~50% coverage core BR mechanics |

### Game Pillars (đề xuất — chưa có trong code)
1. **Shrinking circle** — zone ép player vào trung tâm.
2. **Loot economy** — find > equip > fight, ai gear tốt hơn = lợi thế.
3. **Vehicle traversal** — map lớn cần vehicle.
4. **Last man standing** — solo/duo/squad, 1 squad cuối thắng.

---

## 2. Systems Map

| Hệ thống | Layer | Status | Priority | Bằng chứng |
|----------|-------|--------|----------|------------|
| Core Framework | Core | 🟢 Implemented | P0 | `BaseGameMode`, `BaseGameState`, `BaseGameInstance`, `BasePlayerState`, `BasePlayerController` |
| Enhanced Input | Core | 🟢 Implemented | P0 | `DataAsset_InputConfig`, `Component/Input/` |
| GAS (ASC, AS, Abilities) | Core | 🟢 Implemented | P0 | `AbilitySystem/`, có sub-folders `Ability/`, `AbilityTask/`, `AsyncTask/` |
| Character | Entity | 🟢 Implemented | P0 | `PUBGCharacter`, `Character/` |
| Weapons (Guns + Grenade + Melee + Projectile) | Entity | 🟢 Implemented | P0 | `Weapon/Guns/`, `Weapon/Grenade/`, `Weapon/MeleeWeapon/`, `Weapon/Projectile/` + GA |
| Items (Armor + ItemEffect) | Entity | 🟢 Implemented | P1 | `Item/Armor/`, `Item/ItemEffect/` |
| Vehicle | Entity | 🟢 Implemented | P0 | `Vehicle_UAZ`, `VehicleBase` |
| Components (Equipped/Input/Inventory/ItemData/Movement/NearArea) | System | 🟢 Implemented | P0 | `Component/` 6 folder |
| Inventory (Drag/Drop, Slot) | System | 🟢 Implemented | P0 | `Component/Inventory/`, `DragDrop/`, `Widgets/Inventory/` |
| BR-specific actors (Airplane, Parachute, Zone, RedZone, SupplyDrop, ItemSpawn) | System | 🟢 Implemented | P0 | `GameplayActor/` |
| Manager pattern | System | 🟢 Implemented | P0 | `Manager/`: DataTable, Airplane, Zone, Landscape, Sound, GameEvent, Time |
| HUD (Crosshair) + Widgets | Presentation | 🟢 Implemented | P0 | `HUD/Crosshair/`, `Widgets/HUD/`, `Widgets/Map/`, `Widgets/Notification/` |
| Lobby / Session Browser | Infra | 🟢 Implemented | P0 | `Widgets/Lobby/`: SessionBrowserWidget, FindGameWidget, CreateGameWidget, GameSessionWidget, UserStateWidget, LobbyPlayerState |
| Result / Score | Presentation | 🟢 Implemented | P1 | `Widgets/Result/`, kill count widget |
| Dedicated server / AWS | Infra | 🟡 Partial | P0 | Có Session browser nhưng chưa rõ dedicated vs listen |
| Lag compensation | System | ❓ Unknown | P0 (FPS competitive) | Chưa thấy server-side rewind code |
| Save / Player progress | System | ❓ Unknown | P2 | Có thể không cần cho BR pure |

Layer: **Core → Entity → System → Infra → Presentation**.

---

## 3. Đánh giá theo Pillars

| Pillar | % | Trạng thái | Bằng chứng | Còn thiếu |
|--------|---|-----------|------------|-----------|
| **P1 C++/Build** | 75% | 🟢 | Module `PUBG`, có `BaseLibrary/DataEnum`, `DataStruct` | Có thể thiếu test module |
| **P2 Core Framework** | 90% | 🟢 | Full Base* + Lobby* | Có thể thiếu HUD class C++ (chỉ có Crosshair) |
| **P3 Composition** | 85% | 🟢 | 6 component folder | Tốt — đáng học theo |
| **P4 Enhanced Input** | 80% | 🟢 | `Component/Input/` + DataAsset_Input | Cần check IMC dynamic switch (BR có driving + foot context) |
| **P5 Animation** | 60% | 🟡 | `AnimInstance/` | Chưa có visibility motion warping cho parachute / vehicle |
| **P6 Replication** | 60% | 🟡 | Mọi Manager + Component có vẻ replicated | Không thấy server-side rewind |
| **P7 Dedicated Server** | 40% | 🟡 | Có Lobby UI, Session browser | Chưa thấy GameLift integration, server target config |
| **P8 GAS** | 70% | 🟢 | `AbilitySystem/Ability`, `AbilityTask`, `AsyncTask`. Có GA cho Airplane_Fall/Look | Chưa rõ damage chain advanced |
| **P9 AI** | 10% | 🔴 | Không thấy folder AI | BR có ít AI (bot fill), nhưng thường có. Có thể OK nếu pure PvP |
| **P10 UI** | 85% | 🟢 | `Widgets/` 8 sub: HUD, Inventory, Lobby, Map, Notification, Result, DisplayMessage | Có thể thiếu polish |
| **P11 Inventory** | 80% | 🟢 | DragDrop, ItemSlot, WeaponSlot, ArmorSlot, GrenadeSlot, WeaponPartsSlot, Inventory Widget | Có thể chưa có item fragment composability |
| **P12 Data-driven** | 70% | 🟢 | `DataAsset/Startup/`, `Weapon/DataTable/`, `Manager/DataTableManager` | Cần check curve table |
| **P13 Save/Load** | 0% | 🔴 | Không có | Có thể chấp nhận cho BR pure (không persist) |
| **P14 AssetMgr/Async** | 30% | 🔴 | DataAsset có | Chưa thấy Primary Asset Manager |
| **P15 Performance** | 30% | 🔴 | Chưa thấy Significance Manager | BR map lớn — bắt buộc cần |
| **P16 Math/Physics** | 60% | 🟡 | Vehicle có | Cần check Chaos Vehicle tuning |
| **P17 Lyra** | 0% | 🔴 | Không dùng | OK |
| **P18 Backend** | 20% | 🔴 | Lobby UI có | Chưa thấy AWS/cloud save/leaderboard |

**Tổng %:**

| Group | % | Weight | Score |
|-------|---|--------|-------|
| Foundation | 83% | 25 | 20.7 |
| Combat | 50% | 25 | 12.5 |
| Content | 67% | 20 | 13.4 |
| Production | 30% | 20 | 6.0 |
| Specialty | 30% | 10 | 3.0 |
| **TOTAL** | | 100 | **~55%** |

> **PUBG-KI base ~55% coverage** — gấp 2.6x Palworld base. Phần lớn gameplay đã có, cần production polish.

---

## 4. Đã làm được gì (positive)

1. **Module decomposition rõ:** 14 sub-folder cho 14 domain — model tốt cho team nhỏ-trung bình.
2. **Manager pattern dùng đúng nơi:** 7 Manager (DataTable, Airplane, Zone, RedZone, SupplyDrop, Landscape, Sound, GameEvent, Time) — đúng cho global subsystem của BR. Nhưng cần convert sang **UWorldSubsystem** thay vì AActor để tránh spawn-then-find pattern.
3. **GAS coverage rộng:** có Ability, AbilityTask, AsyncTask folder — phù hợp với weapon ability driven.
4. **Component composition:** 6 component cho Character — không có god-object kiểu ReadyOrNot.
5. **Weapon system phân lớp:** Guns/Grenade/MeleeWeapon/Projectile với GA + DataTable + Component — pattern tốt.
6. **BR-specific actors đầy đủ:** Airplane + Parachute + Zone + RedZone + SupplyDrop + ItemSpawn (House, DropSpawnVolume, ItemSpawnerComponent). Đây là cốt lõi BR.
7. **Lobby pipeline có UI:** ServerBrowser + CreateGame + FindGame + SessionWidget — đầy đủ matchmaking layer client side.

---

## 5. Cần phát triển thêm

### P0
- **Server-side rewind / Lag compensation** (theo [10] Multiplayer Shooter) — bắt buộc cho FPS competitive.
- **Dedicated server packaging + GameLift fleet** (theo [08]/[13]) — pure listen server không scale BR.
- **Significance Manager** — BR map 8x8km cần culling theo distance.
- **Bot fill** (nếu cần) — AI player khi không đủ người real.
- **Anti-cheat baseline** — RPC validation + replay scrubbing.

### P1
- **Async asset load** cho weapon icon, item description (tránh hitch lúc open inventory).
- **Replay system** — cho death cam và highlight.
- **Performance budget** — frame stat, async line trace.
- **AssetManager primary asset** cho weapon + item.

### P2
- **Spectator HUD** chi tiết.
- **Leaderboard** (DynamoDB).
- **Mod/customization** cho weapon skin.

---

## 6. Cấu trúc — đáp ứng không? Sai phần nào?

### Đáp ứng tốt

| Pattern | Đánh giá |
|---------|----------|
| Public/Private split | ✅ |
| Domain-based folder | ✅ (Character, Component, Weapon, GameplayActor, Manager…) |
| GAS isolation | ✅ folder riêng |
| Widget grouping by use-case | ✅ (HUD, Inventory, Lobby, Map…) |
| GameplayActor sub-domain | ✅ Airplane, Parachute, Zone, RedZone, SupplyDrop, ItemSpawn, Vehicle |

### Sai / thiếu

| Vấn đề | Hậu quả | Khuyến nghị |
|--------|---------|-------------|
| `Manager/` là folder các AActor singleton | Spawn-then-find sẽ chậm và mong manh | Convert sang `UWorldSubsystem` — chuẩn UE5 |
| `BaseLibrary/DataEnum`, `DataStruct` chứa toàn enum/struct toàn module | OK nhưng dễ thành "monster bag of structs" | Split theo domain (WeaponStructs.h, ZoneStructs.h…) |
| Không có `Plugins/` cho Inventory/UI | Khó tái dùng | Sau khi stable, tách thành plugin (theo [09]) |
| Không có `Tests/` | Không TDD được | Thêm `Source/PUBGTests/` |
| Không có `AI/` folder | Không có bot fill | Thêm folder + tích hợp BT/StateTree |
| Không có `Replays/` | Không có death cam | Tích hợp Unreal Replay Subsystem |
| Không có `Performance/` | BR map lớn sẽ lag | Thêm Significance + Pool + ActorPool |
| 2 lớp `WeaponSlotWidget` và `WeaponSlotWidget1` | Naming code-smell — có thể duplicate | Refactor thành 1 base + 1 variant |
| Không có `AbilitySystem/Cues/` | Cue dễ tản mác | Thêm folder cho GameplayCue |
| Vehicle chỉ có `Vehicle_UAZ` | Chỉ 1 type vehicle | Add Buggy/Boat/Motorbike — production cần ≥ 5 |

---

## 7. Ánh xạ tới khóa học

| Gap | Course nên học |
|-----|----------------|
| Lag Compensation | **[10] Multiplayer Shooter (chương 12)** — cốt lõi |
| Dedicated AWS | [08] + [13] |
| Significance Manager | [14] Lyra (chương 4 Performance) + ReadyOrNot study |
| WorldSubsystem migration | UE5 docs + Tom Looman blog |
| Async asset | [01] Tom (AssetManager subclass) |
| Cleanup Manager pattern | [04] ReadyOrNot study (Activity Manager pattern + SignificanceManager pattern) |
| Bot AI | [01] Tom (BT + EQS) + [15] State Tree |

---

## 8. ADR cần ghi

| ADR |
|-----|
| ADR-01: Manager là AActor vs WorldSubsystem — đổi sang Subsystem |
| ADR-02: Listen vs Dedicated — bắt buộc dedicated cho 100-player |
| ADR-03: GameLift Anywhere (test) vs Managed Fleet (prod) |
| ADR-04: Replay subsystem (built-in vs custom) |
| ADR-05: Bot AI — có/không + behavior (BT vs ML) |
| ADR-06: Inventory replication — FastArraySerializer hay direct UPROPERTY |

---

## 9. Kết luận

- **PUBG-KI ~55% coverage** — có thể chơi được prototype, không thể ship production.
- **Cấu trúc tốt**, gần với best-practice — đáng học cho team mới làm BR. Còn vài "code-smell" nhỏ (WeaponSlotWidget1, Manager AActor) cần refactor.
- **Pillar yếu nhất:** P6/P7/P15 (lag compensation, dedicated, performance). 3 pillar này quyết định ship được hay không cho BR.
- **Pillar mạnh nhất:** P2 Core Framework + P3 Composition + P8 GAS + P10 UI. Đáng học pattern composition này cho dự án Palworld.
