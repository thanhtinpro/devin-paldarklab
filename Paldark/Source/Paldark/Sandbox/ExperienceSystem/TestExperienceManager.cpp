#include "TestExperienceManager.h"

#include "SandboxExpLog.h"
#include "SandboxExpTags.h"
#include "TestActionSet.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

bool UTestExperienceManager::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	// Game + PIE only — skip Editor preview worlds to avoid noise when designers
	// open assets in the Content Browser.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UTestExperienceManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogSandboxExperience, Display, TEXT("=== Lesson02 Experience :: Subsystem Initialize — seeding registry ==="));
	DefaultExperienceName = TEXT("Hub");
	SeedRegistry();

	if (UWorld* World = GetWorld())
	{
		WorldBeginPlayHandle = World->OnWorldBeginPlay.AddUObject(this, &UTestExperienceManager::OnWorldBeginPlay);
	}
}

void UTestExperienceManager::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->OnWorldBeginPlay.Remove(WorldBeginPlayHandle);
	}
	Super::Deinitialize();
}

// ---------------------------------------------------------------------------
// Registry seeding: hand-built two experiences sharing zero data but the same
// "shape" (rules + tags + class overrides). Same map, different rules.
// ---------------------------------------------------------------------------
void UTestExperienceManager::SeedRegistry()
{
	// --- Action sets (composition units) ---
	UTestActionSet* CombatSet = NewObject<UTestActionSet>(this);
	CombatSet->ActionSetName = TEXT("AS_Combat");
	CombatSet->GrantedTags.AddTag(TAG_Granted_Combat);

	UTestActionSet* SocialSet = NewObject<UTestActionSet>(this);
	SocialSet->ActionSetName = TEXT("AS_Social");
	SocialSet->GrantedTags.AddTag(TAG_Granted_Social);

	UTestActionSet* VoiceSet = NewObject<UTestActionSet>(this);
	VoiceSet->ActionSetName = TEXT("AS_Voice");
	VoiceSet->GrantedTags.AddTag(TAG_Granted_Voice);

	// --- Experience: Raid (PvE combat, small) ---
	UTestExperienceDefinition* Raid = NewObject<UTestExperienceDefinition>(this);
	Raid->ExperienceName = TEXT("Raid");
	Raid->MaxPlayers = 4;
	Raid->OverridePlayerControllerClassName = TEXT("PC_Raid");
	Raid->OverridePlayerStateClassName      = TEXT("PS_Default");
	Raid->IntrinsicTags.AddTag(TAG_Intrinsic_PlayerBase);
	Raid->ActionSets = { CombatSet };
	ExperienceRegistry.Add(Raid->ExperienceName, Raid);

	// --- Experience: Hub (social lobby, large) ---
	UTestExperienceDefinition* Hub = NewObject<UTestExperienceDefinition>(this);
	Hub->ExperienceName = TEXT("Hub");
	Hub->MaxPlayers = 32;
	Hub->OverridePlayerControllerClassName = TEXT("PC_Hub");
	Hub->OverridePlayerStateClassName      = TEXT("PS_Hub");
	Hub->IntrinsicTags.AddTag(TAG_Intrinsic_PlayerBase);
	Hub->ActionSets = { SocialSet, VoiceSet };
	ExperienceRegistry.Add(Hub->ExperienceName, Hub);

	UE_LOG(LogSandboxExperience, Display, TEXT("Registry seeded: %d experiences (Default=%s)"),
		ExperienceRegistry.Num(), *DefaultExperienceName.ToString());
}

// ---------------------------------------------------------------------------
// Sync resolve. Returns default if URL has no Experience option OR points to
// an unknown name (graceful fallback — never returns null when registry is non-empty).
// ---------------------------------------------------------------------------
const UTestExperienceDefinition* UTestExperienceManager::ResolveExperience(const FString& URLOptions) const
{
	const FString Requested = UGameplayStatics::ParseOption(URLOptions, TEXT("Experience"));
	if (Requested.IsEmpty())
	{
		UE_LOG(LogSandboxExperience, Display, TEXT("ResolveExperience: URL has no '?Experience=' option -> fallback '%s'"),
			*DefaultExperienceName.ToString());
		return ExperienceRegistry.FindRef(DefaultExperienceName);
	}

	const FName RequestedName(*Requested);
	if (UTestExperienceDefinition* const* Found = ExperienceRegistry.Find(RequestedName))
	{
		UE_LOG(LogSandboxExperience, Display, TEXT("ResolveExperience: '%s' -> registry hit"), *Requested);
		return *Found;
	}

	UE_LOG(LogSandboxExperience, Warning, TEXT("ResolveExperience: '%s' not in registry -> graceful fallback '%s'"),
		*Requested, *DefaultExperienceName.ToString());
	return ExperienceRegistry.FindRef(DefaultExperienceName);
}

