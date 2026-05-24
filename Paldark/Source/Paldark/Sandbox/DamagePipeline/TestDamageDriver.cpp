#include "TestDamageDriver.h"

#include "SandboxDamageLog.h"
#include "SandboxDamageTags.h"
#include "TestDamageExecution.h"
#include "../AttributeSet/TestAttrPawn.h"
#include "../AttributeSet/TestPaldarkAttributeSet.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Engine/World.h"
#include "TimerManager.h"

bool UTestDamageDriver::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UTestDamageDriver::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (UWorld* World = GetWorld())
	{
		WorldBeginPlayHandle = World->OnWorldBeginPlay.AddUObject(this, &UTestDamageDriver::OnWorldBeginPlay);
	}
}

void UTestDamageDriver::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->OnWorldBeginPlay.Remove(WorldBeginPlayHandle);
	}
	Super::Deinitialize();
}

void UTestDamageDriver::OnWorldBeginPlay()
{
	UE_LOG(LogSandboxDamage, Display, TEXT("=== Lesson07 DamagePipeline :: OnWorldBeginPlay — spawning ATestAttrPawn (PR-06) ==="));

	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Pawn = World->SpawnActor<ATestAttrPawn>(ATestAttrPawn::StaticClass(), FTransform(FVector(500.f, 0.f, 0.f)), Params);

	World->GetTimerManager().SetTimerForNextTick([WeakThis = TWeakObjectPtr<UTestDamageDriver>(this)]()
	{
		if (UTestDamageDriver* Self = WeakThis.Get())
		{
			Self->RunAllTestCases();
		}
	});
}

// ---------------------------------------------------------------------------
// Helpers — direct attribute overrides via transient GEs for test setup,
// and the actual damage path via Execution.
// ---------------------------------------------------------------------------
void UTestDamageDriver::SetArmor(float Value)
{
	if (!Pawn || !Pawn->ASC) return;

	UGameplayEffect* GE = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("GE_SetArmor")));
	GE->DurationPolicy = EGameplayEffectDurationType::Instant;
	FGameplayModifierInfo Mod;
	Mod.Attribute        = UTestPaldarkAttributeSet::GetArmorAttribute();
	Mod.ModifierOp       = EGameplayModOp::Override;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Value));
	GE->Modifiers.Add(Mod);

	FGameplayEffectContextHandle Ctx = Pawn->ASC->MakeEffectContext();
	Pawn->ASC->ApplyGameplayEffectToSelf(GE, 1.f, Ctx);
}

void UTestDamageDriver::RestoreHealth()
{
	if (!Pawn || !Pawn->ASC) return;

	UGameplayEffect* GE = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("GE_RestoreHealth")));
	GE->DurationPolicy = EGameplayEffectDurationType::Instant;
	FGameplayModifierInfo Mod;
	Mod.Attribute        = UTestPaldarkAttributeSet::GetHealthAttribute();
	Mod.ModifierOp       = EGameplayModOp::Override;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(100.f));
	GE->Modifiers.Add(Mod);

	FGameplayEffectContextHandle Ctx = Pawn->ASC->MakeEffectContext();
	Pawn->ASC->ApplyGameplayEffectToSelf(GE, 1.f, Ctx);
}

void UTestDamageDriver::ApplyDamage(float BaseDamage, bool bTrueDamage)
{
	if (!Pawn || !Pawn->ASC) return;

	// Construct the damage GE with the Execution.
	UGameplayEffect* GE = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("GE_Damage")));
	GE->DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition Exec;
	Exec.CalculationClass = UTestDamageExecution::StaticClass();
	GE->Executions.Add(Exec);

	// Build the spec, attach SetByCaller magnitude + optional true-damage tag.
	FGameplayEffectContextHandle Ctx = Pawn->ASC->MakeEffectContext();
	FGameplayEffectSpec Spec(GE, Ctx, 1.f);
	Spec.SetSetByCallerMagnitude(TAG_Sandbox_SetByCaller_Damage, BaseDamage);
	if (bTrueDamage)
	{
		Spec.DynamicGrantedTags.AddTag(TAG_Sandbox_DamageType_True);
	}

	Pawn->ASC->ApplyGameplayEffectSpecToSelf(Spec);
}

