// PALDARK W42-43 — AWS backend subsystem (client side).
//
// `UPaldarkBackendSubsystem` is a `UGameInstanceSubsystem` (same lifetime as
// `UPaldarkNetSubsystem` — survives map travel) that wraps the HTTP calls to
// the Lambda + Cognito layer:
//   - Login: exchange (email, password) for a JWT access token + refresh
//     token. Cached on `SessionToken`.
//   - Logout: clear cached token; backend optional revoke endpoint.
//   - RequestHubFleet / RequestRaidFleet: ask the backend which dedicated
//     server to ClientTravel into. Returns a `FPaldarkFleetSpec` via the
//     `OnFleetRequestCompleted` delegate.
//   - RefreshToken: invoked automatically `TokenRefreshLeadSeconds` before
//     expiry (timer set in `Login`); also callable manually for tests.
//   - Status: read-only snapshot for `Paldark.Backend.Status` console cmd.
//
// Local-only fallback path (`bUseAWSBackend = false`): every method returns
// synthetic data on the next tick (`FPaldarkFleetSpec::MakeLoopback()`,
// `FPaldarkSessionToken{}` cleared, etc.). Keeps the W40-41 quick-test loop
// working when the AWS provisioning is not in place. Mirror of how
// `HostListenServer` is the dev's go-to for "just run a session right now".
//
// What this is NOT (deferred):
//   - Cognito-hosted browser login flow (deep-link callback): W46+. This PR
//     ships plain-text login for dev only; logs WARN on every call.
//   - Server-side fleet register/deregister (dedicated server boots, tells
//     Lambda "I'm online"): separate Q4 week, lives on the server side.
//   - mod.io / Steam Workshop / EOS integration: separate Q4 weeks.
//   - Telemetry to CloudWatch / Sentry: any subsystem can log to its own
//     category; this PR keeps everything on `LogPaldarkBackend`.
//
// All four API methods are non-blocking. They return immediately and fire a
// BlueprintAssignable delegate (`OnLoginCompleted` / `OnFleetRequestCompleted`
// / `OnTokenRefreshed` / `OnBackendError`) on completion so UMG can bind
// directly without polling.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Queue.h"
#include "GameplayTagContainer.h"
#include "Interfaces/IHttpRequest.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "Backend/PaldarkBackendTypes.h"

#include "PaldarkBackendSubsystem.generated.h"

class IConsoleObject;
class IHttpResponse;
class UPaldarkBackendSettings;

// Login completion delegate. `Success` carries the empty struct on failure;
// inspect `Error.ResultTag` for the failure category.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPaldarkBackendLoginSignature,
	FPaldarkSessionToken, SessionToken,
	FPaldarkBackendError, Error);

// Fleet request completion delegate. Inspect `Error.ResultTag` for failures.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPaldarkBackendFleetRequestSignature,
	FPaldarkFleetSpec, FleetSpec,
	FPaldarkBackendError, Error);

// Token refresh completion delegate. Fires both when the timer-driven
// refresh succeeds and when a manual `RefreshToken()` finishes.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPaldarkBackendTokenRefreshedSignature,
	FPaldarkSessionToken, NewSessionToken);

// Generic backend error broadcast — fires alongside the specific delegate
// (login / fleet) for callers that just want one "any backend call failed"
// hook to update a toast widget.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPaldarkBackendErrorSignature,
	FPaldarkBackendError, Error);

