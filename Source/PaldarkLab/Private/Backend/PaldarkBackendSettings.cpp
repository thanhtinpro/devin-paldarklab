#include "Backend/PaldarkBackendSettings.h"

UPaldarkBackendSettings::UPaldarkBackendSettings()
{
	// Ctor body intentionally minimal — every UPROPERTY initialises inline in
	// the header (`bUseAWSBackend = false`, `AWSRegion = ...`, etc.). The
	// `UDeveloperSettings` base class hooks up the Config block loading from
	// `DefaultGame.ini` at engine init, so by the time any subsystem reads
	// the CDO every field is either the ini-overridden value or the inline
	// default. Mirrors the empty-ctor pattern used by `ULyraDeveloperSettings`
	// and other engine-side developer settings.
}
