// PALDARK W1 — Primary game module implementation. Owns:
//   * IMPLEMENT_PRIMARY_GAME_MODULE macro (required by UnrealBuildTool for a Game target).
//   * `Paldark.HelloWorld [optional message]` console command (W1 day 4).
//   * `Paldark.Experience.Current`            — print current experience id (W1 day 6-7).
//   * `Paldark.Experience.Hello`              — log current experience HelloWorldMessage.
//   * `Paldark.Experience.ListExtensions`     — list active action sets + granted tags (W1 day 8-10).
//   * `Paldark.Input.ListBindings`            — list mapped IMCs + InputConfig rows (W1 day 11-14).
//   * `Paldark.Pal.SpawnTestCompanion`        — spawn 1 Pal behind local player (W3-4).
//   * `Paldark.Pal.CurrentActivity`           — log active activity tag for every Pal (W5-6).
//   * `Paldark.Pal.SetActivity <Idle|Follow|Investigate>` — force-switch FSM (W5-6).
//   * `Paldark.Pal.Ping [X Y Z]`              — file Investigate request (W5-6).
//   * `Paldark.Gas.DumpAttributes`            — dump Health/Stamina/MoveSpeed/Armor for every IAbilitySystemInterface actor (W7-8).
//   * `Paldark.Gas.Damage <Amount>`           — apply a one-shot damage GE to the local player (W7-8).
//   * `Paldark.Combat.SpawnDummy [Distance]`  — spawn an APaldarkDummyTarget in front of the local player (W9-10).
//   * `Paldark.Combat.Fire`                   — activate the Fire ability on the local player without keyboard input (W9-10).
//   * `Paldark.Inventory.List`                — dump local player inventory contents (W11-12).
//   * `Paldark.Inventory.Add    <Tag> [Count]` — server-add an item by Paldark.Item.* tag (W11-12).
//   * `Paldark.Inventory.Remove <Tag> [Count]` — server-remove an item by tag (W11-12).
//   * `Paldark.Inventory.Drop`                — clear the local player's inventory on the server (W11-12).
//   * `Paldark.Pal.SpawnFromDefinition <DefId> [X Y Z]`
//                                            — async-load + spawn from a PaldarkPalDefinition primary asset id (W27-28).
//   * `Paldark.Pal.DumpDefinitionRegistry`    — list pre-warm state for every PaldarkPalDefinition tracked by the spawn subsystem (W27-28).
//   * `Paldark.Hub.Stable.List`               — dump active roster + deposit list (W44-45).
//   * `Paldark.Hub.Stable.Deposit  <Idx>`     — controller->RequestDeposit (W44-45).
//   * `Paldark.Hub.Stable.Withdraw <Idx>`     — controller->RequestWithdraw (W44-45).
//   * `Paldark.Hub.Stable.Heal     <Idx>`     — controller->RequestHeal (debit credits, refill HP) (W44-45).
//   * `Paldark.Hub.Market.Catalog [Kiosk]`    — dump kiosk catalog + buy/sell prices (W44-45).
//   * `Paldark.Hub.Market.Buy  <Tag> [Count]` — controller->RequestBuy (W44-45).
//   * `Paldark.Hub.Market.Sell <Tag> [Count]` — controller->RequestSell (W44-45).
//   * `Paldark.Hub.Market.Balance`            — log local player's credits balance (W44-45).

#include "PaldarkLab.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "EngineUtils.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "HAL/IConsoleManager.h"
#include "InputAction.h"

#include "Combat/PaldarkDummyTarget.h"
#include "Experience/PaldarkExperienceActionSet.h"
#include "Experience/PaldarkExperienceDefinition.h"
#include "Experience/PaldarkInputConfig.h"
#include "Experience/PaldarkPawnData.h"
#include "Framework/PaldarkGameModeBase.h"
#include "Framework/PaldarkGameStateBase.h"
#include "Gas/PaldarkAbilitySystemComponent.h"
#include "Gas/PaldarkAttributeSet.h"
#include "Hub/PaldarkBriefingController.h"
#include "Hub/PaldarkBriefingSessionComponent.h"
#include "Hub/PaldarkHubBuildingDefinition.h"
#include "Hub/PaldarkMarketplaceController.h"
#include "Hub/PaldarkMarketplaceKiosk.h"
#include "Hub/PaldarkPalStable.h"
#include "Hub/PaldarkStableController.h"
#include "Inventory/Fragments/PaldarkItemFragment_MarketValue.h"
#include "Inventory/PaldarkItemDefinition.h"
#include "Pal/Activities/PaldarkBaseActivity.h"
#include "Pal/Components/PaldarkPalActivityComponent.h"
#include "Pal/PaldarkPalCharacter.h"
#include "Pal/PaldarkPalDefinition.h"
#include "Pal/PaldarkPalDepositComponent.h"
#include "Pal/PaldarkPalSpawnSubsystem.h"
#include "Pal/PaldarkPlayerPalRosterComponent.h"
#include "PaldarkGameplayTags.h"
#include "Player/Components/PaldarkPlayerInventoryComponent.h"
#include "Player/PaldarkCharacter.h"
#include "Player/PaldarkPlayerController.h"
#include "Player/PaldarkPlayerState.h"
#include "Save/PaldarkSaveSubsystem.h"

#include "Engine/AssetManager.h"
#include "EngineUtils.h"

static void HandleHelloWorldCommand(const TArray<FString>& Args, UWorld* World)
{
	const FString Message = Args.Num() > 0
		? FString::Join(Args, TEXT(" "))
		: TEXT("Hello from PaldarkLab.");

	UE_LOG(LogPaldark, Log, TEXT("[Paldark.HelloWorld] %s"), *Message);

	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			5.0f,
			FColor::Cyan,
			FString::Printf(TEXT("Paldark.HelloWorld: %s"), *Message));
	}
}

static APaldarkGameStateBase* FindPaldarkGameState(UWorld* World)
{
	if (World == nullptr)
	{
		return nullptr;
	}
	return World->GetGameState<APaldarkGameStateBase>();
}

static void HandleExperienceCurrentCommand(const TArray<FString>& Args, UWorld* World)
{
	APaldarkGameStateBase* State = FindPaldarkGameState(World);
	if (State == nullptr)
	{
		UE_LOG(LogPaldark, Warning, TEXT("[Paldark.Experience.Current] no PaldarkGameStateBase in current world."));
		return;
	}

	const FPrimaryAssetId Id = State->GetCurrentExperienceId();
	const UPaldarkExperienceDefinition* Experience = State->GetCurrentExperience();

	UE_LOG(
		LogPaldark,
		Log,
		TEXT("[Paldark.Experience.Current] id=%s ptr=%s"),
		Id.IsValid() ? *Id.ToString() : TEXT("<invalid>"),
		Experience != nullptr ? *Experience->GetPathName() : TEXT("<nullptr>"));
}

static void HandleExperienceHelloCommand(const TArray<FString>& Args, UWorld* World)
{
	APaldarkGameStateBase* State = FindPaldarkGameState(World);
	const UPaldarkExperienceDefinition* Experience = State != nullptr ? State->GetCurrentExperience() : nullptr;
	if (Experience == nullptr)
	{
		UE_LOG(LogPaldark, Warning, TEXT("[Paldark.Experience.Hello] no current experience loaded."));
		return;
	}

	const FText Message = Experience->HelloWorldMessage.IsEmpty()
		? FText::FromString(TEXT("(experience has no HelloWorldMessage set)"))
		: Experience->HelloWorldMessage;

	UE_LOG(LogPaldark, Log, TEXT("[Paldark.Experience.Hello] %s"), *Message.ToString());

	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			6.0f,
			FColor::Green,
			FString::Printf(TEXT("Experience hello: %s"), *Message.ToString()));
	}
}

static void HandleExperienceListExtensionsCommand(const TArray<FString>& Args, UWorld* World)
{
	if (World == nullptr)
	{
		UE_LOG(LogPaldark, Warning, TEXT("[Paldark.Experience.ListExtensions] no world."));
		return;
	}
	const APaldarkGameModeBase* GameMode = World->GetAuthGameMode<APaldarkGameModeBase>();
	if (GameMode == nullptr)
	{
		UE_LOG(LogPaldark, Warning, TEXT("[Paldark.Experience.ListExtensions] no PaldarkGameModeBase (run on server)."));
		return;
	}

	const FPrimaryAssetId& Id = GameMode->GetResolvedExperienceId();
	const UPaldarkExperienceDefinition* Experience = GameMode->GetLoadedExperience();
	const FGameplayTagContainer& ActiveTags = GameMode->GetActiveExperienceTags();

	UE_LOG(
		LogPaldark,
		Log,
		TEXT("[Paldark.Experience.ListExtensions] id=%s experience=%s tags=%d action_sets=%d"),
		Id.IsValid() ? *Id.ToString() : TEXT("<invalid>"),
		Experience != nullptr ? *Experience->GetPathName() : TEXT("<nullptr>"),
		ActiveTags.Num(),
		Experience != nullptr ? Experience->ActionSets.Num() : 0);

	if (ActiveTags.Num() > 0)
	{
		TArray<FGameplayTag> TagArray;
		ActiveTags.GetGameplayTagArray(TagArray);
		for (const FGameplayTag& Tag : TagArray)
		{
			UE_LOG(LogPaldark, Log, TEXT("  tag: %s"), *Tag.ToString());
		}
	}

	if (Experience != nullptr)
	{
		for (const TSoftObjectPtr<UPaldarkExperienceActionSet>& SoftSet : Experience->ActionSets)
		{
			UE_LOG(
				LogPaldark,
				Log,
				TEXT("  action set: %s loaded=%s"),
				*SoftSet.ToString(),
				SoftSet.Get() != nullptr ? TEXT("yes") : TEXT("no"));
		}
	}
}

static void HandleInputListBindingsCommand(const TArray<FString>& Args, UWorld* World)
{
	if (World == nullptr)
	{
		UE_LOG(LogPaldark, Warning, TEXT("[Paldark.Input.ListBindings] no world."));
		return;
	}

	// Walk the world's player controllers — in PIE there's only one local
	// player so this is effectively "the current player". Dedicated-server
	// listeners are skipped (no LocalPlayer).
	int32 Inspected = 0;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APaldarkPlayerController* PC = Cast<APaldarkPlayerController>(It->Get());
		if (PC == nullptr)
		{
			continue;
		}
		++Inspected;

		const UPaldarkPawnData* PawnData = PC->GetActivePawnData();
		const ULocalPlayer* LP = PC->GetLocalPlayer();
		const UEnhancedInputLocalPlayerSubsystem* Subsystem = LP != nullptr
			? LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()
			: nullptr;

		UE_LOG(
			LogPaldark,
			Log,
			TEXT("[Paldark.Input.ListBindings] controller=%s pawn_data=%s subsystem=%s imc_count=%d"),
			*PC->GetName(),
			PawnData != nullptr ? *PawnData->GetName() : TEXT("<null>"),
			Subsystem != nullptr ? TEXT("yes") : TEXT("no"),
			PawnData != nullptr ? PawnData->DefaultMappingContexts.Num() : 0);

		if (PawnData != nullptr)
		{
			for (const FPaldarkMappingContextAndPriority& Row : PawnData->DefaultMappingContexts)
			{
				UE_LOG(
					LogPaldark,
					Log,
					TEXT("  imc=%s priority=%d register_with_settings=%s"),
					*Row.InputMapping.ToString(),
					Row.Priority,
					Row.bRegisterWithSettings ? TEXT("yes") : TEXT("no"));
			}

			if (!PawnData->InputConfig.IsNull())
			{
				const UPaldarkInputConfig* Config = PawnData->InputConfig.Get();
				UE_LOG(
					LogPaldark,
					Log,
					TEXT("  input_config=%s loaded=%s native=%d ability=%d"),
					*PawnData->InputConfig.ToString(),
					Config != nullptr ? TEXT("yes") : TEXT("no"),
					Config != nullptr ? Config->NativeInputActions.Num() : 0,
					Config != nullptr ? Config->AbilityInputActions.Num() : 0);

				if (Config != nullptr)
				{
					for (const FPaldarkInputAction& Row : Config->NativeInputActions)
					{
						UE_LOG(
							LogPaldark,
							Log,
							TEXT("    native tag=%s action=%s"),
							*Row.InputTag.ToString(),
							Row.InputAction != nullptr ? *Row.InputAction->GetName() : TEXT("<null>"));
					}
					for (const FPaldarkInputAction& Row : Config->AbilityInputActions)
					{
						UE_LOG(
							LogPaldark,
							Log,
							TEXT("    ability tag=%s action=%s"),
							*Row.InputTag.ToString(),
							Row.InputAction != nullptr ? *Row.InputAction->GetName() : TEXT("<null>"));
					}
				}
			}
		}
	}

	if (Inspected == 0)
	{
		UE_LOG(LogPaldark, Warning, TEXT("[Paldark.Input.ListBindings] no APaldarkPlayerController in current world (try running from PIE)."));
	}
}

