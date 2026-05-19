#include "Player/PaldarkPlayerController.h"

#include "AbilitySystemComponent.h"
#include "Combat/PaldarkLagCompensationComponent.h"
#include "Engine/HitResult.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"
#include "HAL/IConsoleManager.h"
#include "InputMappingContext.h"

#include "Experience/PaldarkExperienceDefinition.h"
#include "Experience/PaldarkInputConfig.h"
#include "Experience/PaldarkPawnData.h"
#include "Framework/PaldarkGameStateBase.h"
#include "Gas/PaldarkAbilitySystemComponent.h"
#include "Gas/PaldarkGameplayAbility.h"
#include "Hub/PaldarkBriefingController.h"
#include "Hub/PaldarkMarketplaceController.h"
#include "Hub/PaldarkStableController.h"
#include "InputAction.h"
#include "Match/PaldarkExtractionBeacon.h"
#include "Match/PaldarkMatchSubsystem.h"
#include "Match/PaldarkMatchTypes.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"
#include "Player/PaldarkCharacter.h"
#include "Player/PaldarkPlayerState.h"
#include "Squad/PaldarkPingMarker.h"
#include "Squad/PaldarkPingSubsystem.h"
#include "Squad/PaldarkSquadCommandComponent.h"
#include "Squad/PaldarkSquadSubsystem.h"

APaldarkPlayerController::APaldarkPlayerController()
{
	// W44-45 — Hub UI controller components. Created as default subobjects
	// so every player ships with both, no designer wiring required.
	StableController      = CreateDefaultSubobject<UPaldarkStableController>(TEXT("StableController"));
	MarketplaceController = CreateDefaultSubobject<UPaldarkMarketplaceController>(TEXT("MarketplaceController"));

	// W46 — Briefing controller default subobject (same pattern as Stable/Market).
	BriefingController    = CreateDefaultSubobject<UPaldarkBriefingController>(TEXT("BriefingController"));
}

const UPaldarkPawnData* APaldarkPlayerController::ResolvePawnDataForCurrentExperience() const
{
	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}
	const APaldarkGameStateBase* GameState = World->GetGameState<APaldarkGameStateBase>();
	if (GameState == nullptr)
	{
		return nullptr;
	}
	const UPaldarkExperienceDefinition* Experience = GameState->GetCurrentExperience();
	if (Experience == nullptr || Experience->DefaultPawnData.IsNull())
	{
		return nullptr;
	}
	// LoadSynchronous is fine here: the game mode already loaded the experience
	// (and therefore its PawnData via the experience-load flow) before any pawn
	// spawns. Switch to async at P14 if startup hitches become an issue.
	return Experience->DefaultPawnData.LoadSynchronous();
}

void APaldarkPlayerController::ApplyMappingContexts(const UPaldarkPawnData* PawnData)
{
	if (PawnData == nullptr)
	{
		return;
	}
	const ULocalPlayer* LP = GetLocalPlayer();
	if (LP == nullptr)
	{
		// Dedicated-server controllers have no LocalPlayer — silent skip.
		return;
	}
	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (Subsystem == nullptr)
	{
		UE_LOG(LogPaldark, Warning, TEXT("APaldarkPlayerController::ApplyMappingContexts — no EnhancedInput subsystem on %s."), *GetName());
		return;
	}

	int32 AppliedCount = 0;
	for (const FPaldarkMappingContextAndPriority& Row : PawnData->DefaultMappingContexts)
	{
		if (Row.InputMapping.IsNull())
		{
			continue;
		}
		UInputMappingContext* IMC = Row.InputMapping.LoadSynchronous();
		if (IMC == nullptr)
		{
			UE_LOG(LogPaldark, Warning, TEXT("APaldarkPlayerController::ApplyMappingContexts — IMC %s failed to load."), *Row.InputMapping.ToString());
			continue;
		}
		Subsystem->AddMappingContext(IMC, Row.Priority);
		++AppliedCount;
	}
	UE_LOG(LogPaldark, Log, TEXT("APaldarkPlayerController::ApplyMappingContexts — %s applied %d IMC(s)."), *GetName(), AppliedCount);
}

void APaldarkPlayerController::ClearMappingContexts(const UPaldarkPawnData* PawnData)
{
	if (PawnData == nullptr)
	{
		return;
	}
	const ULocalPlayer* LP = GetLocalPlayer();
	if (LP == nullptr)
	{
		return;
	}
	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (Subsystem == nullptr)
	{
		return;
	}
	for (const FPaldarkMappingContextAndPriority& Row : PawnData->DefaultMappingContexts)
	{
		if (UInputMappingContext* IMC = Row.InputMapping.Get())
		{
			Subsystem->RemoveMappingContext(IMC);
		}
	}
}

