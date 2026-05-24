#include "TestClientClock.h"

#include "SandboxClockLog.h"

#include "Engine/World.h"

double UTestClientClock::GetLocalTime() const
{
	return World ? World->GetTimeSeconds() : 0.0;
}

void UTestClientClock::ProcessHandshakeResponse(double T1, double ServerS, double T2)
{
	// Classic NTP-lite:
	//   RTT          = T2 - T1
	//   half-RTT     = RTT / 2  (assume symmetric one-way latency)
	//   estimated server time at T2  =  ServerS + half-RTT
	//   offset(local -> server)      =  estimated_server_at_T2  -  T2
	//
	// Symmetric assumption is the load-bearing simplification: if upstream and
	// downstream latency differ wildly, the offset will be off by half the
	// asymmetry. Production code averages many rounds and rejects outliers.
	const double RTT          = T2 - T1;
	const double HalfRTT      = RTT * 0.5;
	const double EstAtT2      = ServerS + HalfRTT;
	const double NewOffset    = EstAtT2 - T2;

	LastRTT             = RTT;
	LocalToServerOffset = NewOffset;
	bSynced             = true;

	UE_LOG(LogSandboxClock, Display,
		TEXT("Handshake: T1=%.4fs ServerS=%.4fs T2=%.4fs -> RTT=%.4fs halfRTT=%.4fs offset=%.4fs"),
		T1, ServerS, T2, RTT, HalfRTT, NewOffset);
}
