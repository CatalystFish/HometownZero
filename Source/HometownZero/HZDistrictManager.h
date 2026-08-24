#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HZDistrictManager.generated.h"

class USceneComponent;
class UHierarchicalInstancedStaticMeshComponent;
class AHZLootContainer;

/**
 * Path B runtime district loader: reads a district.json produced by
 * Scripts/osm_pipeline.py (schema version 2 contract) and spawns one
 * HierarchicalInstancedStaticMesh cube per building footprint, grouped by
 * category. Also seeds navigation bounds, loot containers and zombies.
 */
UCLASS()
class HOMETOWNZERO_API AHDistrictManager : public AActor
{
	GENERATED_BODY()

public:
	AHDistrictManager();

	/** Convenience accessor for the one manager in the world. */
	static AHDistrictManager* Get(const UWorld* World);

	const TArray<TObjectPtr<AHZLootContainer>>& GetContainers() const { return Containers; }

protected:
	virtual void BeginPlay() override;

	/** Project-relative path to the district JSON (schema v2). */
	UPROPERTY(EditAnywhere, Category = "District")
	FString DistrictJsonPath;

private:
	struct FBuildingData
	{
		int32 Id = 0;
		FString Category;
		float HeightM = 0.f;
		TArray<FVector2D> Footprint;
	};

	struct FRoadData
	{
		int32 Id = 0;
		FString RoadClass;
		float WidthM = 0.f;
		TArray<FVector2D> Polyline;
	};

	bool ParseDistrictJson();

	void SpawnGround();
	void SpawnBuildings();
	void SpawnRoads();
	void SpawnLootContainers();
	void SpawnNavigationBounds();
	void SpawnZombies();
	void SpawnZombieWave();

	/** Spawns ZombieWaveSize zombies at random road vertices; returns spawned count. */
	int32 SpawnZombiesAtRoads(int32 Count);

	UMaterialInterface* FindCategoryMaterial(const FString& Category);

	UPROPERTY(VisibleAnywhere, Category = "District")
	TObjectPtr<USceneComponent> DistrictRoot;

	TArray<FBuildingData> Buildings;
	TArray<FRoadData> Roads;

	/** District bounds in local meters (east/north), from buildings + roads. */
	FBox2D DistrictBounds;

	/** Category name -> instanced mesh component. */
	TMap<FString, TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> CategoryInstances;

	/** Searchable loot containers, one per suitably-sized building. */
	TArray<TObjectPtr<AHZLootContainer>> Containers;

	FTimerHandle HordeWaveTimer;

	int32 NumZombiesSpawned = 0;
};