static void HandlePalSpawnTestCompanionCommand(const TArray<FString>& Args, UWorld* World)
{
	if (World == nullptr)
	{
		UE_LOG(LogPaldarkPal, Warning, TEXT("[Paldark.Pal.SpawnTestCompanion] no world."));
		return;
	}

	// Spawn must run on authority — bail with a clear log on client-only worlds.
	if (World->GetNetMode() == NM_Client)
	{
		UE_LOG(LogPaldarkPal, Warning, TEXT("[Paldark.Pal.SpawnTestCompanion] cannot spawn on a client world."));
		return;
	}

	APlayerController* LocalPC = World->GetFirstPlayerController();
	APawn* LocalPawn = LocalPC != nullptr ? LocalPC->GetPawn() : nullptr;
	if (LocalPawn == nullptr)
	{
		UE_LOG(LogPaldarkPal, Warning, TEXT("[Paldark.Pal.SpawnTestCompanion] no local player pawn (PIE/server must have one)."));
		return;
	}

	// Resolve Pal class — args[0] (index) reads PawnData->DefaultPalCompanions,
	// args[1] (class_path) overrides directly. Falls back to args[0]=0.
	const int32 Index = Args.IsValidIndex(0) ? FCString::Atoi(*Args[0]) : 0;
	TSubclassOf<APaldarkPalCharacter> PalClass;
	FPaldarkPalCompanionSpec ResolvedSpec;

	if (Args.IsValidIndex(1))
	{
		const FSoftClassPath ClassPath(Args[1]);
		PalClass = ClassPath.TryLoadClass<APaldarkPalCharacter>();
	}
	else
	{
		const APaldarkPlayerController* PaldarkPC = Cast<APaldarkPlayerController>(LocalPC);
		const UPaldarkPawnData* PawnData = PaldarkPC != nullptr ? PaldarkPC->GetActivePawnData() : nullptr;
		if (PawnData != nullptr && PawnData->DefaultPalCompanions.IsValidIndex(Index))
		{
			ResolvedSpec = PawnData->DefaultPalCompanions[Index];
			PalClass    = ResolvedSpec.PalClass.LoadSynchronous();
		}
	}

	if (PalClass == nullptr)
	{
		UE_LOG(
			LogPaldarkPal,
			Warning,
			TEXT("[Paldark.Pal.SpawnTestCompanion] no Pal class — pass `class_path` arg or set DefaultPalCompanions[%d] on PawnData."),
			Index);
		return;
	}

	const FVector LeaderLoc = LocalPawn->GetActorLocation();
	const FRotator LeaderRot = LocalPawn->GetActorRotation();
	const FVector LocalOffset = ResolvedSpec.SpawnOffset.IsZero()
		? FVector(-200.0f, 150.0f, 0.0f)
		: ResolvedSpec.SpawnOffset;
	const FVector SpawnLoc = LeaderLoc + LeaderRot.RotateVector(LocalOffset);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	APaldarkPalCharacter* Pal = World->SpawnActor<APaldarkPalCharacter>(PalClass, SpawnLoc, LeaderRot, Params);
	if (Pal == nullptr)
	{
		UE_LOG(LogPaldarkPal, Warning, TEXT("[Paldark.Pal.SpawnTestCompanion] SpawnActor returned null (collision?)."));
		return;
	}

	Pal->SetFollowedPawn(LocalPawn);
	UE_LOG(
		LogPaldarkPal,
		Log,
		TEXT("[Paldark.Pal.SpawnTestCompanion] spawned %s at %s following %s"),
		*Pal->GetName(),
		*SpawnLoc.ToCompactString(),
		*LocalPawn->GetName());

	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			5.0f,
			FColor::Yellow,
			FString::Printf(TEXT("Pal spawned: %s (following %s)"), *Pal->GetName(), *LocalPawn->GetName()));
	}
}

static int32 IteratePalsInWorld(UWorld* World, TFunctionRef<void(APaldarkPalCharacter*)> Visit)
{
	if (World == nullptr)
	{
		return 0;
	}
	int32 Count = 0;
	for (TActorIterator<APaldarkPalCharacter> It(World); It; ++It)
	{
		APaldarkPalCharacter* Pal = *It;
		if (Pal == nullptr)
		{
			continue;
		}
		Visit(Pal);
		++Count;
	}
	return Count;
}

static FGameplayTag ResolveActivityTagArg(const FString& Arg)
{
	const FString Normalised = Arg.ToLower();
	if (Normalised == TEXT("idle"))
	{
		return PaldarkGameplayTags::TAG_Paldark_Pal_Activity_Idle;
	}
	if (Normalised == TEXT("follow"))
	{
		return PaldarkGameplayTags::TAG_Paldark_Pal_Activity_Follow;
	}
	if (Normalised == TEXT("investigate") || Normalised == TEXT("invest"))
	{
		return PaldarkGameplayTags::TAG_Paldark_Pal_Activity_Investigate;
	}
	// Fallback: try treating it as a full tag name (e.g. Paldark.Pal.Activity.Idle).
	return FGameplayTag::RequestGameplayTag(FName(*Arg), /*ErrorIfNotFound=*/false);
}

static void HandlePalCurrentActivityCommand(const TArray<FString>& Args, UWorld* World)
{
	if (World == nullptr)
	{
		UE_LOG(LogPaldarkPal, Warning, TEXT("[Paldark.Pal.CurrentActivity] no world."));
		return;
	}

	const int32 Count = IteratePalsInWorld(World, [](APaldarkPalCharacter* Pal)
	{
		const UPaldarkPalActivityComponent* Activity = Pal->GetActivitySlot();
		const UPaldarkBaseActivity* Current = Activity != nullptr ? Activity->GetCurrentActivity() : nullptr;
		UE_LOG(
			LogPaldarkPal,
			Log,
			TEXT("[Paldark.Pal.CurrentActivity] pal=%s activity=%s tag=%s candidates=%d"),
			*Pal->GetName(),
			Current != nullptr ? *Current->GetClass()->GetName() : TEXT("<none>"),
			Current != nullptr ? *Current->ActivityTag.ToString() : TEXT("<none>"),
			Activity != nullptr ? Activity->GetActivityInstances().Num() : 0);
	});

	if (Count == 0)
	{
		UE_LOG(LogPaldarkPal, Warning, TEXT("[Paldark.Pal.CurrentActivity] no APaldarkPalCharacter in current world."));
	}
}

static void HandlePalSetActivityCommand(const TArray<FString>& Args, UWorld* World)
{
	if (World == nullptr)
	{
		UE_LOG(LogPaldarkPal, Warning, TEXT("[Paldark.Pal.SetActivity] no world."));
		return;
	}
	if (World->GetNetMode() == NM_Client)
	{
		UE_LOG(LogPaldarkPal, Warning, TEXT("[Paldark.Pal.SetActivity] FSM lives on authority; run on server/PIE."));
		return;
	}
	if (!Args.IsValidIndex(0))
	{
		UE_LOG(LogPaldarkPal, Warning, TEXT("[Paldark.Pal.SetActivity] usage: Paldark.Pal.SetActivity <Idle|Follow|Investigate>"));
		return;
	}

	const FGameplayTag TargetTag = ResolveActivityTagArg(Args[0]);
	if (!TargetTag.IsValid())
	{
		UE_LOG(LogPaldarkPal, Warning, TEXT("[Paldark.Pal.SetActivity] unknown activity arg '%s' (expected Idle|Follow|Investigate)."), *Args[0]);
		return;
	}

	int32 Switched = 0;
	const int32 Count = IteratePalsInWorld(World, [&Switched, &TargetTag](APaldarkPalCharacter* Pal)
	{
		UPaldarkPalActivityComponent* Activity = Pal->GetActivitySlot();
		if (Activity != nullptr && Activity->SetCurrentActivityByTag(TargetTag))
		{
			++Switched;
		}
	});

	UE_LOG(LogPaldarkPal, Log, TEXT("[Paldark.Pal.SetActivity] tag=%s pals=%d switched=%d"),
		*TargetTag.ToString(), Count, Switched);
}

static void HandlePalPingCommand(const TArray<FString>& Args, UWorld* World)
{
	if (World == nullptr)
	{
		UE_LOG(LogPaldarkPal, Warning, TEXT("[Paldark.Pal.Ping] no world."));
		return;
	}
	if (World->GetNetMode() == NM_Client)
	{
		UE_LOG(LogPaldarkPal, Warning, TEXT("[Paldark.Pal.Ping] FSM lives on authority; run on server/PIE."));
		return;
	}

	// Resolve target location: explicit X Y Z args, or fall back to local player location.
	FVector PingLocation = FVector::ZeroVector;
	if (Args.Num() >= 3)
	{
		PingLocation = FVector(
			FCString::Atof(*Args[0]),
			FCString::Atof(*Args[1]),
			FCString::Atof(*Args[2]));
	}
	else
	{
		const APlayerController* LocalPC = World->GetFirstPlayerController();
		const APawn* LocalPawn = LocalPC != nullptr ? LocalPC->GetPawn() : nullptr;
		if (LocalPawn == nullptr)
		{
			UE_LOG(LogPaldarkPal, Warning, TEXT("[Paldark.Pal.Ping] no local player pawn and no explicit X Y Z args."));
			return;
		}
		// Pick a point a few metres in front of the player so the Pal has somewhere to go.
		const FVector Forward = LocalPawn->GetActorRotation().Vector();
		PingLocation = LocalPawn->GetActorLocation() + Forward * 600.0f;
	}

	int32 Pinged = 0;
	const int32 Count = IteratePalsInWorld(World, [&Pinged, &PingLocation](APaldarkPalCharacter* Pal)
	{
		if (UPaldarkPalActivityComponent* Activity = Pal->GetActivitySlot())
		{
			Activity->RequestInvestigate(PingLocation);
			++Pinged;
		}
	});

	UE_LOG(LogPaldarkPal, Log, TEXT("[Paldark.Pal.Ping] location=%s pals=%d pinged=%d"),
		*PingLocation.ToCompactString(), Count, Pinged);

	if (GEngine != nullptr && Pinged > 0)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			5.0f,
			FColor::Magenta,
			FString::Printf(TEXT("Pal ping: %s (%d pals)"), *PingLocation.ToCompactString(), Pinged));
	}
}

