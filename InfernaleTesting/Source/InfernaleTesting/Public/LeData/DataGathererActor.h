// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "LD/Buildings/MainBuilding.h"
#include "GameMode/Infernale/GameModeInfernale.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializerMacros.h"
#include "JsonObjectConverter.h"
#include "Structs/SimpleStructs.h"

#include "DataGathererActor.generated.h"

enum class ECampTypeForData : uint8;
class AMainBuilding;
class AGameModeInfernale;
enum class ETeam : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FJsonFileDone);


UCLASS()
class INFERNALETESTING_API ADataGathererActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADataGathererActor();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "DataGatherer")
	void TakeSnapshot();

	UFUNCTION(BlueprintCallable, Category = "DataGatherer")
	void AddMonsterSoulsToOwner(FOwner Player, float Amount, ECampTypeForData CampType);

	UFUNCTION(CallInEditor, Category = "DataGatherer")
	void SaveSnapshotsToJson();

	UFUNCTION(NetMulticast, Reliable)
	void ReplicateTotalSoulsGainForLastSnapshotMulticast(FTotalSoulsGainForLastSnapshotStruct ReplicatedStruct);

	UFUNCTION(NetMulticast, Reliable)
	void SetIsActiveMulticast(bool bIsActive);

	UFUNCTION(NetMulticast, Reliable)
	void ReplicatePlayerSnapshotMulticast(const FPlayerSnapshot& PlayerSnapshotRep);

	UFUNCTION(NetMulticast, Reliable)
	void AllInfoReplicatedMulticast();

	bool GatherEconomyDataForPlayer(APlayerStateInfernale* PlayerState, FDataGathererPlayerInfo& PlayerInfo,
									bool& bValue);
	bool GatherPlayerInfoSnapshot(FPlayerSnapshot* Snapshot, UVictoryManagerComponent* VictoryManagerComponent,
								  APlayerStateInfernale* PlayerState);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DataGathererUtils")
	float GetAverageSoulsGainOfTeam(ETeam Team);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DataGathererUtils")
	TArray<float> GetPlayerSoulsInreserve(FOwner Player, TArray<FDataGathererPlayerInfo> PlayerInfos);

	TArray<float> GetPlayerSoulsGain(FOwner Owner, TArray<FDataGathererPlayerInfo> Array);
	TArray<float> GetPlayerAmalgamsOnMapPerTeam(FOwner Owner);
	TArray<float> GetPlayerBuildingCount(FOwner Owner);
	TArray<float> GetPlayerDominationPoints(FOwner Owner, TArray<FDataGathererPlayerInfo> Array);
	TArray<FString> GetTimetimeMMSS();
	TArray<FPlayerSnapshot> GetPlayerSnapshots();
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DataGathererUtils")
	FPlayerChartData GetPlayerInfos(FOwner Player, TArray<FDataGathererPlayerInfo> PlayerInfos, EChartGetInfo ChartInfoToGet, TArray<FString> TimeMMSS);

	void TryLoad();
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DataGatherer")
	TArray<FPlayerSnapshot> PlayerSnapshots;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DataGatherer")
	TArray<AMainBuilding*> MainBuildings;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DataGatherer")
	TArray<ABoss*> Bosses;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DataGatherer")
	TMap<ETeam, float> EarnedSoulsFromMonsters;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DataGatherer")
	float TimeBetweenSnapshotsInSeconds = 5.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DataGatherer")
	bool bActive = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DataGatherer")
	bool bSaveSnapshots = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DataGathererPick")
	bool bGatherMainBuilding = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DataGathererPick")
	bool bGatherBosses = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DataGathererPick")
	bool bGatherAmalgams = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DataGathererPick")
	bool bGatherPlayerInfo = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DataGathererPick")
	bool bGatherEconomy = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DataGathererPick")
	bool bGatherBases = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DataGathererPick")
	bool bGatherCombat = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DataGathererPick")
	bool bGatherAmalgamPositions = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DataGathererPick")
	bool bGatherAmalgamKill = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DataGathererPick")
	bool bGatherTransmutationInfos = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DataGathererPick")
	bool bGatherDominationPoints = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DataGathererUtils")
	TMap<ETeam, float> TotalSoulsGainForLastSnapshot;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DataGathererUtils")
	TMap<ETeam, float> AveragedTotalSoulsGainForLastSnapshot;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DataGathererUtils")
	int NumberOfSnapshotsPerAverageSoulsGain = 3;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DataGathererUtils")
	FString JsonSavePath;

	UPROPERTY(BlueprintAssignable, Category = "DataGathererUtils")
	FJsonFileDone JsonFileDone;

private:
	float TimeSinceLastSnapshot = 0.f;

	TMap<ETeam, int> SmallCampsKilled;
	TMap<ETeam, int> BigCampsKilled;
	TMap<ETeam, int> BossesKilled;


	TMap<ETeam,float> AverageSoulsGain;

	TMap<ETeam, TArray<float>> SnapshotsSoulsGain;

	bool isJsonLoaded = false;
};