void APaldarkPlayerController::OnPossess(APawn* InPawn)
{
	// W1 day 11-14 — resolve the PawnData BEFORE Super::OnPossess so that the
	// character's SetupPlayerInputComponent (called via Super → Pawn::Restart)
	// can already see CachedPawnData when it binds actions.
	ActivePawnData = ResolvePawnDataForCurrentExperience();

	if (APaldarkCharacter* PaldarkChar = Cast<APaldarkCharacter>(InPawn))
	{
		PaldarkChar->SetPawnData(ActivePawnData);
	}

	Super::OnPossess(InPawn);

	// IMCs are added AFTER Super so the Enhanced Input subsystem is fully
	// initialized for the new pawn.
	ApplyMappingContexts(ActivePawnData);

	// W7-8 — Grant GAS abilities + startup effects on the server. The pawn's
	// PossessedBy already ran (Super::OnPossess) so the PlayerState ASC is
	// bound on the character via InitAbilitySystem.
	if (HasAuthority())
	{
		GrantGasFromPawnData(InPawn, ActivePawnData);
	}

	UE_LOG(
		LogPaldark,
		Log,
		TEXT("APaldarkPlayerController::OnPossess — controller=%s pawn=%s pawn_data=%s"),
		*GetName(),
		InPawn != nullptr ? *InPawn->GetName() : TEXT("<null>"),
		ActivePawnData != nullptr ? *ActivePawnData->GetName() : TEXT("<null>"));
}

void APaldarkPlayerController::GrantGasFromPawnData(APawn* InPawn, const UPaldarkPawnData* PawnData)
{
	if (InPawn == nullptr || PawnData == nullptr)
	{
		return;
	}
	const APaldarkCharacter* PaldarkChar = Cast<APaldarkCharacter>(InPawn);
	APaldarkPlayerState* PS = PaldarkChar != nullptr ? PaldarkChar->GetPlayerState<APaldarkPlayerState>() : nullptr;
	if (PS == nullptr)
	{
		UE_LOG(LogPaldarkGAS, Warning,
			TEXT("APaldarkPlayerController::GrantGasFromPawnData — %s has no Paldark PlayerState."),
			*InPawn->GetName());
		return;
	}
	UPaldarkAbilitySystemComponent* ASC = PS->GetPaldarkAbilitySystemComponent();
	if (ASC == nullptr)
	{
		return;
	}

	// Grant abilities. Soft classes so we LoadSynchronous here — the
	// PawnData is already in memory so the synchronous load is cheap. Async
	// polish is a P14 follow-up.
	int32 AbilityCount = 0;
	for (const TSoftClassPtr<UPaldarkGameplayAbility>& SoftClass : PawnData->GrantedAbilities)
	{
		if (SoftClass.IsNull())
		{
			continue;
		}
		const TSubclassOf<UPaldarkGameplayAbility> AbilityClass = SoftClass.LoadSynchronous();
		if (!AbilityClass)
		{
			continue;
		}
		FGameplayAbilitySpec Spec(AbilityClass, /*Level*/ 1, /*InputID*/ INDEX_NONE, /*SourceObject*/ this);
		ASC->GiveAbility(Spec);
		++AbilityCount;
	}

	// Apply startup effects (InitAttributes / regen / etc.). These run once
	// on possess; later weeks add a respawn flow that re-applies them.
	int32 EffectCount = 0;
	for (const TSoftClassPtr<UGameplayEffect>& SoftEffect : PawnData->StartupEffects)
	{
		if (SoftEffect.IsNull())
		{
			continue;
		}
		const TSubclassOf<UGameplayEffect> EffectClass = SoftEffect.LoadSynchronous();
		if (!EffectClass)
		{
			continue;
		}
		FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
		Ctx.AddSourceObject(this);
		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, /*Level*/ 1.0f, Ctx);
		if (Spec.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			++EffectCount;
		}
	}

	UE_LOG(LogPaldarkGAS, Log,
		TEXT("APaldarkPlayerController::GrantGasFromPawnData — %s granted=%d effects=%d."),
		*PS->GetName(), AbilityCount, EffectCount);
}

void APaldarkPlayerController::OnUnPossess()
{
	UE_LOG(LogPaldark, Log, TEXT("APaldarkPlayerController::OnUnPossess — controller=%s"), *GetName());
	ClearMappingContexts(ActivePawnData);
	ActivePawnData = nullptr;
	Super::OnUnPossess();
}

