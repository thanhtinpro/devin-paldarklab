// PALDARK W18-19 — Pal combat component (W3-4 stub → real shape).
//
// Owns one Pal attack ability + the cooldown / target-pointer state the
// Combat activity reads each tick. Server-only — clients see replicated
// character movement only, the combat decision making lives here on
// authority.
//
// Responsibilities:
//   - On BeginPlay (authority), grant `AttackAbilityClass` to the owning
//     Pal's ASC via `GiveAbility(FGameplayAbilitySpec(...))`. Caches the
//     spec handle for later `TryActivateAbility`.
//   - Bind to `UPaldarkPalPerceptionComponent::OnThreatChanged` so the
//     `CurrentTargetActor` pointer follows the perception's pick. The
//     attack ability reads this back to know who to apply damage to.
//   - Track `LastAttackTime` + `AttackInterval` to gate `IsAttackReady`.
//   - `TryFireAttack()` — called by the Combat activity every tick. Checks
//     readiness + target validity + range and activates the ability by
//     class. Returns true if activation was attempted (the GAS path may
//     still bail in `ActivateAbility`).
//
// What this is NOT (deferred):
//   - No weapon swap / equipment slot. The Pal's "weapon" is its sole
//     `AttackAbilityClass` — designers author one ability per species
//     (per roadmap "1 ability"). Multi-ability dispatch lands W30+ when
//     Pal subclasses diverge.
//   - No reload / ammo. The cooldown gate is the only resource — ammo
//     ships in W22+ when Pal ranged abilities arrive.
//   - No predicted attack. Server-only execution policy on the ability
//     itself; clients see the damage GE apply via attribute replication.
//
// Roadmap reference: Documents/PALDARK/03-Roadmap_1_Year.md § Tuần 18–19.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Components/ActorComponent.h"
#include "PaldarkPalCombatComponent.generated.h"

class AActor;
class APaldarkPalCharacter;
class UPaldarkAbilitySystemComponent;
class UPaldarkGameplayAbility;
class UPaldarkGameplayAbility_PalAttack;
class UPaldarkPalPerceptionComponent;

UCLASS(ClassGroup = (Paldark), meta = (BlueprintSpawnableComponent), Abstract = false)
class PALDARKLAB_API UPaldarkPalCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPaldarkPalCombatComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Designer knobs ----------------------------------------------------

	// Ability granted on BeginPlay (authority). Designer-authored
	// Blueprint subclass of UPaldarkGameplayAbility_PalAttack. Soft so the
	// ability blueprint doesn't pull at pawn-data load time.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Combat")
	TSoftClassPtr<UPaldarkGameplayAbility_PalAttack> AttackAbilityClass;

	// Cooldown in seconds between consecutive Pal attacks. Default 1.5 s —
	// matches the W18-19 sandbox rhythm. The activity ticks faster than
	// this and only fires when `IsAttackReady` returns true.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Combat", meta = (ClampMin = "0.0"))
	float AttackInterval = 1.5f;

	// Minimum distance (cm) to target before the Pal will fire. Too close
	// = the Pal would walk through the target; the activity uses this to
	// step backwards / hold position.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Combat", meta = (ClampMin = "0.0"))
	float MinEngageRange = 200.f;

	// Maximum distance (cm) at which the Pal can fire its attack. The
	// activity uses this both as the gate for `TryFireAttack` and as the
	// "ThreatDistance" consideration's normalisation factor.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Combat", meta = (ClampMin = "0.0"))
	float MaxEngageRange = 1200.f;

	// Base damage written into the outgoing GE spec via SetByCaller
	// magnitude `Paldark.SetByCaller.PalDamage`. The damage execution
	// reads this + applies armor mitigation.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paldark|Pal|Combat", meta = (ClampMin = "0.0"))
	float BasePalDamage = 12.f;

	// Accessors --------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Paldark|Pal|Combat")
	AActor* GetCurrentTarget() const { return CurrentTargetActor.Get(); }

	UFUNCTION(BlueprintPure, Category = "Paldark|Pal|Combat")
	bool IsAttackReady() const;

	UFUNCTION(BlueprintPure, Category = "Paldark|Pal|Combat")
	float GetLastAttackTime() const { return LastAttackTime; }

	UFUNCTION(BlueprintPure, Category = "Paldark|Pal|Combat")
	float GetCooldownRemaining() const;

	// Authority-only entry point used by UPaldarkActivity_Combat. Returns
	// true if the ability activation was attempted; false on missing
	// target / cooldown / out-of-range.
	bool TryFireAttack();

	// Dump combat state to LogPaldarkPal. Used by Paldark.Pal.DumpThreat.
	void DumpToLog() const;

	// Mirror of the perception delegate so the activity can react in
	// EnterActivity without re-resolving the perception every frame.
	void HandleThreatChanged(AActor* OldThreat, AActor* NewThreat);

protected:
	// Resolved on BeginPlay so accessors don't re-cast every call.
	UPROPERTY(Transient)
	TWeakObjectPtr<APaldarkPalCharacter> PalOwner;

	UPROPERTY(Transient)
	TWeakObjectPtr<UPaldarkPalPerceptionComponent> PerceptionRef;

	// Resolved Pal ASC (server). Stays valid for the Pal's lifetime so we
	// don't re-resolve in the hot path.
	UPROPERTY(Transient)
	TWeakObjectPtr<UPaldarkAbilitySystemComponent> CachedASC;

	// Stored result of the GiveAbility() call at BeginPlay — used to
	// activate the ability without re-walking the ASC's spec table.
	FGameplayAbilitySpecHandle GrantedAttackSpec;

	// Updated by HandleThreatChanged. The ability reads this back via
	// `GetCurrentTarget()` to know who to damage. Server-only.
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentTargetActor;

	// World time (server) of the last successful attack. Used by IsAttackReady.
	float LastAttackTime = -100.f;
};
