// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "EnhancedInputSubsystems.h"
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include <Buildings/GRBase.h>

#include "GameMode/Infernale/PlayerControllerInfernale.h"
#include "GRPlayerControllerInfernale.generated.h"


class AGRInfernalePawn;
class UGRGameInputDataAsset;


UENUM(BlueprintType)
enum class EBuildModeGR : uint8
{
	DebugCreateSpot,
	EditBuildSpot,
	CreateBase,
	EditBase,
	None,
};

UENUM(BlueprintType)
enum class EFluxModeGR : uint8
{
	Build,
	Edit,
	Remove,
	None,
};


/**
 * 
 */
UCLASS()
class INFERNALETESTING_API AGRPlayerControllerInfernale : public APlayerController
{
	GENERATED_BODY()

public:

	// Input Delegates
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FMousePrimaryStart MousePrimaryStart;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FMousePrimaryEnd MousePrimaryEnd;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FMousePrimaryEnd MousePrimaryTriggered;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FMouseSecondaryStart MouseSecondaryStart;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FMouseSecondaryEnd MouseSecondaryEnd;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FMouseSecondaryEnd MouseSecondaryTriggered;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FMouseFluxModeStart FluxModeStart;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FMouseMoved MouseMoved;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FMoveTriggered MoveTriggered;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FMoveCompleted MoveCompleted;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FEnableMoveActionTriggered EnableMoveActionTriggered;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FEnableMoveActionCompleted EnableMoveActionCompleted;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FEnableMoveActionCanceled EnableMoveActionCanceled;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FLookTriggered LookTriggered;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FCameraHeightTriggered ScrollTriggered;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FEnableRotationStarted EnableRotationStarted;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FEnableRotationCompleted EnableRotationCompleted;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FMouseScrollUp MouseScrollUp;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FMouseScrollDown MouseScrollDown;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FMouseScrollStarted MouseScrollStarted;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FMouseScrollEnded MouseScrollEnded;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FEscapeStarted EscapeStarted;
	


	// Event Delegates
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FPlayerReadyDelegate PlayerReady;

	void SetPlayerInputComponent(UInputComponent* PlayerInputComponent);

	UFUNCTION(BlueprintImplementableEvent)
	AGRBase* GetBase();
	
protected:
	virtual void BeginPlay() override;

	void SetupInputMappings();
	void EnableMoveActionTriggeredFunc();
	void EnableMoveActionCompletedFunc();
	void BindInputs();
	
	
	// Input Actions
	void MoveActionTriggered(const FInputActionValue& Value);
	void MoveActionCompleted(const FInputActionValue& Value);
	void LookActionTriggered(const FInputActionValue& Value);
	void CameraHeightActionTriggered(const FInputActionValue& Value);
	void EnableRotationActionStarted(const FInputActionValue& Value);
	void EnableRotationActionCompleted(const FInputActionValue& Value);
	void PrimaryActionStarted();
	void PrimaryActionEnded();
	void PrimaryActionTriggered();
	void SecondaryActionStarted();
	void SecondaryActionEnded();
	void SecondaryActionTriggered();
	void FluxModeActionStarted();
	void MouseScroll(const FInputActionValue& Value);
	void EscapeActionStarted();

	void StartScroll();
	void StopScroll();
	void ResetScrollTimer();
	
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* GameInputMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UGRGameInputDataAsset* GameInputDataAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	bool bPrintDebugInputMouse = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TimeBeforeScrollEnd;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	AGRInfernalePawn* InfernalePawn;
	
	UInputComponent* PlayerInputComponent;

	bool IsScrolling = false;
	FTimerHandle ScrollTimerHandle;
};
