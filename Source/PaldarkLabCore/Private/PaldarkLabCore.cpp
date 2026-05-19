// PALDARK W1 — Core module implementation. Defines the log categories declared in
// PaldarkLogCategories.h. IMPLEMENT_MODULE (not IMPLEMENT_PRIMARY_GAME_MODULE) because
// Core is a support module — PaldarkLab.cpp owns the primary game module macro.

#include "PaldarkLabCore.h"

DEFINE_LOG_CATEGORY(LogPaldark);
DEFINE_LOG_CATEGORY(LogPaldarkPal);
DEFINE_LOG_CATEGORY(LogPaldarkInventory);
DEFINE_LOG_CATEGORY(LogPaldarkNet);
DEFINE_LOG_CATEGORY(LogPaldarkGAS);
DEFINE_LOG_CATEGORY(LogPaldarkBackend);
DEFINE_LOG_CATEGORY(LogPaldarkUI);

void FPaldarkLabCoreModule::StartupModule()
{
	UE_LOG(LogPaldark, Log, TEXT("PaldarkLabCore module started — 7 log categories online."));
}

void FPaldarkLabCoreModule::ShutdownModule()
{
	UE_LOG(LogPaldark, Log, TEXT("PaldarkLabCore module shutdown."));
}

IMPLEMENT_MODULE(FPaldarkLabCoreModule, PaldarkLabCore)
