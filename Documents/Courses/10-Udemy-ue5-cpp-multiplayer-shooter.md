# 10. Unreal Engine 5 C++ Multiplayer Shooter (Blaster Game)

| | |
|---|---|
| **Tier** | T4 — Specialized Systems (Multiplayer game thật) |
| **Provider** | Stephen Ulibarri (DruidMech) — Udemy |
| **Link** | https://www.udemy.com/course/unreal-engine-5-cpp-multiplayer-shooter/ |
| **Source** | https://github.com/DruidMech/MultiplayerCourseBlasterGame |
| **Trong repo** | `10.Udemy-ue5-cpp-multiplayer-shooter/` (Documents + Plugins + Source) |

---

## 1. Mục tiêu khóa

Build **third-person multiplayer shooter** từ đầu trên UE5 C++. Trong khóa có **Lag Compensation (Server-Side Rewind)** — cốt lõi của FPS competitive.

---

## 2. Lộ trình giảng dạy (16 chương)

| Chương | Cốt lõi |
|--------|---------|
| 01 - Introduction | Project scope, multiplayer concepts (warm-up cho ai chưa qua [07]) |
| 02 - Creating a Multiplayer Plugin | Custom plugin `Multiplayer Sessions` — wrap OnlineSubsystem |
| 03 - Project Creation | Setup project, third-person, replicate basics |
| 04 - The Weapon | `AWeapon`, attach socket, replicate, state machine |
| 05 - Firing Weapons | Hitscan + projectile, RPC fire, decals |
| 06 - Weapon Aim Mechanics | Aim offset, FABRIK IK, crosshair spread |
| 07 - Health and Player Stats | Replicated health, damage, PlayerState replicate |
| 08 - Ammo | Per-weapon ammo, reload state, sync HUD |
| 09 - Match States | `EMatchState::WaitingToStart / InProgress / Cooldown` |
| 10 - Different Weapon Types | Assault rifle, shotgun, sniper, SMG, rocket launcher, grenade |
| 11 - Pickups | Weapon, ammo, health, shield pickup actor |
| 12 - **Lag Compensation** | Server-Side Rewind: lưu frame history hitbox, rewind theo timestamp khi shoot |
| 13 - More Multiplayer Features | Spectator, scoreboard, elim feed |
| 14 - Teams | Team color, team damage off, team logic |
| 15 - Capture the Flag | Flag actor, capture point, win condition |
| 16 - Congratulations | Wrap up, polish |

---

## 3. Cốt lõi (Pillars)

| Pillar | Mức |
|--------|-----|
| P2 Core Framework | 🟠 (full GameMode + GameState + PS + HUD pipeline) |
| P6 Replication | 🔴 (lag compensation = ngưỡng cao nhất) |
| P5 Animation | 🟠 (aim offset, FABRIK, montage cho fire/reload) |
| P10 UI | 🟠 (full HUD: ammo, health, score, elim feed, scoreboard) |
| P7 Sessions | 🟡 (Multiplayer Sessions plugin custom wrap OSS) |

**Lag Compensation chi tiết:**
- Mỗi character lưu `TArray<FFramePackage>` (snapshot hitbox theo thời gian).
- Khi server xác nhận hit, **rewind** hitbox về timestamp gửi từ client.
- Bù phạm vi ±100ms (config) — cân bằng giữa "shooter behind cover" và "I shot first".

---

## 4. Approach sư phạm

- **Single-player → multiplayer:** mỗi feature bắt đầu local sau đó replicate.
- **HUD bound qua PlayerState delegate** — không poll mỗi frame.
- **Match states FSM:** GameMode handle, GameState replicate — chuẩn pattern UE.
- **Lag Comp dạy cuối:** đặt chương 12 là cố ý — học viên đã hiểu pipeline shoot trước khi rewind.

---

## 5. Đầu ra học viên

- Có **game shooter multiplayer chơi được** với CTF, teams, scoreboard, weapons đa dạng, lag comp.
- Có plugin `Multiplayer Sessions` tái dùng được (wrap OSS Steam).
- Hiểu được tại sao server FPS competitive phải rewind hitbox.

---

## 6. Lưu ý

- **Không cover GAS** — dùng custom system. So sánh với [11]+[13] để thấy GAS có gì hơn.
- **Không cover dedicated server packaging + AWS** — phải lên [08].
- **Không cover anti-cheat** chỉ có validation cơ bản.

---

## 7. Liên kết khóa khác

| Trước | Sau |
|-------|-----|
| [07] MP Crash, [01] Tom | [08] AWS, [13] Crunch |