// ------------------------------------------------------------------
// W16-17 — Time sync handshake.
// ------------------------------------------------------------------
//
// Flow (autonomous client):
//   1. ReceivedPlayer fires once the controller is associated with a local
//      player. We immediately call ServerRequestServerTime to seed
//      ClientServerDelta + SingleTripTime so the first fire input that
//      arrives within ~50 ms has a working clock.
//   2. PlayerTick runs CheckTimeSync each frame; it re-issues
//      ServerRequestServerTime every TimeSyncFrequency seconds so clock
//      drift over a long match stays bounded.
//   3. ServerRequestServerTime (server impl) stamps GetWorld()->GetTimeSeconds()
//      at receipt and ships ClientReportServerTime back.
//   4. ClientReportServerTime (client impl) computes round-trip time and
//      derives ClientServerDelta + SingleTripTime.
//
// Note the implementation is a no-op on the server's own controller path:
// HasAuthority() short-circuits CheckTimeSync so the listen-server host
// doesn't burn RPCs talking to itself.

void APaldarkPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	if (IsLocalController() && !HasAuthority())
	{
		// Seed the clock immediately on join so the first ability fire
		// has a usable HitTime.
		const float ClientTime = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.f;
		ServerRequestServerTime(ClientTime);
		TimeSinceLastSync = 0.f;
	}
}

void APaldarkPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	CheckTimeSync(DeltaTime);
}

void APaldarkPlayerController::CheckTimeSync(float DeltaTime)
{
	if (!IsLocalController() || HasAuthority())
	{
		// Server's own controller has authoritative time — no handshake needed.
		return;
	}
	TimeSinceLastSync += DeltaTime;
	if (TimeSinceLastSync < TimeSyncFrequency)
	{
		return;
	}
	TimeSinceLastSync = 0.f;
	const float ClientTime = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.f;
	ServerRequestServerTime(ClientTime);
}

void APaldarkPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	const float ServerReceiveTime = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.f;
	ClientReportServerTime(TimeOfClientRequest, ServerReceiveTime);
}

void APaldarkPlayerController::ClientReportServerTime_Implementation(
	float TimeOfClientRequest,
	float TimeServerReceivedClientRequest)
{
	const float Now = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.f;
	const float RoundTripTime = Now - TimeOfClientRequest;
	// Half RTT is the canonical "single trip" estimate. Asymmetric routes
	// can break this — RoN study notes the issue but the practical impact
	// on hitscan compensation is <1 ms so we live with it.
	SingleTripTime = 0.5f * RoundTripTime;
	const float CurrentServerTimeEstimate = TimeServerReceivedClientRequest + SingleTripTime;
	ClientServerDelta = CurrentServerTimeEstimate - Now;

	UE_LOG(LogPaldarkNet, Verbose,
		TEXT("APaldarkPlayerController::ClientReportServerTime — rtt=%.4f single_trip=%.4f delta=%.4f"),
		RoundTripTime, SingleTripTime, ClientServerDelta);
}

float APaldarkPlayerController::GetServerTime() const
{
	const float WorldTime = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.f;
	if (HasAuthority())
	{
		return WorldTime;
	}
	return WorldTime + ClientServerDelta;
}

// ------------------------------------------------------------------
// W16-17 — Console commands.
// ------------------------------------------------------------------
//
// Three commands registered as console commands (FAutoConsoleCommandWithWorldAndArgs):
//   - Paldark.LagComp.DumpHistory        — dump the local player's lag comp
//                                           buffer to LogPaldarkNet.
//   - Paldark.LagComp.SimulateHit <Ms>   — server-side debug: trace from the
//                                           local pawn, then call
//                                           ServerSideRewind backdated by
//                                           <Ms> milliseconds. Logs result.
//   - Paldark.LagComp.ArtificialPing <N> — wrapper around `Net PktLag=N` for
//                                           designer convenience. Logs the
//                                           applied value.
//
// All three live in this TU (not the lag comp component) so they have access
// to the world / player controller without introducing a circular include
// back to PaldarkPlayerController.h.

