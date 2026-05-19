# P01 — C++ & Build Foundation

> Tự soạn từ: [12] Pro UE Game Coding, [01] ActionRoguelike, [08] AWS GameLift, [13] Crunch.
>
> Pillar đầu tiên — không có nó, mọi pillar khác fail compile.

---

## Pillar Identity

**Cái gì:** Viết được Unreal C++ idiomatic. Hiểu module/build system, header tool, GC, smart pointer, log category.

**Scope:**
- ✅ UCLASS / USTRUCT / UENUM macro chain.
- ✅ UPROPERTY / UFUNCTION specifier.
- ✅ Module + `*.Build.cs` dependency.
- ✅ GarbageCollection — khi nào dùng UObject vs raw struct.
- ✅ Smart pointer: `TWeakObjectPtr`, `TSharedPtr`, `TStrongObjectPtr`.
- ✅ Log category + Insights basic.
- ✅ Server vs Client vs Editor build target.

**Out of scope (cho pillar khác):**
- ❌ Class lifetime (P02).
- ❌ Component composition (P03).
- ❌ Build server packaging (P07).

---

## PALDARK cần gì từ pillar này?

| Cần | Lý do |
|-----|-------|
| Khai báo UCLASS đúng | Tránh GC bug + reflection sai |
| `*.Build.cs` cho `Paldark` + `PaldarkServer` + `PaldarkEditor` | 3 target khác nhau |
| Log category `LogPaldark`, `LogPalAI`, `LogReplication` | Debug từng hệ thống riêng |
| Smart pointer cho Pal reference | Pal có thể respawn, weak ref tránh dangling |

---

## Sources Matrix

| Source | Coverage | Lesson lấy từ đó |
|--------|----------|------------------|
| [12] Pro UE Coding | 🟢 | L1 Editor + VS + Git, L2 Coding standard |
| [01] ActionRoguelike | 🟡 | L3 Module + Build.cs, L4 Log Category |
| [07] CPP MP Crash | 🟡 | L5 Server target |
| [08] AWS GameLift | 🟡 | L6 Server build packaging |

---

## Prerequisite

**Không có** — đây là pillar gốc.

**Tools cần cài:**
- UE 5.3+
- Visual Studio 2022 (Workload: Game development with C++).
- Git + Git LFS (cho asset binary).
- Rider (optional, faster intellisense).

---

## Lessons

### L1 — Setup project + Visual Studio 📖🧪

**Prerequisite:** Tools đã cài.

**Goal:** Tạo project C++ chạy được, hiểu cấu trúc folder.

**Concept (10 phút đọc):**
- `Source/<ProjectName>/<ProjectName>.Build.cs` — module config.
- `Source/<ProjectName>/<ProjectName>.cpp` + `.h` — module entry.
- `Source/<ProjectName>/<ProjectName>GameModeBase.h` — default game mode.
- `<ProjectName>.uproject` — engine version + module list + plugin.
- `Config/Default*.ini` — runtime config.

**Source learning path:**
- [12] Pro UE § Chapter 1 — Editor walkthrough.
- [12] Pro UE § Chapter 2 — Coding standard.

**API / Class chính:**
- `IMPLEMENT_PRIMARY_GAME_MODULE` macro.
- `FDefaultGameModuleImpl` — module factory.

**Practice exercise (1 hour):**
1. Tạo blank C++ project tên `PaldarkLab` (lab dùng để thử nghiệm, không phải PALDARK chính).
2. Build trong VS — đảm bảo compile clean.
3. Mở solution → xem `PaldarkLab.Build.cs` → liệt kê 5 module mặc định.

**PALDARK take-away:**
- 🟢 PALDARK chính có 3 module: `Paldark` (gameplay client+server), `PaldarkServer` (dedicated only logic), `PaldarkEditor` (editor extension).

**Apply ở week:** Roadmap week 1.

---

### L2 — Coding standard (Epic style) 📖

**Prerequisite:** L1.

**Goal:** Viết được code naming/format đúng UE style.

**Concept:**
- Prefix: `A` Actor, `U` UObject, `F` Struct, `I` Interface, `E` Enum, `T` Template, `S` Slate.
- PascalCase cho class, method, public member.
- Member `b` prefix cho bool: `bIsAlive`.
- Pointer style: `Type* Name` (asterisk hugs type).
- Comments: Doxygen `/**  */`.

