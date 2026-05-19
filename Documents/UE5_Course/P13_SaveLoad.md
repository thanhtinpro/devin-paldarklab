# P13 — Save / Load System

> Tự soạn từ: [01] Tom Looman, [11] Aura, [04] ReadyOrNot.
>
> Extraction game = persistent inventory across match. PALDARK Hub stash + Pal roster + skin must survive crash/exit.

---

## Pillar Identity

**Cái gì:** Persist player + world state qua session.

**Scope:**
- ✅ USaveGame subclass.
- ✅ Memory writer/reader serialization.
- ✅ Hybrid: local SaveGame for cosmetic + backend (DynamoDB) for authoritative persistence.
- ✅ Checkpoint pattern.
- ✅ Slot save/load.
- ✅ Replay-style serialize (FObjectAndNameAsStringProxyArchive).
- ✅ Auto-save timer.

**Out of scope:**
- ❌ Backend DynamoDB write (P18).
- ❌ Match replay system (not in beta scope).

---

## PALDARK cần gì từ pillar này?

| Data type | Where stored |
|-----------|--------------|
| Player progression (level, XP) | Local + Backend (DynamoDB) |
| Hub Stash (item bank) | Backend (authoritative — anti-cheat) |
| Pal Roster (owned Pal + level + nickname) | Backend |
| Customization (cosmetic) | Local |
| Settings (audio, graphic, key bind) | Local |
| Match history | Backend |
| Loadout preset | Local + Backend |
| Tutorial progress | Local |

---

## Sources Matrix

| Source | Coverage | Lesson lấy từ đó |
|--------|----------|------------------|
| [01] Tom Looman | 🟢 | L1 SaveGame basic, L2 Serialization archive, L3 Slot save |
| [11] Aura | 🟢 | L4 Checkpoint pattern, L5 Map entrance save |
| [04] RoN GDD | 🟠 | L6 Settings persistence pattern ref |
| [08] AWS GameLift / [13] Crunch | 🟡 | L7 Backend hybrid (P18 detail) |

---

## Prerequisite

- P01 (UCLASS, UPROPERTY).
- P12 (DataAsset — save reference soft path).

---

## Lessons

### L1 — USaveGame basic 🧪

**Goal:** Create save slot, write data, load back.

**Concept:**
- `USaveGame` subclass với UPROPERTY data.
- `UGameplayStatics::CreateSaveGameObject<T>()`.
- `UGameplayStatics::SaveGameToSlot(SaveObj, SlotName, UserIndex)`.
- `UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex)`.

**Source learning path:**
- [01] Tom Looman § Save Game chapter.

**API / Class chính:**
- `USaveGame`.
- `UGameplayStatics::SaveGameToSlot()`.
- `UGameplayStatics::DoesSaveGameExist()`.

**Practice exercise (2 hour):**
1. `UPaldarkLabSettingsSave : public USaveGame` với MasterVolume float + SfxVolume float.
2. `SaveSettings()` create + populate + SaveGameToSlot("Settings", 0).
3. `LoadSettings()` LoadGameFromSlot, fallback default if not exist.
4. PIE: change → exit → run → confirm reload.

**PALDARK take-away:**
- 🟢 `UPaldarkSettingsSave` cho audio/graphic/keybind.
- 🟢 `UPaldarkCustomizationSave` cho cosmetic choice.

**Apply ở week:** Roadmap week 32.

---

### L2 — Serialization Archive (Tom Looman) 🧪

**Goal:** Serialize transform + custom UPROPERTY of world actor.

**Concept:**
- `FMemoryWriter` write byte stream.
- `FObjectAndNameAsStringProxyArchive` — serialize UPROPERTY tagged `SaveGame`.
- `ActorIterator` → for each actor → serialize transform + tagged props.
- Load: spawn actor → deserialize → restore transform + props.

**Source learning path:**
- [01] Tom Looman § Save Game Serialization chapter.

**API / Class chính:**
- `FMemoryWriter Writer(SaveDataBytes, true);`
- `FObjectAndNameAsStringProxyArchive Ar(Writer, true);`
- `Actor->Serialize(Ar);`

**Practice exercise (3 hour):**
1. `UPaldarkLabWorldSave` chứa `TArray<FActorSaveData>` (ActorName + Transform + Bytes).
2. `SaveWorld()` iterate all actor có tag "Saveable" → serialize.
3. `LoadWorld()` deserialize → spawn → apply transform + bytes.

**PALDARK take-away:**
- 🟡 Beta: KHÔNG save world (raid is per-session, ephemeral).
- 🟢 Hub state save needed: Pal in Hub + decoration.

**Apply ở week:** Roadmap week 35-36 (Hub save).

---

### L3 — Slot Save + Multi-slot 🧪

