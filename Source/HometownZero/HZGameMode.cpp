#include "HZGameMode.h"
#include "HZCharacter.h"
#include "HZDistrictManager.h"

AHZGameMode::AHZGameMode()
{
	DefaultPawnClass = AHZCharacter::StaticClass();
}

void AHZGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Path B: build the city from district.json at runtime.
	GetWorld()->SpawnActor<AHDistrictManager>();
}
