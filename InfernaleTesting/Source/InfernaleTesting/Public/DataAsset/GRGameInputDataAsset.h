// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "Engine/DataAsset.h"
#include "GRGameInputDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UGRGameInputDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UInputAction* MoveAction;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UInputAction* EnableMoveAction;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UInputAction* LookAction;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UInputAction* CameraHeightAction;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UInputAction* EnableRotationAction;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UInputAction* PrimaryAction;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UInputAction* SecondaryAction;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UInputAction* FluxModeAction;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UInputAction* EscapeAction;
};
