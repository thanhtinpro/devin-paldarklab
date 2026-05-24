#include "TestAttrDriver.h"

#include "SandboxAttrLog.h"
#include "TestAttrPawn.h"
#include "TestPaldarkAttributeSet.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Engine/World.h"
#include "TimerManager.h"

bool UTestAttrDriver::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UTestAttrDriver::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (UWorld* World = GetWorld())
	{
		WorldBeginPlayHandle = World->OnWorldBeginPlay.AddUObject(this, &UTestAttrDriver::OnWorldBeginPlay);
	}
}

void UTestAttrDriver::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->OnWorldBeginPlay.Remove(WorldBeginPlayHandle);
	}
	Super::Deinitialize();
}

void UTestAttrDriver::OnWorldBeginPlay()
{
	UE_LOG(LogSandboxAttr, Display, TEXT("=== Lesson06 AttributeSet :: OnWorldBeginPlay — spawning ATestAttrPawn ==="));

	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Pawn = World->SpawnActor<ATestAttrPawn>(ATestAttrPawn::StaticClass(), FTransform::Identity, Params);

	// Defer one tick so the pawn's BeginPlay (InitAbilityActorInfo) completes.
	World->GetTimerManager().SetTimerForNextTick([WeakThis = TWeakObjectPtr<UTestAttrDriver>(this)]()
	{
		if (UTestAttrDriver* Self = WeakThis.Get())
		{
			Self->RunAllTestCases();
		}
	});
}

// ---------------------------------------------------------------------------
// Build a one-shot, instant UGameplayEffect at runtime: Override `Attr` by
// `Value`. Avoids needing cooked GE assets for the sandbox.
// ---------------------------------------------------------------------------
bool UTestAttrDriver::ApplyInstantOverride(const FGameplayAttribute& Attr, float Value)
{
	if (!Pawn || !Pawn->ASC) return false;

	UGameplayEffect* GE = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("GE_SandboxOverride")));
	GE->DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo Mod;
	Mod.Attribute        = Attr;
	Mod.ModifierOp       = EGameplayModOp::Override;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Value));
	GE->Modifiers.Add(Mod);

	FGameplayEffectContextHandle Ctx = Pawn->ASC->MakeEffectContext();
	Pawn->ASC->ApplyGameplayEffectToSelf(GE, /*Level*/ 1.f, Ctx);
	return true;
}

