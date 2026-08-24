#include "HZCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/SpringArmComponent.h"
#include "HZDistrictManager.h"
#include "HZInventoryComponent.h"
#include "HZLootContainer.h"
#include "HZZombie.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr float MaxHealth = 100.f;
	constexpr float WalkSpeed = 500.f;
	constexpr float SprintSpeed = 900.f;
	constexpr float AttackCooldownSeconds = 0.5f;
	constexpr float AttackDamage = 34.f;
	constexpr float AttackRangeCm = 190.f;
	constexpr float InteractRangeCm = 250.f;
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

	Inventory = CreateDefaultSubobject<UHZInventoryComponent>(TEXT("Inventory"));
}

void AHZCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AHZCharacter::SprintStart);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AHZCharacter::SprintStop);
	PlayerInputComponent->BindAction("Attack", IE_Pressed, this, &AHZCharacter::Attack);
	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AHZCharacter::Interact);
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

void AHZCharacter::Attack()
{
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastAttackTime < AttackCooldownSeconds)
	{
		return;
	}
	LastAttackTime = Now;

	// Melee sweep: everything HZZombie-ish inside a sphere just ahead of us.
	const FVector Center = GetActorLocation() + GetActorForwardVector() * (AttackRangeCm * 0.6f);
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HZAttack), false, this);
	GetWorld()->OverlapMultiByChannel(
		Overlaps, Center, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeSphere(AttackRangeCm * 0.5f), Params);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (AHZZombie* Zombie = Cast<AHZZombie>(Overlap.GetActor()))
		{
			UGameplayStatics::ApplyDamage(Zombie, AttackDamage,
				GetController(), this, nullptr);
			UE_LOG(LogTemp, Log, TEXT("[HZ Combat] hit zombie at %.0fm"),
				FVector::Dist(GetActorLocation(), Zombie->GetActorLocation()) / 100.f);
		}
	}
}

void AHZCharacter::Interact()
{
	AHDistrictManager* Manager = AHDistrictManager::Get(GetWorld());
	if (!Manager)
	{
		return;
	}

	AHZLootContainer* Nearest = nullptr;
	float NearestDistSq = FMath::Square(InteractRangeCm);
	for (const TObjectPtr<AHZLootContainer>& Container : Manager->GetContainers())
	{
		if (!Container || Container->bOpened)
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(GetActorLocation(), Container->GetActorLocation());
		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			Nearest = Container;
		}
	}
	if (Nearest)
	{
		Nearest->Open(this);
	}
}

void AHZCharacter::HZDumpLoot(FString Category)
{
	const TArray<FString> Items = AHZLootContainer::RollLootForCategory(Category, 5);
	UE_LOG(LogTemp, Warning, TEXT("[HZ Debug] loot roll for '%s': %s"),
		*Category, *FString::Join(Items, TEXT(", ")));
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
