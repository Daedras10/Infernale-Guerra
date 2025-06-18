// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/GameInstanceInfernale.h"

#include "MoviePlayer.h"
#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "DataAsset/GameSettingsDataAsset.h"
#include "DataAsset/SoundsDataAsset.h"
#include "FunctionLibraries/FunctionLibraryInfernale.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Player/InfernalePawn.h"
#include "Save/PlayerSettingSave.h"

void UGameInstanceInfernale::CreateNewSessionWithName_Implementation(FString& SessionName)
{
}

void UGameInstanceInfernale::Init()
{
	Super::Init();
	bUseLAN = GameSettingsDataAsset->DefaultUseLAN;

	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UGameInstanceInfernale::BeginLoadingScreen);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UGameInstanceInfernale::EndLoadingScreen);
}

void UGameInstanceInfernale::BeginLoadingScreen(const FString& MapName)
{
	LoadingScreenWidget = CreateWidget(this, LoadingScreenWidgetClass);

	if (IsRunningDedicatedServer()) return;

	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("UGameInstanceInfernale::BeginLoadingScreen"));
	FLoadingScreenAttributes LoadingScreen;
	LoadingScreen.bAutoCompleteWhenLoadingCompletes = false;
	//LoadingScreen.bWaitForManualStop = true;
	//LoadingScreen.bAllowEngineTick = true;
	LoadingScreen.WidgetLoadingScreen = LoadingScreenWidget->TakeWidget();
	//LoadingScreen.WidgetLoadingScreen = FLoadingScreenAttributes::NewTestLoadingScreenWidget();

	GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
}

void UGameInstanceInfernale::EndLoadingScreen(UWorld* World)
{
	// FTimerHandle TimerHandle;
	// GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UGameInstanceInfernale::EndLoadingScreenBP, 3.f, false);
	// LoadingScreenWidget->RemoveFromParent();
	// LoadingScreenWidget = nullptr;
	EndLoadingScreenBP();
}

bool UGameInstanceInfernale::UseLAN()
{
	return bUseLAN;
}

void UGameInstanceInfernale::SetUseLAN(bool bNewValue)
{
	bUseLAN = bNewValue;
}

void UGameInstanceInfernale::SetVictoryInfo(const FVictoryInfoMap& Infos)
{
	EndGameVictoryInfo = Infos;
}

void UGameInstanceInfernale::ServerTravelToScene(FString SceneName)
{
	GetWorld()->ServerTravel(SceneName);
}

int UGameInstanceInfernale::GetPlayers()
{
	return Players;
}

void UGameInstanceInfernale::SetPlayers(const int NewPlayers)
{
	Players = NewPlayers;
}

int UGameInstanceInfernale::GetGameDuration()
{
	return GameDuration;
}

void UGameInstanceInfernale::SetGameDuration(const float NewDuration)
{
	GameDuration = NewDuration;
}

int UGameInstanceInfernale::GetCustomGameMode()
{
	return CustomGameMode;
}

void UGameInstanceInfernale::SetCustomGameMode(int NewCustomGameMode)
{
	CustomGameMode = NewCustomGameMode;
}

FString UGameInstanceInfernale::GetLastTravelScene()
{
	return LastTravelScene;
}


void UGameInstanceInfernale::SetLastTravelScene(FString SceneName)
{
	LastTravelScene = SceneName;
}

int UGameInstanceInfernale::GetCustomTeamMode()
{
	return CustomTeamMode;
}

void UGameInstanceInfernale::SetCustomTeamMode(int NewCustomTeamMode)
{
	CustomTeamMode = NewCustomTeamMode;
}

UGameSettingsDataAsset* UGameInstanceInfernale::GetGameSettingsDataAsset()
{
	return GameSettingsDataAsset;
}

void UGameInstanceInfernale::SetVSync(bool bNewValue)
{
	UGameUserSettings* GameUserSettings = GEngine->GetGameUserSettings();
	if (GameUserSettings)
	{
		GameUserSettings->SetVSyncEnabled(bNewValue);
		GameUserSettings->ApplySettings(false);
	}
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
	                                             TEXT("VSync set to: ") + FString::Printf(
		                                             TEXT("%s"), GameUserSettings->bUseVSync
			                                                         ? TEXT("true")
			                                                         : TEXT("false")));
}

