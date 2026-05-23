#include "Pal/Activities/PaldarkBaseActivity.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"

#include "Pal/Components/PaldarkPalActivityComponent.h"
#include "Pal/Components/PaldarkPalLocomotionComponent.h"
#include "Pal/PaldarkPalCharacter.h"
#include "PaldarkLogCategories.h"

UPaldarkBaseActivity::UPaldarkBaseActivity()
{
	// Activity priorities default to 0; concrete subclasses override.
}

bool UPaldarkBaseActivity::InitActivity(UPaldarkPalActivityComponent* InComponent)
{
	if (InComponent == nullptr)
	{
		return false;
	}
	OwningComponent = InComponent;
	PalOwner        = Cast<APaldarkPalCharacter>(InComponent->GetOwner());
	UE_LOG(LogPaldarkPal, Verbose, TEXT("UPaldarkBaseActivity::InitActivity — activity=%s tag=%s pal=%s"),
		*GetName(),
		*ActivityTag.ToString(),
		*GetNameSafe(PalOwner.Get()));
	return true;
}

void UPaldarkBaseActivity::EnterActivity_Implementation()
{
	UE_LOG(LogPaldarkPal, Log, TEXT("UPaldarkBaseActivity::EnterActivity — activity=%s tag=%s pal=%s"),
		*GetClass()->GetName(),
		*ActivityTag.ToString(),
		*GetNameSafe(PalOwner.Get()));
}

void UPaldarkBaseActivity::ExitActivity_Implementation()
{
	UE_LOG(LogPaldarkPal, Log, TEXT("UPaldarkBaseActivity::ExitActivity — activity=%s tag=%s pal=%s"),
		*GetClass()->GetName(),
		*ActivityTag.ToString(),
		*GetNameSafe(PalOwner.Get()));
}

APawn* UPaldarkBaseActivity::GetFollowedPawn() const
{
	const UPaldarkPalLocomotionComponent* Loco = GetLocomotionSlot();
	return Loco != nullptr ? Loco->GetFollowedPawn() : nullptr;
}

UPaldarkPalLocomotionComponent* UPaldarkBaseActivity::GetLocomotionSlot() const
{
	const APaldarkPalCharacter* PalChar = PalOwner.Get();
	return PalChar != nullptr ? PalChar->GetLocomotionSlot() : nullptr;
}

float UPaldarkBaseActivity::GetPlanarDistanceToFollowedPawn() const
{
	const UPaldarkPalLocomotionComponent* Loco = GetLocomotionSlot();
	return Loco != nullptr ? Loco->GetPlanarDistanceToFollowedPawn() : TNumericLimits<float>::Max();
}

UWorld* UPaldarkBaseActivity::GetWorld() const
{
	if (const APaldarkPalCharacter* PalChar = PalOwner.Get())
	{
		return PalChar->GetWorld();
	}
	if (const UPaldarkPalActivityComponent* Component = OwningComponent.Get())
	{
		return Component->GetWorld();
	}
	return nullptr;
}
