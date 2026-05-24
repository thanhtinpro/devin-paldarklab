#include "TestInputBinder.h"

#include "SandboxInputLog.h"
#include "SandboxInputTags.h"
#include "TestInputAction.h"
#include "TestInputConfig.h"

#include "Engine/World.h"

bool UTestInputBinder::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UTestInputBinder::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogSandboxInput, Display, TEXT("=== Lesson03 PawnData+Input :: Subsystem Initialize — seeding PawnData variants ==="));
	SeedTwoPawnDataVariants();

	if (UWorld* World = GetWorld())
	{
		WorldBeginPlayHandle = World->OnWorldBeginPlay.AddUObject(this, &UTestInputBinder::OnWorldBeginPlay);
	}
}

void UTestInputBinder::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->OnWorldBeginPlay.Remove(WorldBeginPlayHandle);
	}
	TagCallbacks.Reset();
	Super::Deinitialize();
}

// ---------------------------------------------------------------------------
// Seed two PawnData rows that share PawnClass but expose different input sets.
// ---------------------------------------------------------------------------
void UTestInputBinder::SeedTwoPawnDataVariants()
{
	auto MakeAction = [this](FName Id) -> UTestInputAction*
	{
		UTestInputAction* A = NewObject<UTestInputAction>(this);
		A->ActionId = Id;
		return A;
	};

	// KBM scheme: full Move+Look+Jump+Sprint+Interact
	UTestInputConfig* KBMConfig = NewObject<UTestInputConfig>(this);
	KBMConfig->NativeInputActions = {
		{TAG_Input_Move,     MakeAction(TEXT("IA_Move_WASD"))},
		{TAG_Input_Look,     MakeAction(TEXT("IA_Look_Mouse"))},
		{TAG_Input_Jump,     MakeAction(TEXT("IA_Jump_Space"))},
		{TAG_Input_Sprint,   MakeAction(TEXT("IA_Sprint_LShift"))},
		{TAG_Input_Interact, MakeAction(TEXT("IA_Interact_E"))},
	};

	PawnData_KBM = NewObject<UTestPawnData>(this);
	PawnData_KBM->PawnDataName  = TEXT("PD_KBM");
	PawnData_KBM->PawnClassName = TEXT("BP_PaldarkCharacter");
	PawnData_KBM->InputConfig   = KBMConfig;

	// Gamepad scheme: same SHAPE, but different concrete actions, AND no Sprint
	// (proves "different config = different available intents" without recompiling Pawn).
	UTestInputConfig* GamepadConfig = NewObject<UTestInputConfig>(this);
	GamepadConfig->NativeInputActions = {
		{TAG_Input_Move,     MakeAction(TEXT("IA_Move_LStick"))},
		{TAG_Input_Look,     MakeAction(TEXT("IA_Look_RStick"))},
		{TAG_Input_Jump,     MakeAction(TEXT("IA_Jump_A"))},
		{TAG_Input_Interact, MakeAction(TEXT("IA_Interact_X"))},
		// no Sprint mapping intentionally
	};

	PawnData_Gamepad = NewObject<UTestPawnData>(this);
	PawnData_Gamepad->PawnDataName  = TEXT("PD_Gamepad");
	PawnData_Gamepad->PawnClassName = TEXT("BP_PaldarkCharacter"); // SAME class as KBM
	PawnData_Gamepad->InputConfig   = GamepadConfig;
}

