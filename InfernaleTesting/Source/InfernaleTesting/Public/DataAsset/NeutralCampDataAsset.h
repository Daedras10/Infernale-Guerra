// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NeutralCampDataAsset.generated.h"

UENUM(Blueprintable)
enum class ENeutralCampReward : uint8
{
	ENeutralMobRewardNone,
	ENeutralMobRewardSouls,
	ENeutralMobRewardVision,
	ENeutralMobRewardDomination
};

USTRUCT(BlueprintType)
struct FNeutralCampRewardData
{
	GENERATED_USTRUCT_BODY()
public:
	FNeutralCampRewardData();
	
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ENeutralCampReward RewardType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float RewardValue;
};
/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UNeutralCampDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	TArray<FNeutralCampRewardData> RewardData;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	float MaxHealth = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	float RespawnTime = 60.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	float AttackOffsetRange = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	bool bUseAOEAttack = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	float TargetableRange = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BossOnly")
	bool bUseBossCapture = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BossOnly")
	float BossCaptureRadius = 6.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BossOnly")
	float BossCapturePointPerTick = 0.15f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BossOnly")
	float BossCaptureTickTime = 0.25f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BossOnly")
	float BossSummonPointsPerSacrifice = 5.f;
};
