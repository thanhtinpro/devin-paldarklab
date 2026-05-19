// PALDARK W1 day 8-10 / 11-14 / W3-4 — Player character skeleton.
//
// `ACharacter` subclass used as the default pawn when the experience's PawnData
// references it. Owns:
//   - the inherited capsule + mesh + character movement,
//   - a spring arm + camera in composition (P03 pillar gold standard),
//   - 12 empty component slots (W3-4) that designers + later weeks fill in.
//   - the Enhanced Input handlers (W1 day 11-14): SetupPlayerInputComponent
//     looks up the cached PawnData (set by APaldarkPlayerController on possess),
//     reads its `UPaldarkInputConfig`, and binds Move/Look/Jump via the
//     `Paldark.InputTag.*` lookup keys.
//
// The 12 player component slots are intentionally empty in W3-4 — they exist
// so each future-week subsystem (GAS, Inventory, Activity, Damage, Network,
// etc.) has a stable C++ entry point to extend instead of forcing a recompile
// of APaldarkCharacter every time a new system lands.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "PaldarkCharacter.generated.h"

class UAbilitySystemComponent;
class UBoxComponent;
class UCameraComponent;
class UEnhancedInputComponent;
class UInputAction;
class APaldarkLootBag;
class UPaldarkAbilitySystemComponent;
class UPaldarkAttributeSet;
class UPaldarkInputConfig;
class UPaldarkLagCompensationComponent;
class UPaldarkPawnData;
class UPaldarkPlayerActivityComponent;
class UPaldarkPlayerCameraExtensionComponent;
class UPaldarkPlayerCombatComponent;
class UPaldarkPlayerDamageComponent;
class UPaldarkPlayerEquipmentComponent;
class UPaldarkPlayerHealthComponent;
class UPaldarkPlayerInteractionComponent;
class UPaldarkPlayerInventoryComponent;
class UPaldarkPlayerLocomotionExtComponent;
class UPaldarkPlayerNetworkComponent;
class UPaldarkPlayerPalCompanionComponent;
class UPaldarkPlayerPalRosterComponent;
class UPaldarkPlayerStaminaComponent;
class UPaldarkSquadCommandComponent;
class UPaldarkSquadMembershipComponent;
class USpringArmComponent;
struct FInputActionValue;

