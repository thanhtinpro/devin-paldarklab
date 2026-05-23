// PALDARK W42-43 — UDeveloperSettings carrying the AWS backend knobs.
//
// Settings are read by `UPaldarkBackendSubsystem` on every request (so
// developer-side ini overrides take effect without restarting the editor) and
// surfaced in Project Settings → Plugins → Paldark Backend so designers can
// twiddle the values without leaving the editor.
//
// Why UDeveloperSettings (not [/Script/Engine.Engine] direct):
//   - Editor-side UI for free — every UPROPERTY shows up under the matching
//     category in Project Settings.
//   - Per-developer `DefaultGame.ini` overrides are first-class; the file
//     ships with `bUseAWSBackend = false` so cold-clone devs / CI hit the
//     mocked fallback path. A developer flips it locally without committing.
//   - CDO validation: a missing field shows up as "default used" instead of
//     silently being an empty string.
//
// The actual Lambda endpoints, Cognito pool ids, IAM roles, and fleet
// allocation policy are owned by SRE and provisioned via Terraform; this
// settings struct is the *client-side* view of those provisioned resources.
// See README § W42-43 → "Backend SRE handoff" for the contract.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "PaldarkBackendSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Paldark Backend"))
class PALDARKLAB_API UPaldarkBackendSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPaldarkBackendSettings();

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	// Master switch. When false (the shipped default), `UPaldarkBackendSubsystem`
	// returns synthetic `FPaldarkFleetSpec::MakeLoopback()` responses on the
	// next tick and skips all HTTP. Local-only quick-test path that keeps
	// W40-41 / W14-15 listen-server semantics unchanged.
	//
	// When true, the subsystem issues HTTP against `LambdaApiBaseUrl` and
	// blocks `HostHubServer` / `HostRaidServer` until the backend returns a
	// FleetSpec.
	UPROPERTY(EditAnywhere, Config, Category = "Backend|AWS",
		meta = (DisplayName = "Use AWS backend",
		        ToolTip = "When false, all backend calls return synthetic loopback responses. Flip to true once SRE provisions the Lambda + Cognito pool."))
	bool bUseAWSBackend = false;

	// AWS region the Lambda + Cognito pool live in. Used for log strings only;
	// the actual region is encoded into `LambdaApiBaseUrl`.
	UPROPERTY(EditAnywhere, Config, Category = "Backend|AWS",
		meta = (DisplayName = "AWS region"))
	FString AWSRegion = TEXT("ap-northeast-1");

	// Lambda API Gateway base URL. Empty by default — flipping
	// `bUseAWSBackend` to true with an empty URL is a configuration error
	// the subsystem logs at Warning level and returns the loopback spec
	// instead.
	UPROPERTY(EditAnywhere, Config, Category = "Backend|AWS",
		meta = (DisplayName = "Lambda API base URL",
		        ToolTip = "e.g. https://abc123.execute-api.ap-northeast-1.amazonaws.com/prod. Trailing slash optional."))
	FString LambdaApiBaseUrl;

	// Cognito User Pool id. Logged + sent as the `X-Paldark-Pool` header
	// on every authenticated request so the Lambda can route to the right
	// pool if multiple pools coexist (dev / staging / prod).
	UPROPERTY(EditAnywhere, Config, Category = "Backend|AWS",
		meta = (DisplayName = "Cognito User Pool ID"))
	FString CognitoUserPoolId;

	// Cognito App Client id. Sent in the login body so Cognito can issue
	// a token bound to this client.
	UPROPERTY(EditAnywhere, Config, Category = "Backend|AWS",
		meta = (DisplayName = "Cognito App Client ID"))
	FString CognitoAppClientId;

	// HTTP request timeout in seconds. Defaults to 15 to absorb Lambda
	// cold start (typical p99 ~5s on first invocation after idle). Lower
	// in production once SRE wires the scheduled warm-up ping.
	UPROPERTY(EditAnywhere, Config, Category = "Backend|HTTP",
		meta = (DisplayName = "Request timeout (seconds)",
		        ClampMin = 1, ClampMax = 120))
	float RequestTimeoutSeconds = 15.f;

	// Number of automatic retries on 5xx / network failure. Login + logout
	// are NEVER retried (security — re-sending credentials on a 5xx is a
	// vector). Only fleet requests honour the retry count.
	UPROPERTY(EditAnywhere, Config, Category = "Backend|HTTP",
		meta = (DisplayName = "Max retries (fleet requests only)",
		        ClampMin = 0, ClampMax = 10))
	int32 MaxRetries = 3;

	// Linear backoff between retries (seconds). The Nth retry waits
	// `N * RetryBackoffSeconds` so retry storms during a Lambda outage
	// taper off naturally.
	UPROPERTY(EditAnywhere, Config, Category = "Backend|HTTP",
		meta = (DisplayName = "Retry backoff (seconds)",
		        ClampMin = 0.f, ClampMax = 60.f))
	float RetryBackoffSeconds = 2.f;

	// Refresh window — how many seconds before `FPaldarkSessionToken::ExpirySeconds`
	// the subsystem proactively calls `RefreshToken`. Defaults to 300 (5
	// minutes); lower for short raid sessions, higher if Cognito is rate
	// limiting refresh.
	UPROPERTY(EditAnywhere, Config, Category = "Backend|Auth",
		meta = (DisplayName = "Token refresh lead (seconds)",
		        ClampMin = 30, ClampMax = 3600))
	int32 TokenRefreshLeadSeconds = 300;
};