**Source learning path:**
- [12] Pro UE § Chapter 2 — Coding standard full.
- [Epic Coding Standard docs](https://docs.unrealengine.com/5.3/en-US/epic-cplusplus-coding-standard-for-unreal-engine/).

**Practice exercise:**
1. Lấy 1 file của [01] ActionRoguelike (vd `SCharacter.h`) → đối chiếu naming có đúng standard không.
2. Tự viết 1 class `APaldarkLabPawn` theo đúng style.

**PALDARK take-away:**
- 🟢 Prefix `APaldark*`, `UPaldark*`, `FPaldark*` cho mọi class.
- 🟢 Setup `.clang-format` + EditorConfig theo Epic style.

**Apply ở week:** Roadmap week 1.

---

### L3 — Module + Build.cs dependency 🧪

**Prerequisite:** L1.

**Goal:** Thêm 1 module mới + manage public/private dependency.

**Concept:**
- `PublicDependencyModuleNames` — module mà module khác inherit khi link tới mình.
- `PrivateDependencyModuleNames` — chỉ mình dùng, không leak.
- `PCHUsage` — pre-compiled header strategy.
- `bEnableExceptions = true/false`.

**Source learning path:**
- [01] ActionRoguelike `ActionRoguelike.Build.cs` — chuẩn cho project size mid.
- [04] ReadyOrNot `Source/ReadyOrNot/ReadyOrNot.Build.cs` (qua GDD `04-ReadyOrNot_GDD.md` § Phase 6) — chuẩn AAA với nhiều dependency.

**API / Class chính:**
- `ReadOnlyTargetRules`
- `ModuleRules` base class.

**Practice exercise (2 hour):**
1. Trong `PaldarkLab`, tạo module mới `PaldarkLabCombat`.
2. Trong `PaldarkLab.uproject` thêm `"Modules": [...]`.
3. Module `PaldarkLab` `PublicDependencyModuleNames` thêm `"PaldarkLabCombat"`.
4. Build — nếu fail, đọc log để hiểu.

**PALDARK take-away:**
- 🟢 PALDARK có 3 module — `Paldark.Build.cs` ref `["Core", "CoreUObject", "Engine", "GameplayAbilities", "GameplayTags", "GameplayTasks", "UMG", "EnhancedInput", "AIModule", "GameFeatures", "ModularGameplay"]`.
- 🟡 Sau khi adopt Lyra (week 33+) sẽ thêm Lyra module deps.

**Apply ở week:** Roadmap week 1.

---

### L4 — Log Category + Console Command 🧪

**Prerequisite:** L3.

**Goal:** Tạo log riêng cho từng hệ thống, console command để toggle.

**Concept:**
- `DECLARE_LOG_CATEGORY_EXTERN` ở `.h`.
- `DEFINE_LOG_CATEGORY` ở `.cpp`.
- `UE_LOG(LogCategory, Verbosity, TEXT("..."))` — verbose: Fatal/Error/Warning/Display/Log/Verbose/VeryVerbose.
- `IConsoleCommand` — register console command.

**Source learning path:**
- [01] ActionRoguelike — `SCharacter.cpp` có `LogTemp`, sau đó custom `LogActionRoguelike`.
- [04] RoN — custom log cho từng manager.

**Practice exercise:**
1. Tạo `LogPaldarkLab` category.
2. Log ở BeginPlay + EndPlay character.
3. Tạo console command `paldark.SpawnTestPal` chạy func spawn dummy.

**PALDARK take-away:**
- 🟢 PALDARK có sẵn 5 category từ week 1: `LogPaldark`, `LogPalAI`, `LogReplication`, `LogPalCombat`, `LogPalBackend`.

**Apply ở week:** Roadmap week 1.

---

### L5 — GC + Smart Pointer 📖🧪

**Prerequisite:** L1.

**Goal:** Hiểu khi nào UObject sẽ bị GC, dùng đúng smart pointer.

**Concept:**
- UObject chỉ "sống" nếu có hard reference từ root object (rooted GameInstance, World, etc).
- `UPROPERTY()` member trên UObject = hard ref, GC sẽ track.
- `UObject*` không UPROPERTY → GC không track → dangling pointer.
- `TWeakObjectPtr<UObject>` — weak ref, không giữ alive, check `IsValid()`.
- `TSharedPtr<T>` — non-UObject (như FStruct, plain class).
- `TStrongObjectPtr<T>` — UObject mạnh nhưng nằm trong non-UObject struct (rare case).

**Source learning path:**
- [12] Pro UE § Chapter on smart pointer.
- [01] ActionRoguelike `SExplosiveBarrel.h` — UPROPERTY OnHit handle.

**API / Class chính:**
- `UPROPERTY()` various specifier: `EditAnywhere`, `BlueprintReadOnly`, `Replicated`, `VisibleAnywhere`.
- `Cast<T>(UObject*)`.
- `IsValid()`.

**Practice exercise:**
1. Tạo `APaldarkLabPickup` có `UPROPERTY() UStaticMeshComponent* Mesh;`.
2. Trong destructor `LogTemp::Warning` — confirm GC chạy khi pickup destroyed.
3. Tạo manager actor giữ `TArray<TWeakObjectPtr<APaldarkLabPickup>>` — pickup destroy thì array auto-clean (`IsValid` returns false).

**PALDARK take-away:**
- 🟢 Pal reference từ Player phải là `TWeakObjectPtr<APaldarkPal>` (Pal có thể die/respawn, không giữ chặt).
- 🟢 Manager Subsystem giữ list active player là `TArray<TWeakObjectPtr<APaldarkCharacter>>`.
- 🔴 KHÔNG dùng raw `UObject*` không UPROPERTY — sẽ dangling sau GC.

**Apply ở week:** Roadmap week 2, mỗi tuần khi viết class mới.

---

### L6 — Server / Client / Editor build target 🧪

**Prerequisite:** L3.

**Goal:** Build 3 target khác nhau, hiểu macro `WITH_SERVER_CODE`, `WITH_EDITOR`.

**Concept:**
- Target file `*.Target.cs` — define build type.
- `TargetType.Game` — client+listen server.
- `TargetType.Server` — dedicated server.
- `TargetType.Editor` — editor with plugin.
- `#if WITH_SERVER_CODE` — code chỉ compile vào server.
- `#if WITH_EDITOR` — code chỉ compile vào editor.
- `#if UE_BUILD_SHIPPING` — ship build, không log verbose.

**Source learning path:**
- [08] AWS GameLift § Chapter "Build Server target".
- [13] Crunch — Dockerfile dùng Server target.

**Practice exercise:**
1. Trong `PaldarkLab`, tạo `PaldarkLabServer.Target.cs` với `Type = TargetType.Server`.
2. Build cả 2 target: Editor + Server.
3. Trong code, dùng `if (HasAuthority())` + `#if WITH_SERVER_CODE` để wrap server-only logic.

**PALDARK take-away:**
- 🟢 3 target: `Paldark.Target.cs` (Game), `PaldarkServer.Target.cs` (Server), `PaldarkEditor.Target.cs` (Editor).
- 🟢 Code coordinator-side wrap `#if WITH_SERVER_CODE`.

**Apply ở week:** Roadmap week 1 (Editor) + week 25 (Server cho GameLift).

---

## ⚡ Capstone Exercise

**Goal:** Build "PaldarkLab" sandbox với foundation hoàn chỉnh.

**Yêu cầu:**
1. Project `PaldarkLab` C++ blank.
2. 3 module: `PaldarkLab`, `PaldarkLabCombat`, `PaldarkLabUI`.
3. 3 target: Game + Server + Editor — build clean cả 3.
4. 5 log category: `LogPaldarkLab`, `LogPaldarkLabAI`, `LogPaldarkLabReplication`, `LogPaldarkLabCombat`, `LogPaldarkLabBackend`.
5. 1 console command: `paldarklab.PrintHello` log "Hello".
6. 1 actor `APaldarkLabPickup` với UPROPERTY mesh + Manager subsystem giữ `TWeakObjectPtr` list.
7. Spawn 3 pickup, destroy 1 → manager log "Pickup gone".

**Acceptance:** Editor + Server build pass; log thấy "Pickup gone" sau destroy.

**Effort:** 2 ngày.

---

## Khi apply vào PALDARK roadmap

| Week | Pillar P01 usage |
|------|------------------|
| W1 | Setup project + 3 module + 5 log category |
| W2 | UPROPERTY/smart pointer cho Pal weak ref |
| W25 | Build Server target cho GameLift container |
| W40 | Build Shipping target cho beta launch |

---

## Anti-patterns (cảnh báo)

- 🔴 Raw `UObject*` không UPROPERTY → dangling.
- 🔴 `using namespace std;` toàn cục — clash với UE namespace.
- 🔴 STL `std::vector`, `std::map` thay vì `TArray`, `TMap` (UE GC compatibility).
- 🔴 `new`/`delete` trên UObject — phải `NewObject<T>()` / `T::StaticClass()`.
- 🔴 Tự sửa file `*.generated.h` — auto-generated, sẽ bị overwrite.

---

## Tham chiếu

- [`Documents/UE5_Core_Pillars.md`](../UE5_Core_Pillars.md) § P1.
- [`Documents/Courses/12-Udemy-ue4-pro-unreal-engine-game-coding.md`](../Courses/12-Udemy-ue4-pro-unreal-engine-game-coding.md).
- [`Documents/GameDesign/12-Pro_UE_GDD.md`](../GameDesign/12-Pro_UE_GDD.md).
- [`Documents/PALDARK/03-Roadmap_1_Year.md`](../PALDARK/03-Roadmap_1_Year.md) — Week 1-2.
