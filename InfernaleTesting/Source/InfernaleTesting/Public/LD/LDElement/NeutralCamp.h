// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Component/ActorComponents/AttackComponent.h"
#include "Interfaces/Damageable.h"
#include "Interfaces/FluxTarget.h"
#include "Interfaces/Ownable.h"
#include "Interfaces/UnitTargetable.h"
#include "LD/LDElement/LDElement.h"
#include "Structs/SimpleStructs.h"
#include "NeutralCamp.generated.h"

class ADataGathererActor;
struct FAttackStruct;
class UAttacksDataAsset;
class UAttackComponent;
class AUnitActorManager;
struct FOwner;
class UEffectAfterDelayComponent;
class UDamageableComponent;
struct FNeutralCampRewardData;
class UNeutralCampDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCampAttackDelegate, FAttackHitInfo, AttackHitInfo);
/**
 * 
 */
UENUM()
enum class ECampTypeForData : uint8
{
	NeutralCamp,
	BigNeutralCamp,
	Boss
};

UCLASS()
class INFERNALETESTING_API ANeutralCamp : public ALDElement, public IOwnable, public IDamageable, public IFluxTarget, public IUnitTargetable
{
	GENERATED_BODY()

public:
	ANeutralCamp();

	UDamageableComponent* GetDamageableComponent() const;
	
	virtual FOwner GetOwner() override;
	virtual float DamageHealthOwner(const float DamageAmount, const bool bDamagePercent, const FOwner DamageOwner) override;
	virtual float GetTargetableRange() override;
	virtual bool IsAttackableBy(ETeam Team) override;
	virtual EEntityType GetEntityType() const override;
	virtual void Tick(float DeltaTime) override;
	
	virtual float GetOffsetRange();
	virtual float GetAttackOffsetRange();

	virtual void EnableTickForActor(bool bEnable);

protected:
	virtual void BeginPlay() override;
	virtual void RegisterLDElement() override;
	
	void SyncDataAsset();
	void StartRespawn();
	void DeathEffects(FOwner Depleter);
	void Respawn();
	void KillInGrid();
	void SpawnInGrid();
	void TryToGetUnitActorManager();
	
	UFUNCTION() void OnRespanwTimerFinished(AActor* _);
	UFUNCTION() void OnTimeRemaining(float TimeRemaining, float TotalTime);
	UFUNCTION() virtual void OnDeath(AActor* _, FOwner Depleter);
	UFUNCTION() void OnDamaged(AActor* _, float NewHealth, float DamageAmount);
	UFUNCTION() void InitAttacks();
	UFUNCTION() virtual bool OnAttackReady(FAttackStruct AttackStruct);
	UFUNCTION() virtual void CallOnAttackReady(FAttackStruct AttackStruct);
	UFUNCTION() virtual void OnPreLauchGame();
	UFUNCTION() virtual void OnLauchGame();

	UFUNCTION(NetMulticast, Reliable) void VisualRespawnTimeMulticast(float TimeRemaining, float TotalTime);
	UFUNCTION(NetMulticast, Reliable) void StartRespawningMulticast(float TimeRemaining, float TotalTime);
	UFUNCTION(NetMulticast, Reliable) void RespawnFinishedMulticast();
	UFUNCTION(NetMulticast, Reliable) void VisualDeathMulticast(FOwner Depleter);
	UFUNCTION(NetMulticast, Reliable) void VisualRespawnMulticast();
	UFUNCTION(NetMulticast, Reliable) void VisualDamagedMulticast(float NewHealth, float DamageAmount, float Percent);

	UFUNCTION(BlueprintImplementableEvent) void VisualRespawnTimeBP(float TimeRemaining, float TotalTime);
	UFUNCTION(BlueprintImplementableEvent) void VisualDeathBP(FOwner Depleter);
	UFUNCTION(BlueprintImplementableEvent) void VisualRespawnBP();
	UFUNCTION(BlueprintImplementableEvent) void VisualDamagedBP(float NewHealth, float DamageAmount, float Percent);

	FCampAttackDelegate AttackPerformed;
	UFUNCTION(NetMulticast, Reliable)
	void OnAttackPerformedMulticast(const FAttackHitInfo& AttackHitInfo);

	UFUNCTION(BlueprintImplementableEvent)
	void OnAttackPerformedBP(const FAttackHitInfo& AttackHitInfo);
	UFUNCTION()
	void OnAttackPerformed(FAttackHitInfo AttackHitInfo);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnDeathEffectVFXTrigger(FVector Location, FOwner Depleter);
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnCampRespawnVFXTrigger(FVector Location);

	UFUNCTION(BlueprintCallable, Category="NeutralCamp")
	void RotateStaticMeshesAroundPoint(TArray<UStaticMeshComponent*> StaticMeshes, FVector Location, float AngleRotationSpeed, float DeltaTime);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NeutralCamp")
	bool bIsDebugTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NeutralCamp")
	float MonsterPower = 0.f;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UDamageableComponent* DamageableComponent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UAttackComponent* AttackComponent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UEffectAfterDelayComponent* EffectAfterDelayComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data asset") UNeutralCampDataAsset* NeutralCampDataAsset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data asset") UAttacksDataAsset* AttacksDataAsset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data asset") bool bUseDataAsset = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings") TArray<FNeutralCampRewardData> NeutralCampRewards = TArray<FNeutralCampRewardData>();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings") float MaxHealth = 200.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings") float RespawnTime = 60.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings") float AttackOffsetRange = 600.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings") float TargetableRange = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings") TSubclassOf<AUnitActorManager> UnitActorManagerClass;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Neutral Camp") bool bStartSpawned = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Neutral Camp") bool bDebugTarget = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Neutral Camp") bool bDebugReplicationSpecific = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Neutral Camp") bool IsAlive = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Neutral Camp") bool WasAddedToGrid = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Neutral Camp") bool bCanRespawn = true;

	AUnitActorManager* UnitActorManager = nullptr;
	ADataGathererActor* DataGathererActor = nullptr;

	bool IsRespawning = false;
	float RespawnCurrentTime = 0.f;
	float RespawnMaxTime = 0.f;
	float LastReplication = 0.f;

	// Do not touch this (:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DataGatherer") ECampTypeForData CampType = ECampTypeForData::NeutralCamp;

	bool bShouldReplicateDamage = false;
	float NewHealthToReplicate = 0.f;
	float DamageAmountToReplicate = 0.f;
	float PercentToReplicate = 0.f;

	float DamageReplicationTime = -1.f;
	float TimeBetweenDamageReplication = 0.1f;
	

	bool bTickRequieredForDamage = false;
	bool bTickRequieredForRespawn = false;
};
