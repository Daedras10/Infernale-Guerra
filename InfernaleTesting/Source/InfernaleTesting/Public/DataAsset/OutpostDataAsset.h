// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "OutpostDataAsset.generated.h"


/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UOutpostDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ChargeTime = 90;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DechargeTime = 15;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Radius = 10000;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float TargetableRange = 500.f;

	// UPROPERTY(EditAnywhere, BlueprintReadWrite)
	// TArray<FInfernaleUnitBuff> OutpostBuffs;
};
