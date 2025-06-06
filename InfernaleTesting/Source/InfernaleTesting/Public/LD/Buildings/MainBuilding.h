// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataAsset/MainBuildingDistrictsDataAsset.h"
#include "Interfaces/InGameUI.h"
#include "LD/Buildings/BuildingParent.h"
#include "MainBuilding.generated.h"

class USpawnerComponent;
class ABoss;
class UMainBuildingDistrictsDataAsset;
class UMainBuildingDataAsset;
class UBattleManagerComponent;
struct FBuildingEffect;
class UAttacksDataAsset;
struct FAttackStruct;
class UAttackComponent;
class UEconomyDataAsset;
class USplineComponent;
class AGameModeInfernale;
class ABreach;

// USTRUCT(Blueprintable)
// struct FMainBuildingDistrictInfo
// {
// 	GENERATED_BODY()
//
// public:
// 	//UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector<
// 	
// 	
// };

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMainBuildingVictoryPointChanged, AMainBuilding*, MainBuilding, float, OldValue, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFloatDelegate, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIntDelegate, int, Value);
/**
 * 
 */
UCLASS()
class INFERNALETESTING_API AMainBuilding : public ABuildingParent, public IInGameUI
{
	GENERATED_BODY()

public:
	AMainBuilding();

	virtual void SetOwner(FOwner NewOwner) override;
	virtual void ChangeOwner(FOwner NewOwner) override;
	virtual bool CanCreateAFlux() const override;
	virtual AMainBuilding* GetMainBuilding() override;
	virtual void OnSelectedForFluxCpp() override;
	virtual void OnDeselectedForFluxCpp() override;
	virtual void UpdateMaxHealth(float NewMaxHealth) override;
	virtual bool IsMainBuilding() const override;
	virtual float GetThornDamage() const override;

	float GetVictoryPointValue() const;
	float GetControlAreaRadius() const;
	void AddVictoryPointValue(float Value);
	void ShowControlArea(bool bShow);
	TArray<ABreach*> GetBreaches() const;
	bool StartOwned();

	void AskRefreshMaxHealth();
	void RandomizeBreachesOnGameStart();

	void StartOrCreateTimerIfNotExists();
	void RefreshSummonBossEffect();
	virtual void ApplyBuildingEffect(ABuilding* Source, FBuildingEffect BuildingEffect, bool UseOverclockEffect) override;
	virtual void RemoveBuildingEffect(ABuilding* Source, FBuildingEffect BuildingEffect, bool UpdateVisual) override;

	virtual void InteractStartHover(APlayerControllerInfernale* Interactor) override;
	virtual void InteractEndHover(APlayerControllerInfernale* Interactor) override;
	
	virtual FString GetBuildingName() override;

	void CreateNewFluxesEffect(ABuilding* Source, FBuildingEffect BuildingEffect);
	void RemoveNewFluxesEffect(ABuilding* Source, FBuildingEffect BuildingEffect);

	void AddSpawnerComponents(USpawnerComponent* Spawner);
	void RemoveSpawnerComponents(USpawnerComponent* Spawner);
	
	void AskUpdateSpawners();
	FVector GetFluxMidAngleNormalized() const;
	float GetAngleToleranceFlux() const;
	float GetStartingAngle() const;
	float GetAngleAroundForFluxes() const;
	int NumberOfFluxes() const;
	float GetTooCloseRadius() const;
	void StartOverclock();

	UBattleManagerComponent* GetBattleManager();
	void DisplayUI(bool Display);

	virtual TArray<TWeakObjectPtr<AFlux>> GetFluxes() override;

	UFUNCTION(BlueprintCallable, BlueprintPure) int GetBreachesConstructedCount();
	UFUNCTION(BlueprintCallable) TArray<USpawnerComponent*> GetSpawnerComponents();
	
