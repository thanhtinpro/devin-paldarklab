// PALDARK W27-28 — Game-feature plugin module umbrella header.
//
// The plugin's IModuleInterface implementation lives in PaldarkRaidContent.cpp
// (IMPLEMENT_MODULE). This header is mostly a future home for plugin-wide
// declarations (log categories, shared types) — kept intentionally empty in
// W27-28 since the plugin currently has no runtime surface area beyond what
// designers author into GameFeatureData_RaidContent.uasset.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FPaldarkRaidContentModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
