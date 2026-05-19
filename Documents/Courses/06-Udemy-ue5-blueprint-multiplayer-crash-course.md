# 06. Unreal Engine 5 Blueprints Multiplayer CRASH COURSE

| | |
|---|---|
| **Tier** | T2 — Online Fundamentals (optional Blueprint pair với [07]) |
| **Provider** | Stephen Ulibarri (DruidMech) — Udemy |
| **Link** | https://www.udemy.com/course/ue5-blueprint-multiplayer-crash-course/ |
| **Trong repo** | ❌ **KHÔNG có source/documents** — chỉ ghi trong `Note.txt` |

---

## 1. Trạng thái

Khóa được liệt kê trong `Note.txt` nhưng **không có thư mục `06.*/` trong repo**. Lý do có thể:
- Người upload bỏ qua vì tương đương Blueprint của [07].
- Hoặc chưa upload — cần xác nhận.

> **Để hoàn chỉnh roadmap, em vẫn ghi entry này để mapping về Note.txt.** Khi nào có source sẽ cập nhật.

---

## 2. Curriculum dự đoán (theo trang Udemy)

Dựa trên tên khóa và best-guess (Stephen Ulibarri style):
1. Networking concepts (Client/Server, Authority)
2. Replication (Variable + Actor)
3. RPC (Server/Client/Multicast) bằng Blueprint
4. Class Framework (GameMode, GameState, PlayerController, PlayerState, Pawn)
5. Travel (ServerTravel, ClientTravel)
6. Hosting + joining (Steam / LAN)
7. Demo small project (e.g., 2-player co-op puzzle)

---

## 3. Pillars (dự đoán)

| Pillar | Mức |
|--------|-----|
| P2 Core Framework | 🟢 |
| P6 Replication (Blueprint flavor) | 🟡 |
| P10 UI | 🟢 |

---

## 4. Khuyến nghị

- **Bỏ qua nếu** đã chắc [07] C++ Multiplayer Crash Course — cùng concept.
- **Học song song nếu** team có designer/QA chỉ làm Blueprint.
- **Không thay được [07]** cho lập trình viên — Blueprint không expose hết RPC validation, custom struct replicate, FastArray.

---

## 5. Khi có source

Khi user push thư mục `06.*/` sẽ cập nhật:
- Chương cụ thể
- Pattern Blueprint riêng (e.g., Replicate Using Condition node, RepNotify functions)
- So sánh BP vs C++ flow

---

## 6. Liên kết

| Tương đương C++ | Trước | Sau |
|----------------|-------|-----|
| [07] C++ Multiplayer Crash | [01] Tom | [10] MP Shooter, [13] Crunch |
