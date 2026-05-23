#include "Pal/PaldarkPalCharacter_Vinewraith.h"

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

APaldarkPalCharacter_Vinewraith::APaldarkPalCharacter_Vinewraith()
{
	TeamTag    = PaldarkGameplayTags::TAG_Paldark_Team_Hostile;
	SpeciesTag = PaldarkGameplayTags::TAG_Paldark_Pal_Species_Vinewraith;

	// Fast enough to maintain spacing.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = 520.f;
	}

	if (UPaldarkPalPerceptionComponent* Perception = PerceptionSlot)
	{
		Perception->FriendlyTeamTag = PaldarkGameplayTags::TAG_Paldark_Team_Hostile;
		Perception->HostileTeamTag  = PaldarkGameplayTags::TAG_Paldark_Team_Player;
	}

	// Ranged tuning — matches the VineWhip ability's preferred reach.
	if (UPaldarkPalCombatComponent* Combat = CombatSlot)
	{
		Combat->MinEngageRange = 400.f;
		Combat->MaxEngageRange = 900.f;
		Combat->AttackInterval = 1.5f;
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

void APaldarkPalCharacter_Vinewraith::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	if (!PackTag.IsValid())
	{
		PackTag = PaldarkGameplayTags::TAG_Paldark_Pal_Pack_Vinewraith;
	}

	if (UWorld* World = GetWorld())
	{
		if (UPaldarkHostilePackSubsystem* PackSub = World->GetSubsystem<UPaldarkHostilePackSubsystem>())
		{
			PackSub->RegisterPalToPack(this, PackTag);
		}
	}

	UE_LOG(LogPaldarkPal, Log,
		TEXT("APaldarkPalCharacter_Vinewraith::BeginPlay — %s team=%s species=%s pack=%s"),
		*GetName(),
		*TeamTag.ToString(),
		*SpeciesTag.ToString(),
		*PackTag.ToString());
}

void APaldarkPalCharacter_Vinewraith::EndPlay(const EEndPlayReason::Type EndPlayReason)
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
