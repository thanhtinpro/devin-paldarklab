# P07 — Dedicated Server + Sessions

> Tự soạn từ: [07] CPP MP Crash, [08] AWS GameLift, [13] Crunch, [06] BP MP Crash.
>
> PALDARK là 4-player co-op extraction — dedicated server bắt buộc (không listen).

---

## Pillar Identity

**Cái gì:** Package Server target, deploy, session management, matchmaking, listen vs dedicated.

**Scope:**
- ✅ Server Target packaging.
- ✅ Listen vs Dedicated trade-off.
- ✅ GameLift Anywhere → Fleet pipeline.
- ✅ Container deploy (Docker + ECR + ECS).
- ✅ Server Coordinator pattern (Lambda dispatch).
- ✅ Session: Create/Find/Join/Destroy.
- ✅ ServerTravel + SeamlessTravel.

**Out of scope:**
- ❌ AWS Lambda/Cognito detail (P18).
- ❌ Replication (P06).

---

## Sources Matrix

| Source | Coverage | Lesson lấy từ đó |
|--------|----------|------------------|
| [07] CPP MP Crash | 🟡 | L1 Listen vs Dedicated, L2 Travel |
| [06] BP MP Crash | 🟡 | L3 Steam session (concept) |
| [08] AWS GameLift | 🟢 | L4 Anywhere Fleet, L5 Server SDK |
| [13] Crunch | 🟢 | L6 Container deploy, L7 Coordinator |

---

## Prerequisite

- P02 (GameMode server-only).
- P06 (Authority + replication basics).

---

## Lessons

### L1 — Listen vs Dedicated trade-off 📖

**Goal:** Biết khi nào dùng gì, vì sao PALDARK cần dedicated.

**Concept:**

| | Listen Server | Dedicated Server |
|-|--------------|-----------------|
| Host | 1 player là server + client | Server riêng, không UI |
| Fair | Host có 0ms ping advantage | Equal ping cho mọi player |
| Cost | $0 | $$ (EC2/GameLift) |
| Cheat | Host toàn quyền | Server-authoritative, anti-cheat dễ |
| Scale | 4-8 player | 4-100+ |

**Source learning path:**
- [07] CPP MP § Listen vs Dedicated section.

**PALDARK take-away:**
- 🟢 PALDARK dedicated server only (cheat prevention + fair ping).
- 🟡 Dev/test phase: listen server OK cho prototyping.

**Apply ở week:** Roadmap week 25.

---

### L2 — ServerTravel + SeamlessTravel 🧪

**Goal:** Server chuyển map, player follow.

**Concept:**
- `ServerTravel("/Game/Maps/Raid?listen")` — non-seamless (disconnect/reconnect).
- `bUseSeamlessTravel = true` + `ServerTravel(...)` — seamless (PlayerController + PlayerState carry).
- Transition map — empty map load giữa 2 level.

**Source learning path:**
- [06] BP MP Crash § Travel chapter.
- [07] CPP MP § Travel chapter.

**Practice exercise (2 hour):**
1. 2 map: `Lobby` + `Raid`.
2. GameMode `Lobby` khi 4 player ready → `ServerTravel("/Game/Maps/Raid?listen")`.
3. Seamless = true → PlayerState survive travel.

**PALDARK take-away:**
- 🟢 Hub → Raid travel seamless (PlayerState carry inventory + Pal team).
- 🟢 Raid → Hub travel (extract success → seamless back).

**Apply ở week:** Roadmap week 11.

---

### L3 — OnlineSubsystem + Session (concept) 📖

**Goal:** Hiểu Session lifecycle Create/Find/Join/Destroy.

**Concept:**
- `IOnlineSubsystem` — abstract layer (Steam, EOS, NULL).
- `IOnlineSession` — Create/Find/Join/Start/End/Destroy.
- Flow: Host `CreateSession` → Joiners `FindSessions` → `JoinSession` → `ServerTravel`.
- `FOnlineSessionSettings` — map name, num player, LAN/Internet.

**Source learning path:**
- [06] BP MP § Session chapter (Steam OSS).

**PALDARK take-away:**
- 🟡 PALDARK dùng GameLift thay OSS Session truyền thống.
- 🟡 Nhưng cần hiểu concept Session để hiểu GameLift game session.

**Apply ở week:** Roadmap week 25.

---

### L4 — GameLift Anywhere → Fleet pipeline 🧪

**Goal:** Deploy server local (Anywhere) → test → promote Fleet (production).

**Concept:**
- **Anywhere Fleet:** register local machine → GameLift thinks it's a fleet server → test matchmaking without EC2 cost.
- **EC2 Fleet:** production — GameLift launch EC2 instances with server build.
- **Pipeline:** Build Server → Upload Build → Create Fleet → Create alias.
- **Game Session:** GameLift tạo "game session" = 1 match instance.
- **Player Session:** GameLift tạo "player session" = 1 player slot.

**Source learning path:**
- [08] AWS GameLift § Anywhere Fleet chapter + EC2 Fleet chapter.
- Cross-ref: `Documents/GameDesign/08-AWS_GameLift_GDD.md` § Phase 5 ADR-3.

**API / Class chính:**
- `Aws::GameLift::Server::InitSDK()`
- `Aws::GameLift::Server::ProcessReady(OnStartGameSession, OnProcessTerminate, HealthCheck, Port)`
- `Aws::GameLift::Server::ActivateGameSession()`

