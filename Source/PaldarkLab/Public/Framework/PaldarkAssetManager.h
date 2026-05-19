// PALDARK W1 day 6-7 — Asset manager subclass.
//
// Mirrors the pattern Lyra/RoN/PUBG use: project-specific UAssetManager that
// registers primary asset types and exposes a small "get-or-load synchronously"
// helper for the framework. Wired in `Config/DefaultEngine.ini` via:
//   [/Script/Engine.Engine]
//   AssetManagerClassName=/Script/PaldarkLab.PaldarkAssetManager
//
// Primary asset types are also declared in `[/Script/Engine.AssetManagerSettings]`
// so the editor's Asset Manager UI picks them up. Doing it both in C++ (here) and
// in ini guards against a designer regenerating settings — the values match.
//
// Primary asset types registered (kept in sync with DefaultGame.ini):
//   - PaldarkExperience          (UPaldarkExperienceDefinition)            — W1 day 6-7
//   - PaldarkPawnData            (UPaldarkPawnData)                        — W1 day 6-7
//   - PaldarkExperienceActionSet (UPaldarkExperienceActionSet)             — W1 day 8-10
//   - PaldarkInputConfig         (UPaldarkInputConfig)                     — W1 day 11-14
//   - PaldarkItem                (UPaldarkItemDefinition)                  — W11-12
//   - PaldarkSquadCommandSet     (UPaldarkSquadCommandSet)                 — W22-23
//   - PaldarkPalDefinition       (UPaldarkPalDefinition)                   — W27-28

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "PaldarkAssetManager.generated.h"

class UPaldarkExperienceDefinition;

UCLASS(Config = Game, meta = (DisplayName = "Paldark Asset Manager"))
class PALDARKLAB_API UPaldarkAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	UPaldarkAssetManager();

	// Convenience: get the active asset manager already cast to ours. Fails fatal
	// if the project ini is wired to a different class — that's intentional, the
	// rest of the framework assumes this type is live.
	static UPaldarkAssetManager& Get();

	// Synchronously loads a Paldark Experience by primary asset id. Use sparingly —
	// preferred path is async via FStreamableManager in the GameMode. Provided for
	// console commands + early-W1 hello-world callsites.
	const UPaldarkExperienceDefinition* GetExperienceSync(const FPrimaryAssetId& AssetId) const;

protected:
	virtual void StartInitialLoading() override;
};