	UFUNCTION(BlueprintCallable, BlueprintPure) int GetFluxesActiveCount();
	UFUNCTION(BlueprintCallable, BlueprintPure) bool IsAllowedToOverclock();
	UFUNCTION(BlueprintCallable, BlueprintPure) bool IsOverclockInCD();
	UFUNCTION(BlueprintCallable, BlueprintPure) bool IsOverclocked();
	UFUNCTION(BlueprintCallable, BlueprintPure) float OverclockedCurrent();
	UFUNCTION(BlueprintCallable, BlueprintPure) float OverclockedCD();
	UFUNCTION(BlueprintCallable, BlueprintPure) float GetOverclockCost();

	void CalculateFluxesActiveCount();

	TArray<ABreach*> GetBreachesAvailableForConstruction();
	TArray<ABreach*> GetBreachesWithBuilding(FString BuildingName);
	TArray<ABreach*> GetBreachesWithAnyBuilding();
	UFUNCTION(BlueprintCallable, BlueprintPure) float GetNextBuildingCost();
	
	/*
	* Returns an array of float
	* 0 - Unit power
	* 1 - Building power
	* 2 - Monster power
	*/
	TArray<float> GetActivatingFluxPower();
	void CheatCapture(FOwner NewOwner);

protected:
	virtual void BeginPlay() override;
	virtual void OnHealthDepletedOwner(AActor* Actor, FOwner Depleter) override;
	virtual void Tick(float DeltaSeconds) override;

	virtual void GameModeInfernaleInitialized();

	void SyncDataAssets();
	void TryGetGameModeInfernale();
	void InitAttacks();
	void RefreshControlRange();
	void RefreshMaxHealth();
	void UpdateTransmutationFromOwner();

	void RefreshVisibilityLocal();

	void TryGainBlood();
	void GainBlood();
	void SetLocalSoulsTimer();
	void UpdateSpawners();
	
	void RandomizeBreaches(bool bResetPrevious);

	virtual void OwnershipChanged();
	void ClearFluxes();
	void ResetFluxes();
	ABreach* SpawnBreachAt(int AngleIndex, int MaxAngleIndex, float Radius, UStaticMesh* Mesh, FVector Scale, float ZOffset = 0);
	void CreateFluxesOnPreLaunchGame();
	void NumberOfFluxesInGameChanged(int NewNumberOfFluxesInGame);

	FDistrictElementProportion GetRandomInnerBreachInfo();
	FDistrictElementProportion GetRandomOuterBreachInfo();

	UFUNCTION() void OnPreLaunchGame();
	UFUNCTION() void OnLaunchGame();
	UFUNCTION() void OnAllBasesAssigned();
	UFUNCTION() void OnAttackReady(FAttackStruct AttackStruct);
	UFUNCTION() void OnMaxRadiusChanged();
	UFUNCTION() void OnCaptureDamaged(ETeam Team, float Percent);
	UFUNCTION() void OnCaptureCompleted(FOwner DamageOwner);
	UFUNCTION() void OnCaptureCancelled();
	UFUNCTION() void AllowToOverclock();
	
	UFUNCTION() void OnBuildingParentFluxesUpdated(ABuildingParent* _);
	UFUNCTION() void OnFluxEnabled(AFlux* Flux, bool bEnabled);

	UFUNCTION(BlueprintCallable, CallInEditor) void DebugBreach();
	UFUNCTION(BlueprintCallable, CallInEditor) void DebugAttackRange();

	virtual void InteractStartMain(APlayerControllerInfernale* Interactor) override;
	virtual void InteractEndMain(APlayerControllerInfernale* Interactor) override;
	virtual bool InteractableHasUIOpen() override;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Spawn Breach Tool") void SpawnBreaches();
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Spawn Breach Tool") void DestroyLinkedBreaches();
	
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Spawn City Districts Tool") void SpawnCityDistricts();
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Spawn City Districts Tool") void DestroyLinkedCityDistricts();
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Spawn City Districts Tool") void UpdateFromData();
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Spawn City Districts Tool") void RandomizeBreaches();
	
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Spawn City Districts Tool V2") void SpawnCityDistrictsV2();
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Spawn City Districts Tool V2") void DestroyLinkedCityDistrictsV2();
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Spawn City Districts Tool V2") void UpdateFromDataV2();
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Spawn City Districts Tool V2") void SaveData();
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Spawn City Districts Tool V2") void LoadData();
	
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Spawn City Districts Tool V2|Debug") void ShowDebugID();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "MainBuildingMesh") void UpdateMainBuildingMesh();


