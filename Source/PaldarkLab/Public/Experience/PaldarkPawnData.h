// PALDARK W1 day 6-7 + 11-14 — Pawn data asset.
//
// Mirrors Lyra's `ULyraPawnData` pattern in a vendor-neutral way: a designer-authored
// data asset that says "this is the pawn class + input + abilities to use for a given
// experience". The Experience asset references one of these so we can swap player
// behaviour purely by data, not by code.
//
// W1 day 11-14 promotes the previous (single IMC + priority) shape into a list of
// IMC + priority rows, plus an `InputConfig` reference. Designers can now stack
// multiple mapping contexts (e.g. base + vehicle + UI) and the C++ side does the
// `AddMappingContext`/`BindAction` glue via tags.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PaldarkPawnData.generated.h"

class AHUD;
class APaldarkPalCharacter;
class APawn;
class UGameplayEffect;
class UInputMappingContext;
class UPaldarkAnimInstance;
class UPaldarkGameplayAbility;
class UPaldarkInputConfig;

// W3-4 — Single row authored on PawnData describing one Pal companion that
// should spawn alongside the player. Designers configure how the Pal follows
// + how many to spawn here. Native code (game mode / console command) does
// the actual spawn + leash-up.
USTRUCT(BlueprintType)
struct FPaldarkPalCompanionSpec
{
	GENERATED_BODY()

	// Pal pawn class. Soft so the Pal blueprint module isn't pulled at
	// experience-load time — the game mode loads it on demand at spawn.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftClassPtr<APaldarkPalCharacter> PalClass;

	// Spawn offset relative to the player's actor location. Default is behind
	// + slightly to the side so the Pal doesn't intersect the capsule.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FVector SpawnOffset = FVector(-200.0f, 150.0f, 0.0f);

	// Follow distance override — leave 0 to fall back to the locomotion
	// component's default (~5m).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0"))
	float FollowDistanceOverride = 0.0f;
};

// W1 day 11-14 — Single IMC row authored on PawnData / experiences. Mirrors the
// `FInputMappingContextAndPriority` struct Lyra exposes on its PawnData.
USTRUCT(BlueprintType)
struct FPaldarkMappingContextAndPriority
{
	GENERATED_BODY()

	// Mapping context asset. Soft pointer so it doesn't pull at module load.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftObjectPtr<UInputMappingContext> InputMapping;

	// Higher priority wins when multiple contexts bind the same key. Lyra
	// convention: base = 0, gameplay = 1, vehicle = 2, UI = 3.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 Priority = 0;

	// If true, also call `Settings->RegisterInputMappingContext` so the editor /
	// remap UI sees this context. W1 leaves false; W11+ UI flips it on.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bRegisterWithSettings = false;
};

UCLASS(BlueprintType, Const, meta = (DisplayName = "Paldark Pawn Data"))
class PALDARKLAB_API UPaldarkPawnData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPaldarkPawnData();

	// Pawn class spawned for a player using this data asset. Soft pointer so the
	// pawn module can be async-loaded with the experience.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pawn")
	TSoftClassPtr<APawn> PawnClass;

	// W3-4 — Pal companion spec list. Each row spawns one Pal next to the
	// player on possess (via game mode / `Paldark.Pal.SpawnTestCompanion`).
	// Leave empty for experiences that don't run with Pals (e.g. lobby).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pawn", meta = (TitleProperty = "PalClass"))
	TArray<FPaldarkPalCompanionSpec> DefaultPalCompanions;

	// W1 day 11-14 — Ordered list of (IMC, priority) rows pushed into the
	// UEnhancedInputLocalPlayerSubsystem on possess. Replaces the old single-IMC
	// `InputMappingContext` / `InputMappingPriority` fields from day 6-7.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Input", meta = (TitleProperty = "InputMapping"))
	TArray<FPaldarkMappingContextAndPriority> DefaultMappingContexts;

	// W1 day 11-14 — Tag→UInputAction* map binding the pawn's native handlers
	// (Move/Look/Jump) and (later) ability activations. Soft pointer so editor
	// data referenced inside the config does not pull at module load time.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Input")
	TSoftObjectPtr<UPaldarkInputConfig> InputConfig;

	// W1 day 8-10 — Optional HUD class used by APaldarkPlayerController when this
	// pawn data is active. Soft pointer so the HUD asset module isn't pulled at
	// experience-asset load time.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|UI")
	TSoftClassPtr<AHUD> HUDClass;

	// W7-8 — Abilities granted to the owner ASC on the first server-side
	// possession. Soft classes so designer-authored ability blueprints don't
	// pull at pawn-data load time. Each ability's `ActivationInputTag` is the
	// lookup key for the matching `UPaldarkInputConfig::AbilityInputActions`
	// row, so the input binding stays data-driven.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|GAS", meta = (TitleProperty = "AbilityClass"))
	TArray<TSoftClassPtr<UPaldarkGameplayAbility>> GrantedAbilities;

	// W7-8 — Gameplay effects applied to the owner ASC at startup. Use this
	// for InitAttributes / regen effects so designers don't have to spawn a
	// "boot" ability just to set up baseline values.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|GAS", meta = (TitleProperty = "EffectClass"))
	TArray<TSoftClassPtr<UGameplayEffect>> StartupEffects;

	// W27 — Default `UAnimInstance` subclass to drive this pawn's skeletal
	// mesh. The base `APaldarkCharacter` / `APaldarkPalCharacter` ctor leaves
	// the mesh's `AnimClass` empty (designer authors `ABP_Paldark_Player`,
	// `ABP_Paldark_Pal`, `ABP_Paldark_Hostile_Direhound`, etc. as Blueprint
	// subclasses of `UPaldarkAnimInstance` / `UPaldarkPalAnimInstance`).
	//
	// Empty soft pointer is acceptable — the pawn will keep whatever
	// AnimInstance the Blueprint subclass already set on its inherited Mesh
	// component. Designer-friendly path: assign this here once per pawn
	// data asset, the C++ pawn classes call `SetAnimInstanceClass` during
	// possess / BeginPlay so a re-init picks up the swap without re-running
	// the constructor.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Anim")
	TSoftClassPtr<UPaldarkAnimInstance> DefaultAnimInstanceClass;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