// W7-8 — GAS debug helpers.
// `Paldark.Gas.DumpAttributes` walks every Actor in the world that implements
// IAbilitySystemInterface, finds the UPaldarkAttributeSet (if present), and
// logs Health/MaxHealth, Stamina/MaxStamina, MoveSpeed. Authority-agnostic —
// safe on clients (values reflect what's replicated locally).
static void HandleGasDumpAttributesCommand(const TArray<FString>& /*Args*/, UWorld* World)
{
	if (World == nullptr)
	{
		UE_LOG(LogPaldarkGAS, Warning, TEXT("[Paldark.Gas.DumpAttributes] no world."));
		return;
	}

	int32 Inspected = 0;
	int32 WithAttributeSet = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor == nullptr)
		{
			continue;
		}
		const IAbilitySystemInterface* AsiActor = Cast<IAbilitySystemInterface>(Actor);
		// PlayerStates aren't found via the actor iterator on the world, so we
		// fall back to inspecting the controller / pawn's PlayerState below.
		UAbilitySystemComponent* ASC = AsiActor != nullptr ? AsiActor->GetAbilitySystemComponent() : nullptr;

		if (ASC == nullptr)
		{
			// Try the standard "pawn forwards to its PlayerState" pattern.
			if (const APawn* AsPawn = Cast<APawn>(Actor))
			{
				if (const APlayerState* PS = AsPawn->GetPlayerState())
				{
					if (const IAbilitySystemInterface* AsiPs = Cast<IAbilitySystemInterface>(PS))
					{
						ASC = AsiPs->GetAbilitySystemComponent();
					}
				}
			}
		}
		if (ASC == nullptr)
		{
			continue;
		}

		++Inspected;
		const UPaldarkAttributeSet* Attribs = Cast<UPaldarkAttributeSet>(
			ASC->GetAttributeSet(UPaldarkAttributeSet::StaticClass()));
		if (Attribs == nullptr)
		{
			UE_LOG(LogPaldarkGAS, Log,
				TEXT("[Paldark.Gas.DumpAttributes] %s ASC=%s (no UPaldarkAttributeSet)"),
				*Actor->GetName(), *ASC->GetName());
			continue;
		}

		++WithAttributeSet;
		UE_LOG(LogPaldarkGAS, Log,
			TEXT("[Paldark.Gas.DumpAttributes] %s Health=%.1f/%.1f Stamina=%.1f/%.1f MoveSpeed=%.1f Armor=%.1f"),
			*Actor->GetName(),
			Attribs->GetHealth(),     Attribs->GetMaxHealth(),
			Attribs->GetStamina(),    Attribs->GetMaxStamina(),
			Attribs->GetMoveSpeed(),
			Attribs->GetArmor());
	}

	UE_LOG(LogPaldarkGAS, Log,
		TEXT("[Paldark.Gas.DumpAttributes] actors_with_ASC=%d with_attribute_set=%d"),
		Inspected, WithAttributeSet);
}

// `Paldark.Gas.Damage <Amount>` — applies a one-shot ModifyAttribute on the
// local player's Health. W7-8 only ships the dynamic-spec path because the
// designer-authored GE_Damage_Standard + SetByCaller magnitude land in W9-10.
// This keeps the sandbox loop closed (sprint costs stamina; damage drops
// health) without forcing a designer to author a Blueprint GE first.
static void HandleGasDamageCommand(const TArray<FString>& Args, UWorld* World)
{
	if (World == nullptr)
	{
		UE_LOG(LogPaldarkGAS, Warning, TEXT("[Paldark.Gas.Damage] no world."));
		return;
	}
	const float Amount = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 10.0f;
	if (Amount <= 0.0f)
	{
		UE_LOG(LogPaldarkGAS, Warning,
			TEXT("[Paldark.Gas.Damage] amount must be > 0 (got %.2f)"), Amount);
		return;
	}

	const APlayerController* LocalPC = World->GetFirstPlayerController();
	const APawn* LocalPawn = LocalPC != nullptr ? LocalPC->GetPawn() : nullptr;
	APlayerState* LocalPS = LocalPC != nullptr ? LocalPC->PlayerState : nullptr;
	if (LocalPawn == nullptr || LocalPS == nullptr)
	{
		UE_LOG(LogPaldarkGAS, Warning,
			TEXT("[Paldark.Gas.Damage] no local player pawn / PlayerState."));
		return;
	}
	if (!LocalPawn->HasAuthority())
	{
		UE_LOG(LogPaldarkGAS, Warning,
			TEXT("[Paldark.Gas.Damage] run on server / standalone (authority required to mutate Health)."));
		return;
	}

	const IAbilitySystemInterface* AsiPs = Cast<IAbilitySystemInterface>(LocalPS);
	UAbilitySystemComponent* ASC = AsiPs != nullptr ? AsiPs->GetAbilitySystemComponent() : nullptr;
	if (ASC == nullptr)
	{
		UE_LOG(LogPaldarkGAS, Warning,
			TEXT("[Paldark.Gas.Damage] local PlayerState %s has no ASC."), *LocalPS->GetName());
		return;
	}
	UPaldarkAttributeSet* Attribs = const_cast<UPaldarkAttributeSet*>(
		Cast<UPaldarkAttributeSet>(ASC->GetAttributeSet(UPaldarkAttributeSet::StaticClass())));
	if (Attribs == nullptr)
	{
		UE_LOG(LogPaldarkGAS, Warning,
			TEXT("[Paldark.Gas.Damage] no UPaldarkAttributeSet on %s ASC."), *LocalPS->GetName());
		return;
	}

	// Apply via ApplyModToAttribute so PreAttributeChange's clamp runs (matches
	// the path a real damage GE would take). The W9-10 follow-up replaces this
	// with `ApplyGameplayEffectToTarget(GE_Damage_Standard, SetByCaller=...)`.
	const float OldHealth = Attribs->GetHealth();
	ASC->ApplyModToAttribute(
		UPaldarkAttributeSet::GetHealthAttribute(),
		EGameplayModOp::Additive,
		-Amount);
	const float NewHealth = Attribs->GetHealth();

	UE_LOG(LogPaldarkGAS, Log,
		TEXT("[Paldark.Gas.Damage] %s Health %.1f -> %.1f (applied=%.1f)"),
		*LocalPS->GetName(), OldHealth, NewHealth, OldHealth - NewHealth);

	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			5.0f,
			FColor::Red,
			FString::Printf(TEXT("Damage %.1f: Health %.1f -> %.1f"),
				Amount, OldHealth, NewHealth));
	}
}

// `Paldark.Combat.SpawnDummy [Distance] [PitchOffset]` — W9-10 helper. Spawns
// one APaldarkDummyTarget at `Distance` cm in front of the local player so the
// hitscan ability has something to chew on. Authority-only; bails with a clear
// log on client-only worlds. Default distance = 500cm (5m) matches the W9-10
// sandbox scale.
static void HandleCombatSpawnDummyCommand(const TArray<FString>& Args, UWorld* World)
{
	if (World == nullptr)
	{
		UE_LOG(LogPaldarkGAS, Warning, TEXT("[Paldark.Combat.SpawnDummy] no world."));
		return;
	}
	if (World->GetNetMode() == NM_Client)
	{
		UE_LOG(LogPaldarkGAS, Warning, TEXT("[Paldark.Combat.SpawnDummy] cannot spawn on a client world."));
		return;
	}

	APlayerController* LocalPC = World->GetFirstPlayerController();
	const APawn* LocalPawn = LocalPC != nullptr ? LocalPC->GetPawn() : nullptr;
	if (LocalPawn == nullptr)
	{
		UE_LOG(LogPaldarkGAS, Warning, TEXT("[Paldark.Combat.SpawnDummy] no local player pawn (PIE/server must have one)."));
		return;
	}

	const float Distance = Args.IsValidIndex(0) ? FCString::Atof(*Args[0]) : 500.0f;

	const FVector  Origin = LocalPawn->GetActorLocation();
	const FRotator Yaw    = FRotator(0.0f, LocalPawn->GetActorRotation().Yaw, 0.0f);
	const FVector  SpawnLoc = Origin + Yaw.Vector() * Distance;
	// Face the player so a head-bone hit is reachable from the camera.
	const FRotator SpawnRot = FRotator(0.0f, Yaw.Yaw + 180.0f, 0.0f);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	APaldarkDummyTarget* Dummy = World->SpawnActor<APaldarkDummyTarget>(
		APaldarkDummyTarget::StaticClass(), SpawnLoc, SpawnRot, Params);

	if (Dummy == nullptr)
	{
		UE_LOG(LogPaldarkGAS, Warning, TEXT("[Paldark.Combat.SpawnDummy] SpawnActor returned null (collision?)."));
		return;
	}

	UE_LOG(LogPaldarkGAS, Log,
		TEXT("[Paldark.Combat.SpawnDummy] spawned %s at %s (distance=%.1fcm)"),
		*Dummy->GetName(), *SpawnLoc.ToCompactString(), Distance);

	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			5.0f,
			FColor::Yellow,
			FString::Printf(TEXT("Dummy spawned: %s"), *Dummy->GetName()));
	}
}

// `Paldark.Combat.Fire` — W9-10 helper. Activates the Fire ability on the local
// player's ASC via the same tag-keyed path the input binding uses. Useful when
// iterating on the trace / damage formula without re-binding keyboard input.
static void HandleCombatFireCommand(const TArray<FString>& /*Args*/, UWorld* World)
{
	if (World == nullptr)
	{
		UE_LOG(LogPaldarkGAS, Warning, TEXT("[Paldark.Combat.Fire] no world."));
		return;
	}

	APlayerController* LocalPC = World->GetFirstPlayerController();
	APlayerState*      LocalPS = LocalPC != nullptr ? LocalPC->PlayerState : nullptr;
	if (LocalPS == nullptr)
	{
		UE_LOG(LogPaldarkGAS, Warning, TEXT("[Paldark.Combat.Fire] no local PlayerState."));
		return;
	}

	const IAbilitySystemInterface* AsiPs = Cast<IAbilitySystemInterface>(LocalPS);
	UPaldarkAbilitySystemComponent* ASC = AsiPs != nullptr
		? Cast<UPaldarkAbilitySystemComponent>(AsiPs->GetAbilitySystemComponent())
		: nullptr;
	if (ASC == nullptr)
	{
		UE_LOG(LogPaldarkGAS, Warning,
			TEXT("[Paldark.Combat.Fire] local PlayerState %s has no UPaldarkAbilitySystemComponent."),
			*LocalPS->GetName());
		return;
	}

	const bool bActivated = ASC->TryActivateAbilityByActivationTag(
		PaldarkGameplayTags::TAG_Paldark_Ability_Fire);

	UE_LOG(LogPaldarkGAS, Log,
		TEXT("[Paldark.Combat.Fire] %s -> activated=%d"),
		*LocalPS->GetName(),
		bActivated ? 1 : 0);
}

// W11-12 — Resolve the local player's inventory component. Returns nullptr +
// logs a warning when something upstream is missing (no world, no pawn, no
// component). Centralising the lookup keeps all four inventory commands
// short and consistent.
static UPaldarkPlayerInventoryComponent* FindLocalPlayerInventory(UWorld* World)
{
	if (World == nullptr)
	{
		UE_LOG(LogPaldark, Warning, TEXT("[Paldark.Inventory] no world."));
		return nullptr;
	}
	APlayerController* LocalPC = World->GetFirstPlayerController();
	APawn* LocalPawn = LocalPC != nullptr ? LocalPC->GetPawn() : nullptr;
	if (LocalPawn == nullptr)
	{
		UE_LOG(LogPaldark, Warning, TEXT("[Paldark.Inventory] no local pawn possessed."));
		return nullptr;
	}
	UPaldarkPlayerInventoryComponent* Inv = LocalPawn->FindComponentByClass<UPaldarkPlayerInventoryComponent>();
	if (Inv == nullptr)
	{
		UE_LOG(LogPaldark, Warning,
			TEXT("[Paldark.Inventory] pawn %s has no UPaldarkPlayerInventoryComponent."),
			*LocalPawn->GetName());
	}
	return Inv;
}

// W11-12 — Scan every registered PaldarkItem primary asset, load it, and
// return the first one whose ItemTag matches `ItemTag`. Synchronous load is
// acceptable here (debug console only). When the user typed a partial tag
// like `Pistol`, fall back to the fully qualified Paldark.Item.<x>.
static UPaldarkItemDefinition* ResolveItemDefByTagArg(const FString& TagArg)
{
	FString TagString = TagArg;
	TagString.TrimStartAndEndInline();
	if (TagString.IsEmpty())
	{
		return nullptr;
	}
	if (!TagString.Contains(TEXT(".")))
	{
		TagString = FString::Printf(TEXT("Paldark.Item.%s"), *TagString);
	}
	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), /*ErrorIfNotFound*/ false);
	if (!Tag.IsValid())
	{
		UE_LOG(LogPaldark, Warning, TEXT("[Paldark.Inventory] unknown item tag '%s'."), *TagString);
		return nullptr;
	}

	UAssetManager& AssetManager = UAssetManager::Get();
	TArray<FPrimaryAssetId> AssetIds;
	AssetManager.GetPrimaryAssetIdList(FPrimaryAssetType(TEXT("PaldarkItem")), AssetIds);

	for (const FPrimaryAssetId& AssetId : AssetIds)
	{
		const FSoftObjectPath Path = AssetManager.GetPrimaryAssetPath(AssetId);
		if (Path.IsNull())
		{
			continue;
		}
		UObject* Loaded = Path.TryLoad();
		UPaldarkItemDefinition* ItemDef = Cast<UPaldarkItemDefinition>(Loaded);
		if (ItemDef != nullptr && ItemDef->ItemTag == Tag)
		{
			return ItemDef;
		}
	}
	UE_LOG(LogPaldark, Warning,
		TEXT("[Paldark.Inventory] no PaldarkItem asset registered for tag %s. Author DA_Item_<...> under /Game/Paldark/Items and set ItemTag."),
		*Tag.ToString());
	return nullptr;
}

