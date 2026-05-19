#include "Framework/PaldarkGameModeBase.h"

#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFeaturesSubsystem.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/IPluginManager.h"
#include "Kismet/GameplayStatics.h"

#include "PaldarkLogCategories.h"
#include "Experience/PaldarkExperienceActionSet.h"
#include "Experience/PaldarkExperienceDefinition.h"
#include "Experience/PaldarkPawnData.h"
#include "Framework/PaldarkAssetManager.h"
#include "Framework/PaldarkGameStateBase.h"
#include "Net/PaldarkNetSubsystem.h"
#include "Pal/PaldarkPalDefinition.h"
#include "Pal/PaldarkPalSpawnSubsystem.h"
#include "Player/PaldarkPlayerController.h"
#include "Player/PaldarkPlayerState.h"

APaldarkGameModeBase::APaldarkGameModeBase()
{
	// Default class wiring. Subclasses / Blueprints can override individual slots;
	// experiences can override on a per-session basis via PlayerControllerClass /
	// PlayerStateClass on UPaldarkExperienceDefinition.
	GameStateClass = APaldarkGameStateBase::StaticClass();
	PlayerControllerClass = APaldarkPlayerController::StaticClass();
	PlayerStateClass = APaldarkPlayerState::StaticClass();
}

void APaldarkGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	UE_LOG(LogPaldarkNet, Log,
		TEXT("[InitGame] Map=%s Options=\"%s\" MaxPlayers=%d"),
		*MapName, *Options, MaxPlayersPerSession);
	RequestExperienceLoad(Options);
}

void APaldarkGameModeBase::PreLogin(
	const FString& Options,
	const FString& Address,
	const FUniqueNetIdRepl& UniqueId,
	FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (!ErrorMessage.IsEmpty())
	{
		// Super already rejected this client (e.g. invalid options).
		UE_LOG(LogPaldarkNet, Warning,
			TEXT("[PreLogin] Super rejected client from %s: %s"),
			*Address, *ErrorMessage);
		return;
	}

	if (ConnectedPlayerCount >= MaxPlayersPerSession)
	{
		ErrorMessage = FString::Printf(
			TEXT("Server full (%d/%d). Please try again later."),
			ConnectedPlayerCount, MaxPlayersPerSession);
		UE_LOG(LogPaldarkNet, Warning,
			TEXT("[PreLogin] Rejected %s: server full %d/%d."),
			*Address, ConnectedPlayerCount, MaxPlayersPerSession);
		return;
	}

	UE_LOG(LogPaldarkNet, Log,
		TEXT("[PreLogin] Approved %s (slot %d/%d)"),
		*Address, ConnectedPlayerCount + 1, MaxPlayersPerSession);
}

void APaldarkGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	++ConnectedPlayerCount;
	UE_LOG(LogPaldarkNet, Log,
		TEXT("[PostLogin] %s joined — %d/%d player(s) connected."),
		NewPlayer != nullptr ? *NewPlayer->GetName() : TEXT("<null>"),
		ConnectedPlayerCount, MaxPlayersPerSession);

	// W39 — L-20 closure. Fire the BlueprintAssignable delegate so UMG lobby
	// widgets (W40-41) and any other listeners can react without subclassing
	// the GameMode. The subsystem broadcast is server-authoritative; clients
	// rely on the GameState `PlayerArray` REPNOTIFY for the same information.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UPaldarkNetSubsystem* NetSubsystem = GI->GetSubsystem<UPaldarkNetSubsystem>())
		{
			NetSubsystem->BroadcastPlayerLogin(NewPlayer);
		}
	}

	if (!bExperienceLoaded || LoadedExperience == nullptr)
	{
		UE_LOG(LogPaldark, Log, TEXT("PostLogin before experience ready — pawn spawn deferred to GameMode default path."));
		return;
	}

	const FText Message = LoadedExperience->HelloWorldMessage.IsEmpty()
		? FText::FromString(TEXT("Hello from Paldark Experience."))
		: LoadedExperience->HelloWorldMessage;

	UE_LOG(
		LogPaldark,
		Log,
		TEXT("[Experience] Player %s logged in. Experience=%s Message=\"%s\""),
		NewPlayer != nullptr ? *NewPlayer->GetName() : TEXT("<null>"),
		*ResolvedExperienceId.ToString(),
		*Message.ToString());

	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			6.0f,
			FColor::Green,
			FString::Printf(TEXT("Experience hello: %s"), *Message.ToString()));
	}
}

