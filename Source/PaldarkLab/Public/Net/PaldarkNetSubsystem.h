// PALDARK W14-15 — Net subsystem (login flow basic + listen/dedicated travel).
//
// `UPaldarkNetSubsystem` is a UGameInstanceSubsystem so it lives for the full
// lifetime of the application (across map travels) and can be reached from any
// console command / blueprint / actor via `GetGameInstance()->GetSubsystem<...>`.
//
// W14-15 scope (matches roadmap "Login flow basic (no auth). Listen server →
// switch dedicated. 4 player connect."):
//   - HostListenServer: ServerTravel into a map with `?listen`. Used by the
//     dev who wants to spin up a quick 2-4 player session without launching a
//     separate process.
//   - JoinServer: ClientTravel to an `IP:Port` address. Mirrors the engine's
//     `open <ip>` console but goes through this subsystem so we can log
//     connection attempts, surface failures via `LogPaldarkNet`, and (in Q4)
//     plug in AWS Cognito JWT as a precondition without changing call sites.
//   - DisconnectFromServer: brings the client back to the front-end map.
//   - GetNetSnapshot: cheap struct describing role/mode/peer count/server
//     address. Used by `Paldark.Net.Status` and HUD widgets.
//
// What this is NOT (deferred to Q2/Q4 per roadmap):
//   - Steam / EOS / Cognito auth (W42-43 — Q4 backend AWS week).
//   - Session find / matchmaking (W14-15 is `open <ip>` direct join only).
//   - Lobby UMG widget (W14-15 ships the C++ entry points; widget is Q3).
//   - Server-side rewind / lag compensation (W16-17 dedicated PR).
//
// All four entry points + a status query are exposed as console commands
// (`Paldark.Net.Host`, `Paldark.Net.Join`, `Paldark.Net.Disconnect`,
// `Paldark.Net.Status`) registered in Initialize / unregistered in Deinitialize
// so the subsystem stays self-contained.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "Backend/PaldarkBackendTypes.h"

#include "PaldarkNetSubsystem.generated.h"

class AController;
class APlayerController;
class IConsoleObject;
class UPaldarkBackendSubsystem;
class UWorld;

// W39 — L-20 closure. BlueprintAssignable delegates fired on every server-side
// player join / leave. UMG lobby widgets (W40-41 hub town brief room, marketplace
// player list, etc.) subscribe to these via the standard Blueprint event-binding
// pattern instead of reaching for GameMode `PreLogin / PostLogin` overrides
// (which require subclassing the GameMode + risk colliding with extraction /
// hub-town subclass logic).
//
// Server-side broadcast — clients see nothing fire because the subsystem only
// reaches `BroadcastPlayer*` from the GameMode override on the authority. For
// the client-side analogue (e.g. "another player joined my session") use the
// existing GameState `PlayerArray` REPNOTIFY in W14-15.
//
// The login delegate carries `APlayerController*` (already non-null by the time
// the GameMode's `PostLogin` runs). The logout delegate carries `AController*`
// because UE's `Logout` callback widens to AController (the controller may be
// an AIController in seamless travel edge cases).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPaldarkPlayerLoginSignature, APlayerController*, NewPlayer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPaldarkPlayerLogoutSignature, AController*, Exiting);

USTRUCT(BlueprintType)
struct FPaldarkNetSnapshot
{
	GENERATED_BODY()

	// Engine-level role of the local machine — derived from GetNetMode() at
	// snapshot time. One of Paldark.Net.Role.{Host,Client} (Standalone returns
	// Host).
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag Role;

	// Topology tag — one of Paldark.Net.Mode.{Standalone,ListenServer,
	// DedicatedServer}. Distinguishes a host-with-local-player (ListenServer)
	// from a headless build (DedicatedServer).
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag Mode;

	// Authoritative peer count read from the world's NetDriver (server-side)
	// or 1 on client. -1 if the world has no net driver yet (pre-login).
	UPROPERTY(BlueprintReadOnly)
	int32 ConnectedPeerCount = -1;

