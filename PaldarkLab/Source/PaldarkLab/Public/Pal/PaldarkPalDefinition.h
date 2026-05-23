// PALDARK W27-28 — Pal species definition (UPrimaryDataAsset).
//
// One DataAsset per Pal species under `/Game/Paldark/Pals/DA_PalDef_<Name>`.
// Carries the soft references the spawn pipeline needs (PalClass, SkeletalMesh,
// AnimInstanceClass, GrantedAbilities) so the cooker / runtime can stream just
// the per-species payload on demand instead of always cooking-and-loading every
// species at experience start.
//
// Two access paths:
//   1. `UPaldarkPalSpawnSubsystem::RequestPreWarmAsync(DefId)` — pre-warms the
//      "Spawn" asset bundle (mesh + anim instance class + ability classes) so
//      subsequent SpawnPalAsync calls hit warm caches.
//   2. `UPaldarkPalSpawnSubsystem::SpawnPalAsync(DefId, Transform)` — pre-warms
//      first if not warm, then spawns the Pal once everything is resolved.
//
// "Spawn" bundle membership is declared via `UpdateAssetBundleData()` so the
// Asset Manager tracks the soft refs as a single chunk; designers can register
// additional bundles (e.g. "Combat" with overlay materials) without touching
// C++.
//
// Registered as primary asset type `PaldarkPalDefinition` via both
// `Config/DefaultGame.ini` and `UPaldarkAssetManager::StartInitialLoading`
// runtime registration. The primary asset id is `PaldarkPalDefinition:<RowName>`
// matching the Lyra convention used by `UPaldarkItemDefinition` (W11-12) and
// `UPaldarkExperienceDefinition` (W1).
//
// Spawn pipeline server-only: clients see spawned Pals through normal actor
// replication once the server has resolved + spawned them.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PaldarkPalDefinition.generated.h"

class APaldarkPalCharacter;
class UAnimInstance;
class UGameplayAbility;
class UPaldarkAnimInstance;
class UPaldarkGameplayAbility;
class USkeletalMesh;

