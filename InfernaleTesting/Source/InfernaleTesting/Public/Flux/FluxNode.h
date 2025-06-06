// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/FluxStart.h"
#include "Interfaces/Interactable.h"
#include "FluxNode.generated.h"

class AFlux;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFluxNodeDestroyed, AFluxNode*, FluxNode);

UENUM(Blueprintable)
enum class EFluxNodeType : uint8
{
	None,
	BaseNode,
	StartNode,
	EndNode,
	FakeNode,
	TmpNode,
	PathNode,
};

UCLASS()
class INFERNALETESTING_API AFluxNode : public AActor, public IFluxStart, public IInteractable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFluxNode();
	
	virtual void Tick(float DeltaTime) override;
	virtual void OnSelectedForFluxCpp() override;
	virtual void OnDeselectedForFluxCpp() override;

	
	void InteractStartHoverFlux(APlayerControllerInfernale* Interactor) override;
	void InteractEndHoverFlux(APlayerControllerInfernale* Interactor) override;
	
	AFlux* GetFlux() const;
	EFluxNodeType GetNodeType() const;
	void Init(AFlux* Flux, EFluxNodeType NodeType, int NodeIndex, bool IsReal);
	void ChangeType(EFluxNodeType NodeType);
	void ChangeTypeForAll(EFluxNodeType NodeType);
	void SetNodeIndex(int NewNodeIndex);
	int GetNodeIndex() const;
	void DestroyNodeByUser();
	bool IsReal() const;
	void SetIsReal(bool IsReal);
	void SetNodeVisibility(const bool bVisible);
	void EnableCollision(bool bEnable);
	void EnableCollisionLocal(bool bEnable);
	void CallInitBP();

	void SetTmpNode();
	void UnsetTmpNode();
	bool IsTmpNode() const;
	void CallNodeTypeUpdated();
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable) void RefreshNodeVisibilityBP(bool NewVisibility);

	UFUNCTION(BlueprintImplementableEvent) void OnDroppedTriggerVFX();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void TryCallNodeCreation();

	UFUNCTION()
	void DestroyNode(AFlux* Flux);

	UFUNCTION(NetMulticast, Reliable) void InitMulticast(AFlux* Flux, bool IsReal);
	UFUNCTION(NetMulticast, Reliable) void ChangeTypeMulticast(EFluxNodeType NodeType);
	UFUNCTION(NetMulticast, Reliable) void EnableCollisionsMulticast(bool bEnabled);
	UFUNCTION(NetMulticast, Reliable) void CallNodeTypeUpdatedMulticast();
	UFUNCTION(NetMulticast, Reliable) void CallInitBPMulticast();
	UFUNCTION(NetMulticast, Reliable) void SetIsRealMulticast(bool bNewIsReal);
	UFUNCTION(BlueprintImplementableEvent) void NodeInitBP(AFlux* Flux, bool IsReal);
	UFUNCTION(BlueprintImplementableEvent) void SetNodeVisibilityBP(bool bVisible);
	UFUNCTION(BlueprintImplementableEvent) void EnableCollisionsBP(bool bEnabled);

	UFUNCTION(BlueprintImplementableEvent) void OnHovered();
	UFUNCTION(BlueprintImplementableEvent) void OnUnHovered();
	UFUNCTION(BlueprintImplementableEvent) void NodeTypeUpdatedBP();

public:
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FFluxNodeDestroyed FluxNodeDestroyed;

protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere) AFlux* FluxOwner;
	UPROPERTY(BlueprintReadOnly, EditAnywhere) EFluxNodeType FluxNodeType;
	UPROPERTY(BlueprintReadOnly, EditAnywhere) int NodeIndex = -1;

	UPROPERTY(BlueprintReadWrite) bool bHovered = false;
	UPROPERTY(BlueprintReadWrite) bool bDebug = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bDebugRemovedNode = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bIsReal = false;

	bool bNodeCreatedCalled = false;
	FTimerHandle NodeCreationTimer;
};
