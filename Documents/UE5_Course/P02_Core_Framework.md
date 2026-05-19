# P02 — Core Class Framework

> Tự soạn từ: [07] CPP MP Crash, [01] ActionRoguelike, [11] Aura, [13] Crunch, [04] ReadyOrNot.
>
> Pillar quyết "code này thuộc về class nào". Đặt sai class → MP bug, save bug, life-cycle bug.

---

## Pillar Identity

**Cái gì:** Biết khi nào dùng GameMode / GameState / PlayerController / PlayerState / Pawn / Character / HUD / GameInstance / Subsystem.

**Scope:**
- ✅ Phân biệt authority: server-only vs client-only vs replicated.
- ✅ Lifetime: per-match vs per-player vs cross-level vs cross-session.
- ✅ Network role: NetRole + RemoteRole, Authority/Autonomous/Simulated.
- ✅ Replication direction: server→client vs client→server (RPC).
- ✅ Subsystem variants: GameInstance / World / LocalPlayer / Engine / Editor.

**Out of scope:**
- ❌ Replication detail (P06).
- ❌ Composition vs inheritance (P03).
- ❌ Backend session lookup (P07).

---

## PALDARK cần gì từ pillar này?

| Class | Vai trò ở PALDARK |
|-------|-------------------|
| `APaldarkGameMode_Raid` | Server-only luật trận Dark Zone (24-min cycle, extract, fail). |
| `APaldarkGameState_Raid` | Replicated match data (timer, fog level, civilian count). |
| `APaldarkPlayerController` | Owner-only RPC input + UI flow. |
| `APaldarkPlayerState` | Replicated player progress + ASC mount point. |
| `APaldarkCharacter` | World presence — animation, movement, combat. |
| `APaldarkHUD` | Owning client — draw HUD widget root. |
| `UPaldarkGameInstance` | Xuyên-level — Coordinator client, save, audio bus. |
| `UPaldarkMatchSubsystem` (WorldSub) | Per-world manager — extract beacon, fog timer. |
| `UPaldarkAudioSubsystem` (GameInstance Sub) | Cross-level FMOD bus. |

---

## Sources Matrix

| Source | Coverage | Lesson lấy từ đó |
|--------|----------|------------------|
| [07] CPP MP Crash | 🟢 | L1 9-class table, L2 Authority/Role, L3 Lifetime |
| [01] ActionRoguelike | 🟡 | L4 Single-player application |
| [10] Blaster | 🟢 | L5 PvP setup |
| [11] Aura | 🟢 | L6 ASC on PlayerState |
| [04] RoN | 🟡 | L7 Subsystem manager 16 actor — anti-pattern note |

---

## Prerequisite

- P01 (C++ basics, UPROPERTY).

---

## Lessons

### L1 — Bảng 9 class chính 📖

**Goal:** Nhớ vai trò + lifetime + authority của 9 class.

**Concept:**

| Class | Authority | Lifetime | Replicate | Use case |
|-------|-----------|----------|-----------|----------|
| `AGameModeBase` | Server only | Per-map | ❌ (server) | Luật chơi, spawn |
| `AGameStateBase` | Server set, all see | Per-map | ✅ All | Match data shared |
| `APlayerController` | Server + Owner | Per-player per-map | Owner-only | Input + UI flow |
| `APlayerState` | Server set, all see | Per-player (persist seamless travel) | ✅ All | Score, name, ASC mount |
| `APawn`/`ACharacter` | Server | Per-instance | ✅ All | World presence |
| `AHUD` | Owning client | Per-player per-map | ❌ Local | Canvas draw + widget root |
| `UGameInstance` | All | Per-game-process | ❌ Local | Singleton xuyên-level |
| `UGameInstanceSubsystem` | All | Per-game-process | ❌ Local | Singleton lifetime GI |
| `UWorldSubsystem` | All | Per-world | ❌ Local | Singleton lifetime World |
| `ULocalPlayerSubsystem` | Owner | Per-local-player | ❌ Local | Per-player singleton |

**Source learning path:**
- [07] CPP MP Crash § Sec 5 — bảng đối chiếu Authority.

**PALDARK take-away:**
- 🟢 In hằng dán cạnh bàn — refer hằng ngày khi viết class mới.

**Apply ở week:** Roadmap week 3.

---

### L2 — Authority + NetRole 📖🧪

**Goal:** Biết `HasAuthority()`, `IsLocallyControlled()`, `GetLocalRole()` vs `GetRemoteRole()`.

**Concept:**
- `HasAuthority()` = "server hoặc standalone".
- `IsLocallyControlled()` = client của my pawn (owning).
- `GetLocalRole()`:
  - `ROLE_Authority` — server hoặc standalone.
  - `ROLE_AutonomousProxy` — client của owning pawn (predict).
  - `ROLE_SimulatedProxy` — client của pawn không phải tôi.
