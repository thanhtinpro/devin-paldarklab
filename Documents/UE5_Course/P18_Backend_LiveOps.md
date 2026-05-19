# P18 — Backend / Live Ops

> Tự soạn từ: [08] AWS GameLift, [13] Crunch, [04] ReadyOrNot.
>
> Pillar cuối — ship + retain player. PALDARK extraction game = persistent inventory + Pal roster + leaderboard → backend bắt buộc.

---

## Pillar Identity

**Cái gì:** Tích hợp dịch vụ ngoài Unreal — auth, leaderboard, matchmaking, telemetry.

**Scope:**
- ✅ AWS Lambda (serverless function).
- ✅ API Gateway (HTTP/WebSocket endpoint).
- ✅ Cognito User Pool + Identity Pool (auth + JWT).
- ✅ DynamoDB (NoSQL persistence).
- ✅ GameLift Fleet (P07 detail) — match server allocation.
- ✅ EC2/ECS Container (P07 detail).
- ✅ Coordinator pattern (Crunch).
- ✅ Steam Workshop (mod.io alternative).
- ✅ EOS (Epic Online Service) — cross-platform auth.
- ✅ Telemetry (analytics, crash reporting).

**Out of scope:**
- ❌ Game Server packaging (P07).
- ❌ Save game local (P13).

---

## PALDARK cần gì từ pillar này?

| Need | Solution |
|------|----------|
| Account login | Cognito User Pool + Steam OSS / EOS |
| Persistent Inventory (Stash) | DynamoDB |
| Pal Roster | DynamoDB |
| Match Queue + Allocation | Coordinator Lambda + GameLift |
| Leaderboard (Pal collect, raid score) | DynamoDB GSI + Lambda |
| Crash report | Sentry or Backtrace |
| Analytics (DAU, retention) | Custom pipeline (Pinpoint or 3rd party) |
| Patch + DLC distribution | Steam + EGS update |
| Anti-cheat | Easy Anti-Cheat (Epic) integration |

---

## Sources Matrix

| Source | Coverage | Lesson lấy từ đó |
|--------|----------|------------------|
| [08] AWS GameLift | 🟢 | L1-5 (full backend stack) |
| [13] Crunch | 🟢 | L6 Coordinator + Container deploy |
| [04] RoN GDD | 🟠 | L7 mod.io + Steam Workshop ref |

---

## Prerequisite

- P07 (Server + Session).
- P13 (Save/Load — hybrid local + backend).

---

## Lessons

### L1 — AWS Lambda + API Gateway 🧪

**Goal:** Serverless function exposed via HTTP endpoint.

**Concept:**
- Lambda = function (Node.js / Python) run on AWS, scale auto.
- API Gateway = HTTP/WebSocket endpoint → trigger Lambda.
- Trade-off: cold start latency 100-500ms.

**Source learning path:**
- [08] AWS GameLift § Lambda chapter.

**API / Class chính:**
- AWS SDK Node.js: `@aws-sdk/client-dynamodb`, `@aws-sdk/client-gamelift`.
- UE side: `IHttpRequest::SetURL/SetVerb/SetContentAsString/ProcessRequest`.
- `IHttpResponse::GetResponseCode/GetContentAsString`.

**Practice exercise (4 hour):**
1. Create Lambda `paldark-hello` return `{"message": "Hello"}`.
2. API Gateway `GET /hello` → Lambda.
3. UE5 HTTP request → log response.

**PALDARK take-away:**
- 🟢 Lambda functions:
  - `auth/login` — Cognito flow.
  - `match/enqueue` — add to queue.
  - `match/allocate` — call GameLift.
  - `inventory/get` — fetch Hub stash.
  - `inventory/set` — update Hub stash.
  - `roster/get` — fetch Pal roster.
  - `leaderboard/get` — top 100 Pal collectors.

**Apply ở week:** Roadmap week 38.

---

### L2 — Cognito User Pool + JWT 🧪

