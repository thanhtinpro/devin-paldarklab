# Vision Reverse Engineering — Phương Pháp Bắt Đầu Một Dự Án

> **Tài liệu này không phải về PALDARK.** Đây là **phương pháp luận** bạn áp dụng cho **mọi** dự án game tương lai.
>
> Áp dụng cụ thể cho PALDARK ở [../PALDARK/00-VISION.md](../PALDARK/00-VISION.md).
>
> Tài liệu này có thể được copy nguyên sang dự án game khác.

---

## I. Quy Tắc Cốt Lõi

> **Không bao giờ bắt đầu một dự án game bằng cách viết code.**
>
> Bạn bắt đầu bằng cách viết **một cảnh** mà bạn muốn người chơi trải nghiệm. Sau đó từ cảnh đó, **reverse engineer** ngược lại các hệ thống kỹ thuật cần thiết.

Đây là quy tắc **#1**. Vi phạm = scope creep, lang mang, bỏ dở.

---

## II. 7-Step Vision Reverse Engineering

### Step 1 — Viết "The Scene"

**Đầu ra:** Một đoạn văn 500–1000 chữ tả cảnh người chơi sẽ trải nghiệm, ở góc nhìn ngôi thứ 2 ("bạn").

**Tiêu chí:**
- Phải có **emotion** rõ (sợ, hồi hộp, ngạc nhiên, ấm áp).
- Phải có ít nhất 1 **NPC/companion** cụ thể (không generic).
- Phải có ít nhất 1 **quyết định** người chơi phải làm.
- Phải có ít nhất 1 **hệ quả** của quyết định.
- Không tả mechanic ("HP", "stat", "level") — tả cảm giác.

**Test:** Đưa cho 3 người không biết game. Nếu họ hỏi "khi nào ra mắt?" thì pass. Nếu họ nói "ờ ờ" thì viết lại.

