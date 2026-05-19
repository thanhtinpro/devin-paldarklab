// PALDARK W7-8 + W9-10 — Attribute set.
//
// W7-8 baseline: Health / MaxHealth / Stamina / MaxStamina / MoveSpeed.
// W9-10 additions: Armor (replicated, defensive), IncomingDamage (meta —
// transient, not replicated, used by the DamageExecutionCalculation), and
// the FOnHealthZeroed delegate fired by PostGameplayEffectExecute when an
// instant damage GE drops Health to zero.
//
// Vendor-neutral mirror of the standard GAS AttributeSet pattern (see
// `11.Udemy-ue5-gas-top-down-rpg` and `13.Udemy-ue5-multiplayer-in-unreal-with-gas-and-aws-dedicated-servers`
// for canonical implementations). Owns the replicated FGameplayAttributeData
// values plus the boilerplate ATTRIBUTE_ACCESSORS macro expansions so designers
// can query Health/Armor/MoveSpeed via `GetHealthAttribute()` etc. from Blueprint.
//
// Roadmap reference: Documents/PALDARK/03-Roadmap_1_Year.md § Tuần 7–8, 9–10.
//
// Replication strategy (matches RoN / Aura):
//   - Each persistent attribute is RepNotify with `OnRep_<Name>(OldValue)`
//     forwarding to `GAMEPLAYATTRIBUTE_REPNOTIFY` so observers fire correctly.
//   - PreAttributeChange clamps Health/Stamina to [0, Max*] and floors Armor
//     at 0, so a stacked debuff can't drive defensive stats negative.
//   - PostGameplayEffectExecute consumes IncomingDamage → Health on the
//     authority side, re-clamps, and broadcasts OnHealthZeroed when Health
//     hits 0 so APaldarkDummyTarget (and future death wiring) can react.
//
// W35-36 additions: Stun + MaxStun (per-Pal stun bar feeding the capture
// minigame). Built up by every Pal attack hit (designer authors `GE_StunOnHit`
// in follow-up branch) and decays per-tick; `UPaldarkPalTameComponent` reads
// `Stun / MaxStun` ratio as one of the capture-probability inputs. Bond level
// itself is NOT an attribute — it lives on `UPaldarkPalBondComponent` because
// it's a discrete int (0..20), not a continuous float that benefits from GAS
// modifier composition.
//
// Future homes (do NOT implement here in W9-10):
//   - Hunger / Thirst attribute (Pal survival sim) — W34+ tame mechanic.
//   - Resistance / element-specific damage modifiers — W21+ combat polish.

#pragma once

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "CoreMinimal.h"
#include "Delegates/DelegateCombinations.h"
#include "PaldarkAttributeSet.generated.h"

// W9-10 — Broadcast on the authority when an instant damage GE drives Health
// to zero. APaldarkDummyTarget binds this in BeginPlay to ragdoll/destroy on
// death. The single param is the source ASC (the killer) so listeners can do
// attribution / xp award later. Non-dynamic: GAS attribute lifetimes don't
// play well with UFUNCTION-bound dynamic delegates (the AttributeSet outlives
// the dying actor's UFunction context).
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPaldarkHealthZeroed, UAbilitySystemComponent* /*Instigator*/);

