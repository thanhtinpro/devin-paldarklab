#include "Backend/PaldarkBackendSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "JsonObjectConverter.h"
#include "Misc/DateTime.h"
#include "TimerManager.h"

#include "Backend/PaldarkBackendSettings.h"
#include "PaldarkGameplayTags.h"
#include "PaldarkLogCategories.h"

namespace
{
	// Trim a single trailing slash from `LambdaApiBaseUrl` if present so a
	// designer who pastes `https://abc.execute-api.../prod/` does not produce
	// `https://abc.execute-api.../prod//sessions/hub`. Cheap to do at request
	// time; cheaper than baking the trimmed string into a UPROPERTY.
	FString TrimTrailingSlash(const FString& In)
	{
		if (In.EndsWith(TEXT("/")))
		{
			return In.LeftChop(1);
		}
		return In;
	}

	const TCHAR* StateToString(FGameplayTag StateTag)
	{
		if (StateTag == PaldarkGameplayTags::TAG_Paldark_Backend_State_LoggedOut)      { return TEXT("LoggedOut"); }
		if (StateTag == PaldarkGameplayTags::TAG_Paldark_Backend_State_Authenticating) { return TEXT("Authenticating"); }
		if (StateTag == PaldarkGameplayTags::TAG_Paldark_Backend_State_LoggedIn)       { return TEXT("LoggedIn"); }
		if (StateTag == PaldarkGameplayTags::TAG_Paldark_Backend_State_Refreshing)     { return TEXT("Refreshing"); }
		return TEXT("Unknown");
	}
}

void UPaldarkBackendSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SessionToken.Reset();
	StateTag = PaldarkGameplayTags::TAG_Paldark_Backend_State_LoggedOut;

	IConsoleManager& Console = IConsoleManager::Get();

	CmdLogin = Console.RegisterConsoleCommand(
		TEXT("Paldark.Backend.Login"),
		TEXT("Login against the backend. Usage: Paldark.Backend.Login <email> <password>"),
		FConsoleCommandWithArgsDelegate::CreateUObject(this, &UPaldarkBackendSubsystem::HandleConsoleLogin),
		ECVF_Default);

	CmdRequestHubFleet = Console.RegisterConsoleCommand(
		TEXT("Paldark.Backend.RequestHubFleet"),
		TEXT("Request a hub fleet allocation. Usage: Paldark.Backend.RequestHubFleet [ExperienceId]"),
		FConsoleCommandWithArgsDelegate::CreateUObject(this, &UPaldarkBackendSubsystem::HandleConsoleRequestHubFleet),
		ECVF_Default);

	CmdRequestRaidFleet = Console.RegisterConsoleCommand(
		TEXT("Paldark.Backend.RequestRaidFleet"),
		TEXT("Request a raid fleet allocation. Usage: Paldark.Backend.RequestRaidFleet [ExperienceId]"),
		FConsoleCommandWithArgsDelegate::CreateUObject(this, &UPaldarkBackendSubsystem::HandleConsoleRequestRaidFleet),
		ECVF_Default);

	CmdStatus = Console.RegisterConsoleCommand(
		TEXT("Paldark.Backend.Status"),
		TEXT("Print a snapshot of the backend subsystem state."),
		FConsoleCommandWithArgsDelegate::CreateUObject(this, &UPaldarkBackendSubsystem::HandleConsoleStatus),
		ECVF_Default);

	UE_LOG(LogPaldarkBackend, Log,
		TEXT("[W42-43][Backend] Subsystem initialized — 4 console commands registered (Login/RequestHubFleet/RequestRaidFleet/Status). bUseAWSBackend=%s."),
		GetSettings() && GetSettings()->bUseAWSBackend ? TEXT("true") : TEXT("false"));
}