**Goal:** Multiple save slot (Slot 1/2/3 + Auto-save).

**Concept:**
- `SaveGameToSlot(SlotName, UserIndex)` — SlotName arbitrary string.
- `EnumerateSaves` (custom) — list all slot files.
- Auto-save timer: every 5 min → save to "AutoSave" slot.

**Source learning path:**
- [01] Tom Looman + [11] Aura (Aura supports multi-slot).

**PALDARK take-away:**
- 🟢 1 slot per Steam account (PALDARK MP-only, no slot variety).
- 🟢 Auto-save every 60s in Hub.

**Apply ở week:** Roadmap week 32.

---

### L4 — Checkpoint pattern (Aura) 🧪

**Goal:** Save player at checkpoint actor.

**Concept:**
- `ACheckpointActor` placed in world.
- Player overlap → save state + currentCheckpointId.
- Death → respawn at last checkpoint, restore state.

**Source learning path:**
- [11] Aura § Checkpoint chapter.

**PALDARK take-away:**
- 🔴 PALDARK extraction game — không có checkpoint trong raid (death = lose all in raid).
- 🟡 Hub có "Last Hub Position" save.

**Apply ở week:** Skip cho beta.

---

### L5 — Map Entrance / World Transition (Aura) 🧪

**Goal:** Save which map player is in + spawn point.

**Concept:**
- `UAuraGameInstance` track `CurrentMap` + `LastSpawnPoint`.
- On `OpenLevel(NewMap)` save → load on new map → spawn at point.

**Source learning path:**
- [11] Aura § Map Entrance chapter.

**PALDARK take-away:**
- 🟡 Beta: Player luôn spawn ở Hub start point hoặc Raid spawn (random).
- 🟢 Pal team selection persists Hub → Raid.

**Apply ở week:** Roadmap week 32.

---

### L6 — Settings persistence (RoN ref) 📖

**Goal:** Audio/graphic/keybind persist.

**Concept:**
- RoN: `UGameUserSettings` extended for custom settings.
- Auto-apply on start.

**Source learning path:**
- [04] RoN GDD § Phase 2 settings.

**PALDARK take-away:**
- 🟢 `UPaldarkGameUserSettings` extend `UGameUserSettings` cho audio bus, FPS cap, keybind.

**Apply ở week:** Roadmap week 32.

---

### L7 — Hybrid: Local + Backend (DynamoDB) 📖

**Goal:** Authoritative data ở backend, cosmetic ở local.

**Concept:**
- Cosmetic (skin, decoration) — local SaveGame.
- Authoritative (currency, inventory, Pal roster, XP) — backend DynamoDB.
- On login → fetch from backend → cache local for offline display.
- On change → push to backend + cache local.

**Source learning path:**
- [08] AWS GameLift § DynamoDB chapter.
- [13] Crunch § Backend persistence chapter.
- Detail in P18.

**PALDARK take-away:**
- 🟢 Hub Stash + Pal Roster + Currency = Backend.
- 🟢 Customization + Settings = Local.

**Apply ở week:** Roadmap week 32 (local) + 39 (backend hybrid).

---

## ⚡ Capstone Exercise

**Goal:** PaldarkLab save/load full stack.

**Yêu cầu:**
1. `UPaldarkLabSettingsSave` — audio/graphic.
2. `UPaldarkLabPlayerSave` — XP, Pal team, equipped weapon.
3. `UPaldarkLabHubSave` — Pal in Hub, decoration position.
4. Auto-save every 60s.
5. Slot enumerate UI.
6. PIE: save → quit → restart → restore.

**Acceptance:** Restart preserves all data; no crash on missing slot.

**Effort:** 1 tuần.

---

## Khi apply vào PALDARK roadmap

| Week | Pillar P13 usage |
|------|------------------|
| W32 | Local SaveGame (settings, customization) |
| W35 | Hub save (Pal position, decoration) |
| W39 | Backend hybrid (Pal Roster, Stash, Currency) |

---

## Anti-patterns (cảnh báo)

- 🔴 Save authoritative data local — cheat exploit.
- 🔴 Sync save on every change → frame freeze. Batch + auto-save timer.
- 🔴 SaveGame chứa raw UObject ref — fail to serialize. Use soft ref or name.
- 🔴 No fallback when corrupt save → game crash.
- 🔴 No version field in SaveGame → upgrade-time data loss.

---

## Tham chiếu

- [`Documents/UE5_Core_Pillars.md`](../UE5_Core_Pillars.md) § P13.
- [`Documents/GameDesign/01-ActionRoguelike_GDD.md`](../GameDesign/01-ActionRoguelike_GDD.md) — Serialization pattern.
- [`Documents/GameDesign/11-Aura_RPG_GDD.md`](../GameDesign/11-Aura_RPG_GDD.md) — Checkpoint + Map.
