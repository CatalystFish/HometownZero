#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HZInventoryComponent.generated.h"

/**
 * Spike inventory: name -> count map with log feedback. Real UI comes later.
 */
UCLASS(ClassGroup = (HZ), meta = (BlueprintSpawnableComponent))
class HOMETOWNZERO_API UHZInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	void AddItem(const FString& ItemName, int32 Count = 1);

	int32 TotalItems() const { return TotalItemCount; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	TMap<FString, int32> Items;

	int32 TotalItemCount = 0;
};
