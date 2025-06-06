// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FluxNode.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Actor.h"
#include "Interfaces/Interactable.h"
#include "Flux.generated.h"

struct FPathStruct;
struct FOwner;
class UFluxSettingsDataAsset;
class USplineComponent;
class ABuildingParent;
class AFluxNode;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFluxDelegate, AFlux*, Flux);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFluxIntDelegate, AFlux*, Flux, int, Index);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFluxBoolDelegate, AFlux*, Flux, bool, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFluxWithNodeDelegate, AFlux*, Flux, AFluxNode*, FluxNode);


USTRUCT(Blueprintable)
struct FFluxMeshInfo
{
	GENERATED_BODY()
public:
	FFluxMeshInfo();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UStaticMeshComponent* Mesh;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UStaticMeshComponent* CollisionMesh;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float Distance;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool IsEndArrow;
};

UCLASS()
class INFERNALETESTING_API AFlux : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFlux();
	
	virtual void Tick(float DeltaTime) override;
	void SetOrigin(ABuildingParent* NewOrigin);

	void Init(ABuildingParent* NewOrigin, FVector Target, float Offset);
	void Init(ABuildingParent* NewOrigin, TArray<FPathStruct> Path, bool ShouldDisplay);
	void InitAsInactiveFlux(ABuildingParent* NewOrigin, FVector FirstNodePos);
	
	void ContinueOld(FVector Target, float Offset);
	void Continue(TArray<FPathStruct> Path);
	void ClearAllPreviousFakePoints(int NodeModified);
	void RefreshSplinesForAmalgams(int NodeModified);
	TArray<TWeakObjectPtr<AFluxNode>> GetFluxNodes() const;
	TArray<FVector> GetPreviewPoints() const;

	UFUNCTION(BlueprintImplementableEvent) void SimpleHoverFlux();
	UFUNCTION(BlueprintImplementableEvent) void SimpleUnHoverFlux();

	void RefreshNodeVisibility();
	UFUNCTION(BlueprintCallable, BlueprintPure) ABuildingParent* GetOrigin() const;
	UFUNCTION(BlueprintCallable, BlueprintPure) int GetFluxNodesCount() const;
	UFUNCTION(BlueprintCallable, BlueprintPure) TArray<AFluxNode*> GetFluxNodes();

	UFUNCTION(BlueprintCallable, BlueprintPure) int GetNextNodeIndex(int Base) const;
	UFUNCTION(BlueprintCallable, BlueprintPure) int GetPreviousNodeIndex(int Base, bool IgnoreTmp) const;
	UFUNCTION(BlueprintCallable, BlueprintPure) AFluxNode* GetPreviousNode(int Base, bool IgnoreTmp) const;
	UFUNCTION(BlueprintCallable, BlueprintPure) TArray<int> GetNextFakeNodesIndex(int Base) const;
	UFUNCTION(BlueprintCallable, BlueprintPure) TArray<int> GetPreviousFakeNodesIndex(int Base) const;

	UFUNCTION(BlueprintCallable) ABuildingParent* GetOriginBP();

	UFUNCTION(Server, Reliable) void RemoveFluxServer();
	UFUNCTION(Server, Reliable) void ResetFluxServer();
	UFUNCTION(Server, Reliable) void RemoveFluxNoDelegateServer();
	UFUNCTION(Server, Reliable) void RemoveFluxNodeServer(AFluxNode* FluxNode);
	UFUNCTION(Server, Reliable) void CreateFluxNodeServer(FVector Target);
	
	bool CreateFluxNode(FVector Target);
	void RemoveFluxTool();
	
	void MoveNodeOLD(AFluxNode* FluxNode, FVector Target, float Offset);
	void MoveNode(AFluxNode* FluxNode, FVector Target, TArray<FPathStruct> BeforePath, TArray<FPathStruct> AfterPath, TArray<FPathStruct> OriginalToNodePath, AFluxNode* OriginalNode, bool UseOriginal);
	void MoveNodePreview(AFluxNode* FluxNode, FVector Target, TArray<FPathStruct> BeforePath, TArray<FPathStruct> AfterPath, TArray<FPathStruct> OriginalToNodePath, AFluxNode* OriginalNode, bool UseOriginal);

	void MoveFirstNode();
	void MoveFirstNode(AFluxNode* FluxNode, AFluxNode* SecondNode, bool CanOverlap = true);

	void CallCreatedNode(AFluxNode* FluxNode);
	AFluxNode* CreateNodeBefore(AFluxNode* FluxNode, bool RealNode, bool bIsTmp);
	AFluxNode* InsertNodes(TArray<FPathStruct> PathStructs, int IndexToInsert, bool MakeLastNodeReal = false);

	USplineComponent* GetSplineComponent() const;
	USplineComponent* GetSplineForAmalgamsComponent() const;
	TWeakObjectPtr<AFluxNode> GetEndNode() const;
	TWeakObjectPtr<AFluxNode> GetFirstNode() const;
	bool IsPathfindingRight() const;
	TArray<FVector> GetPath() const;
	void SetFluxActive(bool bActivate);
	void ForceFluxActiveVisibility(bool bActivate);
	UFUNCTION(BlueprintCallable, BlueprintPure) bool IsFluxActive() const;

	void InteractStartHoverFlux(APlayerControllerInfernale* Interactor) override;
	void InteractEndHoverFlux(APlayerControllerInfernale* Interactor) override;

	UFUNCTION(BlueprintCallable, BlueprintPure) FOwner GetOwnerInfo();
	UFUNCTION(BlueprintCallable, BlueprintPure) int GetNumberOfNodeBeforeFirstPath();
	UFUNCTION(BlueprintCallable, BlueprintPure) AFluxNode* GetLastNodeNotEnd(bool bIgnoreEnd);

	uint32 GetUpdateID();
	uint32 GetUpdateVersion();

	float GetAmalgamsSpeedMult();
	void SetAmalgamsSpeedMult(float NewSpeed);

	void InsertWeakPtrToFluxNodes(TWeakObjectPtr<AFluxNode> FluxNode, int Index, bool bIsLast = false);
	
	/*
	* Returns an array of float
	* 0 - Unit power
	* 1 - Building power
	* 2 - Monster power
	*/
	TArray<float> GetFluxPower();
	void SetFluxPowers(float NewDemonPower, float NewBuildingPower, float NewMonsterPower, int NewPortalCount);
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	void SyncDataAsset();
	void RedrawSplines(bool bFinishChanges, int PointsIndexModified, bool DontDrawLast, const FVector& LastPointPos);
	void CalculatePathfinding();
	void RemovePreviousFakePoints(int IndexLastRealNode);
	void RemoveNextFakePoints(int IndexLastRealNode);
	void LocalReorderIndexes();
	void RotateNodes(bool RotateStart = true);
	TArray<AFluxNode*> MakeFluxNodesRefs();
	void SetFluxNodesLocal(const TArray<AFluxNode*>& NewFluxNodes);
	void ReorderLocalIndexes();
	void SetSplinePointsLocal(const TArray<FVector>& Points, const bool bFinishChanges, int PointsIndexModified, const TArray<FVector>& PointsPreview, const bool DontDrawLast, const FVector& LastPointPos);

	void VerifyFluxValidity();
	FVector GetEndNodePos() const;
	FVector GetBeforeEndNodePos() const;
	
	UFUNCTION() void RefreshVisibilityDisabledFluxes();
	UFUNCTION() void OnPreLauchGame();
	UFUNCTION() void OnLauchGame();

	UFUNCTION(NetMulticast, Reliable) void MoveNodeMulticast(AFluxNode* FluxNode, FVector Target);
	UFUNCTION(NetMulticast, Reliable) void RotateNodeMulticast(AFluxNode* FluxNode, FRotator Target);
	UFUNCTION(NetMulticast, Reliable) void SetSplinePointsMulticast(const TArray<FVector>& Points, const bool bFinishChanges, int PointsIndexModified, const TArray<FVector>& PointsPreview, const bool DontDrawLast, const FVector& LastPointPos);
	UFUNCTION(NetMulticast, Reliable) void ReorderIndexesMulticast();
	UFUNCTION(NetMulticast, Reliable) void CallNodeCreationFailedMulticast();
	UFUNCTION(NetMulticast, Reliable) void SetOriginMulticast(ABuildingParent* NewOrigin);
	UFUNCTION(NetMulticast, Reliable) void SetFluxActiveMulticast(bool bActivate);
	UFUNCTION(NetMulticast, Reliable) void ForceVisibilityFluxActiveMulticast(bool bActivate);
	UFUNCTION(NetMulticast, Reliable) void RefreshNodeVisibilityMulticast();
	UFUNCTION(NetMulticast, Reliable) void ResetFluxMulticast();
	UFUNCTION(NetMulticast, Reliable) void RefreshNodeVisibilityOnOwnerMulticast(AFluxNode* FluxNode, bool bVisible);
	UFUNCTION(NetMulticast, Reliable) void RefreshVisibilityDisabledFluxesMulticast();
	UFUNCTION(NetMulticast, Reliable) void SetFluxPowersMulticast(float NewDemonPower, float NewBuildingPower, float NewMonsterPower, int NewPortalCount);
	UFUNCTION(NetMulticast, Reliable) void SetAmalgamsSpeedMultMulticast(float NewSpeed);
	UFUNCTION(NetMulticast, Reliable) void SetFluxNodesMulticast(const TArray<AFluxNode*>& NewFluxNodes);
	UFUNCTION(NetMulticast, Reliable) void PreLauchGameMulticast(const TArray<AFluxNode*>& NewFluxNodes);
	UFUNCTION(NetMulticast, Reliable) void ChangeNodeTypeMulticast(AFluxNode* Node, EFluxNodeType NewType);

	UFUNCTION(NetMulticast, Unreliable) void MoveNodeMulticastUnreliable(AFluxNode* FluxNode, FVector Target);
	UFUNCTION(NetMulticast, Unreliable) void ReorderIndexesMulticastUnreliable();
	UFUNCTION(NetMulticast, Unreliable) void SetSplinePointsMulticastUnreliable(const TArray<FVector>& Points, const bool bFinishChanges, int PointsIndexModified, const TArray<FVector>& PointsPreview, const bool DontDrawLast, const FVector& LastPointPos);
	UFUNCTION(NetMulticast, Unreliable) void RotateNodeMulticastUnreliable(AFluxNode* FluxNode, FRotator Target);

	void RotateNodesUnreliable(bool RotateStart = true);
	void RedrawSplinesUnreliable(bool bFinishChanges, int PointsIndexModified, bool DontDrawLast, const FVector& LastPointPos);
	

	UFUNCTION(NetMulticast, Reliable) void RedrawFluxFinished(int PointsIndexModified);
	UFUNCTION(NetMulticast, Reliable) void RedrawFluxPreview(int PointsIndexModified, const TArray<FVector>& PointsPreview);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable) void OnFluxHovered();
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable) void OnFluxEndHovered();
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable) void RedrawFluxFinishedBP(int PointsIndexModified, const bool DontDrawLast, const FVector LastPointPos);
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable) void RedrawFluxPreviewBP(int PointsIndexModified, const TArray<FVector>& PointsPreview, const bool DontDrawLast);
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable) void SetFluxActiveBP(bool Active);
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable) void ResetFluxBP();

	
public:
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FFluxDelegate FluxDestroyed;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FFluxDelegate FluxUpdated;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FFluxDelegate FluxFinishUpdate;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FFluxDelegate FluxPathRecalculated;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FFluxIntDelegate FluxNodeRemoved;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FFluxIntDelegate FluxNodeAdded;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FFluxWithNodeDelegate FluxNodeCreated;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FFluxDelegate FluxNodeCreationFailed;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FFluxBoolDelegate FluxEnabled;

protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) USplineComponent* SplineComponent;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) USplineComponent* SplineForAmalgamsComponent;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UFluxSettingsDataAsset* FluxSettingsDataAsset;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bUseDataAsset = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float TooCloseRange = 100;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Flux power") float FluxPowerDemon;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Flux power") float FluxPowerBuilding;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Flux power") float FluxPowerMonster;

	TArray<TWeakObjectPtr<AFluxNode>> FluxNodes = TArray<TWeakObjectPtr<AFluxNode>>();
	TArray<TWeakObjectPtr<AFluxNode>> FluxNodesForPathfinding = TArray<TWeakObjectPtr<AFluxNode>>();

	UPROPERTY() ABuildingParent* Origin;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere) TSubclassOf<AFluxNode> FluxNodeSpawnClass;
	UPROPERTY(BlueprintReadWrite); bool bHovered = false;

	TArray<FVector> PreviewPoints = TArray<FVector>();
	ESplinePointType::Type SplinePointType = ESplinePointType::Linear;

	/* FluxPathfinding */
	TArray<FVector> FluxPath = TArray<FVector>();
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float SpaceBetweenPathfindingNodes = 400.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bDebugRemovedNode = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bDebugTooClose = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bDebugVisibility = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bDebugStartNodeMove = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool DebugShowFluxPath = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float AmalgamsSpeedMult = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) int PortalCount = 0;
	bool bPathfindingIsRight = false;
	bool bFluxIsActive = false;
	bool bFluxNodeNumber = false;

	bool bCanReplicateNow = true;
	float CurrentTime = 0;
	float TimeBetweenReplicates = 0.025f;
	int ValidityCheckThisFrame = 0;

	
	/* Mass Pathfinding Synchronization */
	uint32 UpdateID = 0;
	uint32 UpdateVersion = 0;
};