- `bReplicates = true` ở constructor để pawn replicate.

**Source learning path:**
- [07] CPP MP § Sec 4 — Role examples.
- [10] Blaster — `BlasterCharacter` setup `bReplicates`.

**Practice exercise (1 hour):**
1. Tạo `APaldarkLabPawn` print mỗi tick `Role` + `RemoteRole`.
2. PIE 2 player → so sánh log server vs client.
3. Add `if (HasAuthority())` block — chỉ thấy log ở server.

**PALDARK take-away:**
- 🟢 Mọi server-only logic wrap `if (HasAuthority())`.
- 🟢 Mọi client-only UI wrap `if (IsLocallyControlled())`.

**Apply ở week:** Roadmap week 3 — mọi class MP.

---

### L3 — Lifetime + Seamless Travel 📖

**Goal:** Hiểu class nào survive travel, class nào reset.

**Concept:**
- **Seamless Travel** (default cho `ServerTravel?listen`):
  - GameMode RESET.
  - GameState RESET.
  - PlayerController KEEP.
  - PlayerState KEEP (carry-over).
  - Pawn RESET (respawn).
  - GameInstance KEEP.
- **Non-seamless Travel** (`ServerTravel?dedicated`):
  - PlayerController RESET (reconnect).
  - PlayerState RESET.

**Source learning path:**
- [06] BP MP Crash § Lobby → InGame travel.
- [07] CPP MP § Travel chapter.

**Practice exercise:**
1. Setup 2 map: `Lobby` + `InGame`.
2. PIE listen → travel Lobby → InGame.
3. PlayerState print Score = 5 ở Lobby, check sau travel còn 5 không.

**PALDARK take-away:**
- 🟢 PALDARK PlayerState carry progress qua match (Hub → Raid → Hub).
- 🟢 Persistent inventory (item) lưu ở backend, không phụ thuộc travel.

**Apply ở week:** Roadmap week 11 (Hub → Raid travel flow).

---

### L4 — Single-Player application (ActionRoguelike) 🧪

**Goal:** Apply 9-class cho single-player game đơn giản.

**Source learning path:**
- [01] ActionRoguelike — `ASGameModeBase` (single-player, dùng GameModeBase đủ).
- Cross-ref: `Documents/GameDesign/01-ActionRoguelike_GDD.md` § Phase 6.

**Pattern:**
- `ASCharacter` — Pawn.
- `ASPlayerState` — Score.
- `ASGameModeBase` — spawn bot timer.
- `ASGameStateBase` — không cần extend.
- `ASHUD` — không cần extend (UMG đủ).

**PALDARK take-away:**
- 🟡 PALDARK không single-player, nhưng `PaldarkLab` test scene có thể single → dùng pattern này.

**Apply ở week:** Roadmap week 4 (test scene single).

---

### L5 — PvP / Co-op application (Blaster) 🧪

**Goal:** Apply 9-class cho MP shooter.

**Source learning path:**
- [10] Blaster — full 9-class stack.
- Cross-ref: `Documents/GameDesign/10-MP_Shooter_GDD.md` § Phase 6.

**Pattern:**
- `ABlasterCharacter` — Pawn replicated, Combat + LagComp component.
- `ABlasterPlayerController` — Owner-only RPC + HUD show.
- `ABlasterPlayerState` — Score + DeathCount replicated.
- `ABlasterGameMode` — Server respawn + match end.
- `ABlasterGameState` — Match timer + winning team.

**PALDARK take-away:**
- 🟢 PALDARK Raid GameMode adopt nguyên pattern: spawn → match timer → end condition (extract or wipe).
- 🟢 PlayerController owner-only RPC cho input + Pal command.

**Apply ở week:** Roadmap week 11-13 (Raid match flow).

---

### L6 — ASC trên PlayerState (Aura) 📖

**Goal:** Hiểu vì sao ASC nên ở PlayerState chứ không Pawn.

**Concept:**
- Pawn respawn → ASC mới → mất buff/cooldown.
- PlayerState persist seamless travel → ASC giữ nguyên buff/level.
- Trade-off: Pawn-based ASC nhanh (như AI), không cần network RTT.
- → **AI Pal dùng ASC trên Pawn**; **Player dùng ASC trên PlayerState**.

**Source learning path:**
- [11] Aura § GAS Setup chapter.
- Cross-ref: `Documents/GameDesign/11-Aura_RPG_GDD.md` ADR-1.

