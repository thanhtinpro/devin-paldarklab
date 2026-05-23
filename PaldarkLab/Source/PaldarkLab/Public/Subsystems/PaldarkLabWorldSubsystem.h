// PALDARK W1 — UWorldSubsystem demo. Lives per-world (1 per PIE world / 1 per game
// world / 1 per dedicated server). Tracks every spawned APaldarkLabSampleActor and
// ticks once per second to emit a count line, exercising subsystem lifecycle hooks
// covered in P02 L7.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "PaldarkLabWorldSubsystem.generated.h"

class APaldarkLabSampleActor;

UCLASS()
class PALDARKLAB_API UPaldarkLabWorldSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	// UWorldSubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickableInEditor() const override { return false; }
	virtual bool IsTickable() const override;

	// Registration API used by APaldarkLabSampleActor::BeginPlay / EndPlay.
	void RegisterSampleActor(APaldarkLabSampleActor* Actor);
	void UnregisterSampleActor(APaldarkLabSampleActor* Actor);

	UFUNCTION(BlueprintPure, Category = "Paldark|Lab")
	int32 GetSampleActorCount() const { return TrackedActors.Num(); }

private:
	// UPROPERTY needed to prevent GC from collecting referenced actors before the
	// subsystem hears about EndPlay. Filtered for null/PendingKill in Tick.
	UPROPERTY()
	TArray<TObjectPtr<APaldarkLabSampleActor>> TrackedActors;

	float SecondsSinceLastReport = 0.0f;
};
