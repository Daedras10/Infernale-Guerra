// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PlayerControllerComponent.h"
#include "Component/PlayerState/TransmutationComponent.h"
#include "Components/ActorComponent.h"
#include "UIComponent.generated.h"


enum class EVictoryPointReason : uint8;
enum EFluxModeState : uint8;
class AFlux;
class ASoulBeacon;
enum class ETransmutationNodeType : uint8;
struct FTransmutationSettings;
struct FTransmutationNodeOwned;
enum ERemovingFluxMode : uint8;
enum class ETeam : uint8;
struct FTimeRemaining;




UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class INFERNALETESTING_API UUIComponent : public UPlayerControllerComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UUIComponent();
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void CreateTransmutationNodes(const TArray<FTransmutationNodeOwned>& TransmutationNodesOwned, FTransmutationSettings TransmutationSettings);
	void CreateTransmutationSimpleNodes(const TArray<FTransmutationSimpleNodeOwned>& TransmutationNodesOwned, FTransmutationSettings TransmutationSettings);
	void FluxEntityHovered(const EFluxHoverInfoType InfoType) const;

	void SoulBeaconSelected(ASoulBeacon* Beacon, bool Selected);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable) void SoulsCostHovered(const bool Hovered, const float Cost) const;
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable) void RemoveLoadingScreen(bool bRemove) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable) void CreateLoadingUIBP();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable) void OnTransmutationNodeStartUnlockVFX(const FString& NodeID) const;
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable) void OnTransmutationNodeUnlockVFX(const FString& NodeID) const;
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable) void OnTransmutationNodeEnqueueVFX(const FString& NodeID) const;
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable) void OnEnoughSoulsForTransmutationVFX() const;
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable) void OnNotEnoughSoulsForTransmutationVFX() const;

	UFUNCTION(BlueprintPure, BlueprintCallable) bool IsTransmutationVisible() const;
	UFUNCTION(BlueprintCallable) void SetTransmutationVisible(const bool bVisible);

	UFUNCTION(BlueprintPure, BlueprintCallable) bool ISConstructionMenuVisible() const;
	UFUNCTION(BlueprintCallable) void SetConstructionMenuVisible(const bool bVisible);

	UFUNCTION(BlueprintPure, BlueprintCallable) bool ShouldCloseSomeMenus() const;
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable) void TryCloseMenu() const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	
	void SubscribeToAuthorityEvents();
	void SubscribeToLocalEvents();
	void CheckTimeForNotification(const FTimeRemaining TimeRemaining);
	FVictoryInfoMap MakeVictoryInfoMap(const FVictoryPoints VictoryPoints, const FDeadTeam DeadPlayers);
	bool FVictoryInfoMapValidity(const FVictoryInfoMap& VictoryInfoMap) const;

	UFUNCTION() void OnTimeRemaining(const FTimeRemaining TimeRemaining);
	UFUNCTION() void OnVictoryPointChanged(const FVictoryPoints VictoryPoints, const EVictoryPointReason VictoryPointReason, const ETeam TeamPointChanged, int Score);
	UFUNCTION() void OnFluxModeRemoving(bool Start, EFluxModeState FluxModeState, float Duration);
	UFUNCTION() void OnFluxRemoving(EFluxModeState FluxModeState, float CurrentTimer, float MaxTimer);
	UFUNCTION() void OnTransmutationMode(const bool bTransmutationMode);
	UFUNCTION() void OnNodeOwnedAltered(const FString& NodeID, const FTransmutationQueue TransmutationQueue);
	UFUNCTION() void OnSimpleNodeOwnedAltered(const FString& NodeID, const FSimpleTransmutationQueue TransmutationQueue);
	UFUNCTION() void OnTransmutationQueueAltered(const FTransmutationQueue TransmutationQueue);
	UFUNCTION() void OnSimpleTransmutationQueueAltered(const FSimpleTransmutationQueue TransmutationQueue);
	UFUNCTION() void OnAllPlayersReady();
	UFUNCTION() void OnLocalPlayerLost(const FVictoryPoints VictoryPoints, TArray<ETeam> DeadPlayers, ETeam Team);
	UFUNCTION() void OnGameEnded(const FVictoryPoints VictoryPoints, TArray<ETeam> DeadPlayers, ETeam Team);
	UFUNCTION()	void OnLaunchSequence(const FVictoryPoints VictoryPoints, TArray<ETeam> Teams, ETeam Team);

	UFUNCTION(NetMulticast, Reliable) void CreateTransmutationNodesMulticast(const TArray<FTransmutationNodeOwned>& TransmutationNodesOwned, FTransmutationSettings TransmutationSettings);
	UFUNCTION(NetMulticast, Reliable) void CreateTransmutationSimpleNodesMulticast(const TArray<FTransmutationSimpleNodeOwned>& TransmutationNodesOwned, FTransmutationSettings TransmutationSettings);

	UFUNCTION(Client, Reliable) void UpdateTimeRemainingClient(const FTimeRemaining TimeRemaining);
	UFUNCTION(Client, Reliable) void OnVictoryPointChangedClient(const FVictoryPoints VictoryPoints, const EVictoryPointReason VictoryPointReason, const ETeam TeamPointChanged,int Score);
	UFUNCTION(Client, Reliable) void OnLocalPlayerLostClient(const FVictoryPoints VictoryPoints, const FDeadTeam DeadPlayers);
	UFUNCTION(Client, Reliable) void OnGameEndedClient(const FVictoryPoints VictoryPoints, const FDeadTeam DeadPlayers);
	UFUNCTION(Client, Reliable) void OnLaunchSequenceClient(const FVictoryPoints& VictoryPoints, const FDeadTeam& DeadTeam, ETeam Team);
	UFUNCTION(Client, Reliable) void OnTransmutationQueueAlteredClient(const FTransmutationQueue TransmutationQueue);
	UFUNCTION(Client, Reliable) void OnSimpleTransmutationQueueAlteredClient(const FSimpleTransmutationQueue TransmutationQueue);
	UFUNCTION(Client, Reliable) void OnAllPlayersSpawnedClient(int Players);
	UFUNCTION(Client, Reliable) void SoulBeaconSelectedClient(ASoulBeacon* Beacon, bool Selected);


	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable) void UpdateTimeRemainingBP(const FTimeRemaining TimeRemaining);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable) void OnVictoryPointChangedBP(FVictoryInfoMap VictoryInfo, const EVictoryPointReason VictoryPointReason, const ETeam TeamPointChanged, int Score);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable) void OnLocalPlayerLostBP(FVictoryInfoMap VictoryInfo);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable) void OnGameEndedBP(FVictoryInfoMap VictoryInfo);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable) void OnLaunchSequenceBP(const FVictoryPoints& VictoryPoints, ETeam Team);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable) void SoulBeaconSelectedBP(ASoulBeacon* Beacon, bool Selected);

	UFUNCTION(BlueprintImplementableEvent) void OnFluxModeRemovingBP(bool Start, EFluxModeState FluxModeState, float Duration);
	UFUNCTION(BlueprintImplementableEvent) void OnFluxRemovingBP(EFluxModeState FluxModeState, float CurrentTimer, float MaxTimer);
	UFUNCTION(BlueprintImplementableEvent) void OnTransmutationModeBP(const bool bTransmutationMode);
	UFUNCTION(BlueprintImplementableEvent) void CreateTransmutationNodesBP(const TArray<FTransmutationNodeOwned>& TransmutationNodesOwned, FTransmutationSettings TransmutationSettings);
	UFUNCTION(BlueprintImplementableEvent) void CreateTransmutationSimpleNodesBP(const TArray<FTransmutationSimpleNodeOwned>& TransmutationNodesOwned, FTransmutationSettings TransmutationSettings);
	UFUNCTION(BlueprintImplementableEvent) void OnNodeOwnedAlteredBP(const FString& NodeID, const FTransmutationQueue TransmutationQueue);
	UFUNCTION(BlueprintImplementableEvent) void OnSimpleNodeOwnedAlteredBP(const FString& NodeID, const FSimpleTransmutationQueue TransmutationQueue);
	UFUNCTION(BlueprintImplementableEvent) void OnTransmutationQueueAlteredBP(const FTransmutationQueue TransmutationQueue);
	UFUNCTION(BlueprintImplementableEvent) void OnSimpleTransmutationQueueAlteredBP(const FSimpleTransmutationQueue TransmutationQueue);
	UFUNCTION(BlueprintImplementableEvent) void FluxEntityHoveredBP(EFluxHoverInfoType InfoType) const;
	UFUNCTION(BlueprintImplementableEvent) void OnAllPlayersSpawnedBP(int Players) const;

protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bHoveredCost = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bDebugAnonymScores = true;

	bool bIsTransmutationVisible = false;
	bool bConstructionMenuVisible = false;
};
