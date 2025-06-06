// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "Components/ActorComponent.h"
#include "Structs/SimpleStructs.h"
#include "BattleManagerComponent.generated.h"


class AAmalgamVisualisationManager;
struct FBattleInfo;
class AGameModeInfernale;
enum class EEntityType : uint8;
enum class EUnitTargetType : uint8;
class AUnitBattle;
class AUnitActorManager;


USTRUCT(Blueprintable)
struct FBattleInfoCode
{
	GENERATED_BODY()
public:
	FBattleInfoCode();

public:
	UPROPERTY() FVector2D BattlePositionAttackerWorld;
	UPROPERTY() FVector2D BattlePositionTargetWorld;
	UPROPERTY() EEntityType AttackerUnitType;
	UPROPERTY() EEntityType TargetUnitType;
	UPROPERTY() FOwner AttackerOwner;
	UPROPERTY() FOwner TargetOwner;
	UPROPERTY() FMassEntityHandle AttackerID;
	UPROPERTY() FMassEntityHandle TargetID;
	UPROPERTY() EUnitTargetType UnitTargetTypeTarget;
	UPROPERTY() TEnumAsByte<EAttackType> AttackType;
};


UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class INFERNALETESTING_API UBattleManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UBattleManagerComponent();

	void AtPosBattleInfo(FBattleInfoCode BattleInfoCode);
	void AtPosDeathInfo(FDeathInfo DeathInfo);
	void DoVFXBP(FBattleInfo BattleInfo);
	void DoDeathVFXBP(FDeathInfo DeathInfo);

	FBattleInfo BattleInfoCodeToBattleInfo(const FBattleInfoCode& BattleInfoCode);

	void OnLaunchGame();
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	//bool SearchForBattle(FVector2D BattlePositionWorld, AUnitBattle*& OutBattle);

	UFUNCTION(NetMulticast, Reliable) void DoVFXMulticast(FBattleInfo BattleInfo);
	UFUNCTION(NetMulticast, Reliable) void DoDeathVFXMulticast(FDeathInfo DeathInfo);

	UFUNCTION(BlueprintImplementableEvent, Category = "VFX") void DoVFXonBP(const FBattleInfo BattleInfo);
	UFUNCTION(BlueprintImplementableEvent, Category = "VFX") void DoDeathVFXonBP(const FDeathInfo DeathInfo);


protected:
	TWeakObjectPtr<AUnitActorManager> UnitActorManager;
	TArray<FBattleInfo> BattleInfosSinceLastCheck = TArray<FBattleInfo>();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle")
	float VFXChances = 0.05;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle")
	float VFXZoneChances = 0.05;

	bool Started = false;
	AGameModeInfernale* GameMode;
	AAmalgamVisualisationManager* AmalgamVisualisationManager;

	// float BattleCheckInterval = 1.0f;
	// float BattleCheckTimer = 0.f;

	//TArray<AUnitBattle*> Battles = TArray<AUnitBattle*>();
	// float BattleSearchRadius = 5000.f;
	// float BattleDefaultRadius = 500.f;
		
};
