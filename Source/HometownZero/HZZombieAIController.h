#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "HZZombieAIController.generated.h"

class AHZCharacter;

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
	void TryAttack(AHZCharacter* PlayerCharacter);

	FVector SpawnLocation = FVector::ZeroVector;
	float TimeSinceLastThink = 0.f;
	float LastAttackTime = -10.f;
	bool bHasWanderTarget = false;
};
