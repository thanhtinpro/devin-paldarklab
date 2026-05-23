# P16 — Math / Physics / Audio Foundation

> Tự soạn từ: [12] Pro UE Coding, [04] ReadyOrNot.
>
> Foundation cho mọi pillar khác — vector/quaternion ở mọi nơi, FMOD cho audio, physics cho ragdoll/projectile.

---

## Pillar Identity

**Cái gì:** Math (vector, quaternion, lerp), Physics (sub-stepping, ragdoll), Audio (FMOD propagation), Camera, Time.

**Scope:**
- ✅ FVector / FRotator / FQuat / FTransform.
- ✅ Dot / Cross product.
- ✅ Lerp / Slerp / FInterpTo / Easing curves.
- ✅ Physics sub-stepping (high-speed projectile).
- ✅ Chaos Vehicle (out of beta scope, ref only).
- ✅ FMOD Studio integration.
- ✅ Audio propagation + occlusion.
- ✅ Camera math (FOV, depth of field).
- ✅ Time scale + Time Dilation.

**Out of scope:**
- ❌ Insights profile (P15).
- ❌ Animation root motion (P05).

---

## PALDARK cần gì từ pillar này?

| Need | Solution |
|------|----------|
| Pal lunge attack land precise | Motion Warping (P05) + Lerp/Slerp |
| Bullet ballistic drop (long range) | Sub-stepping physics |
| Audio fog atmosphere | FMOD bus + occlusion |
| Camera shake on hit | FMatineeCameraShake |
| Slow-mo on Pal kill | Global Time Dilation |
| Pal AI line-of-sight | Dot product + trace |

---

## Sources Matrix

| Source | Coverage | Lesson lấy từ đó |
|--------|----------|------------------|
| [12] Pro UE | 🟢 | L1 Vector math, L2 Rotator/Quat, L3 Lerp/Easing, L4 Sub-stepping, L5 Audio basics, L6 Camera, L7 Time |
| [04] RoN GDD | 🟠 | L8 FMOD propagation (advanced) |

---

## Prerequisite

- P01 (UE C++ basics).

---

## Lessons

### L1 — Vector Math: Dot + Cross 🧪

**Goal:** Compute angle, project, check facing.

**Concept:**
- `FVector::DotProduct(A, B)` → scalar, measures alignment.
  - `1` = same direction; `0` = perpendicular; `-1` = opposite.
  - Common use: `FVector::DotProduct(MyForward, ToTarget) > 0.7` → target in front (~45°).
- `FVector::CrossProduct(A, B)` → perpendicular vector.
  - Use: compute up axis, normal of plane.
- `Vector.Size()` length, `Vector.GetSafeNormal()` unit vector.

**Source learning path:**
- [12] Pro UE § Vector Math chapter.

**Practice exercise (2 hour):**
1. AI `IsTargetInView(Target)` use DotProduct of forward × toTarget normalized > cos(60°).
2. Compute LookAtRotation via `FRotationMatrix::MakeFromX(ToTarget).Rotator()`.

**PALDARK take-away:**
- 🟢 Pal "facing player" check → DotProduct.
- 🟢 Bullet "behind player" check → DotProduct.
- 🟢 AI cone vision → DotProduct + distance.

**Apply ở week:** Roadmap week 8 (AI sense).

---

### L2 — Rotator + Quaternion 📖🧪

**Goal:** Hiểu trade-off Rotator vs Quaternion.

**Concept:**
- `FRotator` (Pitch, Yaw, Roll) — human-readable, gimbal lock danger.
- `FQuat` (X, Y, Z, W) — no gimbal, smooth Slerp.
- Convert: `Rotator.Quaternion()` ↔ `Quat.Rotator()`.
- Multiply: `Quat * Vector` rotate vector.
- Slerp: `FQuat::Slerp(A, B, Alpha)` smooth interpolation.

**Source learning path:**
- [12] Pro UE § Quaternion chapter.

**Practice exercise:**
1. Aim Offset blend with Quat Slerp instead of Rotator FInterpTo → check no gimbal lock.

**PALDARK take-away:**
- 🟢 Camera rotation: use Quat Slerp for smooth.
- 🟢 Pal turn-to-target: Quat Slerp not Rotator FInterpTo.

**Apply ở week:** Roadmap week 9 (camera polish).

---

### L3 — Lerp + FInterpTo + Easing 🧪

**Goal:** Smooth value transition over time.

**Concept:**
- `FMath::Lerp(A, B, Alpha)` linear.
- `FMath::FInterpTo(Current, Target, DeltaTime, Speed)` rate-based, framerate-independent.
- Easing curves: EaseInOut, EaseOut, Cubic — `FMath::InterpEaseIn(A, B, Alpha, Exp)`.
- Better: `UCurveFloat` asset → designer tune easing.

**Source learning path:**
- [12] Pro UE § Motion chapter.

**Practice exercise:**
1. Camera FOV zoom in: `FOV = FInterpTo(CurrentFOV, 60.0f, DT, 8.0f)`.
2. UI fade-in: alpha `Lerp(0, 1, Alpha)` with `UCurveFloat`.

**PALDARK take-away:**
- 🟢 Camera zoom on aim → FInterpTo.
- 🟢 UI fade-in/out → CurveFloat easing.
- 🟢 Pal trust level change → Lerp visual indicator.

**Apply ở week:** Roadmap week 9 + 19.

---

### L4 — Sub-stepping Physics 📖🧪

**Goal:** High-speed projectile không tunnel xuyên wall.