static void HandleInventoryListCommand(const TArray<FString>& /*Args*/, UWorld* World)
{
	if (UPaldarkPlayerInventoryComponent* Inv = FindLocalPlayerInventory(World))
	{
		Inv->DumpToLog();
	}
}

static void HandleInventoryAddCommand(const TArray<FString>& Args, UWorld* World)
{
	if (Args.Num() < 1)
	{
		UE_LOG(LogPaldark, Warning, TEXT("[Paldark.Inventory.Add] usage: Paldark.Inventory.Add <Paldark.Item.X> [Count=1]"));
		return;
	}
	UPaldarkPlayerInventoryComponent* Inv = FindLocalPlayerInventory(World);
	if (Inv == nullptr)
	{
		return;
	}
	UPaldarkItemDefinition* ItemDef = ResolveItemDefByTagArg(Args[0]);
	if (ItemDef == nullptr)
	{
		return;
	}
	const int32 Count = Args.Num() >= 2 ? FMath::Max(FCString::Atoi(*Args[1]), 1) : 1;

	AActor* Owner = Inv->GetOwner();
	if (Owner == nullptr || !Owner->HasAuthority())
	{
		UE_LOG(LogPaldark, Warning,
			TEXT("[Paldark.Inventory.Add] local player has no authority (standalone/server only). Run via dedicated server console or PIE-as-listen."));
		return;
	}
	const int32 Added = Inv->AddItem(TSoftObjectPtr<UPaldarkItemDefinition>(ItemDef), Count);
	UE_LOG(LogPaldark, Log,
		TEXT("[Paldark.Inventory.Add] requested=%d added=%d item=%s"),
		Count, Added, *ItemDef->GetDebugLabel());
}

static void HandleInventoryRemoveCommand(const TArray<FString>& Args, UWorld* World)
{
	if (Args.Num() < 1)
	{
		UE_LOG(LogPaldark, Warning, TEXT("[Paldark.Inventory.Remove] usage: Paldark.Inventory.Remove <Paldark.Item.X> [Count=1]"));
		return;
	}
	UPaldarkPlayerInventoryComponent* Inv = FindLocalPlayerInventory(World);
	if (Inv == nullptr)
	{
		return;
	}

	FString TagString = Args[0];
	TagString.TrimStartAndEndInline();
	if (!TagString.Contains(TEXT(".")))
	{
		TagString = FString::Printf(TEXT("Paldark.Item.%s"), *TagString);
	}
	const FGameplayTag ItemTag = FGameplayTag::RequestGameplayTag(FName(*TagString), /*ErrorIfNotFound*/ false);
	if (!ItemTag.IsValid())
	{
		UE_LOG(LogPaldark, Warning, TEXT("[Paldark.Inventory.Remove] unknown item tag '%s'."), *TagString);
		return;
	}
	const int32 Count = Args.Num() >= 2 ? FMath::Max(FCString::Atoi(*Args[1]), 1) : 1;

	AActor* Owner = Inv->GetOwner();
	if (Owner == nullptr || !Owner->HasAuthority())
	{
		UE_LOG(LogPaldark, Warning, TEXT("[Paldark.Inventory.Remove] local player has no authority."));
		return;
	}
	const int32 Removed = Inv->RemoveItemByTag(ItemTag, Count);
	UE_LOG(LogPaldark, Log,
		TEXT("[Paldark.Inventory.Remove] requested=%d removed=%d tag=%s"),
		Count, Removed, *ItemTag.ToString());
}

static void HandleInventoryDropCommand(const TArray<FString>& /*Args*/, UWorld* World)
{
	UPaldarkPlayerInventoryComponent* Inv = FindLocalPlayerInventory(World);
	if (Inv == nullptr)
	{
		return;
	}
	AActor* Owner = Inv->GetOwner();
	if (Owner == nullptr || !Owner->HasAuthority())
	{
		UE_LOG(LogPaldark, Warning, TEXT("[Paldark.Inventory.Drop] local player has no authority."));
		return;
	}
	Inv->DropAllItems();
}

// W37-38 — Adds 1 of every PaldarkItem primary asset known to the
// AssetManager. Synchronous load + add via the standard authority API; cheap
// in the editor since the asset list is < 64 items.
static void HandleInventoryGiveAllCommand(const TArray<FString>& /*Args*/, UWorld* World)
{
	UPaldarkPlayerInventoryComponent* Inv = FindLocalPlayerInventory(World);
	if (Inv == nullptr)
	{
		return;
	}
	AActor* Owner = Inv->GetOwner();
	if (Owner == nullptr || !Owner->HasAuthority())
	{
		UE_LOG(LogPaldark, Warning,
			TEXT("[Paldark.Inventory.GiveAll] local player has no authority (standalone/server only)."));
		return;
	}

	UAssetManager& AssetManager = UAssetManager::Get();
	TArray<FPrimaryAssetId> AssetIds;
	AssetManager.GetPrimaryAssetIdList(FPrimaryAssetType(TEXT("PaldarkItem")), AssetIds);
	if (AssetIds.Num() == 0)
	{
		UE_LOG(LogPaldark, Warning,
			TEXT("[Paldark.Inventory.GiveAll] AssetManager registered 0 PaldarkItem assets. Author DA_Item_<...> under /Game/Paldark/Inventory."));
		return;
	}

	int32 Granted = 0;
	for (const FPrimaryAssetId& AssetId : AssetIds)
	{
		const FSoftObjectPath Path = AssetManager.GetPrimaryAssetPath(AssetId);
		if (Path.IsNull()) { continue; }
		UObject* Loaded = Path.TryLoad();
		UPaldarkItemDefinition* ItemDef = Cast<UPaldarkItemDefinition>(Loaded);
		if (ItemDef == nullptr) { continue; }
		const int32 Added = Inv->AddItem(TSoftObjectPtr<UPaldarkItemDefinition>(ItemDef), /*RequestedCount*/ 1);
		Granted += Added;
	}
	UE_LOG(LogPaldark, Log,
		TEXT("[Paldark.Inventory.GiveAll] registered=%d granted=%d (weight %.1f/%.1f kg)."),
		AssetIds.Num(),
		Granted,
		Inv->GetCurrentWeightKg(),
		Inv->GetMaxWeightKg());
}

// W37-38 — Looks up the backpack item def by Paldark.Item.Backpack.<tier> and
// equips it via the inventory component. Tier-arg accepted forms: `T1`/`T2`/`T3`
// (case-insensitive) or `clear` to clear the slot.
static void HandleInventoryEquipBackpackCommand(const TArray<FString>& Args, UWorld* World)
{
	UPaldarkPlayerInventoryComponent* Inv = FindLocalPlayerInventory(World);
	if (Inv == nullptr)
	{
		return;
	}
	AActor* Owner = Inv->GetOwner();
	if (Owner == nullptr || !Owner->HasAuthority())
	{
		UE_LOG(LogPaldark, Warning,
			TEXT("[Paldark.Inventory.EquipBackpack] local player has no authority."));
		return;
	}
	if (Args.Num() < 1)
	{
		UE_LOG(LogPaldark, Warning,
			TEXT("[Paldark.Inventory.EquipBackpack] usage: Paldark.Inventory.EquipBackpack <T1|T2|T3|clear>"));
		return;
	}

	FString TierArg = Args[0];
	TierArg.TrimStartAndEndInline();
	if (TierArg.Equals(TEXT("clear"), ESearchCase::IgnoreCase))
	{
		const bool bChanged = Inv->SetEquippedBackpack(TSoftObjectPtr<UPaldarkItemDefinition>());
		UE_LOG(LogPaldark, Log,
			TEXT("[Paldark.Inventory.EquipBackpack] clear -> changed=%d cap=%.1f kg"),
			bChanged ? 1 : 0,
			Inv->GetMaxWeightKg());
		return;
	}

	const FString TagString = FString::Printf(TEXT("Paldark.Item.Backpack.%s"), *TierArg.ToUpper());
	const FGameplayTag BackpackTag = FGameplayTag::RequestGameplayTag(FName(*TagString), /*ErrorIfNotFound*/ false);
	if (!BackpackTag.IsValid())
	{
		UE_LOG(LogPaldark, Warning,
			TEXT("[Paldark.Inventory.EquipBackpack] unknown tier tag '%s'."),
			*TagString);
		return;
	}

	UPaldarkItemDefinition* BackpackDef = ResolveItemDefByTagArg(TagString);
	if (BackpackDef == nullptr)
	{
		// ResolveItemDefByTagArg already logged the diagnostic.
		return;
	}
	const bool bChanged = Inv->SetEquippedBackpack(TSoftObjectPtr<UPaldarkItemDefinition>(BackpackDef));
	UE_LOG(LogPaldark, Log,
		TEXT("[Paldark.Inventory.EquipBackpack] %s -> changed=%d cap=%.1f kg"),
		*BackpackDef->GetDebugLabel(),
		bChanged ? 1 : 0,
		Inv->GetMaxWeightKg());
}

// W37-38 — Spawns a loot bag from the local player's inventory without
// killing the pawn. Useful for verifying the drop-on-death pipeline (bag
// spawn + content init + inventory clear) in isolation from the damage
// chain.
static void HandleInventoryTestDeathDropCommand(const TArray<FString>& /*Args*/, UWorld* World)
{
	if (World == nullptr)
	{
		UE_LOG(LogPaldark, Warning, TEXT("[Paldark.Inventory.TestDeathDrop] no world."));
		return;
	}
	APlayerController* LocalPC = World->GetFirstPlayerController();
	APaldarkCharacter* PaldarkPawn = LocalPC != nullptr ? Cast<APaldarkCharacter>(LocalPC->GetPawn()) : nullptr;
	if (PaldarkPawn == nullptr)
	{
		UE_LOG(LogPaldark, Warning,
			TEXT("[Paldark.Inventory.TestDeathDrop] no local APaldarkCharacter possessed."));
		return;
	}
	if (!PaldarkPawn->HasAuthority())
	{
		UE_LOG(LogPaldark, Warning,
			TEXT("[Paldark.Inventory.TestDeathDrop] local pawn has no authority (standalone/server only)."));
		return;
	}
	PaldarkPawn->SpawnDeathLootBagFromInventory();
}

static void HandleInventoryDumpCompositeCommand(const TArray<FString>& /*Args*/, UWorld* World)
{
	if (UPaldarkPlayerInventoryComponent* Inv = FindLocalPlayerInventory(World))
	{
		Inv->DumpCompositeToLog();
	}
}

static FPrimaryAssetId ResolvePalDefinitionIdFromArg(const FString& Arg)
{
	// Two accepted forms (mirrors the experience-id parser used by
	// APaldarkGameModeBase::RequestExperienceLoad):
	//   * "PaldarkPalDefinition:DA_PalDef_Direhound" — full id literal.
	//   * "DA_PalDef_Direhound"                       — bare row name (implied type).
	if (Arg.Contains(TEXT(":")))
	{
		return FPrimaryAssetId::FromString(Arg);
	}
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("PaldarkPalDefinition")), FName(*Arg));
}

