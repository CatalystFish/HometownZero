#include "HZDistrictManager.h"

#include "Components/BoxComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GeomTools.h"
#include "HAL/FileManager.h"
#include "HZLootContainer.h"
#include "HZNavBoundsVolume.h"
#include "HZZombie.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "NavigationSystem.h"
#include "ProceduralMeshComponent.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	constexpr int32 ExpectedSchemaVersion = 2;
	constexpr int32 ZombieCount = 10;
	constexpr float NavMarginCm = 500.f;
	constexpr float HordeWaveIntervalSeconds = 60.f;
	constexpr int32 HordeWaveSize = 2;
	constexpr int32 MaxZombies = 40;
}

AHDistrictManager::AHDistrictManager()
{
	PrimaryActorTick.bCanEverTick = false;

	DistrictRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DistrictRoot"));
	DistrictRoot->SetMobility(EComponentMobility::Static);
	SetRootComponent(DistrictRoot);

	DistrictJsonPath = TEXT("Scripts/samples/pike_place.json");
	DistrictBounds.Init();
}

void AHDistrictManager::BeginPlay()
{
	Super::BeginPlay();

	if (!ParseDistrictJson())
	{
		return;
	}

	SpawnGround();
	SpawnBuildings();
	SpawnRoads();
	SpawnLootContainers();
	SpawnNavigationBounds();
	PickPlayerSpawn();

	// Delayed setup: let the world (and navmesh) settle, then relocate the
	// player to a validated street point and seed zombies around them.
	GetWorldTimerManager().SetTimer(LateStartTimer, this,
		&AHDistrictManager::LateStart, 0.5f, false);
}

