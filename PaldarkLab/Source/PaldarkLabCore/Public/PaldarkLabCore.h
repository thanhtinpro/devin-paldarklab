// PALDARK W1 — Core module umbrella header. Pulls in log category declarations so
// dependents only need `#include "PaldarkLabCore.h"` to access LogPaldark*.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "PaldarkLogCategories.h"

class FPaldarkLabCoreModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
