PALDARK W35-36 — /Game/Paldark/Tame/ directory.

This directory hosts:
  - DA_Item_PalSphere_T1/T2/T3.uasset (UPaldarkItemDefinition) — the three
    sphere tiers consumed by `UPaldarkGameplayAbility_UsePalSphere`.
  - GE_StunOnHit.uasset (UGameplayEffect) — instant magnitude bump on Stun
    when a hostile-Pal attack lands. Designer-authored later.
  - GE_StunDecay.uasset (UGameplayEffect) — periodic drain on Stun so the
    bar doesn't stay full forever. Designer-authored later.

Note: this directory ships as a placeholder in the C++ scaffold PR (W35-36).
The actual .uasset content is designer-authored work after the C++ scaffold
compiles. Per-species `TameDifficulty` + `BaseCaptureProbability` live on
the matching `DA_PalDef_*` data asset (see /Game/Paldark/Pals/ — W27-28).

Designer authoring contract (W35-36):

  1. Author 3 Pal Sphere item data assets under /Game/Paldark/Items/ (or
     here under /Game/Paldark/Tame/ — designer convention):
       DA_Item_PalSphere_T1   (tag Paldark.Item.PalSphere.T1, x1 multiplier).
       DA_Item_PalSphere_T2   (tag Paldark.Item.PalSphere.T2, x1.5 multiplier).
       DA_Item_PalSphere_T3   (tag Paldark.Item.PalSphere.T3, x2 multiplier).
     Weight + Stack fragments per W11-12 convention; the tier multiplier
     itself is hard-coded in `UPaldarkPalTameComponent::GetPalSphereTierMultiplier`.

  2. Author `GA_UsePalSphere.uasset` Blueprint subclass of
     `UPaldarkGameplayAbility_UsePalSphere`:
       PalSphereTierTag = Paldark.Item.PalSphere.T1 (or T2 / T3).
       PalSphereClass   = APaldarkPalSphere (or a designer Blueprint subclass
                          that gives it a sphere mesh + throw montage).
     Grant via `UPaldarkAbilitySet_Player` so the player auto-receives it
     on possess.

  3. Wire `TameDifficulty` + `BaseCaptureProbability` on the per-species
     `DA_PalDef_*` data asset under /Game/Paldark/Pals/:
       DA_PalDef_Direhound   -> TameDifficulty=1.0  BaseCaptureProbability=0.35.
       DA_PalDef_Razorbird   -> TameDifficulty=1.2  BaseCaptureProbability=0.30.
       DA_PalDef_Stoneclad   -> TameDifficulty=1.8  BaseCaptureProbability=0.20.
       DA_PalDef_Vinewraith  -> TameDifficulty=1.4  BaseCaptureProbability=0.25.
       DA_PalDef_Boltmane    -> TameDifficulty=3.0  BaseCaptureProbability=0.10.
     The Pal's `UPaldarkPalTameComponent` async-loads the definition on
     first overlap and copies these fields into its own UPROPERTYs so the
     formula doesn't re-resolve the definition every frame.

  4. Author `GE_StunOnHit.uasset` (instant magnitude on Stun, scaled by
     `Paldark.SetByCaller.PalDamage` * 0.5) and `GE_StunDecay.uasset`
     (periodic -5/sec) under /Game/Paldark/Tame/. Hook to the existing
     Pal combat path via the W18-19 `UPaldarkPalCombatComponent`.

  5. Set `BondXPCurve` on player-side companion BP defaults (optional):
       BP_Pal_Foxparks    -> BondXPCurve with key (1, 100), (5, 600),
                             (10, 2000), (20, 10000).
       BP_Pal_Tombat      -> same curve baseline (designer copy).
     Leave empty for the linear 100 XP/level fallback.

Test loop (UE Editor, after compile):
  1. Author the assets above.
  2. PIE Raid_Sandbox.
  3. Spawn a Direhound: `Paldark.Pal.SpawnTestCompanion 0`.
     (Or hand-place via the W31-32 spawn flow.)
  4. Verify formula inputs:
     - `Paldark.Tame.SetStun Direhound_0 0.5` — stun bar at 50%.
     - Damage the Pal until HP is ~30% (use `Paldark.Gas.Damage` test cmd
       if available).
  5. Throw a Pal Sphere via the GA: pressed input that activates
     `Paldark.Ability.UsePalSphere` (auto-bound from the action set).
  6. Watch `LogPaldarkPal` — expect a `BeginTameAttempt … rolled=X.XX P=Y.YY result=Success/Fail`
     log line per throw.
  7. On Success: Pal destroys, `Paldark.Tame.DumpRoster` prints the new
     entry. On Fail (RNG miss): stun drains 30% MaxStun and the Pal stays.
  8. Force-tame for fast iteration: `Paldark.Tame.Force Direhound_0`.
  9. Grant bond XP manually: `Paldark.Pal.Bond.AddXP Direhound_0 100`
     (bumps level by 1 with the default 100 XP/level fallback curve).

Save-game persistence of the roster across raids is W47 — for W35-36 the
roster lives in memory only and resets on match-end.
