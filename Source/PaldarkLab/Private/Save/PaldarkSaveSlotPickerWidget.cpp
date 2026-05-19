// PALDARK W48 — Save slot picker UMG stub implementation.

#include "Save/PaldarkSaveSlotPickerWidget.h"

#include "PaldarkLab.h"
#include "PaldarkGameplayTags.h"
#include "Save/PaldarkSaveSubsystem.h"
#include "Player/PaldarkPlayerController.h"

#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"

UPaldarkSaveSlotPickerWidget::UPaldarkSaveSlotPickerWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UPaldarkSaveSlotPickerWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BoundSubsystem = GetSaveSubsystem();
	if (BoundSubsystem != nullptr)
	{
		BoundSubsystem->OnSaveCompleted.AddDynamic(this, &UPaldarkSaveSlotPickerWidget::HandleSaveCompleted);
		BoundSubsystem->OnLoadCompleted.AddDynamic(this, &UPaldarkSaveSlotPickerWidget::HandleLoadCompleted);
		UE_LOG(LogPaldarkLab, Verbose,
			TEXT("[Save][W48][Picker] bound to UPaldarkSaveSubsystem"));
	}
	else
	{
		UE_LOG(LogPaldarkLab, Warning,
			TEXT("[Save][W48][Picker] no UPaldarkSaveSubsystem on owning GameInstance — picker idle"));
	}
}

void UPaldarkSaveSlotPickerWidget::NativeDestruct()
{
	if (BoundSubsystem != nullptr)
	{
		BoundSubsystem->OnSaveCompleted.RemoveDynamic(this, &UPaldarkSaveSlotPickerWidget::HandleSaveCompleted);
		BoundSubsystem->OnLoadCompleted.RemoveDynamic(this, &UPaldarkSaveSlotPickerWidget::HandleLoadCompleted);
	}
	BoundSubsystem = nullptr;
	Super::NativeDestruct();
}

UPaldarkSaveSubsystem* UPaldarkSaveSlotPickerWidget::GetSaveSubsystem() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI != nullptr ? GI->GetSubsystem<UPaldarkSaveSubsystem>() : nullptr;
}

APaldarkPlayerController* UPaldarkSaveSlotPickerWidget::GetOwningPaldarkPC() const
{
	APlayerController* PC = GetOwningPlayer();
	return Cast<APaldarkPlayerController>(PC);
}

bool UPaldarkSaveSlotPickerWidget::RequestSave(FName SlotName)
{
	UPaldarkSaveSubsystem* Sub = GetSaveSubsystem();
	APaldarkPlayerController* PC = GetOwningPaldarkPC();
	if (Sub == nullptr || PC == nullptr)
	{
		UE_LOG(LogPaldarkLab, Warning,
			TEXT("[Save][W48][Picker] RequestSave rejected: subsystem=%s PC=%s"),
			Sub != nullptr ? TEXT("ok") : TEXT("null"),
			PC != nullptr ? TEXT("ok") : TEXT("null"));
		return false;
	}
	return Sub->RequestSaveSlot(PC,
		SlotName,
		PaldarkGameplayTags::TAG_Paldark_Save_Trigger_Manual);
}

bool UPaldarkSaveSlotPickerWidget::RequestLoad(FName SlotName)
{
	UPaldarkSaveSubsystem* Sub = GetSaveSubsystem();
	APaldarkPlayerController* PC = GetOwningPaldarkPC();
	if (Sub == nullptr || PC == nullptr)
	{
		UE_LOG(LogPaldarkLab, Warning,
			TEXT("[Save][W48][Picker] RequestLoad rejected: subsystem=%s PC=%s"),
			Sub != nullptr ? TEXT("ok") : TEXT("null"),
			PC != nullptr ? TEXT("ok") : TEXT("null"));
		return false;
	}
	return Sub->RequestLoadSlot(PC, SlotName);
}

bool UPaldarkSaveSlotPickerWidget::RequestDump(FName SlotName)
{
	UPaldarkSaveSubsystem* Sub = GetSaveSubsystem();
	if (Sub == nullptr)
	{
		return false;
	}
	return Sub->RequestDumpSlot(SlotName);
}

bool UPaldarkSaveSlotPickerWidget::RequestClear(FName SlotName)
{
	UPaldarkSaveSubsystem* Sub = GetSaveSubsystem();
	if (Sub == nullptr)
	{
		return false;
	}
	return Sub->ClearSlot(SlotName);
}

int32 UPaldarkSaveSlotPickerWidget::RequestList()
{
	UPaldarkSaveSubsystem* Sub = GetSaveSubsystem();
	if (Sub == nullptr)
	{
		return 0;
	}
	return Sub->ListSlotsToLog();
}

void UPaldarkSaveSlotPickerWidget::HandleSaveCompleted(
	FName SlotName,
	FGameplayTag ResultTag,
	FGameplayTag TriggerTag)
{
	UE_LOG(LogPaldarkLab, Verbose,
		TEXT("[Save][W48][Picker] HandleSaveCompleted slot=%s result=%s trigger=%s"),
		*SlotName.ToString(),
		*ResultTag.ToString(),
		*TriggerTag.ToString());
	K2_OnSaveCompleted(SlotName, ResultTag, TriggerTag);
}

void UPaldarkSaveSlotPickerWidget::HandleLoadCompleted(
	FName SlotName,
	FGameplayTag ResultTag)
{
	UE_LOG(LogPaldarkLab, Verbose,
		TEXT("[Save][W48][Picker] HandleLoadCompleted slot=%s result=%s"),
		*SlotName.ToString(),
		*ResultTag.ToString());
	K2_OnLoadCompleted(SlotName, ResultTag);
}
