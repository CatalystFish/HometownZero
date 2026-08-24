#include "HZZombie.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HZZombieAIController.h"

AHZZombie::AHZZombie()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->SetCapsuleHalfHeight(88.f);

	GetCharacterMovement()->MaxWalkSpeed = 250.f;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 240.f, 0.f);

	bUseControllerRotationYaw = false;

	AIControllerClass = AHZZombieAIController::StaticClass();

	// Visible placeholder body: engine cube (100cm) scaled to ~90cm, resting on
	// the capsule bottom. The default skeletal mesh is never set, so nothing to hide.
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(CubeMesh.Object);
	}
	BodyMesh->SetupAttachment(GetRootComponent());
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -88.f + 45.f));
	BodyMesh->SetRelativeScale3D(FVector(0.9f));
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (UMaterialInterface* ZombieMaterial = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Game/HZMaterials/M_HZ_zombie.M_HZ_zombie")))
	{
		BodyMesh->SetMaterial(0, ZombieMaterial);
	}
}
