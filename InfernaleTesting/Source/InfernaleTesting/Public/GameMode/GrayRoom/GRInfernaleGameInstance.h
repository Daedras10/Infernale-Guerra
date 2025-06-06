// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AdvancedFriendsGameInstance.h"
#include "Engine/GameInstance.h"
#include "GRInfernaleGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UGRInfernaleGameInstance : public UAdvancedFriendsGameInstance //UAdvancedFriendsGameInstance
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void CreateSession();
	
};
