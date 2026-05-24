#include "TestPaldarkAttributeSet.h"

#include "SandboxAttrLog.h"
#include "GameplayEffectExtension.h"

UTestPaldarkAttributeSet::UTestPaldarkAttributeSet()
{
	// SANDBOX: production initializes from a curve table or DataAsset per pawn level.
	InitHealth(100.f);
	InitMaxHealth(100.f);
	InitStamina(100.f);
	InitMaxStamina(100.f);
	InitArmor(50.f);
	InitIncomingDamage(0.f);
}

// ---------------------------------------------------------------------------
// PreAttributeChange runs for EVERY write path:
//   * UAbilitySystemComponent::ApplyModToAttribute / SetNumericAttributeBase
//   * UGameplayEffect modifier commit
//   * Direct SetX accessor
// Clamping here = single source of truth for "Health is in [0, MaxHealth]".
// ---------------------------------------------------------------------------
void UTestPaldarkAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		const float Clamped = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
		if (!FMath::IsNearlyEqual(Clamped, NewValue))
		{
			UE_LOG(LogSandboxAttr, Display, TEXT("PreAttributeChange(Health): requested %.2f clamped to %.2f [0, %.2f]"),
				NewValue, Clamped, GetMaxHealth());
		}
		NewValue = Clamped;
	}
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
	}
}

// ---------------------------------------------------------------------------
// PostGameplayEffectExecute runs after a GE's modifier was committed. This is
// the chokepoint where IncomingDamage (meta) gets translated into a Health
// reduction. Same hook would later host: armor mitigation, ShieldOverHealth,
// damage-type resists, death broadcast.
// ---------------------------------------------------------------------------
void UTestPaldarkAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const FGameplayAttribute ModifiedAttr = Data.EvaluatedData.Attribute;

	if (ModifiedAttr == GetIncomingDamageAttribute())
	{
		const float Damage = GetIncomingDamage();
		SetIncomingDamage(0.f); // reset mailbox immediately so a second pass sees nothing

		if (Damage > 0.f)
		{
			const float HealthBefore = GetHealth();
			SetHealth(HealthBefore - Damage); // PreAttributeChange clamps to [0, MaxHealth]
			const float HealthAfter  = GetHealth();

			UE_LOG(LogSandboxAttr, Display,
				TEXT("PostGameplayEffectExecute: IncomingDamage=%.2f translated -> Health %.2f -> %.2f"),
				Damage, HealthBefore, HealthAfter);

			if (HealthBefore > 0.f && HealthAfter <= 0.f)
			{
				UE_LOG(LogSandboxAttr, Display, TEXT("PostGameplayEffectExecute: Health hit zero -> broadcasting OnHealthZeroed"));
				OnHealthZeroed.Broadcast();
			}
		}
	}
}
