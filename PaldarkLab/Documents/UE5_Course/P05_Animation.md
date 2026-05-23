# P05 — Animation

> Tự soạn từ: [01] ActionRoguelike, [10] Blaster, [13] Crunch, [15] Lyra RPG.
>
> Animation = trải nghiệm "AAA feel". Distance Matching + AnimNotify + Motion Warping = professional.

---

## Pillar Identity

**Cái gì:** Build Animation Blueprint, State Machine, Montage, RootMotion, AnimNotify, Distance Matching, Motion Warping.

**Scope:**
- ✅ AnimBP + EventGraph + AnimGraph.
- ✅ State Machine (Idle/Walk/Run/Jump/Crouch).
- ✅ Animation Montage (combo, reload, fire).
- ✅ AnimNotify + AnimNotifyState (trigger event mid-anim).
- ✅ RootMotion + RootMotionSource (jump attack, dodge).
- ✅ Distance Matching (Lyra) — step length match velocity.
- ✅ Motion Warping (move to target during animation).
- ✅ Aim Offset (recoil + aim direction).

**Out of scope:**
- ❌ AnimationBlueprint compile/build (P01).
- ❌ Audio sync ở animation (P16).

---

## PALDARK cần gì từ pillar này?

| Use case | Solution |
|----------|----------|
| Player walk/run smooth (no foot slide) | Distance Matching |
| Reload weapon | Animation Montage + AnimNotify "MagOut/MagIn/Release" |
| Pal cast ability | Montage + AnimNotify "Cast" (trigger GA effect) |
| Player aim offset (look up/down) | Aim Offset BlendSpace |
| Pal attack jump to target | Motion Warping |
| Death ragdoll | OnDeath → Physics Asset enable ragdoll |
| Civilian fearful behavior | Crouch montage on threat |
| Bond ritual animation (hold E 1.5s) | Montage with AnimNotifyState "BondingActive" |

---

## Sources Matrix

| Source | Coverage | Lesson lấy từ đó |
|--------|----------|------------------|
| [01] ActionRoguelike | 🟡 | L1 AnimBP basic + AnimNotify |
| [10] Blaster | 🟢 | L2 Aim Offset + Recoil + Reload Montage |
| [13] Crunch | 🟢 | L3 Combo system với AnimMontage chain |
| [15] Lyra RPG | 🟢 | L4 Distance Matching + Motion Warping |
| [04] RoN GDD | 🟠 | L5 Ragdoll + Cover Lean ref |

---

## Prerequisite

- P01 (C++ AnimBP class).
- P02 (Character class).
- P03 (Component composition — AnimInstance là sub-actor).

---

## Lessons

### L1 — AnimBP + State Machine + AnimNotify 🧪

**Goal:** Char chuyển state Idle→Walk→Run smooth, AnimNotify trigger event.

**Concept:**
- AnimBP có 2 graph: EventGraph (update variable) + AnimGraph (sample anim).
- State Machine: state + transition rule.
- BlendSpace 1D: 1 axis (speed) → idle/walk/run.
- AnimNotify: trigger func mid-anim (e.g., footstep, mag eject).
- AnimNotifyState: duration trigger (begin + tick + end).

**Source learning path:**
- [01] ActionRoguelike — `ABP_PlayerCharacter` (BP example).
- [10] Blaster — `ABP_BlasterCharacter` C++ subclass.

**API / Class chính:**
- `UAnimInstance::NativeUpdateAnimation`
- `UAnimNotify::Notify(USkeletalMeshComponent*, UAnimSequenceBase*)`
- `BlueprintCallable` AnimBP variable.

**Practice exercise (3 hour):**
1. Tạo `UPaldarkLabAnimInstance` C++ subclass.
2. Update mỗi tick `Speed` + `IsInAir` từ owner Character.
3. AnimBP State Machine: Idle ↔ Walk ↔ Run theo Speed.
4. AnimNotify `Footstep` trigger log "footstep" mỗi 2 step.

**PALDARK take-away:**
- 🟢 `UPaldarkAnimInstance` base cho mọi character.
- 🟢 `AnimNotify_Scent` trigger emit scent ở foot (P09 AI sense).

**Apply ở week:** Roadmap week 5 + 8.

---

### L2 — Aim Offset + Recoil + Reload Montage (Blaster) 🧪

**Goal:** Char nhìn lên xuống (Aim Offset), bắn gun có recoil, reload animation đúng.

**Concept:**
- Aim Offset = BlendSpace 2D (Pitch axis + Yaw axis) → blend pose look direction.
- Recoil = procedural rotation lên trên + side jitter.
- Reload Montage: section MagOut → MagIn → Release.
- AnimNotify `MagOutNotify` ở section đầu để hide magazine mesh.

**Source learning path:**
- [10] Blaster `ABP_BlasterCharacter` Aim Offset setup.

**Practice exercise (4 hour):**
1. Tạo Aim Offset BlendSpace với pitch -90 → +90.
2. AnimBP layer aim offset onto base.
3. Reload Montage 3 section + AnimNotify `MagOut/MagIn/Reloaded`.
4. C++ `Reload()` play montage, listen AnimNotify để swap mesh visibility.

