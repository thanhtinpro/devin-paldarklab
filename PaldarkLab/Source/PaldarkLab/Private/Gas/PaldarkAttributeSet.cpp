#include "Gas/PaldarkAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

#include "PaldarkLogCategories.h"

UPaldarkAttributeSet::UPaldarkAttributeSet()
{
	// Sensible defaults so a Pal / player spawned without an init GE still
	// has live attribute values. The InitAttributes GE (designer-authored,
	// authored after compile) overrides these on the real spawn path.
	InitHealth(100.0f);
	InitMaxHealth(100.0f);
	InitStamina(100.0f);
	InitMaxStamina(100.0f);
	InitMoveSpeed(600.0f);

	// W9-10 defaults — Armor 0 (no mitigation) and IncomingDamage 0 (the
	// meta-attribute is filled by the DamageExecutionCalculation at execute
	// time, then consumed by PostGameplayEffectExecute on the authority).
	InitArmor(0.0f);
	InitIncomingDamage(0.0f);

	// W35-36 defaults — Stun bar empty, MaxStun 100. Designer's
	// `GE_InitStun_<Species>` overrides MaxStun on Pal spawn per species
	// (Direhound 100, Razorbird 80, Stoneclad 200, Boltmane 400). Player
	// characters keep these defaults and never accrue Stun in practice —
	// the stun-on-hit GE only fires from Pal attacks targeting other Pals.
	InitStun(0.0f);
	InitMaxStun(100.0f);
}

void UPaldarkAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Standard GAS replication knobs — `REPNOTIFY_Always` so OnRep fires even
	// when the new value equals the cached client-side prediction (matters for
	// fast attribute oscillations like Stamina during Sprint).
	DOREPLIFETIME_CONDITION_NOTIFY(UPaldarkAttributeSet, Health,     COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPaldarkAttributeSet, MaxHealth,  COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPaldarkAttributeSet, Stamina,    COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPaldarkAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPaldarkAttributeSet, MoveSpeed,  COND_None, REPNOTIFY_Always);

	// W9-10 — Armor is replicated like the other persistent stats. IncomingDamage
	// is intentionally NOT replicated — it's a server-side meta value consumed
	// inside PostGameplayEffectExecute on the same frame, before the next
	// replication tick can pick up a transient non-zero value.
	DOREPLIFETIME_CONDITION_NOTIFY(UPaldarkAttributeSet, Armor,      COND_None, REPNOTIFY_Always);

	// W35-36 — Stun + MaxStun are replicated like the other persistent
	// stats. Clients need both values so a HUD widget can render the stun
	// bar fill ratio without a separate "Stun%" attribute.
	DOREPLIFETIME_CONDITION_NOTIFY(UPaldarkAttributeSet, Stun,       COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPaldarkAttributeSet, MaxStun,    COND_None, REPNOTIFY_Always);
}

void UPaldarkAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
	}
	else if (Attribute == GetMaxHealthAttribute() || Attribute == GetMaxStaminaAttribute())
	{
		// Allow designers to scale MaxHealth/MaxStamina up via GE without
		// dragging Health/Stamina along — but never below 1 so a broken init
		// GE can't softlock the player.
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	else if (Attribute == GetMoveSpeedAttribute())
	{
		// Floor at 0 so a stacked debuff can't drive MoveSpeed negative.
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetArmorAttribute())
	{
		// W9-10 — Armor must be non-negative; a negative armor would invert
		// the damage formula (`1 / (1 + Armor/100)` blows up below -100).
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetStunAttribute())
	{
		// W35-36 — Stun clamps to [0, MaxStun] so a stacked stun-on-hit GE
		// can't push the bar past full or a tame-fail penalty can't drive
		// it negative.
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStun());
	}
	else if (Attribute == GetMaxStunAttribute())
	{
		// MaxStun must be >= 1 so per-species init GEs can't accidentally
		// zero out the cap (which would divide-by-zero the tame formula).
		NewValue = FMath::Max(NewValue, 1.0f);
	}
}

void UPaldarkAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// Re-clamp Health/Stamina AFTER the modifier so a periodic regen GE that
	// over-shoots MaxHealth/MaxStamina settles cleanly.
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		SetStamina(FMath::Clamp(GetStamina(), 0.0f, GetMaxStamina()));
	}
	else if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		// W9-10 — Consume IncomingDamage on the authority. The damage execution
		// calc has already written the final mitigated damage into IncomingDamage
		// (with armor + headshot multipliers applied) so we just move it onto
		// Health here. Zero the meta-attribute so the next damage application
		// starts from a clean slate.
		const float LocalDamage = GetIncomingDamage();
		SetIncomingDamage(0.0f);

		if (LocalDamage > 0.0f)
		{
			const float OldHealth = GetHealth();
			const float NewHealth = FMath::Clamp(OldHealth - LocalDamage, 0.0f, GetMaxHealth());
			SetHealth(NewHealth);

			UE_LOG(LogPaldarkGAS, Log,
				TEXT("UPaldarkAttributeSet::PostGameplayEffectExecute — IncomingDamage=%.3f Health=%.3f -> %.3f"),
				LocalDamage, OldHealth, NewHealth);

			// W9-10 — Broadcast death when Health crosses zero. Single-shot:
			// only fire when we transition from >0 to <=0, so subsequent damage
			// hits on a corpse don't re-trigger the death wiring.
			if (NewHealth <= 0.0f && OldHealth > 0.0f)
			{
				UAbilitySystemComponent* SourceASC = Data.EffectSpec.GetContext().GetInstigatorAbilitySystemComponent();
				OnHealthZeroed.Broadcast(SourceASC);
			}
		}
	}

	UE_LOG(LogPaldarkGAS, Verbose,
		TEXT("UPaldarkAttributeSet::PostGameplayEffectExecute — attr=%s magnitude=%.3f new=%.3f"),
		*Data.EvaluatedData.Attribute.GetName(),
		Data.EvaluatedData.Magnitude,
		Data.EvaluatedData.Magnitude);
}

void UPaldarkAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPaldarkAttributeSet, Health, OldValue);
}

void UPaldarkAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPaldarkAttributeSet, MaxHealth, OldValue);
}

void UPaldarkAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPaldarkAttributeSet, Stamina, OldValue);
}

void UPaldarkAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPaldarkAttributeSet, MaxStamina, OldValue);
}

void UPaldarkAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPaldarkAttributeSet, MoveSpeed, OldValue);
}

void UPaldarkAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPaldarkAttributeSet, Armor, OldValue);
}

void UPaldarkAttributeSet::OnRep_Stun(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPaldarkAttributeSet, Stun, OldValue);
}

void UPaldarkAttributeSet::OnRep_MaxStun(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPaldarkAttributeSet, MaxStun, OldValue);
}
