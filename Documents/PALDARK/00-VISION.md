# PALDARK — Vision Document

> Đây là tài liệu **không có dòng code nào**. Đây là viễn cảnh, là lý do tại sao mỗi ngày bạn thức dậy lại muốn cày tiếp.
>
> Nếu sau khi đọc bạn không thấy **nỗi da gà**, văn bản này thất bại — hãy nói lại để viết lại.

---

## I. Một Câu Pitch (10 giây)

> **PALDARK** là một game **co-op extraction shooter 4 người** trong thế giới hậu-Palworld, nơi bạn dắt 1 Pal vào **Vùng Tối** — 4×4km lãnh thổ bị nhiễm xạ thần kinh khiến Pal cổ đại hoá điên. Mỗi raid: **bắt thêm Pal mới, cướp tài nguyên, hoặc chết và mất hết.** Sống sót về căn cứ thì Pal vừa bắt trở thành huyết mạch vĩnh viễn của bạn.

Tarkov gặp Palworld gặp Ready or Not. Không ai từng làm. Vì khó. Vì đáng.

---

## II. Cảnh Sẽ Bán Game Này

> **Giả lập một buổi tối, năm 2026, beta vừa mở:**

Bạn và 3 người bạn vào hub town — một thị trấn 32-player shard nơi bạn lưu giữ Pal đã thuần và tài sản. Trời tối. Đèn neon hắt qua chiếc lồng chứa **Foxparks** — con Pal đầu tiên bạn bắt được tuần trước. Nó nhận ra bạn, đập đuôi.

Bạn vào **Briefing Room**. Map hôm nay: *"Cảng Bỏ Hoang"*. Mục tiêu cá nhân: bắt một con Pal hạng Hiếm — *Boltmane*. Tin đồn: có một bầy Boltmane phía bắc cảng.

Bạn chọn Foxparks đi cùng. Equip: súng AK + 60 viên + 2 lựu khói + Pal Sphere x10. Squad chia vai: bạn là Scout, Pal Foxparks là tank (nó thấp máu nhưng tankier hơn bạn), 1 thằng cầm sniper Pal là Tombat (Pal đêm có thể đánh dấu kẻ địch), 2 thằng còn lại assault.

Drop. Helicopter Pal-driven hạ cánh. Squad bung ra. **Im lặng.** Foxparks bám sát chân bạn — nó là Pal cấp 12, biết tự né đạn, biết breach cửa hé. Bạn vào hangar. Ba xác lính bandit đã chết. Có ai đó đã vào trước.

Foxparks nhe răng. Hướng đông. Bạn ra hiệu radial wheel → "Investigate". Foxparks chạy 30m, biến mất sau container. Tai bạn nghe gầm — **không phải Foxparks**. Bạn rút sang nấp. Đồng đội qua VOIP: "Có thấy gì không?"

Bạn không trả lời. Vì bạn nghe **tiếng móng vuốt** trên sắt. Không xa lắm. **Một Pal hoang dại đang stalk bạn.**

Foxparks về. Mặt nó… **hoảng.** Lần đầu tiên trong 12 cấp, Pal của bạn hoảng. Hệ AI Activity nó nhảy từ "Scout" sang "Cower". Nó nép vào chân bạn.

Bạn nhìn ra ngoài container. Không thấy gì. Chỉ thấy **một bóng đen** đang chậm rãi đi qua khoảng sáng — to bằng con ngựa, ánh mắt vàng. **Boltmane.** Không phải con bạn muốn bắt — con này là **Boltmane Alpha**. Cấp 30+. Một đòn của nó giết Pal của bạn.

Bạn đứng yên. Foxparks run. Đồng đội qua VOIP: "Anh ở đâu? Có vấn đề?"

Bạn nuốt nước bọt. Gõ map ping. Họ thấy chấm đỏ của bạn. Họ bắt đầu chạy về.

Boltmane Alpha dừng. **Nó quay đầu.** Nó **nghe** thấy đồng đội bạn đang chạy tới — qua **FMOD Audio Propagation** xuyên tường container.

Trong **20 giây tiếp theo**, bạn quyết định: bắn lén với hy vọng kéo aggro về phía mình → cứu đồng đội nhưng chết → mất Foxparks. Hoặc nín thở chờ → đồng đội chạy thẳng vào nó.

Foxparks nhìn bạn. **Nó hiểu.** Vì Pal trong PALDARK có *Bond Level* — và Foxparks cấp 12 với bạn nghĩa là **tin tưởng**. Nó nhảy lên vai bạn, hé miệng phun lửa nhỏ về phía đối diện — đánh lạc hướng. Boltmane quay đi.

