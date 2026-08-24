#pragma once

#include "CoreMinimal.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "HZNavBoundsVolume.generated.h"

class UBoxComponent;

/**
 * Runtime-friendly NavMeshBoundsVolume: the stock volume's root is an empty
 * brush component with no geometry when spawned at runtime, so its bounds
 * collapse to a point. This variant swaps in a real UBoxComponent root that
 * can be sized from code.
 */
UCLASS()
class HOMETOWNZERO_API AHZNavBoundsVolume : public ANavMeshBoundsVolume
{
	GENERATED_BODY()

public:
	AHZNavBoundsVolume();

	/** Half-extents of the navigable region, in world units. */
	void SetNavBoxExtent(const FVector& HalfExtent);

private:
	UPROPERTY(VisibleAnywhere, Category = "Navigation")
	TObjectPtr<UBoxComponent> BoundsBox;
};
