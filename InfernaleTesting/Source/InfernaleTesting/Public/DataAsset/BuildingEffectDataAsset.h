// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BuildingEffectDataAsset.generated.h"

UENUM(Blueprintable)
enum class EBuildingEffectType : uint8
{
	BuildingEffectTypeNone,
	BuildingEffectTypeHealth,
	BuildingEffectTypeRange,
	BuildingEffectTypeSummonBoss,
	BuildingEffectTypeAttack,
	BuildingEffectTypeFlux,
};

USTRUCT(BlueprintType)
struct FBuildingEffect
{
	GENERATED_BODY()
public:
	FBuildingEffect();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EBuildingEffectType Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Value;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ValueOverclocked;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsPercent;
};


/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UBuildingEffectDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FBuildingEffect> BuildingEffects;
};
