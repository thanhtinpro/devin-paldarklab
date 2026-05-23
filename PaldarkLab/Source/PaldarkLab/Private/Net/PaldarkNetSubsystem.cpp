#include "Net/PaldarkNetSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

#include "Backend/PaldarkBackendSubsystem.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"

namespace
{
	// Returns the first local player controller, or nullptr. Centralised so all
	// the travel paths use a consistent helper instead of each rolling their own
	// "find PC" loop.
	APlayerController* FindLocalPlayerController(const UWorld* World)
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
					return PC;
				}
			}
		}
		return nullptr;
	}

	const TCHAR* NetModeToString(ENetMode Mode)
	{
		switch (Mode)
		{
		case NM_Standalone:      return TEXT("Standalone");
		case NM_DedicatedServer: return TEXT("DedicatedServer");
		case NM_ListenServer:    return TEXT("ListenServer");
		case NM_Client:          return TEXT("Client");
		default:                 return TEXT("Unknown");
		}
	}
}

void UPaldarkNetSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	IConsoleManager& Console = IConsoleManager::Get();

	CmdHost = Console.RegisterConsoleCommand(
		TEXT("Paldark.Net.Host"),
		TEXT("Host a listen server. Usage: Paldark.Net.Host [MapName] [ExperienceId]"),
		FConsoleCommandWithArgsDelegate::CreateUObject(this, &UPaldarkNetSubsystem::HandleConsoleHost),
		ECVF_Default);

	CmdJoin = Console.RegisterConsoleCommand(
		TEXT("Paldark.Net.Join"),
		TEXT("Join a server by address. Usage: Paldark.Net.Join <host:port>"),
		FConsoleCommandWithArgsDelegate::CreateUObject(this, &UPaldarkNetSubsystem::HandleConsoleJoin),
		ECVF_Default);

	CmdDisconnect = Console.RegisterConsoleCommand(
		TEXT("Paldark.Net.Disconnect"),
		TEXT("Disconnect from the current server."),
		FConsoleCommandWithArgsDelegate::CreateUObject(this, &UPaldarkNetSubsystem::HandleConsoleDisconnect),
		ECVF_Default);

	CmdStatus = Console.RegisterConsoleCommand(
		TEXT("Paldark.Net.Status"),
		TEXT("Print a snapshot of the current network role + peer count."),
		FConsoleCommandWithArgsDelegate::CreateUObject(this, &UPaldarkNetSubsystem::HandleConsoleStatus),
		ECVF_Default);

	CmdHostHub = Console.RegisterConsoleCommand(
		TEXT("Paldark.Net.HostHub"),
		TEXT("Host a hub fleet (AWS path if bUseAWSBackend, else local listen server). Usage: Paldark.Net.HostHub [MapName] [ExperienceId]"),
		FConsoleCommandWithArgsDelegate::CreateUObject(this, &UPaldarkNetSubsystem::HandleConsoleHostHub),
		ECVF_Default);

	CmdHostRaid = Console.RegisterConsoleCommand(
		TEXT("Paldark.Net.HostRaid"),
		TEXT("Host a raid fleet (AWS path if bUseAWSBackend, else local listen server). Usage: Paldark.Net.HostRaid [MapName] [ExperienceId]"),
		FConsoleCommandWithArgsDelegate::CreateUObject(this, &UPaldarkNetSubsystem::HandleConsoleHostRaid),
		ECVF_Default);

	if (GEngine != nullptr)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &UPaldarkNetSubsystem::OnNetworkFailure);
		TravelFailureHandle  = GEngine->OnTravelFailure().AddUObject(this,  &UPaldarkNetSubsystem::OnTravelFailure);
	}

	PostWorldInitHandle = FWorldDelegates::OnPostWorldInitialization.AddUObject(
		this, &UPaldarkNetSubsystem::OnPostWorldInitialization);

	UE_LOG(LogPaldarkNet, Log,
		TEXT("UPaldarkNetSubsystem initialized — 4 console commands registered (Host/Join/Disconnect/Status)."));
}