void UPaldarkBackendSubsystem::Deinitialize()
{
	// Cancel every in-flight request before tearing down — otherwise the
	// `IHttpRequest` callback fires onto a dead `this` and crashes the
	// editor on PIE exit.
	for (const TSharedRef<IHttpRequest, ESPMode::ThreadSafe>& Request : InFlightRequests)
	{
		Request->CancelRequest();
	}
	InFlightRequests.Reset();

	CancelTokenRefresh();

	IConsoleManager& Console = IConsoleManager::Get();
	if (CmdLogin            != nullptr) { Console.UnregisterConsoleObject(CmdLogin);            CmdLogin = nullptr; }
	if (CmdRequestHubFleet  != nullptr) { Console.UnregisterConsoleObject(CmdRequestHubFleet);  CmdRequestHubFleet = nullptr; }
	if (CmdRequestRaidFleet != nullptr) { Console.UnregisterConsoleObject(CmdRequestRaidFleet); CmdRequestRaidFleet = nullptr; }
	if (CmdStatus           != nullptr) { Console.UnregisterConsoleObject(CmdStatus);           CmdStatus = nullptr; }

	Super::Deinitialize();
}

void UPaldarkBackendSubsystem::Login(const FString& Email, const FString& Password)
{
	if (Email.IsEmpty() || Password.IsEmpty())
	{
		FPaldarkBackendError Error;
		Error.StatusCode = 0;
		Error.ResultTag  = PaldarkGameplayTags::TAG_Paldark_Backend_Result_Fail_Auth;
		Error.Message    = TEXT("Email and password must both be non-empty.");
		OnLoginCompleted.Broadcast(FPaldarkSessionToken{}, Error);
		OnBackendError.Broadcast(Error);
		return;
	}

	UE_LOG(LogPaldarkBackend, Warning,
		TEXT("[W42-43][Backend] Login: plain-text credentials path — do not use outside dev. Cognito-hosted browser login lands in W46+."));

	TransitionState(PaldarkGameplayTags::TAG_Paldark_Backend_State_Authenticating);

	const UPaldarkBackendSettings* Settings = GetSettings();
	if (Settings == nullptr || !Settings->bUseAWSBackend || Settings->LambdaApiBaseUrl.IsEmpty())
	{
		IssueMockedLoginResponse(Email);
		return;
	}

	// Real path — POST /sessions/login with `{Email, Password, ClientId, PoolId}`.
	const FString BodyJson = FString::Printf(
		TEXT("{\"email\":\"%s\",\"password\":\"%s\",\"clientId\":\"%s\",\"poolId\":\"%s\"}"),
		*Email, *Password, *Settings->CognitoAppClientId, *Settings->CognitoUserPoolId);

	IssueHttpRequest(TEXT("POST"), TEXT("/sessions/login"), BodyJson, /*bAuthenticated=*/false,
		[this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
		{
			FPaldarkBackendError Error = BuildErrorFromResponse(Response, bConnectedSuccessfully);
			FPaldarkSessionToken Token;
			if (Error.IsSuccess() && Response.IsValid())
			{
				const FString Body = Response->GetContentAsString();
				if (!FJsonObjectConverter::JsonObjectStringToUStruct(Body, &Token, 0, 0))
				{
					Error.StatusCode = 0;
					Error.ResultTag  = PaldarkGameplayTags::TAG_Paldark_Backend_Result_Fail_Auth;
					Error.Message    = FString::Printf(TEXT("JSON parse failed: %s"), *Body);
				}
			}

			if (Error.IsSuccess())
			{
				SessionToken = Token;
				TransitionState(PaldarkGameplayTags::TAG_Paldark_Backend_State_LoggedIn);
				ScheduleTokenRefresh();
				Error.ResultTag = PaldarkGameplayTags::TAG_Paldark_Backend_Result_Success;
			}
			else
			{
				SessionToken.Reset();
				TransitionState(PaldarkGameplayTags::TAG_Paldark_Backend_State_LoggedOut);
				OnBackendError.Broadcast(Error);
			}

			OnLoginCompleted.Broadcast(SessionToken, Error);
		});
}

void UPaldarkBackendSubsystem::Logout()
{
	UE_LOG(LogPaldarkBackend, Log, TEXT("[W42-43][Backend] Logout — clearing cached session token."));
	SessionToken.Reset();
	CancelTokenRefresh();
	TransitionState(PaldarkGameplayTags::TAG_Paldark_Backend_State_LoggedOut);
}

void UPaldarkBackendSubsystem::RequestHubFleet(const FString& ExperienceId)
{
	const UPaldarkBackendSettings* Settings = GetSettings();
	if (Settings == nullptr || !Settings->bUseAWSBackend || Settings->LambdaApiBaseUrl.IsEmpty())
	{
		IssueMockedFleetResponse(PaldarkGameplayTags::TAG_Paldark_Backend_Fleet_Hub, ExperienceId);
		return;
	}

	const FString BodyJson = FString::Printf(
		TEXT("{\"experienceId\":\"%s\",\"shardKey\":\"%s\"}"),
		*ExperienceId, *SessionToken.UserId);

	IssueHttpRequest(TEXT("POST"), TEXT("/sessions/hub"), BodyJson, /*bAuthenticated=*/true,
		[this, ExperienceId](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
		{
			FPaldarkBackendError Error = BuildErrorFromResponse(Response, bConnectedSuccessfully);
			FPaldarkFleetSpec Spec;
			if (Error.IsSuccess() && Response.IsValid())
			{
				const FString Body = Response->GetContentAsString();
				if (!FJsonObjectConverter::JsonObjectStringToUStruct(Body, &Spec, 0, 0))
				{
					Error.StatusCode = 0;
					Error.ResultTag  = PaldarkGameplayTags::TAG_Paldark_Backend_Result_Fail_NoFleet;
					Error.Message    = FString::Printf(TEXT("JSON parse failed: %s"), *Body);
				}
				else
				{
					Spec.FleetTag = PaldarkGameplayTags::TAG_Paldark_Backend_Fleet_Hub;
					if (Spec.ExperienceId.IsEmpty())
					{
						Spec.ExperienceId = ExperienceId;
					}
				}
			}

			if (!Error.IsSuccess())
			{
				OnBackendError.Broadcast(Error);
			}
			else
			{
				Error.ResultTag = PaldarkGameplayTags::TAG_Paldark_Backend_Result_Success;
			}
			OnFleetRequestCompleted.Broadcast(Spec, Error);
		});
}

void UPaldarkBackendSubsystem::RequestRaidFleet(const FString& ExperienceId)
{
	const UPaldarkBackendSettings* Settings = GetSettings();
	if (Settings == nullptr || !Settings->bUseAWSBackend || Settings->LambdaApiBaseUrl.IsEmpty())
	{
		IssueMockedFleetResponse(PaldarkGameplayTags::TAG_Paldark_Backend_Fleet_Raid, ExperienceId);
		return;
	}

	const FString BodyJson = FString::Printf(
		TEXT("{\"experienceId\":\"%s\"}"),
		*ExperienceId);

	IssueHttpRequest(TEXT("POST"), TEXT("/sessions/raid"), BodyJson, /*bAuthenticated=*/true,
		[this, ExperienceId](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
		{
			FPaldarkBackendError Error = BuildErrorFromResponse(Response, bConnectedSuccessfully);
			FPaldarkFleetSpec Spec;
			if (Error.IsSuccess() && Response.IsValid())
			{
				const FString Body = Response->GetContentAsString();
				if (!FJsonObjectConverter::JsonObjectStringToUStruct(Body, &Spec, 0, 0))
				{
					Error.StatusCode = 0;
					Error.ResultTag  = PaldarkGameplayTags::TAG_Paldark_Backend_Result_Fail_NoFleet;
					Error.Message    = FString::Printf(TEXT("JSON parse failed: %s"), *Body);
				}
				else
				{
					Spec.FleetTag = PaldarkGameplayTags::TAG_Paldark_Backend_Fleet_Raid;
					if (Spec.ExperienceId.IsEmpty())
					{
						Spec.ExperienceId = ExperienceId;
					}
				}
			}

			if (!Error.IsSuccess())
			{
				OnBackendError.Broadcast(Error);
			}
			else
			{
				Error.ResultTag = PaldarkGameplayTags::TAG_Paldark_Backend_Result_Success;
			}
			OnFleetRequestCompleted.Broadcast(Spec, Error);
		});
}

void UPaldarkBackendSubsystem::RefreshToken()
{
	if (SessionToken.RefreshToken.IsEmpty())
	{
		UE_LOG(LogPaldarkBackend, Warning,
			TEXT("[W42-43][Backend] RefreshToken: no refresh token cached — must login first."));
		return;
	}

	TransitionState(PaldarkGameplayTags::TAG_Paldark_Backend_State_Refreshing);

	const UPaldarkBackendSettings* Settings = GetSettings();
	if (Settings == nullptr || !Settings->bUseAWSBackend || Settings->LambdaApiBaseUrl.IsEmpty())
	{
		// Mock: bump expiry by 1 hour and re-broadcast.
		SessionToken.ExpirySeconds = FDateTime::UtcNow().ToUnixTimestamp() + 3600;
		TransitionState(PaldarkGameplayTags::TAG_Paldark_Backend_State_LoggedIn);
		ScheduleTokenRefresh();
		UE_LOG(LogPaldarkBackend, Log,
			TEXT("[W42-43][Backend.Mock] RefreshToken — synthetic +1h extension."));
		OnTokenRefreshed.Broadcast(SessionToken);
		return;
	}

	const FString BodyJson = FString::Printf(
		TEXT("{\"refreshToken\":\"%s\",\"clientId\":\"%s\"}"),
		*SessionToken.RefreshToken, *Settings->CognitoAppClientId);

	IssueHttpRequest(TEXT("POST"), TEXT("/sessions/refresh"), BodyJson, /*bAuthenticated=*/false,
		[this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
		{
			FPaldarkBackendError Error = BuildErrorFromResponse(Response, bConnectedSuccessfully);
			FPaldarkSessionToken NewToken = SessionToken;
			if (Error.IsSuccess() && Response.IsValid())
			{
				const FString Body = Response->GetContentAsString();
				if (!FJsonObjectConverter::JsonObjectStringToUStruct(Body, &NewToken, 0, 0))
				{
					Error.StatusCode = 0;
					Error.ResultTag  = PaldarkGameplayTags::TAG_Paldark_Backend_Result_Fail_Auth;
					Error.Message    = FString::Printf(TEXT("JSON parse failed: %s"), *Body);
				}
			}

			if (Error.IsSuccess())
			{
				SessionToken = NewToken;
				TransitionState(PaldarkGameplayTags::TAG_Paldark_Backend_State_LoggedIn);
				ScheduleTokenRefresh();
				OnTokenRefreshed.Broadcast(SessionToken);
			}
			else
			{
				SessionToken.Reset();
				TransitionState(PaldarkGameplayTags::TAG_Paldark_Backend_State_LoggedOut);
				OnBackendError.Broadcast(Error);
			}
		});
}

FString UPaldarkBackendSubsystem::GetStatusReport() const
{
	const UPaldarkBackendSettings* Settings = GetSettings();
	const bool bUseAws = Settings != nullptr && Settings->bUseAWSBackend;
	const FString BaseUrl = Settings != nullptr ? Settings->LambdaApiBaseUrl : FString();

	return FString::Printf(
		TEXT("[W42-43][Backend] Status:\n")
		TEXT("  State        : %s\n")
		TEXT("  UserId       : %s\n")
		TEXT("  TokenValid   : %s\n")
		TEXT("  ExpirySec    : %lld\n")
		TEXT("  bUseAWSBackend: %s\n")
		TEXT("  LambdaBaseUrl: %s\n")
		TEXT("  InFlightReqs : %d"),
		StateToString(StateTag),
		SessionToken.UserId.IsEmpty() ? TEXT("<none>") : *SessionToken.UserId,
		SessionToken.IsValid() ? TEXT("true") : TEXT("false"),
		SessionToken.ExpirySeconds,
		bUseAws ? TEXT("true") : TEXT("false"),
		BaseUrl.IsEmpty() ? TEXT("<empty>") : *BaseUrl,
		InFlightRequests.Num());
}

void UPaldarkBackendSubsystem::HandleConsoleLogin(const TArray<FString>& Args)
{
	if (Args.Num() < 2)
	{
		UE_LOG(LogPaldarkBackend, Warning,
			TEXT("Paldark.Backend.Login: usage: Paldark.Backend.Login <email> <password>"));
		return;
	}
	Login(Args[0], Args[1]);
}

void UPaldarkBackendSubsystem::HandleConsoleRequestHubFleet(const TArray<FString>& Args)
{
	const FString ExperienceId = Args.Num() >= 1 ? Args[0] : FString();
	RequestHubFleet(ExperienceId);
}

void UPaldarkBackendSubsystem::HandleConsoleRequestRaidFleet(const TArray<FString>& Args)
{
	const FString ExperienceId = Args.Num() >= 1 ? Args[0] : FString();
	RequestRaidFleet(ExperienceId);
}

void UPaldarkBackendSubsystem::HandleConsoleStatus(const TArray<FString>& Args)
{
	UE_LOG(LogPaldarkBackend, Log, TEXT("%s"), *GetStatusReport());
	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, GetStatusReport());
	}
}

void UPaldarkBackendSubsystem::IssueHttpRequest(const FString& Verb,
                                                const FString& Path,
                                                const FString& JsonBody,
                                                bool bAuthenticated,
                                                TFunction<void(FHttpRequestPtr, FHttpResponsePtr, bool)> Callback)
{
	const UPaldarkBackendSettings* Settings = GetSettings();
	if (Settings == nullptr)
	{
		UE_LOG(LogPaldarkBackend, Error, TEXT("[W42-43][Backend] IssueHttpRequest: no settings — aborting."));
		Callback(nullptr, nullptr, false);
		return;
	}

	const FString FullUrl = TrimTrailingSlash(Settings->LambdaApiBaseUrl) + Path;

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(FullUrl);
	Request->SetVerb(Verb);
	Request->SetHeader(TEXT("Content-Type"),    TEXT("application/json"));
	Request->SetHeader(TEXT("X-Paldark-Pool"),  Settings->CognitoUserPoolId);
	Request->SetHeader(TEXT("X-Paldark-Region"), Settings->AWSRegion);
	if (bAuthenticated && SessionToken.IsValid())
	{
		Request->SetHeader(TEXT("Authorization"),
			FString::Printf(TEXT("Bearer %s"), *SessionToken.AccessToken));
	}
	Request->SetTimeout(Settings->RequestTimeoutSeconds);
	if (!JsonBody.IsEmpty())
	{
		Request->SetContentAsString(JsonBody);
	}

	TWeakObjectPtr<UPaldarkBackendSubsystem> WeakThis(this);
	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis, Callback, Verb, Path](FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bConnectedSuccessfully)
		{
			if (UPaldarkBackendSubsystem* Self = WeakThis.Get())
			{
				const int32 StatusCode = InResponse.IsValid() ? InResponse->GetResponseCode() : 0;
				UE_LOG(LogPaldarkBackend, Log,
					TEXT("[W42-43][Backend] HTTP %s %s → %d (connected=%s)"),
					*Verb, *Path, StatusCode, bConnectedSuccessfully ? TEXT("true") : TEXT("false"));
				Self->InFlightRequests.RemoveAll(
					[&InRequest](const TSharedRef<IHttpRequest, ESPMode::ThreadSafe>& Tracked)
					{
						return Tracked == InRequest;
					});
				Callback(InRequest, InResponse, bConnectedSuccessfully);
			}
		});

	UE_LOG(LogPaldarkBackend, Log, TEXT("[W42-43][Backend] HTTP %s %s issued."), *Verb, *FullUrl);
	InFlightRequests.Add(Request);
	Request->ProcessRequest();
}

