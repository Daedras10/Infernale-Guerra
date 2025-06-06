// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AdvancedFriendsGameInstance.h"
#include "Player/InfernalePawn.h"
#include "Structs/SimpleStructs.h"
#include "GameInstanceInfernale.generated.h"

struct FVictoryInfo;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLoadedSettings);

class UWidget;
class UPlayerSettingSave;



class UGameSettingsDataAsset;
/**
 * 
 */

USTRUCT(Blueprintable)
struct FLocalizedText
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FString EnglishText;
};

UCLASS()
class INFERNALETESTING_API UGameInstanceInfernale : public UAdvancedFriendsGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	UFUNCTION() void BeginLoadingScreen(const FString& MapName);
	UFUNCTION() void EndLoadingScreen(UWorld* World);

	UFUNCTION(BlueprintCallable, BlueprintPure) bool UseLAN();
	UFUNCTION(BlueprintCallable) void SetUseLAN(bool bNewValue);

	void SetVictoryInfo(const FVictoryInfoMap& Infos);

	UFUNCTION(BlueprintImplementableEvent) void BeginLoadingScreenBP();
	UFUNCTION(BlueprintImplementableEvent) void EndLoadingScreenBP();
	UFUNCTION(BlueprintImplementableEvent) UWidget* GetWidgetBP();
	
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent) void CreateSession();
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent) void CreateNewSessionWithName(FString& SessionName);
	
	UFUNCTION(BlueprintCallable) void ServerTravelToScene(FString SceneName);
	
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent) void FindSessionInfernale();

	UFUNCTION(BlueprintCallable) int GetPlayers();
	UFUNCTION(BlueprintCallable) void SetPlayers(int NewPlayers);
	UFUNCTION(BlueprintCallable) int GetGameDuration();
	UFUNCTION(BlueprintCallable) void SetGameDuration(float NewDuration);
	UFUNCTION(BlueprintCallable) int GetCustomGameMode();
	UFUNCTION(BlueprintCallable) void SetCustomGameMode(int NewCustomGameMode);
	UFUNCTION(BlueprintCallable) FString GetLastTravelScene();
	UFUNCTION(BlueprintCallable) void SetLastTravelScene(FString SceneName);
	UFUNCTION(BlueprintCallable, BlueprintPure) int GetCustomTeamMode();
	UFUNCTION(BlueprintCallable) void SetCustomTeamMode(int NewCustomTeamMode);
	
	UGameSettingsDataAsset* GetGameSettingsDataAsset();

	// Video settings
	UFUNCTION(BlueprintCallable) void SetVSync(bool bNewValue);

	UFUNCTION(BlueprintCallable) void SaveGameSettings();
	UFUNCTION(BlueprintCallable) void LoadGameSettings();

	UPROPERTY(BlueprintAssignable) FOnLoadedSettings OnLoadedSettings;
	
	// void CheckGameSave(bool OpenLevel = false);
	// void CreateSaveGame();
	// void LoadSaveGame();
	// void SaveGame();
	
	// void ApplyScreenSettings();
	// void ApplyResolutionSettings();
	// void ApplySoundSettings();
	// void ApplyGraphicsSettings();
	
	// UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	// void JoinSessionInfernale(FBlueprintSessionResult LobbyToJoin);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)	UPlayerSettingSave* PlayerSettingSave;

protected:
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UGameSettingsDataAsset* GameSettingsDataAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) UUserWidget* LoadingScreenWidget;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSubclassOf<class UUserWidget> LoadingScreenWidgetClass;
	
	bool bUseLAN = false;
	//TSharedRef<SWidget> LoadingScreenWidgetS;
	
	int Players = 1;
	float GameDuration = 300;
	int CustomGameMode = 0;
	int CustomTeamMode = 1;
	/*
	 * 0 : 2v2
	 * 1 : 1v1v1v1
	 * 2 : 1v1
	 */
	
	// Save Data : TODO move to a struct
	int ViewDistance = 3;
	int AntiAliasing = 3;
	int PostProcess = 3;
	int Shadows = 3;
	int GlobalIllumination = 3;
	int Reflections = 3;
	int Textures = 3;
	int Effects = 3;
	int Foliage = 3;
	int Shading = 3;
	int ScreenID = 0;
	FString LastTravelScene;

	
	float MasterVolume = 1.f;
	float MusicVolume = 1.f;
	float EffectVolume = 1.f;
	float AmbianceVolume = 1.f;
	float UIVolume = 1.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere) FVictoryInfoMap EndGameVictoryInfo;

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FPlayerInfo PersonalPlayerInfo;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) TArray<FPlayerInfo> OpponentPlayerInfo;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bIsPhilosophicallyAccurateGame = true; // si c'est la vraie game c'est vrai
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bDebug = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)	AInfernalePawn* InfernalePawn;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float MasterXMusicVolume = 1.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float MasterXEffectVolume = 1.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float MasterXAmbianceVolume = 1.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float MasterXUIVolume = 1.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float DefaultMultiplier = 1.f;

	UFUNCTION(BlueprintCallable, Category = "SoundsDataAsset")
	FSoundType GetSoundType(FString SoundTypeName, FString SoundName);
	UFUNCTION(BlueprintCallable, Category = "SoundsDataAsset", meta=(WorldContext="WorldContextObject", AdvancedDisplay = "3", UnsafeDuringActorConstruction = "true", Keywords = "play"))
	void PlaySound(const UObject* WorldContextObject, FString SoundTypeName, FString SoundName, FVector Location);
	UFUNCTION(BlueprintCallable, Category = "SoundsDataAsset", meta=(WorldContext="WorldContextObject", AdvancedDisplay = "3", UnsafeDuringActorConstruction = "true", Keywords = "play"))
	void PlaySoundWithChangedVolumePitch(const UObject* WorldContextObject, FString SoundTypeName, FString SoundName, FVector Location, FRotator Rotation, float Volume, float Pitch);
	UFUNCTION(BlueprintCallable, Category = "SoundsDataAsset", meta=(WorldContext="WorldContextObject", AdvancedDisplay = "3", UnsafeDuringActorConstruction = "true", Keywords = "play"))
	void PlaySound2DWithChangedVolumePitch(const UObject* WorldContextObject, FString SoundTypeName, FString SoundName, float Volume, float Pitch);
};
