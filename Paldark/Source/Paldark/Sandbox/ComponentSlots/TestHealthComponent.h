// Sandbox Lesson 04 — Health slot.
//
// Single responsibility: track HP, expose ApplyDamage, broadcast OnHealthZeroed.
// Does NOT know about Inventory, Ability, UI, sound. Decoupling is enforced
// by simply NOT including any other component header.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TestHealthComponent.generated.h"

// Multicast so any subsystem (Inventory, Ability, UI, achievements, ...) can
// listen without Health.cpp ever learning of their existence.
DECLARE_MULTICAST_DELEGATE(FOnSandboxHealthZeroed);

UCLASS(ClassGroup=(Sandbox), meta=(BlueprintSpawnableComponent))
class PALDARK_API UTestHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTestHealthComponent();

	// SANDBOX: thực tế dùng UAttributeSet + GAS pipeline (xem Lesson 06+).
	// Đây dùng int32 thô để giữ test rõ ràng và observable.
	UPROPERTY()
	int32 Health = 100;

	void ApplyDamage(int32 Amount);

	FOnSandboxHealthZeroed OnHealthZeroed;
};
