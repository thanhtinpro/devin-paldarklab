// PALDARK W1 day 6-7 — Game mode base.
//
// Server-only orchestrator. Picks an experience for the session, async-loads it,
// then drives pawn spawning + RepNotifies the experience id onto game state for
// clients. Mirrors the Lyra `ALyraGameMode` -> `ULyraExperienceManagerComponent`
// handshake but flattened: experience id source -> AssetManager.LoadPrimaryAsset
// -> assign onto PaldarkGameStateBase.
//
// Experience id resolution order (first match wins):
//   1. URL option `?Experience=PaldarkExperience.<RowName>` (e.g. open level via
//      `open Test_Map?Experience=PaldarkExperience.HelloWorld`).
//   2. World settings override (future — not in W1).
//   3. `DefaultExperience` configured on this game mode CDO via ini / blueprint.
//
// On PostLogin the game mode logs the experience's HelloWorldMessage to
// LogPaldark and shows an on-screen debug message. That is the W1 hello-world.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameplayTagContainer.h"
#include "PaldarkGameModeBase.generated.h"

class UPaldarkExperienceDefinition;

UCLASS(Config = Game)
class PALDARKLAB_API APaldarkGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	APaldarkGameModeBase();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	// Read-only accessor used by Paldark.Net.Status / future HUD. Returns the
	// number of player controllers currently logged in. Authoritative on the
	// server only; clients should read PlayerArray on PlayerState.
	int32 GetConnectedPlayerCount() const { return ConnectedPlayerCount; }
	int32 GetMaxPlayers() const { return MaxPlayersPerSession; }

	// Snapshot of the experience metadata the game mode is currently using.
	// Server-side only; clients read the same data from game state. Used by
	// `Paldark.Experience.ListExtensions` console command.
	const FPrimaryAssetId& GetResolvedExperienceId() const { return ResolvedExperienceId; }
	const UPaldarkExperienceDefinition* GetLoadedExperience() const { return LoadedExperience; }
	const FGameplayTagContainer& GetActiveExperienceTags() const { return ActiveExperienceTags; }

protected:
	// Used when neither URL option nor world settings supply one. Soft so the
	// asset can be cooked-on-demand.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Experience")
	TSoftObjectPtr<UPaldarkExperienceDefinition> DefaultExperience;

	// W14-15 — Hard cap on concurrent player controllers. Read by PreLogin to
	// reject the (N+1)th client with a friendly error string instead of letting
	// AGameSession's untyped check fire. The 4-player roadmap target.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Net")
	int32 MaxPlayersPerSession = 4;

private:
	// Resolves the experience for this session and kicks off async load.
	void RequestExperienceLoad(const FString& Options);

	// Called once the async load completes (server-side).
	void OnExperienceAssetReady(FPrimaryAssetId AssetId);

	// W1 day 8-10 — Pulls PlayerControllerClass / PlayerStateClass / ActionSets
	// off the loaded experience. Class overrides are applied to the game mode
	// before any login happens; action sets contribute to ActiveExperienceTags.
	void ApplyExperienceClassOverrides(const UPaldarkExperienceDefinition& Experience);
	void ApplyExperienceActionSets(const UPaldarkExperienceDefinition& Experience);

	// W27-28 — Kicks off `UPaldarkPalSpawnSubsystem::RequestPreWarmAsync`
	// for every entry in `Experience.PreWarmPalDefinitions`. Fire-and-forget:
	// completion is logged via LogPaldarkPal but the game mode does NOT
	// block PostLogin / pawn spawn on the pre-warm finishing. SpawnPalAsync
	// callers pay a per-species miss penalty if they fire before pre-warm
	// completes.
	void RequestPalDefinitionPreWarm(const UPaldarkExperienceDefinition& Experience);

	// Effective experience id used by this session. Empty until InitGame resolves it.
	FPrimaryAssetId ResolvedExperienceId;

	// Server-side cache of the loaded experience. Mirrored onto game state once ready.
	UPROPERTY(Transient)
	TObjectPtr<const UPaldarkExperienceDefinition> LoadedExperience = nullptr;

	// Pawn class cached from the resolved experience's pawn data. Falls back to
	// AGameModeBase default pawn class if pawn data is unset.
	UPROPERTY(Transient)
	TSubclassOf<APawn> ResolvedPawnClass;

	// Union of `IntrinsicTags` from the experience + `GrantedTags` from every
	// action set. Read by console commands + future gameplay code.
	FGameplayTagContainer ActiveExperienceTags;

	// Latched flag: PostLogin defers its hello-world message + spawn behaviour
	// until the experience finishes loading.
	bool bExperienceLoaded = false;

	// W14-15 — Server-side counter incremented in PostLogin / decremented in
	// Logout. Mirrors the Game State PlayerArray length but kept here so the
	// PreLogin path can reject the 5th client without reading GameState (which
	// for AGameModeBase is updated AFTER PreLogin runs).
	int32 ConnectedPlayerCount = 0;
};