UCLASS()
class PALDARKLAB_API UPaldarkBackendSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Exchange (email, password) for a JWT access token. Async; fires
	// `OnLoginCompleted` on completion. No retries — login failures are
	// never retried (security). On mock path: returns a synthetic token
	// with `UserId = "mock-user"` on next tick.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Backend")
	void Login(const FString& Email, const FString& Password);

	// Clear cached token. Optional backend revoke call if `bUseAWSBackend`.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Backend")
	void Logout();

	// Request a hub fleet allocation. Async; fires
	// `OnFleetRequestCompleted` with `FleetTag = Paldark.Backend.Fleet.Hub`.
	// Retries up to `MaxRetries` times on 5xx / network failure with
	// `RetryBackoffSeconds` linear backoff. On mock path: returns
	// `FPaldarkFleetSpec::MakeLoopback(Hub)` on next tick.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Backend")
	void RequestHubFleet(const FString& ExperienceId = TEXT(""));

	// Request a raid fleet allocation. Identical shape to
	// `RequestHubFleet` with `FleetTag = Paldark.Backend.Fleet.Raid`.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Backend")
	void RequestRaidFleet(const FString& ExperienceId = TEXT(""));

	// Use the cached refresh token to obtain a new access token. Fires
	// `OnTokenRefreshed` on success or `OnBackendError` on failure.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Backend")
	void RefreshToken();

	// Read-only snapshot of the subsystem state. Reads cached token,
	// in-flight request count, current state tag, current settings
	// (`bUseAWSBackend`, `LambdaApiBaseUrl`). Returned as a multi-line
	// log string for the `Paldark.Backend.Status` console command.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Backend")
	FString GetStatusReport() const;

	// Returns the current session token (empty struct if logged out).
	UFUNCTION(BlueprintCallable, Category = "Paldark|Backend")
	const FPaldarkSessionToken& GetSessionToken() const { return SessionToken; }

	// Returns the current backend state tag —
	// `Paldark.Backend.State.{LoggedOut, Authenticating, LoggedIn, Refreshing}`.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Backend")
	FGameplayTag GetStateTag() const { return StateTag; }

	// Returns true iff the cached token is valid (non-empty + not within
	// 60s of expiry). Cheap inline check; the 5-minute refresh window is
	// enforced by the timer separately.
	UFUNCTION(BlueprintCallable, Category = "Paldark|Backend")
	bool IsLoggedIn() const { return SessionToken.IsValid(); }

	// Broadcast on login completion. Subscribers receive either a valid
	// token + Success result tag, or an empty token + Fail.* result tag.
	UPROPERTY(BlueprintAssignable, Category = "Paldark|Backend")
	FPaldarkBackendLoginSignature OnLoginCompleted;

	// Broadcast on fleet request completion. Subscribers receive either
	// a valid FleetSpec + Success result tag, or an empty FleetSpec +
	// Fail.* result tag.
	UPROPERTY(BlueprintAssignable, Category = "Paldark|Backend")
	FPaldarkBackendFleetRequestSignature OnFleetRequestCompleted;

	// Broadcast on token refresh completion (success only — failure path
	// fires OnBackendError + transitions state to LoggedOut).
	UPROPERTY(BlueprintAssignable, Category = "Paldark|Backend")
	FPaldarkBackendTokenRefreshedSignature OnTokenRefreshed;

	// Generic broadcast — fires alongside the specific delegate (login,
	// fleet, refresh) on every failure. UMG toast widgets bind to this
	// single hook for "show error banner" without binding three
	// separate delegates.
	UPROPERTY(BlueprintAssignable, Category = "Paldark|Backend")
	FPaldarkBackendErrorSignature OnBackendError;

private:
	// Console command handlers — thin wrappers that parse FString arg
	// arrays and forward to the typed entry points above.
	void HandleConsoleLogin(const TArray<FString>& Args);
	void HandleConsoleRequestHubFleet(const TArray<FString>& Args);
	void HandleConsoleRequestRaidFleet(const TArray<FString>& Args);
	void HandleConsoleStatus(const TArray<FString>& Args);

	// HTTP path: build + send a request to `<LambdaApiBaseUrl>/<Path>` with
	// the given JSON body. Stores the resulting `IHttpRequest` in
	// `InFlightRequests` and binds `Callback` to fire when the response is
	// received (or timeout / network failure).
	void IssueHttpRequest(const FString& Verb,
	                      const FString& Path,
	                      const FString& JsonBody,
	                      bool bAuthenticated,
	                      TFunction<void(FHttpRequestPtr, FHttpResponsePtr, bool)> Callback);

	// Mock fallback path: schedule a next-tick callback that returns the
	// loopback fleet spec. Used when `bUseAWSBackend = false` OR
	// `LambdaApiBaseUrl` is empty.
	void IssueMockedFleetResponse(FGameplayTag InFleetTag, const FString& InExperienceId);

	// Mock fallback for login — returns a synthetic token on next tick.
	void IssueMockedLoginResponse(const FString& Email);

	// Common helper: convert HTTP response to FPaldarkBackendError, log
	// under `LogPaldarkBackend`, set state tag, fire OnBackendError.
	FPaldarkBackendError BuildErrorFromResponse(FHttpResponsePtr Response, bool bConnectedSuccessfully) const;

	// Set internal state tag + log transition. Centralised so every
	// transition fires the same log line shape.
	void TransitionState(FGameplayTag NewState);

	// Schedule the periodic token-refresh timer. Cancels any existing
	// timer first. Computes the delay as
	// `(ExpirySeconds - UtcNow - TokenRefreshLeadSeconds)`. If the delay
	// would be negative (e.g. token already expired), the refresh runs
	// immediately.
	void ScheduleTokenRefresh();

	// Cancel + clear the refresh timer.
	void CancelTokenRefresh();

	// Settings accessor — reads CDO on every call so designer-side ini
	// edits take effect without restarting.
	const UPaldarkBackendSettings* GetSettings() const;

	// Cached session token. Empty after Logout() / Initialize().
	UPROPERTY(Transient)
	FPaldarkSessionToken SessionToken;

	// Current state tag —
	// `Paldark.Backend.State.{LoggedOut, Authenticating, LoggedIn, Refreshing}`.
	UPROPERTY(Transient)
	FGameplayTag StateTag;

	// In-flight HTTP requests, tracked so we can cancel them on Deinitialize.
	// Keyed by request id (process-local counter) for log readability.
	TArray<TSharedRef<IHttpRequest, ESPMode::ThreadSafe>> InFlightRequests;

	// Console command handles — owned by IConsoleManager but unregistered
	// in Deinitialize.
	IConsoleObject* CmdLogin              = nullptr;
	IConsoleObject* CmdRequestHubFleet    = nullptr;
	IConsoleObject* CmdRequestRaidFleet   = nullptr;
	IConsoleObject* CmdStatus             = nullptr;

	// Token refresh timer handle — set on login, cancelled on logout.
	FTimerHandle TokenRefreshTimer;
};
