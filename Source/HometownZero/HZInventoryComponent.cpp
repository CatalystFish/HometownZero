#include "HZInventoryComponent.h"

void UHZInventoryComponent::AddItem(const FString& ItemName, int32 Count)
{
	Items.FindOrAdd(ItemName) += Count;
	TotalItemCount += Count;
	UE_LOG(LogTemp, Log, TEXT("[HZ Inv] %s x%d (bag total: %d)"),
		*ItemName, Count, TotalItemCount);
}