static void HandlePalSpawnFromDefinitionCommand(const TArray<FString>& Args, UWorld* World)
{
	if (World == nullptr)
	{
		UE_LOG(LogPaldarkPal, Warning, TEXT("[Paldark.Pal.SpawnFromDefinition] no world."));
		return;
	}
	if (World->GetNetMode() == NM_Client)
	{
		UE_LOG(LogPaldarkPal, Warning, TEXT("[Paldark.Pal.SpawnFromDefinition] cannot spawn on a client world."));
		return;
	}
	if (!Args.IsValidIndex(0))
	{
		UE_LOG(LogPaldarkPal, Warning,
			TEXT("[Paldark.Pal.SpawnFromDefinition] usage: Paldark.Pal.SpawnFromDefinition <DefId> [X Y Z]"));
		return;
	}

	const FPrimaryAssetId DefId = ResolvePalDefinitionIdFromArg(Args[0]);
	if (!DefId.IsValid())
	{
		UE_LOG(LogPaldarkPal, Warning,
			TEXT("[Paldark.Pal.SpawnFromDefinition] could not resolve PaldarkPalDefinition id from %s."),
			*Args[0]);
		return;
	}

	FVector SpawnLoc;
	const bool bExplicitLoc = Args.IsValidIndex(3);
	if (bExplicitLoc)
	{
		SpawnLoc = FVector(
			FCString::Atof(*Args[1]),
			FCString::Atof(*Args[2]),
			FCString::Atof(*Args[3]));
	}
	else
	{
		APlayerController* PC = World->GetFirstPlayerController();
		APawn* Pawn = PC != nullptr ? PC->GetPawn() : nullptr;
		const FVector Loc = Pawn != nullptr ? Pawn->GetActorLocation() : FVector::ZeroVector;
		const FRotator Rot = Pawn != nullptr ? Pawn->GetActorRotation() : FRotator::ZeroRotator;
		SpawnLoc = Loc + Rot.RotateVector(FVector(400.0f, 0.0f, 0.0f));
	}

	UPaldarkPalSpawnSubsystem* Subsystem = UPaldarkPalSpawnSubsystem::Get(World);
	if (Subsystem == nullptr)
	{
		UE_LOG(LogPaldarkPal, Warning,
			TEXT("[Paldark.Pal.SpawnFromDefinition] UPaldarkPalSpawnSubsystem missing on world %s."),
			*World->GetName());
		return;
	}

	const FTransform Where(FRotator::ZeroRotator, SpawnLoc, FVector::OneVector);
	Subsystem->SpawnPalAsync(
		DefId,
		Where,
		FOnPalSpawnedAsync::CreateLambda(
			[](FPrimaryAssetId InId, APaldarkPalCharacter* Spawned)
			{
				UE_LOG(LogPaldarkPal, Log,
					TEXT("[Paldark.Pal.SpawnFromDefinition] complete — id=%s spawned=%s"),
					*InId.ToString(),
					Spawned != nullptr ? *Spawned->GetName() : TEXT("<null>"));
				if (GEngine != nullptr)
				{
					GEngine->AddOnScreenDebugMessage(
						-1, 5.0f,
						Spawned != nullptr ? FColor::Yellow : FColor::Red,
						FString::Printf(TEXT("Pal spawn: id=%s pawn=%s"),
							*InId.ToString(),
							Spawned != nullptr ? *Spawned->GetName() : TEXT("<null>")));
				}
			}));

	UE_LOG(LogPaldarkPal, Log,
		TEXT("[Paldark.Pal.SpawnFromDefinition] queued — id=%s where=%s"),
		*DefId.ToString(), *SpawnLoc.ToCompactString());
}

static void HandlePalDumpDefinitionRegistryCommand(const TArray<FString>& Args, UWorld* World)
{
	if (World == nullptr)
	{
		UE_LOG(LogPaldarkPal, Warning, TEXT("[Paldark.Pal.DumpDefinitionRegistry] no world."));
		return;
	}
	UPaldarkPalSpawnSubsystem* Subsystem = UPaldarkPalSpawnSubsystem::Get(World);
	if (Subsystem == nullptr)
	{
		UE_LOG(LogPaldarkPal, Warning,
			TEXT("[Paldark.Pal.DumpDefinitionRegistry] UPaldarkPalSpawnSubsystem missing on world %s."),
			*World->GetName());
		return;
	}

	const TArray<FPrimaryAssetId> Ids = Subsystem->GetTrackedDefinitionIds();
	UE_LOG(LogPaldarkPal, Log,
		TEXT("[Paldark.Pal.DumpDefinitionRegistry] tracked=%d"), Ids.Num());
	for (const FPrimaryAssetId& Id : Ids)
	{
		const UPaldarkPalDefinition* Warm = Subsystem->FindWarmedDefinition(Id);
		const bool bPreWarmed = Subsystem->IsPreWarmed(Id);
		UE_LOG(LogPaldarkPal, Log,
			TEXT("  id=%s state=%s species=%s class=%s mesh=%s anim=%s"),
			*Id.ToString(),
			bPreWarmed ? TEXT("Resolved") : TEXT("InFlight"),
			Warm != nullptr ? *Warm->SpeciesTag.ToString()   : TEXT("?"),
			Warm != nullptr ? *Warm->PalClass.ToString()     : TEXT("?"),
			Warm != nullptr ? *Warm->SkeletalMesh.ToString() : TEXT("?"),
			Warm != nullptr ? *Warm->AnimInstanceClass.ToString() : TEXT("?"));
	}
}

// ------------------------------------------------------------------
// W44-45 — Stable + Marketplace UI console commands.
// ------------------------------------------------------------------
//
// Eight commands that exercise the W44-45 controller-component plumbing
// without requiring the designer-authored UMG layout. They all resolve the
// local Paldark PlayerController -> its `UPaldarkStableController` /
// `UPaldarkMarketplaceController` (W44-45 default subobjects), then call
// the same `RequestX` entry points the widget would use. Result outcomes
// surface via `LogPaldarkUI` (registered W44-45) when the controller's
// delegate fires.

namespace PaldarkHubConsole
{
	static APaldarkPlayerController* FindLocalPaldarkController(UWorld* World)
	{
		if (World == nullptr)
		{
			return nullptr;
		}
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* PC = It->Get())
			{
				if (PC->IsLocalController())
				{
					return Cast<APaldarkPlayerController>(PC);
				}
			}
		}
		return nullptr;
	}

	static UPaldarkPlayerPalRosterComponent* ResolveRoster(APaldarkPlayerController* PC)
	{
		APawn* Pawn = PC != nullptr ? PC->GetPawn() : nullptr;
		return Pawn != nullptr ? Pawn->FindComponentByClass<UPaldarkPlayerPalRosterComponent>() : nullptr;
	}

	static UPaldarkPalDepositComponent* ResolveDeposit(APaldarkPlayerController* PC)
	{
		if (PC == nullptr) { return nullptr; }
		APaldarkPlayerState* PS = PC->GetPlayerState<APaldarkPlayerState>();
		return PS != nullptr ? PS->GetPalDepositComponent() : nullptr;
	}

	// Finds the closest Marketplace kiosk in the world by linear scan. Used
	// for the console-driven Buy/Sell path so the designer can exercise the
	// flow without standing inside the kiosk's interact radius.
	static APaldarkMarketplaceKiosk* FindFirstKiosk(UWorld* World, const FString& OptionalName)
	{
		if (World == nullptr) { return nullptr; }
		for (TActorIterator<APaldarkMarketplaceKiosk> It(World); It; ++It)
		{
			APaldarkMarketplaceKiosk* Kiosk = *It;
			if (Kiosk == nullptr) { continue; }
			if (OptionalName.IsEmpty() || Kiosk->GetName().Contains(OptionalName))
			{
				return Kiosk;
			}
		}
		return nullptr;
	}

	// Synchronously walk the inventory's flat + nested entries and count the
	// Credits tag (`Paldark.Item.Resource.Currency.Credits`). Mirror of the
	// helper baked into the marketplace controller, kept inline here so the
	// console command stays self-contained.
	static int32 CountCredits(UPaldarkPlayerInventoryComponent* Inv)
	{
		if (Inv == nullptr) { return 0; }
		const FGameplayTag CreditsTag = PaldarkGameplayTags::TAG_Paldark_Item_Resource_Currency_Credits;
		int32 Total = 0;
		for (const FPaldarkInventoryEntry& Entry : Inv->GetEntries())
		{
			const UPaldarkItemDefinition* ItemDef = Entry.ItemDef.IsValid() ? Entry.ItemDef.Get() : nullptr;
			if (ItemDef != nullptr && ItemDef->ItemTag == CreditsTag)
			{
				Total += Entry.StackCount;
			}
			for (const FPaldarkInventoryEntry& Inner : Entry.InnerEntries)
			{
				const UPaldarkItemDefinition* InnerDef = Inner.ItemDef.IsValid() ? Inner.ItemDef.Get() : nullptr;
				if (InnerDef != nullptr && InnerDef->ItemTag == CreditsTag)
				{
					Total += Inner.StackCount;
				}
			}
		}
		return Total;
	}

	static FGameplayTag ResolveItemTagArg(const FString& Arg)
	{
		FString TagString = Arg;
		TagString.TrimStartAndEndInline();
		if (!TagString.Contains(TEXT(".")))
		{
			TagString = FString::Printf(TEXT("Paldark.Item.%s"), *TagString);
		}
		return FGameplayTag::RequestGameplayTag(FName(*TagString), /*ErrorIfNotFound*/ false);
	}
}

static void HandleHubStableListCommand(const TArray<FString>& /*Args*/, UWorld* World)
{
	APaldarkPlayerController* PC = PaldarkHubConsole::FindLocalPaldarkController(World);
	if (PC == nullptr)
	{
		UE_LOG(LogPaldarkUI, Warning, TEXT("[Paldark.Hub.Stable.List] no local Paldark controller."));
		return;
	}
	UPaldarkPlayerPalRosterComponent* Roster = PaldarkHubConsole::ResolveRoster(PC);
	UPaldarkPalDepositComponent*      Deposit = PaldarkHubConsole::ResolveDeposit(PC);
	UE_LOG(LogPaldarkUI, Log,
		TEXT("[Paldark.Hub.Stable.List] roster=%d deposit=%d/%d"),
		Roster != nullptr ? Roster->GetEntries().Num() : -1,
		Deposit != nullptr ? Deposit->GetDepositedEntries().Num() : -1,
		Deposit != nullptr ? Deposit->GetMaxDepositSlots() : -1);
	if (Roster != nullptr) { Roster->DumpToLog(); }
	if (Deposit != nullptr) { Deposit->DumpToLog(); }
}

static void HandleHubStableDepositCommand(const TArray<FString>& Args, UWorld* World)
{
	if (Args.Num() < 1)
	{
		UE_LOG(LogPaldarkUI, Warning, TEXT("[Paldark.Hub.Stable.Deposit] usage: Paldark.Hub.Stable.Deposit <RosterIndex>"));
		return;
	}
	APaldarkPlayerController* PC = PaldarkHubConsole::FindLocalPaldarkController(World);
	UPaldarkStableController* Ctrl = PC != nullptr ? PC->GetStableController() : nullptr;
	if (Ctrl == nullptr)
	{
		UE_LOG(LogPaldarkUI, Warning, TEXT("[Paldark.Hub.Stable.Deposit] no StableController on local PC."));
		return;
	}
	const int32 Index = FCString::Atoi(*Args[0]);
	Ctrl->RequestDeposit(Index);
	UE_LOG(LogPaldarkUI, Log,
		TEXT("[Paldark.Hub.Stable.Deposit] requested roster_index=%d."), Index);
}

static void HandleHubStableWithdrawCommand(const TArray<FString>& Args, UWorld* World)
{
	if (Args.Num() < 1)
	{
		UE_LOG(LogPaldarkUI, Warning, TEXT("[Paldark.Hub.Stable.Withdraw] usage: Paldark.Hub.Stable.Withdraw <DepositIndex>"));
		return;
	}
	APaldarkPlayerController* PC = PaldarkHubConsole::FindLocalPaldarkController(World);
	UPaldarkStableController* Ctrl = PC != nullptr ? PC->GetStableController() : nullptr;
	if (Ctrl == nullptr)
	{
		UE_LOG(LogPaldarkUI, Warning, TEXT("[Paldark.Hub.Stable.Withdraw] no StableController on local PC."));
		return;
	}
	const int32 Index = FCString::Atoi(*Args[0]);
	Ctrl->RequestWithdraw(Index);
	UE_LOG(LogPaldarkUI, Log,
		TEXT("[Paldark.Hub.Stable.Withdraw] requested deposit_index=%d."), Index);
}

