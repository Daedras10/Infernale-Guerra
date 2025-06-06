// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LD/Buildings/MainBuilding.h"
#include "PandemoniumMainBuilding.generated.h"

/**
 * 
 */
UCLASS()
class INFERNALETESTING_API APandemoniumMainBuilding : public AMainBuilding
{
	GENERATED_BODY()

public:

	float GetVictoryPointsTimer() const;
	float GetVictoryPointsPerTime() const;

	
	float GetVictoryPointsTimer(const float Value) const;
	float GetVictoryPointsPerTime(const float Value) const;
	
	UFUNCTION(BlueprintImplementableEvent)
	void TriggerAddRemoveVictoryPointsVFX();

protected:
	virtual void OwnershipChanged() override;
	virtual void GameModeInfernaleInitialized() override;


protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	float PandemoniumMBTimer = 1.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	float PandemoniumMBPointsPerTime = 1.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings") UCurveFloat* DelayBetweenRewards = nullptr;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings") UCurveFloat* PointsAtDelaysRewards = nullptr;
};