Bạn chạy. Foxparks bám đuôi. Bạn báo VOIP: "Backup. Boltmane Alpha. Đông Hangar 4. CHẠY."

Khoảnh khắc này — **bạn không nghĩ về điểm. Không nghĩ về loot.** Bạn nghĩ về **giữ Foxparks sống.**

Đó là PALDARK.

---

## III. 3 Game Pillars (theo Donchitos)

> Mỗi quyết định game design phải đối chiếu lại 3 pillar này. Nếu không phục vụ chúng — cắt.

### 🩸 Pillar 1 — Pal Bond Trumps Gun

> *"Pal của bạn không phải vũ khí. Nó là bạn đồng hành. Mất nó còn đau hơn mất súng."*

- Súng tìm được khắp nơi. Pal đã thuần thì **không**.
- Pal có Bond Level (0–20). Bond cao mở ability mới: revive, scout vô hình, breach door, mark enemy.
- Pal **chết vĩnh viễn** nếu bị 1-shot, nhưng có thể được hồi sinh nếu kéo xác về extraction trước 5 phút.
- Pal khác nhau như tính cách thật: Foxparks nhút nhát nhưng dũng cảm dưới áp lực, Boltmane bướng nhưng trung thành nếu được tôn trọng.

### 🔊 Pillar 2 — Information is Survival

> *"Bạn chết vì bạn không nghe thấy. Bạn sống vì bạn nghe trước nó."*

- Âm thanh **truyền qua tường** (FMOD propagation từ RoN).
- Pal có giác quan riêng (sight, sound, scent) — chúng "alert" bạn trước.
- Map có **dấu vết** (vết chân, máu, lông Pal rơi) — đọc được = sống.
- HUD tối giản. Không minimap khi raid. Compass + ping system + Pal indicator.

### 🧬 Pillar 3 — Tame Once, Trust Forever

> *"Pal bạn bắt được hôm nay sẽ là legacy bạn truyền cho người khác."*

- Pal đã thuần lưu vĩnh viễn trong hub.
- Mỗi Pal có **gen** (Speed/Strength/Stealth/Smarts). Breed 2 Pal sinh con với gen kế thừa.
- **Pal lineage** truyền lại: bạn có thể tặng Pal cho người chơi khác, kèm chữ ký người tao gốc.
- Trong 1 năm beta, top server hub town có "Pal Hall of Fame" — Pal huyền thoại bắt từ raid khó nhất.

---

## IV. Tại Sao Game Này Chưa Tồn Tại

Nhìn vào thị trường 2025:

| Game | Cái thiếu để thành PALDARK |
|------|----------------------------|
| **Palworld** | Không có extraction, không có high-stakes, không co-op tactical |
| **Tarkov** | Không có companion AI sống động, không có hub social |
| **Ready or Not** | Không có collectibles legacy, không có open-world raid |
| **Helldivers 2** | Không có persistent Pal, AI bug như swarm chứ không như cá thể |
| **Monster Hunter** | Không có extraction stakes thực sự, hub không có shard |

Có nhiều game **gần**. Không có game **giao**. Vì để giao 3 trục:
1. Pal AI cá thể (RoN Activity FSM, không phải swarm AI)
2. Extraction stakes (Tarkov-style permadeath)
3. Persistent hub social (MMO-lite shard)

Cần 4 mảng kỹ thuật khác nhau — và chỉ với **15 nguồn tài liệu đã có** thì bạn **đã có hết**.

---

## V. Phạm Vi 1 Năm Beta (NHỎ + SÂU, không LỚN + NÔNG)

### Cắt bỏ ngay (không có trong beta):
- ❌ Open world 64km² (đợi v1.0)
- ❌ 100-player MMO (đợi v1.0)
- ❌ PvP raid (đợi v1.0)
- ❌ Crafting tree đầy đủ (chỉ có crafting cơ bản)
- ❌ Vehicle (đợi v1.0)

### Beta scope (12 tháng):
- ✅ **3 map** Dark Zone (mỗi map 1×1km — đủ căng thẳng, không cần lớn)
- ✅ **10 loài Pal** (5 thường, 4 hiếm, 1 huyền thoại)
- ✅ **4-player co-op + dedicated server AWS**
- ✅ **Hub town shard 8 người** (không phải 32 — vẫn đủ social)
- ✅ **Permadeath extraction loop** đầy đủ
- ✅ **Pal bond + breeding cơ bản** (4 stat gen, 2 generation deep)
- ✅ **AI Activity FSM** cho Pal companion và Pal hostile
- ✅ **Audio propagation FMOD**
- ✅ **15 vũ khí** (3 tier × 5 loại)

