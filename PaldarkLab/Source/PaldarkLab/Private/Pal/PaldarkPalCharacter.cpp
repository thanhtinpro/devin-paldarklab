#include "Pal/PaldarkPalCharacter.h"

#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

#include "Gas/PaldarkAbilitySystemComponent.h"
#include "Gas/PaldarkAttributeSet.h"
#include "Loot/PaldarkLootDropComponent.h"
#include "Pal/Components/PaldarkPalActivityComponent.h"
#include "Pal/Components/PaldarkPalAnimDriverComponent.h"
#include "Pal/Components/PaldarkPalBondComponent.h"
#include "Pal/Components/PaldarkPalCombatComponent.h"
#include "Pal/Components/PaldarkPalDataComponent.h"
#include "Pal/Components/PaldarkPalHealthComponent.h"
#include "Pal/Components/PaldarkPalLocomotionComponent.h"
#include "Pal/Components/PaldarkPalPatrolComponent.h"
#include "Pal/Components/PaldarkPalPerceptionComponent.h"
#include "Pal/Components/PaldarkPalTameComponent.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"

APaldarkPalCharacter::APaldarkPalCharacter()
{
	// Pals should be replicated to all clients — combat / pings depend on
	// every player seeing the same Pal positions.
	bReplicates = true;
	SetReplicateMovement(true);

	// W14-15 — Tune Pal replication frequency for the 4-player dedicated test
	// target. Lower than the player's 100Hz default because the Pal AI runs
	// server-side and there is no input prediction — clients only need the
	// server's transform stream. Significance Manager (W48-49) will further
	// scale this dynamically.
	NetUpdateFrequency    = 30.0f; // Hz, matches server tick.
	MinNetUpdateFrequency = 5.0f;  // Hz, far-from-player floor.
	NetCullDistanceSquared = 2.5e9f; // cm^2 ~ 500m. Coop maps fit in this radius.

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		// Smaller default speed than the player so the follow steering doesn't
		// overshoot. Designers tweak per species via Blueprint.
		Movement->MaxWalkSpeed             = 450.0f;
		Movement->bOrientRotationToMovement = true;
		Movement->RotationRate             = FRotator(0.0f, 480.0f, 0.0f);
		Movement->bConstrainToPlane        = false;
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		// Smaller default capsule than the player; species can override.
		Capsule->InitCapsuleSize(34.0f, 60.0f);
	}

	// 8 component slots, one for each Pal subsystem. Locomotion is the only
	// slot with real W3-4 behaviour; everything else is a placeholder filled in
	// during later weeks (see each header's "Future home" comment).
	LocomotionSlot = CreateDefaultSubobject<UPaldarkPalLocomotionComponent>(TEXT("LocomotionSlot"));
	HealthSlot     = CreateDefaultSubobject<UPaldarkPalHealthComponent>    (TEXT("HealthSlot"));
	CombatSlot     = CreateDefaultSubobject<UPaldarkPalCombatComponent>    (TEXT("CombatSlot"));
	BondSlot       = CreateDefaultSubobject<UPaldarkPalBondComponent>      (TEXT("BondSlot"));
	ActivitySlot   = CreateDefaultSubobject<UPaldarkPalActivityComponent>  (TEXT("ActivitySlot"));
	DataSlot       = CreateDefaultSubobject<UPaldarkPalDataComponent>      (TEXT("DataSlot"));
	PerceptionSlot = CreateDefaultSubobject<UPaldarkPalPerceptionComponent>(TEXT("PerceptionSlot"));
	AnimDriverSlot = CreateDefaultSubobject<UPaldarkPalAnimDriverComponent>(TEXT("AnimDriverSlot"));

	// W20-21 — Patrol slot. Empty waypoint list by default; hostile-Pal
	// subclasses leave the empty list (the spawner fills it at runtime) or
	// designers add hand-authored waypoints on the Blueprint subclass for
	// stationary guards.
	PatrolSlot     = CreateDefaultSubobject<UPaldarkPalPatrolComponent>    (TEXT("PatrolSlot"));

	// W33-34 — Loot drop listener. Default LootTableId is empty so the
	// component is a no-op on the player-side base. Hostile-Pal subclasses
	// stamp the LootTableId in their ctor / BP defaults.
	LootDropSlot   = CreateDefaultSubobject<UPaldarkLootDropComponent>    (TEXT("LootDropSlot"));

	// W35-36 — Tame listener. `bIsTameable` defaults to false on the base
	// so player-side companions don't accept Pal Sphere hits. Hostile-Pal
	// subclasses flip the flag true in their ctor (or designer flips it on
	// the BP CDO). Always created so the slot pattern is uniform — no
	// per-subclass CreateDefaultSubobject churn.
	TameSlot       = CreateDefaultSubobject<UPaldarkPalTameComponent>     (TEXT("TameSlot"));

	// W7-8 — ASC + AttributeSet on the Pal itself. Pals are AI-controlled and
	// have no PlayerState, so the ASC lives on the actor. Minimal replication
	// mode keeps bandwidth low for simulated proxies (no per-client mirror of
	// the full GE container — server-authoritative damage / sprint are enough).
	AbilitySystemComponent = CreateDefaultSubobject<UPaldarkAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UPaldarkAttributeSet>(TEXT("AttributeSet"));

	// W18-19 — Team default. Player-side Pal companion. Hostile-Pal
	// subclasses (Direhound / Razorbird in W20-21) flip to
	// `Paldark.Team.Hostile` in their Blueprint subclass.
	TeamTag = PaldarkGameplayTags::TAG_Paldark_Team_Player;
}