void UGameInstanceInfernale::SaveGameSettings()
{
	// if (!bInitialized) return; 
	if (!IsValid(PlayerSettingSave))
	{
		PlayerSettingSave = Cast<UPlayerSettingSave>(
			UGameplayStatics::CreateSaveGameObject(PlayerSettingSave->GetClass()));
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PlayerSettingSave not found"));
	}
	if (!UGameplayStatics::SaveGameToSlot(PlayerSettingSave, "PlayerSettings", 0))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PlayerSettings not saved"));
	}
	UGameplayStatics::SaveGameToSlot(PlayerSettingSave, "PlayerSettings", 0);
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("PlayerSettings saved successfully, Camera zoom speed: ") +
	//                                              FString::SanitizeFloat(PlayerSettingSave->CameraZoomSpeedMultiplier));
	LoadGameSettings();
}

void UGameInstanceInfernale::SetupPlayerSettings()
{
	if (UGameplayStatics::DoesSaveGameExist("PlayerSettings", 0))
	{
		PlayerSettingSave = Cast<UPlayerSettingSave>(UGameplayStatics::LoadGameFromSlot("PlayerSettings", 0));
		if (!PlayerSettingSave || !IsValid(PlayerSettingSave))
		{
			PlayerSettingSave = Cast<UPlayerSettingSave>(
				UGameplayStatics::CreateSaveGameObject(UPlayerSettingSave::StaticClass()));
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PlayerSettings not found"));
		PlayerSettingSave = Cast<UPlayerSettingSave>(
			UGameplayStatics::CreateSaveGameObject(UPlayerSettingSave::StaticClass()));
	}
}

void UGameInstanceInfernale::LoadGameSettings()
{
	PlayerSettingSave = nullptr;
	SetupPlayerSettings();

	if (InfernalePawn != nullptr)
	{
		if (!IsValid(InfernalePawn))
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("InfernalePawn not found"));
		}
		else
		{
			if (!PlayerSettingSave)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PlayerSettingSave not found"));
			}
			else
			{
				InfernalePawn->MoveSpeedMultiplier = PlayerSettingSave->CameraMoveSpeedMultiplier;
				InfernalePawn->ZoomSpeedMultiplier = PlayerSettingSave->CameraZoomSpeedMultiplier;
				InfernalePawn->RotationSpeedMultiplier = PlayerSettingSave->CameraRotationSpeedMultiplier;
			}
		}
	}

	if (PlayerSettingSave)
	{
		MasterVolume = PlayerSettingSave->MasterVolume * DefaultMultiplier;
		AmbianceVolume = PlayerSettingSave->AmbianceVolume * DefaultMultiplier;
		EffectVolume = PlayerSettingSave->EffectVolume * DefaultMultiplier;
		MusicVolume = PlayerSettingSave->MusicVolume * DefaultMultiplier;
		UIVolume = PlayerSettingSave->UIVolume * DefaultMultiplier;
		MasterXAmbianceVolume = MasterVolume * AmbianceVolume;
		MasterXEffectVolume = MasterVolume * EffectVolume;
		MasterXMusicVolume = MasterVolume * MusicVolume;
		MasterXUIVolume = MasterVolume * UIVolume;
	}
	else
	{
		// Set default values when PlayerSettingSave is null
		MasterVolume = DefaultMultiplier;
		AmbianceVolume = DefaultMultiplier;
		EffectVolume = DefaultMultiplier;
		MusicVolume = DefaultMultiplier;
		UIVolume = DefaultMultiplier;
		MasterXAmbianceVolume = MasterVolume * AmbianceVolume;
		MasterXEffectVolume = MasterVolume * EffectVolume;
		MasterXMusicVolume = MasterVolume * MusicVolume;
		MasterXUIVolume = MasterVolume * UIVolume;
	}

	OnLoadedSettings.Broadcast();

}