**Goal:** User registration + login → JWT token → call other API authenticated.

**Concept:**
- Cognito User Pool — managed user database (email/password).
- Identity Pool — federation (Steam/Google → Cognito identity).
- `InitiateAuth` → return ID token + Access token + Refresh token.
- Send Access token in `Authorization: Bearer <token>` header.

**Source learning path:**
- [08] AWS GameLift § Cognito chapter.

**Practice exercise (1 ngày):**
1. Cognito User Pool setup.
2. UE5 register flow: signup → confirm email → login.
3. UE5 store JWT in memory.
4. Call protected endpoint with token.

**PALDARK take-away:**
- 🟢 Cognito User Pool for paldark.com account.
- 🟢 Identity Pool federate Steam/EOS.
- 🟢 Token refresh every 50 min (1h expiry).

**Apply ở week:** Roadmap week 38.

---

### L3 — DynamoDB NoSQL 🧪

**Goal:** Persistent key-value store, schema-flex, GSI for query.

**Concept:**
- Table = collection of items.
- Item = JSON-like, schemaless but Primary Key required.
- Primary Key: Partition Key + (optional) Sort Key.
- GSI (Global Secondary Index) = secondary lookup pattern.
- TTL: auto-delete after timestamp.

**Source learning path:**
- [08] AWS GameLift § DynamoDB chapter.

**Practice exercise (4 hour):**
1. Table `PaldarkPlayer` (Partition: UserID).
2. Table `PaldarkInventory` (Partition: UserID, Sort: ItemSlot).
3. Lambda CRUD on these tables.
4. UE5 fetch + display inventory from backend.

**PALDARK take-away:**
- 🟢 Tables:
  - `Player` (UserID → profile + level + XP + currency).
  - `Inventory` (UserID + Slot → item).
  - `PalRoster` (UserID + PalSlot → pal data).
  - `MatchHistory` (UserID + Timestamp → result).
  - `Leaderboard` (Category + Score sort → top 100 GSI).
- 🟢 TTL on `MatchQueue` 60s (auto-clean stale queue).

**Apply ở week:** Roadmap week 38-39.

---

### L4 — GameLift Fleet (P07 deeper) 📖🧪

**Goal:** Match server allocation production.

**Concept (recap from P07):**
- Anywhere Fleet for dev/test.
- EC2 Fleet for production.
- Container Fleet (Docker) — easier deploy.
- Allocate: `gamelift.createGameSession(...)` → return endpoint + port.

**Source learning path:**
- [08] AWS GameLift § Fleet chapter.
- [13] Crunch § Container deploy chapter.

**PALDARK take-away:**
- 🟢 Multi-region: ap-southeast-1 (Vietnam) + us-west-2 + eu-west-1.
- 🟢 Auto-scale fleet on demand (queue length > threshold → scale up).

**Apply ở week:** Roadmap week 39.

---

### L5 — UE5 HTTP Client (call backend) 🧪

**Goal:** Send/receive JSON to backend from UE5 client.

**Concept:**
- `FHttpModule::Get().CreateRequest()`.
- Set URL, Verb (GET/POST), Header (Authorization, Content-Type).
- Body: `Request->SetContentAsString(JsonString)`.
- Bind `OnProcessRequestComplete` delegate.
- Parse response with `FJsonObjectConverter`.

**Source learning path:**
- [08] AWS GameLift § HTTP chapter.
- [13] Crunch § HTTP chapter.

**Practice exercise (3 hour):**
1. `UPaldarkLabBackendSubsystem` (GameInstance Subsystem).
2. Method `GetInventory(UserID, CompleteCallback)`.
3. Build JSON request → API Gateway → Lambda → DynamoDB → response.
4. Parse JSON → populate UPaldarkLabInventory ref.

**PALDARK take-away:**
- 🟢 `UPaldarkBackendSubsystem` wrap all HTTP call.
- 🟢 Token attached automatic.
- 🟢 Cache + retry policy.

