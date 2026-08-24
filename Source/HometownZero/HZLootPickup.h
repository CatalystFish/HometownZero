#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HZLootPickup.generated.h"

class UStaticMeshComponent;

/** Walk-over pickup: adds itself to the player's inventory and dies. */
UCLASS()
class HOMETOWNZERO_API AHZLootPickup : public AActor
{
	GENERATED_BODY()

public:
	AHZLootPickup();

	UPROPERTY(EditAnywhere, Category = "Loot")
	FString ItemName;

	UPROPERTY(EditAnywhere, Category = "Loot")
	FString Category;

protected:
	virtual void BeginPlay() override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> MeshComponent;
};
