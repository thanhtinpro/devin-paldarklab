// PALDARK W1 — Editor module implementation. Placeholder only; logs lifecycle so we
// can verify the module loads when the editor opens.

#include "PaldarkLabEditor.h"

#include "PaldarkLogCategories.h"

void FPaldarkLabEditorModule::StartupModule()
{
	UE_LOG(LogPaldark, Log, TEXT("PaldarkLabEditor module started."));
}

void FPaldarkLabEditorModule::ShutdownModule()
{
	UE_LOG(LogPaldark, Log, TEXT("PaldarkLabEditor module shutdown."));
}

IMPLEMENT_MODULE(FPaldarkLabEditorModule, PaldarkLabEditor)