FSoundType UGameInstanceInfernale::GetSoundType(FString SoundTypeName, FString SoundName)
{
	auto Sounds = GetGameSettingsDataAsset()->DataAssetsSettings[GetGameSettingsDataAsset()->DataAssetsSettingsToUse].
	              SoundsAssets->Sounds;
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

void UGameInstanceInfernale::PlaySound(const UObject* WorldContextObject, FString SoundTypeName, FString SoundName,
                                       FVector Location)
{
	const auto SoundType = GetSoundType(SoundTypeName, SoundName);
	float VolumeMultiplier = 1.f;
	VolumeMultiplier = MasterVolume;

	switch (SoundType.SoundFamily)
	{
	case ESoundFamily::Ambient:
		VolumeMultiplier *= AmbianceVolume;
		break;
	case ESoundFamily::Effect:
		VolumeMultiplier *= EffectVolume;
		break;
	case ESoundFamily::Music:
		VolumeMultiplier *= MusicVolume;
		break;
	case ESoundFamily::UI:
		VolumeMultiplier *= UIVolume;
		break;
	}

	if (SoundType.Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(WorldContextObject, SoundType.Sound, Location,
		                                      SoundType.Volume * VolumeMultiplier);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Sound not found: %s"), *SoundName);
	}
}

void UGameInstanceInfernale::PlaySoundWithChangedVolumePitch(const UObject* WorldContextObject, FString SoundTypeName,
                                                             FString SoundName, FVector Location, FRotator Rotation,
                                                             float Volume, float Pitch)
{
	auto SoundType = GetSoundType(SoundTypeName, SoundName);
	float VolumeMultiplier = 1.f;
	const UPlayerSettingSave* PlayerSettings = Cast<UGameInstanceInfernale>(
		WorldContextObject->GetWorld()->GetGameInstance())->PlayerSettingSave;
	if (PlayerSettings != nullptr)
	{
		VolumeMultiplier = PlayerSettings->MasterVolume;

		switch (SoundType.SoundFamily)
		{
		case ESoundFamily::Ambient:
			VolumeMultiplier *= PlayerSettings->AmbianceVolume;
			break;
		case ESoundFamily::Effect:
			VolumeMultiplier *= PlayerSettings->EffectVolume;
			break;
		case ESoundFamily::Music:
			VolumeMultiplier *= PlayerSettings->MusicVolume;
			break;
		case ESoundFamily::UI:
			VolumeMultiplier *= PlayerSettings->UIVolume;
			break;
		}
	}

	if (SoundType.SoundName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Sound not found: %s"), *SoundName);
		return;
	}
	if (SoundType.Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(WorldContextObject, SoundType.Sound, Location, Rotation,
		                                      SoundType.Volume * Volume * VolumeMultiplier, Pitch);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Sound not found: %s"), *SoundName);
	}
}

void UGameInstanceInfernale::PlaySound2DWithChangedVolumePitch(const UObject* WorldContextObject, FString SoundTypeName,
	FString SoundName, float Volume, float Pitch)
{
	auto SoundType = GetSoundType(SoundTypeName, SoundName);
	float VolumeMultiplier = 1.f;
	const UPlayerSettingSave* PlayerSettings = Cast<UGameInstanceInfernale>(
		WorldContextObject->GetWorld()->GetGameInstance())->PlayerSettingSave;
	if (PlayerSettings != nullptr)
	{
		VolumeMultiplier = PlayerSettings->MasterVolume;

		switch (SoundType.SoundFamily)
		{
		case ESoundFamily::Ambient:
			VolumeMultiplier *= PlayerSettings->AmbianceVolume;
			break;
		case ESoundFamily::Effect:
			VolumeMultiplier *= PlayerSettings->EffectVolume;
			break;
		case ESoundFamily::Music:
			VolumeMultiplier *= PlayerSettings->MusicVolume;
			break;
		case ESoundFamily::UI:
			VolumeMultiplier *= PlayerSettings->UIVolume;
			break;
		}
	}
	if (SoundType.SoundName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Sound not found: %s"), *SoundName);
		return;
	}
	if (SoundType.Sound)
	{
		UGameplayStatics::PlaySound2D(WorldContextObject, SoundType.Sound, SoundType.Volume * Volume * VolumeMultiplier,
		                              Pitch);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Sound not found: %s"), *SoundName);
	}
}