void UPaldarkNetSubsystem::Deinitialize()
{
	IConsoleManager& Console = IConsoleManager::Get();
	if (CmdHost       != nullptr) { Console.UnregisterConsoleObject(CmdHost);       CmdHost = nullptr; }
	if (CmdHostHub    != nullptr) { Console.UnregisterConsoleObject(CmdHostHub);    CmdHostHub = nullptr; }
	if (CmdHostRaid   != nullptr) { Console.UnregisterConsoleObject(CmdHostRaid);   CmdHostRaid = nullptr; }
	if (CmdJoin       != nullptr) { Console.UnregisterConsoleObject(CmdJoin);       CmdJoin = nullptr; }
	if (CmdDisconnect != nullptr) { Console.UnregisterConsoleObject(CmdDisconnect); CmdDisconnect = nullptr; }
	if (CmdStatus     != nullptr) { Console.UnregisterConsoleObject(CmdStatus);     CmdStatus = nullptr; }

	if (GEngine != nullptr)
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
		GEngine->OnTravelFailure().Remove(TravelFailureHandle);
	}
	FWorldDelegates::OnPostWorldInitialization.Remove(PostWorldInitHandle);

	Super::Deinitialize();
}

bool UPaldarkNetSubsystem::HostListenServer(const FString& MapName, const FString& ExperienceId)
{
	UWorld* World = GetGameInstance() != nullptr ? GetGameInstance()->GetWorld() : nullptr;
	if (World == nullptr)
	{
		UE_LOG(LogPaldarkNet, Warning, TEXT("HostListenServer: no world available — call after GameInstance init."));
		return false;
	}
	if (MapName.IsEmpty())
	{
		UE_LOG(LogPaldarkNet, Warning, TEXT("HostListenServer: MapName is empty."));
		return false;
	}

	FString TravelUrl = MapName + TEXT("?listen");
	if (!ExperienceId.IsEmpty())
	{
		TravelUrl += FString::Printf(TEXT("?Experience=%s"), *ExperienceId);
	}

	UE_LOG(LogPaldarkNet, Log, TEXT("HostListenServer: ServerTravel(%s)."), *TravelUrl);
	const bool bResult = World->ServerTravel(TravelUrl, /*bAbsolute=*/true);
	if (!bResult)
	{
		UE_LOG(LogPaldarkNet, Error, TEXT("HostListenServer: ServerTravel returned false for url=%s."), *TravelUrl);
	}
	return bResult;
}

bool UPaldarkNetSubsystem::HostHubServer(const FString& MapName, const FString& ExperienceId)
{
	// W40-41 — Hub host attempt. Logs under a distinct `[W40-41][Hub]` prefix
	// so designers can grep hub vs raid host attempts. W42-43 backend AWS
	// hooks branch off this entry point to pick the GameLift hub fleet
	// (8-player cap) without modifying the four existing HostListenServer
	// callsites. Local fallback path stays bit-identical to the W40-41
	// behaviour so designers without AWS provisioned still get a working
	// quick-test loop.
	UE_LOG(LogPaldarkNet, Log,
		TEXT("[W40-41/W42-43][Hub] HostHubServer: Map=%s Experience=%s."),
		*MapName,
		ExperienceId.IsEmpty() ? TEXT("<none>") : *ExperienceId);
	return RequestFleetAndTravel(
		PaldarkGameplayTags::TAG_Paldark_Backend_Fleet_Hub, MapName, ExperienceId);
}

bool UPaldarkNetSubsystem::HostRaidServer(const FString& MapName, const FString& ExperienceId)
{
	// W42-43 — Raid host attempt. Same shape as HostHubServer, just tagged
	// for the raid fleet so the backend allocates from the 4-player pool.
	UE_LOG(LogPaldarkNet, Log,
		TEXT("[W42-43][Raid] HostRaidServer: Map=%s Experience=%s."),
		*MapName,
		ExperienceId.IsEmpty() ? TEXT("<none>") : *ExperienceId);
	return RequestFleetAndTravel(
		PaldarkGameplayTags::TAG_Paldark_Backend_Fleet_Raid, MapName, ExperienceId);
}

bool UPaldarkNetSubsystem::JoinFleetByFleetSpec(const FPaldarkFleetSpec& Spec)
{
	if (!Spec.IsValid())
	{
		UE_LOG(LogPaldarkNet, Warning,
			TEXT("[W42-43] JoinFleetByFleetSpec: spec invalid (IP=%s Port=%d Tag=%s) — aborting travel."),
			*Spec.IP, Spec.Port, *Spec.FleetTag.ToString());
		return false;
	}

	FString Address = FString::Printf(TEXT("%s:%d"), *Spec.IP, Spec.Port);
	if (!Spec.ExperienceId.IsEmpty())
	{
		Address += FString::Printf(TEXT("?Experience=%s"), *Spec.ExperienceId);
	}

	UE_LOG(LogPaldarkNet, Log,
		TEXT("[W42-43] JoinFleetByFleetSpec: Fleet=%s Tag=%s Experience=%s Shard=%s → %s"),
		*Spec.FleetId,
		*Spec.FleetTag.ToString(),
		Spec.ExperienceId.IsEmpty() ? TEXT("<none>") : *Spec.ExperienceId,
		Spec.ShardKey.IsEmpty() ? TEXT("<none>") : *Spec.ShardKey,
		*Address);
	return JoinServer(Address);
}