// Standard GAS accessor macro (Health → GetHealth / SetHealth / InitHealth /
// GetHealthAttribute). Defined here so the .h reads cleanly; UE engine's
// `AbilitySystemComponent.h` provides the underlying GAMEPLAYATTRIBUTE_*
// helpers.
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class PALDARKLAB_API UPaldarkAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UPaldarkAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// PreAttributeChange — clamps Health to [0, MaxHealth] and Stamina to
	// [0, MaxStamina] before the change hits. Pure GAS rule: do NOT call
	// `SetHealth`/`SetStamina` here, only mutate the in-out NewValue.
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	// PostGameplayEffectExecute — runs after an instant/periodic GE modified
	// an attribute. W7-8 only re-clamps + logs; W9-10's damage execution will
	// add the "health hit 0 → broadcast death" path here.
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	// Health — current HP. Drains on damage, regenerates passively via a
	// dedicated GE (out of scope for W7-8; the AttributeSet just exposes the
	// attribute slot so a future GE can drive it).
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Paldark|GAS|Vital")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UPaldarkAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Paldark|GAS|Vital")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UPaldarkAttributeSet, MaxHealth)

	// Stamina — drains while Sprint ability is active (GE_Sprint_Cost), regen
	// when the ability ends (GE_Stamina_Regen). The GA_Sprint logic lives in
	// the Sprint ability, not in this AttributeSet — keeping data and policy
	// separate is the standard GAS pattern.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Stamina, Category = "Paldark|GAS|Vital")
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UPaldarkAttributeSet, Stamina)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxStamina, Category = "Paldark|GAS|Vital")
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(UPaldarkAttributeSet, MaxStamina)

	// MoveSpeed — current movement speed in cm/s. Read by
	// `UCharacterMovementComponent::MaxWalkSpeed` via a binding in the
	// character (see `APaldarkCharacter::HandleMoveSpeedChange`). Sprint
	// ability applies an additive GE on activate to bump this from
	// `BaseWalkSpeed` to `BaseSprintSpeed`, and removes it on Cancel/End.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MoveSpeed, Category = "Paldark|GAS|Locomotion")
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(UPaldarkAttributeSet, MoveSpeed)

	// W9-10 — Armor reduces incoming damage. Read by the damage execution
	// calculation via attribute capture (Target). Replicated so HUD / debug
	// can read it on clients. Designers tune via the InitAttributes GE.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Armor, Category = "Paldark|GAS|Defense")
	FGameplayAttributeData Armor;
	ATTRIBUTE_ACCESSORS(UPaldarkAttributeSet, Armor)

	// W9-10 — IncomingDamage is a meta-attribute (not replicated). The damage
	// execution calc writes the final damage value here; PostGameplayEffectExecute
	// then transfers it to Health on the authority side and zeroes the slot.
	// Storing damage as an attribute (instead of subtracting Health directly in
	// the execution) keeps the damage formula auditable via the standard
	// "GE → ExecutionCalc → AttributeSet" GAS pipeline.
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|GAS|Damage")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UPaldarkAttributeSet, IncomingDamage)

	// W9-10 — Multicast delegate fired on the authority from PostGameplayEffectExecute
	// when Health drops to 0 via an instant damage modifier. APaldarkDummyTarget
	// binds this in BeginPlay (server-only) to drive ragdoll / destroy. The
	// param is the source ASC (the killer) for attribution.
	FOnPaldarkHealthZeroed OnHealthZeroed;

	// W35-36 — Stun bar. Built up by every Pal attack hit (designer authors
	// `GE_StunOnHit` later: Period 0 instant on hit, magnitude =
	// `IncomingDamage` * `Paldark.SetByCaller.PalDamage` * 0.5). Drained per
	// tick by `GE_StunDecay` (Period 0.25 s, magnitude -5/sec). Read by
	// `UPaldarkPalTameComponent::BeginTameAttempt` as the `Stun/MaxStun`
	// ratio — higher stun ⇒ higher capture probability. Player-side companions
	// keep Stun at 0 — only hostile Pals expose the tame component.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Stun, Category = "Paldark|GAS|Status")
	FGameplayAttributeData Stun;
	ATTRIBUTE_ACCESSORS(UPaldarkAttributeSet, Stun)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxStun, Category = "Paldark|GAS|Status")
	FGameplayAttributeData MaxStun;
	ATTRIBUTE_ACCESSORS(UPaldarkAttributeSet, MaxStun)

protected:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MoveSpeed(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Armor(const FGameplayAttributeData& OldValue);

	// W35-36 — Stun / MaxStun replication callbacks.
	UFUNCTION()
	void OnRep_Stun(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxStun(const FGameplayAttributeData& OldValue);
};