UAbilitySystemComponent* APaldarkPalCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void APaldarkPalCharacter::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogPaldarkPal, Log, TEXT("APaldarkPalCharacter::BeginPlay — %s ready (auth=%d)."),
		*GetName(), HasAuthority() ? 1 : 0);

	// W7-8 — Init the ASC actor info on BOTH server and client. The Pal owns
	// its own ASC, so owner = avatar = this. Standalone / dedicated server
	// spawn paths run BeginPlay before any controller possesses the Pal, so
	// we wire the ASC here rather than in PossessedBy.
	if (AbilitySystemComponent != nullptr)
	{
		AbilitySystemComponent->InitAbilityActorInfo(/*OwnerActor*/ this, /*AvatarActor*/ this);
		UE_LOG(LogPaldarkGAS, Log,
			TEXT("APaldarkPalCharacter::BeginPlay — pal=%s ASC bound (self-owned, auth=%d)."),
			*GetName(),
			HasAuthority() ? 1 : 0);
	}
}

void APaldarkPalCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	UE_LOG(LogPaldarkPal, Log, TEXT("APaldarkPalCharacter::PossessedBy — pal=%s controller=%s"),
		*GetName(),
		NewController != nullptr ? *NewController->GetName() : TEXT("<null>"));

	// W7-8 — Server-side. Re-init the actor info in case BeginPlay ran before
	// the Pal had a controller (RoN / Lyra do this defensively too). Pal-side
	// ability grant from a PalDataAsset is W9+ — for now the ASC is reachable
	// via the IAbilitySystemInterface and via `Paldark.Gas.DumpAttributes`.
	if (HasAuthority() && AbilitySystemComponent != nullptr)
	{
		AbilitySystemComponent->InitAbilityActorInfo(/*OwnerActor*/ this, /*AvatarActor*/ this);
	}
}

void APaldarkPalCharacter::SetFollowedPawn(APawn* InFollowedPawn)
{
	if (LocomotionSlot == nullptr)
	{
		UE_LOG(LogPaldarkPal, Warning, TEXT("APaldarkPalCharacter::SetFollowedPawn — %s has no LocomotionSlot."),
			*GetName());
		return;
	}
	LocomotionSlot->SetFollowedPawn(InFollowedPawn);
}

void APaldarkPalCharacter::SetPackTag(const FGameplayTag& InPackTag)
{
	if (!HasAuthority())
	{
		UE_LOG(LogPaldarkPal, Warning,
			TEXT("APaldarkPalCharacter::SetPackTag — refused on non-authority for %s."),
			*GetName());
		return;
	}
	PackTag = InPackTag;
	UE_LOG(LogPaldarkPal, Log,
		TEXT("APaldarkPalCharacter::SetPackTag — pal=%s pack=%s"),
		*GetName(),
		*PackTag.ToString());
}
