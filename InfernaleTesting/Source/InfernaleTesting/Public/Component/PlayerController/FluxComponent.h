// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PlayerControllerComponent.h"
#include "Components/ActorComponent.h"
#include "Enums/Enums.h"
#include "Flux/Flux.h"
#include "FluxComponent.generated.h"

class AMainBuilding;
class AGameModeInfernale;
class UFluxSettingsDataAsset;
class AFlux;
class AFluxNode;
class ABuildingParent;

USTRUCT(Blueprintable)
struct FOriginsStuct
{
	GENERATED_BODY()
public:
	FOriginsStuct();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ABuildingParent* Building;
	
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFluxModeChanged, bool, bIsFluxMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FFluxModeRemoving, bool, bStarted, EFluxModeState, FluxModeState, float, MaxTimer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FFluxRemoving, EFluxModeState, FluxModeState, float, CurrentTimer, float, MaxTimer);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class INFERNALETESTING_API UFluxComponent : public UPlayerControllerComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UFluxComponent();
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable)
	void SetFluxMode(const bool bIsFluxMode);

	UFUNCTION(BlueprintCallable)
	bool IsFluxMode() const;

	void AddBuildingToSelectionForce(TWeakObjectPtr<ABuildingParent> Building);
	void RemoveBuildingFromSelectionForce(TWeakObjectPtr<ABuildingParent> Building);

	UFUNCTION(BlueprintCallable, BlueprintPure) AFluxNode* GetFluxNodeInteracted() const;
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	//virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	void FluxInitialEvents();
	void SyncWithDataAsset();
	void HoverFluxes();
	void RemoveFluxTimers(float DeltaTime);

	UFUNCTION() void OnFluxModeChanged();

	void FluxModeStart();
	void FluxModeEnd();
	void MoveStartNode(AFluxNode* FluxNode, AMainBuilding* MainBuilding, FVector HitPoint, bool bRelease);

	UFUNCTION() void OnPrimaryActionStarted();
	UFUNCTION() void OnPrimaryActionEnded();
	UFUNCTION() void OnSecondaryActionStarted();
	UFUNCTION() void OnSecondaryActionEnd();
	UFUNCTION() void OnEscapeStarted();
	UFUNCTION() void OnTransmutationModeChanged(bool bTransmutationMode);

	UFUNCTION() void OnSecondaryActionTriggered();
	void RemoveFluxTriggered();
	void RemoveFluxNodeTriggered();
	void UpdateMoveNodeOrigin();
	void UseOriginalNode(bool bActive);
	void MoveNodeSaved(AFluxNode* FluxNode, AFlux* Flux);

	bool GetPathfindingFrom(AFluxNode* FluxNode, const FVector Start, const FVector End, TArray<FPathStruct>& Pathfinding, bool bMousePosValid, const FVector HitPoint, const FVector OriginLoc, const float MaxDistance);

	UFUNCTION() void OnSecondaryActionEnded();
	UFUNCTION() void RemoveFlux();
	UFUNCTION() void RemoveFluxNode();
	UFUNCTION() void IsHoldingFluxCreation();
	UFUNCTION() void DragNDropSelectNode();
	UFUNCTION() void ClickNClickNode();

	UFUNCTION() void OnSecondaryActionStartedMoveNode();
	UFUNCTION() void OnSecondaryActionEndedMoveNode();
	UFUNCTION() void CleanMoveNodeEvents();
	UFUNCTION() void OnShiftModeChanged();

	UFUNCTION(Server, Reliable) void RemoveFluxServer(AFlux* Flux);
	UFUNCTION(Server, Reliable) void RemoveFluxNodeServer(AFluxNode* FluxNode);
	UFUNCTION(Server, Reliable) void SelectBuildingServer(ABuildingParent* Building);
	UFUNCTION(Server, Reliable) void DeselectBuildingServer(ABuildingParent* Building);
	UFUNCTION(Server, Reliable) void SelectFluxNodeServer(AFluxNode* FluxNode);
	UFUNCTION(Server, Reliable) void DeselectFluxNodeServer(AFluxNode* FluxNode);
	UFUNCTION(Server, Reliable) void UseOriginalNodeServer(bool bUseOriginal);
	UFUNCTION(Server, Reliable) void SetMoveSelectedNodeCanUseOtherServer(bool Value);
	UFUNCTION(Server, Reliable) void CreateNewFluxNodeServer(AFluxNode* FluxNode, FVector Target);
	
	UFUNCTION(Client, Reliable) void UseOriginalNodeOwner(bool bUseOriginal, AFluxNode* FluxNode);

	UFUNCTION() void MoveSelectedFluxNode(bool Release);
	UFUNCTION() void MoveSelectedFluxNodeNoRelease();
	UFUNCTION() void DragNDropFluxCreatedAndReplicated();

	UFUNCTION() void DragNDropOnNew();
	UFUNCTION() void ClickNClickOnNew();

	UFUNCTION(Server, Reliable) void MoveSelectedFluxNodeServer(AFluxNode* FluxNode, FVector Start, FVector End, bool Release);
	UFUNCTION(Server, Unreliable) void MoveSelectedFluxNodeServerUnreliable(AFluxNode* FluxNode, FVector Start, FVector End, bool Release);
	UFUNCTION(Server, Reliable) void RefreshSplinesForAmalgamsServer(AFluxNode* FluxNode);

	UFUNCTION() void ReleaseSelectedFluxNode();

	void MoveSelectedFluxNodeLocal(AFluxNode* FluxNode, FVector Start, FVector End, bool Release);
	
	void CreateNewFluxes();
	//void CreateFlux(ABuildingParent* Origin, AFluxNode* Destination);
	bool CreateHitResultFlux(FHitResult& HitResult);
	bool CreateHitResultFloor(FHitResult& HitResult);
	bool TryToSelectBuilding(FHitResult& HitResult);
	bool TryToSelectFluxNode(FHitResult& HitResult);
	bool TryToSelectFlux(FHitResult& HitResult);
	bool TryToHoverFlux(FHitResult& HitResult);
	bool TryToHoverFluxNode(FHitResult& HitResult);
	void ClearHoverFlux();
	void ClearHoverFluxNode();
	bool TryToFindFluxToRemove(FHitResult& HitResult, bool bSelectFlux = false);
	bool TryToFindFluxNodeToRemove(FHitResult& HitResult, bool bSelectFluxNode = false);
	FVector GetTargetPosition(FHitResult& HitResult, FVector PreviousLocation);
	FVector GetTargetPosition(FHitResult& HitResult, FVector PreviousLocation, AFluxNode* FluxNode, bool UseOriginal);

	bool IsRemovingMode();
	bool IsMovingNode();

	void FluxNodeCreatedCheck();

	/* Events */
	void AddEventsMoveSelectedFluxNode(const bool AllowToCreatePrevious, const bool DragNDrop);
	void RemoveEventsMoveSelectedFluxNode();
	void AddEventsNodeRemoval();
	void RemoveEventsNodeRemoval();
	void AddEventsFluxRemoval();
	void RemoveEventsFluxRemoval();

	void AddBuildingToSelection(TWeakObjectPtr<ABuildingParent> Building);
	void CalculateFluxTargets(AFluxNode* FluxNode, AFlux* Flux, TArray<float> Powers, bool Release);
	void ResetFluxTargets();

	UFUNCTION() void RemoveBuildingFromSelection(ABuildingParent* Building);
	void ClearBuildingSelection();

	void AddNodeToSelection(AFluxNode* FluxNode);

	UFUNCTION() void RemoveNodeFromSelection(AFluxNode* FluxNode);
	void ClearNodeSelection();

	void CreateFlux(ABuildingParent* Building, bool ShouldDisplay = true);
	void CreateFlux(ABuildingParent* Origin, TArray<FPathStruct> Path, bool ShouldDisplay);
	void CreateFluxOld(ABuildingParent* Building, FVector Target, float Offset); // Could be removed ? Check on next cleanup

	UFUNCTION(Server, Reliable) void CreateFluxServer(ABuildingParent* Building, FVector Start, FVector End, bool ShouldDisplay);
	UFUNCTION(Server, Reliable) void ContinueFluxServer(FVector Start, FVector End);
	void ContinueFlux();
	void ContinueFromFlux(AFlux* Flux, FVector Target, float Offset);

	UFUNCTION(Server, Reliable) void ClearFluxesServer();

	UFUNCTION() void OnFluxNodeCreated(AFlux* Flux, AFluxNode* FluxNode);
	UFUNCTION() void OnFluxNodeCreationFailed(AFlux* Flux);

	UFUNCTION() void OnFluxNodeCreatedWhileMoving(AFlux* Flux, AFluxNode* FluxNode);
	UFUNCTION() void OnFluxNodeCreationFailedWhileMoving(AFlux* Flux);

	UFUNCTION(Server, Reliable) void CreateFluxNodeServer(AFlux* Flux, FVector Target);
	UFUNCTION(Server, Reliable) void SpawnNodeRightBeforeServer(AFluxNode* FluxNodeToMove, bool RealNode, bool bIsTmp);
	UFUNCTION(Server, Reliable) void ClearFluxNodesCreateWhileMovingServer();

	UFUNCTION(NetMulticast, Reliable) void ReplicateFluxesMulticast(const TArray<AFlux*>& FluxesToReplicate);

	UFUNCTION(Client, Reliable) void ReplicateFluxCreatedOwner(AFlux* Flux);
	UFUNCTION(Client, Unreliable) void SetTargetsOwner(const TArray<AActor*>& TargetablesByFlux, const TArray<AActor*>& TargetablesAvailable, AFlux* Flux, const TArray<float>& FluxPowers);
	

