#include "Pal/PaldarkPalCharacter_Direhound.h"

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

APaldarkPalCharacter_Direhound::APaldarkPalCharacter_Direhound()
{
	// Identity flips — the base class set TeamTag=Player + SpeciesTag=<empty>.
	// Override here so the player-side Pal's perception treats this actor as
	// a threat (HostileTeamTag default = Paldark.Team.Hostile).
	TeamTag    = PaldarkGameplayTags::TAG_Paldark_Team_Hostile;
	SpeciesTag = PaldarkGameplayTags::TAG_Paldark_Pal_Species_Direhound;

	// Faster than the player base. Designer can override on the Blueprint
	// subclass per encounter (e.g. an Alpha Direhound at 800).
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = 650.f;
	}

	// Perception flip — Direhound treats Paldark.Team.Player as the enemy.
	if (UPaldarkPalPerceptionComponent* Perception = PerceptionSlot)
	{
		Perception->FriendlyTeamTag = PaldarkGameplayTags::TAG_Paldark_Team_Hostile;
		Perception->HostileTeamTag  = PaldarkGameplayTags::TAG_Paldark_Team_Player;
	}

	// Melee tuning. The W18-19 generic defaults are tuned for ranged
	// player-side Pals; Direhound is a bite-range predator.
	if (UPaldarkPalCombatComponent* Combat = CombatSlot)
	{
		Combat->MinEngageRange = 60.f;
		Combat->MaxEngageRange = 250.f;
		Combat->AttackInterval = 1.0f;
		Combat->BasePalDamage  = 18.f;
	}

	// Hostile activity ladder. Drops Follow (player-side leash) +
	// Investigate (ping-driven), adds Patrol (P15) + Stalk (P25) on top of
	// Combat (P40) from W18-19. Idle (P10) stays as the lowest fallback.
	if (UPaldarkPalActivityComponent* Activity = ActivitySlot)
	{
		Activity->CandidateActivities = {
			UPaldarkActivity_Idle::StaticClass(),
			UPaldarkActivity_Patrol::StaticClass(),
			UPaldarkActivity_Stalk::StaticClass(),
			UPaldarkActivity_Combat::StaticClass(),
		};
	}

	// W35-36 — Hostile, so Pal Sphere hits resolve a capture attempt. The
	// per-species `TameDifficulty` + `BaseCaptureProbability` come from the
	// matching `DA_PalDef_Direhound` data asset; the tame component
	// hydrates these on the first overlap.
	if (UPaldarkPalTameComponent* Tame = TameSlot)
	{
		Tame->bIsTameable = true;
	}
}

void APaldarkPalCharacter_Direhound::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	// Auto-register into the pack subsystem if a PackTag is already set
	// (spawner-driven flow stamps PackTag BEFORE FinishSpawning so this
	// runs after the stamp). Hand-placed Direhounds set PackTag on the BP
	// default object; the same code path handles them.
	if (!PackTag.IsValid())
	{
		// Default lone-Direhound to the canonical pack tag so even a hand-
		// placed actor benefits from pack broadcasts if any other Direhound
		// exists in the level.
		PackTag = PaldarkGameplayTags::TAG_Paldark_Pal_Pack_Direhound;
	}

	if (UWorld* World = GetWorld())
	{
		if (UPaldarkHostilePackSubsystem* PackSub = World->GetSubsystem<UPaldarkHostilePackSubsystem>())
		{
			PackSub->RegisterPalToPack(this, PackTag);
		}
	}

	UE_LOG(LogPaldarkPal, Log,
		TEXT("APaldarkPalCharacter_Direhound::BeginPlay — %s team=%s species=%s pack=%s"),
		*GetName(),
		*TeamTag.ToString(),
		*SpeciesTag.ToString(),
		*PackTag.ToString());
}

void APaldarkPalCharacter_Direhound::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
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
