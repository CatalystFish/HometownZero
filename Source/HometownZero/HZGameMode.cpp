#include "HZGameMode.h"
#include "HZCharacter.h"

AHZGameMode::AHZGameMode()
{
	DefaultPawnClass = AHZCharacter::StaticClass();
}