**PALDARK take-away:**
- 🟢 Player Aim Offset cho weapon shooting.
- 🟢 Reload Montage cho mỗi weapon (Pistol/Rifle/Shotgun) + AnimNotify trigger ammo logic.

**Apply ở week:** Roadmap week 17.

---

### L3 — Combo Montage chain (Crunch) 🧪

**Goal:** Press attack 3 lần liên tiếp → combo Q→W→E damage tăng.

**Concept:**
- Mỗi attack 1 Montage.
- AnimNotifyState `ComboWindow` — active từ frame 30 → frame 60.
- Trong window, nếu player press attack lần nữa → trigger next Montage section.
- Hết window không press → reset combo.

**Source learning path:**
- [13] Crunch § Chapter combo.

**Practice exercise:**
1. 3 Montage Attack_1/2/3.
2. Mỗi Montage có AnimNotifyState `ComboWindow`.
3. C++ `OnComboWindowBegin/End` set flag, `Attack()` check flag → play next.

**PALDARK take-away:**
- 🟡 PALDARK melee không phải combo focus; chỉ Pal có combo (Bite → Maul → Roar).

**Apply ở week:** Roadmap week 27 (Pal combat).

---

### L4 — Distance Matching + Motion Warping (Lyra) 🧪

**Goal:** Step length match velocity (no foot slide); Pal jump to target precise location.

**Concept:**
- **Distance Matching:** AnimBP node "Distance Match" sample animation frame theo distance traveled, not by playback time → step land exactly on ground tick.
- **Motion Warping:** override RootMotion translation/rotation to align with target — e.g., Pal attack jump exact lands on enemy.

**Source learning path:**
- [15] Lyra RPG § Locomotion chapter.
- [Epic Motion Warping doc](https://docs.unrealengine.com/5.3/en-US/motion-warping-in-unreal-engine/).

**API / Class chính:**
- `UMotionWarpingComponent::AddOrUpdateWarpTargetFromTransform(Name, Transform)`
- Distance Matching node ở AnimGraph.

**Practice exercise (4 hour):**
1. Setup AnimBP distance matching cho Walk/Run.
2. PIE → so sánh foot slide before/after.
3. Pal AnimMontage `LungeAttack` với Motion Warping target.
4. AI call `MotionWarpingComp->AddOrUpdateWarpTargetFromTransform("Target", EnemyXform)` → lunge attack lands exact.

**PALDARK take-away:**
- 🟢 **CORE** — PALDARK Player + Pal locomotion dùng Distance Matching.
- 🟢 Pal lunge/jump attack dùng Motion Warping cho hit accuracy.

**Apply ở week:** Roadmap week 9 (locomotion polish) + week 27 (Pal attack).

---

### L5 — Ragdoll + Cover Lean (RoN ref) 📖

**Goal:** Hiểu ragdoll on-death + cover lean blend.

**Concept:**
- Ragdoll: `SetSimulatePhysics(true)` trên skeletal mesh → physics asset takes over.
- Cover lean: AnimBP có pose lean (StateMachine entry "CoverLean") + IK foot adjust.
- Blend từ animated → ragdoll smooth dùng `SetPhysicsBlendWeight(0..1)`.

**Source learning path:**
- [04] RoN GDD § Phase 2 (anim system).

**PALDARK take-away:**
- 🟡 Beta scope: ragdoll on-death đủ. Cover lean defer post-beta.

**Apply ở week:** Roadmap week 14 (death anim).

---

## ⚡ Capstone Exercise

**Goal:** Player char với full anim stack.

**Yêu cầu:**
1. AnimBP `ABP_PaldarkLabChar` C++ subclass.
2. State Machine Idle/Walk/Run/Jump/Crouch/Falling.
3. Aim Offset BlendSpace pitch.
4. Reload Montage với 3 AnimNotify.
5. Distance Matching trong Walk/Run.
6. AnimNotify `Footstep` → log mỗi step + emit scent component.
7. On-death → ragdoll.

**Acceptance:** PIE 30 phút không foot slide; reload play đúng AnimNotify.

**Effort:** 4 ngày.

---

## Khi apply vào PALDARK roadmap

| Week | Pillar P05 usage |
|------|------------------|
| W5 | Pal AnimBP basic state machine |
| W9 | Player Distance Matching locomotion |
| W14 | Death ragdoll |
| W17 | Weapon Aim Offset + Reload Montage |
| W27 | Pal Motion Warping attack |

---

## Anti-patterns (cảnh báo)

- 🔴 BP-only AnimBP cho mọi logic (chậm tick) — C++ AnimInstance subclass cho performance.
- 🔴 RootMotion lung tung không tắt — thiếu controllability.
- 🔴 AnimNotify gọi backend RPC — phải gọi local component, không network call mỗi notify.
- 🔴 Foot slide không fix → trải nghiệm cheap.
- 🔴 Reload Montage 1-shot không AnimNotify → ammo timing không sync với anim.

---

## Tham chiếu

- [`Documents/UE5_Core_Pillars.md`](../UE5_Core_Pillars.md) § P5.
- [`Documents/GameDesign/10-MP_Shooter_GDD.md`](../GameDesign/10-MP_Shooter_GDD.md).
- [`Documents/GameDesign/15-Lyra_RPG_GDD.md`](../GameDesign/15-Lyra_RPG_GDD.md).
