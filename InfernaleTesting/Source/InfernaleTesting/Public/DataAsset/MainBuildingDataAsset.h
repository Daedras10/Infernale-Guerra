// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FogOfWar/FogOfWarManager.h"
#include "MainBuildingDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UMainBuildingDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float VictoryPointValue = 10;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float ControlAreaRadius = 1000;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float TooCloseRadius = 400.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float MaxHealth = 100;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float MaxHealthPlayer = -1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere) float NeutralThornDamage = 0.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float PlayerThornDamage = 50.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float OverclockCD = 120.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float OverclockCost = 300.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere) float Healing = 150.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float HealingDelay = 1.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float HealingDelaySinceLastAttack = 10.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float OffsetRange = 2700.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float AttackOffsetRange = 2700.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float RepulsorRange = 400.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float SightRange = 20.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float TargetableRange = 500.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) VisionType VisionTypeShape = VisionType::Circle;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UCurveFloat* BuildingPriceCurve = nullptr;
};