void UPaldarkBackendSubsystem::IssueMockedFleetResponse(FGameplayTag InFleetTag, const FString& InExperienceId)
{
	UWorld* World = GetGameInstance() != nullptr ? GetGameInstance()->GetWorld() : nullptr;
	if (World == nullptr)
	{
		UE_LOG(LogPaldarkBackend, Warning,
			TEXT("[W42-43][Backend.Mock] No world available — broadcasting fleet response synchronously."));
		FPaldarkFleetSpec Spec = FPaldarkFleetSpec::MakeLoopback(InFleetTag, InExperienceId);
		FPaldarkBackendError Error;
		Error.StatusCode = 200;
		Error.ResultTag  = PaldarkGameplayTags::TAG_Paldark_Backend_Result_Success;
		Error.Message    = TEXT("Mocked fleet response (no world).");
		OnFleetRequestCompleted.Broadcast(Spec, Error);
		return;
	}

	TWeakObjectPtr<UPaldarkBackendSubsystem> WeakThis(this);
	World->GetTimerManager().SetTimerForNextTick(
		[WeakThis, InFleetTag, InExperienceId]()
		{
			if (UPaldarkBackendSubsystem* Self = WeakThis.Get())
			{
				FPaldarkFleetSpec Spec = FPaldarkFleetSpec::MakeLoopback(InFleetTag, InExperienceId);
				FPaldarkBackendError Error;
				Error.StatusCode = 200;
				Error.ResultTag  = PaldarkGameplayTags::TAG_Paldark_Backend_Result_Success;
				Error.Message    = TEXT("Mocked fleet response — bUseAWSBackend=false or empty LambdaApiBaseUrl.");
				UE_LOG(LogPaldarkBackend, Log,
					TEXT("[W42-43][Backend.Mock] Fleet response → %s @ %s:%d (Experience=%s)."),
					*Spec.FleetTag.ToString(), *Spec.IP, Spec.Port, *Spec.ExperienceId);
				Self->OnFleetRequestCompleted.Broadcast(Spec, Error);
			}
		});
}