public:
	FFluxModeChanged FluxModeChanged;
	FFluxModeRemoving FluxModeRemoving;
	FFluxRemoving FluxRemoving;

protected:
	/* Data assets */
	UFluxSettingsDataAsset* FluxSettingsDataAsset;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bUseDataAsset = true;

	/* Settings */
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float RemoveFluxDuration = 5.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float RemoveFluxNodeDuration = 1.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bRemoveMustStayOnFlux = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bMultiSelectionEnabled = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bCanCreateFluxesFromBases = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) TSubclassOf<AFlux> FluxSpawnClass;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float AnticipationInterval = 200;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float MinRangeBetweenNodes = 300.0f;

	/* Debugs */
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bDebugPathfinding = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bDebugNodeRemovalPathfinding = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bDebugReplication = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bDebugMoveNode = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bDebugFluxTargets = false;


	bool bFluxMode = false;
	bool bModeFluxRemoving = false;
	bool bHoverLocked = false;
	bool bInteractionsAllowed = true;
	bool bTryingToHoldPrimaryAction = false;
	bool bWaitingForCreation = false;
	bool bWaitingForHoldType = false;

	// Maybe not Arrays but wait to see
	TArray<ABuildingParent*> Origins = TArray<ABuildingParent*>();
	//TArray<AFluxNode*> FluxNodes = TArray<AFluxNode*>();
	UPROPERTY() TArray<AFlux*> Fluxes = TArray<AFlux*>(); //Replicated
	

	 //TODO: TWeakObjectPtr<AFlux>?

	/* Removal */
	TWeakObjectPtr<AFluxNode> FluxNodeToRemove;
	TWeakObjectPtr<AFlux> FluxToRemove;

	/* Interacted */
	TWeakObjectPtr<AFluxNode> FluxNodeInteracted;
	TWeakObjectPtr<AFlux> FluxInteracted;

	
	AFlux* FluxHovered = nullptr;
	TWeakObjectPtr<AFluxNode> FluxNodeOtherMoving = nullptr;
	int FluxNodeOtherMovingInitialIndex = -1;
	TArray<TWeakObjectPtr<AFluxNode>> FluxNodesCreateWhileMoving = TArray<TWeakObjectPtr<AFluxNode>>();
	AFluxNode* FluxNodeHovered = nullptr;
	AGameModeInfernale* GameModeInfernale; //TODO: TWeakObjectPtr<AFlux>?

	bool bWaitingForFluxReplication = false;

	/* Saved values in case we lose track of the mouse pos (in build mainly) */
	FVector LastVectorStart;
	FVector LastVectorEnd;

	/* Flux Removal */
	float RemoveFluxTimerElapsed = 0.0f;

	/* (Drag & drop timer) */
	FTimerHandle DragNDropTimer;
	float DragNDropTime = 0.15f;
	
	//ERemovingFluxMode RemovingFluxMode = ERemovingFluxMode::None; //Ideally removed in next cleanup and merged with EFluxModeState
	EFluxModeState FluxModeState = EFluxModeState::FMSNone;

	bool MoveNodeCanUseOriginalNode = false;
	bool UsingNodeAtOriginalPos = false;

	/* ShouldCalulateThisFrame */
	bool bShouldCalulateThisFrame = false;
	float ShouldCalulateThisFrameTimer = 0.f;
	float ShouldCalulateThisFrameTimerMax = 0.025f;


	//TODO Can be removed
	bool SecondaryActive = false;

	// Not used, check for usefullness and remove if we don't want to use it
	bool bShouldUpdateMoveTimer = false;
	float FluxUpdateTimer = 0.05f;
	float FluxUpdateInterval = 0.05f;
	bool bWasEverValidSaved = false;

	/* Saves MoveNode */
	FVector MoveTargetSaved;
	TArray<FPathStruct> BeforePathSaved;
	TArray<FPathStruct> AfterPathSaved;
	TArray<FPathStruct> OriginalToNodePathSaved;
	bool UseOriginalSaved;
};
