#include "Framework/PaldarkGameInstance.h"

#include "PaldarkLogCategories.h"

#include "GameFeaturesSubsystem.h"
#include "Interfaces/IPluginManager.h"

void UPaldarkGameInstance::Init()
{
	Super::Init();
	UE_LOG(LogPaldark, Log, TEXT("UPaldarkGameInstance::Init — Paldark game instance live."));

	// W39 — L-34 closure. Dedicated-server-only Game Feature plugin
	// verification + force-activate. Editor / standalone / listen-server
	// boot drives plugins to Active via `BuiltInInitialFeatureState`
	// during engine init; dedicated server (`PaldarkLabServer` target)
	// does not always do the same, so we explicitly verify here and
	// recover the plugin to Active if it's stalled below. Skipping this
	// hook on non-dedicated builds keeps Editor PIE warm-up time
	// unchanged and avoids loading server-only content on clients.
	if (IsDedicatedServerInstance())
	{
		EnsureFeaturePluginsActiveForDedicatedServer();
	}
}

void UPaldarkGameInstance::Shutdown()
{
	UE_LOG(LogPaldark, Log, TEXT("UPaldarkGameInstance::Shutdown."));
	Super::Shutdown();
}

void UPaldarkGameInstance::EnsureFeaturePluginsActiveForDedicatedServer()
{
	// Defensive double-check — caller is responsible for gating on
	// `IsDedicatedServerInstance()` but guarantee the server-only
	// contract here in case this method is ever invoked from elsewhere
	// (e.g. a future console command).
	if (!IsDedicatedServerInstance())
	{
		UE_LOG(LogPaldark, Warning,
			TEXT("EnsureFeaturePluginsActiveForDedicatedServer called on a "
			     "non-dedicated instance; early-out. Caller bug — only "
			     "NM_DedicatedServer boot should drive this path."));
		return;
	}

	UGameFeaturesSubsystem& GFS = UGameFeaturesSubsystem::Get();
	IPluginManager& PluginManager = IPluginManager::Get();

	int32 InspectedCount = 0;
	int32 ActivatedCount = 0;
	int32 SkippedActiveCount = 0;
	int32 FailedCount = 0;

	// Iterate every discovered plugin (.uproject Plugins[] + auto-
	// discovered under PaldarkLab/Plugins/). For each one that is both
	// Enabled and marked `IsGameFeaturePlugin: true` in its .uplugin
	// descriptor, verify the Game Features subsystem has it at `Active`
	// state. If not, force-activate via `LoadAndActivateGameFeaturePlugin`.
	//
	// The activation is intentionally fire-and-forget for L-34 — we log
	// completion via the lambda but do NOT block engine init waiting on
	// it (would deadlock if the plugin tries to register subsystems that
	// themselves depend on engine init having completed). The runtime
	// L-21 assertion in `PaldarkGameModeBase::OnExperienceAssetReady`
	// still fires on PIE / level load if activation hasn't reached
	// `Active` by the time the experience is requested, so the designer
	// still sees the same red banner if recovery fails.
	for (const TSharedRef<IPlugin>& Plugin : PluginManager.GetDiscoveredPlugins())
	{
		const FPluginDescriptor& Descriptor = Plugin->GetDescriptor();
		if (!Plugin->IsEnabled())
		{
			continue;
		}
		if (!Descriptor.bIsGameFeaturePlugin)
		{
			continue;
		}

		++InspectedCount;
		const FString PluginName = Plugin->GetName();

		FString PluginURL;
		if (!GFS.GetPluginURLByName(PluginName, PluginURL))
		{
			++FailedCount;
			UE_LOG(LogPaldark, Error,
				TEXT("[L-34] Dedicated server boot — feature plugin %s "
				     "registered with IPluginManager but UGameFeaturesSubsystem "
				     "could not resolve its URL. Check %s.uplugin "
				     "`IsGameFeaturePlugin: true` is set."),
				*PluginName, *PluginName);
			continue;
		}

		const EGameFeaturePluginState State = GFS.GetPluginState(PluginURL);
		if (State == EGameFeaturePluginState::Active)
		{
			++SkippedActiveCount;
			UE_LOG(LogPaldark, Log,
				TEXT("[L-34] Dedicated server boot — feature plugin %s "
				     "already Active (state=%d); no recovery needed."),
				*PluginName, static_cast<int32>(State));
			continue;
		}

		UE_LOG(LogPaldark, Warning,
			TEXT("[L-34] Dedicated server boot — feature plugin %s is "
			     "below Active (state=%d, expected %d). Forcing "
			     "LoadAndActivateGameFeaturePlugin. Editor / standalone "
			     "would have driven this via `BuiltInInitialFeatureState: "
			     "Active` in the .uplugin descriptor; NM_DedicatedServer "
			     "boot path needs this explicit nudge."),
			*PluginName,
			static_cast<int32>(State),
			static_cast<int32>(EGameFeaturePluginState::Active));

		++ActivatedCount;
		GFS.LoadAndActivateGameFeaturePlugin(
			PluginURL,
			FGameFeaturePluginLoadComplete::CreateLambda(
				[PluginName](const UE::GameFeatures::FResult& Result)
				{
					if (Result.HasValue())
					{
						UE_LOG(LogPaldark, Log,
							TEXT("[L-34] Feature plugin %s activation complete."),
							*PluginName);
					}
					else
					{
						UE_LOG(LogPaldark, Error,
							TEXT("[L-34] Feature plugin %s activation FAILED: %s. "
							     "Dedicated server will boot without %s content "
							     "(DA_PalDef_*, maps, raid abilities). Designer "
							     "fix: verify PaldarkLab.uproject Plugins[] "
							     "lists %s with Enabled=true and the .uplugin "
							     "sets EnabledByDefault=true."),
							*PluginName,
							*Result.GetError(),
							*PluginName,
							*PluginName);
					}
				}));
	}

	UE_LOG(LogPaldark, Log,
		TEXT("[L-34] Dedicated server feature plugin sweep — Inspected=%d "
		     "AlreadyActive=%d Forced=%d Failed=%d. NM_DedicatedServer boot path."),
		InspectedCount, SkippedActiveCount, ActivatedCount, FailedCount);
}