**Practice exercise (1 ngày):**
1. Setup AWS CLI + GameLift local.
2. Register Anywhere fleet.
3. Server binary call `InitSDK()` + `ProcessReady()`.
4. Console `aws gamelift create-game-session --fleet-id ...`.
5. Client connect → play → end.

**PALDARK take-away:**
- 🟢 Dev phase: Anywhere Fleet cho mỗi dev.
- 🟢 Staging: EC2 Fleet ap-southeast-1 (Vietnam latency).
- 🟢 Prod: Multi-region fleet ap-southeast-1 + us-west-2.

**Apply ở week:** Roadmap week 25-27.

---

### L5 — Server SDK integration 🧪

**Goal:** UE5 server call GameLift SDK lifecycle.

**Concept:**
- `AGameMode::BeginPlay` → `InitSDK()` + `ProcessReady()`.
- Callback `OnStartGameSession` → parse session data → open map.
- Callback `OnProcessTerminate` → graceful shutdown.
- `HealthCheck` → return true.
- Khi match done → `Aws::GameLift::Server::TerminateGameSession()` + `ProcessEnding()`.

**Source learning path:**
- [08] AWS GameLift § Server SDK chapter.

**Practice exercise:**
1. Trong `APaldarkLabGameMode`, add GameLift SDK call lifecycle.
2. `#if WITH_GAMELIFT` wrap.
3. Build Server target → test Anywhere Fleet.

**PALDARK take-away:**
- 🟢 `APaldarkGameMode_Raid::BeginPlay` init GameLift lifecycle.
- 🟢 Graceful shutdown save match result → DynamoDB.

**Apply ở week:** Roadmap week 26.

---

### L6 — Container deploy (Docker + ECR) 🧪

**Goal:** Server binary → Docker image → ECR → GameLift Fleet run container.

**Concept:**
- Dockerfile: base `amazonlinux:2023` + copy server binary + entrypoint.
- `docker build -t paldark-server .`
- Push ECR: `aws ecr create-repository ... && docker push ...`.
- GameLift `create-container-fleet --fleet-name paldark-prod --...`.

**Source learning path:**
- [13] Crunch § Container Deploy chapter.

**Practice exercise (1 ngày):**
1. Package Server build.
2. Dockerfile cho server.
3. Build + push ECR.
4. GameLift container fleet create.

**PALDARK take-away:**
- 🟢 Container deploy cho staging/prod.
- 🟢 CI/CD pipeline: push main → build server → push ECR → update fleet.

**Apply ở week:** Roadmap week 38-39.

---

### L7 — Server Coordinator pattern (Crunch) 📖

**Goal:** Tách match orchestration khỏi game server.

**Concept:**
- Coordinator = Lambda/backend service quản lý:
  - Match queue (players waiting).
  - Session allocation (ask GameLift for server).
  - Session health check.
  - Result recording (DynamoDB).
- Game server chỉ chạy gameplay, không quản lý match.

**Source learning path:**
- [13] Crunch § Coordinator chapter.
- Cross-ref: `Documents/GameDesign/13-Crunch_GDD.md` § Phase 5 ADR-1.

**PALDARK take-away:**
- 🟢 `PaldarkCoordinator` Lambda function:
  - `POST /match/enqueue` → add player to queue.
  - `POST /match/allocate` → ask GameLift → return endpoint.
  - `POST /match/result` → record to DynamoDB.

**Apply ở week:** Roadmap week 38.

---

## ⚡ Capstone Exercise

**Goal:** End-to-end: Client → Coordinator → GameLift Anywhere → Server → Play → End.

**Yêu cầu:**
1. Server binary with GameLift SDK lifecycle.
2. Anywhere Fleet registered.
3. Lambda Coordinator `POST /match/enqueue` + `/match/allocate`.
4. Client login → find match → connect → play 60s → match end → back to lobby.

**Acceptance:** 2 client instances connect to Anywhere server, play, match end clean.

**Effort:** 1 tuần.

---

## Khi apply vào PALDARK roadmap

| Week | Pillar P07 usage |
|------|------------------|
| W11 | ServerTravel Hub → Raid |
| W25 | GameLift Anywhere setup |
| W26 | Server SDK integration |
| W38 | Coordinator Lambda + Container deploy |
| W39 | Multi-region fleet |

---

## Anti-patterns (cảnh báo)

- 🔴 Listen server cho production game — cheat + unfair ping.
- 🔴 Hardcode server IP — dùng GameLift/Coordinator dispatch.
- 🔴 `GameMode` quản lý match queue — separation of concerns vi phạm.
- 🔴 Non-seamless travel mà PlayerState quan trọng — sẽ mất data.
- 🔴 Forget `ProcessEnding()` → GameLift timeout wait → $$$.

---

## Tham chiếu

- [`Documents/UE5_Core_Pillars.md`](../UE5_Core_Pillars.md) § P7.
- [`Documents/GameDesign/08-AWS_GameLift_GDD.md`](../GameDesign/08-AWS_GameLift_GDD.md).
- [`Documents/GameDesign/13-Crunch_GDD.md`](../GameDesign/13-Crunch_GDD.md).
- [`Documents/PALDARK/03-Roadmap_1_Year.md`](../PALDARK/03-Roadmap_1_Year.md) W25-39.