**Ví dụ PALDARK:** Cảnh ở [VISION.md mục II](../PALDARK/00-VISION.md#ii-cnh-sẽ-bn-game-này).

### Step 2 — Tách "The Scene" thành System Triggers

Đọc lại Scene câu-từng-câu. Mỗi câu có hành động/cảm xúc, hỏi: **"Cần system nào để chuyện này xảy ra?"**

**Ví dụ PALDARK:**

| Câu trong Scene | System cần |
|-----------------|-----------|
| "Foxparks bám sát chân bạn" | Pal AI Follow Activity, NavMesh |
| "Foxparks nhe răng. Hướng đông." | Pal Sense (sight), animation reaction |
| "Tai bạn nghe gầm" | Audio Propagation FMOD |
| "Boltmane Alpha cấp 30+" | GAS attribute + level + AI tier system |
| "20 giây tiếp theo, bạn quyết định" | Tension UI / time-based decision |
| "Foxparks nhảy lên vai bạn" | Bond Level + companion behaviour |
| "Bạn không nghĩ về điểm" | Game design: emotion-first, not score-first |

→ **System list:** Pal AI, NavMesh, Sense, Audio Propagation, GAS, AI tier, Bond, Anim.

### Step 3 — Map Systems vào 18 UE5 Pillar

Mỗi system → pillar nào? Bảng đối chiếu.

**Ví dụ PALDARK Scene:**

| System | Pillar |
|--------|--------|
| Pal AI Follow | P9 |
| NavMesh | P9 |
| Sense | P9 |
| Audio Propagation | P16 |
| GAS attribute | P8 |
| AI tier | P9 + P12 |
| Bond | P8 (attribute) + P12 (data) |
| Anim reaction | P5 |

→ Pillar cần thiết cho Scene: **P5, P8, P9, P12, P16**.

→ **Tiếp tục** thêm các pillar nền (P1, P2, P3, P4, P6, P7) — vì không có chúng không chạy được code multiplayer.

### Step 4 — Define 3 Game Pillars (Donchitos)

> **3 pillar duy nhất**, không hơn không kém. Phải cô đọng đủ để **cắt một feature** dựa trên chúng.

**Format:**
```
Pillar N: [Tên]
"[Câu trích định nghĩa cảm xúc]"
- [Mệnh đề]
- [Mệnh đề]
```

**Ví dụ PALDARK:**
- Pillar 1: Pal Bond Trumps Gun
- Pillar 2: Information is Survival
- Pillar 3: Tame Once, Trust Forever

**Quy tắc:**
- Nếu 1 feature **không phục vụ** ≥1 pillar trong 3 → **cắt**.
- Nếu 1 feature **vi phạm** ≥1 pillar → **cắt mạnh**.

### Step 5 — Define Scope Limits

> Quan trọng hơn cả "làm gì" là **không làm gì**.

**Format:**
```
Sẽ KHÔNG có (trong beta):
- [Feature lớn 1] → đẩy v1.0
- [Feature lớn 2] → đẩy v0.9
...
```

**Quy tắc:**
- Cut ≥5 feature lớn.
- Cut phải có **lý do** (vi phạm pillar / scope / cost).
- Cut phải có **destination** (đẩy vào version sau hoặc cancel).

### Step 6 — Reverse Roadmap

Từ "Beta launch" ngược lại về "Tuần 1".

**Quy tắc:**
- Quý cuối: polish + backend + content cuối + closed beta.
- Quý 3: content + map + Pal/enemy.
- Quý 2: core gameplay vertical slice.
- Quý 1: foundation + Lyra + 1 system core demo.
- Tuần 1–2: setup + first build.

**Test:** Mỗi quý phải có **Milestone Build** chạy được, không phải "đang nghiên cứu".

### Step 7 — Resource Map (Pillar × Nguồn)

Mỗi pillar → đọc nguồn nào trước?

**Format:** Xem [04-Resource_Map.md](../PALDARK/04-Resource_Map.md) làm mẫu.

**Quy tắc:**
- Mỗi pillar tối thiểu 1 nguồn 🥇 (must-read).
- Nguồn phải có pattern cụ thể (file/class/chapter) — không generic.

---

## III. Anti-Patterns

> Các sai lầm thường gặp khi bắt đầu một dự án.

| Anti-pattern | Triệu chứng | Cách sửa |
|--------------|-------------|----------|
| **"Tôi muốn làm game MMO"** | Scope vô hạn, không bao giờ ship | Cắt thành "game co-op 4 người, hub town shard 8 người" |
| **"Bắt đầu bằng việc setup Lyra"** | 3 tháng chỉ setup không có gameplay | Bắt đầu bằng Scene → Pillar → System map → mới Setup |
| **"Tôi học GAS xong rồi mới quyết định game"** | Học vô tận, không bao giờ làm | Quyết định game trước, học GAS có mục đích |
| **"Game của tôi giống X nhưng tốt hơn"** | Generic, không có pillar riêng | Định nghĩa 3 pillar cụ thể khác X |
| **"Sẽ thêm tính năng nếu có thời gian"** | Lazy scope, sẽ bùng | Cắt thẳng list "Sẽ KHÔNG có" |
| **"PvE và PvP đồng thời"** | Balance impossible | Chọn 1 cho beta |
| **"Open world ngay từ đầu"** | Tech debt khổng lồ | Closed map nhỏ trước, mở rộng sau |
| **"Mình tự làm asset luôn"** | 6 tháng làm art, gameplay rỗng | Asset placeholder hoặc mua marketplace |

---

## IV. Daily Reality Check

Mỗi sáng tự hỏi:

> **"Hôm nay tôi làm gì để Cảnh [Step 1] xảy ra thực sự?"**

Nếu trả lời được = cày. Nếu không = mở roadmap, tìm task tuần đó.

---

## V. Weekly Outcome Check

Cuối mỗi tuần:

1. ✅ Build chạy được không?
2. ✅ Có screenshot/video demo feature mới không?
3. ✅ Outcome roadmap tuần đó đạt chưa?
4. ✅ Pillar % cumulative tăng so với tuần trước?
5. ✅ Có cắt được feature nào không cần không?

Nếu 0–1 câu yes → flag risk. Nếu 4–5 câu yes → bình thường. Nếu 5 câu yes liên tục 8 tuần → có thể nâng scope.

---

## VI. When Vision Conflicts with Reality

Khi nào điều chỉnh Vision?

| Tình huống | Cách xử |
|-----------|---------|
| Pillar 2 (Info Survival) cần FMOD nhưng license $$$ | Tìm alternative (MetaSound), giữ pillar, mất ít chất lượng |
| Pal Activity FSM port từ RoN khó hơn dự đoán 3x | Cắt số Activity từ 8 → 4, giữ chất lượng |
| Closed beta tester nói "không hiểu Bond" | UI tutorial mạnh hơn, không hủy mechanic |
| Quy mô team không kịp 52 tuần | Cắt 1 map, đẩy 1 Pal loài, beta vẫn 12 tháng |
| Nếu cắt sâu vẫn không kịp | Lùi beta sang 14 tháng — KHÔNG cắt pillar |

**Quy tắc bất di:** **KHÔNG cắt pillar.** Pillar là viễn cảnh — cắt = mất hồn game. Mọi thứ khác cắt được.

---

## VII. Template để áp cho dự án tiếp theo

Khi user/bạn muốn bắt đầu game khác:

```markdown
# [TÊN GAME] — Vision Document

## I. Một Câu Pitch (10 giây)
> [Một dòng]

## II. The Scene (cảnh bán game)
[500–1000 chữ ngôi thứ 2]

## III. 3 Game Pillars
1. **Pillar 1 — [Tên]:** "[Định nghĩa]"
2. **Pillar 2 — [Tên]:** "[Định nghĩa]"
3. **Pillar 3 — [Tên]:** "[Định nghĩa]"

## IV. Tại Sao Game Này Chưa Tồn Tại
[Phân tích so với game thị trường gần nhất]

## V. Phạm Vi Beta (Sẽ KHÔNG có)
- ❌ [Feature 1] → đẩy [version]
- ❌ [Feature 2] → đẩy [version]
...

## VI. Tại Sao Realistic
- ✅ Đã có [nguồn 1]
- ✅ Đã có [nguồn 2]
...

## VII. Mỗi Sáng Để Làm Gì
[Hook về daily routine]

## VIII. Câu Tự Hỏi Mỗi Sáng
> *"Hôm nay tôi làm gì để cảnh ở mục II xảy ra thực sự?"*

## IX. Tài Liệu Liên Quan
- 01-GameDesign.md
- 02-Pillar_Coverage.md
- 03-Roadmap_1_Year.md
- 04-Resource_Map.md

## X. Tuyên Bố Cuối Cùng
[1 đoạn ngắn về tại sao game này tồn tại]
```

---

## VIII. Câu trích nguồn cảm hứng

> *"Plan ngắn hạn cho 1 năm. Plan 10 năm cho cảm xúc."*  
> — Aphorism game dev (đặt làm tagline)

> *"Bạn không thiếu thời gian. Bạn thiếu focus."*  
> — Lý do tài liệu này tồn tại

> *"Mỗi ngày 0.1%, mỗi tuần 0.7%, mỗi tháng 3%, 1 năm 36%. Beta đủ rồi."*  
> — Toán học của persistence

---

## IX. Tài Liệu Liên Quan

- [01-Donchitos_Applied.md](01-Donchitos_Applied.md) — Workflow Donchitos cụ thể.
- [../Donchitos_GameStudios_Framework.md](../Donchitos_GameStudios_Framework.md) — Framework tóm tắt.
- [../PALDARK/00-VISION.md](../PALDARK/00-VISION.md) — Ví dụ áp dụng đầy đủ.
