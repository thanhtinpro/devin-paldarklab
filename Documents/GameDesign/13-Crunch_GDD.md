# 13 — Multiplayer Unreal with GAS + AWS Dedicated (Crunch) (Donchitos Reverse GDD)

> Source: [github.com/TutLeeUdemy/Crunch](https://github.com/TutLeeUdemy/Crunch) — [Udemy course](https://www.udemy.com/course/multiplayer-in-unreal-with-gas-and-aws-dedicated-servers/).
>
> Sample game: **Crunch** — 3v3 PvP MOBA-style brawler với GAS + AWS dedicated.

---

## Identity Card

| Field | Value |
|-------|-------|
| 📦 Type | Course (advanced — GAS + MP + AWS) |
| Genre | 3v3 PvP brawler (MOBA-ish) |
| Engine | UE5 |
| Đóng góp PALDARK | **Server Coordinator pattern + GAS optimization + Container deploy** |

---

## Phase 1 — Brainstorm (reverse)

### Implicit pitch
> *"3 bạn vs 3. Mỗi nhân vật có 4 ability. AWS server tìm trận. Lobby coordinator. Combo gameplay."*

### Cảm xúc cốt lõi
- **Pro-grade** — không phải hobby, có backend thật.
- **Team play** — không 1 man army.

---

## Phase 2 — Decompose

### Verb cốt lõi
- Login (AWS)
- Find match
- Pick character
- Spawn at base
- Combo ability
- Push lane
- Kill opponent
- Capture point
- Match end → score

### System hierarchy

```
┌────────────────────────────────────┐
│ Coordinator Server (AWS Lambda + DB)│
│ ─────────────────────────────────  │
│ - Auth (Cognito)                   │
│ - Match queue                       │
│ - Server allocation (GameLift)      │
└──────────────┬─────────────────────┘
               │
               ▼
┌────────────────────────────────────┐
│ Dedicated Server (Container)       │
│ ─────────────────────────────────  │
│ - GameMode 3v3                     │
│ - ASC + AS for each player         │
│ - Combo logic                      │
└────────────────────────────────────┘
               ▲
               │ Connect
               │
┌────────────────────────────────────┐
│ Client (UE5 Game)                  │
└────────────────────────────────────┘
```

---

## Phase 3 — Define (GDD reverse)

### Game Loop
1. Login.
2. Find match (3 player vs 3).
3. Pick character (4 selectable).
4. Spawn at base.
5. Cast abilities + combo (chain Q→W→E→R for combo bonus).
6. Push capture point.
7. Match timer + score.
8. End → reward → return lobby.

### Character (4 selectable)
- Each has 4 GA (Q/W/E/R).
- Each has 1 passive.
- Each has 4 stats (Health, Mana, Damage, Speed).

### GAS Optimization
- AS replication mode: Mixed (server validate, owner predict).
- GA cost (mana) checked server.
- Cooldown via GE.

### Combo
- GA chain checks `LastAbilityTag` within window.
- If match → bonus damage GE.

### Server Coordinator
- Lambda function pool servers.
- Track open slots.
- Allocate session khi 6 player ready.
- Send endpoint to clients.

### Container Deploy
- Server target packaged → Docker.
- Pushed to ECR.
- GameLift Fleet runs containers.

---

## Phase 4 — 3 Game Pillars (inferred)

### Pillar 1 — Combo is Identity
> *"Mỗi character mạnh ở combo nhất định."*

### Pillar 2 — Server is Referee
> *"Damage server validate. Owner predict cho responsive feel."*

### Pillar 3 — Match Starts Before Match
> *"Coordinator quyết bạn match với ai → game fair từ trước khi cast spell."*

---

## Phase 5 — Architecture Decisions (implicit)

### ADR-1: Coordinator pattern
- **Quyết định:** Tách Lambda Coordinator khỏi GameMode.
- **Lý do:** Lobby state khác Game state. Separation of concerns.
- **PALDARK adopt:** 🟢 — `UPaldarkMatchCoordinatorSubsystem` qua AWS.

### ADR-2: Container deploy GameLift
- **Quyết định:** Container thay vì native binary.
- **Lý do:** Multi-region, easy deploy.
- **PALDARK adopt:** 🟢.

### ADR-3: GAS replication mode Mixed
- **Quyết định:** Mixed = server validate + owner predict.
- **PALDARK adopt:** 🟢.

### ADR-4: Cooldown via GameplayEffect (không native timer)
- **Quyết định:** Cooldown là GE with Duration.
- **PALDARK adopt:** 🟢.

### ADR-5: Combo via Tag chain
- **Quyết định:** Last ability tag stored + window timer.
- **PALDARK adopt:** 🟡 Adapt — PALDARK Pal cast combo (Player → Pal command → Pal cast).

---

## Phase 6 — System Map

| System | Class | File |
|--------|-------|------|
| Coordinator | Lambda function (Python/JS) | `Backend/Lambda/` |
| Auth | `UCRAuthSubsystem` | `Online/...` |
| Char | `ACrunchCharacter` | `Character/...` |
| ASC | `UCrunchAbilitySystemComponent` | `GAS/...` |
| AS | `UCrunchAttributeSet` | `GAS/...` |
| Combo Manager | `UComboManager` | `Combat/...` |
| Container | Dockerfile | `Server/Dockerfile` |

---

## Phase 7 — Stories taught

| Phần | US |
|------|----|
| 1 | Là dev, tôi setup project + GAS |
| 2 | Là dev, tôi viết character với 4 GA (Q/W/E/R) |
| 3 | Là dev, tôi viết AS với 4 stat |
| 4 | Là dev, tôi setup combo detection (last tag chain) |
| 5 | Là dev, tôi setup AWS Cognito Auth |
| 6 | Là dev, tôi viết Coordinator Lambda |
| 7 | Là dev, tôi setup GameLift Anywhere → Fleet |
| 8 | Là dev, tôi containerize Server target |
| 9 | Là dev, tôi setup match queue + allocate |
| 10 | Là dev, tôi connect-to-server flow |
| 11 | Là dev, tôi optimize GAS net update |
| 12 | Là dev, tôi setup Score + Match End + reward |

---

## Take-aways cho PALDARK

### 🟢 Copy nguyên
- **Coordinator pattern** (Lambda dispatch session).
- **Container deploy** GameLift Fleet.
- **GAS replication mode Mixed**.
- **Cooldown via GE**.
- **Combo Tag chain** (apply Pal combo).

---

## Pillar Mapping

| Pillar | Đóng góp |
|--------|----------|
| P7 | 🟢 **Coordinator + Container deploy** |
| P8 | 🟢 **GAS production MP + optimize** |
| P18 | 🟢 **AWS dispatcher** |
| P6 | 🟢 GAS replication Mixed mode |

---

## Tham chiếu

- [Courses/13-Udemy-ue5-multiplayer-in-unreal-with-gas-and-aws-dedicated-servers.md](../Courses/13-Udemy-ue5-multiplayer-in-unreal-with-gas-and-aws-dedicated-servers.md)
- [08-AWS_GameLift_GDD.md](08-AWS_GameLift_GDD.md) — basics.
- [11-Aura_RPG_GDD.md](11-Aura_RPG_GDD.md) — GAS deep.
