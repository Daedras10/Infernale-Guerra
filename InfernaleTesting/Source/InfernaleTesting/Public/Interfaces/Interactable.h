// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"


class APlayerControllerInfernale;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStopInteract);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPCBoolDelegate, APlayerControllerInfernale*, PlayerControllerInfernale, bool, bVal);

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class INFERNALETESTING_API IInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION()
	virtual void InteractStartMain(APlayerControllerInfernale* Interactor);
	
	UFUNCTION()
	virtual void InteractEndMain(APlayerControllerInfernale* Interactor);

	
	UFUNCTION()
	virtual void InteractStartHover(APlayerControllerInfernale* Interactor);

	UFUNCTION()
	virtual void InteractEndHover(APlayerControllerInfernale* Interactor);

	UFUNCTION()
	virtual void InteractStartHoverFlux(APlayerControllerInfernale* Interactor);

	UFUNCTION()
	virtual void InteractEndHoverFlux(APlayerControllerInfernale* Interactor);

	
	UFUNCTION()
	virtual void InteractStartSecondary(APlayerControllerInfernale* Interactor);
	
	UFUNCTION()
	virtual void InteractEndSecondary(APlayerControllerInfernale* Interactor);

	UFUNCTION()
	virtual bool InteractableHasUIOpen();
	
	UFUNCTION()
	virtual bool ShouldEndMainInteractOnMove();

public:
	FStopInteract StopInteract;

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
};
