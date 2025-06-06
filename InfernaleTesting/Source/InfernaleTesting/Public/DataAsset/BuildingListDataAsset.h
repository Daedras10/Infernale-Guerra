// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BuildingListDataAsset.generated.h"


class ABuilding;

USTRUCT(Blueprintable)
struct FBuildingStruct
{
	GENERATED_BODY()
public:
	FBuildingStruct();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString BuildingName = "Building";
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Category = "Category";

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BuildingCost = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BuildingCostPerUnit = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float UpgradeCost = 200;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float UpgradeCostPerUnit = 40;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RecycleReturn = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float OverclockCost = 200;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LifeTime = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ConstructionTime = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHealth = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float OverclockSpawnMultiplier = 0.5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UClass* BuildingClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool CanBeBuilt = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool CanBeRecycled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool CanBeOverclocked = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool CanBeUpgraded = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RepulsorRange = 400;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString TooltipInfo = "This is a building";

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* InfoImage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* IconImage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* TypeIconImage;
};

 
USTRUCT(Blueprintable)
struct FCategoryStruct
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString CategorieName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FBuildingStruct> Buildings;
};


/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UBuildingListDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FCategoryStruct> BuildingsCategories;
	
};
