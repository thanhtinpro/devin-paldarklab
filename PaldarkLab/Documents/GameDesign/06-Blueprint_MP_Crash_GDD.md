# 06 — Blueprint Multiplayer Crash Course (Donchitos Reverse GDD)

> Source: [Udemy — UE5 Blueprint Multiplayer Crash Course](https://www.udemy.com/course/ue5-blueprint-multiplayer-crash-course/).
>
> Sample game: **party arena multiplayer** — lobby + map travel + listen server + Steam.

---

## Identity Card

| Field | Value |
|-------|-------|
| 📦 Type | Course (Blueprint-only) |
| Genre | Party arena multiplayer |
| Engine | UE5 |
| Đóng góp PALDARK | Multiplayer khái niệm cơ bản — listen vs dedicated, session, travel |

---

## Phase 1 — Brainstorm (reverse)

### Implicit pitch
> *"Bạn host một party. Bạn bè join qua Steam. Cùng vào map, chạy quanh, làm gì cũng được."*

### Cảm xúc cốt lõi
- **Tụ tập** — không phải competitive, đơn giản là cùng map.
- **Quen tay** với MP backbone trước khi thêm gameplay.

---

## Phase 2 — Decompose

### Verb cốt lõi
- Host (listen server)
- Browse lobby
- Join
- Travel to map
- Walk around

### System hierarchy

```
┌───────────────┐
│ GameInstance  │ Session manage
│ Subsystem     │
└──────┬────────┘
       │
   ┌───┴────────┬──────────┐
   ▼            ▼          ▼
GameMode    PlayerCtrl  HUD/Widget
(Lobby)     Travel      Browse
GameMode    PossessChar Server List
(InGame)
```

---

## Phase 3 — Define (GDD reverse)

### Game Loop
1. Open game → main menu.
2. Host (Create Session) hoặc Join (Find Session).
3. Travel: Lobby → CountDown → Game map.
4. Walk around with friends.

### Character
- Default UE Character.
- Walking + jump.
- Replicated transform.

### Lobby
- Host displays "Waiting for 2/4..." with player list.
- Start button (host-only).

### Travel
- `ServerTravel` qua URL với listen mode.
- Steam OnlineSubsystem for connection.

---

## Phase 4 — 3 Game Pillars (inferred)

### Pillar 1 — Connection is Gameplay
> *"Gặp nhau = gameplay. Server tốt = trải nghiệm tốt."*

### Pillar 2 — Listen vs Dedicated Matters
> *"Listen = bạn host. Dedicated = bên thứ 3 host. Authority cùng nơi."*

### Pillar 3 — UI is the Lobby
> *"Trước khi chơi là UI. UI dở = không ai chơi nữa."*

---

## Phase 5 — Architecture Decisions (implicit)

### ADR-1: Blueprint-only (không C++)
- **Quyết định:** Toàn Blueprint cho dạy nhanh.
- **PALDARK adopt:** 🔴 Skip — PALDARK C++ hết.

### ADR-2: Listen server (không dedicated)
- **Quyết định:** Sample dùng listen server.
- **PALDARK adopt:** 🔴 Skip — PALDARK dedicated only.

### ADR-3: Session qua OnlineSubsystem Steam
- **Quyết định:** Dùng OSS Steam plugin.
- **PALDARK adopt:** 🟡 Tham chiếu — PALDARK dùng EOS hoặc AWS GameLift matchmaker.

### ADR-4: ServerTravel cho map change
- **Quyết định:** Travel qua URL.
- **PALDARK adopt:** 🟢 — Travel Hub → Raid Map.

---

## Phase 6 — System Map

| System | Class | File |
|--------|-------|------|
| GameInstance | `BP_GameInstance` | BP |
| Session Subsys | UE built-in | C++ engine |
| MainMenu Widget | `WBP_MainMenu` | BP |
| Lobby GameMode | `BP_LobbyGameMode` | BP |
| InGame GameMode | `BP_InGameGameMode` | BP |
| Char | `BP_Character` | BP |

---

## Phase 7 — Stories taught

| Chapter | US |
|---------|----|
| 1 | Là dev, tôi setup project + enable OSS Steam |
| 2 | Là dev, tôi viết MainMenu widget |
| 3 | Là dev, tôi tạo Session (host) qua subsystem |
| 4 | Là dev, tôi find session + populate list |
| 5 | Là dev, tôi join session |
| 6 | Là dev, tôi ServerTravel sang Lobby map |
| 7 | Là dev, tôi count player + start button |
| 8 | Là dev, tôi travel sang InGame map |
| 9 | Là dev, tôi setup replicated character |

---

## Take-aways cho PALDARK

### 🟢 Copy nguyên
- **Concept Lobby → Travel → InGame** — PALDARK Hub → Brief → Raid (ServerTravel pattern).
- **Host/Join UI pattern** (tham chiếu cho Hub).

### 🟡 Adapt
- **Listen server** → PALDARK dedicated (AWS).
- **Steam OSS** → PALDARK EOS hoặc GameLift matchmaker.

### 🔴 Avoid
- **Blueprint-only** → PALDARK C++.

---

## Pillar Mapping

| Pillar | Đóng góp |
|--------|----------|
| P2 | 🟠 GameMode/Instance pattern |
| P6 | 🟠 Replication basic |
| P7 | 🟠 Session + Travel |
| P10 | 🟠 Lobby UI |

---

## Tham chiếu

- [Courses/06-Udemy-ue5-blueprint-multiplayer-crash-course.md](../Courses/06-Udemy-ue5-blueprint-multiplayer-crash-course.md)
- [07-CPP_MP_Crash_GDD.md](07-CPP_MP_Crash_GDD.md) — phiên bản C++.
