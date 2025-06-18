// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AISystem.h"
#include "MyAISystem.generated.h"

/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UMyAISystem : public UAISystem
{
	GENERATED_BODY()

public:
	void InitializeActorsForPlay(bool bTimeGotReset) override;
	
};
