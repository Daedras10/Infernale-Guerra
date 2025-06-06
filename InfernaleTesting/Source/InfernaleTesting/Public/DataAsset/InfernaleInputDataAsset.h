// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InfernaleInputDataAsset.generated.h"

class UInputAction;
/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UInfernaleInputDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UInputAction* MoveAction;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UInputAction* EnableMoveAction;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UInputAction* LookAction;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UInputAction* CameraHeightAction;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UInputAction* EnableRotationAction;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UInputAction* PrimaryAction;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UInputAction* SecondaryAction;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UInputAction* FluxModeAction;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UInputAction* EscapeAction;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UInputAction* ControlAction;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UInputAction* ShiftAction;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UInputAction* PKeyAction;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UInputAction* CameraProfilesAction;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UInputAction* TransmutationAction;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UInputAction* DotDotAction;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UInputAction* IncreaseAction;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UInputAction* DecreaseAction;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UInputAction* SpaceAction;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UInputAction* EnableLANAction;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UInputAction* KeyboardRotationAction;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UInputAction* ResetRotationAction;
};
