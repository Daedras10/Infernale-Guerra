// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PlayerControllerEndLevel.generated.h"

/**
 * 
 */
UCLASS()
class INFERNALETESTING_API APlayerControllerEndLevel : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	
};
