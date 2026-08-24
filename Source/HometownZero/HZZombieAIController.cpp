#include "HZZombieAIController.h"

#include "GameFramework/Character.h"
#include "HZCharacter.h"
#include "HZZombie.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"

namespace
{
	constexpr float ThinkIntervalSeconds = 0.5f;
	constexpr float ChaseRangeCm = 8000.f;
	constexpr float WanderRadiusCm = 2000.f;
	constexpr float ReachedWanderDistanceCm = 100.f;
}

AHZZombieAIController::AHZZombieAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = ThinkIntervalSeconds;
}

void AHZZombieAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (InPawn)
	{
		SpawnLocation = InPawn->GetActorLocation();
	}
}

void AHZZombieAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TimeSinceLastThink += DeltaSeconds;
	if (TimeSinceLastThink < ThinkIntervalSeconds)
	{
		return;
	}
	UpdateBehavior();
}

void AHZZombieAIController::UpdateBehavior()
{
	TimeSinceLastThink = 0.f;

	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	AHZCharacter* PlayerCharacter = Cast<AHZCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (PlayerCharacter)
	{
		const float DistanceSq = FVector::DistSquared(ControlledPawn->GetActorLocation(), PlayerCharacter->GetActorLocation());
		if (DistanceSq <= FMath::Square(ChaseRangeCm))
		{
			bHasWanderTarget = false;
			MoveToActor(PlayerCharacter, 120.f);
			return;
		}
	}

	// Wander around spawn.
	if (!bHasWanderTarget || GetMoveStatus() != EPathFollowingStatus::Moving
		|| FVector::DistSquared(ControlledPawn->GetActorLocation(), SpawnLocation) > FMath::Square(WanderRadiusCm))
	{
		const FVector Target = SpawnLocation + FVector(
			FMath::FRandRange(-WanderRadiusCm, WanderRadiusCm),
			FMath::FRandRange(-WanderRadiusCm, WanderRadiusCm),
			0.f);
		bHasWanderTarget =
			MoveToLocation(Target, ReachedWanderDistanceCm) == EPathFollowingRequestResult::RequestSuccessful;
	}
}