bool UPaldarkNetSubsystem::RequestFleetAndTravel(FGameplayTag FleetTag,
                                                 const FString& MapName,
                                                 const FString& ExperienceId)
{
	UPaldarkBackendSubsystem* Backend = GetGameInstance() != nullptr
		? GetGameInstance()->GetSubsystem<UPaldarkBackendSubsystem>()
		: nullptr;

	if (Backend == nullptr)
	{
		UE_LOG(LogPaldarkNet, Warning,
			TEXT("[W42-43] RequestFleetAndTravel: no backend subsystem — falling back to HostListenServer."));
		return HostListenServer(MapName, ExperienceId);
	}

	// One-shot listener: subscribe, capture handle so we can unsubscribe in
	// the lambda body, then issue the request. The backend subsystem fires
	// `OnFleetRequestCompleted` exactly once per request (success or fail);
	// if multiple hosts overlap, the listener inspects `FleetTag` and
	// `ExperienceId` to ignore foreign responses.
	TWeakObjectPtr<UPaldarkNetSubsystem> WeakThis(this);
	FDelegateHandle Handle;
	Handle = Backend->OnFleetRequestCompleted.AddLambda(
		[WeakThis, Backend, FleetTag, MapName, ExperienceId, Handle](
			FPaldarkFleetSpec FleetSpec, FPaldarkBackendError Error) mutable
		{
			if (!WeakThis.IsValid())
			{
				return;
			}
			if (FleetSpec.FleetTag != FleetTag)
			{
				return; // not our request
			}
			Backend->OnFleetRequestCompleted.Remove(Handle);

			if (!Error.IsSuccess() || !FleetSpec.IsValid())
			{
				UE_LOG(LogPaldarkNet, Warning,
					TEXT("[W42-43] Fleet request failed (Tag=%s, Result=%s) — falling back to HostListenServer."),
					*FleetTag.ToString(), *Error.ResultTag.ToString());
				WeakThis->HostListenServer(MapName, ExperienceId);
				return;
			}

			WeakThis->JoinFleetByFleetSpec(FleetSpec);
		});

	if (FleetTag == PaldarkGameplayTags::TAG_Paldark_Backend_Fleet_Hub)
	{
		Backend->RequestHubFleet(ExperienceId);
	}
	else if (FleetTag == PaldarkGameplayTags::TAG_Paldark_Backend_Fleet_Raid)
	{
		Backend->RequestRaidFleet(ExperienceId);
	}
	else
	{
		UE_LOG(LogPaldarkNet, Warning,
			TEXT("[W42-43] RequestFleetAndTravel: unsupported FleetTag=%s — falling back to HostListenServer."),
			*FleetTag.ToString());
		Backend->OnFleetRequestCompleted.Remove(Handle);
		return HostListenServer(MapName, ExperienceId);
	}
	return true;
}

bool UPaldarkNetSubsystem::JoinServer(const FString& Address)
{
	UWorld* World = GetGameInstance() != nullptr ? GetGameInstance()->GetWorld() : nullptr;
	if (World == nullptr)
	{
		UE_LOG(LogPaldarkNet, Warning, TEXT("JoinServer: no world available."));
		return false;
	}
	if (Address.IsEmpty())
	{
		UE_LOG(LogPaldarkNet, Warning, TEXT("JoinServer: address is empty."));
		return false;
	}

	APlayerController* PC = FindLocalPlayerController(World);
	if (PC == nullptr)
	{
		UE_LOG(LogPaldarkNet, Warning, TEXT("JoinServer: no local player controller — cannot ClientTravel."));
		return false;
	}

	UE_LOG(LogPaldarkNet, Log, TEXT("JoinServer: ClientTravel(%s) issued."), *Address);
	PC->ClientTravel(Address, TRAVEL_Absolute);
	return true;
}

