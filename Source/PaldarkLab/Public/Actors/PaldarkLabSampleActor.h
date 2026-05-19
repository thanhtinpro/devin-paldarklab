// PALDARK W1 — Sample actor. Demonstrates: UCLASS, UPROPERTY, USTATICMESHCOMPONENT
// composition, replicated property, lifecycle (BeginPlay/Tick), and registration with
// UPaldarkLabWorldSubsystem.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PaldarkLabSampleActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class PALDARKLAB_API APaldarkLabSampleActor : public AActor
{
	GENERATED_BODY()

public:
	APaldarkLabSampleActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Paldark|Lab")
	float GetRotationSpeedDegPerSec() const { return RotationSpeedDegPerSec; }

protected:
	// Root mesh — points to the engine cube by default in CDO so the actor is visible
	// when dropped into a level without per-instance setup.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paldark|Lab", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// Tunable in editor. Replicated so server-driven changes (e.g. a future game effect
	// adjusting the rate) propagate to clients in a multiplayer session.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Paldark|Lab")
	float RotationSpeedDegPerSec = 45.0f;

	// Vertical bob amplitude (cm). Edit-time only; not replicated because clients can
	// derive identical motion from BeginPlay time + amplitude.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paldark|Lab")
	float BobAmplitudeCm = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paldark|Lab")
	float BobFrequencyHz = 0.5f;

private:
	FVector InitialLocation = FVector::ZeroVector;
	float ElapsedSeconds = 0.0f;
};
