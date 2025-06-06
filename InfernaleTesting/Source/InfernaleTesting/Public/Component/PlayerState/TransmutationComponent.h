// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Component/PlayerController/PlayerControllerComponent.h"
#include "Components/ActorComponent.h"
#include "DataAsset/TransmutationDataAsset.h"
#include "TransmutationComponent.generated.h"


struct FTransmutationNode;
class UTransmutationDataAsset;

USTRUCT(Blueprintable)
struct FTransmutationNodeOwned
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere) FTransmutationNode TransmutationNode;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bOwned = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bActivating = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FTimerHandle TimerHandle;
	FTimerDelegate TimerDel;
};

USTRUCT(Blueprintable)
struct FTransmutationQueueElement
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere) FTransmutationNode TransmutationNode;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) int Index = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool Activating = false;
};

USTRUCT(Blueprintable)
struct FTransmutationQueue
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere) TArray<FTransmutationQueueElement> Queue = TArray<FTransmutationQueueElement>();
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool IsProcessing = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FString CurrentNodeID = "";

	bool IsInQueue(const FString NodeId);
	void AddToQueue(const FTransmutationQueueElement Node);
	void RemoveFromQueue(const FString NodeId);
};


USTRUCT(Blueprintable)
struct FTransmutationSimpleNodeOwned
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere) FTransmutationSimpleNode TransmutationNode;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) int Level = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) int LevelTarget = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bLevelingUp = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FTimerHandle TimerHandle;
	FTimerDelegate TimerDel;
};

USTRUCT(Blueprintable)
struct FSimpleTransmutationQueueElement
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere) FTransmutationSimpleNode TransmutationNode;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) int Index = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) int LevelToActivate = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool Activating = false;
};

USTRUCT(Blueprintable)
struct FSimpleTransmutationQueue
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere) TArray<FSimpleTransmutationQueueElement> Queue = TArray<FSimpleTransmutationQueueElement>();
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool IsProcessing = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FString CurrentNodeID = "";

	bool IsInQueue(const FString NodeId, int LevelTarget);
	void AddToQueue(const FSimpleTransmutationQueueElement Node);
	void RemoveFromQueue(const FString NodeId, int LevelTarget);
};



DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTransmutationMode, bool, bTransmutationMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTransmutationNodes, TArray<FTransmutationNodeOwned>, TransmutationNodesOwned);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTransmutationNodeString, const FString&, NodeID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTransmutationNodeStringQueue, const FString&, NodeID, FTransmutationQueue, Queue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTransmutationNodeStringSimpleQueue, const FString&, NodeID, FSimpleTransmutationQueue, Queue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTransmutationNodeQueue, FTransmutationQueue, Queue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTransmutationSimpleNodeQueue, FSimpleTransmutationQueue, Queue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTransmutationEffectAltered, ENodeEffect, Effect);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTransmutationNodeEffects, TArray<ENodeEffect>, Effects, UTransmutationComponent*, TransmutationComponent);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class INFERNALETESTING_API UTransmutationComponent : public UPlayerControllerComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTransmutationComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	UFUNCTION(BlueprintCallable, BlueprintPure) float GetCostNextNode(const ETransmutationNodeType NodeType) const;
	UFUNCTION(BlueprintCallable, BlueprintPure) float GetCostCurrentNode(const ETransmutationNodeType NodeType) const;
	UFUNCTION(BlueprintCallable, BlueprintPure) FTransmutationEffects GetEffect(const ENodeEffect NodeEffect) const;
	UFUNCTION(BlueprintCallable, BlueprintPure) float GetSimplePercent(const ENodeEffect NodeEffect) const;

	/* Values */
	UFUNCTION(BlueprintCallable, BlueprintPure) float ApplyEffect(float InitialValue, const ENodeEffect NodeEffect) const;
	UFUNCTION(BlueprintCallable, BlueprintPure) float ApplyEffectAtIndex(float InitialValue, int Index) const;
	UFUNCTION(BlueprintCallable, BlueprintPure) float GetEffectDamageToBuilding(float InitialValue) const;
	UFUNCTION(BlueprintCallable, BlueprintPure) float GetEffectDamageToMonster(float InitialValue) const;
	UFUNCTION(BlueprintCallable, BlueprintPure) float GetEffectDamageToUnit(float InitialValue) const;
	UFUNCTION(BlueprintCallable, BlueprintPure) float GetEffectHealthUnit(float InitialValue) const;
	UFUNCTION(BlueprintCallable, BlueprintPure) float GetEffectHealthBuilding(float InitialValue) const;
	UFUNCTION(BlueprintCallable, BlueprintPure) float GetEffectFluxRange(float InitialValue) const;
	UFUNCTION(BlueprintCallable, BlueprintPure) float GetEffectUnitSight(float InitialValue) const;
	UFUNCTION(BlueprintCallable, BlueprintPure) float GetEffectUnitSpeed(float InitialValue) const;
	UFUNCTION(BlueprintCallable, BlueprintPure) float GetEffectBuildingSight(float InitialValue) const;
	UFUNCTION(BlueprintCallable, BlueprintPure) float GetEffectBuildingRecycleSouls(float InitialValue) const;
	UFUNCTION(BlueprintCallable, BlueprintPure) float GetEffectBuildingOverclockDuration(float InitialValue) const;
	UFUNCTION(BlueprintCallable, BlueprintPure) float GetEffectBuildingConstructionTime(float InitialValue) const;
	UFUNCTION(BlueprintCallable, BlueprintPure) float GetEffectBuildingConstructionCost(float InitialValue) const;

	UFUNCTION() void ToogleTransmutation();

	UFUNCTION(BlueprintCallable) void BuyNode(const FString& NodeID);
	UFUNCTION(BlueprintCallable) void BuySimpleNode(const FString& NodeID);
	UFUNCTION(BlueprintCallable) void RefundNode(const FString& NodeID);
	UFUNCTION(BlueprintCallable) FTransmutationNodeOwned GetNode(const FString& NodeID) const;
	UFUNCTION(BlueprintCallable) TArray<float> GetSimpleNodesCost() const;

	UFUNCTION(BlueprintCallable) int32 GetUpdateID() const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void SyncWithDataAsset();
	void StartTransmutation();
	void EndTransmutation();
	void ActivateNode(const FString& NodeID);
	void ActivateSimpleNode(const FString& NodeID, const int LevelTarget);
	void DeactivateNode(const FString& NodeID);
	void TryStartNextNodeInQueue();
	void TryStartNextNodeInSimpleQueue();
	void InitEffect(ENodeEffect NodeEffect);
	void InitSimpleEffect(ENodeEffect NodeEffect, UCurveFloat* Curve);
	void ConsiderEffect(const ENodeEffect NodeEffect);
	
	int CountCurrentNodesOfType(const ETransmutationNodeType NodeType) const;

	UFUNCTION() void NodeActivateTimerDone(const FString& NodeID, bool Activating);
	UFUNCTION() void SimpleNodeActivateTimerDone(const FString& NodeID, int LevelTarget);
	UFUNCTION() void OnPreLaunchGame();
	
	UFUNCTION(Server, Reliable) void BuyNodeServer(const FString& NodeID);
	UFUNCTION(Server, Reliable) void BuySimpleNodeServer(const FString& NodeI);
	UFUNCTION(Server, Reliable) void RefundNodeServer(const FString& NodeID);

	UFUNCTION(NetMulticast, Reliable) void ActivateNodeMulticast(const FString& NodeID, const bool bActivate, const bool bOwned, const FTransmutationQueue Queue);
	UFUNCTION(NetMulticast, Reliable) void ActivateSimpleNodeMulticast(const FString& NodeID, const int LevelReached, const FSimpleTransmutationQueue Queue);
	UFUNCTION(NetMulticast, Reliable) void UpdateEffectMulticast(const FTransmutationEffects Effect);
	UFUNCTION(NetMulticast, Reliable) void UpdateSimpleEffectMulticast(const FTransmutationSimpleEffects Effect);

	UFUNCTION(Client, Reliable) void TransmutationRefusedOwning(const FString& NodeID);