UCLASS()
class PALDARKLAB_API APaldarkCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	APaldarkCharacter();

	// Called by APaldarkPlayerController during OnPossess (before Super) so the
	// SetupPlayerInputComponent path can pick up the cached InputConfig before
	// binding. Safe to call multiple times — re-binding is idempotent (we hold
	// onto the binding handles so RemoveActionBinding can clean up).
	void SetPawnData(const UPaldarkPawnData* InPawnData);

	// Returns the PawnData last set via SetPawnData, or null before possess.
	const UPaldarkPawnData* GetPawnData() const { return CachedPawnData; }

	// W18-19 — Read the team identity of this pawn. Pal perception calls
	// this via `UPaldarkPalPerceptionComponent::GetActorTeamTag` to decide
	// who counts as a threat. Default is `Paldark.Team.Player` (set in the
	// constructor); designer-authored player factions (PvP, multi-team)
	// override in the Blueprint subclass via TeamTag.
	UFUNCTION(BlueprintPure, Category = "Paldark|Team")
	FGameplayTag GetTeamTag() const { return TeamTag; }

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void OnRep_Controller() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// Bind native handlers (Move/Look/Jump) using the tag lookup. Splits into
	// its own function so SetPawnData can also drive it when PawnData arrives
	// after SetupPlayerInputComponent (rare but possible during late-joiners).
	void BindNativeInputActions();

	// W7-8 — Resolve and stash the PlayerState ASC on this pawn. Called from
	// both PossessedBy (server + standalone) and OnRep_PlayerState (autonomous
	// proxy on remote clients). Idempotent; subsequent calls are no-ops.
	void InitAbilitySystem();

	// W7-8 — Bind ability inputs (Sprint pressed/released) using the
	// UPaldarkInputConfig.AbilityInputActions table. Requires both the ASC and
	// CachedPawnData; bails (logs) when either is missing.
	void BindAbilityInputActions();

	// Enhanced Input handlers — kept tiny, just forward to the existing
	// AddMovementInput / AddControllerYawInput / Jump APIs so the W1 path is
	// the canonical "third-person template" mapping.
	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_Jump_Pressed(const FInputActionValue& Value);
	void Input_Jump_Released(const FInputActionValue& Value);

	// W7-8 — Sprint input handlers. Pressed -> TryActivate via tag; Released ->
	// CancelAbility by tag. Both go through the ASC so the activation is
	// predicted on the autonomous client and confirmed by the server.
	void Input_Sprint_Pressed(const FInputActionValue& Value);
	void Input_Sprint_Released(const FInputActionValue& Value);

	// W9-10 — Fire input handler. Started -> TryActivate via tag. Semi-auto:
	// the ability ends on the same frame so we don't need a Released handler
	// (full-auto / hold-to-fire is W17-18 weapon polish).
	void Input_Fire_Pressed(const FInputActionValue& Value);

	// Third-person camera rig — composition slot so designers can swap behaviour
	// from Blueprint without touching this class.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	// W3-4 — 12 empty player component slots. Each slot is a `UActorComponent`
	// subclass that future weeks fill in (see each header's "Future home"
	// comment). Slots live as UPROPERTY references so they ship in the CDO and
	// survive replication / blueprint subclassing without re-instantiation.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Slots", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkPlayerHealthComponent> HealthSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Slots", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkPlayerStaminaComponent> StaminaSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Slots", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkPlayerCombatComponent> CombatSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Slots", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkPlayerInventoryComponent> InventorySlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Slots", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkPlayerEquipmentComponent> EquipmentSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Slots", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkPlayerPalCompanionComponent> PalCompanionSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Slots", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkPlayerLocomotionExtComponent> LocomotionExtSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Slots", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkPlayerActivityComponent> ActivitySlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Slots", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkPlayerInteractionComponent> InteractionSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Slots", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkPlayerCameraExtensionComponent> CameraExtSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Slots", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkPlayerNetworkComponent> NetworkSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Slots", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkPlayerDamageComponent> DamageSlot;

	// W16-17 — Lag compensation slot. Server-only tick records per-bone
	// hitbox transforms into a frame buffer so the rewind algorithm can
	// resolve client-reported HitTime against historical geometry. Always
	// constructed (server / client) so the ServerScoreRequest RPC has an
	// owner; the cpp short-circuits the tick when the owner isn't authority.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Slots", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkLagCompensationComponent> LagCompSlot;

	// W22-23 — Squad system slots.
	//
	// SquadMembershipSlot: auto-registers the player into the
	// `UPaldarkSquadSubsystem` under `Paldark.Squad.Default` on BeginPlay
	// (authority only). Designer may flip `SquadTag` in BP_PaldarkCharacter
	// subclass to drop the player into a PvP team (W40+).
	//
	// SquadCommandSlot: owns the per-player Server_IssueCommand RPC for
	// the radial wheel. Holds a soft pointer to the
	// `DA_SquadCommandSet_Default` asset (designer-authored). Empty soft
	// pointer is acceptable for headless tests using
	// `Paldark.Squad.Command`.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Slots", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkSquadMembershipComponent> SquadMembershipSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Slots", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkSquadCommandComponent> SquadCommandSlot;

	// W35-36 — 13th player slot. Per-player roster of tamed Pals (transient,
	// match-scoped). Server appends entries in `UPaldarkPlayerPalRosterComponent::TamePal`;
	// the array replicates `COND_OwnerOnly` so each client only sees its own
	// roster. Persistence across raids is the W47 save-game pass — W35-36
	// roster is intentionally in-memory only.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Slots", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaldarkPlayerPalRosterComponent> RosterSlot;

