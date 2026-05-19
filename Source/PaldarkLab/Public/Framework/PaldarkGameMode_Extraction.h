// PALDARK W24-25 — Extraction game mode.
//
// `APaldarkGameModeBase` subclass that owns the lifecycle of one extraction
// match. Adds:
//   - PostLogin / Logout hooks that register/unregister the joining player
//     into `UPaldarkMatchSubsystem`.
//   - End-of-match handling: binds `OnMatchEnded` so the GameMode can log a
//     scoreboard. (Lobby travel is W40+; this hook is the integration seam.)
//
// Lives in Framework/ so the canonical `APaldarkGameModeBase` shape is
// extended cleanly without touching the base class.
//
// Class registration (W26 L-01 closure — pick one):
//   1. Map World Override → GameMode = APaldarkGameMode_Extraction (per-map).
//   2. URL flag: `open Raid_Sandbox?game=Extraction?Experience=...` (uses
//      the `+GameModeClassAliases=(Name="Extraction",...)` entry shipped in
//      DefaultEngine.ini W26).
//   3. Authoring the experience asset: set
//      `UPaldarkExperienceDefinition::RequiredGameModeClass =
//      APaldarkGameMode_Extraction`. This does NOT change the spawned GameMode
//      class (UE locks that at InitGame), but the base GameMode asserts at
//      experience-ready time and logs Error + on-screen red warning if the
//      spawned GameMode is not a subclass, so the designer notices the miswire
//      on the first PIE frame instead of debugging a silent-Warmup match.

#pragma once

#include "CoreMinimal.h"
#include "Framework/PaldarkGameModeBase.h"
#include "Match/PaldarkMatchTypes.h"

#include "PaldarkGameMode_Extraction.generated.h"

class UPaldarkMatchSubsystem;

UCLASS(Config = Game)
class PALDARKLAB_API APaldarkGameMode_Extraction : public APaldarkGameModeBase
{
	GENERATED_BODY()

public:
	APaldarkGameMode_Extraction();

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

protected:
	// Bound to `UPaldarkMatchSubsystem::OnMatchEnded` in BeginPlay (server-
	// only). Logs the scoreboard; designers can extend in a BP subclass
	// to RestartGame or ServerTravel.
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void HandleMatchEnded(EPaldarkMatchEndReason Reason);

private:
	// Resolve the world subsystem with a fallback log so PostLogin / Logout
	// don't crash on early-PIE edge cases.
	UPaldarkMatchSubsystem* GetMatchSubsystem() const;

	// Delegate handle so EndPlay can remove the binding cleanly.
	FDelegateHandle MatchEndedHandle;
};
