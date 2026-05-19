# 08 — Dedicated Servers with AWS and GameLift (Donchitos Reverse GDD)

> Source: [github.com/DruidMech/DedicatedServers](https://github.com/DruidMech/DedicatedServers) — Stephen Ulibarri.
>
> Sample game: **PvP arena + AWS backend stack** — login, matchmaking, dedicated, leaderboard.

---

## Identity Card

| Field | Value |
|-------|-------|
| 📦 Type | Course (backend-heavy) |
| Genre | PvP arena + Live Ops stack |
| Engine | UE5 |
| AWS services | Cognito, Lambda, API Gateway, GameLift Anywhere/Fleet, DynamoDB, S3 |
| Đóng góp PALDARK | **AWS production playbook** — login + match + dedicated + leaderboard |

---

## Phase 1 — Brainstorm (reverse)

### Implicit pitch
> *"Bạn build game backend như startup thật. Player đăng nhập, matchmaker tìm trận, fleet spawn server, score lưu DB."*

### Cảm xúc cốt lõi
- **Pro level** — không phải hobby project nữa.
- **Confidence** — game của bạn deploy được production.

---

## Phase 2 — Decompose

### Verb cốt lõi
- Sign up (Cognito user pool)
- Verify email
- Sign in → JWT token
- Find match (Matchmaker)
- Get server endpoint
- Connect to dedicated
- Play match
- Upload score → leaderboard

### System hierarchy

```
┌────────────────────────────────────────┐
│ Client (UE5 game)                       │
└─────┬──────────────────────────────────┘
      │
      ▼
┌──────────────────┐
│ API Gateway      │  HTTP REST
└──────┬───────────┘
       │
   ┌───┴────┬─────────┬──────────┐
   ▼        ▼         ▼          ▼
Cognito  Lambda    Lambda     GameLift
 Pool    (Match)   (Score)    (Fleet)
   │        │         │          │
   └────────┴────────┬┴──────────┘
                    ▼
                DynamoDB
                S3
```

---

## Phase 3 — Define (GDD reverse)

### Login Flow
1. UI input email/password.
2. Cognito Pool sign up.
3. Cognito email code → verify.
4. Sign in → JWT (access + id + refresh).
5. Token stored client-side.

### Match Flow
1. Player click "Find Match".
2. HTTP POST to API Gateway with token.
3. Lambda validates token, queries GameLift.
4. GameLift Matchmaker waits N seconds.
5. Find suitable opponents → place session.
6. Return server endpoint to client.
7. Client `Open` URL → connect dedicated.

### Match Game
- Simple PvP arena (2–4 players).
- Damage + win condition.
- Match end → score upload.

### Leaderboard
- Score → API Gateway → Lambda → DynamoDB.
- Query top 10 → display.

### Dedicated Server
- Compile Server target.
- Package + upload to GameLift (or Anywhere Fleet).
- Auto-scale.

---

## Phase 4 — 3 Game Pillars (inferred)

### Pillar 1 — Identity Precedes Play
> *"Trước khi chơi là biết bạn là ai. Account = ownership."*

### Pillar 2 — Match Before Mayhem
> *"Matchmaker quyết experience. Fair match > nhanh match."*

### Pillar 3 — Persistence is Product
> *"Game không kết thúc khi quit. Score tồn tại."*

---

## Phase 5 — Architecture Decisions (implicit)

### ADR-1: Cognito cho identity
- **Quyết định:** Cognito Pool + Identity Pool.
- **Lý do:** Managed, có MFA + social login.
- **Hệ quả:** Vendor lock-in mild — nhưng tiết kiệm 1 team backend.
- **PALDARK adopt:** 🟢 — `UPaldarkAuthSubsystem` wrap Cognito.

### ADR-2: API Gateway + Lambda (serverless)
- **Quyết định:** REST API qua Lambda.
- **Lý do:** Pay-per-request, không server idle.
- **Hệ quả:** Cold start latency.
- **PALDARK adopt:** 🟢.

### ADR-3: GameLift Anywhere Fleet (dev) → Fleet (prod)
- **Quyết định:** Dev với Anywhere Fleet (chạy local), Prod với Fleet EC2.
- **Lý do:** Anywhere cho dev iteration, Fleet cho auto-scale prod.
- **PALDARK adopt:** 🟢.

### ADR-4: DynamoDB cho hot data
- **Quyết định:** Key-value đơn giản, low-latency.
- **Lý do:** Scale tự động, no schema migrations đau đầu.
- **PALDARK adopt:** 🟢 — Pal inventory + player progress.

### ADR-5: JWT cho session token
- **Quyết định:** Client gửi token mỗi HTTP call.
- **PALDARK adopt:** 🟢.

### ADR-6: HTTP REST chứ không gRPC
- **Quyết định:** REST đơn giản, debug dễ.
- **PALDARK adopt:** 🟢.

---

## Phase 6 — System Map

| System | Class | File |
|--------|-------|------|
| Auth Subsys | `UDSAuthSubsystem` | `DedicatedServers/.../AuthSubsystem.h/cpp` |
| Match Subsys | `UDSMatchSubsystem` | `MatchSubsystem.h/cpp` |
| HTTP Manager | `UDSHttpManager` | `HttpManager.h/cpp` |
| Login UI | `UDSLoginWidget` | `UI/...` |
| Server Game | `ADSGameMode` (server target) | `Server/...` |

---

## Phase 7 — Stories taught

| Chapter | US |
|---------|----|
| 1 | Là dev, tôi setup AWS account + IAM role |
| 2 | Là dev, tôi setup Cognito User Pool |
| 3 | Là dev, tôi viết Login UI + HTTP call signup |
| 4 | Là dev, tôi handle email verify + sign in |
| 5 | Là dev, tôi setup Lambda + API Gateway |
| 6 | Là dev, tôi setup GameLift Anywhere Fleet (dev) |
| 7 | Là dev, tôi viết Matchmaker config + Lambda invoke |
| 8 | Là dev, tôi viết Connect-to-Server flow |
| 9 | Là dev, tôi compile + upload server build |
| 10 | Là dev, tôi setup DynamoDB Score table |
| 11 | Là dev, tôi viết Score Upload Lambda |
| 12 | Là dev, tôi setup Leaderboard UI |
| 13 | Là dev, tôi setup Fleet (production) + auto-scale |

---

## Take-aways cho PALDARK

### 🟢 Copy nguyên (Backend Playbook)
- **Cognito Pool + Identity Pool** cho login.
- **JWT token** client-side.
- **API Gateway + Lambda** cho match + score + persistence.
- **GameLift Anywhere → Fleet** pipeline (dev → prod).
- **DynamoDB** cho Pal/Inventory persistent.
- **HTTP REST** không gRPC.

---

## Pillar Mapping

| Pillar | Đóng góp |
|--------|----------|
| P7 | 🟢 **Dedicated + Matchmaker** |
| P18 | 🟢 **AWS stack hoàn chỉnh** |
| P10 | 🟠 Login UI pattern |

---

## Tham chiếu

- [Courses/08-Udemy-ue5-dedicated-servers-with-aws-and-gamelift.md](../Courses/08-Udemy-ue5-dedicated-servers-with-aws-and-gamelift.md)
- [13-Crunch_GDD.md](13-Crunch_GDD.md) — apply backend này + GAS.
- [PALDARK/03-Roadmap_1_Year.md](../PALDARK/03-Roadmap_1_Year.md) Q4 — AWS Backend setup tuần 40–44.
