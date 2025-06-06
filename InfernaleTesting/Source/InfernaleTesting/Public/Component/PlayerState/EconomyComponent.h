// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PlayerStateComponent.h"
#include "Components/ActorComponent.h"
#include "DataAsset/EconomyDataAsset.h"
#include "Structs/SimpleStructs.h"
#include "EconomyComponent.generated.h"


enum class ETransmutationNodeType : uint8;
class UTransmutationDataAsset;
struct FBuildingStruct;
class ABuilding;
class AMainBuilding;
class UEconomyDataAsset;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSoulsValueChanged, float, Souls, float, LastSouls);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBuildingCostChanged, UEconomyComponent*, EconomyComponent);



UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class INFERNALETESTING_API UEconomyComponent : public UPlayerStateComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UEconomyComponent();

	UFUNCTION(BlueprintCallable) float GetSouls() const;
	UFUNCTION(BlueprintCallable) float GetSoulsDisplayed() const;
	UFUNCTION(BlueprintCallable) float GetNodeCost(ETransmutationNodeType NodeType) const;
	
	void AddSouls(AActor* Source, ESoulsGainCostReason SoulsGainReason, const float Amount);
	void RemoveSouls(const float Amount, const bool bInstantVisual = false);
	void SetSouls(const float Amount);
	void AddBaseBuilding(AMainBuilding* Building);
	void RemoveBaseBuilding(AMainBuilding* Building);
	void AddBuilding(ABuilding* Building);
	void RemoveBuilding(ABuilding* Building);
	void GainDebugIncome();
	void GainCheatIncome(const float Amount);

	UFUNCTION(Client, Reliable) void ReplicateSoulsClient(const float Amount);
	
	UFUNCTION(Server, Reliable) void AddBaseBuildingServer(AMainBuilding* Building);
	UFUNCTION(Server, Reliable) void RemoveBaseBuildingServer(AMainBuilding* Building);

	UFUNCTION(BlueprintCallable) float GetBuildingCostWithMultiplier(const ESoulsGainCostReason SoulsGainReason, const FBuildingStruct BuildingStruct) const;

	UFUNCTION(BlueprintCallable, BlueprintPure) float GetNegativeSoulsAllowed() const;
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	float GetCostWithMultiplier(const ESoulsGainCostReason SoulsGainReason) const;
	float GetCostMultiplier(const ESoulsGainCostReason SoulsGainReason) const;

	float GetBuildingBuildCost(const FBuildingStruct BuildingStruct) const;
	float GetBuildingUpgradeCost(const FBuildingStruct BuildingStruct) const;

	UFUNCTION(Server, Reliable) void AddSoulsServer(AActor* Source, ESoulsGainCostReason SoulsGainReason, const float Amount);
	UFUNCTION(Server, Reliable) void RemoveSoulsServer(const float Amount, const bool bInstantVisual = false);
	UFUNCTION(Server, Reliable) void SetSoulsServer(const float Amount);
	UFUNCTION(Server, Reliable) void GainDebugIncomeServer();
	UFUNCTION(Server, Reliable) void GainCheatIncomeServer(const float Amount);

	UFUNCTION() void OnSoulsChanged(const float NewSouls, const float LastSouls);
	UFUNCTION() void OnLaunchGame();
	UFUNCTION() void RetryBeginPlay();

	void GainIncome();
	void GainBaseIncome();

public:
	UPROPERTY(BlueprintAssignable) FSoulsValueChanged SoulsValueChanged;
	UPROPERTY(BlueprintAssignable) FSoulsValueChanged SoulsGained;
	UPROPERTY(BlueprintAssignable) FSoulsValueChanged SoulsLost;
	UPROPERTY(BlueprintAssignable) FSoulsValueChanged DisplaySoulsValueChange;
	UPROPERTY(BlueprintAssignable) FBuildingCostChanged BuildingCostChanged; // Cost multiplier changed

	// DataGatherer
	float BuildingIncome = 0;
	float SoulsBeaconIncome = 0;
	
protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UEconomyDataAsset* EconomyDataAsset;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float Souls = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bDebugSouls = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bDebugSoulsDisplayed = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float LerpSpeed = 3;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float IncomeDelay = 20;

	TArray<AMainBuilding*> MainBuildings = TArray<AMainBuilding*>();
	float SoulsDisplayed = 0;
	
	TArray<ABuilding*> Buildings = TArray<ABuilding*>();
	FSoulsGainCostValues BaseIncome;
	FTimerHandle BaseIncomeTimer;
	FTimerHandle IncomeHandle;
	bool bIsPrinting = false;

};
