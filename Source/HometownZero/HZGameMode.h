#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HZGameMode.generated.h"

UCLASS()
class HOMETOWNZERO_API AHZGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AHZGameMode();

	virtual void BeginPlay() override;
};
