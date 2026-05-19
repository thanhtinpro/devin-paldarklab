// PALDARK W42-43 — AWS backend integration. Plain-old USTRUCTs shared between
// `UPaldarkBackendSubsystem` (issues requests, caches state) and any caller
// that wants to read a session token or react to a fleet allocation.
//
// All three structs are USTRUCT(BlueprintType) so UMG widgets can bind to them
// without round-tripping through a C++ helper — the lobby UI in W44+ reads
// `FPaldarkFleetSpec` directly off the subsystem's BlueprintAssignable
// `OnFleetRequestCompleted` payload.
//
// Wire format note: when `bUseAWSBackend = true`, every struct here also
// round-trips through `FJsonObjectConverter::UStructToJsonObjectString` (and
// the inverse) when talking to the Lambda. Adding a new field to one of these
// structs requires bumping the Lambda's request/response schema too — see
// README § W42-43 → "Backend SRE handoff" for the contract.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "PaldarkBackendTypes.generated.h"

// JWT access token + refresh token returned by Cognito after a successful
// login. Cached on `UPaldarkBackendSubsystem::SessionToken` for the lifetime
// of the GameInstance (i.e. survives map travel) so subsequent
// `RequestHubFleet` / `RequestRaidFleet` calls can attach the bearer token
// as the `Authorization: Bearer <AccessToken>` header.
//
// `ExpirySeconds` is wall-clock seconds since UNIX epoch (server-issued).
// Compared against `FDateTime::UtcNow().ToUnixTimestamp()` by the subsystem's
// 5-minutes-before-expiry refresh timer.
USTRUCT(BlueprintType)
struct PALDARKLAB_API FPaldarkSessionToken
{
	GENERATED_BODY()

	// Short-lived bearer token (~1 hour). Sent as `Authorization: Bearer <x>`
	// on every backend request. Empty until login completes.
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Backend")
	FString AccessToken;

	// Long-lived refresh token (~30 days). Sent as the body of
	// `POST /sessions/refresh` to obtain a fresh AccessToken when the cached
	// one is within 5 minutes of expiry. Empty until login completes.
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Backend")
	FString RefreshToken;

	// Cognito-issued user id (e.g. `us-east-1:abc-123-def`). Stamped onto
	// `FPaldarkFleetSpec::ShardKey` for hub fleets so the player rejoins
	// the same hub shard across multiple raids in one session.
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Backend")
	FString UserId;

	// UNIX epoch seconds at which `AccessToken` becomes invalid. Compared
	// against `FDateTime::UtcNow().ToUnixTimestamp()` by the refresh timer.
	// Defaults to 0 (treated as "no token") so the empty-default struct
	// behaves as logged-out.
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Backend")
	int64 ExpirySeconds = 0;

	// True iff `AccessToken` is non-empty AND we are not within 60s of
	// expiry. Cheap inline check; the 5-minute refresh window is enforced
	// elsewhere — this is just a "do not bother sending the request" gate.
	bool IsValid() const
	{
		if (AccessToken.IsEmpty())
		{
			return false;
		}
		const int64 NowSeconds = FDateTime::UtcNow().ToUnixTimestamp();
		return ExpirySeconds > NowSeconds + 60;
	}

	// Reset every field — used by `Logout` and by the subsystem's
	// `OnLoginFailed` path so a partial token does not leak into the next
	// session.
	void Reset()
	{
		AccessToken.Reset();
		RefreshToken.Reset();
		UserId.Reset();
		ExpirySeconds = 0;
	}
};

// Fleet allocation result returned by the backend's
// `POST /sessions/{hub|raid}` Lambda. The subsystem hands this to
// `UPaldarkNetSubsystem::JoinFleetByFleetSpec` which issues the
// `ClientTravel(IP:Port)` on the local PC.
//
// `FleetTag` distinguishes hub (long-lived 8-player shard) from raid
// (per-match 4-player) — `UPaldarkNetSubsystem` reads the tag to pick which
// pre-travel hook to fire (e.g. clear inventory on raid entry, restore
// hub roster on hub entry).
USTRUCT(BlueprintType)
struct PALDARKLAB_API FPaldarkFleetSpec
{
	GENERATED_BODY()

