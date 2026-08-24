#include "HZLootContainer.h"

#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "HZCharacter.h"
#include "HZLootPickup.h"
#include "Materials/MaterialInterface.h"

namespace
{
	struct FLootEntry
	{
		const TCHAR* Item;
		float Weight;
	};

	// The 13 category tables. Weights within a table are relative.
	const TMap<FString, TArray<FLootEntry>>& LootTables()
	{
		static const TMap<FString, TArray<FLootEntry>> Tables = {
			{TEXT("medical"), {{TEXT("Bandage"), .35f}, {TEXT("Antibiotics"), .25f}, {TEXT("Painkillers"), .2f}, {TEXT("SurgicalKit"), .2f}}},
			{TEXT("food"), {{TEXT("CannedBeans"), .3f}, {TEXT("WaterBottle"), .3f}, {TEXT("Jerky"), .2f}, {TEXT("FreshProduce"), .2f}}},
			{TEXT("hardware"), {{TEXT("Nails"), .3f}, {TEXT("Planks"), .3f}, {TEXT("Hammer"), .2f}, {TEXT("DuctTape"), .2f}}},
			{TEXT("weapons_outdoors"), {{TEXT("Arrows"), .3f}, {TEXT("HuntingKnife"), .25f}, {TEXT("Rope"), .25f}, {TEXT("Compass"), .2f}}},
			{TEXT("retail"), {{TEXT("DuctTape"), .3f}, {TEXT("Batteries"), .25f}, {TEXT("Lighter"), .25f}, {TEXT("Newspaper"), .2f}}},
			{TEXT("office"), {{TEXT("Coffee"), .3f}, {TEXT("Documents"), .25f}, {TEXT("Electronics"), .25f}, {TEXT("Pens"), .2f}}},
			{TEXT("residential"), {{TEXT("CannedFood"), .25f}, {TEXT("Flashlight"), .3f}, {TEXT("Batteries"), .2f}, {TEXT("Clothes"), .25f}}},
			{TEXT("emergency"), {{TEXT("RiotHelmet"), .3f}, {TEXT("Radio"), .3f}, {TEXT("Flares"), .2f}, {TEXT("FirstAidKit"), .2f}}},
			{TEXT("education"), {{TEXT("SkillBook"), .35f}, {TEXT("Map"), .35f}, {TEXT("Chalk"), .3f}}},
			{TEXT("fuel"), {{TEXT("GasCan"), .4f}, {TEXT("Propane"), .3f}, {TEXT("LighterFluid"), .3f}}},
			{TEXT("industrial"), {{TEXT("ScrapMetal"), .35f}, {TEXT("Wire"), .35f}, {TEXT("MachineParts"), .3f}}},
			{TEXT("civic"), {{TEXT("GeneratorSchematic"), .3f}, {TEXT("CommunityPlans"), .35f}, {TEXT("Seeds"), .35f}}},
			{TEXT("unknown"), {{TEXT("Junk"), .4f}, {TEXT("Scrap"), .3f}, {TEXT("MysteryBox"), .3f}}},
		};
		return Tables;
	}

	FString WeightedPick(const TArray<FLootEntry>& Table)
	{
		float Total = 0.f;
		for (const FLootEntry& Entry : Table)
		{
			Total += Entry.Weight;
		}
		float Roll = FMath::FRandRange(0.f, Total);
		for (const FLootEntry& Entry : Table)
		{
			Roll -= Entry.Weight;
			if (Roll <= 0.f)
			{
				return Entry.Item;
			}
		}
		return Table.Last().Item;
	}
}

AHZLootContainer::AHZLootContainer()
{
	PrimaryActorTick.bCanEverTick = false;

	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	if (CubeMesh)
	{
		MeshComponent->SetStaticMesh(CubeMesh);
	}
	SetRootComponent(MeshComponent);
	MeshComponent->SetRelativeScale3D(FVector(0.6f, 0.6f, 0.8f));
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Game/HZMaterials/M_HZ_hardware.M_HZ_hardware")))
	{
		MeshComponent->SetMaterial(0, Material);
	}
}

TArray<FString> AHZLootContainer::RollLootForCategory(const FString& Category, int32 Count)
{
	TArray<FString> Result;
	const TArray<FLootEntry>* Table = LootTables().Find(Category);
	if (!Table)
	{
		Table = LootTables().Find(TEXT("unknown"));
	}
	for (int32 Index = 0; Index < Count && Table; ++Index)
	{
		Result.Add(WeightedPick(*Table));
	}
	return Result;
}

void AHZLootContainer::Open(AActor* Taker)
{
	if (bOpened)
	{
		return;
	}
	bOpened = true;

	UWorld* World = GetWorld();
	if (!World || !Taker)
	{
		return;
	}

	TArray<FString> Items = RollLootForCategory(Category, FMath::RandRange(1, 3));
	const FVector Base = Taker->GetActorLocation() + FVector(60.f, 0.f, 40.f);
	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		const float Angle = (360.f / Items.Num()) * Index + FMath::FRandRange(-20.f, 20.f);
		const FVector Offset(FMath::Cos(FMath::DegreesToRadians(Angle)) * 50.f,
			FMath::Sin(FMath::DegreesToRadians(Angle)) * 50.f, 0.f);

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AHZLootPickup* Pickup = World->SpawnActor<AHZLootPickup>(
			Base + Offset, FRotator::ZeroRotator, Params);
		if (Pickup)
		{
			Pickup->ItemName = Items[Index];
			Pickup->Category = Category;
		}
	}

	// Visually mark as looted: drop it into the road-dark material.
	if (UMaterialInterface* OpenedMaterial = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Game/HZMaterials/M_HZ_road.M_HZ_road")))
	{
		MeshComponent->SetMaterial(0, OpenedMaterial);
	}
	UE_LOG(LogTemp, Log, TEXT("[HZ Loot] container '%s' opened -> %s"),
		*Category, *FString::Join(Items, TEXT(", ")));
}
