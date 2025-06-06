// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LD/LDElement/LDElement.h"
#include "Outpost.generated.h"

class UOutpostDataAsset;
class UEffectAfterDelayComponent;
/**
 * 
 */
UCLASS()
class INFERNALETESTING_API AOutpost : public ALDElement
{
	GENERATED_BODY()

public:
	AOutpost();

protected:
	virtual void BeginPlay() override;

	void SyncDataAsset();

	UFUNCTION(BlueprintImplementableEvent) void VisualSetOutpostChargedBP(bool bIsChargedValue);
	UFUNCTION(BlueprintImplementableEvent) void VisualSetOutpostChargingBP(float RemainingTime, float TotalTime);
	UFUNCTION(BlueprintImplementableEvent) void VisualSetOutpostDechargingBP(float RemainingTime, float TotalTime);
	


protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UEffectAfterDelayComponent* EffectAfterDelayComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data Asset")
	UOutpostDataAsset* OutpostDataAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data Asset")
	bool bUseDataAsset;

	bool bIsCharged;
	bool bIsDecharging;
	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	float ChargeTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	float DechargeTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	float Radius = 10000;

	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	// TArray<FInfernaleUnitBuff> OutpostBuffs;
};
