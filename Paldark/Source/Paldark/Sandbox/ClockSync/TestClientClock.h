// Sandbox Lesson 05 — Client clock.
//
// Holds the *offset* between local time and authoritative server time, plus
// the NTP-style handshake math. After ProcessHandshakeResponse() runs once,
// GetEstimatedServerTime() should be within ~half-RTT-asymmetry seconds of
// the true server time.
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TestClientClock.generated.h"

UCLASS()
class PALDARK_API UTestClientClock : public UObject
{
	GENERATED_BODY()

public:
	void InitClock(UWorld* InWorld) { World = InWorld; }

	double GetLocalTime() const;

	// Local + offset. Pre-sync this lies; post-sync it's accurate.
	double GetEstimatedServerTime() const { return GetLocalTime() + LocalToServerOffset; }

	bool IsSynced() const { return bSynced; }

	double GetLastRTT() const           { return LastRTT; }
	double GetLocalToServerOffset() const { return LocalToServerOffset; }

	// The handshake math, factored so tests can pass synthetic values.
	//   T1 = client local time when request was sent
	//   ServerS = server's clock at request receipt
	//   T2 = client local time when response arrived
	// Result is stored in LocalToServerOffset.
	void ProcessHandshakeResponse(double T1, double ServerS, double T2);

private:
	UPROPERTY()
	UWorld* World = nullptr;

	double LocalToServerOffset = 0.0;
	double LastRTT = 0.0;
	bool bSynced = false;
};
