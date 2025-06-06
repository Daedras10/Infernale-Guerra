// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EconomyDataAsset.generated.h"


struct FSoulsGainCostValues;
/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UEconomyDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) TArray<FSoulsGainCostValues> SoulsGainedValues = TArray<FSoulsGainCostValues>();

	UPROPERTY(BlueprintReadWrite, EditAnywhere) float InitialSouls = 300;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float InitialSoulsSandbox = 1000;

	UPROPERTY(BlueprintReadWrite, EditAnywhere) float NegativeSoulsAllowedSandbox = -1000;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float EconomyGainGlobalDelay = 20.f;
	
};
