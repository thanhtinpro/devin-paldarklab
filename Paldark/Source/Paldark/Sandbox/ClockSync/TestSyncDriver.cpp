#include "TestSyncDriver.h"

#include "SandboxClockLog.h"
#include "TestServerClock.h"
#include "TestClientClock.h"

#include "Engine/World.h"
#include "TimerManager.h"

// SANDBOX: this is the "server has been up for ~17 minutes" offset. In
// production, the server's World->GetTimeSeconds() will simply be whatever
// it is; the client has no reason to expect it to match its own.
static constexpr double GServerSyntheticOffset = 1000.0;

// Tolerances chosen for sandbox; production picks based on jitter histograms.
static constexpr double GToleranceLowRTT  = 0.005; // 5  ms
static constexpr double GToleranceHighRTT = 0.025; // 25 ms (accounts for SetTimer granularity)

bool UTestSyncDriver::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UTestSyncDriver::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Server = NewObject<UTestServerClock>(this);
	Client = NewObject<UTestClientClock>(this);
	Server->InitClock(GetWorld(), GServerSyntheticOffset);
	Client->InitClock(GetWorld());

	if (UWorld* World = GetWorld())
	{
		WorldBeginPlayHandle = World->OnWorldBeginPlay.AddUObject(this, &UTestSyncDriver::OnWorldBeginPlay);
	}
}

void UTestSyncDriver::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->OnWorldBeginPlay.Remove(WorldBeginPlayHandle);
	}
	Super::Deinitialize();
}

void UTestSyncDriver::OnWorldBeginPlay()
{
	UE_LOG(LogSandboxClock, Display, TEXT("=== Lesson05 ClockSync :: OnWorldBeginPlay — RUN ALL TESTS ==="));
	RunAllTestCases();
}

// ---------------------------------------------------------------------------
// Synthetic round-trip: we *compute* the three timestamps directly. No timer.
// This isolates the MATH from the network plumbing so the lesson stays focused.
// ---------------------------------------------------------------------------
void UTestSyncDriver::DoSyntheticHandshake(double SimulatedRTT)
{
	const double T1            = Client->GetLocalTime();
	const double ServerAtT1Mid = Server->GetServerTime() + (SimulatedRTT * 0.5); // server "receives" at T1 + RTT/2
	const double T2            = T1 + SimulatedRTT;
	Client->ProcessHandshakeResponse(T1, ServerAtT1Mid, T2);
}

// ---------------------------------------------------------------------------
// Async round-trip: client "sends" (records T1), we wait SimulatedRTT seconds
// via SetTimer, then call the handshake on response. Proves the async nature.
// ---------------------------------------------------------------------------
void UTestSyncDriver::DoAsyncHandshakeRoundtrip(double SimulatedRTT, TFunction<void()> OnDone)
{
	const double T1 = Client->GetLocalTime();
	UWorld* World = GetWorld();
	if (!World) { OnDone(); return; }

	UE_LOG(LogSandboxClock, Display, TEXT("Async handshake: sent at T1=%.4fs, awaiting reply in %.0f ms..."),
		T1, SimulatedRTT * 1000.0);

	FTimerHandle H;
	FTimerDelegate Delegate;
	Delegate.BindLambda([WeakThis = TWeakObjectPtr<UTestSyncDriver>(this), T1, OnDone]()
	{
		UTestSyncDriver* Self = WeakThis.Get();
		if (!Self) { return; }

		const double ServerS = Self->Server->GetServerTime(); // sample server "at receipt"
		const double T2      = Self->Client->GetLocalTime();
		Self->Client->ProcessHandshakeResponse(T1, ServerS, T2);
		OnDone();
	});
	World->GetTimerManager().SetTimer(H, Delegate, SimulatedRTT, false);
}

