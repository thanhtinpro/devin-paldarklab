// Sandbox Lesson 05 — Server clock.
//
// Returns "the server's authoritative time" — a value that may be far ahead
// of any client's local time (server can be hours into a long match while
// a player just connected). SANDBOX: server runs in same process as the
// "client" object, so we inject SyntheticOffset so the two clocks are
// observably different. In real Paldark, the server is on a different
// machine entirely and its World->GetTimeSeconds() simply differs.
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TestServerClock.generated.h"

UCLASS()
class PALDARK_API UTestServerClock : public UObject
{
	GENERATED_BODY()

public:
	void InitClock(UWorld* InWorld, double InSyntheticOffsetSeconds)
	{
		World = InWorld;
		SyntheticOffsetSeconds = InSyntheticOffsetSeconds;
	}

	// SANDBOX: trong thực tế chỉ là World->GetTimeSeconds() trên process server.
	double GetServerTime() const;

private:
	UPROPERTY()
	UWorld* World = nullptr;

	// SANDBOX: thực tế không cần — process server có sẵn clock riêng.
	double SyntheticOffsetSeconds = 0.0;
};