// ---------------------------------------------------------------------------
// Bind: iterate config and install a single callback per tag. The callback
// increments a per-action counter so tests can prove which fired.
// ---------------------------------------------------------------------------
void UTestInputBinder::BindToPawnData(const UTestPawnData* PawnData)
{
	TagCallbacks.Reset();
	ResetScoreboard();

	if (!PawnData || !PawnData->InputConfig)
	{
		UE_LOG(LogSandboxInput, Warning, TEXT("BindToPawnData: null PawnData or InputConfig — no bindings created"));
		return;
	}

	for (const FTestInputActionBinding& B : PawnData->InputConfig->NativeInputActions)
	{
		if (!B.InputAction)
		{
			UE_LOG(LogSandboxInput, Warning, TEXT("BindToPawnData: tag %s has null action — skipping"), *B.InputTag.ToString());
			continue;
		}

		FOnSandboxInputFired Cb;
		Cb.BindLambda([WeakThis = TWeakObjectPtr<UTestInputBinder>(this)](const UTestInputAction* Fired)
		{
			if (UTestInputBinder* Self = WeakThis.Get())
			{
				if (Fired)
				{
					Self->FiredActionCounts.FindOrAdd(Fired->ActionId)++;
				}
			}
		});
		TagCallbacks.Add(B.InputTag, MoveTemp(Cb));
	}

	UE_LOG(LogSandboxInput, Display,
		TEXT("BindToPawnData: '%s' (PawnClass=%s) -> %d tag callbacks installed"),
		*PawnData->PawnDataName.ToString(),
		*PawnData->PawnClassName.ToString(),
		TagCallbacks.Num());
}

// ---------------------------------------------------------------------------
// Simulate "input event arrives with intent tag X". Production code wouldn't
// look up tag at the input-arrival path — EnhancedInput already gave it an
// UInputAction. The sandbox uses the tag-keyed map so we can prove the routing.
// ---------------------------------------------------------------------------
void UTestInputBinder::SimulateInput(const FGameplayTag& InputTag)
{
	if (FOnSandboxInputFired* Cb = TagCallbacks.Find(InputTag))
	{
		// Look up the concrete action via the current PawnData — pretend the
		// event payload carries it (in production it would).
		UE_LOG(LogSandboxInput, Verbose, TEXT("SimulateInput: %s found in TagCallbacks"), *InputTag.ToString());

		// For sandbox: walk both pawn datas to find which action this tag maps to
		// (we just want to feed the callback a non-null ptr so it can record).
		UTestInputAction* Action = nullptr;
		for (const UTestPawnData* PD : { PawnData_KBM, PawnData_Gamepad })
		{
			if (PD && PD->InputConfig)
			{
				if (UTestInputAction* A = PD->InputConfig->FindActionByTag(InputTag))
				{
					Action = A;
					// don't break — the *currently bound* config's action wins logically,
					// but for the FiredActionCounts the callback uses whatever it receives.
					// In a real impl this would just be GetCurrentPawnData()->InputConfig->FindActionByTag.
					break;
				}
			}
		}
		Cb->ExecuteIfBound(Action);
	}
	else
	{
		UE_LOG(LogSandboxInput, Warning,
			TEXT("SimulateInput: tag '%s' not bound on current PawnData — input ignored (graceful)"),
			*InputTag.ToString());
	}
}

// ---------------------------------------------------------------------------
// Auto driver
// ---------------------------------------------------------------------------
void UTestInputBinder::OnWorldBeginPlay()
{
	UE_LOG(LogSandboxInput, Display, TEXT("=== Lesson03 PawnData+Input :: OnWorldBeginPlay — RUN ALL TESTS ==="));
	RunAllTestCases();
	UE_LOG(LogSandboxInput, Display, TEXT("=== Lesson03 PawnData+Input :: DONE ==="));
}