void APaldarkGameModeBase::Logout(AController* Exiting)
{
	if (ConnectedPlayerCount > 0)
	{
		--ConnectedPlayerCount;
	}
	UE_LOG(LogPaldarkNet, Log,
		TEXT("[Logout] %s left — %d/%d player(s) remaining."),
		Exiting != nullptr ? *Exiting->GetName() : TEXT("<null>"),
		ConnectedPlayerCount, MaxPlayersPerSession);

	// W39 — L-20 closure. Broadcast BEFORE Super::Logout so listeners that
	// reach into Exiting->PlayerState still see a valid pointer; the engine
	// usually clears it during the super call's `RemovePlayer` path.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UPaldarkNetSubsystem* NetSubsystem = GI->GetSubsystem<UPaldarkNetSubsystem>())
		{
			NetSubsystem->BroadcastPlayerLogout(Exiting);
		}
	}

	Super::Logout(Exiting);
}

UClass* APaldarkGameModeBase::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (ResolvedPawnClass != nullptr)
	{
		return ResolvedPawnClass;
	}
	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void APaldarkGameModeBase::RequestExperienceLoad(const FString& Options)
{
	// 1) URL option ?Experience=PaldarkExperience.<RowName>
	FString Requested = UGameplayStatics::ParseOption(Options, TEXT("Experience"));
	if (Requested.IsEmpty() && !DefaultExperience.IsNull())
	{
		// 2) Fall back to game-mode-default experience asset.
		Requested = DefaultExperience.ToSoftObjectPath().ToString();
	}

	FPrimaryAssetId AssetId;
	if (Requested.Contains(TEXT(".")) && !Requested.StartsWith(TEXT("/")))
	{
		// "PaldarkExperience.HelloWorld" form — split on the first dot.
		FString TypeName;
		FString NameStr;
		Requested.Split(TEXT("."), &TypeName, &NameStr);
		AssetId = FPrimaryAssetId(FPrimaryAssetType(*TypeName), FName(*NameStr));
	}
	else if (Requested.StartsWith(TEXT("/")))
	{
		// Soft object path form — look up id by path via asset manager.
		const FSoftObjectPath Path(Requested);
		AssetId = UAssetManager::Get().GetPrimaryAssetIdForPath(Path);
	}

	if (!AssetId.IsValid())
	{
		UE_LOG(
			LogPaldark,
			Warning,
			TEXT("APaldarkGameModeBase::RequestExperienceLoad — could not resolve experience id from option %s. "
			     "Falling back to default pawn class."),
			*Requested);
		bExperienceLoaded = true;
		return;
	}

	ResolvedExperienceId = AssetId;
	UE_LOG(LogPaldark, Log, TEXT("Requesting async load of experience %s."), *AssetId.ToString());

	UAssetManager& Manager = UAssetManager::Get();
	const TArray<FName> Bundles;
	const FStreamableDelegate OnLoaded = FStreamableDelegate::CreateUObject(
		this,
		&APaldarkGameModeBase::OnExperienceAssetReady,
		AssetId);
	Manager.LoadPrimaryAsset(AssetId, Bundles, OnLoaded);
}

