// PALDARK W24-25 — Match phase + per-player outcome enums + replication row.
//
// Shared types used by `UPaldarkMatchSubsystem`, `APaldarkPlayerState`,
// `APaldarkGameStateBase`, `APaldarkGameMode_Extraction`, and
// `APaldarkExtractionBeacon`. Lives in its own header so any of the above
// can include just the types without pulling in the subsystem (which would
// trigger a transitive include of WorldSubsystem.h and balloon header
// dependencies).
//
// Roadmap reference: Documents/PALDARK/03-Roadmap_1_Year.md § Tuần 24-25.
// Outcome target: "Full raid loop functional (drop → fight → extract)."

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/ObjectMacros.h"

#include "PaldarkMatchTypes.generated.h"

class APlayerState;

// EPaldarkMatchPhase — server-authoritative match state machine.
//
// Transitions (all server-side, guarded by `UPaldarkMatchSubsystem::SetPhase`):
//   Warmup    -> Active     when StartMatch is called (after lobby is full
//                            or a timeout, designer-tunable).
//   Active    -> Extracting when the first beacon timer kicks off (purely
//                            informational; does NOT gate further extractions).
//   Active    -> Ended      when the end-condition predicate trips (all
//                            players extracted or dead, or hard timer hit).
//   Extracting-> Ended      same end-condition predicate.
//   Ended     -> *          terminal; new sessions require ServerTravel.
UENUM(BlueprintType)
enum class EPaldarkMatchPhase : uint8
{
	Warmup     UMETA(DisplayName = "Warmup"),
	Active     UMETA(DisplayName = "Active"),
	Extracting UMETA(DisplayName = "Extracting"),
	Ended      UMETA(DisplayName = "Ended"),
};

// EPaldarkPlayerOutcome — per-player terminal state. Initially Alive while
// the player is in the match; flipped to Extracted / KIA / Disconnected by
// the match subsystem on the corresponding event.
//
// `bIsTerminal` of any non-Alive value is true; once a player hits a
// terminal outcome the subsystem does NOT re-process them (e.g. a dead
// player overlapping a beacon doesn't extract).
UENUM(BlueprintType)
enum class EPaldarkPlayerOutcome : uint8
{
	Alive        UMETA(DisplayName = "Alive"),
	Extracted    UMETA(DisplayName = "Extracted"),
	KIA          UMETA(DisplayName = "KIA"),
	Disconnected UMETA(DisplayName = "Disconnected"),
};

// EPaldarkMatchEndReason — why the match ended. Replicated on the
// GameState alongside MatchPhase so clients can render the "all extracted"
// vs "wipe" end screen without re-deriving the reason from per-player rows.
UENUM(BlueprintType)
enum class EPaldarkMatchEndReason : uint8
{
	None             UMETA(DisplayName = "None"),
	AllExtracted     UMETA(DisplayName = "AllExtracted"),
	TeamWipe         UMETA(DisplayName = "TeamWipe"),
	Timeout          UMETA(DisplayName = "Timeout"),
	ForcedByConsole  UMETA(DisplayName = "ForcedByConsole"),
};

// FPaldarkMatchPlayerRow — per-player bookkeeping inside the match
// subsystem. Indexed by PlayerState pointer (weak ref so logged-out
// players don't pin the PlayerState).
USTRUCT()
struct PALDARKLAB_API FPaldarkMatchPlayerRow
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TWeakObjectPtr<APlayerState> PlayerState;

	UPROPERTY(Transient)
	EPaldarkPlayerOutcome Outcome = EPaldarkPlayerOutcome::Alive;

	// Cached server time (seconds, GetWorld()->GetTimeSeconds) of when the
	// player joined. Used by the GameMode end-of-match scoreboard and by
	// designer telemetry in W42-43.
	UPROPERTY(Transient)
	float JoinedAtServerTime = 0.f;

	// Cached server time when Outcome flipped to non-Alive. Zero while alive.
	UPROPERTY(Transient)
	float TerminalAtServerTime = 0.f;

	// True when Outcome != Alive. Helper so call sites read intent, not enum.
	bool IsTerminal() const
	{
		return Outcome != EPaldarkPlayerOutcome::Alive;
	}
};

// Helper: convert EPaldarkPlayerOutcome → matching `Paldark.Match.Outcome.*`
// gameplay tag. Lives next to the enum so any caller (PlayerState, beacon,
// end-screen widget) can stamp the tag without duplicating the switch.
PALDARKLAB_API FGameplayTag PaldarkOutcomeToTag(EPaldarkPlayerOutcome Outcome);

// Helper: convert EPaldarkMatchPhase → matching `Paldark.Match.Phase.*` tag.
PALDARKLAB_API FGameplayTag PaldarkPhaseToTag(EPaldarkMatchPhase Phase);
