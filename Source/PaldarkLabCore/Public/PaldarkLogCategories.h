// PALDARK W1 — 5 log category for the project. Declared in PaldarkLabCore (loads
// PreDefault) so every other module can emit logs to these categories during their
// own StartupModule. Defaults set in Config/DefaultEngine.ini → [Core.Log].

#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

// General log — anything not covered by the other 4.
PALDARKLABCORE_API DECLARE_LOG_CATEGORY_EXTERN(LogPaldark, Log, All);

// Pal AI, Activity FSM, perception, taming. Used heavily from W3 (Pal pawn).
PALDARKLABCORE_API DECLARE_LOG_CATEGORY_EXTERN(LogPaldarkPal, Log, All);

// Inventory, equipment, item fragment. Used from W11 (P11).
PALDARKLABCORE_API DECLARE_LOG_CATEGORY_EXTERN(LogPaldarkInventory, Log, All);

// Replication, RPC, RepNotify, lag comp. Used from W14 (P06).
PALDARKLABCORE_API DECLARE_LOG_CATEGORY_EXTERN(LogPaldarkNet, Log, All);

// Gameplay Ability System: ASC, AS, GA, GE, Cue. Used from W7 (P08).
PALDARKLABCORE_API DECLARE_LOG_CATEGORY_EXTERN(LogPaldarkGAS, Log, All);

// W42-43 — AWS backend (Cognito login, fleet allocation Lambda, JWT
// refresh, mocked fallback). Kept separate from `LogPaldarkNet` so the
// noisy HTTP request/response logs do not drown out the W14-15 listen-
// server / W16-17 lag-comp lines designers grep for.
PALDARKLABCORE_API DECLARE_LOG_CATEGORY_EXTERN(LogPaldarkBackend, Log, All);

// W44-45 — Hub UI (Pal Stable + Marketplace controller + UMG widget
// classes). Kept separate so the noisy "click → server RPC → client
// callback → widget refresh" trace doesn't bleed into LogPaldark when
// designers are debugging marketplace pricing or stable swaps.
PALDARKLABCORE_API DECLARE_LOG_CATEGORY_EXTERN(LogPaldarkUI, Log, All);
