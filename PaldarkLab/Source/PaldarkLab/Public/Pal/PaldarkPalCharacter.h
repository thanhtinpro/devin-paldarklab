// PALDARK W3-4 — Pal companion pawn skeleton.
//
// `APaldarkPalCharacter` is the base class for every Pal species in the game.
// W3-4 ships a deliberately tiny shape:
//   - inherits from ACharacter so navmesh + capsule + character movement come
//     for free,
//   - holds 8 empty component slots (locomotion / health / combat / bond /
//     activity / data / perception / anim driver) that designers will fill in
//     across later weeks,
//   - the locomotion component is the only one with real behaviour in W3-4:
//     it ticks toward the followed player at a configurable target distance.
//
// W7-8 additions:
//   - The Pal owns its own UPaldarkAbilitySystemComponent + UPaldarkAttributeSet
//     directly (no PlayerState — Pals are AI-controlled). Implements
//     IAbilitySystemInterface so GAS lookup helpers resolve to this actor.
//   - Server-side, GrantedAbilities from the matching UPaldarkPalDataAsset are
//     granted on PossessedBy (deferred until W9+ for the asset wiring).
//
// Future homes (do NOT implement here in W3-4):
//   - W5-6 Activity FSM ticking (port from `04.ReadyOrNot`).
//   - W20-21 hostile Pal AI subclasses (Direhound / Razorbird) — landed.
//
// W20-21 additions:
//   - 9th component slot `PatrolSlot` (UPaldarkPalPatrolComponent), reserved
//     for hostile subclasses; player-side Pals leave it null.
//   - `SpeciesTag` (default empty on the base class; Direhound/Razorbird
//     subclasses stamp it in their constructor) — read by the pack subsystem
//     to filter broadcasts by species.
//   - `PackTag` (default empty; hostile spawners stamp it on each spawned
//     Pal so the pack subsystem groups members per spawner).
//
// Spawn flow in W3-4:
//   APaldarkPlayerController::OnPossess (or `Paldark.Pal.SpawnTestCompanion`
//   console command) reads `UPaldarkPawnData::DefaultPalCompanionClasses` and
//   spawns one Pal per entry behind the player, then assigns the player as the
//   `FollowedPawn` on the Pal's locomotion component.

#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "PaldarkPalCharacter.generated.h"

class UAbilitySystemComponent;
class UPaldarkAbilitySystemComponent;
class UPaldarkAttributeSet;
class UPaldarkPalAnimDriverComponent;
class UPaldarkPalActivityComponent;
class UPaldarkPalBondComponent;
class UPaldarkPalCombatComponent;
class UPaldarkPalDataComponent;
class UPaldarkPalHealthComponent;
class UPaldarkPalLocomotionComponent;
class UPaldarkPalPatrolComponent;
class UPaldarkPalPerceptionComponent;
class UPaldarkPalTameComponent;
class UPaldarkLootDropComponent;

