// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameMode/Infernale/PlayerControllerInfernale.h"
#include "UObject/Interface.h"
#include "Ownable.generated.h"

class APlayerControllerInfernale;


// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UOwnable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class INFERNALETESTING_API IOwnable
{
	GENERATED_BODY()

public:
	virtual FOwner GetOwner();
	virtual void SetOwner(FOwner NewOwner);
	virtual void ChangeOwner(FOwner NewOwner);
};
