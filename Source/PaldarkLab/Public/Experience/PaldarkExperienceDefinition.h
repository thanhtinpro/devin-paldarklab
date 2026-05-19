// PALDARK W1 day 6-7 — Experience definition data asset.
//
// Vendor-neutral mirror of Lyra's `ULyraExperienceDefinition`. An "experience" is
// the top-level data-driven description of a play session: which pawn class to
// spawn, which abilities/input to load, which game features to enable.
//
// W1 hello-world scope keeps just three fields:
//   - DisplayName     : human label for the experience.
//   - DefaultPawnData : reference to a Paldark Pawn Data asset (pawn class + input).
//   - HelloWorldMessage: text the GameMode logs / broadcasts on PostLogin so a
//                        designer can verify the experience loaded end-to-end.
//
// Action sets, game feature plugin names, and pawn extension components get added
// at W3+ when those systems exist. This is the minimum surface area required to
// satisfy the W1 outcome "Lyra Experience hello-world".

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PaldarkExperienceDefinition.generated.h"

class APaldarkGameModeBase;
class APaldarkPlayerController;
class APaldarkPlayerState;
class UPaldarkExperienceActionSet;
class UPaldarkMapDefinition;
class UPaldarkPalDefinition;
class UPaldarkPawnData;

UCLASS(BlueprintType, Const, meta = (DisplayName = "Paldark Experience"))
class PALDARKLAB_API UPaldarkExperienceDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPaldarkExperienceDefinition();

	// Human-readable label, shown in logs / UI.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Experience")
	FText DisplayName;

	// Pawn data used by this experience. Soft pointer so the data asset can be
	// async-loaded by AssetManager along with the experience itself.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Experience")
	TSoftObjectPtr<UPaldarkPawnData> DefaultPawnData;

	// Logged via LogPaldark when the experience finishes loading on the server.
	// "hello world" surface for W1; replaced by gameplay events at W3+.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Experience")
	FText HelloWorldMessage;

	// W1 day 8-10 — Lyra-style action sets. Each action set bundles additive
	// extensions (gameplay tags now, ability sets + game features later).
	// Soft pointers so the experience asset only pulls them on load.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Experience")
	TArray<TSoftObjectPtr<UPaldarkExperienceActionSet>> ActionSets;

	// PlayerController class used while this experience is active. Leave null to
	// fall back to the game mode's default. Soft so the class doesn't pull at
	// experience-asset load time.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Player")
	TSoftClassPtr<APaldarkPlayerController> PlayerControllerClass;

	// PlayerState class used while this experience is active. Same fallback +
	// soft-loading rules as PlayerControllerClass.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Player")
	TSoftClassPtr<APaldarkPlayerState> PlayerStateClass;

	// W26 — Required GameMode class for this experience. Unlike
	// PlayerControllerClass / PlayerStateClass above, the GameMode class itself
	// cannot be swapped after `InitGame`, so this is an *assertion* rather
	// than an override: when the experience finishes loading, the GameMode
	// inspects this field and logs a loud Error + on-screen red warning if
	// the currently spawned GameMode is not a subclass of RequiredGameModeClass.
	// Designer fix path is map-level World Override or DefaultEngine.ini
	// `+GameModeClassAliases` + `?game=<Alias>` URL flag. Leave null for
	// experiences that work with the default GameMode (e.g. HelloWorld).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Player")
	TSoftClassPtr<APaldarkGameModeBase> RequiredGameModeClass;

	// Gameplay tags asserted on this experience at load time. Concrete consumers
	// (HUD, AI director, game features) gate behaviour off these tags. The set
	// is the union of `IntrinsicTags` and every action set's `GrantedTags`.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Tags")
	FGameplayTagContainer IntrinsicTags;

	// W27-28 — Pal definitions whose "Spawn" asset bundle should be
	// pre-warmed on the server as soon as this experience finishes loading.
	// Fire-and-forget: the spawn subsystem (`UPaldarkPalSpawnSubsystem`)
	// kicks off async loads in parallel and logs each completion to
	// LogPaldarkPal. Subsequent `SpawnPalAsync` calls hit warm caches
	// instead of paying the streaming cost mid-fight.
	//
	// Designers populate this with the species expected to spawn in the
	// experience's first 30 s of play (e.g. PX_RaidSandbox lists Direhound
	// + Razorbird). Species that only appear later via Activity Director
	// can be left out and will pay a one-time stream cost on first spawn.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal")
	TArray<TSoftObjectPtr<UPaldarkPalDefinition>> PreWarmPalDefinitions;

	// W31-32 — Map definition this experience is balanced for. Soft so
	// the experience asset doesn't root-load the .umap on AssetManager
	// init; the GameMode resolves + logs it on `OnExperienceAssetReady`
	// so designers can spot mismatches between the experience's expected
	// map and the level the server actually opened (a common bug after
	// hand-editing DefaultEngine.ini map defaults). Leave null for
	// experiences that have no opinion about the map (e.g. HelloWorld /
	// debug experiences shared across all sandboxes).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Map")
	TSoftObjectPtr<UPaldarkMapDefinition> MapDefinition;

	// W39 — L-21 closure. Plugin names of every Game Feature this
	// experience requires to be in the `Active` state by the time
	// `OnExperienceAssetReady` fires. Plain FString (not FName / soft
	// ptr) because the Game Features subsystem keys plugins by string
	// name resolved via `UGameFeaturesSubsystem::GetPluginURLByName`.
	//
	// Unlike `RequiredGameModeClass` (which the engine can read but
	// the GameMode class cannot be swapped at runtime), Game Feature
	// plugins CAN be activated at runtime — but only if the host
	// project enables them in `PaldarkLab.uproject` Plugins[].
	// Designers who forget to enable a required plugin silently
	// boot the experience with missing content (no `DA_PalDef_*`
	// from `PaldarkRaidContent`, etc.). This field plus the runtime
	// assertion in the base GameMode turns that silent failure into
	// a loud Error log + red on-screen warning.
	//
	// Example entry: "PaldarkRaidContent" (the W27-28 game-feature
	// plugin shipping Pal definitions + maps + raid abilities).
	// Leave empty for experiences that don't depend on any feature
	// plugin (e.g. HelloWorld).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|GameFeatures")
	TArray<FString> RequiredGameFeatures;

	// W40-41 — Per-experience max-player cap. Read by
	// `APaldarkGameModeBase::OnExperienceAssetReady` to seed the GameMode's
	// `MaxPlayersPerSession` after the experience finishes loading. The
	// raid path keeps the default 4 (matches the W14-15 dedicated server
	// target); hub experiences (`PX_HubTown`) override to 8 (matches the
	// roadmap Q4 § Tuần 40-41 "8-player shard" line item) so two raid
	// teams plus a few stragglers can share one hub instance.
	//
	// The cap is read after async load, NOT in `InitGame`, because the
	// experience asset is not yet resolved at InitGame time. `PreLogin`
	// uses the live cap, so the (N+1)th client is still rejected with a
	// friendly error string even if they connect before the experience
	// finishes loading (in which case the CDO default 4 from
	// `MaxPlayersPerSession` applies until OnExperienceAssetReady fires).
	//
	// W42-43 backend AWS reads this field directly to size GameLift
	// fleet matchmaking buckets — keep the contract stable.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Net",
		meta = (ClampMin = "1", UIMin = "1", UIMax = "64"))
	int32 MaxPlayers = 4;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
