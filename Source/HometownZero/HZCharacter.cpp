#include "HZCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr float MaxHealth = 100.f;
	constexpr float WalkSpeed = 500.f;
	constexpr float SprintSpeed = 900.f;
}

AHZCharacter::AHZCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 400.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void AHZCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AHZCharacter::SprintStart);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AHZCharacter::SprintStop);
	PlayerInputComponent->BindAxis("MoveForward", this, &AHZCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AHZCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
}

void AHZCharacter::MoveForward(float Value)
{
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		AddMovementInput(YawRotation.Vector(), Value);
	}
}

void AHZCharacter::MoveRight(float Value)
{
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Value);
	}
}

void AHZCharacter::SprintStart()
{
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void AHZCharacter::SprintStop()
{
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

float AHZCharacter::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	const float Actual = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	Health = FMath::Clamp(Health - Actual, 0.f, MaxHealth);
	UE_LOG(LogTemp, Log, TEXT("[HZ Player] took %.0f damage, health %.0f"), Actual, Health);
	if (Health <= 0.f)
	{
		Respawn();
	}
	return Actual;
}

void AHZCharacter::Respawn()
{
	APlayerStart* Spawn = Cast<APlayerStart>(UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass()));
	const FVector Target = Spawn ? Spawn->GetActorLocation() + FVector(0.f, 0.f, 100.f)
		: GetActorLocation();
	SetActorLocation(Target);
	GetCharacterMovement()->Velocity = FVector::ZeroVector;
	Health = MaxHealth;
	UE_LOG(LogTemp, Warning, TEXT("[HZ Player] died and respawned (health %.0f)"), Health);
}
