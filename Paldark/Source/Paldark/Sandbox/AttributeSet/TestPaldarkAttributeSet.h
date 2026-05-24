// Sandbox Lesson 06 — UAttributeSet subclass demonstrating the four-attribute
// shape used in Paldark combat: Health/MaxHealth, Stamina/MaxStamina, Armor,
// and the meta-attribute IncomingDamage.
//
// WHY the meta-attribute?  Because GameplayEffects shouldn't touch Health
// directly — they hand a *value* into IncomingDamage, and PostGameplayEffectExecute
// is the single place where damage formula + clamping + death broadcast lives.
// Same hook = same auditable damage path. Lesson 07 will fill the formula
// (armor mitigation) inside PostExecute.
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "TestPaldarkAttributeSet.generated.h"

// User-defined helper. Lyra has the same macro under the same name —
// Epic ships it as a doc-block example in AttributeSet.h, not a real define.
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

DECLARE_MULTICAST_DELEGATE(FOnSandboxHealthZeroed);

UCLASS()
class PALDARK_API UTestPaldarkAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UTestPaldarkAttributeSet();

	// ----- Normal attributes -----
	UPROPERTY(BlueprintReadOnly, Category="Attr")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UTestPaldarkAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category="Attr")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UTestPaldarkAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, Category="Attr")
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UTestPaldarkAttributeSet, Stamina)

	UPROPERTY(BlueprintReadOnly, Category="Attr")
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(UTestPaldarkAttributeSet, MaxStamina)

	UPROPERTY(BlueprintReadOnly, Category="Attr")
	FGameplayAttributeData Armor;
	ATTRIBUTE_ACCESSORS(UTestPaldarkAttributeSet, Armor)

	// ----- Meta-attribute -----
	// IncomingDamage is a write-only "mailbox". GEs deposit a damage value here,
	// PostGameplayEffectExecute translates the deposit into a Health change and
	// resets the mailbox to 0. Health itself is NEVER touched directly by GEs.
	UPROPERTY(BlueprintReadOnly, Category="Attr")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UTestPaldarkAttributeSet, IncomingDamage)

	// Hooks
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	// Fires when Health transitions from >0 to 0 inside PostGameplayEffectExecute.
	FOnSandboxHealthZeroed OnHealthZeroed;
};
