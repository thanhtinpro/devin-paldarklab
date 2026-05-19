#include "Pal/Activities/PaldarkActivity_Stalk.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Pal/Components/PaldarkPalCombatComponent.h"
#include "Pal/Components/PaldarkPalLocomotionComponent.h"
#include "Pal/Components/PaldarkPalPerceptionComponent.h"
#include "Pal/PaldarkPalCharacter.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"

UPaldarkActivity_Stalk::UPaldarkActivity_Stalk()
{
	ActivityTag = PaldarkGameplayTags::TAG_Paldark_Pal_Activity_Stalk;
	// Priority 25 — between Follow (20) and Investigate (30). For hostile
	// Pals (no Follow/Investigate), Stalk preempts Patrol (15) when a threat
	// is visible but still out of MinEngageRange.
	Priority    = 25;
}

bool UPaldarkActivity_Stalk::InitActivity(UPaldarkPalActivityComponent* InComponent)
{
	if (!Super::InitActivity(InComponent))
	{
		return false;
	}

	if (APaldarkPalCharacter* Pal = GetPalOwner())
	{
		CombatRef     = Pal->FindComponentByClass<UPaldarkPalCombatComponent>();
		PerceptionRef = Pal->FindComponentByClass<UPaldarkPalPerceptionComponent>();
	}

	if (!CombatRef.IsValid() || !PerceptionRef.IsValid())
	{
		UE_LOG(LogPaldarkPal, Verbose,
			TEXT("UPaldarkActivity_Stalk::InitActivity — pal=%s missing combat/perception (combat=%d perception=%d); activity will never run."),
			*GetNameSafe(GetPalOwner()),
			CombatRef.IsValid() ? 1 : 0,
			PerceptionRef.IsValid() ? 1 : 0);
	}
	return true;
}

bool UPaldarkActivity_Stalk::CanRun_Implementation() const
{
	const UPaldarkPalCombatComponent*     Combat     = CombatRef.Get();
	const UPaldarkPalPerceptionComponent* Perception = PerceptionRef.Get();
	const APaldarkPalCharacter*           Pal        = GetPalOwner();
	if (Combat == nullptr || Perception == nullptr || Pal == nullptr)
	{
		return false;
	}

	const AActor* Target = Perception->GetCurrentThreat();
	if (Target == nullptr || !IsValid(Target) || Target->IsActorBeingDestroyed())
	{
		return false;
	}

	const float Distance = FVector::Dist(Target->GetActorLocation(), Pal->GetActorLocation());
	// Only stalk when the target is visible but out of attack range. Combat
	// (Priority 40) preempts via its own CanRun once inside MinEngageRange.
	return (Distance > Combat->MinEngageRange) && (Distance <= LoseSightDistance);
}

bool UPaldarkActivity_Stalk::ShouldContinue_Implementation() const
{
	return CanRun();
}

void UPaldarkActivity_Stalk::EnterActivity_Implementation()
{
	Super::EnterActivity_Implementation();

	// Pause the (defensive) follow leash and ramp speed down to stalk pace.
	if (UPaldarkPalLocomotionComponent* Loco = GetLocomotionSlot())
	{
		Loco->SetFollowEnabled(false);
	}

	if (APaldarkPalCharacter* Pal = GetPalOwner())
	{
		if (UCharacterMovementComponent* Movement = Pal->GetCharacterMovement())
		{
			SavedMaxWalkSpeed = Movement->MaxWalkSpeed;
			Movement->MaxWalkSpeed = SavedMaxWalkSpeed * StalkSpeedScale;
			UE_LOG(LogPaldarkPal, Log,
				TEXT("UPaldarkActivity_Stalk::EnterActivity — pal=%s saved_speed=%.1f stalk_speed=%.1f"),
				*Pal->GetName(),
				SavedMaxWalkSpeed,
				Movement->MaxWalkSpeed);
		}
	}
}

void UPaldarkActivity_Stalk::TickActivity_Implementation(float /*DeltaSeconds*/)
{
	UPaldarkPalPerceptionComponent* Perception = PerceptionRef.Get();
	APaldarkPalCharacter*           Pal        = GetPalOwner();
	if (Perception == nullptr || Pal == nullptr)
	{
		return;
	}

	AActor* Target = Perception->GetCurrentThreat();
	if (Target == nullptr || !IsValid(Target))
	{
		return;
	}

	FVector Delta = Target->GetActorLocation() - Pal->GetActorLocation();
	Delta.Z = 0.f;
	const float Distance = Delta.Size();
	if (Distance < KINDA_SMALL_NUMBER)
	{
		return;
	}

	Pal->AddMovementInput(Delta.GetSafeNormal(), 1.f);
}

void UPaldarkActivity_Stalk::ExitActivity_Implementation()
{
	if (APaldarkPalCharacter* Pal = GetPalOwner())
	{
		if (UCharacterMovementComponent* Movement = Pal->GetCharacterMovement())
		{
			// Restore the species default. If SavedMaxWalkSpeed is 0 (Stalk
			// never properly entered), leave the current value alone.
			if (SavedMaxWalkSpeed > 0.f)
			{
				Movement->MaxWalkSpeed = SavedMaxWalkSpeed;
				UE_LOG(LogPaldarkPal, Log,
					TEXT("UPaldarkActivity_Stalk::ExitActivity — pal=%s restored_speed=%.1f"),
					*Pal->GetName(),
					Movement->MaxWalkSpeed);
			}
		}
	}

	if (UPaldarkPalLocomotionComponent* Loco = GetLocomotionSlot())
	{
		Loco->SetFollowEnabled(true);
	}

	Super::ExitActivity_Implementation();
}