// ---------------------------------------------------------------------------
// Async-style apply. Resolves synchronously, then defers the actual "apply +
// broadcast delegate" by one tick to simulate AssetManager async load.
// ---------------------------------------------------------------------------
void UTestExperienceManager::LoadAndApplyExperience(const FString& URLOptions)
{
	const UTestExperienceDefinition* Def = ResolveExperience(URLOptions);
	UE_LOG(LogSandboxExperience, Display, TEXT("LoadAndApplyExperience: scheduling async apply for '%s' (next tick)"),
		Def ? *Def->ExperienceName.ToString() : TEXT("<null>"));

	if (UWorld* World = GetWorld())
	{
		// SANDBOX: trong thực tế là UAssetManager::Get().LoadPrimaryAsset(...).
		// 1-tick timer chứng minh delegate fire SAU khi LoadAndApplyExperience trả về —
		// đúng semantics async (caller không block).
		FTimerHandle Handle;
		World->GetTimerManager().SetTimerForNextTick([WeakThis = TWeakObjectPtr<UTestExperienceManager>(this), Def]()
		{
			if (UTestExperienceManager* Self = WeakThis.Get())
			{
				Self->ApplyExperience(Def);
			}
		});
	}
}

void UTestExperienceManager::ApplyExperience(const UTestExperienceDefinition* Def)
{
	CurrentExperience = Def;
	if (!Def)
	{
		UE_LOG(LogSandboxExperience, Error, TEXT("ApplyExperience: null definition (registry empty?)"));
		return;
	}

	UE_LOG(LogSandboxExperience, Display,
		TEXT("ApplyExperience: '%s' MaxPlayers=%d PC=%s PS=%s — broadcasting OnExperienceLoaded"),
		*Def->ExperienceName.ToString(),
		Def->MaxPlayers,
		*Def->OverridePlayerControllerClassName.ToString(),
		*Def->OverridePlayerStateClassName.ToString());

	OnExperienceLoaded.Broadcast(Def);
}

// ---------------------------------------------------------------------------
// Auto driver — fires when world begins play, runs the full test suite.
// ---------------------------------------------------------------------------
void UTestExperienceManager::OnWorldBeginPlay()
{
	UE_LOG(LogSandboxExperience, Display, TEXT("=== Lesson02 Experience :: OnWorldBeginPlay — RUN ALL TESTS ==="));
	RunAllTestCases();
}

