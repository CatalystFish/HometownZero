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
	void SpawnZombieWave();

	/** Spawns ZombieWaveSize zombies at random road vertices; returns spawned count. */
	int32 SpawnZombiesAtRoads(int32 Count);

	/** Spawns zombies on street points in a ring around Center. */
	int32 SpawnZombiesNear(const FVector2D& Center, float MinDistM, float MaxDistM, int32 Count);

	/** True if the point falls inside any building AABB (+1m margin). */
	bool IsPointBlocked(const FVector2D& Point) const;

	/** Picks an open street point near the district center as the player spawn. */
	void PickPlayerSpawn();

	/** Delayed setup: relocate player to a safe street point + seed nearby zombies. */
	void LateStart();

	struct FBuildingBox
	{
		FVector2D Min;
		FVector2D Max;
		float HeightM = 0.f;
	};

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
	FTimerHandle LateStartTimer;

	/** Building AABBs with 1m margin, for spawn-point validation. */
	TArray<FBuildingBox> BuildingBoxes;

	/** Chosen open-street or rooftop player spawn (local meters). */
	FVector2D PlayerSpawnPoint = FVector2D::ZeroVector;
	float PlayerSpawnHeightM = 0.f;
	bool bHasPlayerSpawn = false;
	bool bRooftopSpawn = false;

	int32 NumZombiesSpawned = 0;
};
