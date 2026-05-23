// PALDARK W44-45 — Pal Stable UMG widget implementation.

#include "Hub/PaldarkStableWidget.h"

#include "Hub/PaldarkStableController.h"
#include "PaldarkLogCategories.h"
#include "Player/PaldarkPlayerController.h"

UPaldarkStableController* UPaldarkStableWidget::GetStableController() const
{
	if (CachedController.IsValid())
	{
		return CachedController.Get();
	}

	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return nullptr;
	}

	UPaldarkStableController* Found = PC->FindComponentByClass<UPaldarkStableController>();
	if (Found)
	{
		CachedController = Found;
	}
	return Found;
}

void UPaldarkStableWidget::CloseAndDestroy()
{
	if (UPaldarkStableController* Controller = GetStableController())
	{
		Controller->RequestCloseStable();
	}
	RemoveFromParent();
}

void UPaldarkStableWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (UPaldarkStableController* Controller = GetStableController())
	{
		Controller->OnStableOpened.AddDynamic(this, &UPaldarkStableWidget::HandleStableOpened);
		Controller->OnStableClosed.AddDynamic(this, &UPaldarkStableWidget::HandleStableClosed);
		Controller->OnStableActionResult.AddDynamic(this, &UPaldarkStableWidget::HandleStableActionResult);
	}
	else
	{
		UE_LOG(LogPaldarkUI, Verbose,
			TEXT("UPaldarkStableWidget::NativeOnInitialized — stable controller not resolvable yet on widget=%s."),
			*GetName());
	}
}

void UPaldarkStableWidget::NativeDestruct()
{
	if (UPaldarkStableController* Controller = CachedController.Get())
	{
		Controller->OnStableOpened.RemoveDynamic(this, &UPaldarkStableWidget::HandleStableOpened);
		Controller->OnStableClosed.RemoveDynamic(this, &UPaldarkStableWidget::HandleStableClosed);
		Controller->OnStableActionResult.RemoveDynamic(this, &UPaldarkStableWidget::HandleStableActionResult);
	}
	CachedController.Reset();
	Super::NativeDestruct();
}

void UPaldarkStableWidget::HandleStableOpened(AActor* KioskActor)
{
	K2_OnStableOpened(KioskActor);
}

void UPaldarkStableWidget::HandleStableClosed()
{
	K2_OnStableClosed();
}

void UPaldarkStableWidget::HandleStableActionResult(
	FGameplayTag ActionTag,
	FGameplayTag ResultTag,
	int32 IndexHint,
	int32 CreditsRemaining)
{
	K2_OnStableActionResult(ActionTag, ResultTag, IndexHint, CreditsRemaining);
}
