# 08. Unreal Engine 5 Dedicated Servers with AWS and GameLift

| | |
|---|---|
| **Tier** | T5 — Pro Production |
| **Provider** | Stephen Ulibarri (DruidMech) — Udemy |
| **Link** | https://www.udemy.com/course/unreal-engine-5-dedicated-servers-with-aws-and-gamelift/ |
| **Source** | https://github.com/DruidMech/DedicatedServers |
| **Trong repo** | `08.Udemy-ue5-dedicated-servers-with-aws-and-gamelift/` (Documents + Lambdas + Prompts + Source) |

---

## 1. Mục tiêu khóa

Đưa game UE5 lên **production dedicated server stack** dùng AWS GameLift + Lambda + DynamoDB. Sau khóa, học viên có:
- Server target build & package.
- Fleet on AWS GameLift (cả Anywhere + Managed).
- Login flow: AWS Cognito + JWT access token.
- Sessions: create / list / join qua API Gateway → Lambda → GameLift.
- Lobbies + timers.
- Leaderboards trên DynamoDB.

---

## 2. Lộ trình giảng dạy (16 chương)

| Chương | Cốt lõi |
|--------|---------|
| 01 - Introduction | Phạm vi khóa, AWS pricing, free tier |
| 02 - Getting Started | Source build UE5, FPS template, server target |
| 03 - AWS Account | IAM user, billing alert, root vs IAM best practice |
| 04 - Integrating GameLift with UE5 | GameLift Server SDK + plugin |
| 05 - GameLift Anywhere Fleets | Anywhere fleet để test local |
| 06 - GameLift Fleets | Managed EC2 fleet (cost-aware) |
| 07 - AWS Lambda | Serverless function, IAM role |
| 08 - API Gateway | HTTP API endpoint → Lambda |
| 09 - Game Sessions | `CreateGameSession`, `DescribeGameSessions` qua Lambda |
| 10 - Player Sessions | `CreatePlayerSession`, mapping player ↔ session |
| 11 - AWS Cognito | User pool, sign up, sign in, MFA |
| 12 - Access Tokens | JWT, refresh token, attach token vào API request |
| 13 - Lobbies and Timers | Lobby pattern + countdown server-side |
| 14 - Accepting Player Sessions | `AcceptPlayerSession`, validate session id, kick imposters |
| 15 - DynamoDB | Partition key + sort key, GetItem/PutItem từ Lambda |
| 16 - Leaderboards | Read/write DynamoDB từ Lambda + UE5 HTTP request |

**Extra folder trong repo:**
- `Lambdas/` — code Node.js cho Lambda function.
- `Prompts/` — prompt template để generate Lambda (course có dùng AI để gen serverless).
- `Source/` — game UE5 wrapper.

---

## 3. Cốt lõi (Pillars)

| Pillar | Mức |
|--------|-----|
| P7 Dedicated Server | 🔴 (production stack) |
| P18 Backend / Live Ops | 🔴 (GameLift + Lambda + Cognito + DynamoDB) |
| P1 C++/Build | 🟠 (server target build, package từ command-line) |
| P10 UI | 🟢 (login screen, lobby UI) |

---

## 4. Approach sư phạm

- **AWS-first, UE second:** mỗi chương đều set up AWS trước, sau đó mới đến UE C++ wrapper.
- **Code AWS Lambda thật bằng Node.js:** không phải pseudo-code. Có folder `Lambdas/` đầy đủ.
- **An ninh:** dạy JWT validation server-side để chống fake session.
- **Cost-aware:** chương 6 explicit nói về EC2 instance type và spending alarm.

---

## 5. Đầu ra học viên

- Có hosted dedicated server thật, đăng nhập qua Cognito, vào lobby, chơi match, kết quả lưu lên DynamoDB leaderboard.
- Hiểu pricing AWS GameLift / Lambda / DynamoDB.
- Có template Lambda + API Gateway + Cognito tái dùng cho dự án khác.

---

## 6. Lưu ý

- **Không cover containerization Docker** — chỉ EC2 managed fleet. Containerize là phần của [13] Crunch.
- **Không cover matchmaking thật** (skill rating, MMR) — Lobby pattern đơn giản.
- **Không có GAS** — sử dụng plain ability trong demo.
- **AWS pricing thay đổi:** một số screenshot UI có thể outdated.

---

## 7. Liên kết khóa khác

| Trước | Sau |
|-------|-----|
| [07] MP Crash | [13] Crunch (gộp GAS + AWS) |
