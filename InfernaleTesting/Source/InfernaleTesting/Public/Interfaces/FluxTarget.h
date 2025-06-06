// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "FluxTarget.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UFluxTarget : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class INFERNALETESTING_API IFluxTarget
{
	GENERATED_BODY()

public:
	//UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	virtual float GetOffsetRange();
	virtual float GetAttackOffsetRange();
};
