// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataAsset/UnitActorDataAsset.h"
#include "FogOfWar/FogOfWarComponent.h"
#include "GameFramework/Actor.h"
#include "Interfaces/Damageable.h"
#include "Interfaces/Ownable.h"
#include "Interfaces/UnitTargetable.h"
#include "Structs/SimpleStructs.h"
#include "UnitActor.generated.h"

class AUnitActorManager;
class ALDElement;
class AFogOfWarManager;
class ABuildingParent;
class AFlux;
class UAttackComponent;
class UDamageableComponent;



// USTRUCT(Blueprintable)
// struct FInfernaleUnitBuff
// {
// 	GENERATED_BODY()
//
// public:
// 	UPROPERTY(BlueprintReadWrite, EditAnywhere)
// 	EStat StatAffected;
//
// 	UPROPERTY(BlueprintReadWrite, EditAnywhere)
// 	float Value;
//
// 	UPROPERTY(BlueprintReadWrite, EditAnywhere)
// 	bool bIsPercent;
//
// 	//Duration??
// };


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUnitActorOwnerDelegate, AUnitActor*, UnitActor, FOwner, Owner);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUnitActorVectorDelegate, AUnitActor*, UnitActor, FVector, OldLocation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUnitActorStateDelegate, AUnitActor*, UnitActor, EUnitStateAsk, UnitStateAsk);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUnitActorDelegate, AUnitActor*, UnitActor);


UCLASS()
class INFERNALETESTING_API AUnitActor : public AActor, public IOwnable, public IDamageable, public IUnitTargetable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AUnitActor();
	
	virtual void Tick(float DeltaTime) override;
	virtual FOwner GetOwner() override;
	virtual void SetOwner(FOwner NewOwner) override;
	virtual void ChangeOwner(FOwner NewOwner) override;
	virtual float DamageHealthOwner(const float DamageAmount, const bool bDamagePercent, const FOwner DamageOwner) override;
	virtual float GetTargetableRange() override;

	void Init(FUnitStruct NewUnitStruct, FOwner NewOwner, TWeakObjectPtr<AFlux> FluxTarget, TWeakObjectPtr<AUnitActorManager> NewUnitActorManager);
	FUnitStruct GetUnitStruct() const;

	void SetTarget(TWeakObjectPtr<AActor> NewTarget, EUnitTargetType NewTargetType);
	void SetTargetToReplicate(TWeakObjectPtr<AActor> NewTarget, EUnitTargetType NewTargetType); // Only called on server
	TWeakObjectPtr<AActor> GetTarget() const;
	EUnitTargetType GetTargetType() const;
	TWeakObjectPtr<UFogOfWarComponent> GetFogOfWarComponent() const;
	void AddUnitForPathfindingRefresh();
	void UpdateMaxHealth(float NewMaxHealth);
	void UpdateSightFogOfWar(float NewSight);
	bool CanHaveAdditionalAttackers() const;
	bool IsAttacker(TWeakObjectPtr<AUnitActor> Attacker) const;
	void AddAttackers(TWeakObjectPtr<AUnitActor> Attacker);
	void RemoveAttackers(TWeakObjectPtr<AUnitActor> Attacker);
	bool IsGettingDestroyed() const;

	UFUNCTION(Server, Reliable) void SetReplicateMovementServer(bool bInReplicateMovement);

	UFUNCTION() void RefreshFluxPathOwnFlux();
	UFUNCTION() void AskRefreshFluxPath(AFlux* FluxTarget);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void RefreshPath(bool StartAtBeginning);
	void DestroyThisUnit();
	void MovementAI(float DeltaTime);
	void FollowPath(float DeltaTime);
	void AttackTarget(float DeltaTime);
	void CalculatePath(bool StartAtBeginning);
	void UpdateFogOfWar(bool bRemove = false);
	void SyncNiagaraActor();
	
	void TargetBuilding();
	void TargetUnit();
	void TargetNeutralCamp();
	void CreatePathArray(TWeakObjectPtr<AFlux> FluxTarget, bool StartAtBeginning);
	void CopyPathFromFlux(TWeakObjectPtr<AFlux> FluxTarget, bool StartAtBeginning);
	void RemoveLastTarget();

	//void ChangeTargetActor(TWeakObjectPtr<AUnitActor> NewTargetActor);
	UFUNCTION() void OnFluxNodeAdded(AFlux* FluxUpdated, int Index);
	UFUNCTION() void OnFluxNodeRemoved(AFlux* FluxUpdated, int Index);
	UFUNCTION() void OnUnitActorDestroyed(AUnitActor* UnitActor);
	UFUNCTION() void OnAttackersUnitActorDestroyed(AUnitActor* UnitActor);
	UFUNCTION() void OnBuildingParentDestroyed(ABuildingParent* Building);
	UFUNCTION() void OnNeutralCampRemoved(ALDElement* Element);
	UFUNCTION() void OnDamaged(AActor* Actor, float NewHealth, float DamageAmount);
	UFUNCTION() void OnHealthDepleted();
	UFUNCTION() void OnFluxDestroyed(AFlux* FluxDestroyed);
	
	UFUNCTION(NetMulticast, Reliable) void SetOwnerMulticast(FOwner NewOwner);
	UFUNCTION(NetMulticast, Reliable) void SetNiagaraActorMulticast(TSubclassOf<ANiagaraUnitAsActor> NiagaraActorClass);
	UFUNCTION(NetMulticast, Reliable) void OnDamagedMulticast(float NewHealth, float MaxHealth, float DamageAmount);
	UFUNCTION(NetMulticast, Reliable) void PreDestroyThisUnitMulticast();
	UFUNCTION(NetMulticast, Reliable) void SetReplicateMovementMulticast(bool bInReplicateMovement);
	//UFUNCTION(NetMulticast, Reliable) void SetMovementLocallyMulticast(bool bInMovementLocally);
	UFUNCTION(NetMulticast, Reliable) void ReplicatePathMulticast(FVector NewLocation, const TArray<FVector> &NewPath);
	UFUNCTION(NetMulticast, Reliable) void ReplicateCalculatePathMulticast(AFlux* FluxTarget, bool StartAtBeginning);
	UFUNCTION(NetMulticast, Reliable) void InitDoneMulticast();
	UFUNCTION(NetMulticast, Reliable) void SetUnitStructMulticast(FUnitStruct NewUnitStruct);
	UFUNCTION(NetMulticast, Reliable) void InitFogMulticast(FOwner NewOwner);
	UFUNCTION(NetMulticast, Reliable) void SetTargetMulticast(AActor* NewTarget, EUnitTargetType NewTargetType);
	UFUNCTION(NetMulticast, Reliable) void UpdateSightFogOfWarMulticast(float NewSight);

	UFUNCTION(BlueprintImplementableEvent)
	void OnDamagedBP(float NewHealth, float MaxHealth, float DamageAmount);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void RefreshVisualOwned();

	
