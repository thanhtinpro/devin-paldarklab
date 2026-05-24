// Sandbox Lesson 05 — driver that owns one server clock + one client clock,
// runs handshake rounds with simulated RTTs, and validates the math.
//
// Why a single-process sandbox? The math is what matters. Network plumbing
// (FRPC / UNetConnection) only delivers the three timestamps (T1, S, T2);
// it does not change how the offset is computed.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TestSyncDriver.generated.h"

class UTestServerClock;
class UTestClientClock;

UCLASS()
class PALDARK_API UTestSyncDriver : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

protected:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
	void OnWorldBeginPlay();
	void RunAllTestCases();

	// Synthetic handshake: pretend the request "travels" for SimulatedRTT/2
	// each way, server receives at T1 + RTT/2 (server time), responds, client
	// gets reply at T1 + RTT. Then we call the math.
	void DoSyntheticHandshake(double SimulatedRTT);

	// Real async handshake using SetTimer to prove delegate-fires-after-return.
	void DoAsyncHandshakeRoundtrip(double SimulatedRTT, TFunction<void()> OnDone);

	UPROPERTY() UTestServerClock* Server = nullptr;
	UPROPERTY() UTestClientClock* Client = nullptr;

	FDelegateHandle WorldBeginPlayHandle;
};