void UPaldarkBackendSubsystem::IssueMockedLoginResponse(const FString& Email)
{
	UWorld* World = GetGameInstance() != nullptr ? GetGameInstance()->GetWorld() : nullptr;

	FPaldarkSessionToken Token;
	Token.AccessToken   = TEXT("mock-access-token");
	Token.RefreshToken  = TEXT("mock-refresh-token");
	Token.UserId        = FString::Printf(TEXT("mock-user:%s"), *Email);
	Token.ExpirySeconds = FDateTime::UtcNow().ToUnixTimestamp() + 3600;

	const auto Broadcast = [this, Token]()
	{
		SessionToken = Token;
		TransitionState(PaldarkGameplayTags::TAG_Paldark_Backend_State_LoggedIn);
		ScheduleTokenRefresh();
		FPaldarkBackendError Error;
		Error.StatusCode = 200;
		Error.ResultTag  = PaldarkGameplayTags::TAG_Paldark_Backend_Result_Success;
		Error.Message    = TEXT("Mocked login response — bUseAWSBackend=false or empty LambdaApiBaseUrl.");
		UE_LOG(LogPaldarkBackend, Log,
			TEXT("[W42-43][Backend.Mock] Login response → UserId=%s expiry=%lld."),
			*Token.UserId, Token.ExpirySeconds);
		OnLoginCompleted.Broadcast(SessionToken, Error);
	};

	if (World == nullptr)
	{
		Broadcast();
		return;
	}

	TWeakObjectPtr<UPaldarkBackendSubsystem> WeakThis(this);
	World->GetTimerManager().SetTimerForNextTick(
		[WeakThis, Broadcast]()
		{
			if (WeakThis.IsValid())
			{
				Broadcast();
			}
		});
}

