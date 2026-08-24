#include "HZNavBoundsVolume.h"

#include "Components/BoxComponent.h"
#include "Engine/CollisionProfile.h"

AHZNavBoundsVolume::AHZNavBoundsVolume()
{
	BoundsBox = CreateDefaultSubobject<UBoxComponent>(TEXT("NavBoundsBox"));
	BoundsBox->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	BoundsBox->Mobility = EComponentMobility::Movable;
	// Replace the (empty at runtime) brush root so GetComponentsBoundingBox is real.
	RootComponent = BoundsBox;
}

void AHZNavBoundsVolume::SetNavBoxExtent(const FVector& HalfExtent)
{
	if (BoundsBox)
	{
		BoundsBox->SetBoxExtent(HalfExtent, false);
	}
}
