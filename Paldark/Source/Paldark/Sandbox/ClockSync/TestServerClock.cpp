#include "TestServerClock.h"

#include "Engine/World.h"

double UTestServerClock::GetServerTime() const
{
	if (!World)
	{
		return 0.0;
	}
	return World->GetTimeSeconds() + SyntheticOffsetSeconds;
}