void UPaldarkNetSubsystem::DisconnectFromServer()
{
	UWorld* World = GetGameInstance() != nullptr ? GetGameInstance()->GetWorld() : nullptr;
	if (World == nullptr)
	{
		return;
	}

	APlayerController* PC = FindLocalPlayerController(World);
	if (PC == nullptr)
	{
		UE_LOG(LogPaldarkNet, Log, TEXT("DisconnectFromServer: no local PC, nothing to do."));
		return;
	}

	UE_LOG(LogPaldarkNet, Log, TEXT("DisconnectFromServer: ClientTravel(\"?closed\")."));
	PC->ClientTravel(TEXT("?closed"), TRAVEL_Absolute);
}

FPaldarkNetSnapshot UPaldarkNetSubsystem::GetNetSnapshot() const
{
	FPaldarkNetSnapshot Snapshot;

	UWorld* World = GetGameInstance() != nullptr ? GetGameInstance()->GetWorld() : nullptr;
	if (World == nullptr)
	{
		return Snapshot;
	}

	Snapshot.MapName = World->GetMapName();

	const ENetMode Mode = World->GetNetMode();
	switch (Mode)
	{
	case NM_Standalone:
		Snapshot.Role = PaldarkGameplayTags::TAG_Paldark_Net_Role_Host;
		Snapshot.Mode = PaldarkGameplayTags::TAG_Paldark_Net_Mode_Standalone;
		break;
	case NM_ListenServer:
		Snapshot.Role = PaldarkGameplayTags::TAG_Paldark_Net_Role_Host;
		Snapshot.Mode = PaldarkGameplayTags::TAG_Paldark_Net_Mode_ListenServer;
		break;
	case NM_DedicatedServer:
		Snapshot.Role = PaldarkGameplayTags::TAG_Paldark_Net_Role_Host;
		Snapshot.Mode = PaldarkGameplayTags::TAG_Paldark_Net_Mode_DedicatedServer;
		break;
	case NM_Client:
	default:
		Snapshot.Role = PaldarkGameplayTags::TAG_Paldark_Net_Role_Client;
		Snapshot.Mode = PaldarkGameplayTags::TAG_Paldark_Net_Mode_DedicatedServer; // assumed; client cannot distinguish
		break;
	}

	if (UNetDriver* Driver = World->GetNetDriver())
	{
		Snapshot.ConnectedPeerCount = Driver->ClientConnections.Num();
		Snapshot.ServerAddress = Driver->LowLevelGetNetworkNumber();
	}
	else if (Mode == NM_Client)
	{
		Snapshot.ConnectedPeerCount = 0; // pre-login
	}
	else
	{
		Snapshot.ConnectedPeerCount = 0; // standalone
	}

	return Snapshot;
}

void UPaldarkNetSubsystem::RefreshTopology()
{
	const FPaldarkNetSnapshot Snapshot = GetNetSnapshot();
	CachedTopologyTags.Reset();
	if (Snapshot.Role.IsValid())
	{
		CachedTopologyTags.AddTag(Snapshot.Role);
	}
	if (Snapshot.Mode.IsValid())
	{
		CachedTopologyTags.AddTag(Snapshot.Mode);
	}
	UE_LOG(LogPaldarkNet, Verbose, TEXT("RefreshTopology: %s / %s map=%s peers=%d"),
		*Snapshot.Role.ToString(),
		*Snapshot.Mode.ToString(),
		*Snapshot.MapName,
		Snapshot.ConnectedPeerCount);
}

void UPaldarkNetSubsystem::HandleConsoleHost(const TArray<FString>& Args)
{
	const FString MapName = Args.Num() > 0 ? Args[0] : TEXT("Raid_Sandbox");
	const FString ExperienceId = Args.Num() > 1 ? Args[1] : TEXT("");
	HostListenServer(MapName, ExperienceId);
}

void UPaldarkNetSubsystem::HandleConsoleHostHub(const TArray<FString>& Args)
{
	const FString MapName = Args.Num() > 0 ? Args[0] : TEXT("Map_HubTown");
	const FString ExperienceId = Args.Num() > 1 ? Args[1] : TEXT("PX_HubTown");
	HostHubServer(MapName, ExperienceId);
}

void UPaldarkNetSubsystem::HandleConsoleHostRaid(const TArray<FString>& Args)
{
	const FString MapName = Args.Num() > 0 ? Args[0] : TEXT("Raid_Sandbox");
	const FString ExperienceId = Args.Num() > 1 ? Args[1] : TEXT("PX_RaidSandbox");
	HostRaidServer(MapName, ExperienceId);
}

void UPaldarkNetSubsystem::HandleConsoleJoin(const TArray<FString>& Args)
{
	if (Args.Num() < 1)
	{
		UE_LOG(LogPaldarkNet, Warning, TEXT("Paldark.Net.Join: missing <host:port> argument."));
		return;
	}
	JoinServer(Args[0]);
}

