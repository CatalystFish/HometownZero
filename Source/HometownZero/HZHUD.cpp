#include "HZHUD.h"

#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "HZCharacter.h"
#include "HZDistrictManager.h"
#include "HZInventoryComponent.h"
#include "HZLootContainer.h"
#include "Kismet/GameplayStatics.h"

void AHZHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	AHZCharacter* Character = Cast<AHZCharacter>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (!Character)
	{
		return;
	}

	// Health bar (bottom-left).
	const float BarX = 40.f;
	const float BarY = Canvas->SizeY - 80.f;
	const float HealthFraction = FMath::Clamp(Character->GetHealth() / 100.f, 0.f, 1.f);
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), BarX - 2.f, BarY - 2.f, 304.f, 28.f);
	DrawRect(FLinearColor(0.85f, 0.15f, 0.15f, 0.9f), BarX, BarY, 300.f * HealthFraction, 24.f);
	DrawText(FString::Printf(TEXT("HP %d"), FMath::RoundToInt(Character->GetHealth())),
		FLinearColor::White, BarX + 8.f, BarY + 3.f);

	// Bag count (top-left).
	if (Character->GetInventory())
	{
		DrawText(FString::Printf(TEXT("Bag: %d items"), Character->GetInventory()->TotalItems()),
			FLinearColor(0.9f, 0.9f, 0.7f), 40.f, 40.f);
	}

	// Controls hint (top-left, under bag count).
	DrawText(TEXT("WASD move | Mouse look | Shift sprint | LMB attack | E search | ` ghost/fly/walk"),
		FLinearColor(0.7f, 0.7f, 0.7f), 40.f, 66.f);

	// Search prompt: nearest unopened container within 250cm.
	AHDistrictManager* Manager = AHDistrictManager::Get(GetWorld());
	if (Manager)
	{
		const FVector PlayerLocation = Character->GetActorLocation();
		const TArray<TObjectPtr<AHZLootContainer>>& Containers = Manager->GetContainers();
		const AHZLootContainer* Nearest = nullptr;
		float NearestDistSq = FMath::Square(250.f);
		for (const TObjectPtr<AHZLootContainer>& Container : Containers)
		{
			if (!Container || Container->bOpened)
			{
				continue;
			}
			const float DistSq = FVector::DistSquared(PlayerLocation, Container->GetActorLocation());
			if (DistSq < NearestDistSq)
			{
				NearestDistSq = DistSq;
				Nearest = Container;
			}
		}
		if (Nearest)
		{
			const FString Prompt = FString::Printf(TEXT("[E] Search %s"), *Nearest->Category);
			DrawText(Prompt, FLinearColor::White,
				Canvas->SizeX * 0.5f - 60.f, Canvas->SizeY * 0.6f);
		}
	}
}
