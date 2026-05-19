#include "Pal/PaldarkPalCharacter_Stoneclad.h"

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

APaldarkPalCharacter_Stoneclad::APaldarkPalCharacter_Stoneclad()
{
	TeamTag    = PaldarkGameplayTags::TAG_Paldark_Team_Hostile;
	SpeciesTag = PaldarkGameplayTags::TAG_Paldark_Pal_Species_Stoneclad;

	// Heavy + slow — reads as a stone-armoured ground tank.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = 380.f;
	}

	if (UPaldarkPalPerceptionComponent* Perception = PerceptionSlot)
	{
		Perception->FriendlyTeamTag = PaldarkGameplayTags::TAG_Paldark_Team_Hostile;
		Perception->HostileTeamTag  = PaldarkGameplayTags::TAG_Paldark_Team_Player;
	}

	// Slow-melee tuning. Wide MaxEngageRange covers the charge windup
	// without losing the lock; high damage compensates the slow rhythm.
	if (UPaldarkPalCombatComponent* Combat = CombatSlot)
	{
		Combat->MinEngageRange = 80.f;
		Combat->MaxEngageRange = 280.f;
		Combat->AttackInterval = 2.0f;
		Combat->BasePalDamage  = 28.f;
	}

	// Standard hostile ladder. NB: assignment, not Append, so Stoneclad
	// does NOT inherit FollowSquadCommand from any future base default
	// (L-13 validator enforces this).
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
	// stone-armoured tank is the hardest of the W29-30 hostile set; the
	// final difficulty number comes from `DA_PalDef_Stoneclad`.
	if (UPaldarkPalTameComponent* Tame = TameSlot)
	{
		Tame->bIsTameable = true;
	}
}

void APaldarkPalCharacter_Stoneclad::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	if (!PackTag.IsValid())
	{
		PackTag = PaldarkGameplayTags::TAG_Paldark_Pal_Pack_Stoneclad;
	}

	if (UWorld* World = GetWorld())
	{
		if (UPaldarkHostilePackSubsystem* PackSub = World->GetSubsystem<UPaldarkHostilePackSubsystem>())
		{
			PackSub->RegisterPalToPack(this, PackTag);
		}
	}

	UE_LOG(LogPaldarkPal, Log,
		TEXT("APaldarkPalCharacter_Stoneclad::BeginPlay — %s team=%s species=%s pack=%s"),
		*GetName(),
		*TeamTag.ToString(),
		*SpeciesTag.ToString(),
		*PackTag.ToString());
}

void APaldarkPalCharacter_Stoneclad::EndPlay(const EEndPlayReason::Type EndPlayReason)
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
