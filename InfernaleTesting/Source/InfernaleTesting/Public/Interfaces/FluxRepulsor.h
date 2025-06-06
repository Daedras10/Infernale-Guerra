// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "FluxRepulsor.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UFluxRepulsor : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class INFERNALETESTING_API IFluxRepulsor
{
	GENERATED_BODY()

public:
	virtual float GetRepulsorRange() const = 0;

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
};