	// Address we are connected to (client) or listening on (server). Empty if
	// standalone.
	UPROPERTY(BlueprintReadOnly)
	FString ServerAddress;

	// Map currently loaded. Mirrors `UWorld::GetMapName()` short form.
	UPROPERTY(BlueprintReadOnly)
	FString MapName;
};

UCLASS()
class PALDARKLAB_API UPaldarkNetSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Host a listen server by ServerTravelling into `MapName` with `?listen`
	// appended. Optionally appends `?Experience=<id>` so the server side picks
	// up the right experience definition on InitGame. Returns true if the
	// travel request was issued (does not wait for completion).
	UFUNCTION(BlueprintCallable, Category = "Paldark|Net")
	bool HostListenServer(const FString& MapName, const FString& ExperienceId = TEXT(""));

	// W40-41 / W42-43 — Hub-flavoured wrapper.
	//
	// W40-41 shipped the wrapper around `HostListenServer` so designers could
	// grep hub-host attempts under a distinct `[W40-41][Hub]` log prefix.
	// W42-43 extends the same entry point with an AWS backend branch:
	//   - If `UPaldarkBackendSettings::bUseAWSBackend == true`, route through
	//     `UPaldarkBackendSubsystem::RequestHubFleet` and `ClientTravel` to
	//     the returned `FleetSpec.IP:Port`. The local PIE process becomes a
	//     pure client; the dedicated server is hosted on the AWS-allocated
	//     GameLift fleet.
	//   - Otherwise (default), fall through to `HostListenServer(MapName)`
	//     which preserves the W40-41 dev quick-test path exactly.
	// `ExperienceId` is forwarded to the backend so the right experience
	// definition is loaded on the dedicated server.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Net")
	bool HostHubServer(const FString& MapName, const FString& ExperienceId = TEXT(""));

	// W42-43 — Raid-flavoured wrapper around the host path. Identical to
	// `HostHubServer` but tagged `Paldark.Backend.Fleet.Raid` so the
	// backend allocates from the raid fleet (4-player cap, per-match
	// shard). Fallback path: `HostListenServer(MapName)` so the dev quick-
	// test loop still works without AWS provisioning. Designers reach
	// this via `Paldark.Net.HostRaid` or via the briefing-room interact
	// handler.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Net")
	bool HostRaidServer(const FString& MapName, const FString& ExperienceId = TEXT(""));

	// W42-43 — Travel into a fleet returned by the backend. Builds the
	// `IP:Port` string from the spec, optionally appends
	// `?Experience=<id>` so the dedicated server picks up the right
	// experience on InitGame, and issues `JoinServer`. Validates the
	// spec via `FPaldarkFleetSpec::IsValid()` first — invalid specs log
	// + early-out instead of issuing a malformed travel.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Net")
	bool JoinFleetByFleetSpec(const FPaldarkFleetSpec& Spec);

	// Issue a `ClientTravel(Address, TRAVEL_Absolute)` to the given
	// `host:port` string. Logs the attempt; engine surface delivery errors via
	// the network failure delegate which we relay to `LogPaldarkNet`.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Net")
	bool JoinServer(const FString& Address);

	// Disconnect from the current server (no-op if already standalone) by
	// ClientTravelling to a `?closed` URL. The next call to HostListenServer
	// or JoinServer brings us back online.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Net")
	void DisconnectFromServer();

	// Cheap query — reads the current world's NetDriver and ENetMode. Safe to
	// call from console / HUD tick.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Net")
	FPaldarkNetSnapshot GetNetSnapshot() const;

	// Re-evaluates GetNetMode() and stores the result in `CachedTopologyTags`
	// so other systems can read tag-keyed state instead of duplicating the
	// switch on ENetMode. Called automatically on WorldChanged (post-travel).
	void RefreshTopology();

	// Tag accessors. Empty until RefreshTopology() has run at least once.
	const FGameplayTagContainer& GetCachedTopologyTags() const { return CachedTopologyTags; }

	// W39 — L-20 closure. Server-side broadcast called by
	// `APaldarkGameModeBase::PostLogin` / `Logout`. Early-out on null or on
	// non-authority worlds; the GameMode hooks only fire on server but the
	// extra guard means a future Blueprint caller cannot accidentally fire
	// a client-side broadcast that wouldn't reach any listeners anyway.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Net")
	void BroadcastPlayerLogin(APlayerController* NewPlayer);

	UFUNCTION(BlueprintCallable, Category = "Paldark|Net")
	void BroadcastPlayerLogout(AController* Exiting);

	// BlueprintAssignable so UMG widgets / Blueprint actors can wire
	// `Bind Event to OnPlayerLogin` without subclassing the GameMode.
	// Multicast: any number of listeners. Server-only firing.
	UPROPERTY(BlueprintAssignable, Category = "Paldark|Net")
	FPaldarkPlayerLoginSignature OnPlayerLogin;

	UPROPERTY(BlueprintAssignable, Category = "Paldark|Net")
	FPaldarkPlayerLogoutSignature OnPlayerLogout;

