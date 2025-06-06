// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Structs/SimpleStructs.h"
#include "Enums/Enums.h"
#include "SoundsDataAsset.generated.h"

class UPlayerSettingSave;
class AInfernalePawn;





/**
 * 
 */
UCLASS()
class INFERNALETESTING_API USoundsDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	TArray<FSoundStruct> Sounds;

	UFUNCTION(BlueprintCallable, Category = "SoundsDataAsset")
	FSoundType GetSoundType(FString SoundTypeName, FString SoundName);
	UFUNCTION(BlueprintCallable, Category = "SoundsDataAsset")
	float GetSoundVolumeFromSound(USoundBase* Sound);
	UFUNCTION(BlueprintCallable, Category = "SoundsDataAsset")
	float GetSoundVolumeFromSoundInType(USoundBase* Sound, FString SoundTypeName);

	UFUNCTION(BlueprintCallable, Category = "SoundsDataAsset", meta=(WorldContext="WorldContextObject", AdvancedDisplay = "3", UnsafeDuringActorConstruction = "true", Keywords = "play"))
	void PlaySound(const UObject* WorldContextObject, FString SoundTypeName, FString SoundName, FVector Location);
	UFUNCTION(BlueprintCallable, Category = "SoundsDataAsset", meta=(WorldContext="WorldContextObject", AdvancedDisplay = "3", UnsafeDuringActorConstruction = "true", Keywords = "play"))
	void PlaySoundWithChangedVolumePitch(const UObject* WorldContextObject, FString SoundTypeName, FString SoundName, FVector Location, FRotator Rotation, float Volume, float Pitch);
	
};
