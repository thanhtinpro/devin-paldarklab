// Copyright Epic Games, Inc. All Rights Reserved.

#include "Paldark.h"
#include "Modules/ModuleManager.h"
#include "AbilitySystemGlobals.h"

class FPaldarkModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		// GAS requires global data init exactly once before any ability/effect runs.
		// See UAbilitySystemGlobals docs — without this many GE features no-op or crash.
		UAbilitySystemGlobals::Get().InitGlobalData();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE( FPaldarkModule, Paldark, "Paldark" );
