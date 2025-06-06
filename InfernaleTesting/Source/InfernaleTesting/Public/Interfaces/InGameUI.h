// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InGameUI.generated.h"

UENUM(Blueprintable)
enum class EDisplayWidgetType : uint8
{
	DisplayWidgetNone,
	DisplayWidgetBreach,
	DisplayWidgetMainBuilding,
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UInGameUI : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class INFERNALETESTING_API IInGameUI
{
	GENERATED_BODY()

public:
	UFUNCTION()
	virtual void DisplayUI(bool Display);

	UFUNCTION(BlueprintImplementableEvent)
	void FacePosition(FVector Position);

protected:
	UFUNCTION(BlueprintImplementableEvent)
	void DisplayWidget(bool Display, EDisplayWidgetType DisplayWidgetType);

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
};