public:
	FUnitActorOwnerDelegate OwnerChanged;
	FUnitActorDelegate UnitActorDestroyed;
	FUnitActorVectorDelegate UnitActorMoved;
	FUnitActorStateDelegate UnitStateAsk;

protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UDamageableComponent* DamageableComponent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UAttackComponent* AttackComponent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UFogOfWarComponent* FogOfWarComponent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TWeakObjectPtr<AActor> TargetActor;
	EUnitTargetType TargetType = EUnitTargetType::UTargetNone;

	bool bUseForgOfWar = false;
	TSubclassOf<AFogOfWarManager> FogOfWarManagerClass;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FUnitStruct UnitStruct;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FOwner OwnerInfo;
	TWeakObjectPtr<AFlux> Flux;
	
	TArray<FVector> Path = TArray<FVector>();
	TWeakObjectPtr<ANiagaraUnitAsActor> NiagaraActor;

	int CurrentSplineIndex = 0;

	bool bWasInit = false;
	bool bTargetIsValid = false;
	bool bNiagaraActorInit = false;

	float CheckRangeTimer = 0;
	float AttackTimer = 0;

	bool bFluxValid = false;
	bool bIsGettingDestroyed = false;
	//bool bShouldDoMovementLocally = false;
	
	float BaseCheckRangeTimer = 1.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TWeakObjectPtr<UStaticMeshComponent> DebugCube;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float SpaceBetweenPathfindingNodes = 200.0f;

	TArray<TWeakObjectPtr<AUnitActor>> Attackers = TArray<TWeakObjectPtr<AUnitActor>>();
	TWeakObjectPtr<UTransmutationComponent> TransmutationComponent;
	TWeakObjectPtr<AUnitActorManager> UnitActorManager;
};