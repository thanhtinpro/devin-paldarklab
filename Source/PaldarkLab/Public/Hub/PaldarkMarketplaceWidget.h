// PALDARK W44-45 — Marketplace UMG widget base class.
//
// Mirror of `UPaldarkStableWidget` for the marketplace kiosk. Designer
// authors `WBP_PaldarkMarketplaceWidget` (List View of items + Buy/Sell
// buttons + credits balance label) in UMG; the C++ side wires the
// controller's delegates into BlueprintImplementableEvent hooks.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "PaldarkMarketplaceWidget.generated.h"

class APaldarkMarketplaceKiosk;
class UPaldarkMarketplaceController;

UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisplayName = "Paldark Marketplace Widget"))
class PALDARKLAB_API UPaldarkMarketplaceWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Paldark|Hub|Marketplace")
	UPaldarkMarketplaceController* GetMarketplaceController() const;

	UFUNCTION(BlueprintCallable, Category = "Paldark|Hub|Marketplace")
	void CloseAndDestroy();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Paldark|Hub|Marketplace",
		meta = (DisplayName = "On Marketplace Opened"))
	void K2_OnMarketplaceOpened(AActor* KioskActor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Paldark|Hub|Marketplace",
		meta = (DisplayName = "On Marketplace Closed"))
	void K2_OnMarketplaceClosed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Paldark|Hub|Marketplace",
		meta = (DisplayName = "On Marketplace Action Result"))
	void K2_OnMarketplaceActionResult(
		FGameplayTag ActionTag,
		FGameplayTag ResultTag,
		FGameplayTag ItemTag,
		int32 CountSettled,
		int32 CreditsRemaining);

private:
	UFUNCTION()
	void HandleMarketplaceOpened(AActor* KioskActor);

	UFUNCTION()
	void HandleMarketplaceClosed();

	UFUNCTION()
	void HandleMarketplaceActionResult(
		FGameplayTag ActionTag,
		FGameplayTag ResultTag,
		FGameplayTag ItemTag,
		int32 CountSettled,
		int32 CreditsRemaining);

	UPROPERTY(Transient)
	mutable TWeakObjectPtr<UPaldarkMarketplaceController> CachedController;
};