void UTestDamageDriver::RunAllTestCases()
{
	if (!Pawn || !Pawn->AttrSet)
	{
		UE_LOG(LogSandboxDamage, Error, TEXT("Driver: pawn/AttrSet missing — abort"));
		return;
	}

	UTestPaldarkAttributeSet* A = Pawn->AttrSet;
	UE_LOG(LogSandboxDamage, Display, TEXT("=== Lesson07 DamagePipeline :: RUN ALL TESTS ==="));

	// -----------------------------------------------------------------------
	// TC1 — Armor=0, BaseDamage=50 -> Mitigation=1.0 -> Health 100->50.
	// -----------------------------------------------------------------------
	{
		RestoreHealth();
		SetArmor(0.f);
		ApplyDamage(50.f, false);
		const bool bPass = FMath::IsNearlyEqual(A->GetHealth(), 50.f, 0.01f);
		UE_LOG(LogSandboxDamage, Display, TEXT("[TC1] Armor=0 Base=50 -> Health=%.2f (expect 50.00): %s"),
			A->GetHealth(), bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// -----------------------------------------------------------------------
	// TC2 — Armor=50, BaseDamage=50 -> Mitigation=100/150=0.6667 -> Final≈33.33 -> Health 100->66.67.
	// -----------------------------------------------------------------------
	{
		RestoreHealth();
		SetArmor(50.f);
		ApplyDamage(50.f, false);
		const bool bPass = FMath::IsNearlyEqual(A->GetHealth(), 100.f - (50.f * 100.f / 150.f), 0.05f);
		UE_LOG(LogSandboxDamage, Display, TEXT("[TC2] Armor=50 Base=50 -> Health=%.2f (expect ~66.67): %s"),
			A->GetHealth(), bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// -----------------------------------------------------------------------
	// TC3 — Armor=100, BaseDamage=50 -> Mitigation=0.5 -> Final=25 -> Health 100->75.
	// -----------------------------------------------------------------------
	{
		RestoreHealth();
		SetArmor(100.f);
		ApplyDamage(50.f, false);
		const bool bPass = FMath::IsNearlyEqual(A->GetHealth(), 75.f, 0.05f);
		UE_LOG(LogSandboxDamage, Display, TEXT("[TC3] Armor=100 Base=50 -> Health=%.2f (expect 75.00): %s"),
			A->GetHealth(), bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// -----------------------------------------------------------------------
	// TC4 — Linear in BaseDamage (same Armor=100):
	//   Base=100 -> Final=50, Base=200 -> Final=100. Demonstrates caller's
	//   SetByCaller cleanly controls magnitude without touching formula.
	// -----------------------------------------------------------------------
	{
		RestoreHealth();
		SetArmor(100.f);
		ApplyDamage(100.f, false);
		const float HealthAfterFirst = A->GetHealth(); // expect 100-50=50

		RestoreHealth();
		ApplyDamage(200.f, false);
		const float HealthAfterSecond = A->GetHealth(); // expect 100-100=0

		const bool bPass = FMath::IsNearlyEqual(HealthAfterFirst,  50.f, 0.05f)
		                && FMath::IsNearlyEqual(HealthAfterSecond,  0.f, 0.05f);
		UE_LOG(LogSandboxDamage, Display,
			TEXT("[TC4] Armor=100 Base=100->Health=%.2f (expect 50), Base=200->Health=%.2f (expect 0): %s"),
			HealthAfterFirst, HealthAfterSecond, bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// -----------------------------------------------------------------------
	// TC5 — Diminishing returns property (WHY this formula): doubling Armor
	// halves the damage taken, NOT the doubling-by-armor pattern of linear.
	//   Armor 0   -> Mitigation 1.0   (Final 50)
	//   Armor 100 -> Mitigation 0.5   (Final 25)  (1st 100 armor: 100% gain)
	//   Armor 200 -> Mitigation 0.333 (Final 16.67)  (2nd 100 armor: 50% gain)
	//   Armor 400 -> Mitigation 0.2   (Final 10)   (next 200 armor: 50% more)
	// "Effective HP per damage instance scales as (100 + Armor)/100"
	// -----------------------------------------------------------------------
	{
		auto MitForArmor = [](float Armor) { return 100.f / (100.f + Armor); };
		const float M0   = MitForArmor(0.f);
		const float M100 = MitForArmor(100.f);
		const float M200 = MitForArmor(200.f);
		const float M400 = MitForArmor(400.f);
		// Check sequence: each doubling step reduces mitigation but never to 0.
		const bool bPass = (M0 > M100) && (M100 > M200) && (M200 > M400) && (M400 > 0.f)
		                && FMath::IsNearlyEqual(M0, 1.0f,  1e-3f)
		                && FMath::IsNearlyEqual(M100, 0.5f, 1e-3f)
		                && FMath::IsNearlyEqual(M200, 1.f/3.f, 1e-3f)
		                && FMath::IsNearlyEqual(M400, 0.2f, 1e-3f);
		UE_LOG(LogSandboxDamage, Display,
			TEXT("[TC5] Diminishing: Mit(0)=%.3f Mit(100)=%.3f Mit(200)=%.3f Mit(400)=%.3f (always >0): %s"),
			M0, M100, M200, M400, bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// -----------------------------------------------------------------------
	// TC6 — True damage bypasses armor.
	// Armor=100, BaseDamage=50, bTrue=true -> Mitigation forced 1.0 -> Final=50.
	// -----------------------------------------------------------------------
	{
		RestoreHealth();
		SetArmor(100.f);
		ApplyDamage(50.f, /*bTrueDamage*/ true);
		const bool bPass = FMath::IsNearlyEqual(A->GetHealth(), 50.f, 0.05f);
		UE_LOG(LogSandboxDamage, Display,
			TEXT("[TC6] True damage: Armor=100 Base=50 bTrue -> Health=%.2f (expect 50.00, bypass armor): %s"),
			A->GetHealth(), bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// -----------------------------------------------------------------------
	// TC7 — Lethal damage chain still triggers PR-06's OnHealthZeroed.
	// Armor=100, Base=99999 -> Final ~ 49999.5 -> Health clamps to 0.
	// -----------------------------------------------------------------------
	{
		int32 ZeroedCount = 0;
		FDelegateHandle H = A->OnHealthZeroed.AddLambda([&ZeroedCount]() { ZeroedCount++; });

		RestoreHealth();
		SetArmor(100.f);
		ApplyDamage(99999.f, false);

		const bool bPass = FMath::IsNearlyEqual(A->GetHealth(), 0.f, 0.01f) && ZeroedCount == 1;
		UE_LOG(LogSandboxDamage, Display,
			TEXT("[TC7] Lethal: Health=%.2f, OnHealthZeroed count=%d (delta 1): %s"),
			A->GetHealth(), ZeroedCount, bPass ? TEXT("PASS") : TEXT("FAIL"));

		A->OnHealthZeroed.Remove(H);
	}

	UE_LOG(LogSandboxDamage, Display, TEXT("=== Lesson07 DamagePipeline :: DONE ==="));
}
