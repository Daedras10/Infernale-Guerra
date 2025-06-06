// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InfernalePawnDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UInfernalePawnDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTS Camera")
	float CameraHeightSpeed = 5;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTS Camera")
	float CameraDefaultHeight = 2000;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTS Camera")
	FVector2D CameraDefaultAngle = FVector2D(0, -36.5);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTS Camera")
	FVector2D CameraHeightLimits = FVector2D(10000, 80000);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTS Camera")
	FVector2D CameraXYLimits = FVector2D(100000, 100000);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTS Camera")
	FVector2D CameraAnglePitchLimits = FVector2D(-89, 20);
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTS Camera")
	FVector2D ScreenEdgePadding = FVector2D(0, 0);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTS Camera")
	FVector GameStartPosition = FVector(0, 0, 0);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTS Camera")
	float MoveSpeed = 150000;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTS Camera")
	float RotationSpeed = 200;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTS Camera")
	float LerpSpeed = 50;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTS Camera")
	UCurveFloat* CameraPitchCurve;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTS Camera")
	UCurveFloat* CameraHeightMoveSpeedCurve;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTS Camera")
	UCurveFloat* CameraTravelingSpeed;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTS Camera")
	bool RotateOnSelf;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTS Camera")
	float RotationCenterOffset = 2000;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTS Camera")
	float ZoomFocusDelay = 1;
};
