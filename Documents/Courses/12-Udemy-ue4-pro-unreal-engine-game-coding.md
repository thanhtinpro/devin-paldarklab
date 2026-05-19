# 12. Pro Unreal Engine Game Coding (UE4)

| | |
|---|---|
| **Tier** | T0 — Foundation (bắt buộc trước mọi khóa khác) |
| **Provider** | Udemy |
| **Link** | https://www.udemy.com/course/pro-unreal-engine-game-coding/ |
| **Trong repo** | `12.Udemy-ue4-pro-unreal-engine-game-coding/Documents/` (chỉ subtitle, không Source) |

---

## 1. Mục tiêu khóa

Khóa **kỹ năng nền** cho lập trình viên game Unreal: math, motion, physics, AI cơ bản, audio, camera. **Engine UE4 nhưng 90% concept giữ nguyên ở UE5.**

---

## 2. Lộ trình giảng dạy (14 chương)

| Chương | Cốt lõi (đọc từ subtitle) |
|--------|---------------------------|
| 01 - Getting Started | What is Unreal Engine, Epic Launcher, stock vs custom engine, **source build**, Visual Studio, version control, basic terminology, exploring editor, C++ vs Blueprints, coding standard |
| 02 - Metal in motion, part I | Movement basics, velocity, acceleration |
| 03 - **Game mathematics fundamentals** | Math primer, evaluation order, degrees/radians, ratios, trig, Abs/Sign, Pow, **Interpolation**, easing in/out, time-based smoothing, RNG, **Vectors, Dot/Cross**, **Quaternions**, Rotators, Transforms, coordinate spaces, matrices |
| 04 - Metal in motion, part II | Advanced movement, momentum |
| 05 - Vehicle dynamics, part I | Chaos Vehicles primer |
| 06 - Time and timing | DeltaTime, fixed step, FixedTimestep, time dilation |
| 07 - Graphics | Render basics, material primer, Niagara intro |
| 08 - Vehicle dynamics, part II | Suspension, tire friction, downforce |
| 09 - Audio | Sound Cue, attenuation, sound class, Concurrency |
| 10 - Artificial intelligence | BT, Blackboard, EQS basics |
| 11 - Vehicle dynamics, part III | Tuning, transmission |
| 12 - Pickups | Spawn pattern, dispatch, replicate |
| 13 - Camera work | Spring arm, lag, FoV, post-process |
| 14 - Bringing it all together | Mini-game wrap-up |

---

## 3. Cốt lõi (Pillars)

| Pillar | Mức |
|--------|-----|
| P1 C++/Build | 🟠 (toàn chương 1: editor, source build, VS, Git, standard) |
| P16 Math/Physics/Vehicle/Audio | 🔴 (chương 3 + 5,8,11 + 6 + 7 + 9) |
| P9 AI | 🟢 (chương 10 nhập môn) |
| P15 Performance | 🟡 (chương 7 graphics + 6 timing) |
| P2 Core Framework | 🟢 (chạm khẽ ở chương 12,14) |

---

## 4. Approach sư phạm

- **Cover-the-basics:** mỗi chương là 1 chủ đề độc lập, không phải build 1 game xuyên suốt.
- **Math + Physics đi đôi:** chương 3 math sau đó áp ngay vào chương 4–5 motion + vehicle.
- **Long-form lecture:** mỗi video dài, đầy theory. Khác với [05] GAS Crash quick demo.
- **Engine-agnostic concept:** vectors/easing/RNG/audio attenuation đúng cho UE4/UE5/Unity/Godot. Đây là khóa có **giá trị bền nhất** trong 12 khóa.

---

## 5. Đầu ra học viên

- Biết source-build engine (cần thiết cho [08] dedicated server).
- Hiểu vector/dot/cross/quaternion đủ để debug game-feel issue.
- Hiểu được Chaos Vehicle ở mức tuning cơ bản.
- Setup được audio attenuation đúng cho thirstperson/firstperson.

---

## 6. Lưu ý

- **UE4 nhưng concept dùng cho UE5.** Code Chaos Vehicle ở UE5 đã khác — chỉ học concept.
- **Không có Source code** trong repo, chỉ subtitle.
- **Không cover GAS** — quá sớm cho học viên T0.
- **Không cover multiplayer** — đợi [07].
- **AI chương 10 chỉ nhập môn** — phải lên [01] Tom + [11] Aura cho AI thật.

---

## 7. Liên kết khóa khác

| Trước | Sau |
|-------|-----|
| (không) — đây là entry point | Tất cả các khóa khác |
