// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameModeParentComponent.h"
#include "Components/ActorComponent.h"
#include "Structs/SimpleStructs.h"
#include "VictoryManagerComponent.generated.h"


class ADataGathererActor;
struct FPlayerSnapshot;
class APandemoniumMainBuilding;
enum class ETeam : uint8;
class ABuildingParent;
struct FOwner;
class AMainBuilding;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTimeRemainingDelegate, FTimeRemaining, TimeRemaining);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FVictoryPointChanged, FVictoryPoints, VictoryPoints, EVictoryPointReason, Reason, ETeam, TeamScoreChanged, int, Score);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPlayerLost, FVictoryPoints, VictoryPoints, TArray<ETeam>, DeadPlayers, ETeam, Team);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPlaySequence, FVictoryPoints, VictoryPoints, TArray<ETeam>, DeadPlayers, ETeam, Team);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class INFERNALETESTING_API UVictoryManagerComponent : public UGameModeParentComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UVictoryManagerComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void AddMainBuilding(AMainBuilding* MainBuilding);
	void AddPandemoniumMainBuilding(APandemoniumMainBuilding* MainBuilding);
	FTimeRemaining GetGameTimer() const;
	void SetGameTimer(float Time);
	void AddKilledTeam(ETeam Team);

	UFUNCTION() void AddPointsOnBossDeath(FOwner Depleter, float Value);
	UFUNCTION(BlueprintCallable) float GetVictoryPointsOfTeam(ETeam Team) const;
	UFUNCTION(BlueprintCallable) void CallOnVictoryPointsChanged();

	void CheatAddVictoryPoints(ETeam Team, float Amount, EVictoryPointReason Reason);
	void CheatEndGame();
	void CallOnGameEnded();
	void CallLaunchSequence();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UFUNCTION() void OnPreLaunchGame();
	UFUNCTION() void OnLaunchGame();
	UFUNCTION() void OnMainBuildingCaptured(ABuildingParent* MainBuilding, FOwner OldOwner, FOwner NewOwner);
	UFUNCTION() void OnMainBuildingVictoryPointChanged(AMainBuilding* MainBuilding, float OldValue, float NewValue);
	UFUNCTION() void PandemoniumMBTimerTick(APandemoniumMainBuilding* PandemoniumMB);
	UFUNCTION() void SendData();

	bool DoesPlayerStillHaveBuildings(FOwner Owner);
	void StartGameTimer();
	void StartPandemoniumMBTimer();
	void GainPandemoniumMBIncome(APandemoniumMainBuilding* PandemoniumMB, float TimeElapsed);
	void AddRemoveVictoryPoints(FOwner Owner, float Value, EVictoryPointReason Reason);
	void AddRemoveVictoryPointsNoEvent(FOwner Owner, float Value);
	void EndGameByTime();

public:
	UPROPERTY(BlueprintAssignable)
	FTimeRemainingDelegate TimeRemainingDelegate;
	
	UPROPERTY(BlueprintAssignable)
	FTimeRemainingDelegate GameDurationStarted;

	UPROPERTY(BlueprintAssignable)
	FVictoryPointChanged VictoryPointChanged;

	UPROPERTY(BlueprintAssignable)
	FPlayerLost OnPlayerLost;

	UPROPERTY(BlueprintAssignable)
	FPlayerLost OnGameEnded;
	FOnPlaySequence OnLaunchSequence;

protected:
	TArray<AMainBuilding*> MainBuildings = TArray<AMainBuilding*>();
	TArray<APandemoniumMainBuilding*> PandemoniumMainBuildings = TArray<APandemoniumMainBuilding*>();
	TArray<ETeam> DeadTeams = TArray<ETeam>();
	FVictoryPoints VictoryPoints;

	bool bGameStarted = false;
	bool bGamePaused = false;

	float UpdateTimer = 0;
	float InitialTime = 0;

	int CurrentSnapshotIndex = 0;
	
	FTimeRemaining GameTimer;

private:
	ADataGathererActor* DataGatherer;
	
};
