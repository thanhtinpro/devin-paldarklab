#include "TestShellDriver.h"

#include "SandboxShellLog.h"
#include "TestShellPawn.h"
#include "TestHealthComponent.h"
#include "TestInventoryComponent.h"
#include "TestAbilityComponent.h"

// Reuse the ability tags defined in Lesson 01 instead of redefining new ones —
// proves cross-lesson cohesion of the project.
#include "../GameplayTags/SandboxTags.h"

#include "Engine/World.h"
#include "TimerManager.h"

bool UTestShellDriver::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UTestShellDriver::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (UWorld* World = GetWorld())
	{
		WorldBeginPlayHandle = World->OnWorldBeginPlay.AddUObject(this, &UTestShellDriver::OnWorldBeginPlay);
	}
}

void UTestShellDriver::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->OnWorldBeginPlay.Remove(WorldBeginPlayHandle);
	}
	Super::Deinitialize();
}

void UTestShellDriver::OnWorldBeginPlay()
{
	UE_LOG(LogSandboxShell, Display, TEXT("=== Lesson04 ComponentSlots :: OnWorldBeginPlay — spawning ATestShellPawn ==="));

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnedPawn = World->SpawnActor<ATestShellPawn>(ATestShellPawn::StaticClass(), FTransform::Identity, Params);

	// Defer one tick so the pawn's BeginPlay has finished brokering delegates.
	World->GetTimerManager().SetTimerForNextTick([WeakThis = TWeakObjectPtr<UTestShellDriver>(this)]()
	{
		if (UTestShellDriver* Self = WeakThis.Get())
		{
			Self->RunAllTestCases();
		}
	});
}

