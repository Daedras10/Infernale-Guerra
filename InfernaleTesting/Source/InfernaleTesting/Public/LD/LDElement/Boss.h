// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NeutralCamp.h"
#include "GameMode/Infernale/GameModeInfernale.h"
#include "LeData/DataGathererActor.h"
#include "Boss.generated.h"

class AMainBuilding;
class UBoolProperty;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBossSpawned);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBossCaptured);

UCLASS()
class INFERNALETESTING_API ABoss : public ANeutralCamp
{
	GENERATED_BODY()

protected:

	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere, Category = "Boss")
	float SpawnCharge = 0.f;

public:
	// Sets default values for this actor's properties
	ABoss();

	UFUNCTION(Server, Reliable)
	void CaptureHintUpdateServer();

	UFUNCTION(BlueprintCallable, NetMulticast, Reliable, Category = "Boss")
	void CaptureHintUpdateReplicated();

	void CaptureBoss();
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Boss")
	void SetSpawnCharge(float Charge);

	void AskAddToSpawnCharge(float Charge);
	void AddToSpawnCharge(float Charge);

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "Boss")
	void AddToSpawnChargeMulticast(float Charge);

	UFUNCTION(BlueprintCallable, Category = "Boss")
	float GetSpawnCharge();

	UFUNCTION()
	void UpdateChargeAddedPerTick(float OldValue, float NewValue);

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Boss")
	void DebugBossCaptureRange();

	float GetSummonRange();

	virtual bool OnAttackReady(FAttackStruct AttackStruct) override;
	virtual bool IsAttackableBy(ETeam Team) override;
	virtual EEntityType GetEntityType() const override;
	virtual void EnableTickForActor(bool bEnable) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss")
	void UpdateCaptureBarBP(float Capture, const FVector& Vector);
protected:
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnDeath(AActor* _, FOwner Depleter) override;

	UFUNCTION(NetMulticast, Reliable) void OnAttackChangedMulticast(bool bAttackingVal);
	UFUNCTION(NetMulticast, Reliable) void OnAwakeChangedMulticast(bool bAwakeVal);
	UFUNCTION(NetMulticast, Reliable) void OnDeadChangedMulticast(bool bDeadVal);
	UFUNCTION(NetMulticast, Reliable) void OnCaptureCompletedMulticast(FOwner NewOwner);

	UFUNCTION(NetMulticast, Reliable) void UpdateCapturePlaneMaterialReplicated(float Capture, FVector ColorToVector);
	UFUNCTION(NetMulticast, Reliable) void UpdateCapturePlaneVisibilityReplicated(bool bVisible);

	UFUNCTION(BlueprintImplementableEvent) void OnAttackChangedBP(bool bAttackingVal);
	UFUNCTION(BlueprintImplementableEvent) void OnAwakeChangedBP(bool bAwakeVal);
	UFUNCTION(BlueprintImplementableEvent) void OnDeadChangedBP(bool bDeadVal);
	UFUNCTION(BlueprintImplementableEvent) void OnSpawnChargeChangedBP(float Charge);
	UFUNCTION(BlueprintImplementableEvent) void OnCaptureCompletedBP(FOwner NewOwner);

	void CaptureEffects(FOwner CaptureOwner);
	UFUNCTION() void OnCaptureCompleted(FOwner CaptureOwner);

	TArray<ETeam> GetTeamInCaptureRange();


public:
	UPROPERTY(BlueprintAssignable, Category = "Boss")
	FOnBossSpawned OnBossSpawned;

	UPROPERTY(BlueprintAssignable, Category = "Boss")
	FOnBossCaptured OnBossCaptured;

	float LastCaptureTickTime = 0.f;

	TArray<FOwner> OwnerForCapture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	UStaticMeshComponent* CapturePlaneMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	TMap<ETeam, float> TeamCaptureData = TMap<ETeam, float>();
	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere, Category = "Boss")
	bool bHasBeenSpawned = false;

	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere, Category = "Boss")
	bool bCaptured = false;

	UAnimInstance* BossAnimInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	bool bAwake = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	bool bAttacking = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	bool bDead = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	TArray<AMainBuilding*> LinkedMainBuildings;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	float ChargeAddedPerTick = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	float ChargePerKill;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Settings")
	bool bDebug = false;

	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere, Category = "Boss")
	bool bIsContested = false;
	
	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere, Category = "Boss")
	float MaxValue = 0.f;
	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere, Category = "Boss")
	ETeam MaxKey = ETeam::NatureTeam;

	bool bTickRequiredForBoss = false;
};