void UTestSyncDriver::RunAllTestCases()
{
	// -----------------------------------------------------------------------
	// TC1 — Server clock is intentionally offset from world/client local time.
	// -----------------------------------------------------------------------
	{
		const double Local  = Client->GetLocalTime();
		const double Server_T = Server->GetServerTime();
		const double Diff = Server_T - Local;
		const bool bPass = FMath::IsNearlyEqual(Diff, GServerSyntheticOffset, 0.01);
		UE_LOG(LogSandboxClock, Display,
			TEXT("[TC1] Server time %.4fs vs client local %.4fs (diff %.4fs ~= synthetic %.0fs): %s"),
			Server_T, Local, Diff, GServerSyntheticOffset, bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// -----------------------------------------------------------------------
	// TC2 — Without sync, client's naive "use my local time" wildly misses
	// the actual server time.
	// -----------------------------------------------------------------------
	{
		const double Naive  = Client->GetLocalTime();           // pretends local IS server
		const double Truth  = Server->GetServerTime();
		const double Error  = FMath::Abs(Truth - Naive);
		const bool bPass = Error > 100.0; // wildly off (by ~1000s)
		UE_LOG(LogSandboxClock, Display,
			TEXT("[TC2] Pre-sync naive estimate=%.4fs, true server=%.4fs, error=%.4fs (huge): %s"),
			Naive, Truth, Error, bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// -----------------------------------------------------------------------
	// TC3 — Synthetic handshake with RTT=100ms. Math should record RTT and
	// produce an offset roughly equal to the synthetic server offset.
	// -----------------------------------------------------------------------
	{
		DoSyntheticHandshake(0.100);
		const bool bRTTOk     = FMath::IsNearlyEqual(Client->GetLastRTT(), 0.100, 1e-6);
		const bool bOffsetOk  = FMath::IsNearlyEqual(Client->GetLocalToServerOffset(), GServerSyntheticOffset, GToleranceLowRTT);
		UE_LOG(LogSandboxClock, Display,
			TEXT("[TC3] Synthetic RTT=100ms -> recorded RTT=%.4fs offset=%.4fs (target %.4fs): %s"),
			Client->GetLastRTT(), Client->GetLocalToServerOffset(), GServerSyntheticOffset,
			(bRTTOk && bOffsetOk) ? TEXT("PASS") : TEXT("FAIL"));
	}

	// -----------------------------------------------------------------------
	// TC4 — After sync, GetEstimatedServerTime tracks Server->GetServerTime
	// within tolerance.
	// -----------------------------------------------------------------------
	{
		const double Estimate = Client->GetEstimatedServerTime();
		const double Truth    = Server->GetServerTime();
		const double Error    = FMath::Abs(Estimate - Truth);
		const bool bPass = Error < GToleranceLowRTT;
		UE_LOG(LogSandboxClock, Display,
			TEXT("[TC4] Post-sync estimate=%.4fs truth=%.4fs error=%.4fs (tol %.4fs): %s"),
			Estimate, Truth, Error, GToleranceLowRTT, bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// -----------------------------------------------------------------------
	// TC5 — High RTT (500ms) still produces a good estimate when latency is
	// roughly symmetric.
	// -----------------------------------------------------------------------
	{
		DoSyntheticHandshake(0.500);
		const double Estimate = Client->GetEstimatedServerTime();
		const double Truth    = Server->GetServerTime();
		const double Error    = FMath::Abs(Estimate - Truth);
		const bool bPass = Error < GToleranceHighRTT;
		UE_LOG(LogSandboxClock, Display,
			TEXT("[TC5] RTT=500ms post-sync error=%.4fs (tol %.4fs): %s"),
			Error, GToleranceHighRTT, bPass ? TEXT("PASS") : TEXT("FAIL"));
	}

	// -----------------------------------------------------------------------
	// TC6 — Async round-trip: handshake fires LATER via SetTimer. Verify
	// the response arrived (delegate executed) and produced a sane offset.
	// -----------------------------------------------------------------------
	{
		UE_LOG(LogSandboxClock, Display, TEXT("[TC6] Kicking async handshake (200ms simulated)..."));
		DoAsyncHandshakeRoundtrip(0.200, [WeakThis = TWeakObjectPtr<UTestSyncDriver>(this)]()
		{
			UTestSyncDriver* Self = WeakThis.Get();
			if (!Self) return;
			const double Error = FMath::Abs(Self->Client->GetEstimatedServerTime() - Self->Server->GetServerTime());
			const bool bPass = Error < GToleranceHighRTT;
			UE_LOG(LogSandboxClock, Display,
				TEXT("[TC6] Async response received, post-sync error=%.4fs: %s"),
				Error, bPass ? TEXT("PASS") : TEXT("FAIL"));

			// -----------------------------------------------------------------
			// TC7 — The "why" payoff: client claims "I shot at TimeOfShot=X
			// (local time)". With sync, server can translate this into its own
			// timeline (S = X + offset) and locate the right frame to rewind
			// for lag compensation. Without sync, X is just a number with no
			// meaning to the server.
			// -----------------------------------------------------------------
			const double LocalTimeOfShot = Self->Client->GetLocalTime() - 0.050; // "50 ms ago locally"
			const double InterpretedOnServer = LocalTimeOfShot + Self->Client->GetLocalToServerOffset();
			const double ServerNow = Self->Server->GetServerTime();
			const double TimeOfShotInServerHistory = ServerNow - InterpretedOnServer; // ~ 50 ms in the past
			const bool bPass7 = FMath::IsNearlyEqual(TimeOfShotInServerHistory, 0.050, GToleranceHighRTT);
			UE_LOG(LogSandboxClock, Display,
				TEXT("[TC7] Client TimeOfShot=%.4fs (local) -> server interprets at %.4fs (server). ServerNow=%.4fs. Shot is %.4fs in server history (target ~0.050s): %s"),
				LocalTimeOfShot, InterpretedOnServer, ServerNow, TimeOfShotInServerHistory,
				bPass7 ? TEXT("PASS") : TEXT("FAIL"));

			UE_LOG(LogSandboxClock, Display, TEXT("=== Lesson05 ClockSync :: DONE ==="));
		});
	}
}
