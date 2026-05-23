# 13. Multiplayer in Unreal with GAS and AWS Dedicated Servers (Crunch — MOBA-style)

| | |
|---|---|
| **Tier** | T5 — Pro Production (course khủng nhất) |
| **Provider** | TutLeeUdemy — Udemy |
| **Link** | https://www.udemy.com/course/multiplayer-in-unreal-with-gas-and-aws-dedicated-servers/ |
| **Source** | https://github.com/TutLeeUdemy/Crunch |
| **Trong repo** | `13.Udemy-ue5-multiplayer-in-unreal-with-gas-and-aws-dedicated-servers/Documents/` (subtitle theo chương) |

---

## 1. Mục tiêu khóa

Build **MOBA-style multiplayer game "Crunch"** đầy đủ stack: GAS + Lobby + Coordinator + AWS Dedicated Server + Containerization. **Gộp [05] + [08] + [10] + [11] vào 1 dự án.**

---

## 2. Lộ trình giảng dạy (28 chương)

| Chương | Cốt lõi |
|--------|---------|
| 01 - Introduction | Course intro |
| 02 - Basic Character Movement Control and Animations | Third-person, Enhanced Input, locomotion |
| 03 - **Integrate GAS to the Character** | ASC + AS setup |
| 04 - Setup Basic UI Widgets | Health/Mana/Cooldown widget |
| 05 - **Implement the Combo Gameplay Ability** | Combo state machine via GA + AbilityTask |
| 06 - Setup Death, Respawn and Teams | Respawn cycle, team allocation, friendly fire off |
| 07 - **Create the Minions with AI Perception and Behavior** | BT + Perception cho lane minion |
| 08 - The Upper Cut Ability | Knockback ability, anim notify |
| 09 - Implement the Ability Gauge Widget | Cooldown gauge, charge bar |
| 10 - The Ground Blast Ability | AoE ground targeting |
| 11 - **Character Level, Stats and Stats Driven Attributes** | Level curve, stat → attribute pipeline |
| 12 - Ability Upgrading | Spend skill point on ability rank |
| 13 - **GAS Optimization** | Net update freq cho ASC, replication tweak |
| 14 - The Shop System | Buy items mid-match |
| 15 - The Inventory System | Item slot, stack, sell |
| 16 - Create a Headshot Render | Player portrait render-to-texture |
| 17 - Storm Core | Match objective actor |
| 18 - Another Character with Ranged Attack | Hero #2 |
| 19 - Lazer Ability | Beam channel ability |
| 20 - Black Hole Ability | Pull/gravity AoE |
| 21 - Create the Lobby | Lobby map + UI |
| 22 - **Hero Selection** | Lobby hero pick, lock-in |
| 23 - Test Dedicated Server Packaging and Connecting | Server target build, packaging |
| 24 - **User Login and Session Creation Request** | HTTP login → Cognito-like auth → session |
| 25 - **Server Coordinator and Session Orchestration** | Coordinator service (Node/Lambda) match-make |
| 26 - **Server and Client Packaging and Containerization** | Docker container cho dedicated server |
| 27 - **Server Deployment on AWS** | ECS / GameLift fleet deploy |
| 28 - Conclusion | Wrap |

---

## 3. Cốt lõi (Pillars)

| Pillar | Mức |
|--------|-----|
| P8 GAS | 🔴 (combo + level + optimization + multiplayer) |
| P6 Replication | 🟠 |
| P7 Dedicated Server | 🔴 (containerized — sâu hơn [08]) |
| P18 Backend | 🔴 (Coordinator + Auth + Orchestration) |
| P9 AI | 🟡 (minion BT) |
| P11 Inventory | 🟠 (shop + slot) |
| P10 UI | 🟠 (lobby, hero select, HUD, shop) |

**Pattern key:**
- **Server Coordinator:** thay matchmaking đơn giản bằng coordinator riêng — assign player vào dedicated server instance phù hợp (region, version).
- **Containerization:** dạy đóng gói server thành Docker image rồi deploy lên ECS/GameLift.
- **GAS Optimization:** giảm bandwidth của ASC replication — turn off attribute không cần replicate, reduce frequency.

---

## 4. Approach sư phạm

- **Build-along style:** mỗi chương tăng dần độ phức tạp.
- **Hero / Ability đa dạng:** dạy 5+ ability với pattern khác nhau (instant, combo, beam, AoE, gravity).
- **Stack đầy đủ:** end-to-end từ character → ability → AI → lobby → matchmaking → containerize → deploy.
- **Production-grade:** containerization là điểm vượt [08].

---

## 5. Đầu ra học viên

- Có game MOBA-style 5v5 (hoặc nhỏ hơn) chạy trên AWS production.
- Hiểu được container Docker cho dedicated server.
- Hiểu Coordinator pattern (không phải matchmaking GameLift mặc định).
- Optimize được GAS bandwidth.

---

## 6. Lưu ý

- **Yêu cầu nền cao:** học viên cần xong [01] + [05] + [07] + [08] + [11] trước.
- **Source phức tạp** — không phải khóa đầu tiên cho người mới.
- **Không cover State Tree** — phải [15] Lyra.
- **Không cover Lyra** — đây là full custom, không phải Lyra.

---

## 7. Liên kết khóa khác

| Trước | Sau |
|-------|-----|
| [05] + [07] + [08] + [11] | (đỉnh stack — đến đây xong là production-ready) |