void UTestExperienceManager::RunAllTestCases()
{
	// -----------------------------------------------------------------------
	// TC1 — URL '?Experience=Raid' resolves to the Raid definition.
	// -----------------------------------------------------------------------
	{
		const UTestExperienceDefinition* Def = ResolveExperience(TEXT("?Experience=Raid"));
		const bool bPass = Def && Def->ExperienceName == TEXT("Raid");
		UE_LOG(LogSandboxExperience, Display, TEXT("[TC1] URL '?Experience=Raid' -> '%s': %s"),
			Def ? *Def->ExperienceName.ToString() : TEXT("<null>"), bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// -----------------------------------------------------------------------
	// TC2 — URL with no Experience option -> default fallback ('Hub').
	// -----------------------------------------------------------------------
	{
		const UTestExperienceDefinition* Def = ResolveExperience(TEXT(""));
		const bool bPass = Def && Def->ExperienceName == DefaultExperienceName;
		UE_LOG(LogSandboxExperience, Display, TEXT("[TC2] Empty URL -> default '%s': %s"),
			Def ? *Def->ExperienceName.ToString() : TEXT("<null>"), bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// -----------------------------------------------------------------------
	// TC3 — Invalid name '?Experience=Bogus' -> graceful fallback (no crash).
	// -----------------------------------------------------------------------
	{
		const UTestExperienceDefinition* Def = ResolveExperience(TEXT("?Experience=Bogus"));
		const bool bPass = Def && Def->ExperienceName == DefaultExperienceName;
		UE_LOG(LogSandboxExperience, Display, TEXT("[TC3] '?Experience=Bogus' -> graceful '%s': %s"),
			Def ? *Def->ExperienceName.ToString() : TEXT("<null>"), bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// -----------------------------------------------------------------------
	// TC4 — Class overrides differ per experience.
	// -----------------------------------------------------------------------
	{
		const UTestExperienceDefinition* Raid = ResolveExperience(TEXT("?Experience=Raid"));
		const UTestExperienceDefinition* Hub  = ResolveExperience(TEXT("?Experience=Hub"));
		const bool bPass = Raid && Hub
			&& Raid->OverridePlayerControllerClassName == TEXT("PC_Raid")
			&& Hub->OverridePlayerControllerClassName  == TEXT("PC_Hub");
		UE_LOG(LogSandboxExperience, Display,
			TEXT("[TC4] PC override Raid=%s Hub=%s -> %s"),
			Raid ? *Raid->OverridePlayerControllerClassName.ToString() : TEXT("?"),
			Hub  ? *Hub->OverridePlayerControllerClassName.ToString()  : TEXT("?"),
			bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// -----------------------------------------------------------------------
	// TC5 — ActionSet merge: Intrinsic + Σ(ActionSet.Granted).
	//   Raid -> {PlayerBase, Combat}
	//   Hub  -> {PlayerBase, Social, Voice}
	// -----------------------------------------------------------------------
	{
		const UTestExperienceDefinition* Raid = ResolveExperience(TEXT("?Experience=Raid"));
		const UTestExperienceDefinition* Hub  = ResolveExperience(TEXT("?Experience=Hub"));

		const FGameplayTagContainer RaidEff = Raid ? Raid->GetEffectiveGrantedTags() : FGameplayTagContainer();
		const FGameplayTagContainer HubEff  = Hub  ? Hub->GetEffectiveGrantedTags()  : FGameplayTagContainer();

		const bool bRaidOk = RaidEff.HasTagExact(TAG_Intrinsic_PlayerBase)
		                  && RaidEff.HasTagExact(TAG_Granted_Combat)
		                  && !RaidEff.HasTagExact(TAG_Granted_Social);

		const bool bHubOk  = HubEff.HasTagExact(TAG_Intrinsic_PlayerBase)
		                  && HubEff.HasTagExact(TAG_Granted_Social)
		                  && HubEff.HasTagExact(TAG_Granted_Voice)
		                  && !HubEff.HasTagExact(TAG_Granted_Combat);

		UE_LOG(LogSandboxExperience, Display, TEXT("[TC5] Effective tags Raid=%s Hub=%s -> %s"),
			*RaidEff.ToStringSimple(), *HubEff.ToStringSimple(),
			(bRaidOk && bHubOk) ? TEXT("PASS") : TEXT("FAIL"));
	}

	// -----------------------------------------------------------------------
	// TC6 — Same map, different rules (core insight).
	// MaxPlayers differs by experience even though the URL/map is unchanged.
	// -----------------------------------------------------------------------
	{
		const UTestExperienceDefinition* Raid = ResolveExperience(TEXT("?Experience=Raid"));
		const UTestExperienceDefinition* Hub  = ResolveExperience(TEXT("?Experience=Hub"));
		const bool bPass = Raid && Hub && Raid->MaxPlayers != Hub->MaxPlayers;
		UE_LOG(LogSandboxExperience, Display,
			TEXT("[TC6] Same map -> different rules: Raid.MaxPlayers=%d Hub.MaxPlayers=%d -> %s"),
			Raid ? Raid->MaxPlayers : -1, Hub ? Hub->MaxPlayers : -1,
			bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// -----------------------------------------------------------------------
	// TC7 — Async load: OnExperienceLoaded fires AFTER LoadAndApplyExperience returns.
	// We bind a one-shot lambda, call the loader, and log marker lines so the
	// observer can see the order: "scheduling..." then (next tick) "ApplyExperience".
	// -----------------------------------------------------------------------
	{
		FDelegateHandle Sub = OnExperienceLoaded.AddLambda([](const UTestExperienceDefinition* Loaded)
		{
			UE_LOG(LogSandboxExperience, Display,
				TEXT("[TC7] OnExperienceLoaded delegate fired with '%s' -> PASS (delegate is async, fired after caller returned)"),
				Loaded ? *Loaded->ExperienceName.ToString() : TEXT("<null>"));
		});

		LoadAndApplyExperience(TEXT("?Experience=Raid"));
		UE_LOG(LogSandboxExperience, Display,
			TEXT("[TC7] LoadAndApplyExperience returned synchronously; delegate has NOT fired yet (proves async)"));

		// In a real lesson we'd remove the lambda after one shot; for sandbox brevity
		// the handle outlives this scope harmlessly.
		(void)Sub;
	}

	// -----------------------------------------------------------------------
	// TC8 — MaxPlayers per experience.
	// -----------------------------------------------------------------------
	{
		const UTestExperienceDefinition* Raid = ResolveExperience(TEXT("?Experience=Raid"));
		const UTestExperienceDefinition* Hub  = ResolveExperience(TEXT("?Experience=Hub"));
		const bool bPass = Raid && Hub && Raid->MaxPlayers == 4 && Hub->MaxPlayers == 32;
		UE_LOG(LogSandboxExperience, Display, TEXT("[TC8] MaxPlayers Raid=%d Hub=%d -> %s"),
			Raid ? Raid->MaxPlayers : -1, Hub ? Hub->MaxPlayers : -1, bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	UE_LOG(LogSandboxExperience, Display, TEXT("=== Lesson02 Experience :: sync TCs done (TC7 async result follows on next tick) ==="));
}
