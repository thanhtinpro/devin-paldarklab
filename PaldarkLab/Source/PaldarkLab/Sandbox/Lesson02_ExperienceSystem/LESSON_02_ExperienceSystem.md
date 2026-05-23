# Lesson 02 — Experience System

## Câu hỏi cốt lõi
> Tại sao cùng 1 map có thể chạy Raid (4 players, combat) hoặc Hub Town (8 players, trading) chỉ bằng cách đổi `?Experience=` trong URL?

## WHY — Không chỉ WHAT

### Vấn đề nếu hardcode rules vào GameMode
- Mỗi game mode (Raid, Hub, Tutorial) = 1 GameMode C++ class mới → code explosion
- Sửa max players = sửa code, rebuild, redeploy
- Designer không tự thay đổi được rules → bottleneck engineer

### Experience System giải quyết gì
- **Data-driven**: Rules nằm trong data asset (ExperienceDefinition), không phải code
- **Composition**: ActionSets additive — cùng base experience, thêm/bớt features qua ActionSet
- **Decouple**: Map ≠ Rules. TestMap + Raid = combat. TestMap + Hub = trading. Cùng binary, cùng map, khác behavior
- **Async**: Load experience qua AssetManager, không hitch khi map transition

### Flow cốt lõi (6 bước)
1. `InitGame(Options)` → gọi `RequestExperienceLoad(Options)`
2. Parse `?Experience=` từ URL, fallback to `DefaultExperience`
3. `LoadPrimaryAsset(AssetId)` async → callback `OnExperienceAssetReady`
4. `ApplyExperienceClassOverrides` — swap PC, PS class
5. `ApplyExperienceActionSets` — merge IntrinsicTags + ActionSet.GrantedTags
6. `GameState::SetCurrentExperience` → replicate ID to clients → `OnRep` → broadcast delegate

## Flow Diagram

```mermaid
sequenceDiagram
    participant GM as GameMode
    participant AM as AssetManager
    participant GS as GameState
    participant Client as Client

    GM->>GM: InitGame(Options)
    GM->>GM: RequestExperienceLoad(Options)
    GM->>GM: Parse ?Experience= from URL
    alt No URL option
        GM->>GM: Fallback to DefaultExperience
    end
    GM->>AM: LoadPrimaryAsset(ExperienceId)
    AM-->>GM: OnExperienceAssetReady(Experience)
    GM->>GM: ApplyExperienceClassOverrides(PC, PS)
    GM->>GM: ApplyExperienceActionSets(IntrinsicTags + ActionSet.GrantedTags)
    GM->>GM: Resolve PawnClass from PawnData
    GM->>GS: SetCurrentExperience(AssetId, Experience)
    GS-->>Client: OnRep_CurrentExperienceId
    Client->>Client: OnExperienceLoaded.Broadcast(Experience)
```

## Test Plan

| # | Test | Bước reproduce | PASS criteria |
|---|------|---------------|---------------|
| 1 | URL selects Raid | Spawn ATestExperienceActor, URL has ?Experience=RaidSandbox | Log: `[PASS] Test01_URLOption_SelectsRaid` |
| 2 | No URL → default | URL has no ?Experience= | Log: `[PASS] Test02_NoURLOption_FallbackToDefault` |
| 3 | Invalid experience | URL has ?Experience=NonExistent | Log: Warning + `[PASS] Test03_InvalidExperience_GracefulFallback` |
| 4 | Class overrides | Load Raid | Log: PC=RaidPlayerController, PS=RaidPlayerState |
| 5 | ActionSet merge tags | Load Raid | Log: Raid=Y Combat=Y LagComp=Y NoTrading=Y |
| 6 | Same map, diff rules | Load Raid then Hub | Log: different pawn, different tags |
| 7 | Delegate fires | Subscribe then load | Log: DelegateFired=Y |
| 8 | MaxPlayers per experience | Load Raid (4) and Hub (8) | Log: Raid=4 Hub=8 |

