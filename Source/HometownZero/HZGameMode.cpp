#include "HZGameMode.h"
#include "HZCharacter.h"
#include "HZDistrictManager.h"
#include "HZHUD.h"

AHZGameMode::AHZGameMode()
{
	DefaultPawnClass = AHZCharacter::StaticClass();
	HUDClass = AHZHUD::StaticClass();
}

void AHZGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Path B: build the city from district.json at runtime.
	GetWorld()->SpawnActor<AHDistrictManager>();
}