Đây là **PALDARK Beta**. Sống động, không trống. Đủ để hooked 100 người chơi đầu tiên.

---

## VI. Tại Sao 1 Năm Là Realistic

Bạn **không** xây từ con số 0. Bạn đã có:

- ✅ Palworld skeleton (Pal capture base, character framework)
- ✅ ReadyOrNot study (Activity AI, ThreatOctree, FMOD propagation, Significance) — playbook đã có
- ✅ PUBG-KI (lobby, dedicated server prototype, zone manager) — playbook đã có
- ✅ 12 khóa Udemy — coverage 18/18 pillar
- ✅ Donchitos framework — phương pháp luận game studio

Bạn cần **import**, không cần **invent**. Mỗi ngày học 0.1% = 100 ngày là 10%. 1 năm = 36.5% = đủ cho beta nếu scope đúng.

---

## VII. Mỗi Sáng Thức Dậy Để Làm Gì

Khi bạn mở terminal:

| Ngày trong roadmap | Việc cụ thể | Pillar liên quan |
|---------------------|--------------|------------------|
| Tuần 1 ngày 3 | Compile Palworld base, fix encoding BOM | P1 |
| Tuần 6 ngày 2 | Đọc ReadyOrNot `BaseActivity.cpp`, tóm pattern Activity FSM | P9 |
| Tuần 12 ngày 5 | Port Activity FSM vào Pal companion AI | P9 |
| Tuần 20 ngày 1 | Setup AWS GameLift Anywhere Fleet đầu tiên | P7 |
| Tuần 32 ngày 4 | Lyra Experience cho map "Cảng Bỏ Hoang" | P17 |
| Tuần 48 ngày 6 | Internal closed beta build đầu tiên | All |

Mỗi ngày 0.1%. Mỗi ngày một dòng code cụ thể có mục đích. **Không lang mang.**

---

## VIII. Một Câu Tự Hỏi Mỗi Sáng

> *"Hôm nay tôi làm gì để cảnh ở mục II xảy ra thực sự?"*

Nếu trả lời được — cày. Nếu không — đọc roadmap, tìm task tuần tự.

---

## IX. Tài Liệu Liên Quan

| File | Nội dung |
|------|----------|
| [01-GameDesign.md](01-GameDesign.md) | GDD đầy đủ theo Donchitos 7-phase |
| [02-Pillar_Coverage.md](02-Pillar_Coverage.md) | Bảng 18 pillar × PALDARK feature |
| [03-Roadmap_1_Year.md](03-Roadmap_1_Year.md) | 52 tuần với milestones tuần-từng-tuần |
| [04-Resource_Map.md](04-Resource_Map.md) | Pillar nào học từ source nào trong 15 nguồn |
| [../Methodology/00-Vision_Reverse_Engineering.md](../Methodology/00-Vision_Reverse_Engineering.md) | Tại sao bắt đầu từ vision |
| [../Methodology/01-Donchitos_Applied.md](../Methodology/01-Donchitos_Applied.md) | Workflow Donchitos cho mọi dự án |

---

## X. Tuyên Bố Cuối Cùng

PALDARK không phải là *cố gắng làm Palworld*. PALDARK là **kết quả của việc bạn học được 15 nguồn tài liệu này một cách có hệ thống**.

Mỗi cảnh trong mục II của tài liệu này:
- Pal hoảng = **Activity FSM (RoN)**
- Audio xuyên tường = **FMOD Propagation (RoN)**
- Boltmane stalk = **Custom AISense (RoN)**
- 4-player drop = **Dedicated Server (AWS course)**
- Foxparks cấp 12 = **GAS + Attribute Set (Aura, GAS Crash)**
- Hub town shard = **WorldSubsystem + Backend (Lyra + AWS)**
- Bond Level 20 = **DataAsset progression (Aura)**
- Extraction timer = **Zone Manager (PUBG)**
- Pal bond UI = **CommonUI radial (RoN + Lyra)**
- Foxparks breach door = **InteractInterface + Activity (RoN + Tom Looman)**

**Game này tồn tại được vì 15 nguồn này tồn tại.**

Bây giờ là lúc bắt đầu.
