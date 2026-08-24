#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "HZHUD.generated.h"

/**
 * Spike HUD: health bar, item count, and container search prompt.
 */
UCLASS()
class HOMETOWNZERO_API AHZHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
};