FPaldarkBackendError UPaldarkBackendSubsystem::BuildErrorFromResponse(FHttpResponsePtr Response,
                                                                      bool bConnectedSuccessfully) const
{
	FPaldarkBackendError Error;
	if (!bConnectedSuccessfully || !Response.IsValid())
	{
		Error.StatusCode = 0;
		Error.ResultTag  = PaldarkGameplayTags::TAG_Paldark_Backend_Result_Fail_Network;
		Error.Message    = TEXT("Network failure or no response (timeout, DNS, offline).");
		return Error;
	}

	Error.StatusCode = Response->GetResponseCode();
	Error.Message    = Response->GetContentAsString();
	if (Error.StatusCode >= 200 && Error.StatusCode < 300)
	{
		Error.ResultTag = PaldarkGameplayTags::TAG_Paldark_Backend_Result_Success;
	}
	else if (Error.StatusCode == 401 || Error.StatusCode == 403)
	{
		Error.ResultTag = PaldarkGameplayTags::TAG_Paldark_Backend_Result_Fail_Auth;
	}
	else if (Error.StatusCode == 408 || Error.StatusCode == 504)
	{
		Error.ResultTag = PaldarkGameplayTags::TAG_Paldark_Backend_Result_Fail_Timeout;
	}
	else if (Error.StatusCode == 404 || Error.StatusCode == 503)
	{
		Error.ResultTag = PaldarkGameplayTags::TAG_Paldark_Backend_Result_Fail_NoFleet;
	}
	else
	{
		Error.ResultTag = PaldarkGameplayTags::TAG_Paldark_Backend_Result_Fail_Network;
	}
	return Error;
}

