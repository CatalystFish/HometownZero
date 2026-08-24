#include "HZDistrictManager.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HZLootContainer.h"
#include "HZNavBoundsVolume.h"
#include "HZZombie.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "NavigationSystem.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

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
	SpawnZombies();
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
	return true;
}

void AHDistrictManager::SpawnBuildings()
{
	UWorld* World = GetWorld();
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!World || !CubeMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("[H District] Missing world or engine cube mesh - no buildings spawned"));
		return;
	}

	TMap<FString, int32> CategoryCounts;

	for (const FBuildingData& Building : Buildings)
	{
		TObjectPtr<UHierarchicalInstancedStaticMeshComponent>& Instances =
			CategoryInstances.FindOrAdd(Building.Category);
		if (!Instances)
		{
			UHierarchicalInstancedStaticMeshComponent* Component =
				NewObject<UHierarchicalInstancedStaticMeshComponent>(
					this, FName(*FString::Printf(TEXT("HISM_%s"), *Building.Category)));
			Component->SetupAttachment(GetRootComponent());
			Component->SetMobility(EComponentMobility::Static);
			Component->SetStaticMesh(CubeMesh);
			Component->RegisterComponent();

			if (UMaterialInterface* Material = FindCategoryMaterial(Building.Category))
			{
				Component->SetMaterial(0, Material);
			}
			Instances = Component;
		}

		FVector2D Min(TNumericLimits<float>::Max(), TNumericLimits<float>::Max());
		FVector2D Max(TNumericLimits<float>::Lowest(), TNumericLimits<float>::Lowest());
		FVector2D Sum = FVector2D::ZeroVector;
		for (const FVector2D& Point : Building.Footprint)
		{
			Min = Min.ComponentMin(Point);
			Max = Max.ComponentMax(Point);
			Sum += Point;
		}
		const FVector2D Centroid = Sum / static_cast<float>(Building.Footprint.Num());

		const float WidthCm = FMath::Max((Max.X - Min.X) * 100.f, 1.f);
		const float DepthCm = FMath::Max((Max.Y - Min.Y) * 100.f, 1.f);
		const float HeightCm = FMath::Max(Building.HeightM * 100.f, 1.f);

		// Engine cube is a centered 100cm box: scale to footprint size and lift by half height so it rests on z=0.
		const FTransform InstanceTransform(FRotator::ZeroRotator,
			FVector(Centroid.X * 100.f, Centroid.Y * 100.f, HeightCm * 0.5f),
			FVector(WidthCm / 100.f, DepthCm / 100.f, HeightCm / 100.f));

		Instances->AddInstance(InstanceTransform, false);
		CategoryCounts.FindOrAdd(Building.Category) += 1;
	}

	int32 TotalInstances = 0;
	for (const TPair<FString, TObjectPtr<UHierarchicalInstancedStaticMeshComponent>>& Pair : CategoryInstances)
	{
		const int32 Count = Pair.Value != nullptr ? Pair.Value->GetInstanceCount() : 0;
		TotalInstances += Count;
		UE_LOG(LogTemp, Warning, TEXT("[H District] Category '%s': %d instances"), *Pair.Key, Count);
	}
	UE_LOG(LogTemp, Warning, TEXT("[H District] Spawned %d buildings across %d categories (%d instances total)"),
		Buildings.Num(), CategoryInstances.Num(), TotalInstances);

	(void)CategoryCounts; // kept minimal: counts are reported straight from the components above
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
	AStaticMeshActor* Ground = World->SpawnActor<AStaticMeshActor>(Params);
	if (!Ground)
	{
		return;
	}

	Ground->SetMobility(EComponentMobility::Static);
	UStaticMeshComponent* MeshComponent = Ground->GetStaticMeshComponent();
	MeshComponent->SetStaticMesh(PlaneMesh);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	const FVector2D Center = DistrictBounds.bIsValid ? DistrictBounds.GetCenter() : FVector2D::ZeroVector;
	const FVector2D Size = DistrictBounds.bIsValid
		? DistrictBounds.GetSize() + FVector2D(NavMarginCm / 50.f, NavMarginCm / 50.f)
		: FVector2D(2000.f, 2000.f);
	// Engine plane is 100x100cm: scale to district size in cm, sit 2cm below street ribbons.
	MeshComponent->SetWorldLocationAndRotation(
		FVector(Center.X * 100.f, Center.Y * 100.f, -2.f), FRotator::ZeroRotator);
	MeshComponent->SetWorldScale3D(FVector(Size.X * 100.f / 100.f, Size.Y * 100.f / 100.f, 1.f));
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

void AHDistrictManager::SpawnZombies()
{
	NumZombiesSpawned += SpawnZombiesAtRoads(ZombieCount);
	UE_LOG(LogTemp, Warning, TEXT("[H District] Spawned %d/%d zombies at random road vertices"),
		NumZombiesSpawned, ZombieCount);

	// Session arc: the dead trickle in over time.
	GetWorldTimerManager().SetTimer(HordeWaveTimer, this,
		&AHDistrictManager::SpawnZombieWave, HordeWaveIntervalSeconds, true);
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
	UWorld* World = GetWorld();
	if (!World || Roads.Num() == 0)
	{
		return 0;
	}

	// Building AABBs (with 1m margin) - zombies spawning inside these would be
	// stuck in the solid boxes forever. Precompute once per call.
	struct FBuildingBox
	{
		FVector2D Min;
		FVector2D Max;
	};
	TArray<FBuildingBox> BuildingBoxes;
	BuildingBoxes.Reserve(Buildings.Num());
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
		BuildingBoxes.Add(Box);
	}
	const auto IsBlocked = [&BuildingBoxes](const FVector2D& Point) -> bool
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
	};

	int32 Spawned = 0;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		// Try several road vertices; accept the first one on open street.
		FVector2D Chosen(TNumericLimits<float>::Max(), TNumericLimits<float>::Max());
		for (int32 Attempt = 0; Attempt < 12; ++Attempt)
		{
			const FRoadData& Road = Roads[FMath::RandRange(0, Roads.Num() - 1)];
			if (Road.Polyline.Num() == 0)
			{
				continue;
			}
			const FVector2D& Point = Road.Polyline[FMath::RandRange(0, Road.Polyline.Num() - 1)];
			if (!IsBlocked(Point))
			{
				Chosen = Point;
				break;
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