static void HandleHubStableHealCommand(const TArray<FString>& Args, UWorld* World)
{
	if (Args.Num() < 1)
	{
		UE_LOG(LogPaldarkUI, Warning, TEXT("[Paldark.Hub.Stable.Heal] usage: Paldark.Hub.Stable.Heal <RosterIndex>"));
		return;
	}
	APaldarkPlayerController* PC = PaldarkHubConsole::FindLocalPaldarkController(World);
	UPaldarkStableController* Ctrl = PC != nullptr ? PC->GetStableController() : nullptr;
	if (Ctrl == nullptr)
	{
		UE_LOG(LogPaldarkUI, Warning, TEXT("[Paldark.Hub.Stable.Heal] no StableController on local PC."));
		return;
	}
	const int32 Index = FCString::Atoi(*Args[0]);
	Ctrl->RequestHeal(Index);
	UE_LOG(LogPaldarkUI, Log,
		TEXT("[Paldark.Hub.Stable.Heal] requested roster_index=%d cost=%d."), Index, Ctrl->GetHealPriceCredits());
}

static void HandleHubMarketCatalogCommand(const TArray<FString>& Args, UWorld* World)
{
	const FString KioskName = Args.Num() > 0 ? Args[0] : FString();
	APaldarkMarketplaceKiosk* Kiosk = PaldarkHubConsole::FindFirstKiosk(World, KioskName);
	if (Kiosk == nullptr)
	{
		UE_LOG(LogPaldarkUI, Warning,
			TEXT("[Paldark.Hub.Market.Catalog] no APaldarkMarketplaceKiosk in world (filter='%s')."), *KioskName);
		return;
	}
	const UPaldarkHubBuildingDefinition* Def = Kiosk->GetBuildingDefinition();
	if (Def == nullptr)
	{
		UE_LOG(LogPaldarkUI, Warning,
			TEXT("[Paldark.Hub.Market.Catalog] kiosk %s has no BuildingDefinition resolved (cold soft ref)."),
			*Kiosk->GetName());
		return;
	}
	UE_LOG(LogPaldarkUI, Log,
		TEXT("[Paldark.Hub.Market.Catalog] kiosk=%s catalog_size=%d"),
		*Kiosk->GetName(), Def->MarketplaceCatalog.Num());
	for (const TSoftObjectPtr<UPaldarkItemDefinition>& SoftItem : Def->MarketplaceCatalog)
	{
		const UPaldarkItemDefinition* ItemDef = SoftItem.IsValid() ? SoftItem.Get() : SoftItem.LoadSynchronous();
		if (ItemDef == nullptr)
		{
			UE_LOG(LogPaldarkUI, Log, TEXT("  <unresolved> %s"), *SoftItem.ToString());
			continue;
		}
		const UPaldarkItemFragment_MarketValue* MV = ItemDef->FindFragmentByClass<UPaldarkItemFragment_MarketValue>();
		UE_LOG(LogPaldarkUI, Log,
			TEXT("  item=%s tag=%s buy=%d sell=%d allow_buy=%s allow_sell=%s"),
			*ItemDef->GetDebugLabel(),
			*ItemDef->ItemTag.ToString(),
			MV != nullptr ? MV->BuyPriceCredits  : -1,
			MV != nullptr ? MV->SellPriceCredits : -1,
			MV != nullptr ? (MV->bAllowBuy  ? TEXT("yes") : TEXT("no")) : TEXT("?"),
			MV != nullptr ? (MV->bAllowSell ? TEXT("yes") : TEXT("no")) : TEXT("?"));
	}
}

static void HandleHubMarketBuyCommand(const TArray<FString>& Args, UWorld* World)
{
	if (Args.Num() < 1)
	{
		UE_LOG(LogPaldarkUI, Warning,
			TEXT("[Paldark.Hub.Market.Buy] usage: Paldark.Hub.Market.Buy <Paldark.Item.X> [Count=1]"));
		return;
	}
	APaldarkPlayerController* PC = PaldarkHubConsole::FindLocalPaldarkController(World);
	UPaldarkMarketplaceController* Ctrl = PC != nullptr ? PC->GetMarketplaceController() : nullptr;
	if (Ctrl == nullptr)
	{
		UE_LOG(LogPaldarkUI, Warning, TEXT("[Paldark.Hub.Market.Buy] no MarketplaceController on local PC."));
		return;
	}
	const FGameplayTag ItemTag = PaldarkHubConsole::ResolveItemTagArg(Args[0]);
	if (!ItemTag.IsValid())
	{
		UE_LOG(LogPaldarkUI, Warning, TEXT("[Paldark.Hub.Market.Buy] unknown item tag '%s'."), *Args[0]);
		return;
	}
	const int32 Count = Args.Num() >= 2 ? FMath::Max(FCString::Atoi(*Args[1]), 1) : 1;
	Ctrl->RequestBuy(ItemTag, Count);
	UE_LOG(LogPaldarkUI, Log,
		TEXT("[Paldark.Hub.Market.Buy] requested tag=%s count=%d."), *ItemTag.ToString(), Count);
}

static void HandleHubMarketSellCommand(const TArray<FString>& Args, UWorld* World)
{
	if (Args.Num() < 1)
	{
		UE_LOG(LogPaldarkUI, Warning,
			TEXT("[Paldark.Hub.Market.Sell] usage: Paldark.Hub.Market.Sell <Paldark.Item.X> [Count=1]"));
		return;
	}
	APaldarkPlayerController* PC = PaldarkHubConsole::FindLocalPaldarkController(World);
	UPaldarkMarketplaceController* Ctrl = PC != nullptr ? PC->GetMarketplaceController() : nullptr;
	if (Ctrl == nullptr)
	{
		UE_LOG(LogPaldarkUI, Warning, TEXT("[Paldark.Hub.Market.Sell] no MarketplaceController on local PC."));
		return;
	}
	const FGameplayTag ItemTag = PaldarkHubConsole::ResolveItemTagArg(Args[0]);
	if (!ItemTag.IsValid())
	{
		UE_LOG(LogPaldarkUI, Warning, TEXT("[Paldark.Hub.Market.Sell] unknown item tag '%s'."), *Args[0]);
		return;
	}
	const int32 Count = Args.Num() >= 2 ? FMath::Max(FCString::Atoi(*Args[1]), 1) : 1;
	Ctrl->RequestSell(ItemTag, Count);
	UE_LOG(LogPaldarkUI, Log,
		TEXT("[Paldark.Hub.Market.Sell] requested tag=%s count=%d."), *ItemTag.ToString(), Count);
}

static void HandleHubMarketBalanceCommand(const TArray<FString>& /*Args*/, UWorld* World)
{
	UPaldarkPlayerInventoryComponent* Inv = FindLocalPlayerInventory(World);
	if (Inv == nullptr)
	{
		return;
	}
	const int32 Credits = PaldarkHubConsole::CountCredits(Inv);
	UE_LOG(LogPaldarkUI, Log,
		TEXT("[Paldark.Hub.Market.Balance] credits=%d weight=%.1f/%.1f kg."),
		Credits, Inv->GetCurrentWeightKg(), Inv->GetMaxWeightKg());
}

// --------------------------------------------------------------------------
// W46 — Briefing room (hub QA pass) console commands.
//
// Six commands that exercise the vote/ready/countdown loop without authoring
// the UMG layout:
//   `Paldark.Hub.Brief.Dump`             — UPaldarkBriefingSessionComponent::DumpToLog.
//   `Paldark.Hub.Brief.Vote <MapTag>`    — controller->RequestVote (tag arg accepts
//                                          full `Paldark.Map.X` OR short leaf form).
//   `Paldark.Hub.Brief.Unvote`           — controller->RequestUnvote.
//   `Paldark.Hub.Brief.Ready`            — controller->RequestReady.
//   `Paldark.Hub.Brief.Unready`          — controller->RequestUnready.
//   `Paldark.Hub.QA.ForceTravel <MapTag> [MapName]`
//                                        — authority-only bypass; jumps the shard
//                                          straight to Travelling phase + issues
//                                          HostHubServer with the supplied map.
// --------------------------------------------------------------------------

namespace PaldarkBriefConsole
{
	static FGameplayTag ResolveMapTagArg(const FString& Arg)
	{
		FString TagString = Arg;
		TagString.TrimStartAndEndInline();
		if (!TagString.Contains(TEXT(".")))
		{
			TagString = FString::Printf(TEXT("Paldark.Map.%s"), *TagString);
		}
		return FGameplayTag::RequestGameplayTag(FName(*TagString), /*ErrorIfNotFound*/ false);
	}
}

static void HandleHubBriefDumpCommand(const TArray<FString>& /*Args*/, UWorld* World)
{
	UPaldarkBriefingSessionComponent* Session = UPaldarkBriefingSessionComponent::Get(World);
	if (Session == nullptr)
	{
		UE_LOG(LogPaldarkUI, Warning,
			TEXT("[Paldark.Hub.Brief.Dump] no UPaldarkBriefingSessionComponent on GameState (W46 wire-up missing?)."));
		return;
	}
	Session->DumpToLog();
}

static void HandleHubBriefVoteCommand(const TArray<FString>& Args, UWorld* World)
{
	if (Args.Num() < 1)
	{
		UE_LOG(LogPaldarkUI, Warning,
			TEXT("[Paldark.Hub.Brief.Vote] usage: Paldark.Hub.Brief.Vote <Paldark.Map.X>"));
		return;
	}
	APaldarkPlayerController* PC = PaldarkHubConsole::FindLocalPaldarkController(World);
	UPaldarkBriefingController* Ctrl = PC != nullptr ? PC->GetBriefingController() : nullptr;
	if (Ctrl == nullptr)
	{
		UE_LOG(LogPaldarkUI, Warning, TEXT("[Paldark.Hub.Brief.Vote] no BriefingController on local PC."));
		return;
	}
	const FGameplayTag MapTag = PaldarkBriefConsole::ResolveMapTagArg(Args[0]);
	if (!MapTag.IsValid())
	{
		UE_LOG(LogPaldarkUI, Warning,
			TEXT("[Paldark.Hub.Brief.Vote] unknown map tag '%s'."), *Args[0]);
		return;
	}
	Ctrl->RequestVote(MapTag);
	UE_LOG(LogPaldarkUI, Log, TEXT("[Paldark.Hub.Brief.Vote] requested tag=%s."), *MapTag.ToString());
}

static void HandleHubBriefUnvoteCommand(const TArray<FString>& /*Args*/, UWorld* World)
{
	APaldarkPlayerController* PC = PaldarkHubConsole::FindLocalPaldarkController(World);
	UPaldarkBriefingController* Ctrl = PC != nullptr ? PC->GetBriefingController() : nullptr;
	if (Ctrl == nullptr)
	{
		UE_LOG(LogPaldarkUI, Warning, TEXT("[Paldark.Hub.Brief.Unvote] no BriefingController on local PC."));
		return;
	}
	Ctrl->RequestUnvote();
	UE_LOG(LogPaldarkUI, Log, TEXT("[Paldark.Hub.Brief.Unvote] requested."));
}

static void HandleHubBriefReadyCommand(const TArray<FString>& /*Args*/, UWorld* World)
{
	APaldarkPlayerController* PC = PaldarkHubConsole::FindLocalPaldarkController(World);
	UPaldarkBriefingController* Ctrl = PC != nullptr ? PC->GetBriefingController() : nullptr;
	if (Ctrl == nullptr)
	{
		UE_LOG(LogPaldarkUI, Warning, TEXT("[Paldark.Hub.Brief.Ready] no BriefingController on local PC."));
		return;
	}
	Ctrl->RequestReady();
	UE_LOG(LogPaldarkUI, Log, TEXT("[Paldark.Hub.Brief.Ready] requested."));
}

static void HandleHubBriefUnreadyCommand(const TArray<FString>& /*Args*/, UWorld* World)
{
	APaldarkPlayerController* PC = PaldarkHubConsole::FindLocalPaldarkController(World);
	UPaldarkBriefingController* Ctrl = PC != nullptr ? PC->GetBriefingController() : nullptr;
	if (Ctrl == nullptr)
	{
		UE_LOG(LogPaldarkUI, Warning, TEXT("[Paldark.Hub.Brief.Unready] no BriefingController on local PC."));
		return;
	}
	Ctrl->RequestUnready();
	UE_LOG(LogPaldarkUI, Log, TEXT("[Paldark.Hub.Brief.Unready] requested."));
}