AHDistrictManager* AHDistrictManager::Get(const UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<AHDistrictManager> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

bool AHDistrictManager::ParseDistrictJson()
{
	const FString FullPath = FPaths::ProjectDir() / DistrictJsonPath;
	FString RawJson;
	if (!FFileHelper::LoadFileToString(RawJson, *FullPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[H District] Failed to read district JSON at %s"), *FullPath);
		return false;
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(RawJson);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[H District] Failed to parse district JSON at %s"), *FullPath);
		return false;
	}

	double Version = 0.0;
	if (Root->TryGetNumberField(TEXT("version"), Version) && FMath::RoundToInt(Version) != ExpectedSchemaVersion)
	{
		UE_LOG(LogTemp, Warning, TEXT("[H District] district.json schema version %.0f, expected %d - continuing anyway"),
			Version, ExpectedSchemaVersion);
	}

	DistrictBounds.Init();

	const TArray<TSharedPtr<FJsonValue>>* BuildingArray = nullptr;
	if (Root->TryGetArrayField(TEXT("buildings"), BuildingArray))
	{
		Buildings.Reserve(BuildingArray->Num());
		for (const TSharedPtr<FJsonValue>& Value : *BuildingArray)
		{
			const TSharedPtr<FJsonObject>* Obj = nullptr;
			if (!Value.IsValid() || !Value->TryGetObject(Obj))
			{
				continue;
			}

			FBuildingData Building;
			double Number = 0.0;
			FString String;
			Building.Id = (*Obj)->TryGetNumberField(TEXT("id"), Number) ? static_cast<int32>(Number) : -1;
			if (!(*Obj)->TryGetStringField(TEXT("category"), String))
			{
				String = TEXT("unknown");
			}
			Building.Category = String;
			Building.HeightM = (*Obj)->TryGetNumberField(TEXT("height_m"), Number) ? static_cast<float>(Number) : 5.f;

			const TArray<TSharedPtr<FJsonValue>>* FootprintArray = nullptr;
			if ((*Obj)->TryGetArrayField(TEXT("footprint"), FootprintArray))
			{
				for (const TSharedPtr<FJsonValue>& PointValue : *FootprintArray)
				{
					const TArray<TSharedPtr<FJsonValue>>* PointArray = nullptr;
					if (!PointValue.IsValid() || !PointValue->TryGetArray(PointArray) || PointArray->Num() < 2)
					{
						continue;
					}
					const FVector2D Point((*PointArray)[0]->AsNumber(), (*PointArray)[1]->AsNumber());
					Building.Footprint.Add(Point);
					DistrictBounds += Point;
				}
			}

			if (Building.Footprint.Num() >= 2)
			{
				Buildings.Add(MoveTemp(Building));
			}
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* RoadArray = nullptr;
	if (Root->TryGetArrayField(TEXT("roads"), RoadArray))
	{
		Roads.Reserve(RoadArray->Num());
		for (const TSharedPtr<FJsonValue>& Value : *RoadArray)
		{
			const TSharedPtr<FJsonObject>* Obj = nullptr;
			if (!Value.IsValid() || !Value->TryGetObject(Obj))
			{
				continue;
			}

			FRoadData Road;
			double Number = 0.0;
			FString String;
			Road.Id = (*Obj)->TryGetNumberField(TEXT("id"), Number) ? static_cast<int32>(Number) : -1;
			if ((*Obj)->TryGetStringField(TEXT("class"), String))
			{
				Road.RoadClass = String;
			}
			Road.WidthM = (*Obj)->TryGetNumberField(TEXT("width_m"), Number) ? static_cast<float>(Number) : 3.f;

			const TArray<TSharedPtr<FJsonValue>>* LineArray = nullptr;
			if ((*Obj)->TryGetArrayField(TEXT("polyline"), LineArray))
			{
				for (const TSharedPtr<FJsonValue>& PointValue : *LineArray)
				{
					const TArray<TSharedPtr<FJsonValue>>* PointArray = nullptr;
					if (!PointValue.IsValid() || !PointValue->TryGetArray(PointArray) || PointArray->Num() < 2)
					{
						continue;
					}
					const FVector2D Point((*PointArray)[0]->AsNumber(), (*PointArray)[1]->AsNumber());
					Road.Polyline.Add(Point);
					DistrictBounds += Point;
				}
			}

			if (Road.Polyline.Num() >= 1)
			{
				Roads.Add(MoveTemp(Road));
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[H District] Parsed %s: version=%.0f buildings=%d roads=%d bounds=%s"),
		*FullPath, Version, Buildings.Num(), Roads.Num(),
		DistrictBounds.bIsValid ? *DistrictBounds.ToString() : TEXT("invalid"));

	// Precompute building AABBs (+1m margin) for spawn validation.
	BuildingBoxes.Reset();
	for (const FBuildingData& Building : Buildings)
	{
		FBuildingBox Box;
		Box.Min = FVector2D(TNumericLimits<float>::Max(), TNumericLimits<float>::Max());
		Box.Max = FVector2D(TNumericLimits<float>::Lowest(), TNumericLimits<float>::Lowest());
		for (const FVector2D& Point : Building.Footprint)
		{
			Box.Min = Box.Min.ComponentMin(Point);
			Box.Max = Box.Max.ComponentMax(Point);
		}
		Box.Min -= FVector2D(1.f, 1.f);
		Box.Max += FVector2D(1.f, 1.f);
		Box.HeightM = Building.HeightM;
		BuildingBoxes.Add(Box);
	}
	return true;
}

void AHDistrictManager::SpawnBuildings()
{
	UWorld* World = GetWorld();
	if (!World || Buildings.Num() == 0)
	{
		return;
	}

	int32 Spawned = 0;
	for (const FBuildingData& Building : Buildings)
	{
		if (Building.Footprint.Num() < 3)
		{
			continue;
		}

		const float HeightCm = FMath::Max(Building.HeightM * 100.f, 1.f);
		const int32 N = Building.Footprint.Num();

		TArray<FVector2D> Ring2D;
		Ring2D.Reserve(N);
		FVector2D Centroid = FVector2D::ZeroVector;
		FVector2D Min(TNumericLimits<float>::Max(), TNumericLimits<float>::Max());
		FVector2D Max(TNumericLimits<float>::Lowest(), TNumericLimits<float>::Lowest());
		for (const FVector2D& Point : Building.Footprint)
		{
			Ring2D.Add(Point);
			Centroid += Point;
			Min = Min.ComponentMin(Point);
			Max = Max.ComponentMax(Point);
		}
		Centroid /= static_cast<float>(N);

		// Roof triangulation: flat list of vertex POSITIONS, 3 per triangle.
		// Footprints are CCW (pipeline reverses to positive shoelace area),
		// which is exactly what FGeomTools2D expects.
		TArray<FVector2D> RoofTris2D;
		if (!FGeomTools2D::TriangulatePoly(RoofTris2D, Ring2D))
		{
			continue; // degenerate footprint, skip
		}

		TArray<FVector> Vertices;
		TArray<int32> Indices;
		TArray<FVector> Normals;

		// Emit one triangle (both windings, same outward normal) so faces are
		// visible regardless of winding and light correctly.
		auto EmitTriNormal = [&](const FVector& A, const FVector& B, const FVector& C, const FVector& Normal)
		{
			const int32 Base = Vertices.Num();
			Vertices.Add(A); Vertices.Add(B); Vertices.Add(C);
			Normals.Add(Normal); Normals.Add(Normal); Normals.Add(Normal);
			Indices.Add(Base); Indices.Add(Base + 1); Indices.Add(Base + 2);
			Indices.Add(Base); Indices.Add(Base + 2); Indices.Add(Base + 1);
		};

		// Walls: outward horizontal normal = (dy, -dx) for a CCW ring.
		for (int32 i = 0; i < N; ++i)
		{
			const int32 j = (i + 1) % N;
			const FVector2D P0 = Ring2D[i];
			const FVector2D P1 = Ring2D[j];
			const FVector2D Edge = P1 - P0;
			const FVector WallNormal(Edge.Y, -Edge.X, 0.f);
			const FVector N2 = WallNormal.GetSafeNormal();

			const FVector A(P0.X * 100.f, P0.Y * 100.f, 0.f);
			const FVector B(P1.X * 100.f, P1.Y * 100.f, 0.f);
			const FVector C(P1.X * 100.f, P1.Y * 100.f, HeightCm);
			const FVector D(P0.X * 100.f, P0.Y * 100.f, HeightCm);
			EmitTriNormal(A, B, C, N2);
			EmitTriNormal(A, C, D, N2);
		}

		// Roof.
		for (int32 t = 0; t + 2 < RoofTris2D.Num(); t += 3)
		{
			const FVector A(RoofTris2D[t].X * 100.f, RoofTris2D[t].Y * 100.f, HeightCm);
			const FVector B(RoofTris2D[t + 1].X * 100.f, RoofTris2D[t + 1].Y * 100.f, HeightCm);
			const FVector C(RoofTris2D[t + 2].X * 100.f, RoofTris2D[t + 2].Y * 100.f, HeightCm);
			EmitTriNormal(A, B, C, FVector::UpVector);
		}

		// Visual mesh.
		UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(this);
		Mesh->SetupAttachment(GetRootComponent());
		Mesh->SetMobility(EComponentMobility::Static);
		Mesh->CreateMeshSection(0, Vertices, Indices, Normals,
			TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>(), false);
		if (UMaterialInterface* Material = FindCategoryMaterial(Building.Category))
		{
			Mesh->SetMaterial(0, Material);
		}
		Mesh->RegisterComponent();

		// Cheap box collision per prism (spec: box/convex, not complex-as-simple).
		UBoxComponent* Collision = NewObject<UBoxComponent>(this);
		Collision->SetupAttachment(GetRootComponent());
		Collision->SetMobility(EComponentMobility::Static);
		Collision->SetBoxExtent(FVector(
			FMath::Max((Max.X - Min.X) * 100.f * 0.5f, 1.f),
			FMath::Max((Max.Y - Min.Y) * 100.f * 0.5f, 1.f),
			HeightCm * 0.5f));
		Collision->SetRelativeLocation(FVector(Centroid.X * 100.f, Centroid.Y * 100.f, HeightCm * 0.5f));
		Collision->SetCollisionProfileName(TEXT("BlockAll"));
		Collision->RegisterComponent();

		++Spawned;
	}

	UE_LOG(LogTemp, Warning, TEXT("[H District] Spawned %d building prisms (real footprints)"), Spawned);
}

void AHDistrictManager::SpawnGround()
{
	UWorld* World = GetWorld();
	UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	UMaterialInterface* GroundMaterial = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Game/HZMaterials/M_HZ_ground.M_HZ_ground"));
	if (!World || !PlaneMesh)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector2D Center = DistrictBounds.bIsValid ? DistrictBounds.GetCenter() : FVector2D::ZeroVector;
	const FVector2D Size = DistrictBounds.bIsValid
		? DistrictBounds.GetSize() + FVector2D(NavMarginCm / 50.f, NavMarginCm / 50.f)
		: FVector2D(2000.f, 2000.f);
	// Spawn directly at the target transform; a single plane stays Movable
	// (one actor, zero perf cost) so we avoid static-mobility move errors.
	AStaticMeshActor* Ground = World->SpawnActor<AStaticMeshActor>(
		FVector(Center.X * 100.f, Center.Y * 100.f, -2.f), FRotator::ZeroRotator, Params);
	if (!Ground)
	{
		return;
	}

	UStaticMeshComponent* MeshComponent = Ground->GetStaticMeshComponent();
	MeshComponent->SetStaticMesh(PlaneMesh);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	// Engine plane is 100x100cm: scale to district size in cm.
	MeshComponent->SetWorldScale3D(FVector(Size.X, Size.Y, 1.f));
	if (GroundMaterial)
	{
		MeshComponent->SetMaterial(0, GroundMaterial);
	}
	UE_LOG(LogTemp, Log, TEXT("[H District] Ground plane spawned at %s size %s"),
		*Center.ToString(), *Size.ToString());
}

void AHDistrictManager::SpawnRoads()
{
	UWorld* World = GetWorld();
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UMaterialInterface* RoadMaterial = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Game/HZMaterials/M_HZ_road.M_HZ_road"));
	if (!World || !CubeMesh || Roads.Num() == 0)
	{
		return;
	}

	UHierarchicalInstancedStaticMeshComponent* RoadInstances =
		NewObject<UHierarchicalInstancedStaticMeshComponent>(this, TEXT("HISM_Roads"));
	RoadInstances->SetupAttachment(GetRootComponent());
	RoadInstances->SetMobility(EComponentMobility::Static);
	RoadInstances->SetStaticMesh(CubeMesh);
	RoadInstances->RegisterComponent();
	if (RoadMaterial)
	{
		RoadInstances->SetMaterial(0, RoadMaterial);
	}

	int32 Segments = 0;
	for (const FRoadData& Road : Roads)
	{
		for (int32 Index = 0; Index + 1 < Road.Polyline.Num(); ++Index)
		{
			const FVector2D& Start = Road.Polyline[Index];
			const FVector2D& End = Road.Polyline[Index + 1];
			const FVector2D Delta = End - Start;
			const float LengthCm = FMath::Max(Delta.Size() * 100.f, 1.f);
			const float WidthCm = FMath::Max(Road.WidthM * 100.f, 50.f);
			const float YawRadians = FMath::Atan2(Delta.Y, Delta.X);

			const FVector2D Mid = (Start + End) * 0.5f;
			const FTransform InstanceTransform(
				FRotator(0.f, FMath::RadiansToDegrees(YawRadians), 0.f),
				FVector(Mid.X * 100.f, Mid.Y * 100.f, 1.f),
				FVector(LengthCm / 100.f, WidthCm / 100.f, 0.1f));
			RoadInstances->AddInstance(InstanceTransform, false);
			++Segments;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[H District] Spawned %d road segments across %d roads"),
		Segments, Roads.Num());
}

void AHDistrictManager::SpawnLootContainers()
{
	UWorld* World = GetWorld();
	if (!World || Buildings.Num() == 0)
	{
		return;
	}

	for (const FBuildingData& Building : Buildings)
	{
		if (Building.Footprint.Num() < 3)
		{
			continue;
		}

		// Only buildings big enough to bother searching.
		float Area = 0.f;
		FVector2D Centroid = FVector2D::ZeroVector;
		for (int32 Index = 0; Index < Building.Footprint.Num(); ++Index)
		{
			const FVector2D& A = Building.Footprint[Index];
			const FVector2D& B = Building.Footprint[(Index + 1) % Building.Footprint.Num()];
			Area += A.X * B.Y - B.X * A.Y;
			Centroid += A;
		}
		Centroid /= static_cast<float>(Building.Footprint.Num());
		if (FMath::Abs(Area) * 0.5f < 30.f)
		{
			continue;
		}

		// Drop the container at a footprint corner, nudged outward so it sits
		// on the street side of the wall (buildings are solid in the spike).
		const FVector2D& Corner = Building.Footprint[0];
		const FVector2D Outward = (Corner - Centroid).GetSafeNormal();
		const FVector Location((Corner.X + Outward.X * 1.5f) * 100.f,
			(Corner.Y + Outward.Y * 1.5f) * 100.f, 40.f);

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AHZLootContainer* Container = World->SpawnActor<AHZLootContainer>(Location, FRotator::ZeroRotator, Params);
		if (Container)
		{
			Container->Category = Building.Category;
			Containers.Add(Container);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[H District] Spawned %d loot containers"), Containers.Num());
}

void AHDistrictManager::SpawnNavigationBounds()
{
	UWorld* World = GetWorld();
	if (!World || !DistrictBounds.bIsValid)
	{
		return;
	}

	AHZNavBoundsVolume* Volume = World->SpawnActor<AHZNavBoundsVolume>();
	if (!Volume)
	{
		return;
	}

	const FVector Center(DistrictBounds.GetCenter().X * 100.f, DistrictBounds.GetCenter().Y * 100.f, 0.f);
	const FVector HalfExtent(
		(DistrictBounds.GetSize().X * 100.f) * 0.5f + NavMarginCm,
		(DistrictBounds.GetSize().Y * 100.f) * 0.5f + NavMarginCm,
		400.f);

	Volume->SetActorLocation(Center);
	Volume->SetNavBoxExtent(HalfExtent);

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (NavSys)
	{
		NavSys->OnNavigationBoundsUpdated(Volume);
		UE_LOG(LogTemp, Warning, TEXT("[H District] Nav bounds set: center=%s half-extent=%s"),
			*Center.ToCompactString(), *HalfExtent.ToCompactString());
	}
}

void AHDistrictManager::LateStart()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Relocate the player to the validated spawn point (rooftop or street).
	if (bHasPlayerSpawn)
	{
		const float SpawnZCm = bRooftopSpawn ? PlayerSpawnHeightM * 100.f + 120.f : 120.f;
		const FVector SpawnCm(PlayerSpawnPoint.X * 100.f, PlayerSpawnPoint.Y * 100.f, SpawnZCm);
		if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0))
		{
			Pawn->SetActorLocation(SpawnCm);
			if (ACharacter* Character = Cast<ACharacter>(Pawn))
			{
				Character->GetCharacterMovement()->Velocity = FVector::ZeroVector;
			}
			UE_LOG(LogTemp, Warning, TEXT("[H District] Player relocated to %s spawn (%.0f, %.0f, z=%.0fm)"),
				bRooftopSpawn ? TEXT("ROOFTOP") : TEXT("street"),
				PlayerSpawnPoint.X, PlayerSpawnPoint.Y,
				bRooftopSpawn ? PlayerSpawnHeightM : 0.f);
		}
	}

	// Initial horde seeds around the player so the threat is visible fast.
	const int32 Spawned = bHasPlayerSpawn
		? SpawnZombiesNear(PlayerSpawnPoint, 15.f, 80.f, ZombieCount)
		: SpawnZombiesAtRoads(ZombieCount);
	NumZombiesSpawned += Spawned;
	UE_LOG(LogTemp, Warning, TEXT("[H District] Spawned %d/%d zombies near the player"),
		Spawned, ZombieCount);

	// Session arc: the dead trickle in over time.
	GetWorldTimerManager().SetTimer(HordeWaveTimer, this,
		&AHDistrictManager::SpawnZombieWave, HordeWaveIntervalSeconds, true);
}

bool AHDistrictManager::IsPointBlocked(const FVector2D& Point) const
{
	for (const FBuildingBox& Box : BuildingBoxes)
	{
		if (Box.Min.X <= Point.X && Point.X <= Box.Max.X &&
			Box.Min.Y <= Point.Y && Point.Y <= Box.Max.Y)
		{
			return true;
		}
	}
	return false;
}

void AHDistrictManager::PickPlayerSpawn()
{
	// Preferred: rooftop of the tallest substantial building near the
	// district center - an unmistakable "you can see everything" opening.
	const FVector2D Center = DistrictBounds.bIsValid ? DistrictBounds.GetCenter() : FVector2D::ZeroVector;
	float BestScore = 0.f;
	bool bFoundRoof = false;
	for (int32 Index = 0; Index < Buildings.Num(); ++Index)
	{
		const FBuildingData& Building = Buildings[Index];
		const FBuildingBox& Box = BuildingBoxes[Index];
		const float AreaM = (Box.Max.X - Box.Min.X) * (Box.Max.Y - Box.Min.Y);
		if (AreaM < 400.f)
		{
			continue; // too small to land on comfortably
		}
		const float CenterDist = FVector2D::Distance(
			(Building.Footprint.Num() > 0)
				? Building.Footprint[0] : Center, Center);
		if (CenterDist > 250.f)
		{
			continue; // keep the opening near the middle of the map
		}
		// Score: tall and central wins.
		const float Score = Building.HeightM * 1000.f - CenterDist * 10.f;
		if (Score > BestScore)
		{
			BestScore = Score;
			FVector2D Centroid = FVector2D::ZeroVector;
			for (const FVector2D& Point : Building.Footprint)
			{
				Centroid += Point;
			}
			Centroid /= static_cast<float>(Building.Footprint.Num());
			PlayerSpawnPoint = Centroid;
			PlayerSpawnHeightM = Building.HeightM;
			bFoundRoof = true;
		}
	}

	if (bFoundRoof)
	{
		bRooftopSpawn = true;
		bHasPlayerSpawn = true;
		UE_LOG(LogTemp, Warning, TEXT("[H District] Player rooftop spawn at (%.0f, %.0f), %.0fm up"),
			PlayerSpawnPoint.X, PlayerSpawnPoint.Y, PlayerSpawnHeightM);
		return;
	}

	// Fallback: open street vertex closest to the district center.
	if (Roads.Num() == 0)
	{
		return;
	}
	float BestDistSq = TNumericLimits<float>::Max();
	bool bFound = false;
	for (const FRoadData& Road : Roads)
	{
		for (const FVector2D& Point : Road.Polyline)
		{
			if (IsPointBlocked(Point))
			{
				continue;
			}
			const float DistSq = FVector2D::DistSquared(Point, Center);
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				PlayerSpawnPoint = Point;
				bFound = true;
			}
		}
	}
	bHasPlayerSpawn = bFound;
	UE_LOG(LogTemp, Warning, TEXT("[H District] Player street spawn %s at (%.0f, %.0f)"),
		bFound ? TEXT("chosen") : TEXT("UNAVAILABLE - keeping placed PlayerStart"),
		PlayerSpawnPoint.X, PlayerSpawnPoint.Y);
}

void AHDistrictManager::SpawnZombieWave()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	// Only count living zombies for the cap.
	int32 LivingZombies = 0;
	for (TActorIterator<AHZZombie> It(World); It; ++It)
	{
		++LivingZombies;
	}
	const int32 ToSpawn = FMath::Min(HordeWaveSize, MaxZombies - LivingZombies);
	if (ToSpawn <= 0)
	{
		return;
	}
	const int32 Spawned = SpawnZombiesAtRoads(ToSpawn);
	NumZombiesSpawned += Spawned;
	UE_LOG(LogTemp, Warning, TEXT("[H District] Horde wave: +%d zombies (living %d, spawned total %d)"),
		Spawned, LivingZombies + Spawned, NumZombiesSpawned);
}

int32 AHDistrictManager::SpawnZombiesAtRoads(int32 Count)
{
	return SpawnZombiesNear(
		DistrictBounds.bIsValid ? DistrictBounds.GetCenter() : FVector2D::ZeroVector,
		0.f, TNumericLimits<float>::Max(), Count);
}

int32 AHDistrictManager::SpawnZombiesNear(const FVector2D& Center, float MinDistM, float MaxDistM, int32 Count)
{
	UWorld* World = GetWorld();
	if (!World || Roads.Num() == 0)
	{
		return 0;
	}

	int32 Spawned = 0;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		// Rejection-sample street vertices: open street, within the ring.
		FVector2D Chosen(TNumericLimits<float>::Max(), TNumericLimits<float>::Max());
		for (int32 Attempt = 0; Attempt < 60; ++Attempt)
		{
			const FRoadData& Road = Roads[FMath::RandRange(0, Roads.Num() - 1)];
			if (Road.Polyline.Num() == 0)
			{
				continue;
			}
			const FVector2D& Point = Road.Polyline[FMath::RandRange(0, Road.Polyline.Num() - 1)];
			if (IsPointBlocked(Point))
			{
				continue;
			}
			const float Dist = FVector2D::Distance(Point, Center);
			if (Dist < MinDistM || Dist > MaxDistM)
			{
				continue;
			}
			Chosen = Point;
			break;
		}
		// Ring sampling can fail in dense cores (sidewalk vertices sit inside
		// inflated building AABBs) - fall back to any open street anywhere.
		if (Chosen.X == TNumericLimits<float>::Max())
		{
			for (int32 Attempt = 0; Attempt < 20; ++Attempt)
			{
				const FRoadData& Road = Roads[FMath::RandRange(0, Roads.Num() - 1)];
				if (Road.Polyline.Num() == 0)
				{
					continue;
				}
				const FVector2D& Point = Road.Polyline[FMath::RandRange(0, Road.Polyline.Num() - 1)];
				if (!IsPointBlocked(Point))
				{
					Chosen = Point;
					break;
				}
			}
		}
		if (Chosen.X == TNumericLimits<float>::Max())
		{
			continue;
		}

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AHZZombie* Zombie = World->SpawnActor<AHZZombie>(
			FVector(Chosen.X * 100.f, Chosen.Y * 100.f, 100.f),
			FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f),
			Params);
		if (Zombie)
		{
			++Spawned;
		}
	}
	return Spawned;
}

UMaterialInterface* AHDistrictManager::FindCategoryMaterial(const FString& Category)
{
	// Deterministic generated materials first (Tools/generate_materials.py).
	if (UMaterialInterface* Generated = LoadObject<UMaterialInterface>(
		nullptr, *FString::Printf(TEXT("/Game/HZMaterials/M_HZ_%s.M_HZ_%s"), *Category, *Category)))
	{
		return Generated;
	}

	// Convention: one .uasset material per category under Content/Districts/<district>/...
	static const FString SearchDirs[] = {
		TEXT("Districts/pike_place/Materials"),
		TEXT("Districts/pike_place/StaticMeshes/pike_place/Materials")
	};

	for (const FString& RelativeDir : SearchDirs)
	{
		const FString AbsoluteDir = FPaths::Combine(FPaths::ProjectContentDir(), RelativeDir);
		TArray<FString> Files;
		IFileManager::Get().FindFiles(Files, *(AbsoluteDir / TEXT("*.uasset")), true, false);

		for (const FString& File : Files)
		{
			if (!File.Contains(Category, ESearchCase::IgnoreCase))
			{
				continue;
			}
			const FString BaseName = FPaths::GetBaseFilename(File);
			const FString ObjectPath = FString(TEXT("/Game/")) / RelativeDir / BaseName / BaseName;
			if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *ObjectPath))
			{
				UE_LOG(LogTemp, Log, TEXT("[H District] Category '%s' using material %s"), *Category, *ObjectPath);
				return Material;
			}
		}
	}

	return nullptr; // fall back to engine default gray
}
