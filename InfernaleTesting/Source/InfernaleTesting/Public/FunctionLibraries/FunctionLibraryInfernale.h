// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LD/LDElement/NeutralCamp.h"
#include "LeData/DataGathererActor.h"
#include "FunctionLibraryInfernale.generated.h"

enum class EEntityType : uint8;
struct FNodeEffect;
class AUnitActorManager;
class UGameSettingsDataAsset;
struct FTimeRemaining;
enum class EPlayerOwning : uint8;
enum class ETeam : uint8;

UENUM(Blueprintable)
enum ECustomTraceChannel
{
	FluxStartEnd,
	HoverInteractor,
	PathfindingCollision,
	CameraCollision,
};

/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UFunctionLibraryInfernale : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static ETeam GetNextTeam(ETeam CurrentTeam);
	static ETeam GetPreviousTeam(ETeam CurrentTeam);
	static EPlayerOwning GetNextPlayerOwning(EPlayerOwning CurrentPlayerOwning);
	static EPlayerOwning GetPreviousPlayerOwning(EPlayerOwning CurrentPlayerOwning);
	static FString GetRomanStringFromNumber(int Number);
	static bool TryGetUnitActorManager(AUnitActorManager*& OutUnitActorManager, TSubclassOf<AUnitActorManager> UnitActorManagerClass);
	static FLinearColor GetOldTeamColorCpp(ETeam Team);
	static FLinearColor GetTeamColorCpp(ETeam Team, EEntityType EntityType);
	static FLinearColor GetTeamColorEmissiveCpp(ETeam Team, EEntityType EntityType);
	static UStaticMesh* GetMeshFromId(const FString Id);
	static UStaticMesh* GetMeshFromId(const FString Id, UGameSettingsDataAsset* GameSettingsDataAsset);
	
	UFUNCTION( BlueprintPure, Category = "FunctionLibraryInfernale" )
	static bool InEditor();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "FunctionLibraryInfernale")
	static UGameSettingsDataAsset* GetGameSettingsDataAsset();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "FunctionLibraryInfernale")
	static FString GetTimeRemaining(FTimeRemaining TimeRemaining);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "FunctionLibraryInfernale")
	static ECollisionChannel GetCustomTraceChannel(ECustomTraceChannel CustomTraceChannel);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "FunctionLibraryInfernale")
	static FText GetNodeEffectAsText(FNodeEffect NodeEffect, const FString& Style = "<TITLE_sm9>");
	
	UFUNCTION(BlueprintCallable, Category = "FunctionLibraryInfernale", meta = (WorldContext = "WorldContextObject"))
	static void ForceDestroyComponent(const UObject* WorldContextObject, UActorComponent* ActorComponent);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DataGathererUtils")
	static TArray<FPlayerSnapshot> GetPlayerSnapshotsFromJsonFromPath(FString Path);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DataGathererUtils")
	static TArray<FDataGathererPlayerInfo> GetDataGathererPlayerInfoFromJsonFromPath(FString Path);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DataGathererUtils")
	static TArray<FString> GetTimeMMSSFromJsonFromPath(FString Path);

};
