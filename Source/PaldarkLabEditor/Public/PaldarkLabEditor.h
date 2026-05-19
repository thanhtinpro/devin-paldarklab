// PALDARK W1 — Editor module umbrella header. Empty placeholder — future home for
// asset validators, custom thumbnail renderers, asset type actions.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FPaldarkLabEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