void APaldarkGameModeBase::OnExperienceAssetReady(FPrimaryAssetId AssetId)
{
	UAssetManager& Manager = UAssetManager::Get();
	const FSoftObjectPath Path = Manager.GetPrimaryAssetPath(AssetId);
	UObject* LoadedObject = Path.IsNull() ? nullptr : Path.ResolveObject();
	const UPaldarkExperienceDefinition* Experience = Cast<UPaldarkExperienceDefinition>(LoadedObject);

	if (Experience == nullptr)
	{
		UE_LOG(
			LogPaldark,
			Error,
			TEXT("Experience %s failed to resolve to UPaldarkExperienceDefinition (got %s)."),
			*AssetId.ToString(),
			LoadedObject != nullptr ? *LoadedObject->GetClass()->GetPathName() : TEXT("nullptr"));
		bExperienceLoaded = true;
		return;
	}

	LoadedExperience = Experience;
	bExperienceLoaded = true;

	// W40-41 — Seed `MaxPlayersPerSession` from the experience's `MaxPlayers`
	// field so a hub experience (PX_HubTown, MaxPlayers=8) lifts the cap
	// above the W14-15 raid default of 4. Done here rather than in InitGame
	// because the experience asset is not yet resolved at InitGame time; any
	// (N+1)th client that connects before this point gets rejected at the
	// CDO default 4, which is the desired conservative behaviour.
	if (Experience->MaxPlayers > 0 && Experience->MaxPlayers != MaxPlayersPerSession)
	{
		const int32 OldMax = MaxPlayersPerSession;
		MaxPlayersPerSession = Experience->MaxPlayers;
		UE_LOG(LogPaldarkNet, Log,
			TEXT("[Experience] MaxPlayersPerSession bumped %d → %d from experience %s."),
			OldMax, MaxPlayersPerSession, *AssetId.ToString());
		if (AGameSession* Session = GameSession)
		{
			Session->MaxPlayers = MaxPlayersPerSession;
		}
	}

	// W26 — L-01 closure. Assert the spawned GameMode satisfies the experience's
	// declared RequiredGameModeClass *before* applying class overrides, so the
	// error surfaces at the start of the OnExperienceAssetReady chain (designers
	// stop reading after the first red line in the log). The GameMode class
	// itself cannot be swapped at runtime, so this is informational — designer
	// must restart the level with World Override or `?game=<Alias>` URL flag.
	if (!Experience->RequiredGameModeClass.IsNull())
	{
		UClass* RequiredClass = Experience->RequiredGameModeClass.LoadSynchronous();
		if (RequiredClass != nullptr && !GetClass()->IsChildOf(RequiredClass))
		{
			const FString ExpectedName = RequiredClass->GetName();
			const FString ActualName = GetClass()->GetName();
			UE_LOG(
				LogPaldark,
				Error,
				TEXT("APaldarkGameModeBase::OnExperienceAssetReady — experience %s "
				     "requires GameMode class %s but the spawned GameMode is %s. "
				     "Match/extraction features that rely on GameMode hooks (PostLogin, "
				     "Logout, OnMatchEnded) will NOT fire. Fix: set the map's World "
				     "Override → GameMode to %s, or relaunch with "
				     "?game=<Alias> matching a +GameModeClassAliases entry in DefaultEngine.ini."),
				*AssetId.ToString(),
				*ExpectedName,
				*ActualName,
				*ExpectedName);
			if (GEngine != nullptr)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					15.0f,
					FColor::Red,
					FString::Printf(
						TEXT("Experience %s expects GameMode %s (got %s). "
						     "Match features disabled."),
						*AssetId.ToString(),
						*ExpectedName,
						*ActualName));
			}
		}
	}

	// W39 — L-21 closure. Assert every required Game Feature plugin is in
	// the `Active` state. Designers who forget to enable a feature plugin
	// in `PaldarkLab.uproject` (e.g. `PaldarkRaidContent`) silently boot
	// the experience with missing content; the runtime assertion surfaces
	// the failure as a loud Error log + red on-screen banner so the
	// problem is caught at PIE startup rather than at first
	// `SpawnPalAsync` miss several minutes into a session.
	//
	// Recovery path is fully runtime: designer toggles plugin in Editor's
	// `Plugins` UI (or edits `PaldarkLab.uproject` Plugins[]) then restarts
	// the level / Editor. No `?game=` URL flag needed (unlike L-01).
	if (Experience->RequiredGameFeatures.Num() > 0)
	{
		UGameFeaturesSubsystem& GFS = UGameFeaturesSubsystem::Get();
		int32 MissingCount = 0;
		for (const FString& PluginName : Experience->RequiredGameFeatures)
		{
			if (PluginName.IsEmpty())
			{
				continue;
			}
			// First: is the plugin even registered with the host project?
			// `IPluginManager::FindPlugin` returns null if it's not in
			// PaldarkLab.uproject Plugins[] (or auto-discovered under
			// Plugins/), which is the most common designer mistake.
			TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(PluginName);
			if (!Plugin.IsValid())
			{
				++MissingCount;
				UE_LOG(
					LogPaldark,
					Error,
					TEXT("APaldarkGameModeBase::OnExperienceAssetReady — experience %s "
					     "requires Game Feature plugin %s but IPluginManager could not "
					     "find it. Designer fix: add the plugin to PaldarkLab.uproject "
					     "Plugins[] (or drop it under PaldarkLab/Plugins/) and restart "
					     "the Editor."),
					*AssetId.ToString(),
					*PluginName);
				if (GEngine != nullptr)
				{
					GEngine->AddOnScreenDebugMessage(
						-1,
						15.0f,
						FColor::Red,
						FString::Printf(
							TEXT("Experience %s requires Game Feature plugin %s — NOT FOUND. "
							     "Add to .uproject Plugins[]."),
							*AssetId.ToString(),
							*PluginName));
				}
				continue;
			}

			// Second: resolve the plugin URL the Game Features subsystem
			// keys on. If `GetPluginURLByName` fails the plugin exists on
			// disk but did not register as an IsGameFeaturePlugin in its
			// .uplugin (so it's just a regular content plugin) — still an
			// error for our purposes because RequiredGameFeatures was
			// authored expecting feature-plugin semantics.
			FString PluginURL;
			if (!GFS.GetPluginURLByName(PluginName, PluginURL))
			{
				++MissingCount;
				UE_LOG(
					LogPaldark,
					Error,
					TEXT("APaldarkGameModeBase::OnExperienceAssetReady — experience %s "
					     "requires Game Feature plugin %s but UGameFeaturesSubsystem "
					     "could not resolve its URL. Check that %s.uplugin sets "
					     "`\"IsGameFeaturePlugin\": true`."),
					*AssetId.ToString(),
					*PluginName,
					*PluginName);
				if (GEngine != nullptr)
				{
					GEngine->AddOnScreenDebugMessage(
						-1,
						15.0f,
						FColor::Red,
						FString::Printf(
							TEXT("Experience %s requires Game Feature %s — not a feature plugin. "
							     "Check IsGameFeaturePlugin in .uplugin."),
							*AssetId.ToString(),
							*PluginName));
				}
				continue;
			}

			// Third: feature plugin must be in `Active` state. The
			// subsystem ramp is Installed → Registered → Loaded → Active;
			// designer enabling the plugin in .uproject is enough for the
			// engine to drive it to Active on Editor / standalone startup,
			// but a plugin disabled at runtime via Editor UI sits below
			// Active.
			const EGameFeaturePluginState State = GFS.GetPluginState(PluginURL);
			if (State != EGameFeaturePluginState::Active)
			{
				++MissingCount;
				UE_LOG(
					LogPaldark,
					Error,
					TEXT("APaldarkGameModeBase::OnExperienceAssetReady — experience %s "
					     "requires Game Feature plugin %s to be Active, but its state "
					     "is %d (expected %d). Designer fix: Editor → Plugins → search "
					     "%s → check Enabled → restart Editor. Content from this "
					     "plugin (DA_PalDef_*, maps, abilities) will NOT be loadable."),
					*AssetId.ToString(),
					*PluginName,
					static_cast<int32>(State),
					static_cast<int32>(EGameFeaturePluginState::Active),
					*PluginName);
				if (GEngine != nullptr)
				{
					GEngine->AddOnScreenDebugMessage(
						-1,
						15.0f,
						FColor::Red,
						FString::Printf(
							TEXT("Experience %s requires Game Feature %s Active (state=%d). "
							     "Enable plugin in Editor."),
							*AssetId.ToString(),
							*PluginName,
							static_cast<int32>(State)));
				}
			}
		}
		UE_LOG(
			LogPaldark,
			Log,
			TEXT("[Experience] RequiredGameFeatures check — %d required, %d missing."),
			Experience->RequiredGameFeatures.Num(),
			MissingCount);
	}

	// W1 day 8-10: pull PlayerController / PlayerState / HUD class overrides off
	// the experience BEFORE we resolve the pawn class so that GetDefaultPawnClass
	// + login flow see consistent values.
	ApplyExperienceClassOverrides(*Experience);
	ApplyExperienceActionSets(*Experience);

	// W27-28: kick off async pre-warm of every Pal definition the experience
	// expects to spawn early. Fire-and-forget — PostLogin / pawn spawn does
	// NOT wait for these to finish.
	RequestPalDefinitionPreWarm(*Experience);

	// Resolve pawn class from pawn data, if provided.
	if (!Experience->DefaultPawnData.IsNull())
	{
		const UPaldarkPawnData* PawnData = Experience->DefaultPawnData.LoadSynchronous();
		if (PawnData != nullptr && !PawnData->PawnClass.IsNull())
		{
			if (UClass* LoadedClass = PawnData->PawnClass.LoadSynchronous())
			{
				ResolvedPawnClass = LoadedClass;
			}
		}
	}

	UE_LOG(
		LogPaldark,
		Log,
		TEXT("Experience %s loaded. PawnClass=%s PlayerControllerClass=%s PlayerStateClass=%s ActionSets=%d Tags=%d"),
		*AssetId.ToString(),
		ResolvedPawnClass != nullptr ? *ResolvedPawnClass->GetPathName() : TEXT("<default>"),
		PlayerControllerClass != nullptr ? *PlayerControllerClass->GetPathName() : TEXT("<null>"),
		PlayerStateClass != nullptr ? *PlayerStateClass->GetPathName() : TEXT("<null>"),
		Experience->ActionSets.Num(),
		ActiveExperienceTags.Num());

	if (APaldarkGameStateBase* PaldarkState = GetGameState<APaldarkGameStateBase>())
	{
		PaldarkState->SetCurrentExperience(AssetId, Experience);
	}
}

