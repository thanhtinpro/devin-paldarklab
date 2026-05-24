// Sandbox Lesson 01 — isolated log category so production can filter just this subsystem.
#pragma once

#include "Logging/LogMacros.h"

// SANDBOX: trong thực tế đây là LogPaldarkAbility / LogPaldarkInventory / ...
// mỗi subsystem một category để filter độc lập (`log LogSandboxTags Verbose`).
DECLARE_LOG_CATEGORY_EXTERN(LogSandboxTags, Log, All);