**Apply ở week:** Roadmap week 38.

---

### L6 — Coordinator pattern (Crunch CORE) 🧪

**Goal:** Tách match orchestration khỏi game server.

**Concept (recap from P07):**
- Coordinator Lambda = orchestration brain.
- Match queue management.
- GameLift session allocation.
- Result recording.

**Source learning path:**
- [13] Crunch § Coordinator chapter.

**Practice exercise:**
1. Lambda `match/enqueue` — push UserID to queue table.
2. Lambda `match/check` (scheduled every 5s) — find 4 queued → allocate GameLift → notify clients.
3. Lambda `match/result` — record DynamoDB MatchHistory.
4. Client poll `match/status` → get endpoint.

**PALDARK take-away:**
- 🟢 Coordinator with match queue + result recording.
- 🟢 Stretch: Skill-based matchmaking (use Player ELO from DynamoDB).

**Apply ở week:** Roadmap week 38.

---

### L7 — mod.io / Steam Workshop / EOS (RoN ref) 📖

**Goal:** Distribute mod content + cross-platform achievement.

**Concept:**
- mod.io SDK — mod browser + download (RoN dùng).
- Steam Workshop — Steam-only mod.
- EOS — Epic auth, achievement, friends.

**Source learning path:**
- [04] RoN GDD § Phase 2 community/mods.

**PALDARK take-away:**
- 🟡 Beta scope: KHÔNG có mod system.
- 🟡 Post-beta Q4: Steam Workshop cho cosmetic mod.

**Apply ở week:** Defer post-beta.

---

## ⚡ Capstone Exercise

**Goal:** PaldarkLab full backend stack.

**Yêu cầu:**
1. Cognito User Pool — register + login flow.
2. DynamoDB 3 table: Player, Inventory, Roster.
3. API Gateway 6 endpoint.
4. Lambda 6 function: auth, inventory get/set, roster get/set, match enqueue/allocate.
5. GameLift Anywhere Fleet integrate.
6. UE5 Backend Subsystem call all 6 endpoint.
7. End-to-end test: register → login → fetch inventory → match queue → match allocate → join server → match end → update inventory.

**Acceptance:** 2 client end-to-end pass; no error.

**Effort:** 2 tuần.

---

## Khi apply vào PALDARK roadmap

| Week | Pillar P18 usage |
|------|------------------|
| W38 | Backend Subsystem + Cognito + 3 Lambda |
| W38-39 | DynamoDB tables + GameLift + Coordinator |
| W39 | Multi-region fleet + Skill-based matchmaking |
| W41 | Telemetry pipeline |
| W42-43 | Anti-cheat + crash reporting |
| W48 | Beta launch — backend stable |

---

## Anti-patterns (cảnh báo)

- 🔴 Authoritative data (Inventory, Currency) saved local → cheat.
- 🔴 No token refresh → user logged out every hour.
- 🔴 Sync HTTP call blocking game tick → freeze.
- 🔴 No retry policy → 1 backend hiccup = match drop.
- 🔴 DynamoDB without GSI → leaderboard scan O(N) — slow + $$$.
- 🔴 Cold start 500ms on critical path → P99 latency bad. Use provisioned concurrency or Step Function.
- 🔴 No telemetry → can't debug live issue.
- 🔴 No analytics → don't know retention, can't tune game.

---

## Tham chiếu

- [`Documents/UE5_Core_Pillars.md`](../UE5_Core_Pillars.md) § P18.
- [`Documents/GameDesign/08-AWS_GameLift_GDD.md`](../GameDesign/08-AWS_GameLift_GDD.md) — Full backend stack.
- [`Documents/GameDesign/13-Crunch_GDD.md`](../GameDesign/13-Crunch_GDD.md) — Coordinator + Container.
- [`Documents/PALDARK/03-Roadmap_1_Year.md`](../PALDARK/03-Roadmap_1_Year.md) — W38-48 backend phase.
