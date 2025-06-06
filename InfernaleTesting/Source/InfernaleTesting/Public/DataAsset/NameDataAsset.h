// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NameDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UNameDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Name Data Asset")
	TArray<FString> Names;

public:
	UFUNCTION(BlueprintCallable, Category = "Name Data Asset")
	FString GetRandomName();
	
};