	// GameLift fleet id (`fleet-1234567890abcdef0` shape). Opaque to the
	// client; backend uses it for telemetry + capacity tracking.
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Backend")
	FString FleetId;

	// Allocated server address (IPv4 dotted-quad). Joined with `Port` into
	// the `ClientTravel` URL.
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Backend")
	FString IP;

	// Allocated server port. UE default is 7777; production fleets pool
	// dynamically across the GameLift port range (33430..33450 by default).
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Backend")
	int32 Port = 0;

	// Fleet kind — `Paldark.Backend.Fleet.Hub` or `Paldark.Backend.Fleet.Raid`
	// today. `Paldark.Backend.Fleet.Reserved` is a placeholder for the W44+
	// private hub feature (paid tier; pre-allocated 8-player rooms).
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Backend")
	FGameplayTag FleetTag;

	// Experience id the fleet was provisioned for (e.g.
	// `PaldarkExperience.PX_HubTown`). The dedicated server reads this from
	// the URL on PreLogin to load the right experience definition.
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Backend")
	FString ExperienceId;

	// Hub shard key — for hub fleets, the backend issues a sticky key so
	// the same user lands on the same hub shard across raids (so their
	// roster + chat history are preserved). Empty for raid fleets (each
	// raid is a fresh shard).
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Backend")
	FString ShardKey;

	// Returns true when the spec has enough information to issue a
	// ClientTravel — IP / Port / FleetTag all set.
	bool IsValid() const
	{
		return !IP.IsEmpty() && Port > 0 && FleetTag.IsValid();
	}

	// Convenience builder used by the mocked fallback path. Returns the
	// "localhost loopback" fleet so the dev quick-test path matches the
	// W40-41 HostListenServer semantics. Real path: backend returns a
	// JSON blob, the subsystem `JsonObjectStringToUStruct` into this
	// struct.
	static FPaldarkFleetSpec MakeLoopback(FGameplayTag InFleetTag, const FString& InExperienceId)
	{
		FPaldarkFleetSpec Spec;
		Spec.FleetId      = TEXT("fleet-loopback");
		Spec.IP           = TEXT("127.0.0.1");
		Spec.Port         = 7777;
		Spec.FleetTag     = InFleetTag;
		Spec.ExperienceId = InExperienceId;
		Spec.ShardKey     = TEXT("loopback");
		return Spec;
	}
};

// Result envelope handed to `OnFleetRequestCompleted` / `OnLoginCompleted`
// callers. Carries either a success payload (FleetSpec or token) or one of
// `Paldark.Backend.Result.Fail.*` tags so UMG can branch on the failure
// type without scraping the message string.
USTRUCT(BlueprintType)
struct PALDARKLAB_API FPaldarkBackendError
{
	GENERATED_BODY()

	// HTTP status code (or 0 for client-side errors — timeout, no
	// network, JSON parse failure). Always populated even on success
	// (200 / 201 / 204).
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Backend")
	int32 StatusCode = 0;

	// `Paldark.Backend.Result.Success` or one of `Paldark.Backend.Result.Fail.*`.
	// Tag-keyed so UMG can `Switch on GameplayTag` directly.
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Backend")
	FGameplayTag ResultTag;

	// Free-form server message (logged + displayed in dev console).
	// On success, may carry an audit id; on failure carries the
	// human-readable reason.
	UPROPERTY(BlueprintReadOnly, Category = "Paldark|Backend")
	FString Message;

	bool IsSuccess() const
	{
		return StatusCode >= 200 && StatusCode < 300;
	}
};
