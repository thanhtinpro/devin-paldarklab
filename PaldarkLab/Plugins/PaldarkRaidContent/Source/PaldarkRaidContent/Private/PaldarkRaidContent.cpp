#include "PaldarkRaidContent.h"

#include "PaldarkLogCategories.h"

void FPaldarkRaidContentModule::StartupModule()
{
	UE_LOG(LogPaldark, Log, TEXT("PaldarkRaidContent module started."));
}

void FPaldarkRaidContentModule::ShutdownModule()
{
	UE_LOG(LogPaldark, Log, TEXT("PaldarkRaidContent module shutdown."));
}

IMPLEMENT_MODULE(FPaldarkRaidContentModule, PaldarkRaidContent)
