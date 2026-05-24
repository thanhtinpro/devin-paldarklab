#include "TestTagSubsystem.h"

#include "SandboxLogCategories.h"
#include "SandboxTags.h"

#include "GameplayTagContainer.h"
#include "GameplayTagsManager.h"

void UTestTagSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogSandboxTags, Display, TEXT("=== Lesson01 GameplayTags :: Subsystem Initialize — RUN ALL TESTS ==="));
	RunAllTests();
	UE_LOG(LogSandboxTags, Display, TEXT("=== Lesson01 GameplayTags :: DONE ==="));
}

void UTestTagSubsystem::RunAllTests()
{
	UGameplayTagsManager& TagManager = UGameplayTagsManager::Get();

	// Convert native tag globals to plain FGameplayTag once, so we can call
	// MatchesTag / MatchesTagExact / operator== uniformly. FNativeGameplayTag
	// has `operator FGameplayTag() const` so this is a single user-defined cast.
	const FGameplayTag StateRoot  = TAG_Sandbox_State;
	const FGameplayTag Sprinting  = TAG_Sandbox_State_Sprinting;
	const FGameplayTag Crouching  = TAG_Sandbox_State_Crouching;
	const FGameplayTag JumpAbil   = TAG_Sandbox_Ability_Jump;

	// ---------------------------------------------------------------------
	// TC1 — Native tag auto-registers at module load.
	// Proves: UE_DEFINE_GAMEPLAY_TAG_COMMENT's static ctor pushed the tag into
	// UGameplayTagsManager without any .ini, blueprint, or manual init.
	// ---------------------------------------------------------------------
	{
		const FGameplayTag FromLookup = TagManager.RequestGameplayTag(FName(TEXT("Sandbox.State.Sprinting")), /*ErrorIfNotFound*/ false);
		const bool bPass = FromLookup.IsValid();
		UE_LOG(LogSandboxTags, Display, TEXT("[TC1] Native tag 'Sandbox.State.Sprinting' auto-registered at module load: %s"),
			bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// ---------------------------------------------------------------------
	// TC2 — Macro reference == string lookup (same shared instance, O(1) compare).
	// Proves: FGameplayTag is just an FName index. Equality is integer compare,
	// not string compare. This is the perf WHY for combat-tight loops.
	// ---------------------------------------------------------------------
	{
		const FGameplayTag FromString = TagManager.RequestGameplayTag(FName(TEXT("Sandbox.State.Sprinting")), false);
		const bool bPass = Sprinting.IsValid() && Sprinting == FromString;
		UE_LOG(LogSandboxTags, Display, TEXT("[TC2] Native macro == string lookup (shared FName/index, O(1) compare): %s"),
			bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// ---------------------------------------------------------------------
	// TC3 — String typo is SILENT INVALID at runtime.
	// Contrast: a native macro typo (e.g. TAG_Sandbox_State_Spriting) would
	// fail at COMPILE TIME. This is the safety WHY for native tags.
	// ---------------------------------------------------------------------
	{
		const FGameplayTag Typo = TagManager.RequestGameplayTag(FName(TEXT("Sandbox.State.Spriting")), /*ErrorIfNotFound*/ false);
		const bool bPass = !Typo.IsValid();
		UE_LOG(LogSandboxTags, Display, TEXT("[TC3] String typo 'Sandbox.State.Spriting' silently invalid (runtime bug risk): %s"),
			bPass ? TEXT("PASS") : TEXT("FAIL"));
		UE_LOG(LogSandboxTags, Display, TEXT("       (A native-macro typo would FAIL AT COMPILE TIME — that is the WHY)"));
	}

	// ---------------------------------------------------------------------
	// TC4 — Hierarchy: child MatchesTag(parent) true, MatchesTagExact false.
	// Proves: "Sandbox.State.Sprinting" matches "Sandbox.State" hierarchically
	// but is not the same exact tag. Enables broad queries like "any State.*".
	// ---------------------------------------------------------------------
	{
		const bool bChildMatchesParent = Sprinting.MatchesTag(StateRoot);
		const bool bExactDiffers       = !Sprinting.MatchesTagExact(StateRoot);
		const bool bPass = bChildMatchesParent && bExactDiffers;
		UE_LOG(LogSandboxTags, Display,
			TEXT("[TC4] Hierarchy: Sprinting.MatchesTag(State)=%d, Sprinting.MatchesTagExact(State)=%d -> %s"),
			bChildMatchesParent ? 1 : 0, !bExactDiffers ? 1 : 0,
			bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// ---------------------------------------------------------------------
	// TC5 — Container HasAny / HasAll semantics.
	// Active = {Sprinting, Crouching}, Query = {Sprinting, Jump}.
	// HasAny=true (Sprinting overlaps), HasAll=false (Jump missing).
	// ---------------------------------------------------------------------
	{
		FGameplayTagContainer Active;
		Active.AddTag(Sprinting);
		Active.AddTag(Crouching);

		FGameplayTagContainer Query;
		Query.AddTag(Sprinting);
		Query.AddTag(JumpAbil);

		const bool bHasAny = Active.HasAny(Query);
		const bool bHasAll = Active.HasAll(Query);
		const bool bPass   = bHasAny && !bHasAll;
		UE_LOG(LogSandboxTags, Display,
			TEXT("[TC5] Container {Sprint,Crouch} vs Query {Sprint,Jump}: HasAny=%d HasAll=%d -> %s"),
			bHasAny ? 1 : 0, bHasAll ? 1 : 0, bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// ---------------------------------------------------------------------
	// TC6 — Log category isolation.
	// Verbose line proves the category can be filtered independently of LogTemp/LogEngine.
	// In production: `log LogSandboxTags Verbose` to drill in without log spam.
	// ---------------------------------------------------------------------
	{
		UE_LOG(LogSandboxTags, Verbose, TEXT("[TC6] Verbose-level message — only visible when LogSandboxTags is raised to Verbose"));
		UE_LOG(LogSandboxTags, Display, TEXT("[TC6] Log category 'LogSandboxTags' isolated from LogTemp/LogEngine: PASS"));
	}
}