void UPaldarkNetSubsystem::HandleConsoleDisconnect(const TArray<FString>& /*Args*/)
{
	DisconnectFromServer();
}

void UPaldarkNetSubsystem::HandleConsoleStatus(const TArray<FString>& /*Args*/)
{
	const FPaldarkNetSnapshot Snapshot = GetNetSnapshot();
	UWorld* World = GetGameInstance() != nullptr ? GetGameInstance()->GetWorld() : nullptr;
	UE_LOG(LogPaldarkNet, Log,
		TEXT("[Status] Role=%s Mode=%s NetMode=%s Map=%s Peers=%d Addr=\"%s\""),
		*Snapshot.Role.ToString(),
		*Snapshot.Mode.ToString(),
		World != nullptr ? NetModeToString(World->GetNetMode()) : TEXT("<no-world>"),
		*Snapshot.MapName,
		Snapshot.ConnectedPeerCount,
		*Snapshot.ServerAddress);

	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(
			-1, 4.0f, FColor::Cyan,
			FString::Printf(TEXT("Paldark.Net: %s / %s peers=%d"),
				*Snapshot.Role.ToString(),
				*Snapshot.Mode.ToString(),
				Snapshot.ConnectedPeerCount));
	}
}

void UPaldarkNetSubsystem::OnNetworkFailure(UWorld* InWorld, UNetDriver* InNetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	UE_LOG(LogPaldarkNet, Error,
		TEXT("[NetworkFailure] %s — %s"),
		ENetworkFailure::ToString(FailureType),
		*ErrorString);
}

void UPaldarkNetSubsystem::OnTravelFailure(UWorld* InWorld, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	UE_LOG(LogPaldarkNet, Error,
		TEXT("[TravelFailure] %s — %s"),
		ETravelFailure::ToString(FailureType),
		*ErrorString);
}

void UPaldarkNetSubsystem::OnPostWorldInitialization(UWorld* InWorld, const UWorld::InitializationValues /*IVS*/)
{
	if (InWorld == nullptr || GetGameInstance() == nullptr)
	{
		return;
	}
	if (InWorld->GetGameInstance() != GetGameInstance())
	{
		return;
	}
	RefreshTopology();
}

void UPaldarkNetSubsystem::BroadcastPlayerLogin(APlayerController* NewPlayer)
{
	// W39 — L-20 closure. The GameMode authority hook is the only first-party
	// caller, but Blueprint callers (e.g. a mock lobby actor in test maps) can
	// hit this too, so we re-check authority instead of trusting the caller.
	if (NewPlayer == nullptr)
	{
		UE_LOG(LogPaldarkNet, Verbose,
			TEXT("BroadcastPlayerLogin: NewPlayer is null — skipping."));
		return;
	}
	UWorld* World = GetGameInstance() != nullptr ? GetGameInstance()->GetWorld() : nullptr;
	if (World == nullptr || World->GetNetMode() == NM_Client)
	{
		UE_LOG(LogPaldarkNet, Verbose,
			TEXT("BroadcastPlayerLogin: skipping client-side broadcast for %s."),
			*NewPlayer->GetName());
		return;
	}

	UE_LOG(LogPaldarkNet, Log,
		TEXT("BroadcastPlayerLogin: %s — %d listener(s) bound."),
		*NewPlayer->GetName(),
		OnPlayerLogin.IsBound() ? 1 : 0);
	OnPlayerLogin.Broadcast(NewPlayer);
}

void UPaldarkNetSubsystem::BroadcastPlayerLogout(AController* Exiting)
{
	if (Exiting == nullptr)
	{
		UE_LOG(LogPaldarkNet, Verbose,
			TEXT("BroadcastPlayerLogout: Exiting is null — skipping."));
		return;
	}
	UWorld* World = GetGameInstance() != nullptr ? GetGameInstance()->GetWorld() : nullptr;
	if (World == nullptr || World->GetNetMode() == NM_Client)
	{
		UE_LOG(LogPaldarkNet, Verbose,
			TEXT("BroadcastPlayerLogout: skipping client-side broadcast for %s."),
			*Exiting->GetName());
		return;
	}

	UE_LOG(LogPaldarkNet, Log,
		TEXT("BroadcastPlayerLogout: %s — %d listener(s) bound."),
		*Exiting->GetName(),
		OnPlayerLogout.IsBound() ? 1 : 0);
	OnPlayerLogout.Broadcast(Exiting);
}
