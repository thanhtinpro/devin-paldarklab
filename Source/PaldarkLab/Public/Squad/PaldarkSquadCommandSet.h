// PALDARK W22-23 — Squad radial command set (PrimaryDataAsset).
//
// Designer-authored list of `(FGameplayTag, FText DisplayName, UTexture2D* Icon)`
// rows. The radial-wheel widget (W22+ UMG task) reads these rows to populate
// the wheel; the runtime command component does NOT consume the icon — it
// only needs the tag. Splitting the asset off into its own data type keeps
// designer iteration cheap (rename a label / swap an icon → no recompile).
//
// Default content shipped in C++ is empty; the W22-23 PR adds a stub data
// asset `DA_SquadCommandSet_Default` via designer authoring (see README
// Authoring section). At runtime, `UPaldarkSquadCommandComponent` reads this
// asset's `Commands` array; the actual command broadcast goes through the
// `UPaldarkSquadSubsystem`.
//
// Why PrimaryDataAsset (not DataTable):
//   - PDA is loaded once + cached in AssetManager; no row-lookup cost per
//     `IssueCommand`. Mirrors the W11-12 inventory item-definition pattern.
//   - Designer-friendly inspector for icon previews.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PaldarkSquadCommandSet.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct FPaldarkSquadCommandRow
{
	GENERATED_BODY()

	// Tag identifying the command. Must be under `Paldark.Squad.Command.*`
	// so the activity FSM's tag matcher can route correctly. Default empty
	// — designer fills in per row.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Squad")
	FGameplayTag CommandTag;

	// Human-readable label shown on the radial wheel slice. Defaults to
	// the tag name for headless test runs.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Squad")
	FText DisplayName;

	// Optional icon for the wheel slice. Null is acceptable — the widget
	// falls back to a placeholder.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Squad")
	TSoftObjectPtr<UTexture2D> Icon;
};

UCLASS(BlueprintType)
class PALDARKLAB_API UPaldarkSquadCommandSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPaldarkSquadCommandSet();

	// Designer-authored command list. Order = wheel slice order (clockwise
	// from 12-o'clock). Designer should keep this <= 8 entries — the
	// radial wheel widget is hard-coded to 8 slots in W22+ UMG.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Squad")
	TArray<FPaldarkSquadCommandRow> Commands;

	// Static-typed asset bundle so AssetManager primary-asset queries can
	// find every command-set asset under a single PrimaryAssetType.
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	// Lookup helper for the runtime command component: returns true and
	// fills `OutRow` if `InCommandTag` is present in this set.
	bool FindRowByTag(const FGameplayTag& InCommandTag, FPaldarkSquadCommandRow& OutRow) const;
};
