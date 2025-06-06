// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FluxSettingsDataAsset.generated.h"

class AFluxNode;
class AFlux;
/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UFluxSettingsDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float RemoveFluxDuration = 5.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float RemoveFluxNodeDuration = 1.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float FluxCreationHoldTime = 0.15f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MinRangeBetweenNodes = 100.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MinRangeBetweenNodesOnCreation = 110.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bRemoveMustHoverOnFlux = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bMultiSelectionEnabled = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<AFlux> FluxSpawnClass;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<AFluxNode> FluxNodeSpawnClass;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UCurveFloat* SpeedRatioCurve;
};
