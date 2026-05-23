// PALDARK W9-10 — Damage-chain dummy target.
//
// Stand-in actor used to validate the GAS damage pipeline (Fire ability →
// GE_Damage_Standard → DamageExecutionCalculation → Health → OnHealthZeroed
// → destroy). Lives in `Public/Combat/` so the W9-10 combat surface keeps a
// distinct namespace from the W3-4 Pal pawn skeletons.
//
// Shape:
//   - Subclasses ACharacter so it picks up the standard capsule + mesh +
//     character movement (designers can drop the W9-10 mannequin onto it).
//   - Implements IAbilitySystemInterface; ASC + AttributeSet are owned by
//     this actor directly (no PlayerState — it is an unowned world prop).
//   - Replication mode = Minimal (server → simulated proxies only), same as
//     APaldarkPalCharacter. The hitscan ability runs on the server so this
//     is enough for the W9-10 sandbox.
//   - BeginPlay applies the designer-authored InitAttributesEffect to seed
//     Health / MaxHealth / Armor, then binds the AttributeSet's OnHealthZeroed
//     multicast to OnHealthZeroed → destroys the actor after a short delay
//     so the kill is visible on clients before the corpse despawns.
//
// Future homes (do NOT implement here in W9-10):
//   - Ragdoll on death (mesh -> simulate physics) — W17-18 weapon polish.
//   - Respawn timer (re-init attributes instead of destroy) — W19-20 sandbox.
//   - Damage numbers floating text — W22+ HUD polish.

#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "PaldarkDummyTarget.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UPaldarkAbilitySystemComponent;
class UPaldarkAttributeSet;

UCLASS()
class PALDARKLAB_API APaldarkDummyTarget : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	APaldarkDummyTarget();

	// IAbilitySystemInterface — self-owned ASC (no PlayerState).
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UPaldarkAbilitySystemComponent* GetPaldarkAbilitySystemComponent() const { return AbilitySystemComponent; }
	const UPaldarkAttributeSet*     GetPaldarkAttributeSet()          const { return AttributeSet; }

	// W18-19 — Team identity. Default `Paldark.Team.Hostile` (set in the
	// constructor) so the W9-10 dummy now also doubles as the W18-19
	// "hostile" Pal perception target without a separate actor class.
	UFUNCTION(BlueprintPure, Category = "Paldark|Team")
	FGameplayTag GetTeamTag() const { return TeamTag; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Handles OnHealthZeroed (broadcast by the AttributeSet on the authority
	// when an instant damage GE drops Health to 0). Stages the kill — adds
	// Paldark.State.IsDead, disables collision + movement, schedules destroy.
	void HandleHealthZeroed(UAbilitySystemComponent* Instigator);

	// Designer-authored GE applied on BeginPlay to seed Health / MaxHealth /
	// Armor on the authority. When null, the AttributeSet's hard-coded
	// defaults (100/100/0) are used so the dummy still receives damage.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|DummyTarget|Init")
	TSubclassOf<UGameplayEffect> InitAttributesEffect;

	// Seconds between OnHealthZeroed and Destroy(). 1.5s default — long
	// enough for the on-screen death log to be visible without leaving a
	// frozen corpse littering the sandbox.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|DummyTarget|Init")
	float DestroyDelaySeconds = 1.5f;

	UPROPERTY(VisibleAnywhere, Category = "Paldark|GAS")
	TObjectPtr<UPaldarkAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UPaldarkAttributeSet> AttributeSet;

	// W18-19 — Team identity. Default `Paldark.Team.Hostile`; designer can
	// override per-dummy if they want a friendly dummy (e.g. for VFX tests).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paldark|Team")
	FGameplayTag TeamTag;

private:
	// Guard so HandleHealthZeroed only fires the death wiring once even if a
	// follow-up damage GE applies on the same frame.
	bool bIsDead = false;

	FDelegateHandle HealthZeroedHandle;
	FTimerHandle    DestroyTimerHandle;
};
