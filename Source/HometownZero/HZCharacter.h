#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HZCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UHZInventoryComponent;

UCLASS()
class HOMETOWNZERO_API AHZCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AHZCharacter();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual float TakeDamage(float DamageAmount, const struct FDamageEvent& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	float GetHealth() const { return Health; }
	UHZInventoryComponent* GetInventory() const { return Inventory; }

	UFUNCTION(Exec, Category = "HZ Debug")
	void HZDumpLoot(FString Category);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	UCameraComponent* FollowCamera;

private:
	void MoveForward(float Value);
	void MoveRight(float Value);
	void SprintStart();
	void SprintStop();
	void Attack();
	void Interact();
	void Respawn();

	float Health = 100.f;
	float LastAttackTime = -10.f;

	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	TObjectPtr<UHZInventoryComponent> Inventory;
};
