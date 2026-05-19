#include "Pal/PaldarkPalCharacter_Razorbird.h"

#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Pal/Activities/PaldarkActivity_Combat.h"
#include "Pal/Activities/PaldarkActivity_Idle.h"
#include "Pal/Activities/PaldarkActivity_Patrol.h"
#include "Pal/Activities/PaldarkActivity_Stalk.h"
#include "Pal/Components/PaldarkPalActivityComponent.h"
#include "Pal/Components/PaldarkPalCombatComponent.h"
#include "Pal/Components/PaldarkPalPerceptionComponent.h"
#include "Pal/Components/PaldarkPalTameComponent.h"
#include "Pal/PaldarkHostilePackSubsystem.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"

APaldarkPalCharacter_Razorbird::APaldarkPalCharacter_Razorbird()
{
	TeamTag    = PaldarkGameplayTags::TAG_Paldark_Team_Hostile;
	SpeciesTag = PaldarkGameplayTags::TAG_Paldark_Pal_Species_Razorbird;

	// Aerial movement — gravity off so the Pal hovers instead of falling.
	// MaxFlySpeed (UCharacterMovementComponent) drives top speed in flying
	// mode; the activity drives planar input via AddMovementInput, the Z
	// component falls out of waypoint placement (designers place waypoints
	// above the ground for the hover-then-dive feel).
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->DefaultLandMovementMode = MOVE_Flying;
		Movement->SetMovementMode(MOVE_Flying);
		Movement->MaxFlySpeed             = 700.f;
		Movement->BrakingDecelerationFlying = 2048.f;
		Movement->GravityScale            = 0.f;
		Movement->bOrientRotationToMovement = true;
	}

	if (UPaldarkPalPerceptionComponent* Perception = PerceptionSlot)
	{
		Perception->FriendlyTeamTag = PaldarkGameplayTags::TAG_Paldark_Team_Hostile;
		Perception->HostileTeamTag  = PaldarkGameplayTags::TAG_Paldark_Team_Player;

		// Wider scan radius — birds see further than ground predators.
		Perception->ThreatRadius = 2200.f;
	}

	if (UPaldarkPalCombatComponent* Combat = CombatSlot)
	{
		Combat->MinEngageRange = 100.f;
		Combat->MaxEngageRange = 800.f;
		Combat->AttackInterval = 2.0f;
		Combat->BasePalDamage  = 14.f;
	}

	if (UPaldarkPalActivityComponent* Activity = ActivitySlot)
	{
		Activity->CandidateActivities = {
			UPaldarkActivity_Idle::StaticClass(),
			UPaldarkActivity_Patrol::StaticClass(),
			UPaldarkActivity_Stalk::StaticClass(),
			UPaldarkActivity_Combat::StaticClass(),
		};
	}

	// W35-36 — Hostile, so Pal Sphere hits resolve a capture attempt.
	if (UPaldarkPalTameComponent* Tame = TameSlot)
	{
		Tame->bIsTameable = true;
	}
}

void APaldarkPalCharacter_Razorbird::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	// Razorbirds default to lone — no Pack.Razorbird tag in W20-21. A
	// designer can still stamp the spawner's PackTag onto the Pal manually
	// to share threat sightings between birds.
	if (PackTag.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			if (UPaldarkHostilePackSubsystem* PackSub = World->GetSubsystem<UPaldarkHostilePackSubsystem>())
			{
				PackSub->RegisterPalToPack(this, PackTag);
			}
		}
	}

	UE_LOG(LogPaldarkPal, Log,
		TEXT("APaldarkPalCharacter_Razorbird::BeginPlay — %s team=%s species=%s pack=%s mode=Flying"),
		*GetName(),
		*TeamTag.ToString(),
		*SpeciesTag.ToString(),
		*PackTag.ToString());
}

void APaldarkPalCharacter_Razorbird::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority() && PackTag.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			if (UPaldarkHostilePackSubsystem* PackSub = World->GetSubsystem<UPaldarkHostilePackSubsystem>())
			{
				PackSub->UnregisterPalFromPack(this);
			}
		}
	}
	Super::EndPlay(EndPlayReason);
}