void UTestInputBinder::RunAllTestCases()
{
	// -----------------------------------------------------------------------
	// TC1 — PawnData wires an InputConfig.
	// -----------------------------------------------------------------------
	{
		const bool bPass = PawnData_KBM && PawnData_KBM->InputConfig
		                && PawnData_Gamepad && PawnData_Gamepad->InputConfig;
		UE_LOG(LogSandboxInput, Display, TEXT("[TC1] Both PawnData variants have InputConfig: %s"),
			bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// -----------------------------------------------------------------------
	// TC2 — FindActionByTag returns the right concrete action per scheme.
	// -----------------------------------------------------------------------
	{
		const UTestInputAction* MoveKBM = PawnData_KBM->InputConfig->FindActionByTag(TAG_Input_Move);
		const UTestInputAction* MoveGP  = PawnData_Gamepad->InputConfig->FindActionByTag(TAG_Input_Move);
		const bool bPass = MoveKBM && MoveGP
		                && MoveKBM->ActionId == FName(TEXT("IA_Move_WASD"))
		                && MoveGP->ActionId  == FName(TEXT("IA_Move_LStick"));
		UE_LOG(LogSandboxInput, Display,
			TEXT("[TC2] Input.Move: KBM->%s Gamepad->%s : %s"),
			MoveKBM ? *MoveKBM->ActionId.ToString() : TEXT("<null>"),
			MoveGP  ? *MoveGP->ActionId.ToString()  : TEXT("<null>"),
			bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// -----------------------------------------------------------------------
	// TC3 — Unknown tag returns null (graceful) and a SimulateInput on an
	// unbound tag logs Warning + no callback fires.
	// -----------------------------------------------------------------------
	{
		BindToPawnData(PawnData_Gamepad); // Gamepad has NO Sprint binding
		const int32 SprintBefore = CountFor(TEXT("IA_Sprint_LShift"));
		SimulateInput(TAG_Input_Sprint);
		const int32 SprintAfter = CountFor(TEXT("IA_Sprint_LShift"));
		const bool bPass = (SprintBefore == 0 && SprintAfter == 0);
		UE_LOG(LogSandboxInput, Display,
			TEXT("[TC3] Gamepad bound, simulate Input.Sprint (unmapped): Sprint count %d->%d : %s"),
			SprintBefore, SprintAfter, bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// -----------------------------------------------------------------------
	// TC4 — Same Pawn class, different bound action count proves the input
	// scheme is *purely data*. KBM=5 bindings, Gamepad=4 bindings.
	// -----------------------------------------------------------------------
	{
		BindToPawnData(PawnData_KBM);
		const int32 KBMCount = TagCallbacks.Num();
		BindToPawnData(PawnData_Gamepad);
		const int32 GPCount = TagCallbacks.Num();
		const bool bSamePawnClass = PawnData_KBM->PawnClassName == PawnData_Gamepad->PawnClassName;
		const bool bPass = bSamePawnClass && KBMCount == 5 && GPCount == 4;
		UE_LOG(LogSandboxInput, Display,
			TEXT("[TC4] Same PawnClass=%s, KBM bindings=%d, Gamepad bindings=%d : %s"),
			*PawnData_KBM->PawnClassName.ToString(), KBMCount, GPCount,
			bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// -----------------------------------------------------------------------
	// TC5 — Tag routing isolation: SimulateInput(Jump) fires ONLY Jump, not Move.
	// -----------------------------------------------------------------------
	{
		BindToPawnData(PawnData_KBM);
		SimulateInput(TAG_Input_Jump);
		const int32 JumpHits = CountFor(TEXT("IA_Jump_Space"));
		const int32 MoveHits = CountFor(TEXT("IA_Move_WASD"));
		const bool bPass = JumpHits == 1 && MoveHits == 0;
		UE_LOG(LogSandboxInput, Display,
			TEXT("[TC5] After SimulateInput(Jump): Jump=%d Move=%d : %s"),
			JumpHits, MoveHits, bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// -----------------------------------------------------------------------
	// TC6 — Hot-swap PawnData: re-bind to Gamepad, simulate Move again,
	// and the *Gamepad* IA_Move_LStick counter should increment, not KBM's.
	// -----------------------------------------------------------------------
	{
		BindToPawnData(PawnData_Gamepad); // fresh scoreboard
		SimulateInput(TAG_Input_Move);
		const int32 KBMMove = CountFor(TEXT("IA_Move_WASD"));
		const int32 GPMove  = CountFor(TEXT("IA_Move_LStick"));
		const bool bPass = KBMMove == 0 && GPMove == 1;
		UE_LOG(LogSandboxInput, Display,
			TEXT("[TC6] After hot-swap to Gamepad + SimulateInput(Move): KBM.Move=%d Gamepad.Move=%d : %s"),
			KBMMove, GPMove, bPass ? TEXT("PASS") : TEXT("FAIL"));
	}
}
