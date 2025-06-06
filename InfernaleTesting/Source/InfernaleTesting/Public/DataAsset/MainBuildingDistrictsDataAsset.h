// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MainBuildingDistrictsDataAsset.generated.h"

class ABreach;

USTRUCT(Blueprintable)
struct FDistrictElementProportion
{
	GENERATED_BODY()
public:
	FDistrictElementProportion();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMesh* DistrictMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Scale = FVector(1, 1, 1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ZOffset = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector BuildingOffset = FVector(0, 0, 100);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Proportion = 1;
};

USTRUCT(Blueprintable)
struct FDistrictElementSpawnInfo
{
	GENERATED_BODY()
public:
	FDistrictElementSpawnInfo();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Id = "";
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> Neighbors = TArray<FString>();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector OffsetFromMainBuilding = FVector(0, 0, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool MustAlwaysSpawn = false;
	

	/* Info */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString MeshId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Scale = FVector(1, 1, 1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BreachOffsetZ = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BuildingOffsetZ = 0;
};

USTRUCT(Blueprintable)
struct FDistrictData
{
	GENERATED_BODY()
public:
	FDistrictData();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Id = "";
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ABreach* Breach;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FDistrictElementSpawnInfo SpawnInfo;
};


USTRUCT(Blueprintable)
struct FDistrictInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FDistrictElementProportion> DistrictMeshes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FDistrictElementProportion> BreachMeshes;

	// UPROPERTY(EditAnywhere, BlueprintReadWrite)
	// int DistrictsToSpawn = 8;
	//
	// UPROPERTY(EditAnywhere, BlueprintReadWrite)
	// float RangeToSpawnDistricts = 400;
};


USTRUCT(Blueprintable)
struct FDistrictElement
{
	GENERATED_BODY()
public:
	FDistrictElement();

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int ID = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool IsPresent = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool IsBreach = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UStaticMesh* Mesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ABreach* Breach;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector Scale;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float ZOffset;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector BuildingOffset = FVector(0, 0, 100);
};

USTRUCT(Blueprintable)
struct FCityElements
{
	GENERATED_BODY()
public:
	FCityElements();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FDistrictElement> InnerCity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FDistrictElement> OuterCity;
};

/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UMainBuildingDistrictsDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	 FDistrictInfo InnerCity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FDistrictInfo OuterCity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "City v3")
	TArray<FDistrictElementSpawnInfo> DistrictElementSpawnInfos;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "City v3")
	bool AllowIsolatedBuildings = false;
};
