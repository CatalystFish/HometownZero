#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "HZZombieAIController.generated.h"

/**
 * Throttled chase/wander brain: pursue the player within 8000cm, otherwise
 * wander within 2000cm of spawn.
 */
UCLASS()
class HOMETOWNZERO_API AHZZombieAIController : public AAIController
{
	GENERATED_BODY()

public:
	AHZZombieAIController();

	virtual void OnPossess(class APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void UpdateBehavior();

	FVector SpawnLocation = FVector::ZeroVector;
	float TimeSinceLastThink = 0.f;
	bool bHasWanderTarget = false;
};
