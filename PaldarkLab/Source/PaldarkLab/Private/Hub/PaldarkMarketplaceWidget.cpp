// PALDARK W44-45 — Marketplace UMG widget implementation.

#include "Hub/PaldarkMarketplaceWidget.h"

#include "Hub/PaldarkMarketplaceController.h"
#include "PaldarkLogCategories.h"

UPaldarkMarketplaceController* UPaldarkMarketplaceWidget::GetMarketplaceController() const
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

	UPaldarkMarketplaceController* Found = PC->FindComponentByClass<UPaldarkMarketplaceController>();
	if (Found)
	{
		CachedController = Found;
	}
	return Found;
}

void UPaldarkMarketplaceWidget::CloseAndDestroy()
{
	if (UPaldarkMarketplaceController* Controller = GetMarketplaceController())
	{
		Controller->RequestCloseMarketplace();
	}
	RemoveFromParent();
}

void UPaldarkMarketplaceWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (UPaldarkMarketplaceController* Controller = GetMarketplaceController())
	{
		Controller->OnMarketplaceOpened.AddDynamic(this, &UPaldarkMarketplaceWidget::HandleMarketplaceOpened);
		Controller->OnMarketplaceClosed.AddDynamic(this, &UPaldarkMarketplaceWidget::HandleMarketplaceClosed);
		Controller->OnMarketplaceActionResult.AddDynamic(this, &UPaldarkMarketplaceWidget::HandleMarketplaceActionResult);
	}
	else
	{
		UE_LOG(LogPaldarkUI, Verbose,
			TEXT("UPaldarkMarketplaceWidget::NativeOnInitialized — marketplace controller not resolvable yet on widget=%s."),
			*GetName());
	}
}

void UPaldarkMarketplaceWidget::NativeDestruct()
{
	if (UPaldarkMarketplaceController* Controller = CachedController.Get())
	{
		Controller->OnMarketplaceOpened.RemoveDynamic(this, &UPaldarkMarketplaceWidget::HandleMarketplaceOpened);
		Controller->OnMarketplaceClosed.RemoveDynamic(this, &UPaldarkMarketplaceWidget::HandleMarketplaceClosed);
		Controller->OnMarketplaceActionResult.RemoveDynamic(this, &UPaldarkMarketplaceWidget::HandleMarketplaceActionResult);
	}
	CachedController.Reset();
	Super::NativeDestruct();
}

void UPaldarkMarketplaceWidget::HandleMarketplaceOpened(AActor* KioskActor)
{
	K2_OnMarketplaceOpened(KioskActor);
}

void UPaldarkMarketplaceWidget::HandleMarketplaceClosed()
{
	K2_OnMarketplaceClosed();
}

void UPaldarkMarketplaceWidget::HandleMarketplaceActionResult(
	FGameplayTag ActionTag,
	FGameplayTag ResultTag,
	FGameplayTag ItemTag,
	int32 CountSettled,
	int32 CreditsRemaining)
{
	K2_OnMarketplaceActionResult(ActionTag, ResultTag, ItemTag, CountSettled, CreditsRemaining);
}
