// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Structs/SimpleStructs.h"
#include "PlayerSettingSave.generated.h"

/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UPlayerSettingSave : public USaveGame
{
	GENERATED_BODY()

public:

	//Game
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float CameraMoveSpeedMultiplier = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float CameraZoomSpeedMultiplier = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float CameraRotationSpeedMultiplier = 1.0f;

	//Sound
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MasterVolume = .5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MusicVolume = .5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float EffectVolume = .5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float AmbianceVolume = .5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float UIVolume = .5f;
	
};