**Concept:**
- Default physics step 1/30s. High-speed object (bullet 1000m/s) moves 33m/step → tunneling.
- Sub-stepping: physics tick chia thành N sub-step per frame.
- `Project Settings → Physics → Substepping enabled, Max delta 0.0166, Max sub-steps 6`.
- Trade-off: CPU cost ↑.

**Source learning path:**
- [12] Pro UE § Physics chapter.

**PALDARK take-away:**
- 🟢 Sub-stepping enabled cho beta (4-player MP fine).
- 🟡 OR: use line trace projectile (no physics simulation), trace each frame.
- 🟢 Beta: line trace cho fast bullet + sub-stepping cho explosive arc.

**Apply ở week:** Roadmap week 17.

---

### L5 — Audio basics (FMOD or built-in) 📖🧪

**Goal:** Bus structure + spatialization + cue.

**Concept:**
- Audio Bus: Master → Music + SFX + Voice + Ambient.
- Spatialization: 3D audio falloff (linear, log, custom curve).
- Sound Cue (built-in UE) or FMOD Studio (industry standard).
- Audio Volume actor for reverb zone.

**Source learning path:**
- [12] Pro UE § Audio chapter.

**API / Class chính:**
- `USoundCue`, `USoundBase`, `UAudioComponent`.
- `UGameplayStatics::PlaySoundAtLocation`.
- FMOD: `UFMODEvent`, `UFMODAudioComponent`.

**Practice exercise:**
1. Setup 4 bus: Master/Music/SFX/Voice.
2. Footstep sound 3D spatialized.
3. Reverb zone in cave.

**PALDARK take-away:**
- 🟢 5 bus: Master/Music/SFX/Voice/Ambient/Pal.
- 🟢 FMOD Studio (or Wwise) — beta scope decide.
- 🟢 Pal voice → unique bus (volume slider).

**Apply ở week:** Roadmap week 28-29.

---

### L6 — Camera math 📖🧪

**Goal:** Camera shake, FOV zoom, depth of field.

**Concept:**
- `UCameraComponent` on Pawn or PlayerController.
- `UMatineeCameraShake` for hit/explosion feedback.
- FOV math: distance to target.
- Depth of Field: aperture + focal distance.

**Source learning path:**
- [12] Pro UE § Camera chapter.

**PALDARK take-away:**
- 🟢 Camera shake on weapon fire + Pal roar + explosion.
- 🟢 ADS (aim down sights) FOV zoom 90 → 60.

**Apply ở week:** Roadmap week 17.

---

### L7 — Time + Time Dilation 📖

**Goal:** Slow-mo on kill, freeze on death.

**Concept:**
- `UGameplayStatics::SetGlobalTimeDilation(World, 0.3f)` → slow-mo.
- `Actor->CustomTimeDilation = 2.0f` → speed up for individual actor (player still normal, enemy fast).

**Source learning path:**
- [12] Pro UE § Time chapter.

**PALDARK take-away:**
- 🟢 Slow-mo 0.3x for 1s on last-kill of raid.
- 🟢 Slow-mo 0.5x for 2s on bond ritual complete.

**Apply ở week:** Roadmap week 27.

---

### L8 — FMOD Audio Propagation (RoN advanced) 📖

**Goal:** Audio occlusion by wall + room reverb.

**Concept:**
- Trace from source to listener → measure wall thickness → LPF cutoff.
- Room ID detection → trigger reverb FMOD snapshot.
- Smooth between sample (avoid pop).

**Source learning path:**
- [04] RoN GDD § Phase 2 audio.

**PALDARK take-away:**
- 🟢 Beta: simple FMOD occlusion (trace check, binary).
- 🟢 Post-beta: full propagation (smooth LPF).

**Apply ở week:** Roadmap week 29.

---

## ⚡ Capstone Exercise

**Goal:** Mini-shooter scene tuning math/physics/audio.

**Yêu cầu:**
1. AI vision cone via DotProduct.
2. Camera FOV interp on ADS via FInterpTo.
3. Bullet sub-stepping enabled, 1000m/s no tunnel.
4. 5 audio bus + footstep 3D + reverb zone.
5. Slow-mo on last enemy kill.

**Acceptance:** All features visible and tunable in editor.

**Effort:** 1 tuần.

---

## Khi apply vào PALDARK roadmap

| Week | Pillar P16 usage |
|------|------------------|
| W8 | AI vision DotProduct |
| W9 | Camera Quat Slerp + FOV interp |
| W17 | Bullet sub-stepping + ADS FOV + camera shake |
| W27 | Slow-mo time dilation |
| W28-29 | FMOD bus + propagation |

---

## Anti-patterns (cảnh báo)

- 🔴 Rotator FInterpTo for camera → gimbal lock.
- 🔴 Lerp without DeltaTime aware → framerate-dependent.
- 🔴 Bullet no sub-stepping at high speed → tunnel.
- 🔴 1 audio cue file, no bus → mix nightmare.
- 🔴 Global Time Dilation 0 → game freeze, recover broken.

---

## Tham chiếu

- [`Documents/UE5_Core_Pillars.md`](../UE5_Core_Pillars.md) § P16.
- [`Documents/GameDesign/12-Pro_UE_GDD.md`](../GameDesign/12-Pro_UE_GDD.md) — Math/Physics/Audio chapters.
- [`Documents/GameDesign/04-ReadyOrNot_GDD.md`](../GameDesign/04-ReadyOrNot_GDD.md) — Audio propagation.