void UPaldarkBackendSubsystem::TransitionState(FGameplayTag NewState)
{
	if (StateTag == NewState)
	{
		return;
	}
	UE_LOG(LogPaldarkBackend, Log,
		TEXT("[W42-43][Backend] State: %s → %s."),
		StateToString(StateTag), StateToString(NewState));
	StateTag = NewState;
}

void UPaldarkBackendSubsystem::ScheduleTokenRefresh()
{
	CancelTokenRefresh();

	const UPaldarkBackendSettings* Settings = GetSettings();
	if (Settings == nullptr || SessionToken.AccessToken.IsEmpty())
	{
		return;
	}

	UWorld* World = GetGameInstance() != nullptr ? GetGameInstance()->GetWorld() : nullptr;
	if (World == nullptr)
	{
		return;
	}

	const int64 NowSeconds = FDateTime::UtcNow().ToUnixTimestamp();
	const int64 LeadSeconds = static_cast<int64>(Settings->TokenRefreshLeadSeconds);
	const int64 DelaySeconds = FMath::Max<int64>(1, SessionToken.ExpirySeconds - NowSeconds - LeadSeconds);

	TWeakObjectPtr<UPaldarkBackendSubsystem> WeakThis(this);
	World->GetTimerManager().SetTimer(TokenRefreshTimer, FTimerDelegate::CreateLambda(
		[WeakThis]()
		{
			if (UPaldarkBackendSubsystem* Self = WeakThis.Get())
			{
				UE_LOG(LogPaldarkBackend, Log,
					TEXT("[W42-43][Backend] Token-refresh timer fired — calling RefreshToken()."));
				Self->RefreshToken();
			}
		}), static_cast<float>(DelaySeconds), /*bLoop=*/false);

	UE_LOG(LogPaldarkBackend, Log,
		TEXT("[W42-43][Backend] Token-refresh scheduled in %lld seconds (expiry=%lld now=%lld lead=%lld)."),
		DelaySeconds, SessionToken.ExpirySeconds, NowSeconds, LeadSeconds);
}

void UPaldarkBackendSubsystem::CancelTokenRefresh()
{
	UWorld* World = GetGameInstance() != nullptr ? GetGameInstance()->GetWorld() : nullptr;
	if (World != nullptr && TokenRefreshTimer.IsValid())
	{
		World->GetTimerManager().ClearTimer(TokenRefreshTimer);
	}
	TokenRefreshTimer.Invalidate();
}

const UPaldarkBackendSettings* UPaldarkBackendSubsystem::GetSettings() const
{
	return GetDefault<UPaldarkBackendSettings>();
}