namespace PaldarkSaveConsole
{
	// Resolve a console-arg trigger keyword to its tag. Designer-facing
	// shortcuts so QA doesn't have to type the full tag path. Unknown
	// strings fall back to Manual + a warning so typos don't silently
	// stamp the wrong trigger on the delegate payload.
	static FGameplayTag ResolveTriggerArg(const FString& Raw)
	{
		const FString S = Raw.TrimStartAndEnd();
		if (S.Equals(TEXT("HubReturn"), ESearchCase::IgnoreCase) ||
			S.Equals(TEXT("Paldark.Save.Trigger.HubReturn"), ESearchCase::IgnoreCase))
		{
			return PaldarkGameplayTags::TAG_Paldark_Save_Trigger_HubReturn;
		}
		if (S.Equals(TEXT("Logout"), ESearchCase::IgnoreCase) ||
			S.Equals(TEXT("Paldark.Save.Trigger.Logout"), ESearchCase::IgnoreCase))
		{
			return PaldarkGameplayTags::TAG_Paldark_Save_Trigger_Logout;
		}
		if (S.Equals(TEXT("Manual"), ESearchCase::IgnoreCase) ||
			S.Equals(TEXT("Paldark.Save.Trigger.Manual"), ESearchCase::IgnoreCase) ||
			S.IsEmpty())
		{
			return PaldarkGameplayTags::TAG_Paldark_Save_Trigger_Manual;
		}
		// Last-resort: ask the tag manager directly in case the designer
		// types a tag we don't recognise statically (e.g. a sub-tag they
		// added in Project Settings).
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*S), /*ErrorIfNotFound*/ false);
		if (Tag.IsValid())
		{
			return Tag;
		}
		UE_LOG(LogPaldark, Warning,
			TEXT("[Paldark.Save.Save] unknown trigger '%s' — falling back to Paldark.Save.Trigger.Manual"),
			*S);
		return PaldarkGameplayTags::TAG_Paldark_Save_Trigger_Manual;
	}

	static APaldarkPlayerController* ResolveLocalPC(UWorld* World)
	{
		if (World == nullptr)
		{
			return nullptr;
		}
		return Cast<APaldarkPlayerController>(World->GetFirstPlayerController());
	}

	static UPaldarkSaveSubsystem* ResolveSubsystem(UWorld* World)
	{
		if (World == nullptr)
		{
			return nullptr;
		}
		const UGameInstance* GI = World->GetGameInstance();
		return GI != nullptr ? GI->GetSubsystem<UPaldarkSaveSubsystem>() : nullptr;
	}
}

static void HandleSaveSaveCommand(const TArray<FString>& Args, UWorld* World)
{
	UPaldarkSaveSubsystem* Sub = PaldarkSaveConsole::ResolveSubsystem(World);
	APaldarkPlayerController* PC = PaldarkSaveConsole::ResolveLocalPC(World);
	if (Sub == nullptr || PC == nullptr)
	{
		UE_LOG(LogPaldark, Warning,
			TEXT("[Paldark.Save.Save] missing subsystem (=%s) or local PC (=%s)"),
			Sub  != nullptr ? TEXT("ok") : TEXT("null"),
			PC   != nullptr ? TEXT("ok") : TEXT("null"));
		return;
	}

	const FName  SlotName    = Args.IsValidIndex(0) ? FName(*Args[0]) : NAME_None;
	const FGameplayTag TriggerTag = Args.IsValidIndex(1)
		? PaldarkSaveConsole::ResolveTriggerArg(Args[1])
		: PaldarkGameplayTags::TAG_Paldark_Save_Trigger_Manual;
	const bool bQueued = Sub->RequestSaveSlot(PC, SlotName, TriggerTag);
	UE_LOG(LogPaldark, Log,
		TEXT("[Paldark.Save.Save] slot=%s trigger=%s queued=%s"),
		SlotName.IsNone() ? TEXT("<default>") : *SlotName.ToString(),
		*TriggerTag.ToString(),
		bQueued ? TEXT("yes") : TEXT("no"));
}

static void HandleSaveLoadCommand(const TArray<FString>& Args, UWorld* World)
{
	UPaldarkSaveSubsystem* Sub = PaldarkSaveConsole::ResolveSubsystem(World);
	APaldarkPlayerController* PC = PaldarkSaveConsole::ResolveLocalPC(World);
	if (Sub == nullptr || PC == nullptr)
	{
		UE_LOG(LogPaldark, Warning,
			TEXT("[Paldark.Save.Load] missing subsystem (=%s) or local PC (=%s)"),
			Sub  != nullptr ? TEXT("ok") : TEXT("null"),
			PC   != nullptr ? TEXT("ok") : TEXT("null"));
		return;
	}
	const FName SlotName = Args.IsValidIndex(0) ? FName(*Args[0]) : NAME_None;
	const bool bQueued = Sub->RequestLoadSlot(PC, SlotName);
	UE_LOG(LogPaldark, Log,
		TEXT("[Paldark.Save.Load] slot=%s queued=%s"),
		SlotName.IsNone() ? TEXT("<default>") : *SlotName.ToString(),
		bQueued ? TEXT("yes") : TEXT("no"));
}

static void HandleSaveDumpCommand(const TArray<FString>& Args, UWorld* World)
{
	UPaldarkSaveSubsystem* Sub = PaldarkSaveConsole::ResolveSubsystem(World);
	if (Sub == nullptr)
	{
		UE_LOG(LogPaldark, Warning, TEXT("[Paldark.Save.Dump] missing UPaldarkSaveSubsystem"));
		return;
	}
	if (!Args.IsValidIndex(0))
	{
		UE_LOG(LogPaldark, Warning,
			TEXT("[Paldark.Save.Dump] usage: Paldark.Save.Dump <SlotName>"));
		return;
	}
	const FName SlotName(*Args[0]);
	const bool bOk = Sub->RequestDumpSlot(SlotName);
	UE_LOG(LogPaldark, Log,
		TEXT("[Paldark.Save.Dump] slot=%s result=%s"),
		*SlotName.ToString(),
		bOk ? TEXT("ok") : TEXT("fail"));
}

static void HandleSaveClearSlotCommand(const TArray<FString>& Args, UWorld* World)
{
	UPaldarkSaveSubsystem* Sub = PaldarkSaveConsole::ResolveSubsystem(World);
	if (Sub == nullptr)
	{
		UE_LOG(LogPaldark, Warning, TEXT("[Paldark.Save.ClearSlot] missing UPaldarkSaveSubsystem"));
		return;
	}
	if (!Args.IsValidIndex(0))
	{
		UE_LOG(LogPaldark, Warning,
			TEXT("[Paldark.Save.ClearSlot] usage: Paldark.Save.ClearSlot <SlotName>"));
		return;
	}
	const FName SlotName(*Args[0]);
	const bool bOk = Sub->ClearSlot(SlotName);
	UE_LOG(LogPaldark, Log,
		TEXT("[Paldark.Save.ClearSlot] slot=%s result=%s"),
		*SlotName.ToString(),
		bOk ? TEXT("ok") : TEXT("fail"));
}

static void HandleSaveListSlotsCommand(const TArray<FString>& Args, UWorld* World)
{
	UPaldarkSaveSubsystem* Sub = PaldarkSaveConsole::ResolveSubsystem(World);
	if (Sub == nullptr)
	{
		UE_LOG(LogPaldark, Warning, TEXT("[Paldark.Save.ListSlots] missing UPaldarkSaveSubsystem"));
		return;
	}
	const int32 Count = Sub->ListSlotsToLog();
	UE_LOG(LogPaldark, Log, TEXT("[Paldark.Save.ListSlots] listed=%d"), Count);
}

static void HandleQAWipeAllSlotsCommand(const TArray<FString>& Args, UWorld* World)
{
	UPaldarkSaveSubsystem* Sub = PaldarkSaveConsole::ResolveSubsystem(World);
	if (Sub == nullptr)
	{
		UE_LOG(LogPaldark, Warning, TEXT("[Paldark.QA.WipeAllSlots] missing UPaldarkSaveSubsystem"));
		return;
	}
	const int32 Deleted = Sub->WipeAllSlots();
	UE_LOG(LogPaldark, Log, TEXT("[Paldark.QA.WipeAllSlots] deleted=%d"), Deleted);
}

// W48 — Polish smoke test for the hub→raid handoff path. Wraps
// UPaldarkSaveSubsystem::RequestHubToRaidHandoffSmoke so QA / designers
// can grep the log for the `Paldark.Save.Trigger.Travel` save without
// going through an actual ServerTravel. Args:
//   [SlotName] — optional, defaults to GetDefaultSlotName(local PC).
static void HandleQAHubToRaidHandoffCommand(const TArray<FString>& Args, UWorld* World)
{
	UPaldarkSaveSubsystem* Sub = PaldarkSaveConsole::ResolveSubsystem(World);
	APaldarkPlayerController* PC = PaldarkSaveConsole::ResolveLocalPC(World);
	if (Sub == nullptr || PC == nullptr)
	{
		UE_LOG(LogPaldark, Warning,
			TEXT("[Paldark.QA.HubToRaidHandoff] missing subsystem (=%s) or local PC (=%s)"),
			Sub != nullptr ? TEXT("ok") : TEXT("null"),
			PC  != nullptr ? TEXT("ok") : TEXT("null"));
		return;
	}

	const FName SlotName = (Args.Num() >= 1 && !Args[0].IsEmpty())
		? FName(*Args[0])
		: UPaldarkSaveSubsystem::GetDefaultSlotName(PC);

	const bool bQueued = Sub->RequestHubToRaidHandoffSmoke(PC, SlotName);
	UE_LOG(LogPaldark, Log,
		TEXT("[Paldark.QA.HubToRaidHandoff] slot=%s trigger=Paldark.Save.Trigger.Travel queued=%s"),
		*SlotName.ToString(),
		bQueued ? TEXT("ok") : TEXT("fail"));
}

static void HandleHubQAForceTravelCommand(const TArray<FString>& Args, UWorld* World)
{
	if (Args.Num() < 1)
	{
		UE_LOG(LogPaldarkUI, Warning,
			TEXT("[Paldark.Hub.QA.ForceTravel] usage: Paldark.Hub.QA.ForceTravel <Paldark.Map.X> [MapName]"));
		return;
	}
	if (World == nullptr || World->GetNetMode() == NM_Client)
	{
		UE_LOG(LogPaldarkUI, Warning,
			TEXT("[Paldark.Hub.QA.ForceTravel] authority-only command; run on server / listen-host."));
		return;
	}
	UPaldarkBriefingSessionComponent* Session = UPaldarkBriefingSessionComponent::Get(World);
	if (Session == nullptr)
	{
		UE_LOG(LogPaldarkUI, Warning,
			TEXT("[Paldark.Hub.QA.ForceTravel] no UPaldarkBriefingSessionComponent on GameState."));
		return;
	}
	const FGameplayTag MapTag = PaldarkBriefConsole::ResolveMapTagArg(Args[0]);
	if (!MapTag.IsValid())
	{
		UE_LOG(LogPaldarkUI, Warning,
			TEXT("[Paldark.Hub.QA.ForceTravel] unknown map tag '%s'."), *Args[0]);
		return;
	}
	const FString MapName = Args.Num() >= 2 ? Args[1] : FString();
	Session->ServerForceTravel(MapTag, MapName);
	UE_LOG(LogPaldarkUI, Log,
		TEXT("[Paldark.Hub.QA.ForceTravel] force-travel tag=%s map_name='%s'."),
		*MapTag.ToString(), MapName.IsEmpty() ? TEXT("<auto>") : *MapName);
}

