#include "Pal/PaldarkPalCharacter_Boltmane.h"

#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Pal/Activities/PaldarkActivity_BossPhase.h"
#include "Pal/Activities/PaldarkActivity_Idle.h"
#include "Pal/Activities/PaldarkActivity_Patrol.h"
#include "Pal/Activities/PaldarkActivity_Stalk.h"
#include "Pal/Components/PaldarkPalActivityComponent.h"
#include "Pal/Components/PaldarkPalCombatComponent.h"
#include "Pal/Components/PaldarkPalPerceptionComponent.h"
#include "Pal/Components/PaldarkPalTameComponent.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"

APaldarkPalCharacter_Boltmane::APaldarkPalCharacter_Boltmane()
{
	TeamTag    = PaldarkGameplayTags::TAG_Paldark_Team_Hostile;
	SpeciesTag = PaldarkGameplayTags::TAG_Paldark_Pal_Species_Boltmane;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = 500.f;
	}

	if (UPaldarkPalPerceptionComponent* Perception = PerceptionSlot)
	{
		Perception->FriendlyTeamTag = PaldarkGameplayTags::TAG_Paldark_Team_Hostile;
		Perception->HostileTeamTag  = PaldarkGameplayTags::TAG_Paldark_Team_Player;
	}

	// Mid-range melee/caster — primary jump is the chain lightning. Higher
	// MinEngageRange keeps the boss at "caster distance" instead of
	// hugging.
	if (UPaldarkPalCombatComponent* Combat = CombatSlot)
	{
		Combat->MinEngageRange = 300.f;
		Combat->MaxEngageRange = 1100.f;
		Combat->AttackInterval = 2.5f;
		Combat->BasePalDamage  = 18.f;
	}

	// Activity ladder — Boss replaces Combat. Boltmane never reverts out
	// of BossPhase once entered, but Stalk still applies during the
	// approach (target out of MinEngageRange).
	if (UPaldarkPalActivityComponent* Activity = ActivitySlot)
	{
		Activity->CandidateActivities = {
			UPaldarkActivity_Idle::StaticClass(),
			UPaldarkActivity_Patrol::StaticClass(),
			UPaldarkActivity_Stalk::StaticClass(),
			UPaldarkActivity_BossPhase::StaticClass(),
		};
	}

	// W35-36 — Boss is technically tameable but `DA_PalDef_Boltmane`
	// authors `TameDifficulty=3.0` + `BaseCaptureProbability=0.10` so the
	// realistic capture rate is sub-1% even at full stun + low HP. Designer
	// can flip `bIsTameable=false` on a per-arena BP subclass if a specific
	// fight should disable tame.
	if (UPaldarkPalTameComponent* Tame = TameSlot)
	{
		Tame->bIsTameable = true;
	}
}

void APaldarkPalCharacter_Boltmane::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	// No pack registration — boss is solo. PackTag intentionally left
	// invalid so any future pack-broadcast logic skips this Pal.

	UE_LOG(LogPaldarkPal, Log,
		TEXT("APaldarkPalCharacter_Boltmane::BeginPlay — %s team=%s species=%s (solo boss)"),
		*GetName(),
		*TeamTag.ToString(),
		*SpeciesTag.ToString());
}

void APaldarkPalCharacter_Boltmane::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}