UCLASS(BlueprintType, Const, meta = (DisplayName = "Paldark Pal Definition"))
class PALDARKLAB_API UPaldarkPalDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPaldarkPalDefinition();

	// Identity ----------------------------------------------------------

	// Stable identity tag for this species. Must match a tag under
	// `Paldark.Pal.Species.*` (e.g. `Paldark.Pal.Species.Direhound`). The
	// spawn subsystem stamps this onto `APaldarkPalCharacter::SpeciesTag`
	// after spawn so the pack subsystem + HUD see consistent values.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Definition",
		meta = (Categories = "Paldark.Pal.Species"))
	FGameplayTag SpeciesTag;

	// Localised display name shown in spawn console output / future bestiary.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Definition")
	FText DisplayName;

	// Soft references — pulled by the "Spawn" asset bundle. -------------

	// Pal pawn class to spawn. Required — leaving this null means
	// `SpawnPalAsync` logs a warning and returns nullptr. Soft so the BP
	// blueprint asset cooks on demand instead of being root-loaded at
	// AssetManager init.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Definition",
		AssetRegistrySearchable, meta = (AssetBundles = "Spawn"))
	TSoftClassPtr<APaldarkPalCharacter> PalClass;

	// SkeletalMesh override applied to the spawned Pal's mesh component
	// after spawn. Leave null to use the mesh baked into the Pal's BP CDO
	// (designer-default). Soft — pulled via the "Spawn" bundle.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Definition",
		AssetRegistrySearchable, meta = (AssetBundles = "Spawn"))
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

	// AnimInstance class applied to the spawned Pal's mesh after spawn.
	// Overrides `UPaldarkPawnData::DefaultAnimInstanceClass` (W27). Leave
	// null to inherit from PawnData / BP. Soft — pulled via "Spawn" bundle.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Definition",
		AssetRegistrySearchable, meta = (AssetBundles = "Spawn"))
	TSoftClassPtr<UPaldarkAnimInstance> AnimInstanceClass;

	// Gameplay abilities granted to the Pal's ASC server-side after
	// PossessedBy resolves. Soft so the BP_GA_* assets don't pull until
	// the species actually spawns. "Spawn" bundle.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Definition",
		AssetRegistrySearchable, meta = (AssetBundles = "Spawn"))
	TArray<TSoftClassPtr<UGameplayAbility>> GrantedAbilities;

	// Eager stats — small, no benefit to streaming. Applied to the Pal's
	// AttributeSet server-side after spawn (`MaxHealth` → Health/MaxHealth
	// init, `MoveSpeedBase` → MoveSpeed init).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Definition|Stats",
		meta = (ClampMin = "1.0"))
	float MaxHealth = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Definition|Stats",
		meta = (ClampMin = "0.0"))
	float MoveSpeedBase = 600.f;

	// W35-36 — Per-species tame parameters. `TameDifficulty` is the divisor
	// in the capture probability formula (higher = harder to tame). Boltmane
	// boss sets 2.5, Direhound / Razorbird 1.0 (baseline), Foxparks 0.6
	// (easiest companion species). `BaseCaptureProbability` is the additive
	// baseline before HP / Stun / sphere-tier multipliers are applied;
	// designer can dial per-species "tameability" without touching code.
	// `UPaldarkPalTameComponent::HydrateFromPalDefinition` copies these
	// values into the component on Pal BeginPlay; component UPROPERTY
	// defaults stand if no definition is resolved.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Definition|Tame",
		meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float TameDifficulty = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Definition|Tame",
		meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float BaseCaptureProbability = 0.35f;

	// W39 — L-28 closure. Per-species boss-phase HP transition thresholds.
	// Read by `UPaldarkActivity_BossPhase::HydrateThresholdsFromPalDefinition`
	// on activity enter; replaces the hardcoded class-CDO defaults
	// (`EnragedThresholdPct=0.5f` + `TelegraphThresholdPct=0.25f`) that
	// previously required a designer to subclass the activity per boss to
	// retune. Each entry is a normalized HP fraction `[0..1]` at which the
	// boss steps into the next monotonic phase:
	//   - PhaseHealthThresholds[0]: Normal → Enraged       (e.g. 0.6 = 60% HP).
	//   - PhaseHealthThresholds[1]: Enraged → Telegraph    (e.g. 0.3 = 30% HP).
	// Order is descending; the activity asserts the order on hydrate and
	// falls back to its UPROPERTY defaults if the array is malformed.
	// Empty array = "use activity defaults" (the W29-30 baseline for back-
	// compat with existing `DA_PalDef_Boltmane` authors who haven't
	// re-saved with the new field). Designers point at non-boss species
	// (Direhound, Foxparks, ...) can leave this empty since their activity
	// ladder doesn't include `UPaldarkActivity_BossPhase` and the field is
	// then dead data. The validator's L-28 check enforces this field is
	// declared on the header so the contract stays compile-stable; it
	// does NOT police whether `DA_PalDef_Boltmane` actually populates it
	// (that's a designer-side authoring task, similar to L-25 / L-27).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Definition|Boss",
		meta = (ClampMin = "0.05", ClampMax = "0.95",
		        ToolTip = "Descending HP fractions per boss phase (e.g. [0.6, 0.3] for 2-phase). Empty = use activity defaults."))
	TArray<float> PhaseHealthThresholds;

	// Primary asset id with the `PaldarkPalDefinition` type. Matches the
	// row in `[/Script/Engine.AssetManagerSettings]` so editor + runtime
	// discovery stay aligned.
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	// Debug label used by the spawn console command + dump command.
	// Returns `DisplayName` when set, otherwise `SpeciesTag` string,
	// otherwise the asset name.
	FString GetDebugLabel() const;
};
