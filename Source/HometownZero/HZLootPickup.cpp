#include "HZLootPickup.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "HZCharacter.h"
#include "HZInventoryComponent.h"
#include "Materials/MaterialInterface.h"

AHZLootPickup::AHZLootPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	if (CubeMesh)
	{
		MeshComponent->SetStaticMesh(CubeMesh);
	}
	SetRootComponent(MeshComponent);
	MeshComponent->SetRelativeScale3D(FVector(0.3f));
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AHZLootPickup::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	AHZCharacter* Character = Cast<AHZCharacter>(OtherActor);
	if (!Character || !Character->GetInventory())
	{
		return;
	}

	Character->GetInventory()->AddItem(ItemName.IsEmpty() ? TEXT("Unknown") : ItemName);
	Destroy();
}

void AHZLootPickup::BeginPlay()
{
	Super::BeginPlay();

	const FString SafeCategory = Category.IsEmpty() ? TEXT("unknown") : Category;
	if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(
		nullptr, *FString::Printf(TEXT("/Game/HZMaterials/M_HZ_%s.M_HZ_%s"),
			*SafeCategory, *SafeCategory)))
	{
		MeshComponent->SetMaterial(0, Material);
	}
}