void FPaldarkLabModule::StartupModule()
{
	UE_LOG(LogPaldark, Log, TEXT("PaldarkLab module started."));

	IConsoleManager& Console = IConsoleManager::Get();

	HelloWorldCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.HelloWorld"),
		TEXT("Emit a Hello World line to LogPaldark and on-screen debug. Args: optional message."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleHelloWorldCommand),
		ECVF_Default);

	ExperienceCurrentCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Experience.Current"),
		TEXT("Log the FPrimaryAssetId + object path of the current experience on the active world."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleExperienceCurrentCommand),
		ECVF_Default);

	ExperienceHelloCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Experience.Hello"),
		TEXT("Log the HelloWorldMessage of the current experience to LogPaldark + on-screen debug."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleExperienceHelloCommand),
		ECVF_Default);

	ExperienceListExtensionsCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Experience.ListExtensions"),
		TEXT("List active action sets, granted gameplay tags, and class overrides for the current experience (server-side game mode)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleExperienceListExtensionsCommand),
		ECVF_Default);

	InputListBindingsCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Input.ListBindings"),
		TEXT("List the active PawnData InputConfig + mapping contexts on each PaldarkPlayerController in the current world."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleInputListBindingsCommand),
		ECVF_Default);

	PalSpawnTestCompanionCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Pal.SpawnTestCompanion"),
		TEXT("Spawn one Pal companion behind the local player. Args: [index] (PawnData->DefaultPalCompanions row, default 0) [class_path] (override)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandlePalSpawnTestCompanionCommand),
		ECVF_Default);

	PalCurrentActivityCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Pal.CurrentActivity"),
		TEXT("Log the active activity (class + tag) for every Pal in the current world."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandlePalCurrentActivityCommand),
		ECVF_Default);

	PalSetActivityCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Pal.SetActivity"),
		TEXT("Force every Pal's Activity FSM to a state. Args: <Idle|Follow|Investigate> | full Paldark.Pal.Activity.* tag."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandlePalSetActivityCommand),
		ECVF_Default);

	PalPingCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Pal.Ping"),
		TEXT("File an Investigate request on every Pal. Args: optional X Y Z world location; defaults to 6m in front of the local player."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandlePalPingCommand),
		ECVF_Default);

	GasDumpAttributesCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Gas.DumpAttributes"),
		TEXT("Dump Health/Stamina/MoveSpeed from every IAbilitySystemInterface actor in the world (player PS + pals)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleGasDumpAttributesCommand),
		ECVF_Default);

	GasDamageCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Gas.Damage"),
		TEXT("Apply a one-shot Health -= <Amount> to the local player (authority only). Args: <Amount> (default 10)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleGasDamageCommand),
		ECVF_Default);

	CombatSpawnDummyCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Combat.SpawnDummy"),
		TEXT("Spawn an APaldarkDummyTarget in front of the local player (W9-10). Args: [distance_cm=500]."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleCombatSpawnDummyCommand),
		ECVF_Default);

	CombatFireCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Combat.Fire"),
		TEXT("Activate the Fire ability on the local player's ASC without going through Enhanced Input (W9-10)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleCombatFireCommand),
		ECVF_Default);

	InventoryListCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Inventory.List"),
		TEXT("Dump the local player's inventory contents (W11-12)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleInventoryListCommand),
		ECVF_Default);

	InventoryAddCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Inventory.Add"),
		TEXT("Server-add an item to the local player's inventory by Paldark.Item.* tag (W11-12). Args: <Tag> [Count=1]."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleInventoryAddCommand),
		ECVF_Default);

	InventoryRemoveCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Inventory.Remove"),
		TEXT("Server-remove an item from the local player's inventory by Paldark.Item.* tag (W11-12). Args: <Tag> [Count=1]."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleInventoryRemoveCommand),
		ECVF_Default);

	InventoryDropCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Inventory.Drop"),
		TEXT("Clear the local player's inventory on the server (W11-12). World pickup-actor spawn is a follow-up PR."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleInventoryDropCommand),
		ECVF_Default);

	InventoryGiveAllCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Inventory.GiveAll"),
		TEXT("Server-add 1 of every PaldarkItem asset registered with the AssetManager (W37-38). Authority only."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleInventoryGiveAllCommand),
		ECVF_Default);

	InventoryEquipBackpackCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Inventory.EquipBackpack"),
		TEXT("Equip the backpack item tagged Paldark.Item.Backpack.<tier> (W37-38). Args: <T1|T2|T3|clear>. Authority only."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleInventoryEquipBackpackCommand),
		ECVF_Default);

	InventoryTestDeathDropCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Inventory.TestDeathDrop"),
		TEXT("Spawn a loot bag from the local player's inventory without taking damage (W37-38). Authority only."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleInventoryTestDeathDropCommand),
		ECVF_Default);

	InventoryDumpCompositeCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Inventory.DumpComposite"),
		TEXT("Dump the local player's inventory + any nested container InnerEntries (W37-38)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleInventoryDumpCompositeCommand),
		ECVF_Default);

	PalSpawnFromDefinitionCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Pal.SpawnFromDefinition"),
		TEXT("Async-load + spawn from a PaldarkPalDefinition primary asset id (W27-28). Args: <DefId> [X Y Z] (defaults to 4m in front of local player)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandlePalSpawnFromDefinitionCommand),
		ECVF_Default);

	PalDumpDefinitionRegistryCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Pal.DumpDefinitionRegistry"),
		TEXT("List every PaldarkPalDefinition tracked by UPaldarkPalSpawnSubsystem with its pre-warm state (W27-28)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandlePalDumpDefinitionRegistryCommand),
		ECVF_Default);

	// ---- W44-45 — Stable + Marketplace UI console commands ---------------
	StableListCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Hub.Stable.List"),
		TEXT("Dump the local player's active roster + deposit list (W44-45)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleHubStableListCommand),
		ECVF_Default);

	StableDepositCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Hub.Stable.Deposit"),
		TEXT("Server RPC to deposit one roster entry into the PlayerState deposit list. Args: <RosterIndex> (W44-45)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleHubStableDepositCommand),
		ECVF_Default);

	StableWithdrawCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Hub.Stable.Withdraw"),
		TEXT("Server RPC to withdraw one deposit entry back into the active roster. Args: <DepositIndex> (W44-45)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleHubStableWithdrawCommand),
		ECVF_Default);

	StableHealCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Hub.Stable.Heal"),
		TEXT("Server RPC to debit credits + refill HealthAtTame on one roster entry. Args: <RosterIndex> (W44-45)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleHubStableHealCommand),
		ECVF_Default);

	MarketCatalogCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Hub.Market.Catalog"),
		TEXT("Dump the nearest Marketplace kiosk's catalog + buy/sell prices. Args: [KioskNameFilter] (W44-45)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleHubMarketCatalogCommand),
		ECVF_Default);

	MarketBuyCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Hub.Market.Buy"),
		TEXT("Server RPC to buy an item from the marketplace (debit credits, credit item). Args: <Paldark.Item.X> [Count=1] (W44-45)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleHubMarketBuyCommand),
		ECVF_Default);

	MarketSellCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Hub.Market.Sell"),
		TEXT("Server RPC to sell an item to the marketplace (debit item, credit credits). Args: <Paldark.Item.X> [Count=1] (W44-45)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleHubMarketSellCommand),
		ECVF_Default);

	MarketBalanceCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Hub.Market.Balance"),
		TEXT("Log the local player's current credits balance (W44-45)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleHubMarketBalanceCommand),
		ECVF_Default);

	// ---- W46 — Hub QA / Briefing room console commands ------------------
	BriefDumpCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Hub.Brief.Dump"),
		TEXT("Dump the shard-wide briefing session state (phase, tallies, voters) (W46)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleHubBriefDumpCommand),
		ECVF_Default);

	BriefVoteCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Hub.Brief.Vote"),
		TEXT("Cast a vote for the supplied raid map (W46). Args: <Paldark.Map.X|leaf>."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleHubBriefVoteCommand),
		ECVF_Default);

	BriefUnvoteCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Hub.Brief.Unvote"),
		TEXT("Clear the local player's vote (W46)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleHubBriefUnvoteCommand),
		ECVF_Default);

	BriefReadyCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Hub.Brief.Ready"),
		TEXT("Signal that the local player is ready to depart (W46)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleHubBriefReadyCommand),
		ECVF_Default);

	BriefUnreadyCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Hub.Brief.Unready"),
		TEXT("Clear the local player's ready bit (W46). Cancels countdown if threshold drops."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleHubBriefUnreadyCommand),
		ECVF_Default);

	QAForceTravelCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Hub.QA.ForceTravel"),
		TEXT("Authority-only bypass — force the shard to ServerTravel to the supplied map (W46). Args: <Paldark.Map.X> [MapName]."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleHubQAForceTravelCommand),
		ECVF_Default);

	// ---- W47 — Save game console commands -------------------------------
	SaveSaveCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Save.Save"),
		TEXT("Capture local player progression and async-write to a save slot (W47). Args: [SlotName] [TriggerTag=HubReturn|Logout|Manual]."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleSaveSaveCommand),
		ECVF_Default);

	SaveLoadCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Save.Load"),
		TEXT("Async-load a save slot and apply it to the local player (W47). Args: [SlotName]."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleSaveLoadCommand),
		ECVF_Default);

	SaveDumpCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Save.Dump"),
		TEXT("Load a save slot synchronously and log its contents without applying (W47). Args: <SlotName>."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleSaveDumpCommand),
		ECVF_Default);

	SaveClearSlotCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Save.ClearSlot"),
		TEXT("Delete one named save slot from disk (W47). Args: <SlotName>."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleSaveClearSlotCommand),
		ECVF_Default);

	SaveListSlotsCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.Save.ListSlots"),
		TEXT("List every .sav file under <ProjectSaved>/SaveGames with size + mtime (W47)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleSaveListSlotsCommand),
		ECVF_Default);

	QAWipeAllSlotsCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.QA.WipeAllSlots"),
		TEXT("Designer / QA only — delete every save slot the engine can see (W47)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleQAWipeAllSlotsCommand),
		ECVF_Default);

	// ---- W48 — Polish smoke test ----------------------------------------
	QAHubToRaidHandoffCommand = Console.RegisterConsoleCommand(
		TEXT("Paldark.QA.HubToRaidHandoff"),
		TEXT("Fire UPaldarkSaveSubsystem::RequestHubToRaidHandoffSmoke (W48). Args: [SlotName] (defaults to GetDefaultSlotName(local PC))."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleQAHubToRaidHandoffCommand),
		ECVF_Default);
}

void FPaldarkLabModule::ShutdownModule()
{
	IConsoleManager& Console = IConsoleManager::Get();
	for (IConsoleObject** Slot : {
		&HelloWorldCommand,
		&ExperienceCurrentCommand,
		&ExperienceHelloCommand,
		&ExperienceListExtensionsCommand,
		&InputListBindingsCommand,
		&PalSpawnTestCompanionCommand,
		&PalCurrentActivityCommand,
		&PalSetActivityCommand,
		&PalPingCommand,
		&GasDumpAttributesCommand,
		&GasDamageCommand,
		&CombatSpawnDummyCommand,
		&CombatFireCommand,
		&InventoryListCommand,
		&InventoryAddCommand,
		&InventoryRemoveCommand,
		&InventoryDropCommand,
		&InventoryGiveAllCommand,
		&InventoryEquipBackpackCommand,
		&InventoryTestDeathDropCommand,
		&InventoryDumpCompositeCommand,
		&PalSpawnFromDefinitionCommand,
		&PalDumpDefinitionRegistryCommand,
		&StableListCommand,
		&StableDepositCommand,
		&StableWithdrawCommand,
		&StableHealCommand,
		&MarketCatalogCommand,
		&MarketBuyCommand,
		&MarketSellCommand,
		&MarketBalanceCommand,
		&BriefDumpCommand,
		&BriefVoteCommand,
		&BriefUnvoteCommand,
		&BriefReadyCommand,
		&BriefUnreadyCommand,
		&QAForceTravelCommand,
		&SaveSaveCommand,
		&SaveLoadCommand,
		&SaveDumpCommand,
		&SaveClearSlotCommand,
		&SaveListSlotsCommand,
		&QAWipeAllSlotsCommand,
	})
	{
		if (*Slot != nullptr)
		{
			Console.UnregisterConsoleObject(*Slot);
			*Slot = nullptr;
		}
	}

	UE_LOG(LogPaldark, Log, TEXT("PaldarkLab module shutdown."));
}

IMPLEMENT_PRIMARY_GAME_MODULE(FPaldarkLabModule, PaldarkLab, "PaldarkLab")