void UTestShellDriver::RunAllTestCases()
{
	UE_LOG(LogSandboxShell, Display, TEXT("=== Lesson04 ComponentSlots :: RUN ALL TESTS ==="));

	// ---------------------------------------------------------------------
	// TC1 — Pawn was actually spawned by the driver.
	// ---------------------------------------------------------------------
	{
		const bool bPass = SpawnedPawn != nullptr;
		UE_LOG(LogSandboxShell, Display, TEXT("[TC1] ATestShellPawn auto-spawned by driver: %s"),
			bPass ? TEXT("PASS") : TEXT("FAIL"));
		if (!bPass) return;
	}

	// ---------------------------------------------------------------------
	// TC2 — Shell holds all three slot components.
	// ---------------------------------------------------------------------
	{
		const bool bPass = SpawnedPawn->HealthComp && SpawnedPawn->InventoryComp && SpawnedPawn->AbilityComp;
		UE_LOG(LogSandboxShell, Display, TEXT("[TC2] Shell has Health+Inventory+Ability slots: %s"),
			bPass ? TEXT("PASS") : TEXT("FAIL"));
		if (!bPass) return;
	}

	// ---------------------------------------------------------------------
	// TC3 — Component independence: FindComponentByClass returns the right
	// single instance for each type.
	// ---------------------------------------------------------------------
	{
		UTestHealthComponent*    H = SpawnedPawn->FindComponentByClass<UTestHealthComponent>();
		UTestInventoryComponent* I = SpawnedPawn->FindComponentByClass<UTestInventoryComponent>();
		UTestAbilityComponent*   A = SpawnedPawn->FindComponentByClass<UTestAbilityComponent>();
		const bool bPass = H == SpawnedPawn->HealthComp
		                && I == SpawnedPawn->InventoryComp
		                && A == SpawnedPawn->AbilityComp;
		UE_LOG(LogSandboxShell, Display, TEXT("[TC3] FindComponentByClass returns the same instance per type: %s"),
			bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// ---------------------------------------------------------------------
	// TC4 — ApplyDamage(20) only touches Health state.
	// ---------------------------------------------------------------------
	{
		const int32 InvBefore  = SpawnedPawn->InventoryComp->Items.Num();
		const int32 AbilBefore = SpawnedPawn->AbilityComp->ActiveAbilities.Num();

		SpawnedPawn->HealthComp->ApplyDamage(20);

		const bool bPass = SpawnedPawn->HealthComp->Health == 80
		                && SpawnedPawn->InventoryComp->Items.Num() == InvBefore
		                && SpawnedPawn->AbilityComp->ActiveAbilities.Num() == AbilBefore;
		UE_LOG(LogSandboxShell, Display,
			TEXT("[TC4] Damage 20 -> Health=%d, Inventory size=%d (unchanged), Ability count=%d (unchanged): %s"),
			SpawnedPawn->HealthComp->Health, SpawnedPawn->InventoryComp->Items.Num(),
			SpawnedPawn->AbilityComp->ActiveAbilities.Num(),
			bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// ---------------------------------------------------------------------
	// TC5 — AddItem only touches Inventory state.
	// ---------------------------------------------------------------------
	{
		const int32 HealthBefore = SpawnedPawn->HealthComp->Health;
		const int32 AbilBefore   = SpawnedPawn->AbilityComp->ActiveAbilities.Num();

		SpawnedPawn->InventoryComp->AddItem(TEXT("Potion"));
		SpawnedPawn->InventoryComp->AddItem(TEXT("Key"));

		const bool bPass = SpawnedPawn->InventoryComp->Items.Num() == 2
		                && SpawnedPawn->HealthComp->Health == HealthBefore
		                && SpawnedPawn->AbilityComp->ActiveAbilities.Num() == AbilBefore;
		UE_LOG(LogSandboxShell, Display,
			TEXT("[TC5] AddItem x2 -> Inventory=%d, Health=%d (unchanged), Ability=%d (unchanged): %s"),
			SpawnedPawn->InventoryComp->Items.Num(), SpawnedPawn->HealthComp->Health,
			SpawnedPawn->AbilityComp->ActiveAbilities.Num(),
			bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// ---------------------------------------------------------------------
	// TC6 — ActivateAbility only touches Ability state.
	// Reuses Lesson 01's TAG_Sandbox_Ability_Sprint to demonstrate cross-lesson cohesion.
	// ---------------------------------------------------------------------
	{
		const int32 HealthBefore = SpawnedPawn->HealthComp->Health;
		const int32 InvBefore    = SpawnedPawn->InventoryComp->Items.Num();

		SpawnedPawn->AbilityComp->ActivateAbility(TAG_Sandbox_Ability_Sprint);
		SpawnedPawn->AbilityComp->ActivateAbility(TAG_Sandbox_Ability_Jump);

		const bool bPass = SpawnedPawn->AbilityComp->ActiveAbilities.Num() == 2
		                && SpawnedPawn->HealthComp->Health == HealthBefore
		                && SpawnedPawn->InventoryComp->Items.Num() == InvBefore;
		UE_LOG(LogSandboxShell, Display,
			TEXT("[TC6] ActivateAbility x2 -> Ability=%d, Health=%d (unchanged), Inventory=%d (unchanged): %s"),
			SpawnedPawn->AbilityComp->ActiveAbilities.Num(),
			SpawnedPawn->HealthComp->Health, SpawnedPawn->InventoryComp->Items.Num(),
			bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// ---------------------------------------------------------------------
	// TC7 — Cross-component event: kill pawn via Health, observe that
	// Inventory dropped all and Ability cancelled all — without Inventory
	// or Ability source files ever including TestHealthComponent.h.
	// This is the broker pattern in action.
	// ---------------------------------------------------------------------
	{
		// Current state: Health=80, Inventory=2 items, Ability=2 tags
		UE_LOG(LogSandboxShell, Display, TEXT("[TC7] Pre-kill state: Health=%d Inv=%d Ability=%d"),
			SpawnedPawn->HealthComp->Health, SpawnedPawn->InventoryComp->Items.Num(),
			SpawnedPawn->AbilityComp->ActiveAbilities.Num());

		SpawnedPawn->HealthComp->ApplyDamage(999); // overkill -> Health=0, broadcast

		const bool bPass = SpawnedPawn->HealthComp->Health == 0
		                && SpawnedPawn->InventoryComp->Items.Num() == 0
		                && SpawnedPawn->AbilityComp->ActiveAbilities.Num() == 0;
		UE_LOG(LogSandboxShell, Display,
			TEXT("[TC7] Post-kill (via OnHealthZeroed broker): Health=%d Inv=%d Ability=%d -> %s"),
			SpawnedPawn->HealthComp->Health, SpawnedPawn->InventoryComp->Items.Num(),
			SpawnedPawn->AbilityComp->ActiveAbilities.Num(),
			bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	UE_LOG(LogSandboxShell, Display, TEXT("=== Lesson04 ComponentSlots :: DONE ==="));
}
