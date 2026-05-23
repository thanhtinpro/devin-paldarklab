# 12 — Pro Unreal Engine Game Coding (Donchitos Reverse GDD)

> Source: [Udemy — Pro Unreal Engine Game Coding](https://www.udemy.com/course/pro-unreal-engine-game-coding/).
>
> Course này **không phải 1 sample game** mà là **sandbox compendium** — 14 chương riêng biệt, mỗi chương 1 mini-system (math, physics, vehicle, audio, AI, camera, etc.).

---

## Identity Card

| Field | Value |
|-------|-------|
| 📦 Type | Course (textbook compendium) |
| Genre | Sandbox dạy nền tảng UE4/5 |
| Engine | UE4/5 |
| Đóng góp PALDARK | Reference textbook cho math/physics/vehicle/audio |

---

## Phase 1 — Brainstorm (reverse)

### Implicit pitch
> *"14 chương = 14 'aha'. Sau khóa bạn có vocabulary đầy đủ về math, physics, audio, AI để build bất kỳ game nào."*

### Cảm xúc cốt lõi
- **Compendium** — không phải gameplay loop mà là toolbox.
- **Foundation** — sau khóa bạn nói được "đây là sub-stepping" thay vì "physics bị giật".

---

## Phase 2 — Decompose

### 14 chương = 14 mini-system

```
Editor/VS/Git → Coding standard
↓
Math (vec/quat/matrix/lerp/easing)
↓
Motion (root motion, AnimNotify)
↓
Vehicle Part 1/2/3 (Chaos vehicle dynamics)
↓
Time (delta, sub-step, time dilation)
↓
Graphics (camera, post-process)
↓
Audio (FMOD basics, attenuation, occlusion)
↓
AI (BT/EQS intro)
↓
Camera (cinematic, spring arm)
↓
Pickup (game-style demo)
↓
Build / Packaging
```

---

## Phase 3 — Define (per-chapter mini-GDD)

### Vehicle (3 chương)
- Sample: Drivable car với suspension.
- Chaos Vehicle plugin.
- Engine torque curve.
- Steering + wheel friction.
- Animation rig wheel.

### Math
- Vector dot/cross, projection.
- Quaternion rotation.
- Lerp/Slerp.
- Easing curves.

### Audio
- FMOD Studio basics (event, parameter, snapshot).
- Distance attenuation.
- Occlusion via line trace.

### AI
- BT/EQS minimal.

### Time
- DeltaTime correctness.
- Sub-stepping for physics.
- Time dilation cho slow-mo.

---

## Phase 4 — 3 Game Pillars (inferred — applicable cho any game)

### Pillar 1 — Math is Mechanic
> *"Lerp không phải utility. Lerp là gameplay (smooth aim, smooth move)."*

### Pillar 2 — Physics has Personality
> *"Suspension stiffness = car character. Wheel friction = drift identity."*

### Pillar 3 — Audio is Feedback
> *"Sound = confirmation. Mỗi action có sound = trải nghiệm có trọng lượng."*

---

## Phase 5 — Architecture Decisions (implicit)

### ADR-1: FMOD Studio cho audio (không UE native)
- **Quyết định:** Author audio ở FMOD, runtime trigger qua plugin.
- **PALDARK adopt:** 🟡 ADR-006 cần quyết — FMOD vs MetaSound.

### ADR-2: Chaos Vehicle plugin cho vehicle
- **Quyết định:** UE Chaos thay vì legacy PhysX.
- **PALDARK adopt:** 🔴 Skip — beta scope không có vehicle.

### ADR-3: Sub-stepping cho physics fidelity
- **Quyết định:** Tick rate 60Hz physics, sub-step 4×.
- **PALDARK adopt:** 🟡 Tham chiếu nếu Pal ragdoll cần.

---

## Phase 6 — System Map

| Chương | Hệ thống | File approx |
|--------|----------|--------------|
| 1 | Setup | (config) |
| 2 | Coding standard | (docs) |
| 3 | Math examples | `Math/...` |
| 4 | Motion | `AnimNotify_*` |
| 5–7 | Vehicle | `Vehicle/*` |
| 8 | Time | `Time/*` |
| 9 | Graphics | `PostProcess/*` |
| 10 | Audio (FMOD) | `Audio/*` |
| 11 | AI intro | `AI/*` |
| 12 | Camera | `Camera/*` |
| 13 | Pickup | `Pickup/*` |
| 14 | Build | (packaging) |

---

## Phase 7 — Stories taught (14 phần)

| Phần | US |
|------|----|
| 1 | Là dev, tôi setup Editor + Visual Studio + Git |
| 2 | Là dev, tôi follow coding standard |
| 3 | Là dev, tôi hiểu Vector, Quaternion, Lerp, Slerp |
| 4 | Là dev, tôi dùng AnimNotify + Root Motion |
| 5–7 | Là dev, tôi build drivable vehicle với Chaos |
| 8 | Là dev, tôi handle DeltaTime + sub-step |
| 9 | Là dev, tôi setup post-process |
| 10 | Là dev, tôi tích hợp FMOD audio |
| 11 | Là dev, tôi viết BT/EQS đầu tiên |
| 12 | Là dev, tôi setup cinematic camera |
| 13 | Là dev, tôi viết Pickup actor |
| 14 | Là dev, tôi build / package |

---

## Take-aways cho PALDARK

### 🟢 Copy nguyên (Vocabulary)
- **Lerp/Slerp/easing** — Pal AI follow path.
- **Sub-stepping** — Pal ragdoll, knockback.
- **FMOD basics** — apply ADR-006 quyết định.

### 🔴 Skip (out of beta scope)
- **Vehicle** — PALDARK beta không có vehicle.

---

## Pillar Mapping

| Pillar | Đóng góp |
|--------|----------|
| P1 | 🟠 Coding standard |
| P16 | 🟢 **Math + Physics + Audio reference** |
| P5 | 🟠 AnimNotify + Root Motion |
| P9 | 🟠 BT/EQS intro |

---

## Tham chiếu

- [Courses/12-Udemy-ue4-pro-unreal-engine-game-coding.md](../Courses/12-Udemy-ue4-pro-unreal-engine-game-coding.md)
- Reference khi quyết định ADR-006 (audio engine).