**Practice exercise:**
1. Tạo `APaldarkLabPlayerState` thừa kế `APlayerState`, add `UPaldarkLabAbilitySystemComponent`.
2. Override `IAbilitySystemInterface::GetAbilitySystemComponent()` ở PlayerState.
3. Pawn forward `GetAbilitySystemComponent()` về PlayerState.

**PALDARK take-away:**
- 🟢 `APaldarkPlayerState` own ASC; `APaldarkPalCharacter` own ASC.

**Apply ở week:** Roadmap week 6 (GAS first cast).

---

### L7 — Subsystem patterns 📖🧪

**Goal:** Chọn đúng Subsystem variant cho từng nhu cầu.

**Concept:**

| Subsystem | Lifetime | Use case |
|-----------|----------|----------|
| `UEngineSubsystem` | Editor + game | Engine plugin |
| `UEditorSubsystem` | Editor only | Editor tool |
| `UGameInstanceSubsystem` | Per-game-process | Audio bus, backend session, settings |
| `UWorldSubsystem` | Per-world | Match manager, extract beacon, fog timer |
| `ULocalPlayerSubsystem` | Per-local-player | Save slot, input config, preference |

**Source learning path:**
- [04] RoN GDD § 16 Manager — pattern nhưng có debt (AActor singleton).
- [14] Exploring Lyra — Indicator Manager là `UWorldSubsystem`.

**Practice exercise:**
1. Tạo `UPaldarkLabMatchSubsystem : public UWorldSubsystem`.
2. Override `Initialize` log "World started".
3. Override `Deinitialize` log "World ended".
4. PIE → travel map → confirm Initialize chạy lại.

**PALDARK take-away:**
- 🟢 5 Subsystem từ đầu:
  - `UPaldarkMatchSubsystem` (WorldSub) — extract beacon + timer.
  - `UPaldarkPalRosterSubsystem` (GameInstanceSub) — tracking Pal team xuyên match.
  - `UPaldarkAudioSubsystem` (GameInstanceSub) — FMOD bus.
  - `UPaldarkInputSubsystem` (LocalPlayerSub) — input config.
  - `UPaldarkBackendSubsystem` (GameInstanceSub) — Coordinator + DynamoDB.
- 🔴 KHÔNG dùng `AActor singleton` như PUBG/RoN — debt.

**Apply ở week:** Roadmap week 2 (setup 5 subsystem skeleton).

---

## ⚡ Capstone Exercise

**Goal:** Build `PaldarkLab` MP scaffold với toàn bộ 9-class + 5 subsystem.

**Yêu cầu:**
1. Tạo 9 class custom: `APaldarkLabGameMode`, `APaldarkLabGameState`, `APaldarkLabPC`, `APaldarkLabPS`, `APaldarkLabCharacter`, `APaldarkLabHUD`, `UPaldarkLabGameInstance`.
2. Tạo 5 subsystem skeleton (Initialize chỉ log).
3. Setup PIE listen 2 player.
4. PlayerState carry score 5 qua travel.
5. Log Authority + Role mỗi BeginPlay.

**Acceptance:** 2 player join, score carry, log đúng role server vs client.

**Effort:** 3 ngày.

---

## Khi apply vào PALDARK roadmap

| Week | Pillar P02 usage |
|------|------------------|
| W2 | 5 Subsystem skeleton |
| W3 | 9-class skeleton — Pawn + PC + PS + GameMode |
| W6 | ASC trên PlayerState |
| W11 | Hub → Raid travel (PlayerState carry) |
| W11-13 | Raid GameMode pattern (Blaster) |

---

## Anti-patterns (cảnh báo)

- 🔴 ASC trên Pawn (cho Player) → mất buff khi respawn.
- 🔴 Match logic trong GameInstance → reset không đúng giữa match.
- 🔴 Singleton manager `AActor` (PUBG/RoN pattern) — dùng Subsystem thay vì.
- 🔴 Quên `bReplicates = true` cho Pawn → client thấy ma.
- 🔴 GameMode replicate biến cho client — GameMode server-only, dùng GameState.

---

## Tham chiếu

- [`Documents/UE5_Core_Pillars.md`](../UE5_Core_Pillars.md) § P2.
- [`Documents/Courses/07-Udemy-ue5-multiplayer-crash-course.md`](../Courses/07-Udemy-ue5-multiplayer-crash-course.md).
- [`Documents/GameDesign/07-CPP_MP_Crash_GDD.md`](../GameDesign/07-CPP_MP_Crash_GDD.md).
- [`Documents/GameDesign/10-MP_Shooter_GDD.md`](../GameDesign/10-MP_Shooter_GDD.md).
- [`Documents/PALDARK/01-GameDesign.md`](../PALDARK/01-GameDesign.md) — class hierarchy.
