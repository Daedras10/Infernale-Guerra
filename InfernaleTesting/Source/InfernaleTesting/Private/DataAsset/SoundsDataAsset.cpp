// Fill out your copyright notice in the Description page of Project Settings.


#include "DataAsset/SoundsDataAsset.h"

#include "GameMode/GameInstanceInfernale.h"
#include "Save/PlayerSettingSave.h"


FSoundType USoundsDataAsset::GetSoundType(FString SoundTypeName, FString SoundName)
{
    for (const auto& Sound : Sounds)
    {
    	if (Sound.TypeName == SoundTypeName)
    	{
    		for (const auto& SoundType : Sound.Sounds)
    		{
    			if (SoundType.SoundName == SoundName)
    			{
    				return SoundType;
    			}
    		}
    	}
    }
    return FSoundType();
}

float USoundsDataAsset::GetSoundVolumeFromSound(USoundBase* Sound)
{
	for (const auto& SoundType : Sounds)
	{
		for (const auto& SoundTypeItem : SoundType.Sounds)
		{
			if (SoundTypeItem.Sound == Sound)
			{
				return SoundTypeItem.Volume;
			}
		}
	}
	return 1.f;
}

float USoundsDataAsset::GetSoundVolumeFromSoundInType(USoundBase* Sound, FString SoundTypeName)
{
	for (const auto& SoundType : Sounds)
	{
		if (SoundType.TypeName == SoundTypeName)
		{
			for (const auto& SoundTypeItem : SoundType.Sounds)
			{
				if (SoundTypeItem.Sound == Sound)
				{
					return SoundTypeItem.Volume;
				}
			}
		}
	}
	return 1.f;
}

void USoundsDataAsset::PlaySound(const UObject* WorldContextObject, FString SoundTypeName, FString SoundName, FVector Location)
{
	Cast<UGameInstanceInfernale>(WorldContextObject->GetWorld()->GetGameInstance())->PlaySound(WorldContextObject, SoundTypeName, SoundName, Location);
}

void USoundsDataAsset::PlaySoundWithChangedVolumePitch(const UObject* WorldContextObject, FString SoundTypeName, FString SoundName, FVector Location, FRotator Rotation,float Volume, float Pitch)
{
	Cast<UGameInstanceInfernale>(WorldContextObject->GetWorld()->GetGameInstance())->PlaySoundWithChangedVolumePitch(WorldContextObject, SoundTypeName, SoundName, Location, Rotation, Volume, Pitch);
}
