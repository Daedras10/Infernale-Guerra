// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameModeInfernale.generated.h"

enum class ETeam : uint8;
enum class EPlayerOwning : uint8;
struct FPlayerInfo;
struct FTimeRemaining;
class AUnitActorManager;
class UVictoryManagerComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGMIDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGMIDelegatePCI, APlayerControllerInfernale*, PlayerController);

class APlayerControllerInfernale;
struct FOwner;
class APlayerStateInfernale;
/**
 * 
 */
UCLASS()
class INFERNALETESTING_API AGameModeInfernale : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGameModeInfernale();
	
	TWeakObjectPtr<APlayerStateInfernale> GetPlayerState(EPlayerOwning Player);
	TArray<APlayerStateInfernale*> GetPlayerStates();
	void AddPlayerState(FOwner NewOwner, APlayerStateInfernale* PlayerState);
	void NotifyPlayerReady(APlayerControllerInfernale* PlayerController, FPlayerInfo PlayerInfo);
	UVictoryManagerComponent* GetVictoryManagerComponent() const;
	AUnitActorManager* GetUnitActorManager() const;
	TSubclassOf<AUnitActorManager> GetUnitActorManagerClass() const;
	float GetRadiusFromGameDurationAdditive() const;
	float GetRadiusFromGameDurationMultiplicative() const;
	float GetRadiusFromGameDuration(float InitialValue) const;
	void ResynchPS(APlayerStateInfernale* PlayerState);
	TMap<EPlayerOwning, ETeam> GetPlayerStatesTeam() const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure) TArray<APlayerControllerInfernale*> GetPlayerControllers() const;

	bool IsGameStarted() const;

	UFUNCTION() void BeforeAllPlayersReadyWaiter();
	UFUNCTION() void RandomDraftBasesForPlayers();

	UFUNCTION(BlueprintCallable) void CycleTurns();
	UFUNCTION(BlueprintCallable) void GameCleanup();

	FString GetPlayerNameInfo(int Index);
	int EPlayerToInt(EPlayerOwning Player);
	int ETeamToInt(ETeam Team);
	EPlayerOwning IntToEPlayer(int Index);
	ETeam IntToETeam(int Index);
	void StartNewGame(float NewTimeLeft);
	bool GameHasEnded() const;


	/* Functions for cheats */
	void CheatAddSoulsToPlayer(int PlayerID, float Amount);
	
protected:
	virtual void BeginPlay() override;
	
	UFUNCTION() void OnPlayerStateOwnerChanged(APlayerStateInfernale* PlayerState, FOwner NewOwner);
	UFUNCTION() void OnGameDurationStarted(FTimeRemaining GameDuration);

	UFUNCTION(BlueprintCallable) void StartDraft();
	void MoveAllPlayersToSpawns();
	void UpdateRadiusChange();
	void BeforeAllPlayersReady();
	void SetAllPlayerState();
	void PostSetupStartGame();
	bool AreAllPlayersReady() const;
	void LaunchGameAfterDelay(bool bPrelaunchGame, bool bLaunchGame);
	void BroadcastPreLaunchGame();
	void BroadcastLaunchGame();
	FOwner GetOwnerInfoPlayer(int Index);

	
	void KillEverything();


public:
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FGMIDelegatePCI PlayerReady;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FGMIDelegate AllPlayersSpawned;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FGMIDelegate AllPlayersReady;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FGMIDelegate AllBasesAssigned;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FGMIDelegate LaunchGame;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FGMIDelegate PreLaunchGame; // Transform into GamePreparations and mabyB add a new PreLaunchGame when the draft is here (this one is used for everything that prepare the game)
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FGMIDelegate MaxRadiusChanged;
	UPROPERTY(Blueprintable, BlueprintReadWrite) int CurrentPlayerTurn = 0;
	
	
protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UVictoryManagerComponent* VictoryManagerComponent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<AUnitActorManager> UnitActorManagerClass;
	
	TMap<EPlayerOwning, APlayerStateInfernale*> PlayerStates = TMap<EPlayerOwning, APlayerStateInfernale*>();
	TMap<EPlayerOwning, ETeam> PlayerStatesTeam = TMap<EPlayerOwning, ETeam>();
	TArray<APlayerControllerInfernale*> PlayerControllers = TArray<APlayerControllerInfernale*>();
	int PlayersReady = 0;
	bool bGameStarted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) float LaunchGameDelay = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float PreLaunchGameDelay = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float RadiusChangeUpdate = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) UCurveFloat* GameDurationToRadiusCurveAdditive;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UCurveFloat* GameDurationToRadiusCurveMultiplicative;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName StarterMainBuildingTag = "StarterMainBuilding";
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float DraftNumberOfBases = 3;

	//Draft
	UPROPERTY(BlueprintReadWrite) int CurrentCycle = 0;
	UPROPERTY(BlueprintReadWrite) int MaxCycles = 2;

	float MaxDuration = 0;
	FTimerHandle RadiusChangeTimerHandle;

	bool bDebug = false;
	bool GameEnded = false;
};
