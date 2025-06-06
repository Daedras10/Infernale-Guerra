// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "FluxStart.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UFluxStart : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class INFERNALETESTING_API IFluxStart
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnSelectedForFlux();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnDeselectedForFlux();
	
	virtual void OnSelectedForFluxCpp();
	virtual void OnDeselectedForFluxCpp();

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
};
