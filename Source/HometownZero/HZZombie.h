#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HZZombie.generated.h"

class UStaticMeshComponent;

/**
 * Spike placeholder zombie: capsule-driven Character with a visible cube body
 * (no skeletal mesh) that chases or wanders via AHZZombieAIController.
 */
UCLASS()
class HOMETOWNZERO_API AHZZombie : public ACharacter
{
	GENERATED_BODY()

public:
	AHZZombie();

	virtual float TakeDamage(float DamageAmount, const struct FDamageEvent& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Zombie")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	float Health = 100.f;
};