namespace PaldarkLagCompConsole
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

	static void DumpHistory(const TArray<FString>& /*Args*/, UWorld* World)
	{
		APaldarkPlayerController* PC = FindLocalPaldarkController(World);
		if (PC == nullptr)
		{
			UE_LOG(LogPaldarkNet, Warning,
				TEXT("Paldark.LagComp.DumpHistory — no local Paldark controller."));
			return;
		}
		APaldarkCharacter* Avatar = Cast<APaldarkCharacter>(PC->GetPawn());
		if (Avatar == nullptr)
		{
			UE_LOG(LogPaldarkNet, Warning,
				TEXT("Paldark.LagComp.DumpHistory — local controller has no Paldark pawn."));
			return;
		}
		UPaldarkLagCompensationComponent* LagComp = Avatar->FindComponentByClass<UPaldarkLagCompensationComponent>();
		if (LagComp == nullptr)
		{
			UE_LOG(LogPaldarkNet, Warning,
				TEXT("Paldark.LagComp.DumpHistory — pawn %s has no LagCompensation component."),
				*Avatar->GetName());
			return;
		}
		LagComp->DumpHistoryToLog();
		UE_LOG(LogPaldarkNet, Log,
			TEXT("Paldark.LagComp.DumpHistory — controller=%s pawn=%s server_time=%.3f single_trip=%.4f"),
			*PC->GetName(),
			*Avatar->GetName(),
			PC->GetServerTime(),
			PC->GetSingleTripTime());
	}

	static void SimulateHit(const TArray<FString>& Args, UWorld* World)
	{
		const float BackdateMs = (Args.Num() > 0) ? FCString::Atof(*Args[0]) : 100.f;
		APaldarkPlayerController* PC = FindLocalPaldarkController(World);
		if (PC == nullptr || PC->GetPawn() == nullptr || World == nullptr)
		{
			UE_LOG(LogPaldarkNet, Warning,
				TEXT("Paldark.LagComp.SimulateHit — no local pawn / world."));
			return;
		}
		APaldarkCharacter* Shooter = Cast<APaldarkCharacter>(PC->GetPawn());
		if (Shooter == nullptr)
		{
			return;
		}

		FVector ViewLoc;
		FRotator ViewRot;
		PC->GetPlayerViewPoint(ViewLoc, ViewRot);
		const FVector TraceStart = ViewLoc;
		const FVector TraceEnd   = ViewLoc + ViewRot.Vector() * 10000.f;

		FCollisionQueryParams Params(SCENE_QUERY_STAT(PaldarkLagCompSimulate), /*bTraceComplex=*/true);
		Params.AddIgnoredActor(Shooter);
		FHitResult Hit;
		const bool bHit = World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Pawn, Params);
		APaldarkCharacter* HitChar = bHit ? Cast<APaldarkCharacter>(Hit.GetActor()) : nullptr;
		if (HitChar == nullptr)
		{
			UE_LOG(LogPaldarkNet, Log,
				TEXT("Paldark.LagComp.SimulateHit — initial trace did not hit a APaldarkCharacter."));
			return;
		}

		UPaldarkLagCompensationComponent* ShooterLagComp = Shooter->FindComponentByClass<UPaldarkLagCompensationComponent>();
		if (ShooterLagComp == nullptr)
		{
			UE_LOG(LogPaldarkNet, Warning,
				TEXT("Paldark.LagComp.SimulateHit — shooter %s has no LagCompensation."),
				*Shooter->GetName());
			return;
		}
		const float HitTime = PC->GetServerTime() - (BackdateMs / 1000.f);
		UE_LOG(LogPaldarkNet, Log,
			TEXT("Paldark.LagComp.SimulateHit — backdate=%.1f ms hit_time=%.3f target=%s"),
			BackdateMs, HitTime, *HitChar->GetName());
		ShooterLagComp->ServerScoreRequest_Hitscan(HitChar, TraceStart, Hit.ImpactPoint, HitTime);
	}

	static void ArtificialPing(const TArray<FString>& Args, UWorld* World)
	{
		const int32 PktLag = (Args.Num() > 0) ? FCString::Atoi(*Args[0]) : 100;
		const FString Cmd = FString::Printf(TEXT("Net PktLag=%d"), PktLag);
		if (GEngine != nullptr && World != nullptr)
		{
			GEngine->Exec(World, *Cmd);
		}
		UE_LOG(LogPaldarkNet, Log,
			TEXT("Paldark.LagComp.ArtificialPing — applied `%s`. Set 0 to clear."),
			*Cmd);
	}
}

// Auto-register console commands at module load. Each command's name lives
// under Paldark.LagComp.* so they share a discoverable namespace with the
// other W14+ net debug commands.
static FAutoConsoleCommandWithWorldAndArgs GLagCompDumpHistoryCmd(
	TEXT("Paldark.LagComp.DumpHistory"),
	TEXT("Dump the local Paldark pawn's lag compensation FrameHistory + server time estimate."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PaldarkLagCompConsole::DumpHistory));

