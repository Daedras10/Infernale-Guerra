// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GRInfernalePawn.generated.h"

class AGRPlayerControllerInfernale;
class UCameraComponent;

UCLASS()
class INFERNALETESTING_API AGRInfernalePawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AGRInfernalePawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION()
	void MoveTriggered(const FVector2D Value);

	UFUNCTION()
	void MoveCompleted(const FVector2D _);

	UFUNCTION()
	void CameraHeightTriggered(const float Value);

	UFUNCTION()
	void RotationStarted();

	UFUNCTION()
	void RotationCompleted();

	UFUNCTION()
	void LookTriggered(const FVector2D Value);

	UFUNCTION()
	void AddOrOverrideCameraPositionAtIndex(int32 Index);

	UFUNCTION()
	void SetCameraPositionAtIndex(int32 Index);
	
	UFUNCTION()
	void MoveStarted();

	UFUNCTION()
	void SecondaryMoveCompleted();


	void SubscribeToInputEvents();
	void UnsubscribeFromInputEvents();

	UFUNCTION(BlueprintCallable)
	void SubscribeToLookEvents();

	UFUNCTION(BlueprintCallable)
	void UnsubscribeFromLookEvents();
	
	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	float CameraHeightSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	float CameraDefaultHeight;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	FVector2D CameraDefaultAngle;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	FVector2D CameraHeightLimits;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	FVector2D CameraAnglePitchLimits;
	
	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	FVector2D ScreenEdgePadding;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	FVector2D MoveSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	float RotationSpeed;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTS Camera")
	TObjectPtr<UCameraComponent> CameraComponent;
	
	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	TObjectPtr<APlayerController> PlayerController;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	float LerpSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	UCurveFloat* CameraPitchCurve;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	bool RotateOnSelf;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	float RotationCenterOffset;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	FVector4d ClampedCameraPositionXY;


	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	AGRPlayerControllerInfernale* PlayerControllerInfernale;
	
	FVector2D CurrentInputMoveSpeed;
	bool ShouldRotate;
	bool ShouldMove;
	float HeightTarget;
	float PitchTarget;
	FVector3d CameraCenterOfInterest;

	struct FCameraPosition
	{
		FVector Position;
		FRotator Rotation;
	};
	
	TArray<FCameraPosition> CameraSavedPositions;
};
