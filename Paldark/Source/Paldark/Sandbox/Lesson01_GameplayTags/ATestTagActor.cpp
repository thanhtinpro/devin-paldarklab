#include "ATestTagActor.h"
#include "SandboxTags.h"
#include "SandboxLogCategories.h"
#include "GameplayTagsManager.h"
#include "GameplayTagContainer.h"

DEFINE_LOG_CATEGORY(LogSandboxGeneral);
DEFINE_LOG_CATEGORY(LogSandboxCombat);

void ATestTagActor::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogSandboxGeneral, Log, TEXT("=== LESSON 01: GameplayTags + Log Categories ==="));

    Test01_NativeTagExists();
    Test02_NativeTagCompareCheap();
    Test03_StringLookup_TypoFailsSilently();
    // Test04 is a compile-time test — see LESSON_01 doc
    Test05_TagHierarchy_ParentChildMatch();
    Test06_LogCategory_FilterBySubsystem();
    Test07_TagContainer_HasTag();

    UE_LOG(LogSandboxGeneral, Log, TEXT("=== END LESSON 01 ==="));
}

void ATestTagActor::Test01_NativeTagExists()
{
    // FLOW: UE_DEFINE_GAMEPLAY_TAG auto-registers at module load
    // ASSERT: Tag handle IsValid() == true without any manual registration
    FGameplayTag TankTag = FGameplayTag(SandboxTags::TAG_Sandbox_Role_Tank);
    bool bValid = TankTag.IsValid();
    LogResult(TEXT("Test01_NativeTagExists"), bValid,
        FString::Printf(TEXT("TAG_Sandbox_Role_Tank.IsValid() = %s"), bValid ? TEXT("true") : TEXT("false")));
}

void ATestTagActor::Test02_NativeTagCompareCheap()
{
    // FLOW: Two references to the same native tag compare by index, not string
    // WHY: FGameplayTag::operator== uses FGameplayTagNetIndex (uint16) internally
    FGameplayTag A = FGameplayTag(SandboxTags::TAG_Sandbox_Role_Tank);
    FGameplayTag B = FGameplayTag(SandboxTags::TAG_Sandbox_Role_Tank);
    FGameplayTag C = FGameplayTag(SandboxTags::TAG_Sandbox_Role_Healer);
    bool bSame = (A == B);
    bool bDiff = (A != C);
    LogResult(TEXT("Test02_NativeTagCompareCheap"), bSame && bDiff,
        FString::Printf(TEXT("Tank==Tank: %s, Tank!=Healer: %s"), bSame ? TEXT("true") : TEXT("false"), bDiff ? TEXT("true") : TEXT("false")));
}

void ATestTagActor::Test03_StringLookup_TypoFailsSilently()
{
    // FLOW: FGameplayTag::RequestGameplayTag with a typo returns invalid tag — NO compile error
    // WHY: This is the failure mode native tags prevent
    FGameplayTag TypoTag = FGameplayTag::RequestGameplayTag(FName("Sandbox.Role.Tnk")); // typo!
    bool bInvalid = !TypoTag.IsValid();
    LogResult(TEXT("Test03_StringLookup_TypoFails"), bInvalid,
        FString::Printf(TEXT("Typo tag 'Sandbox.Role.Tnk' IsValid() = %s (expected false)"),
            TypoTag.IsValid() ? TEXT("true") : TEXT("false")));
}

void ATestTagActor::Test04_NativeTag_TypoFailsAtCompile()
{
    // FLOW: This test is intentionally left empty
    // WHY: To demonstrate compile-time error, you must manually uncomment the line below and build
    // FGameplayTag TypoTag = SandboxTags::TAG_Sandbox_Role_Tnk; // This will NOT compile!
    LogResult(TEXT("Test04_NativeTag_TypoFailsAtCompile"), true, TEXT("See LESSON_01.md for manual test"));
}

void ATestTagActor::Test05_TagHierarchy_ParentChildMatch()
{
    // FLOW: "Sandbox.Role.Tank".MatchesTag("Sandbox.Role") == true (parent match)
    // WHY: Hierarchy lets you query "any role?" without listing all children
    FGameplayTag ParentTag = FGameplayTag::RequestGameplayTag(FName("Sandbox.Role"));
    FGameplayTag TankTag = FGameplayTag(SandboxTags::TAG_Sandbox_Role_Tank);
    bool bMatches = TankTag.MatchesTag(ParentTag);
    LogResult(TEXT("Test05_TagHierarchy"), bMatches,
        FString::Printf(TEXT("Tank.MatchesTag(Sandbox.Role) = %s"), bMatches ? TEXT("true") : TEXT("false")));
}

void ATestTagActor::Test06_LogCategory_FilterBySubsystem()
{
    // FLOW: Different log categories let you filter output
    // WHY: In production, "Log LogSandboxCombat Verbose" shows only combat logs
    UE_LOG(LogSandboxGeneral, Log, TEXT("[General] This is a general message"));
    UE_LOG(LogSandboxCombat, Warning, TEXT("[Combat] This is a combat warning"));
    UE_LOG(LogSandboxCombat, Log, TEXT("[Combat] Damage applied: 50"));
    LogResult(TEXT("Test06_LogCategory"), true, TEXT("Check Output Log — 2 categories visible"));
}

void ATestTagActor::Test07_TagContainer_HasTag()
{
    // FLOW: FGameplayTagContainer holds multiple tags, query with HasTag/HasAny
    // WHY: Characters/Pawns carry a tag container representing their state
    FGameplayTagContainer Container;
    Container.AddTag(FGameplayTag(SandboxTags::TAG_Sandbox_Role_DPS));
    Container.AddTag(FGameplayTag(SandboxTags::TAG_Sandbox_State_Alive));

    bool bHasDPS = Container.HasTag(FGameplayTag(SandboxTags::TAG_Sandbox_Role_DPS));
    bool bNoDead = !Container.HasTag(FGameplayTag(SandboxTags::TAG_Sandbox_State_Dead));
    bool bHasAlive = Container.HasTag(FGameplayTag(SandboxTags::TAG_Sandbox_State_Alive));

    bool bPass = bHasDPS && bNoDead && bHasAlive;
    LogResult(TEXT("Test07_TagContainer"), bPass,
        FString::Printf(TEXT("HasDPS=%s, NoDead=%s, HasAlive=%s"),
            bHasDPS ? TEXT("Y") : TEXT("N"),
            bNoDead ? TEXT("Y") : TEXT("N"),
            bHasAlive ? TEXT("Y") : TEXT("N")));
}

void ATestTagActor::LogResult(const FString& TestName, bool bPassed, const FString& Detail)
{
    if (bPassed)
    {
        UE_LOG(LogSandboxGeneral, Log, TEXT("[PASS] %s — %s"), *TestName, *Detail);
    }
    else
    {
        UE_LOG(LogSandboxGeneral, Error, TEXT("[FAIL] %s — %s"), *TestName, *Detail);
    }
}
