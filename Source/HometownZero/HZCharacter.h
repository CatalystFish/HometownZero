#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HZCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;

UCLASS()
class HOMETOWNZERO_API AHZCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AHZCharacter();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	UCameraComponent* FollowCamera;

private:
	void MoveForward(float Value);
	void MoveRight(float Value);
};