public:
	// W16-17 — Read-only access to the per-bone hitbox map. The lag
	// compensation component reads this in CacheBoxPositions / MoveBoxes /
	// ResetBoxes / EnableCharacterMeshCollision; exposed via accessor so
	// the map can stay private but the rewind code doesn't need friendship.
	const TMap<FName, TObjectPtr<UBoxComponent>>& GetHitCollisionBoxes() const { return HitCollisionBoxes; }

	// Accessors so debug / future systems can poke at slots without round-tripping
	// through `FindComponentByClass`. Read-only on purpose — slot replacement is
	// a blueprint-subclass operation, not a runtime one.
	UPaldarkPlayerHealthComponent*            GetHealthSlot()         const { return HealthSlot; }
	UPaldarkPlayerStaminaComponent*           GetStaminaSlot()        const { return StaminaSlot; }
	UPaldarkPlayerCombatComponent*            GetCombatSlot()         const { return CombatSlot; }
	UPaldarkPlayerInventoryComponent*         GetInventorySlot()      const { return InventorySlot; }
	UPaldarkPlayerEquipmentComponent*         GetEquipmentSlot()      const { return EquipmentSlot; }
	UPaldarkPlayerPalCompanionComponent*      GetPalCompanionSlot()   const { return PalCompanionSlot; }
	UPaldarkPlayerLocomotionExtComponent*     GetLocomotionExtSlot()  const { return LocomotionExtSlot; }
	UPaldarkPlayerActivityComponent*          GetActivitySlot()       const { return ActivitySlot; }
	UPaldarkPlayerInteractionComponent*       GetInteractionSlot()    const { return InteractionSlot; }
	UPaldarkPlayerCameraExtensionComponent*   GetCameraExtSlot()      const { return CameraExtSlot; }
	UPaldarkPlayerNetworkComponent*           GetNetworkSlot()        const { return NetworkSlot; }
	UPaldarkPlayerDamageComponent*            GetDamageSlot()         const { return DamageSlot; }
	UPaldarkSquadMembershipComponent*         GetSquadMembershipSlot()const { return SquadMembershipSlot; }
	UPaldarkSquadCommandComponent*            GetSquadCommandSlot()   const { return SquadCommandSlot; }
	UPaldarkPlayerPalRosterComponent*         GetRosterSlot()         const { return RosterSlot; }

private:
	// W16-17 — Per-bone hitbox map. Constructor creates one UBoxComponent
	// per skeletal mesh socket and attaches them via AttachToComponent so
	// each box rides the animated bone. Boxes default to NoCollision so
	// they don't interfere with live combat / movement — the lag comp
	// component flips them to QueryOnly only during a rewind trace.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|HitBoxes", meta = (AllowPrivateAccess = "true"))
	TMap<FName, TObjectPtr<UBoxComponent>> HitCollisionBoxes;

	// PawnData handed in by the controller — owns the InputConfig + IMC list.
	UPROPERTY(Transient)
	TObjectPtr<const UPaldarkPawnData> CachedPawnData;

	// EnhancedInputComponent captured during SetupPlayerInputComponent. Held so
	// late-arriving PawnData (via SetPawnData) can still trigger binding.
	UPROPERTY(Transient)
	TObjectPtr<UEnhancedInputComponent> CachedEnhancedInputComponent;

	// W7-8 — Cached strongly-typed ASC pointer resolved from PlayerState on
	// possess (server / standalone) or OnRep_PlayerState (clients). Owned by
	// the PlayerState; this pawn never CreateDefaultSubobject's its own ASC.
	UPROPERTY(Transient)
	TObjectPtr<UPaldarkAbilitySystemComponent> CachedAbilitySystemComponent;

	// W24-25 — Server-only handle + cached pointer for the
	// `UPaldarkAttributeSet::OnHealthZeroed` binding that routes the
	// player's death into `UPaldarkMatchSubsystem::RecordDeath`. The
	// AttributeSet ref is cached so EndPlay can `Remove(Handle)` cleanly
	// even after the PlayerState pointer has gone away (e.g. seamless
	// travel between maps).
	UPROPERTY(Transient)
	TObjectPtr<UPaldarkAttributeSet> CachedDeathHookAttributeSet;

	FDelegateHandle MatchDeathHookHandle;

	// Guard against double-binding when both SetupPlayerInputComponent and a
	// late SetPawnData try to bind the same handlers.
	bool bNativeInputBound  = false;
	bool bAbilityInputBound = false;

	// W18-19 — Team identity. Used by Pal perception (friendly filter) and
	// by future PvP/raid wave logic to decide who counts as a threat. The
	// default value is set in the constructor to `Paldark.Team.Player`.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Team", meta = (AllowPrivateAccess = "true"))
	FGameplayTag TeamTag;

	// W37-38 — Loot bag class to spawn when this character dies. The match
	// death hook (set up in InitAbilitySystem) packs the player's current
	// inventory `Entries` into this bag's `InitializeContents` and clears
	// the inventory, mirroring the W33-34 hostile-Pal loot drop. Leave null
	// to disable drop-on-death (e.g. PvE bot pawns); the default value in
	// the constructor points at `APaldarkLootBag::StaticClass()` so designers
	// don't need to wire it manually.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Loot", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<APaldarkLootBag> PlayerDeathBagClass;

	// W37-38 — Pulls the inventory contents into a freshly-spawned
	// `APaldarkLootBag` and clears the inventory. Server-only; called from
	// the `OnHealthZeroed` lambda installed in `InitAbilitySystem`.
	void SpawnDeathLootBagFromInventory();
};