	/* Flux Angles */
	void ShowAnglesTick();
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Flux Angles") void ShowAngles();
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Flux Angles") void SpawnFluxes();
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Flux Angles") void ClearFluxesTool();

	/* Functions for tools */
	ABreach* SpawnBreachWithInfo(FDistrictElementSpawnInfo Info, FVector BaseLocation, FRotator BaseRotation);

	UFUNCTION(Server, Reliable) void BossPercentageGains();

	UFUNCTION(NetMulticast, Reliable) void ShowControlAreaMulticast(bool bShow);
	UFUNCTION(NetMulticast, Reliable) void MaxRadiusMulticast(float NewControlAreaRadius);
	UFUNCTION(NetMulticast, Reliable) void CaptureDamageMulticast(ETeam Team, float Percent);
	UFUNCTION(NetMulticast, Reliable) void CaptureCompletedMulticast(FOwner NewOwner);
	UFUNCTION(NetMulticast, Reliable) void BreachesSpawnedMulticast(int BreachesCount);
	UFUNCTION(NetMulticast, Reliable) void UpdateBreachesMulticast(const TArray<ABreach*>& NewBreaches);
	UFUNCTION(NetMulticast, Reliable) void SychronizeFluxesMulticast(const TArray<AFlux*>& NewFluxes);
	UFUNCTION(NetMulticast, Reliable) void RefreshFluxVisibilityMulticast();
	UFUNCTION(NetMulticast, Reliable) void RefreshVisibilityMulticast();
	UFUNCTION(NetMulticast, Reliable) void SetAllowedToOverclockMulticast(const bool bAllowedToOverclock);
	UFUNCTION(NetMulticast, Reliable) void StartOverclockCDMulticast(const bool Overclocked);
	UFUNCTION(NetMulticast, Reliable) void NumberOfFluxesInGameChangedMulticast(int NewNumberOfFluxesInGame);
	UFUNCTION(NetMulticast, Reliable) void ReplicateLastFluxActiveNumbersMulticast(int NewLastFluxActiveNumbers);
	UFUNCTION(NetMulticast, Reliable) void CaptureCancelledMulticast();
	
	UFUNCTION(BlueprintImplementableEvent) void ShowControlAreaBP();
	UFUNCTION(BlueprintImplementableEvent) void HideControlAreaBP();
	UFUNCTION(BlueprintImplementableEvent) void MaxRadiusBP(float NewControlAreaRadius);
	UFUNCTION(BlueprintImplementableEvent) void CaptureDamageBP(ETeam Team, float Percent);
	UFUNCTION(BlueprintImplementableEvent) void CaptureCompletedBP(FOwner NewOwner);
	UFUNCTION(BlueprintImplementableEvent) void BreachesSpawnedBP(int BreachesCount);
	UFUNCTION(BlueprintImplementableEvent) void CaptureCancelledBP();
	UFUNCTION(BlueprintImplementableEvent) void UpdateMainBuildingMeshBP();

public:
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FMainBuildingVictoryPointChanged MainBuildingVictoryPointChanged;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FFloatDelegate NumberOfFluxInGameUpdated;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FIntDelegate BreachesUpdatedEvent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	UMainBuildingDataAsset* MainBuildingDataAsset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main Building")
	UStaticMeshComponent* MainBaseStaticMesh;

	// UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main Building")
	// USplineMeshComponent* MainBaseSplineMesh;

