// PALDARK W1 day 6-7 — Game instance subclass.
//
// Class wiring slot in `DefaultEngine.ini`:
//   [/Script/EngineSettings.GameMapsSettings]
//   GameInstanceClass=/Script/PaldarkLab.PaldarkGameInstance
//
// W39 — L-34 closure. `Init` overrides serve as the dedicated-server
// boot hook for verifying Game Feature plugins are in the `Active`
// state. Editor / standalone / listen-server boot drives feature
// plugins to Active automatically via the `BuiltInInitialFeatureState`
// .uplugin field. Dedicated-server (`PaldarkLabServer` target) does not
// always do the same — depending on launch flags, the plugin can be
// installed + registered but stalled below Active by the time
// `UGameInstance::Init` returns, which means a `?game=Extraction`
// launch into `Raid_Sandbox` would silently boot without the
// `PaldarkRaidContent` plugin's content available (no `DA_PalDef_*`,
// no raid maps, broken raid abilities). The boot hook iterates the
// declared feature plugins and explicitly calls
// `UGameFeaturesSubsystem::LoadAndActivateGameFeaturePlugin` for any
// plugin that isn't already Active, with the result tied to the same
// `LogPaldark Error` channel L-21 uses on the GameMode-side
// assertion.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "PaldarkGameInstance.generated.h"

UCLASS(Config = Game)
class PALDARKLAB_API UPaldarkGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

private:
	// W39 — L-34 closure. Called by Init() on dedicated-server builds only
	// (early-out on listen / standalone / editor — those already drive the
	// plugin to Active via `BuiltInInitialFeatureState`). Iterates the
	// project's declared feature plugins and force-activates any that are
	// stalled below `Active`. Server-only because force-activating a
	// feature plugin on a client would load asset content the client
	// doesn't need (and pollutes Editor PIE warm-up time).
	void EnsureFeaturePluginsActiveForDedicatedServer();
};
