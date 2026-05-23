# 07 — C++ Multiplayer Crash Course (Donchitos Reverse GDD)

> Source: [Udemy — UE5 C++ Multiplayer Crash Course](https://www.udemy.com/course/ue5-multiplayer-crash-course/).
>
> Sample game: similar to [06] nhưng C++ thay vì BP. Dạy theory replication + RPC.

---

## Identity Card

| Field | Value |
|-------|-------|
| 📦 Type | Course (C++) |
| Genre | Multiplayer foundational (theory > game) |
| Engine | UE5 |
| Đóng góp PALDARK | **Authority model + RPC pattern + Net Role** — nền tảng lý thuyết MP |

---

## Phase 1 — Brainstorm (reverse)

### Implicit pitch
> *"Hiểu Authority. Hiểu RepNotify. Hiểu Server/Client/Multicast. Sau khoá này bạn không sợ bug replication."*

### Cảm xúc cốt lõi
- **Clarity** — không còn mơ hồ về "ai sở hữu gì".
- **Power** — biết viết MP đúng từ đầu.

---

## Phase 2 — Decompose

### Verb cốt lõi
- Spawn (server-only)
- Replicate variable (DOREPLIFETIME)
- RPC server/client/multicast
- RepNotify
- Owner detection

### System hierarchy

```
┌──────────────────┐
│ GameMode (server)│
│ GameState (rep)  │
└────────┬─────────┘
         │
   ┌─────┴───────┬─────────┐
   ▼             ▼         ▼
PlayerCtrl   PlayerState   Pawn
 (per client) (per player) (in world)
RPC target   rep stats     rep transform
```

---

## Phase 3 — Define (GDD reverse)

### Game Loop
- Như [06] — Lobby → Travel → InGame walking around.
- Khác biệt: code C++, theory rõ ràng từng concept.

### Key Concepts

#### Authority
- `HasAuthority()` = server.
- `IsLocallyControlled()` = client own.
- `GetNetMode()` = NM_DedicatedServer / NM_ListenServer / NM_Client.

#### Replication
- `DOREPLIFETIME(Class, Var)` cho mọi variable replicate.
- `DOREPLIFETIME_CONDITION(...)` cho conditional.
- `RepNotify` callback khi value đổi client-side.

#### RPC
- `Server` RPC (client → server) — phải `WithValidation`.
- `Client` RPC (server → owning client only).
- `Multicast` (server → all clients).

#### Bandwidth
- `NetUpdateFrequency` (Hz/s update).
- `NetCullDistanceSquared` (cull beyond).

---

## Phase 4 — 3 Game Pillars (inferred)

### Pillar 1 — Authority is Sacred
> *"Server quyết. Client suggest. Không bao giờ trust client."*

### Pillar 2 — Replication is Choice
> *"Mọi rep cost bandwidth. Lười rep = lag. Quá rep = waste."*

### Pillar 3 — RPC is Intent
> *"Mỗi RPC là 1 intent rõ. Không spam, không server-multicast vô tội vạ."*

---

## Phase 5 — Architecture Decisions (implicit)

### ADR-1: PlayerState là nguồn truth player data
- **Quyết định:** Stats trên PlayerState, replicate cho all.
- **PALDARK adopt:** 🟢 — kill/loot stats trên PlayerState.

### ADR-2: PlayerController owner-only RPC
- **Quyết định:** Client-only logic ở PlayerController.
- **PALDARK adopt:** 🟢.

### ADR-3: GameState match-wide state
- **Quyết định:** Score, phase, time trên GameState.
- **PALDARK adopt:** 🟢 — Raid phase, extract beacon state.

---

## Phase 6 — System Map

| System | Class | File |
|--------|-------|------|
| GameMode | `AMyGameMode` | C++ |
| GameState | `AMyGameState` | C++ |
| PlayerController | `AMyPlayerController` | C++ |
| PlayerState | `AMyPlayerState` | C++ |
| Pawn | `AMyPawn` | C++ |

---

## Phase 7 — Stories taught

| Chapter | US |
|---------|----|
| 1 | Là dev, tôi setup Server target + listen launch |
| 2 | Là dev, tôi viết Replicated variable + DOREPLIFETIME |
| 3 | Là dev, tôi viết RepNotify callback |
| 4 | Là dev, tôi viết Server RPC + WithValidation |
| 5 | Là dev, tôi viết Client RPC |
| 6 | Là dev, tôi viết Multicast |
| 7 | Là dev, tôi check `HasAuthority()` cho server-only code |
| 8 | Là dev, tôi cull replicate qua NetCullDistance |
| 9 | Là dev, tôi setup GameMode/State/Player split |

---

## Take-aways cho PALDARK

### 🟢 Copy nguyên (CORE LÝ THUYẾT)
- **Authority model** — mọi spawn/damage/loot server-only.
- **PlayerState là truth player data**.
- **PlayerController per-client owner**.
- **GameState match-wide**.
- **RPC pattern** (server validated).
- **NetUpdateFrequency tuning** (Pal 30Hz vs Player 60Hz).

---

## Pillar Mapping

| Pillar | Đóng góp |
|--------|----------|
| P2 | 🟢 **Core Framework** |
| P6 | 🟢 **Replication theory** |
| P7 | 🟠 Listen vs dedicated khái niệm |

---

## Tham chiếu

- [Courses/07-Udemy-ue5-multiplayer-crash-course.md](../Courses/07-Udemy-ue5-multiplayer-crash-course.md)
- [10-MP_Shooter_GDD.md](10-MP_Shooter_GDD.md) — apply theory này vào lag comp.
