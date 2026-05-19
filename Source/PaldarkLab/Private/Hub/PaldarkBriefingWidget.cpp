// PALDARK W46 — Briefing room UMG widget stub implementation.

#include "Hub/PaldarkBriefingWidget.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Hub/PaldarkBriefingController.h"
#include "Hub/PaldarkBriefingSessionComponent.h"
#include "PaldarkLogCategories.h"
#include "Player/PaldarkPlayerController.h"

UPaldarkBriefingWidget::UPaldarkBriefingWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UPaldarkBriefingController* UPaldarkBriefingWidget::GetBriefingController() const
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (APaldarkPlayerController* PaldarkPC = Cast<APaldarkPlayerController>(PC))
		{
			return PaldarkPC->FindComponentByClass<UPaldarkBriefingController>();
		}
	}
	return nullptr;
}

UPaldarkBriefingSessionComponent* UPaldarkBriefingWidget::GetSessionComponent() const
{
	return UPaldarkBriefingSessionComponent::Get(GetWorld());
}

void UPaldarkBriefingWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BoundController = GetBriefingController();
	if (BoundController != nullptr)
	{
		BoundController->OnBriefingOpened.AddDynamic(this, &UPaldarkBriefingWidget::HandleBriefingOpened);
		BoundController->OnBriefingClosed.AddDynamic(this, &UPaldarkBriefingWidget::HandleBriefingClosed);
		BoundController->OnBriefingActionResult.AddDynamic(this, &UPaldarkBriefingWidget::HandleBriefingActionResult);
	}
	else
	{
		UE_LOG(LogPaldarkUI, Warning,
			TEXT("[Hub.Brief.Widget] NativeOnInitialized — no UPaldarkBriefingController on owning PC; widget binds will no-op."));
	}

	BoundSession = GetSessionComponent();
	if (BoundSession != nullptr)
	{
		BoundSession->OnSessionChanged.AddDynamic(this, &UPaldarkBriefingWidget::HandleSessionChanged);
	}
}

void UPaldarkBriefingWidget::NativeDestruct()
{
	if (BoundController != nullptr)
	{
		BoundController->OnBriefingOpened.RemoveDynamic(this, &UPaldarkBriefingWidget::HandleBriefingOpened);
		BoundController->OnBriefingClosed.RemoveDynamic(this, &UPaldarkBriefingWidget::HandleBriefingClosed);
		BoundController->OnBriefingActionResult.RemoveDynamic(this, &UPaldarkBriefingWidget::HandleBriefingActionResult);
		BoundController = nullptr;
	}
	if (BoundSession != nullptr)
	{
		BoundSession->OnSessionChanged.RemoveDynamic(this, &UPaldarkBriefingWidget::HandleSessionChanged);
		BoundSession = nullptr;
	}
	Super::NativeDestruct();
}

void UPaldarkBriefingWidget::HandleBriefingOpened(AActor* BriefingRoomActor)
{
	K2_OnBriefingOpened(BriefingRoomActor);
}

void UPaldarkBriefingWidget::HandleBriefingClosed()
{
	K2_OnBriefingClosed();
}

void UPaldarkBriefingWidget::HandleBriefingActionResult(
	FGameplayTag ActionTag,
	FGameplayTag ResultTag,
	FGameplayTag MapTag,
	int32 CountdownRemaining)
{
	K2_OnBriefingActionResult(ActionTag, ResultTag, MapTag, CountdownRemaining);
}

void UPaldarkBriefingWidget::HandleSessionChanged()
{
	K2_OnSessionChanged();
}
