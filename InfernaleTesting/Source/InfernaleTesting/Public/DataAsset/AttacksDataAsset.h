// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AttacksDataAsset.generated.h"

struct FAttackStruct;
/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UAttacksDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FAttackStruct> Attacks;
};