void APaldarkGameModeBase::ApplyExperienceClassOverrides(const UPaldarkExperienceDefinition& Experience)
{
	// PlayerController override — leave existing class if experience didn't specify.
	if (!Experience.PlayerControllerClass.IsNull())
	{
		if (UClass* PCClass = Experience.PlayerControllerClass.LoadSynchronous())
		{
			PlayerControllerClass = PCClass;
		}
	}

	// PlayerState override.
	if (!Experience.PlayerStateClass.IsNull())
	{
		if (UClass* PSClass = Experience.PlayerStateClass.LoadSynchronous())
		{
			PlayerStateClass = PSClass;
		}
	}

	// HUD override comes from PawnData (W1 day 8-10: HUD is per-pawn, not per-experience).
	if (!Experience.DefaultPawnData.IsNull())
	{
		if (const UPaldarkPawnData* PawnData = Experience.DefaultPawnData.LoadSynchronous())
		{
			if (!PawnData->HUDClass.IsNull())
			{
				if (UClass* HUDLoaded = PawnData->HUDClass.LoadSynchronous())
				{
					HUDClass = HUDLoaded;
				}
			}
		}
	}
}

void APaldarkGameModeBase::RequestPalDefinitionPreWarm(const UPaldarkExperienceDefinition& Experience)
{
	if (Experience.PreWarmPalDefinitions.Num() == 0)
	{
		return;
	}

	UPaldarkPalSpawnSubsystem* Subsystem = UPaldarkPalSpawnSubsystem::Get(GetWorld());
	if (Subsystem == nullptr)
	{
		UE_LOG(LogPaldarkPal, Warning,
			TEXT("RequestPalDefinitionPreWarm — UPaldarkPalSpawnSubsystem not available on world %s. "
			     "Pre-warm skipped; SpawnPalAsync will pay per-species streaming cost on first hit."),
			GetWorld() != nullptr ? *GetWorld()->GetName() : TEXT("<null>"));
		return;
	}

	int32 Requested = 0;
	for (const TSoftObjectPtr<UPaldarkPalDefinition>& SoftDef : Experience.PreWarmPalDefinitions)
	{
		if (SoftDef.IsNull())
		{
			continue;
		}
		// Resolve to a primary asset id via the asset path; the definition
		// itself does NOT need to be loaded yet — RequestPreWarmAsync will
		// pull it (and its "Spawn" bundle) via the streamable manager.
		const FSoftObjectPath Path = SoftDef.ToSoftObjectPath();
		const FPrimaryAssetId DefId = UAssetManager::Get().GetPrimaryAssetIdForPath(Path);
		if (!DefId.IsValid())
		{
			UE_LOG(LogPaldarkPal, Warning,
				TEXT("RequestPalDefinitionPreWarm — no primary asset id for path %s; "
				     "is the PaldarkPalDefinition row registered in DefaultGame.ini?"),
				*Path.ToString());
			continue;
		}

		Subsystem->RequestPreWarmAsync(
			DefId,
			FOnPalPreWarmComplete::CreateLambda(
				[DefId](FPrimaryAssetId, bool bSuccess)
				{
					UE_LOG(LogPaldarkPal, Log,
						TEXT("PreWarm complete — %s Success=%d."),
						*DefId.ToString(), bSuccess ? 1 : 0);
				}));
		++Requested;
	}

	UE_LOG(LogPaldarkPal, Log,
		TEXT("RequestPalDefinitionPreWarm — requested %d/%d Pal definition pre-warm(s)."),
		Requested, Experience.PreWarmPalDefinitions.Num());
}

void APaldarkGameModeBase::ApplyExperienceActionSets(const UPaldarkExperienceDefinition& Experience)
{
	ActiveExperienceTags.Reset();
	ActiveExperienceTags.AppendTags(Experience.IntrinsicTags);

	for (const TSoftObjectPtr<UPaldarkExperienceActionSet>& SoftSet : Experience.ActionSets)
	{
		if (SoftSet.IsNull())
		{
			continue;
		}
		const UPaldarkExperienceActionSet* Set = SoftSet.LoadSynchronous();
		if (Set == nullptr)
		{
			UE_LOG(LogPaldark, Warning, TEXT("ActionSet %s failed to load."), *SoftSet.ToString());
			continue;
		}
		ActiveExperienceTags.AppendTags(Set->GrantedTags);

		if (!Set->DebugNote.IsEmpty())
		{
			UE_LOG(LogPaldark, Log, TEXT("[ActionSet] %s: %s"), *Set->GetName(), *Set->DebugNote.ToString());
		}
		else
		{
			UE_LOG(LogPaldark, Log, TEXT("[ActionSet] %s applied (%d tags)."), *Set->GetName(), Set->GrantedTags.Num());
		}
	}
}