public:
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FTransmutationMode TransmutationMode;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FTransmutationNodes TransmutationNodesSet;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FTransmutationNodeStringQueue NodeOwnedAltered;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FTransmutationNodeQueue TransmutationQueueAltered;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FTransmutationNodeEffects NodeOwnedOwnerShipAltered;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FTransmutationEffectAltered TransmutationEffectAltered;

	
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FTransmutationNodeEffects SimpleNodeOwnedOwnerShipAltered;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FTransmutationNodeStringSimpleQueue SimpleNodeOwnedAltered;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FTransmutationSimpleNodeQueue SimpleTransmutationQueueAltered;

	UPROPERTY(BlueprintAssignable) FTransmutationNodeString TransmutationRefused;

protected:
	UTransmutationDataAsset* TransmutationDataAsset; /* Data asset initialise by the one on GameSettings */

	UPROPERTY(BlueprintReadWrite, EditAnywhere) TArray<FTransmutationNode> TransmutationNodes = TArray<FTransmutationNode>();
	UPROPERTY(BlueprintReadWrite, EditAnywhere) TArray<FTransmutationNodeOwned> TransmutationNodesOwned = TArray<FTransmutationNodeOwned>();
	UPROPERTY(BlueprintReadWrite, EditAnywhere) TArray<FTransmutationEffects> TransmutationEffects = TArray<FTransmutationEffects>();

	UPROPERTY(BlueprintReadWrite, EditAnywhere) TArray<FTransmutationSimpleNode> TransmutationSimpleNodes = TArray<FTransmutationSimpleNode>();
	UPROPERTY(BlueprintReadWrite, EditAnywhere) TArray<FTransmutationSimpleNodeOwned> TransmutationSimpleNodesOwned = TArray<FTransmutationSimpleNodeOwned>();
	UPROPERTY(BlueprintReadWrite, EditAnywhere) TArray<FTransmutationSimpleEffects> TransmutationSimpleEffects = TArray<FTransmutationSimpleEffects>();
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Settings") bool bUseComplexeEffects = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Debugs") bool bDebug = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Debugs") bool bDebugSimpleEffect = false;

	
	bool bTransmutationMode = false;
	FTransmutationSettings TransmutationSettings;
	FTransmutationQueue TransmutationQueue;
	FSimpleTransmutationQueue SimpleTransmutationQueue;

	int32 UpdateID = 0;

public:
	TArray<FString> GetActiveTransmutations();
};