void UTestAttrDriver::RunAllTestCases()
{
	if (!Pawn || !Pawn->AttrSet || !Pawn->ASC)
	{
		UE_LOG(LogSandboxAttr, Error, TEXT("Driver: pawn / AttrSet / ASC missing — abort"));
		return;
	}

	UTestPaldarkAttributeSet* A = Pawn->AttrSet;

	// Track OnHealthZeroed
	A->OnHealthZeroed.AddLambda([this]()
	{
		HealthZeroedCallCount++;
	});

	UE_LOG(LogSandboxAttr, Display, TEXT("=== Lesson06 AttributeSet :: RUN ALL TESTS ==="));

	// -----------------------------------------------------------------------
	// TC1 — Init defaults (set by AttributeSet ctor).
	// -----------------------------------------------------------------------
	{
		const bool bPass = FMath::IsNearlyEqual(A->GetHealth(),         100.f)
		                && FMath::IsNearlyEqual(A->GetMaxHealth(),      100.f)
		                && FMath::IsNearlyEqual(A->GetStamina(),        100.f)
		                && FMath::IsNearlyEqual(A->GetMaxStamina(),     100.f)
		                && FMath::IsNearlyEqual(A->GetArmor(),           50.f)
		                && FMath::IsNearlyEqual(A->GetIncomingDamage(),   0.f);
		UE_LOG(LogSandboxAttr, Display,
			TEXT("[TC1] Init H=%.0f/%.0f S=%.0f/%.0f Armor=%.0f InDmg=%.0f: %s"),
			A->GetHealth(), A->GetMaxHealth(), A->GetStamina(), A->GetMaxStamina(),
			A->GetArmor(), A->GetIncomingDamage(),
			bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// -----------------------------------------------------------------------
	// TC2 — PreAttributeChange upper-clamp on Health. Apply Override=999 -> 100.
	// -----------------------------------------------------------------------
	{
		ApplyInstantOverride(UTestPaldarkAttributeSet::GetHealthAttribute(), 999.f);
		const bool bPass = FMath::IsNearlyEqual(A->GetHealth(), 100.f);
		UE_LOG(LogSandboxAttr, Display, TEXT("[TC2] Override Health=999 -> clamped to %.0f (max 100): %s"),
			A->GetHealth(), bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// -----------------------------------------------------------------------
	// TC3 — PreAttributeChange lower-clamp on Health. Override=-50 -> 0.
	// -----------------------------------------------------------------------
	{
		ApplyInstantOverride(UTestPaldarkAttributeSet::GetHealthAttribute(), -50.f);
		const bool bPass = FMath::IsNearlyEqual(A->GetHealth(), 0.f);
		UE_LOG(LogSandboxAttr, Display, TEXT("[TC3] Override Health=-50 -> clamped to %.0f (min 0): %s"),
			A->GetHealth(), bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// Reset for next TCs
	ApplyInstantOverride(UTestPaldarkAttributeSet::GetHealthAttribute(), 60.f);

	// -----------------------------------------------------------------------
	// TC4 — In-range override goes through untouched.
	// -----------------------------------------------------------------------
	{
		const bool bPass = FMath::IsNearlyEqual(A->GetHealth(), 60.f);
		UE_LOG(LogSandboxAttr, Display, TEXT("[TC4] Override Health=60 (in range) -> %.0f: %s"),
			A->GetHealth(), bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// -----------------------------------------------------------------------
	// TC5 — Meta-attribute: deposit IncomingDamage=30 -> PostExecute drains
	// it into Health (60 -> 30) and resets IncomingDamage to 0.
	// -----------------------------------------------------------------------
	{
		const float HealthBefore = A->GetHealth();
		ApplyInstantOverride(UTestPaldarkAttributeSet::GetIncomingDamageAttribute(), 30.f);
		const bool bPass = FMath::IsNearlyEqual(A->GetHealth(), HealthBefore - 30.f)
		                && FMath::IsNearlyEqual(A->GetIncomingDamage(), 0.f);
		UE_LOG(LogSandboxAttr, Display,
			TEXT("[TC5] Deposit IncomingDamage=30: Health %.0f -> %.0f, IncomingDamage=%.0f (mailbox cleared): %s"),
			HealthBefore, A->GetHealth(), A->GetIncomingDamage(),
			bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// -----------------------------------------------------------------------
	// TC6 — Lethal damage path: Health=30, deposit 9999, expect clamp to 0
	// AND OnHealthZeroed fires exactly once.
	// -----------------------------------------------------------------------
	{
		const int32 ZeroedCountBefore = HealthZeroedCallCount;
		ApplyInstantOverride(UTestPaldarkAttributeSet::GetIncomingDamageAttribute(), 9999.f);
		const bool bPass = FMath::IsNearlyEqual(A->GetHealth(), 0.f)
		                && HealthZeroedCallCount == ZeroedCountBefore + 1;
		UE_LOG(LogSandboxAttr, Display,
			TEXT("[TC6] Lethal IncomingDamage=9999: Health=%.0f, OnHealthZeroed fired count=%d (delta %d): %s"),
			A->GetHealth(), HealthZeroedCallCount, HealthZeroedCallCount - ZeroedCountBefore,
			bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// -----------------------------------------------------------------------
	// TC7 — Independence: through TCs 1–6 we never touched Stamina or Armor.
	// They should sit at their init values (100, 50).
	// -----------------------------------------------------------------------
	{
		const bool bPass = FMath::IsNearlyEqual(A->GetStamina(), 100.f)
		                && FMath::IsNearlyEqual(A->GetArmor(),    50.f);
		UE_LOG(LogSandboxAttr, Display,
			TEXT("[TC7] Untouched attrs: Stamina=%.0f (init 100), Armor=%.0f (init 50): %s"),
			A->GetStamina(), A->GetArmor(), bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	UE_LOG(LogSandboxAttr, Display, TEXT("=== Lesson06 AttributeSet :: DONE ==="));
}