## Expected Output

```
LogSandboxExpTest: === LESSON 02: Experience System ===
LogSandboxExperience: [Registry] Registered experience 'RaidSandbox' (Raid Sandbox)
LogSandboxExperience: [Registry] Registered experience 'HubTown' (Hub Town)
LogSandboxExperience: [RequestLoad] URL resolved: 'RaidSandbox'
LogSandboxExperience: [RequestLoad] Async load started for 'RaidSandbox' (SANDBOX: synchronous).
LogSandboxExperience: [Override] PlayerController → RaidPlayerController
LogSandboxExperience: [Override] PlayerState → RaidPlayerState
LogSandboxExperience: [ActionSet] CombatLoop ActionSet
LogSandboxExperience: [Ready] Experience 'RaidSandbox' loaded. Pawn=PaldarkCharacter PC=RaidPlayerController PS=RaidPlayerState ActionSets=1 Tags=3
LogSandboxExpTest: [PASS] Test01_URLOption_SelectsRaid — ResolvedKey='RaidSandbox'
...
LogSandboxExpTest: [PASS] Test02_NoURLOption_FallbackToDefault — ResolvedKey='RaidSandbox' (expected 'RaidSandbox' as default)
...
LogSandboxExpTest: [PASS] Test03_InvalidExperience_GracefulFallback — LoadedExperience is nullptr (graceful fallback, check Warning log above)
...
LogSandboxExpTest: [PASS] Test04_ClassOverrides_PCandPS — PC=RaidPlayerController PS=RaidPlayerState
...
LogSandboxExpTest: [PASS] Test05_ActionSets_MergeTags — Raid=Y Combat=Y LagComp=Y NoTrading=Y
...
LogSandboxExpTest: [PASS] Test06_SameMap_DifferentRules — Raid:Combat=Y Hub:Trading=Y Hub:NoCombat=Y DiffPawn=Y (Raid=PaldarkCharacter, Hub=HubCharacter)
...
LogSandboxExpTest: [PASS] Test07_OnExperienceLoadedDelegate — DelegateFired=Y ExpName='Hub Town'
...
LogSandboxExpTest: [PASS] Test08_MaxPlayers — Raid=4 Hub=8
LogSandboxExpTest: === END LESSON 02 ===
```

## Mapping sang code thật

| Sandbox | Production |
|---------|-----------|
| `USandboxExperienceDefinition` | `UPaldarkExperienceDefinition` |
| `FSandboxActionSet` | `UPaldarkExperienceActionSet` |
| `USandboxExperienceManager` | `APaldarkGameModeBase` + `UPaldarkAssetManager` + `APaldarkGameStateBase` |
| `RegisterExperience()` | `DefaultGame.ini` PrimaryAssetTypesToScan |
| `RequestExperienceLoad()` | `APaldarkGameModeBase::RequestExperienceLoad()` |
| `OnExperienceAssetReady()` | `APaldarkGameModeBase::OnExperienceAssetReady()` |
| `ApplyClassOverrides()` | `APaldarkGameModeBase::ApplyExperienceClassOverrides()` |
| `ApplyActionSets()` | `APaldarkGameModeBase::ApplyExperienceActionSets()` |
| `OnExperienceLoaded delegate` | `APaldarkGameStateBase::OnExperienceLoaded` |
| String-based "PawnClassName" | `TSoftObjectPtr<UPaldarkPawnData>` → `PawnClass` |

## Sandbox files

| File | Vai trò |
|------|---------|
| `SandboxExpTags.h/.cpp` | Register Sandbox.Mode.* + Sandbox.Feature.* tags |
| `USandboxExperienceDefinition.h` | Data container (UObject thay vì UPrimaryDataAsset) |
| `USandboxExperienceManager.h/.cpp` | Gộp GameMode + AssetManager + GameState flow |
| `ATestExperienceActor.h/.cpp` | 8 test cases chạy trong BeginPlay |
