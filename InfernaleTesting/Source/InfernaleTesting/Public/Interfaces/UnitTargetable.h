// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Enums/Enums.h"
#include "UnitTargetable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UUnitTargetable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class INFERNALETESTING_API IUnitTargetable
{
	GENERATED_BODY()

public:
	virtual float GetTargetableRange();

	UFUNCTION(BlueprintNativeEvent) void ToggleDetectionDecal(bool Hide);
	//UFUNCTION(BlueprintNativeEvent, NetMulticast, Reliable) void SetDetectionDecalColorMulticast(FColor Color);
	UFUNCTION(BlueprintNativeEvent) void SetDetectionDecalColor(FColor Color);

	UFUNCTION(BlueprintNativeEvent) void TriggerDetectionVFX(EFluxPowerScaling Scaling, int Power);
	UFUNCTION(BlueprintNativeEvent) void DisableDetectionVFX();

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	EUnitTargetType Type = EUnitTargetType::UTargetNone;
	bool bIsDetected = false;
};
