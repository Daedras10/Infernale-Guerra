// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Component/ActorComponents/DestroyedAfterDurationComponent.h"
#include "LD/Buildings/BuildingParent.h"
#include "Building.generated.h"


struct FAttackStruct;
class UAttackComponent;
class UBuildingEffectComponent;
class AMainBuilding;
class UConstructWithDelayComponent;
class UWidgetComponent;
class UDestroyedAfterDurationComponent;
class UDamageableComponent;
class USpawnerComponent;
class UBattleManagerComponent;
/**
 * 
 */
UCLASS()
class INFERNALETESTING_API ABuilding : public ABuildingParent
{
	GENERATED_BODY()

public:
	ABuilding();

	virtual void BeginPlay() override;
	
	void DestroyBuilding(const bool bDestroyedByUnit = true);
	void SetBuildingInfo(const FBuildingStruct& NewBuildingInfo, UTransmutationComponent* NewTransmutationComponent);
	void SetBreach(ABreach* Parent);
	UDestroyedAfterDurationComponent* GetDestroyedAfterDurationComponent() const;
	void SetPermanentFromServer(const bool IsPermanent);
	UFUNCTION(BlueprintCallable, BlueprintPure) bool IsFullyConstructed() const;
	bool IsPermanent() const;
	void SetOverclocked();
	UFUNCTION(BlueprintCallable, BlueprintPure) bool IsOverclocked();
	void UpdateOverclockDuration();
	void UpdateConstructionTime();

	void SetAttackOverclocked(const bool bOverclocked);

	float GetConstructionTime() const;

	virtual void InteractStartHover(APlayerControllerInfernale* Interactor) override;
	virtual void InteractEndHover(APlayerControllerInfernale* Interactor) override;
	virtual void InteractStartMain(APlayerControllerInfernale* Interactor) override;
	virtual bool CanCreateAFlux() const override;
	virtual TArray<TWeakObjectPtr<AFlux>> GetFluxes() override;
	virtual TArray<TWeakObjectPtr<AFlux>> GetLocalFluxes();

	virtual void ChangeOwner(FOwner NewOwner) override;
	virtual AMainBuilding* GetMainBuilding() override;
	virtual void OnSelectedForFluxCpp() override;
	virtual void OnDeselectedForFluxCpp() override;
	virtual float GetRepulsorRange() const override;

	UFUNCTION(BlueprintCallable)
	FBuildingStruct GetBuildingInfo() const;

	UBattleManagerComponent* GetBattleManager();

	UFUNCTION() void EndInteraction();
	UFUNCTION() void InteractionMousePrimary();

	void AddFlux(AFlux* Flux);
	void RemoveAllFluxes();
	USpawnerComponent* GetSpawnerComponent();

	UFUNCTION(BlueprintCallable, BlueprintPure) bool WasBuildingInfoSet() const;

protected:
	virtual void OnHealthDepleted() override;
	virtual void OnHealthDepletedOwner(AActor* Actor, FOwner Depleter) override;

	UFUNCTION() void OnBuildingDestroyedByTime(ABuilding* Building);
	UFUNCTION() void OnConstructionCompleted(AActor* Building);
	UFUNCTION() void OnConstructionTimeRemaining(float RemainingTime, float InitialDuration);
	UFUNCTION() void OnBreachOwnershipChanged(ABreach* BreachChanged, FOwner NewOwner, FOwner OldOwner);
	UFUNCTION() void OnAttackReady(FAttackStruct AttackStruct);

	void InitAttacks();
	
	UFUNCTION(NetMulticast, Reliable)
	void ConstructionTimeRemaining(const float RemainingTime, float InitialDuration, const bool shouldHide);

	UFUNCTION(BlueprintImplementableEvent)
	void OnConstructionTimeRemainingBP(const float RemainingTime, const float InitialDuration, const bool shouldHide);

	UFUNCTION(NetMulticast, Reliable) void LifeTimeRemainingMulticast(const float RemainingTime, float InitialDuration);

	UFUNCTION(BlueprintImplementableEvent) void OnLifeTimeRemainingBP(const float RemainingTime, const float InitialDuration);
	UFUNCTION(BlueprintImplementableEvent) void OverclickVisualsBP();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable) void InteractedBP(bool Interacted);
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable) void HoveredBP(bool bHovered);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void BlinkEffect();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void UpgradeBuildingVisuals();

	UFUNCTION(NetMulticast, Reliable) void SetPermanentMulticast(const bool IsPermanent);
	UFUNCTION(NetMulticast, Reliable) void SetOverclockedMulticast(const bool IsOverclocked);
	UFUNCTION(NetMulticast, Reliable) void OnBuildingConstructedMulticast(const bool bConstructed);
	UFUNCTION(NetMulticast, Reliable) void BuildingDestroyedMulticast();
	UFUNCTION(NetMulticast, Reliable) void ReplicateBuildingInfoMulticast(FBuildingStruct NewBuildingInfo);

	void SetLifeTime(bool KeepProgress) const;
	void SetConstructionTime(const float Duration, const float SpeedModifier, bool KeepProgress) const;
	void SetBuildingHealth(const float MaxHealth) const;
	void RemoveBuildingFromOwner();
	void AddBuildingToOwner();
	void CalculAndUpdateConstructionTime(bool KeepProgress);
	
	void BlinkTimer();

	UFUNCTION(BlueprintCallable) void SetPermanent(const bool IsPermanent);
	
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(BlueprintAssignable) FBuildingDelegate BuildingDestroyed;
	UPROPERTY(BlueprintAssignable) FBuildingDelegate BuildingStructUpdated;
	UPROPERTY(BlueprintAssignable) FBuildingDelegate BuildingOverclocked;
	UPROPERTY(BlueprintAssignable) FBuildingDelegate BuildingOverclockEnded;
	UPROPERTY(BlueprintAssignable) FBuildingDelegate BuildingMadePermanent;
	UPROPERTY(BlueprintAssignable) FBuildingDelegate BuildingConstructed;

	ABreach* Breach;

protected:
	UPROPERTY(EditAnywhere) UDestroyedAfterDurationComponent* DestroyedAfterDurationComponent;
	UPROPERTY(EditAnywhere) UConstructWithDelayComponent* ConstructWithDelayComponent;
	UPROPERTY(EditAnywhere) USpawnerComponent* SpawnerComponent;
	UPROPERTY(EditAnywhere) UWidgetComponent* ConstructionWidgetComponent;
	UPROPERTY(EditAnywhere) UBuildingEffectComponent* BuildingEffectComponent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UAttackComponent* AttackComponent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UCurveFloat* LifeTimeTransparencyCurve;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FBuildingStruct BuildingInfo = FBuildingStruct();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIsPermanent = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bCanCreateFlux = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float BlinkBaseDelay = 1.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings") UAttacksDataAsset* AttacksOverclockDataAsset;

	bool bIsBuildingInfoSet = false;
	float BlinkDelay = 0.2f;
	float BlinkingTime = 0.0f;
	
	bool bIsConstructed = false;
	bool bShouldBinkForLifeTime = false;
	bool bIsHovered = false;
	bool bOverclocked = false;
	bool bUseMass = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDebugHealth = false;


	FTimerHandle TimerHandle_Blink;
	APlayerControllerInfernale* LastInteractor;
};
