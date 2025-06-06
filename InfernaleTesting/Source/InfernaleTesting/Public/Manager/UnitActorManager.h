// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataAsset/TransmutationDataAsset.h"
#include "GameFramework/Actor.h"
#include "UnitAsActor/UnitActor.h"
#include "UnitActorManager.generated.h"


class AMainBuilding;
class UBattleManagerComponent;
class IFluxRepulsor;
class UPathfindingGridComponent;
struct FUnitActorGridCell;
enum class EAttackerType : uint8;
struct FAttackStruct;
class UUnitActorGridComponent;
class AUnitActor;
class ABuildingParent;
class UUnitActorDataAsset;

USTRUCT()
struct FUnitActors
{
	GENERATED_BODY()

	TArray<TWeakObjectPtr<AUnitActor>> UnitActors = TArray<TWeakObjectPtr<AUnitActor>>();
};

USTRUCT()
struct FBuildings
{
	GENERATED_BODY()

	TArray<TWeakObjectPtr<ABuildingParent>> Buildings = TArray<TWeakObjectPtr<ABuildingParent>>();
};


UCLASS()
class INFERNALETESTING_API AUnitActorManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AUnitActorManager();

	virtual void Tick(float DeltaTime) override;
	virtual bool ShouldTickIfViewportsOnly() const override;

	void BlueprintEditorTick(float DeltaTime);

	void SpawnUnit(TWeakObjectPtr<ABuildingParent> Building);
	TWeakObjectPtr<UUnitActorGridComponent> GetGridComponent() const;
	bool Attack(TWeakObjectPtr<AActor> Attacker, FAttackStruct AttackStruct, EAttackerType AttackerType);
	FTransmutationEffects GetEffect(const ENodeEffect NodeEffect, FOwner EffectOwner) const;
	float ApplyEffect(float InitialValue, const ENodeEffect NodeEffect, FOwner EffectOwner) const;
	void AddUnitToPathfind(TWeakObjectPtr<AUnitActor> UnitActor);
	TMap<EPlayerOwning, FUnitActors>& GetUnitActorsByPlayers();
	TWeakObjectPtr<UTransmutationComponent> GetTransmutationComponent(EPlayerOwning Player) const;
	TWeakObjectPtr<UBattleManagerComponent> GetBattleManagerComponent() const;
	TArray<IFluxRepulsor*> GetRepulsorsInRange(FVector Location, float Range);
	TArray<AMainBuilding*> GetMainBuildings() const;

	UFUNCTION(NetMulticast, Reliable) void DrawDebugLineMulticast(FVector Start, FVector End, FColor Color, float LifeTime, float Thickness);
	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void SyncWithDataAsset();
	void CheckUnitRange(TWeakObjectPtr<AUnitActor> UnitActor);
	void CheckUnitAttack(TWeakObjectPtr<AUnitActor> UnitActor);
	FUnitActorGridCell GetAllInRange(FVector Location, float Range, float Angle, FVector Forward, bool bDebug);
	void HandleEffectChanged(EPlayerOwning Player, ENodeEffect Effect, UTransmutationComponent* TransmutationComponent);

	void UpdateHealthUnit(EPlayerOwning Player, UTransmutationComponent* TransmutationComponent);
	void UpdateHealthBuilding(EPlayerOwning Player, UTransmutationComponent* TransmutationComponent);
	void UpdateSightUnit(EPlayerOwning Player, UTransmutationComponent* TransmutationComponent);
	void UpdateSightBuilding(EPlayerOwning Player, UTransmutationComponent* TransmutationComponent);
	void UpdateConstructionTimeBuilding(EPlayerOwning Player, UTransmutationComponent* TransmutationComponent);
	void UpdateOverclockDurationBuilding(EPlayerOwning Player, UTransmutationComponent* TransmutationComponent);
	

	UFUNCTION() void OnUnitStateAsk(AUnitActor* UnitActor, EUnitStateAsk UnitStateAsk);
	UFUNCTION() void OnPreLaunchGame();
	UFUNCTION() void OnLaunchGame();
	UFUNCTION() void OnNodeOwnedOwnerShipAltered(TArray<ENodeEffect> Effects, UTransmutationComponent* TransmutationComponent);

protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UUnitActorGridComponent* GridComponent;
	
	// UPROPERTY(BlueprintReadWrite, EditAnywhere)
	// TWeakObjectPtr<UPathfindingGridComponent> PathfindingGridComponent;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UBattleManagerComponent* BattleManagerComponent;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UUnitActorDataAsset* UnitActorDataAsset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bUseDataAsset = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bUseEditorTick = true;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<AMainBuilding*> MainBuildings;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FUnitStruct> UnitStructs = TArray<FUnitStruct>();

	TMap<EPlayerOwning, UTransmutationComponent*> TransmutationComponents = TMap<EPlayerOwning, UTransmutationComponent*>();
	TArray<TWeakObjectPtr<AUnitActor>> UnitActorsToSync = TArray<TWeakObjectPtr<AUnitActor>>();

	TMap<EPlayerOwning, FUnitActors> UnitActorsByPlayers = TMap<EPlayerOwning, FUnitActors>();
};