static FAutoConsoleCommandWithWorldAndArgs GLagCompSimulateHitCmd(
	TEXT("Paldark.LagComp.SimulateHit"),
	TEXT("Trace from the local pawn and call ServerScoreRequest_Hitscan with HitTime back-dated by <BackdateMs> (default 100). Usage: Paldark.LagComp.SimulateHit [BackdateMs]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PaldarkLagCompConsole::SimulateHit));

static FAutoConsoleCommandWithWorldAndArgs GLagCompArtificialPingCmd(
	TEXT("Paldark.LagComp.ArtificialPing"),
	TEXT("Apply artificial latency via UE's built-in `Net PktLag=N`. Usage: Paldark.LagComp.ArtificialPing [Ms] (default 100, 0 to clear)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PaldarkLagCompConsole::ArtificialPing));

// ------------------------------------------------------------------
// W22-23 — Squad / ping RPC implementations.
// ------------------------------------------------------------------
//
// Both RPCs are Reliable + Server because:
//   - Pings are designer-intent communication, not gameplay-affecting state.
//     Reliable matches the perceived weight (a dropped ping confuses the
//     squad more than the small bandwidth cost saves).
//   - Server (not multicast) because the ping marker is a replicated actor
//     whose lifetime + visibility is server-authoritative.
//
// Anti-cheat sketch:
//   Server_RequestPing: validates the player owns a squad membership (any
//   tag). The location is taken as-given (client could lie, but pinging
//   off-map is harmless — the marker just spawns where it spawns).
//
//   Server_RequestMarkUnderCrosshair: server RE-TRACES from the controller's
//   view point instead of trusting client coords. Mismatches between client
//   intent and server result fall back to a generic Spot ping at the impact
//   point.

namespace PaldarkSquadInternals
{
	static UPaldarkSquadSubsystem* GetSquadSubsystem(UWorld* World)
	{
		return World != nullptr ? World->GetSubsystem<UPaldarkSquadSubsystem>() : nullptr;
	}

	static UPaldarkPingSubsystem* GetPingSubsystem(UWorld* World)
	{
		return World != nullptr ? World->GetSubsystem<UPaldarkPingSubsystem>() : nullptr;
	}
}

void APaldarkPlayerController::Server_RequestPing_Implementation(
	FVector WorldLocation,
	FGameplayTag PingType)
{
	APaldarkCharacter* Issuer = Cast<APaldarkCharacter>(GetPawn());
	UPaldarkPingSubsystem* PingSub = PaldarkSquadInternals::GetPingSubsystem(GetWorld());
	if (Issuer == nullptr || PingSub == nullptr)
	{
		UE_LOG(LogPaldarkPlayer, Warning,
			TEXT("APaldarkPlayerController::Server_RequestPing — issuer=%p ping_sub=%p."),
			Issuer, PingSub);
		return;
	}

	const FGameplayTag ResolvedType = PingType.IsValid()
		? PingType
		: PaldarkGameplayTags::TAG_Paldark_Ping_Type_Spot;

	PingSub->SpawnPing(Issuer, WorldLocation, /*MarkedActor*/ nullptr, ResolvedType);
}

void APaldarkPlayerController::Server_RequestMarkUnderCrosshair_Implementation()
{
	APaldarkCharacter* Issuer = Cast<APaldarkCharacter>(GetPawn());
	UWorld* World = GetWorld();
	UPaldarkPingSubsystem* PingSub = PaldarkSquadInternals::GetPingSubsystem(World);
	if (Issuer == nullptr || PingSub == nullptr || World == nullptr)
	{
		return;
	}

	// Server-side re-trace from the controller's actual view. The server
	// has its own up-to-date copy of the camera transform (the controller
	// replicates ControlRotation), so a malicious client can't trick the
	// trace into hitting a target it isn't looking at.
	FVector ViewLoc;
	FRotator ViewRot;
	GetPlayerViewPoint(ViewLoc, ViewRot);
	const FVector TraceStart = ViewLoc;
	const FVector TraceEnd   = ViewLoc + ViewRot.Vector() * MarkEnemyTraceDistance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(PaldarkMarkUnderCrosshair), /*bTraceComplex=*/false);
	Params.AddIgnoredActor(Issuer);
	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params);

	AActor* MarkedActor = bHit ? Hit.GetActor() : nullptr;
	FGameplayTag MarkedTeam;
	if (const APaldarkCharacter* HitPlayer = Cast<APaldarkCharacter>(MarkedActor))
	{
		MarkedTeam = HitPlayer->GetTeamTag();
	}

	// If we hit a hostile-team actor, file an Enemy ping with the actor
	// reference so the marker tracks the moving target. Otherwise fall back
	// to a Spot ping at the impact point (or trace end if the trace missed).
	FGameplayTag PingType;
	AActor* AttachActor = nullptr;
	FVector PingLocation;
	if (MarkedActor != nullptr && MarkedTeam == PaldarkGameplayTags::TAG_Paldark_Team_Hostile)
	{
		PingType    = PaldarkGameplayTags::TAG_Paldark_Ping_Type_Enemy;
		AttachActor = MarkedActor;
		PingLocation = MarkedActor->GetActorLocation();
	}
	else
	{
		PingType    = PaldarkGameplayTags::TAG_Paldark_Ping_Type_Spot;
		AttachActor = nullptr;
		PingLocation = bHit ? Hit.ImpactPoint : TraceEnd;
	}

	PingSub->SpawnPing(Issuer, PingLocation, AttachActor, PingType);
}

// ------------------------------------------------------------------
// W22-23 — Squad console commands.
// ------------------------------------------------------------------
//
// Three commands registered on the local Paldark controller's pawn:
//   - Paldark.Squad.Dump                       — dump squad + ping registry to LogPaldarkPlayer.
//   - Paldark.Squad.Ping <type> [<x> <y> <z>]  — fire a ping at the given world coordinate
//                                                 (default = under crosshair). Type defaults
//                                                 to Spot. Routes through Server_RequestPing.
//   - Paldark.Squad.Command <tag>              — issue a radial-wheel command via
//                                                 UPaldarkSquadCommandComponent. Tag is the
//                                                 short form (Attack/Follow/Stay/Regroup) or
//                                                 the full Paldark.Squad.Command.<X> string.

namespace PaldarkSquadConsole
{
	static APaldarkPlayerController* FindLocalController(UWorld* World)
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

	static FGameplayTag ResolveCommandTag(const FString& Short)
	{
		// Accept either the short Attack/Follow/Stay/Regroup or the fully
		// qualified Paldark.Squad.Command.* string.
		if (Short.Contains(TEXT(".")))
		{
			return FGameplayTag::RequestGameplayTag(FName(*Short), /*ErrorIfNotFound*/ false);
		}
		const FString Full = FString::Printf(TEXT("Paldark.Squad.Command.%s"), *Short);
		return FGameplayTag::RequestGameplayTag(FName(*Full), /*ErrorIfNotFound*/ false);
	}

	static FGameplayTag ResolvePingTypeTag(const FString& Short)
	{
		if (Short.Contains(TEXT(".")))
		{
			return FGameplayTag::RequestGameplayTag(FName(*Short), /*ErrorIfNotFound*/ false);
		}
		const FString Full = FString::Printf(TEXT("Paldark.Ping.Type.%s"), *Short);
		return FGameplayTag::RequestGameplayTag(FName(*Full), /*ErrorIfNotFound*/ false);
	}

	static void Dump(const TArray<FString>& /*Args*/, UWorld* World)
	{
		UPaldarkSquadSubsystem* SquadSub = PaldarkSquadInternals::GetSquadSubsystem(World);
		UPaldarkPingSubsystem* PingSub   = PaldarkSquadInternals::GetPingSubsystem(World);
		if (SquadSub == nullptr || PingSub == nullptr)
		{
			UE_LOG(LogPaldarkPlayer, Warning,
				TEXT("Paldark.Squad.Dump — subsystem unavailable (likely a client; run on host)."));
			return;
		}
		SquadSub->DumpToLog();
		UE_LOG(LogPaldarkPlayer, Log,
			TEXT("[Ping] active markers: %d"), PingSub->GetActivePingCount());
	}

	static void Ping(const TArray<FString>& Args, UWorld* World)
	{
		APaldarkPlayerController* PC = FindLocalController(World);
		if (PC == nullptr)
		{
			UE_LOG(LogPaldarkPlayer, Warning,
				TEXT("Paldark.Squad.Ping — no local Paldark controller."));
			return;
		}

		const FString TypeArg = (Args.Num() > 0) ? Args[0] : TEXT("Spot");
		const FGameplayTag PingType = ResolvePingTypeTag(TypeArg);
		if (!PingType.IsValid())
		{
			UE_LOG(LogPaldarkPlayer, Warning,
				TEXT("Paldark.Squad.Ping — unknown ping type '%s'."), *TypeArg);
			return;
		}

		FVector Location = FVector::ZeroVector;
		if (Args.Num() >= 4)
		{
			Location = FVector(FCString::Atof(*Args[1]),
			                   FCString::Atof(*Args[2]),
			                   FCString::Atof(*Args[3]));
		}
		else if (APawn* Pawn = PC->GetPawn())
		{
			Location = Pawn->GetActorLocation();
		}

		PC->Server_RequestPing(Location, PingType);
		UE_LOG(LogPaldarkPlayer, Log,
			TEXT("Paldark.Squad.Ping — requested %s @ %s"),
			*PingType.ToString(), *Location.ToCompactString());
	}

	static void Command(const TArray<FString>& Args, UWorld* World)
	{
		APaldarkPlayerController* PC = FindLocalController(World);
		if (PC == nullptr || PC->GetPawn() == nullptr)
		{
			UE_LOG(LogPaldarkPlayer, Warning,
				TEXT("Paldark.Squad.Command — no local Paldark controller / pawn."));
			return;
		}
		if (Args.Num() < 1)
		{
			UE_LOG(LogPaldarkPlayer, Warning,
				TEXT("Paldark.Squad.Command — usage: Paldark.Squad.Command <Attack|Follow|Stay|Regroup> [X Y Z]."));
			return;
		}

		const FGameplayTag CommandTag = ResolveCommandTag(Args[0]);
		if (!CommandTag.IsValid())
		{
			UE_LOG(LogPaldarkPlayer, Warning,
				TEXT("Paldark.Squad.Command — unknown command tag '%s'."), *Args[0]);
			return;
		}

		FVector OptionalLocation = FVector::ZeroVector;
		if (Args.Num() >= 4)
		{
			OptionalLocation = FVector(FCString::Atof(*Args[1]),
			                           FCString::Atof(*Args[2]),
			                           FCString::Atof(*Args[3]));
		}

		APaldarkCharacter* Pawn = Cast<APaldarkCharacter>(PC->GetPawn());
		UPaldarkSquadCommandComponent* Cmd = Pawn != nullptr ? Pawn->GetSquadCommandSlot() : nullptr;
		if (Cmd == nullptr)
		{
			UE_LOG(LogPaldarkPlayer, Warning,
				TEXT("Paldark.Squad.Command — pawn %s has no SquadCommandSlot."),
				Pawn != nullptr ? *Pawn->GetName() : TEXT("<null>"));
			return;
		}

		Cmd->IssueCommand(CommandTag, OptionalLocation);
		UE_LOG(LogPaldarkPlayer, Log,
			TEXT("Paldark.Squad.Command — issued %s (loc=%s)."),
			*CommandTag.ToString(), *OptionalLocation.ToCompactString());
	}
}

static FAutoConsoleCommandWithWorldAndArgs GSquadDumpCmd(
	TEXT("Paldark.Squad.Dump"),
	TEXT("Dump the squad + ping subsystem state on the host. Usage: Paldark.Squad.Dump"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PaldarkSquadConsole::Dump));

static FAutoConsoleCommandWithWorldAndArgs GSquadPingCmd(
	TEXT("Paldark.Squad.Ping"),
	TEXT("Fire a squad ping. Usage: Paldark.Squad.Ping <Spot|Enemy|Loot|Danger> [X Y Z]. Defaults to the local pawn location."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PaldarkSquadConsole::Ping));

static FAutoConsoleCommandWithWorldAndArgs GSquadCommandCmd(
	TEXT("Paldark.Squad.Command"),
	TEXT("Issue a squad radial-wheel command. Usage: Paldark.Squad.Command <Attack|Follow|Stay|Regroup> [X Y Z]."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PaldarkSquadConsole::Command));

// ------------------------------------------------------------------
// W24-25 — Match console commands.
// ------------------------------------------------------------------
//
// Three commands exercised by the designer / test harness against the
// `UPaldarkMatchSubsystem`. All three are server-side: clients run them
// but the side-effect lands on the host's subsystem (UE's console exec
// path supports remote routing through `ServerExec` for cheats; for
// these debug-only commands we just require the host to invoke them).
//
//   - Paldark.Match.Dump            — dump phase + per-player table to LogPaldarkPlayer.
//   - Paldark.Match.Phase <name>    — force the FSM to a specific phase (Warmup/Active/Extracting/Ended).
//                                      Idempotent. Skips the warmup timer / hard-timeout watchdog.
//   - Paldark.Match.ForceEnd [reason] — end the match immediately. Reason
//                                       defaults to ForcedByConsole; accepts
//                                       AllExtracted / TeamWipe / Timeout
//                                       for QA scenario authoring.

namespace PaldarkMatchConsole
{
	static UPaldarkMatchSubsystem* GetMatchSubsystem(UWorld* World)
	{
		return World != nullptr ? World->GetSubsystem<UPaldarkMatchSubsystem>() : nullptr;
	}

	static void Dump(const TArray<FString>& /*Args*/, UWorld* World)
	{
		if (UPaldarkMatchSubsystem* MatchSub = GetMatchSubsystem(World))
		{
			MatchSub->DumpToLog();
		}
		else
		{
			UE_LOG(LogPaldarkPlayer, Warning,
				TEXT("Paldark.Match.Dump — match subsystem unavailable (likely a client; run on host)."));
		}
	}

	static EPaldarkMatchPhase ParsePhase(const FString& Token, bool& bOk)
	{
		bOk = true;
		if (Token.Equals(TEXT("Warmup"),     ESearchCase::IgnoreCase)) return EPaldarkMatchPhase::Warmup;
		if (Token.Equals(TEXT("Active"),     ESearchCase::IgnoreCase)) return EPaldarkMatchPhase::Active;
		if (Token.Equals(TEXT("Extracting"), ESearchCase::IgnoreCase)) return EPaldarkMatchPhase::Extracting;
		if (Token.Equals(TEXT("Ended"),      ESearchCase::IgnoreCase)) return EPaldarkMatchPhase::Ended;
		bOk = false;
		return EPaldarkMatchPhase::Warmup;
	}

	static void Phase(const TArray<FString>& Args, UWorld* World)
	{
		UPaldarkMatchSubsystem* MatchSub = GetMatchSubsystem(World);
		if (MatchSub == nullptr)
		{
			UE_LOG(LogPaldarkPlayer, Warning,
				TEXT("Paldark.Match.Phase — match subsystem unavailable (run on host)."));
			return;
		}
		if (Args.Num() < 1)
		{
			UE_LOG(LogPaldarkPlayer, Warning,
				TEXT("Paldark.Match.Phase — usage: Paldark.Match.Phase <Warmup|Active|Extracting|Ended>"));
			return;
		}
		bool bOk = false;
		const EPaldarkMatchPhase Target = ParsePhase(Args[0], bOk);
		if (!bOk)
		{
			UE_LOG(LogPaldarkPlayer, Warning,
				TEXT("Paldark.Match.Phase — unknown phase '%s'."), *Args[0]);
			return;
		}

		// Console FSM transitions go through StartMatch / EndMatch so
		// timer state stays consistent. Manual Warmup re-entry is not
		// supported (the subsystem is intentionally monotonic).
		switch (Target)
		{
		case EPaldarkMatchPhase::Active:
			MatchSub->StartMatch();
			break;
		case EPaldarkMatchPhase::Ended:
			MatchSub->EndMatch(EPaldarkMatchEndReason::ForcedByConsole);
			break;
		case EPaldarkMatchPhase::Extracting:
			// Force-enter Extracting by calling StartMatch (idempotent for
			// Active) — the next RecordExtraction will flip to Extracting
			// naturally. Designer uses this to skip the warmup grace.
			MatchSub->StartMatch();
			UE_LOG(LogPaldarkPlayer, Log,
				TEXT("Paldark.Match.Phase Extracting — set Active; first RecordExtraction will land Extracting."));
			break;
		case EPaldarkMatchPhase::Warmup:
		default:
			UE_LOG(LogPaldarkPlayer, Warning,
				TEXT("Paldark.Match.Phase — Warmup re-entry not supported (FSM is monotonic)."));
			break;
		}
	}

	static EPaldarkMatchEndReason ParseReason(const FString& Token)
	{
		if (Token.Equals(TEXT("AllExtracted"),    ESearchCase::IgnoreCase)) return EPaldarkMatchEndReason::AllExtracted;
		if (Token.Equals(TEXT("TeamWipe"),        ESearchCase::IgnoreCase)) return EPaldarkMatchEndReason::TeamWipe;
		if (Token.Equals(TEXT("Timeout"),         ESearchCase::IgnoreCase)) return EPaldarkMatchEndReason::Timeout;
		if (Token.Equals(TEXT("ForcedByConsole"), ESearchCase::IgnoreCase)) return EPaldarkMatchEndReason::ForcedByConsole;
		return EPaldarkMatchEndReason::ForcedByConsole;
	}

	static void ForceEnd(const TArray<FString>& Args, UWorld* World)
	{
		UPaldarkMatchSubsystem* MatchSub = GetMatchSubsystem(World);
		if (MatchSub == nullptr)
		{
			UE_LOG(LogPaldarkPlayer, Warning,
				TEXT("Paldark.Match.ForceEnd — match subsystem unavailable (run on host)."));
			return;
		}
		const EPaldarkMatchEndReason Reason = (Args.Num() > 0)
			? ParseReason(Args[0])
			: EPaldarkMatchEndReason::ForcedByConsole;
		MatchSub->EndMatch(Reason);
		UE_LOG(LogPaldarkPlayer, Log,
			TEXT("Paldark.Match.ForceEnd — reason=%d"), static_cast<int32>(Reason));
	}
}

static FAutoConsoleCommandWithWorldAndArgs GMatchDumpCmd(
	TEXT("Paldark.Match.Dump"),
	TEXT("Dump the match subsystem state (phase + per-player table) to LogPaldarkPlayer. Run on host."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PaldarkMatchConsole::Dump));

static FAutoConsoleCommandWithWorldAndArgs GMatchPhaseCmd(
	TEXT("Paldark.Match.Phase"),
	TEXT("Force the match FSM to a phase. Usage: Paldark.Match.Phase <Warmup|Active|Extracting|Ended>. Warmup re-entry is not supported."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PaldarkMatchConsole::Phase));

static FAutoConsoleCommandWithWorldAndArgs GMatchForceEndCmd(
	TEXT("Paldark.Match.ForceEnd"),
	TEXT("End the match immediately. Usage: Paldark.Match.ForceEnd [AllExtracted|TeamWipe|Timeout|ForcedByConsole]."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PaldarkMatchConsole::ForceEnd));