	// Set the boss assigned to this main building
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main Building")
	ABoss* Boss;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main Building")
	float BuildingPower = 0.f;

protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main Building") UAttackComponent* AttackComponent;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings") UEconomyDataAsset* EconomyDataAsset;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings") UMainBuildingDistrictsDataAsset* MainBuildingDistrictsDataAsset;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings") UGameSettingsDataAsset* GameSettingsDataAsset;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main Building") AGameModeInfernale* GameModeInfernale;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main Building") TArray<ABreach*> Breaches = TArray<ABreach*>();
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main Building") FString MainBuildingName = "Galtiory";

	UBattleManagerComponent* BattleManager;

	bool bUseMass = true;
	int LastFluxActiveNumbers = 0;


	/* Spawn Breach Tool */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spawn Breach Tool") int BreachesToSpawn = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spawn Breach Tool") float BreachSpawnRadius = 10000;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spawn Breach Tool") TSubclassOf<ABreach> BreachSpawnClass;

	/* Spawn City Districts Tool */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spawn City Districts Tool") int InnerCityDistrictsToSpawn = 8;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spawn City Districts Tool")	int OuterCityDistrictsToSpawn = 12;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spawn City Districts Tool") float InnerCityDistrictsSpawnRadius = 100;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spawn City Districts Tool") float OuterCityDistrictsSpawnRadius = 500;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spawn City Districts Tool V2") int MinBreachToSpawn = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spawn City Districts Tool V2") int MaxBreachToSpawn = 12;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spawn City Districts Tool") FCityElements CityElements;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spawn City Districts Tool V2") FString LoadFromSavedDataString;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spawn City Districts Tool V2") FString SaveDataString;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spawn City Districts Tool V2") TArray<FDistrictData> DistrictDataV2;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spawn City Districts Tool V2|Debug") FString DebugId;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spawn City Districts Tool V2|Debug") float DebugDuration = 5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spawn City Districts Tool V2|Debug") bool bDebugTool = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "MainBuildingMesh") UStaticMesh* MainBuildingMesh;

	/* Flux angles */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Flux Angles", meta = (ClampMin = "0", ClampMax = "360")) float StartingAngleDegree = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Flux Angles", meta = (ClampMin = "0", ClampMax = "360")) float AngleAroundForFluxes = 90;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Flux Angles", meta = (ClampMin = "1")) int NumberOfFlux = 2;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings") int NumberOfFluxInGame = 2;
	
	// Artifacts

	// Spline list
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main Building") TArray<USplineComponent*> SplineList = TArray<USplineComponent*>();
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main Building") bool bStartOwned = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "Main Building") float ActualControlAreaRadius = 1000;
	UPROPERTY(BlueprintReadOnly, Category = "Main Building") float ActualMaxHealth = 100;
	UPROPERTY(BlueprintReadOnly, Category = "Main Building") float PowerDemon = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Main Building") float PowerBuilding = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Main Building") float PowerMonster = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Main Building") float OverclockCD = 120.f;
	UPROPERTY(BlueprintReadOnly, Category = "Main Building") float OverclockDuration = 20.f;
	UPROPERTY(BlueprintReadOnly, Category = "Main Building") float OverclockCost = 300.f;
	float OverclockCurrentTime = 0.f;
	bool bIsOverclockedInCD = false;
	bool bIsOverclocked = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main Building") float VictoryPointValue = 10;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main Building") float ControlAreaRadius = 1000;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main Building") float BaseMaxHealth = 100;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main Building") float BaseMaxHealthPlayer = -1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main Building") float NeutralThornDamage = 0.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main Building") float PlayerThornDamage = 75.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main Building") UCurveFloat* BuildingPriceCurve = nullptr;


	TArray<TWeakObjectPtr<USpawnerComponent>> SpawnerComponents = TArray<TWeakObjectPtr<USpawnerComponent>>();

	bool GameModeInfernaleWasInitialized = false;
	bool bAreaVisible = false;
	bool UIisOpened = false;
	bool AllowedToOverclock = true;

	//float BloodGainedPerTime = 5;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main Building")
	float TimeBeforeBloodGain = 1;
	FTimerHandle TimerHandle_BloodGain;

	float BossPercentageToAdd = .0f;
	FTimerHandle TimerHandle_BossPercentageGains;

	ETeam LastCapturedTeam = ETeam::NatureTeam;
	float LastPercent = -1.f;
	float CaptureRefreshRate = 0.5f;
	float CaptureCurrentTime = 0.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings") bool bDebugFluxSpawn = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")	bool bDebugAngles = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")	bool bDebugFluxCreationPos = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main Building") bool bHasAtLeastOneBossBuilding = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main Building") float TooCloseRadius = 450.f;
};