UCLASS()
class PALDARKLAB_API APaldarkPalCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	APaldarkPalCharacter();

	// IAbilitySystemInterface — Pal owns its own ASC because it has no
	// PlayerState (it is an AI pawn). The ASC sub-object is created in the
	// constructor and replicates with the pawn.
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UPaldarkAbilitySystemComponent* GetPaldarkAbilitySystemComponent() const { return AbilitySystemComponent; }
	const UPaldarkAttributeSet*     GetPaldarkAttributeSet()          const { return AttributeSet; }

	// Convenience helper for the spawn flow — sets the Pal's locomotion
	// component to follow the given pawn. Logged via LogPaldarkPal.
	void SetFollowedPawn(APawn* InFollowedPawn);

	// Accessors so other systems (HUD, debug console command) can poke at the
	// slot components without round-tripping through `FindComponentByClass`.
	UPaldarkPalLocomotionComponent* GetLocomotionSlot() const { return LocomotionSlot; }
	UPaldarkPalHealthComponent*     GetHealthSlot()     const { return HealthSlot; }
	UPaldarkPalCombatComponent*     GetCombatSlot()     const { return CombatSlot; }
	UPaldarkPalBondComponent*       GetBondSlot()       const { return BondSlot; }
	UPaldarkPalActivityComponent*   GetActivitySlot()   const { return ActivitySlot; }
	UPaldarkPalDataComponent*       GetDataSlot()       const { return DataSlot; }
	UPaldarkPalPerceptionComponent* GetPerceptionSlot() const { return PerceptionSlot; }
	UPaldarkPalAnimDriverComponent* GetAnimDriverSlot() const { return AnimDriverSlot; }
	UPaldarkPalPatrolComponent*     GetPatrolSlot()     const { return PatrolSlot; }
	UPaldarkLootDropComponent*      GetLootDropSlot()   const { return LootDropSlot; }
	UPaldarkPalTameComponent*       GetTameSlot()       const { return TameSlot; }

	// W18-19 — Team identity. Default is `Paldark.Team.Player` (the Pal is
	// on the player's side). Hostile-Pal subclasses (W20-21) flip this to
	// `Paldark.Team.Hostile` in their C++ subclass constructor.
	UFUNCTION(BlueprintPure, Category = "Paldark|Team")
	FGameplayTag GetTeamTag() const { return TeamTag; }

	// W20-21 — Species identity. Empty on the base class; set in the
	// hostile-Pal subclass constructor (e.g. `Paldark.Pal.Species.Direhound`).
	// Read by the pack subsystem to filter broadcasts + by HUD/debug code.
	UFUNCTION(BlueprintPure, Category = "Paldark|Pal")
	FGameplayTag GetSpeciesTag() const { return SpeciesTag; }

	// W20-21 — Pack identity (designer-authored or spawner-stamped). Empty
	// means "lone Pal, no pack broadcast". `UPaldarkHostilePackSubsystem`
	// reads this on the broadcasting Pal to know which pack to fan out to.
	UFUNCTION(BlueprintPure, Category = "Paldark|Pal|Pack")
	FGameplayTag GetPackTag() const { return PackTag; }

	// Set the pack tag at runtime (used by the spawner actor so designers
	// can stamp every spawned Pal with the spawner's authored PackTag).
	// Authority only; logs via LogPaldarkPal.
	void SetPackTag(const FGameplayTag& InPackTag);

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

	// 8 Pal component slots — empty placeholders in W3-4 except the locomotion
	// slot. Exposed via `VisibleAnywhere` so designers can swap subclasses in
	// derived Blueprint pawns.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Pal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkPalLocomotionComponent> LocomotionSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Pal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkPalHealthComponent> HealthSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Pal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkPalCombatComponent> CombatSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Pal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkPalBondComponent> BondSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Pal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkPalActivityComponent> ActivitySlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Pal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkPalDataComponent> DataSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Pal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkPalPerceptionComponent> PerceptionSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Pal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkPalAnimDriverComponent> AnimDriverSlot;

	// W20-21 — 9th slot. Reserved for hostile-Pal subclasses
	// (APaldarkPalCharacter_Direhound / Razorbird). Player-side Pals
	// leave this null — Patrol activity simply CanRun=false in that case.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Pal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkPalPatrolComponent> PatrolSlot;

	// W33-34 — 10th slot. Server-only listener on the Pal's
	// `UPaldarkAttributeSet::OnHealthZeroed`. Designer sets `LootTableId`
	// on this component (per hostile-Pal Blueprint subclass) to wire up the
	// drop. Player-side companions leave `LootTableId` empty so the
	// component subscribes nothing and is a no-op. Always present so
	// designers don't have to add it to every hostile-Pal BP manually.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Pal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkLootDropComponent> LootDropSlot;

	// W35-36 — 11th slot. Server-only tame listener. `bIsTameable` defaults
	// to false on this base; hostile-Pal subclasses (Direhound / Razorbird /
	// Stoneclad / Vinewraith / Boltmane) flip it true in their ctor so the
	// Pal Sphere overlap path resolves a capture attempt. Player-side
	// companions leave it false and the component is a no-op on overlap.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Pal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkPalTameComponent> TameSlot;

	// W7-8 — ASC + AttributeSet owned by the Pal directly. Pals have no
	// PlayerState (they are AI-controlled by APaldarkPalAIController in a
	// later week) so the ASC must live somewhere replicated — the actor is
	// the natural home. Replication mode = Minimal (server -> simulated
	// proxies only) which is the standard pattern for AI in Aura / Crunch.
	UPROPERTY(VisibleAnywhere, Category = "Paldark|GAS")
	TObjectPtr<UPaldarkAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UPaldarkAttributeSet> AttributeSet;

	// W18-19 — Team identity. Default `Paldark.Team.Player` set in the
	// constructor; hostile-Pal subclasses override in their constructor.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Team", meta = (AllowPrivateAccess = "true"))
	FGameplayTag TeamTag;

	// W20-21 — Species identity (e.g. `Paldark.Pal.Species.Direhound`). Set
	// in the hostile-Pal C++ subclass constructor; empty on the player-side
	// base.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Pal", meta = (AllowPrivateAccess = "true"))
	FGameplayTag SpeciesTag;

	// W20-21 — Pack identity. Set by the spawner at runtime (designer can
	// also override in BP for hand-placed Pals). Empty = lone Pal.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Pal|Pack", meta = (AllowPrivateAccess = "true"))
	FGameplayTag PackTag;
};
