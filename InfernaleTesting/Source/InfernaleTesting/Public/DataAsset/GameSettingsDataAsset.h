// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Structs/SimpleStructs.h"
#include "GameSettingsDataAsset.generated.h"


class UTransmutationDataAsset;
class USoundsDataAsset;
class UEconomyDataAsset;
class UNameDataAsset;
class AFogOfWarManager;
class UFluxSettingsDataAsset;
class UMassEntityConfigAsset;
class UBuildingListDataAsset;

USTRUCT(Blueprintable)
struct FMapInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString MapName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString MapDescription;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSoftObjectPath MapReference;
};

USTRUCT(Blueprintable)
struct FMeshById
{
	GENERATED_BODY()
public:
	FMeshById();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString MeshId;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UStaticMesh* Mesh;
};

USTRUCT(Blueprintable)
struct FDataAssetsSettings
{
	GENERATED_BODY()
public:
	FDataAssetsSettings();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UFluxSettingsDataAsset* FluxSettings;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) USoundsDataAsset* SoundsAssets;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UEconomyDataAsset* EconomyAssets;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UTransmutationDataAsset* TransmutationAssets;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) TArray<UMassEntityConfigAsset*> AmalgamAssets;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UBuildingListDataAsset* BuildingListAssets;
};

/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UGameSettingsDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float RayCastLength = 10000000.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) TArray<FMapInfo> Maps;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) int DefaultMapIndex = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings") bool ShowSplines = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings") bool UseFluxMode = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Fog of War") bool UseFogOfWar = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Fog of War") TSubclassOf<AFogOfWarManager> FogOfWarManagerClass;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Draft") bool IgnoreForceUserName = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Victory") bool EndAfterTimeDepleted = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings") bool UseMass = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings") bool bIsJuryMode = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings") bool ArthurDesoleMaisNique = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings") bool bUsePreGameSandbox = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings") bool bUseHoldSpaceMode = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings") bool RandomBreachOnStart = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings") bool PandemoniumMBGivesVictoryPoints = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings") bool SelfTesting = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings") bool DefaultUseLAN = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings") bool UseMassLocal = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere) TArray<FMeshById> MeshesById;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings") float MaxUnitsPerSecondsPerBase = 1.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data Assets") TArray<FDataAssetsSettings> DataAssetsSettings = TArray<FDataAssetsSettings>();
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data Assets") int DataAssetsSettingsToUse = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Random Names") UNameDataAsset* NameDataAsset;

	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Debugs") FDebugSettings DebugSettings = FDebugSettings();

	// UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Victory")
	// bool EndAfterPointsReached = false;
	//
	// UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Victory")
	// float PointsForVictory = 70;
};
