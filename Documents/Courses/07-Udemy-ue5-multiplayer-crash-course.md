# 07. Unreal Engine 5 C++ Multiplayer CRASH COURSE

| | |
|---|---|
| **Tier** | T2 — Online Fundamentals (bắt buộc trước mọi khóa multiplayer khác) |
| **Provider** | Stephen Ulibarri (DruidMech) — Udemy |
| **Link** | https://www.udemy.com/course/ue5-multiplayer-crash-course/ |
| **Trong repo** | `07.Udemy-ue5-multiplayer-crash-course/Documents/` (đủ subtitle, không có Source) |

---

## 1. Mục tiêu khóa

Crash course **lý thuyết-thực-hành** về multiplayer C++ trong UE5: hiểu Client-Server model, replication, RPC, class framework, travel — đủ để vào [10] Shooter và [13] Crunch.

---

## 2. Lộ trình giảng dạy (đọc từ thư mục Documents)

| Chương | Lecture | Cốt lõi |
|--------|---------|---------|
| 01 - Introduction | 001 Introduction | Phạm vi khóa |
| **02 - Multiplayer Fundamentals** | 001 Client-Server Model | Authority, Listen vs Dedicated |
|  | 002 Testing Multiplayer | PIE multi-player options |
|  | 003 LAN Connection | Open IP, OpenLevel |
|  | 004 Listen Servers via Steam | OnlineSubsystem Steam |
| **03 - Actor Replication** | 001 Actor Replication | `bReplicates`, `bAlwaysRelevant` |
|  | 002 Authority and Net Role | `HasAuthority`, `GetLocalRole()`, `GetRemoteRole()` |
|  | 003 Attachment | Replicate parent → child |
|  | 004 Variable Replication | `UPROPERTY(Replicated)` + `DOREPLIFETIME` |
|  | 005 RepNotifies | `UPROPERTY(ReplicatedUsing=OnRep_X)` + `OnRep_X()` |
|  | 006 Replication Conditions | `COND_OwnerOnly`, `COND_SkipOwner`, `COND_InitialOnly` |
|  | 007 Custom Rep Conditions | `DOREPLIFETIME_ACTIVE_OVERRIDE`, `PreReplication` |
|  | 008 Ownership | `SetOwner` để biến RPC owner-only chạy |
|  | 009 Actor Components | Component cũng phải `SetIsReplicated(true)` |
| **04 - Remote Functions** | 001 Run on Client | `UFUNCTION(Client)` — chỉ owner client |
|  | 002 Run on Server | `UFUNCTION(Server, Reliable)` — client → server |
|  | 003 Multicast | `UFUNCTION(NetMulticast)` — server → all clients |
|  | 004 RPC Validation | `WithValidation` + `_Validate` function (anti-cheat baseline) |
|  | 005 Relevancy and Priority | `NetUpdateFrequency`, `bAlwaysRelevant`, `bOnlyRelevantToOwner` |
| **05 - Class Framework** | 001 The Game Mode | Server-only |
|  | 002 The Game State | Replicated, mọi client thấy |
|  | 003 The Player State | Mỗi player 1 cái, replicated |
|  | 004 The Player Controller | Owning client + server, owner-only |
|  | 005 Pawn and Character | Movement Component, replicate |
|  | 006 HUD and Widgets | Client-side, không replicate |
|  | 007 Static Accessor Functions | Helpers lấy GameMode/GameState an toàn |
| **06 - Travel in Multiplayer** | 001 Travel | ServerTravel vs ClientTravel |
|  | 002 Implementing Seamless Travel | `bUseSeamlessTravel`, transition map |

---

## 3. Cốt lõi (Pillars)

| Pillar | Mức | Bằng chứng |
|--------|-----|------------|
| P2 Core Framework | 🟠 (production-grade) | Chương 5 toàn bộ |
| P6 Replication | 🟠 | Chương 3 + 4 đủ depth |
| P7 Dedicated/Sessions | 🟡 (cơ bản) | Chương 2 + 6 |

---

## 4. Approach sư phạm

- **Mỗi concept = 1 video ngắn (~5–10 phút).**
- **Authority-first:** mọi ví dụ bắt đầu bằng câu hỏi "ai có Authority?". 
- **Replication Conditions trước RPC:** dạy biến trước, hàm sau — đúng vì biến rep tự động và "an toàn" hơn RPC.
- **Không xài plugin matchmaking** ở khóa này — chỉ Steam listen server. Matchmaking nâng cao để [08] và [13].

---

## 5. Đầu ra học viên

- Đọc/sửa được mọi code có `UPROPERTY(Replicated)` hay `UFUNCTION(Server)` mà không phải tra tài liệu.
- Hiểu khi nào dùng `Reliable` vs `Unreliable` (Unreliable cho movement, Reliable cho gameplay event).
- Tự build được mini-game 2 player co-op + listen server.

---

## 6. Lưu ý

- Không cover **Lag Compensation** — phải lên [10] MP Shooter.
- Không cover **Sessions Online Subsystem advanced** — phải lên [08].
- Không cover **GAS Replication** — phải lên [13].
- Không có Source thực tế trong repo, chỉ subtitle để học khái niệm.

---

## 7. Liên kết khóa khác

| Trước | Sau |
|-------|-----|
| [01] Tom Action Roguelike, [12] Pro Coding | [10] MP Shooter, [08] AWS, [13] Crunch |
