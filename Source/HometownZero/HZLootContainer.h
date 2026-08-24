#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HZLootContainer.generated.h"

class UStaticMeshComponent;

/**
 * A searchable loot container placed at a building corner. Its category comes
 * from the building it belongs to and drives the loot table on Open().
 */
UCLASS()
class HOMETOWNZERO_API AHZLootContainer : public AActor
{
	GENERATED_BODY()

public:
	AHZLootContainer();

	/** Weighted roll of the category table; exposed for headless testing. */
	static TArray<FString> RollLootForCategory(const FString& Category, int32 Count);

	UPROPERTY(EditAnywhere, Category = "Loot")
	FString Category = TEXT("unknown");

	UPROPERTY(VisibleAnywhere, Category = "Loot")
	bool bOpened = false;

	void Open(AActor* Taker);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> MeshComponent;
};
