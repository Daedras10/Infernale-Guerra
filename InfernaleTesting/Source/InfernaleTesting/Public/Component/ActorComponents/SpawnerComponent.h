// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Mass/Spawner/AmalgamSpawerParent.h"
#include "SpawnerComponent.generated.h"

class UAmalgamTraitBase;
class UUnitActorDataAsset;
class AUnitActorManager;
class ABuildingParent;
struct FMassEntityConfig;
struct FMassSpawnDataGenerator;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSpawnerDelegate, AAmalgamSpawnerParent*, Spawner); //Should likely be TWeakObjectPtr<AAmalgamSpawnerParent>

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class INFERNALETESTING_API USpawnerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USpawnerComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void DestroyComponent(bool bPromoteChildren) override;
	void SetSpawnDelay(float NewSpawnDelay);
	void SetSpawnerNumbers(int NumberOfSpawners);
	UFUNCTION(BlueprintCallable) void DisableSpawning();

	inline ABuildingParent* GetBuilding() { return Building; }
	inline FAmalgamSpawnData GetSpawnData() { return AmalgamData; }
	inline TArray<FMassSpawnedEntityType> GetEntityTypes() { return EntityTypes; }
	inline TArray<FMassSpawnDataGenerator> GetSpawnDataGenerators() { return SpawnDataGenerators; }

	UFUNCTION(BlueprintCallable) const UAmalgamTraitBase* GetAmalgamTrait();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetEnabled(bool bEnabled);
	
	float GetSpawnDelayMult();
	float GetDefaultSpawnDelay();

	void SetStrengthMultiplier(float NewStrengthMultiplier);
	float GetStrengthMultiplier();
	
	void CallOnFluxUpdated();

protected:
	virtual void BeginPlay() override;

	void SyncDataAsset();
	void SpawnUnits();
	void SpawnUnitsMass();
	void SpawnUnitsActors();
	void AddToBuildings();
	void RemoveFromBuildings();

	void TryGetActorManager();
	void StartTimerUpdateFluxes();

	UFUNCTION(NetMulticast, Reliable) void OnBuildingConstructedMulticast(ABuildingParent* InBuilding, AAmalgamSpawnerParent* InSpawner);
	UFUNCTION(NetMulticast, Reliable) void OnSpawnCreateNiagaraComponent(UNiagaraSystem* NiagaraSystem, FVector Location, AAmalgamSpawnerParent* InSpawner);
	UFUNCTION(NetMulticast, Reliable) void ReplicateSpawn(AAmalgamSpawnerParent* InSpawner, int Index);

	UFUNCTION() void OnFluxUpdated(ABuildingParent* BuildingUpdated);
	UFUNCTION() void OnBuildingOverclocked(ABuilding* BuildingOverclocked);
	UFUNCTION() void OnBuildingConstructed(ABuilding* BuildingConstructed);
	UFUNCTION() void CallInitSpawner();

public:	
	FSpawnerDelegate SpawnerActivated;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FAmalgamSpawnData AmalgamData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UUnitActorDataAsset* UnitActorDataAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	ABuildingParent* Building;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DefaultSpawnDelay;

	bool bUseMass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool Enabled = false;
	
	float BaseSpawnDelay = 1.5f;

	float SpawnDelay;
	float SpawnTimer;
	float SpawnDelayMult = 1;

	float StrengthMultiplier = 1;
		
	UPROPERTY(Replicated)
	AAmalgamSpawnerParent* MassSpawner;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<AUnitActorManager> UnitActorManagerClass;
	
	AUnitActorManager* UnitActorManager;

	UPROPERTY(EditAnywhere)
	TArray<FMassSpawnedEntityType> EntityTypes;

	UPROPERTY(EditAnywhere)
	TArray<FMassSpawnDataGenerator> SpawnDataGenerators;

	bool bCanBeActive;
	bool bShouldNotWait = true;
	bool bForceDisable = false;

	int FluxToSpawn = 0;
};
