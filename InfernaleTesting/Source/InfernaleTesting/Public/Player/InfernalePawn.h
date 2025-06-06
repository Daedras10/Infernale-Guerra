// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "GameFramework/Pawn.h"
#include "InfernalePawn.generated.h"

class UGameInstanceInfernale;
class UInfernalePawnDataAsset;
class APlayerControllerInfernale;
class UCameraComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPawnMoved, const FVector, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPawnSetAtSpawn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPawnSpawnTravelFinished);


UCLASS()
class INFERNALETESTING_API AInfernalePawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AInfernalePawn();

	UFUNCTION(BlueprintCallable) void SetDoCameraEndGameTraving();
	void SubscribeToRotateEvents();
	void UnsubscribeFromRotateEvents();
	void MoveToSpawn(FVector Location);
	void SetCenterPoint(FVector NewCenterPoint);
	void ResetRotation();

	void GetLookAtLocation(FHitResult& OutHit, bool Debug = false) const;
	UFUNCTION(BlueprintCallable) FVector GetLookAtLocationPoint() const;
	void AddOverrideCameraProfileIndexPosition(int32 Index, FVector Location, FRotator Rotation);
	
	UFUNCTION() FVector GetPawnStartLocation();
	UFUNCTION(Client, Reliable) void SetDoCameraStartGameTravelingOwning(bool Value);
	UFUNCTION(Client, Reliable) void SetCanMoveOwning(bool Value);
	void KeyboardRotationActionTriggered(const FInputActionValue& InputActionValue);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(CallInEditor) void SyncFromDataAsset();

	UFUNCTION() void MoveTriggered(const FVector2D Value);
	UFUNCTION() void MoveCompleted(const FVector2D _);
	UFUNCTION() void CameraHeightTriggered(const float Value);
	UFUNCTION() void RotationStarted();
	UFUNCTION() void RotationCompleted();
	UFUNCTION() void LookTriggered(const FVector2D Value);
	UFUNCTION() void AddOrOverrideCameraPositionAtIndex(int32 Index);
	UFUNCTION() void SetCameraPositionAtIndex(int32 Index);
	UFUNCTION()	void MoveStarted();
	UFUNCTION()	void SecondaryMoveCompleted();

	UFUNCTION(BlueprintCallable) void SubscribeToLookEvents();
	UFUNCTION(BlueprintCallable) void UnsubscribeFromLookEvents();


	UFUNCTION(NetMulticast, Reliable) void MoveToLocationMulticast(FVector Location);
	UFUNCTION(NetMulticast, Reliable) void AddOverrideCameraProfileIndexPositionMulticast(int32 Index, FVector Location, FRotator Rotation);

	UFUNCTION() void CameraProfileTriggered(FVector2D CameraProfileVector);
	UFUNCTION() void CameraProfileCompleted(FVector2D CameraProfileVector);
	void SubscribeToInputEvents();
	void UnsubscribeFromInputEvents();
	
	void TryInitializePlayerControllerInfernale();
	void TryInitializePlayerControllerInfernaleSimple();
	void TryInitializePlayerControllerInfernaleDelayed();
	void InitializePlayerControllerInfernale();
	bool CheckBoundaries(FVector Location, UE::Math::TVector<double>& NextLocation) const;
	UFUNCTION()	void OnPawnSetAtSpawn();
	UFUNCTION(BlueprintImplementableEvent) void OnPawnSetAtSpawnBP();
	UFUNCTION () void OnPawnSpawnTravelFinished();
	UFUNCTION(BlueprintImplementableEvent) void OnPawnSpawnTravelFinishedBP();

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UCameraComponent* GetCameraComponent() const;

public:
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FPawnMoved PawnMoved;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Save Camera")
	float MoveSpeedMultiplier = 1.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Save Camera")
	float ZoomSpeedMultiplier = 1.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Save Camera")
	float RotationSpeedMultiplier = 1.0f;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FPawnSetAtSpawn PawnSetAtSpawn;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FPawnSpawnTravelFinished PawnSpawnTravelFinished;

	

protected:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTS Camera")
	bool CanMove = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTS Camera")
	bool bDoCameraStartGameTraveling;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTS Camera")
	bool bDoCameraEndGameTraveling;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTS Camera")
	bool PawnSetAtLocation = false;
	
	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	UInfernalePawnDataAsset* InfernalePawnDataAsset;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	bool bUseDataAsset = true;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTS Camera")
	float CameraHeightSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	float CameraDefaultHeight;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	FVector2D CameraDefaultAngle;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	FVector2D CameraHeightLimits;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	FVector2D CameraXYLimits;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	FVector2D CameraAnglePitchLimits;
	
	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	FVector2D ScreenEdgePadding;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	float MoveSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	float RotationSpeed;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTS Camera components")
	UCameraComponent* CameraComponent;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	float LerpSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	UCurveFloat* CameraPitchCurve;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	UCurveFloat* CameraHeightMoveSpeedCurve;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	UCurveFloat* CameraCurveSpeed;

	float DistanceFromStartPosToMainBase;
	float DistanceFromCurrentPosToStart;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	bool RotateOnSelf;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	float RotationCenterOffset;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera")
	float ZoomFocusDelay;

	UPROPERTY(EditDefaultsOnly, Category = "Debug Settings")
	bool bDebug = false;

	UPROPERTY(EditDefaultsOnly, Category = "Debug Settings")
	bool bDebugZoomLevel = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	APlayerControllerInfernale* PlayerControllerInfernale;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bisMoving = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UGameInstanceInfernale* GameInstanceInfernale = nullptr;
	
	FVector2D CurrentInputMoveSpeed;
	bool ShouldRotate;
	bool ShouldMove;
	bool PlayerControllerWasInitialized = false;
	float PitchTarget;
	FVector3d CameraCenterOfInterest;
	const float FixedDeltaTime = 0.0167f; // Approx. 60 FPS frame time
	const int32 MaxSubsteps = 5;
	float AccumulatedDeltaTime = 0.0f;
	FVector3d PreviousTouchPosition;
	FVector3d NewTouchPosition;
	float CurrentZoomFocusDelay;
	FHitResult ZoomHitResult;

	FVector ZoomPositionTarget = FVector::ZeroVector;
	FVector CameraCenterPoint;
	
	TArray<FVector> CameraSavedPositions;
	TArray<FRotator> CameraSavedRotations;

	UInputComponent* PlayerInputComponent;

	FVector2D LastMousePosition;
	ECollisionChannel CameraCollsionChannel;

	float ShouldMoveFalseTimer = 1.0f;
	float RotationAE;
};