private:
	// Console command handlers — thin wrappers that parse FString arg arrays
	// and forward to the typed entry points above.
	void HandleConsoleHost(const TArray<FString>& Args);
	void HandleConsoleHostHub(const TArray<FString>& Args);
	void HandleConsoleHostRaid(const TArray<FString>& Args);
	void HandleConsoleJoin(const TArray<FString>& Args);
	void HandleConsoleDisconnect(const TArray<FString>& Args);
	void HandleConsoleStatus(const TArray<FString>& Args);

	// W42-43 — Shared body of HostHubServer / HostRaidServer. Looks up the
	// backend subsystem, picks the right RequestFleet method based on
	// `FleetTag`, and binds a one-shot listener that calls
	// `JoinFleetByFleetSpec`. Returns true if the request was issued
	// (either via backend HTTP or local fallback path) — does NOT wait
	// for completion.
	bool RequestFleetAndTravel(FGameplayTag FleetTag,
	                           const FString& MapName,
	                           const FString& ExperienceId);

	// Engine network failure relay. Maps `ENetworkFailure` to a log line under
	// `LogPaldarkNet` so a designer reading the editor log can see why a
	// `Paldark.Net.Join` attempt fell over without scraping `LogNet`.
	void OnNetworkFailure(UWorld* InWorld, UNetDriver* InNetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	// Travel failure relay — same shape as OnNetworkFailure but for the
	// ClientTravel / ServerTravel path.
	void OnTravelFailure(UWorld* InWorld, ETravelFailure::Type FailureType, const FString& ErrorString);

	// Re-derives topology tags on every PostWorldInitialization so that
	// post-travel state (especially client joining a dedicated server) ends
	// up tagged correctly without callers needing to remember to refresh.
	void OnPostWorldInitialization(UWorld* InWorld, const UWorld::InitializationValues IVS);

	// Console command handles — owned by IConsoleManager but unregistered in
	// Deinitialize so reloading the subsystem doesn't double-register.
	IConsoleObject* CmdHost = nullptr;
	IConsoleObject* CmdHostHub = nullptr;
	IConsoleObject* CmdHostRaid = nullptr;
	IConsoleObject* CmdJoin = nullptr;
	IConsoleObject* CmdDisconnect = nullptr;
	IConsoleObject* CmdStatus = nullptr;

	// Container populated by RefreshTopology(). Read-only after that point.
	FGameplayTagContainer CachedTopologyTags;

	// Delegate handles so we can unbind cleanly in Deinitialize.
	FDelegateHandle NetworkFailureHandle;
	FDelegateHandle TravelFailureHandle;
	FDelegateHandle PostWorldInitHandle;
};
