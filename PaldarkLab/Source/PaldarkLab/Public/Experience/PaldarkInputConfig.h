// PALDARK W1 day 11-14 — Input config asset.
//
// Vendor-neutral mirror of Lyra's `ULyraInputConfig`. Designers fill in a list of
// (FGameplayTag, UInputAction) rows; C++ binds the right handler at runtime
// based on the tag, not on the action's name. This keeps input bindings:
//   - data-driven  (no hard-coded UInputAction* in the character),
//   - swappable    (different pawn data assets can point at different configs
//                   without re-compiling),
//   - extensible   (W7+ GAS abilities pick `AbilityInputActions` out of the
//                   same asset using the activation tag as the key).
//
// W1 day 11-14 ships only the data shape + lookup helpers. Concrete handlers
// live on APaldarkCharacter (Move/Look/Jump) and APaldarkPlayerController.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PaldarkInputConfig.generated.h"

class UInputAction;

// Pair of (tag, input action) — single row in the designer-facing table.
USTRUCT(BlueprintType)
struct FPaldarkInputAction
{
	GENERATED_BODY()

	// The UInputAction asset that fires the bound handler.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<const UInputAction> InputAction = nullptr;

	// Gameplay tag used as the lookup key in C++. Conventionally lives under
	// `Paldark.InputTag.*` (declared in `PaldarkGameplayTags.h`).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "Paldark.InputTag"))
	FGameplayTag InputTag;
};

UCLASS(BlueprintType, Const, meta = (DisplayName = "Paldark Input Config"))
class PALDARKLAB_API UPaldarkInputConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPaldarkInputConfig();

	// Native (non-ability) actions — Move, Look, Jump, etc. The character binds
	// these in SetupPlayerInputComponent.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Input", meta = (TitleProperty = "InputAction"))
	TArray<FPaldarkInputAction> NativeInputActions;

	// Ability activation actions — bound at W7+ when GAS lands. Reserved here
	// so the data shape doesn't churn between weeks.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Input", meta = (TitleProperty = "InputAction"))
	TArray<FPaldarkInputAction> AbilityInputActions;

	// Lookup helpers — return the UInputAction* for a given tag, or null if the
	// tag isn't in this config. `bLogNotFound` toggles the warning log so the
	// binding path can probe optional tags without spamming the log.
	const UInputAction* FindNativeInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = true) const;
	const UInputAction* FindAbilityInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = true) const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
