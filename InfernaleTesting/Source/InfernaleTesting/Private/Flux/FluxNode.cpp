// Fill out your copyright notice in the Description page of Project Settings.


#include "Flux/FluxNode.h"
#include "Flux/Flux.h"
#include "LD/Buildings/BuildingParent.h"

// Sets default values
AFluxNode::AFluxNode()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

AFlux* AFluxNode::GetFlux() const
{
	return FluxOwner;
}

EFluxNodeType AFluxNode::GetNodeType() const
{
	return FluxNodeType;
}

void AFluxNode::Init(AFlux* Flux, EFluxNodeType NodeType, int NewNodeIndex, bool IsReal)
{
	FluxOwner = Flux;
	FluxNodeType = NodeType;
	NodeIndex = NewNodeIndex;
	bIsReal = IsReal;
	
	FluxOwner->FluxDestroyed.AddDynamic(this, &AFluxNode::DestroyNode);
	TryCallNodeCreation();
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Node init"));
	InitMulticast(Flux, IsReal);
	ChangeTypeMulticast(NodeType);
}

void AFluxNode::ChangeType(EFluxNodeType NodeType)
{
	FluxNodeType = NodeType;
	NodeTypeUpdatedBP();
}

void AFluxNode::ChangeTypeForAll(EFluxNodeType NodeType)
{
	ChangeTypeMulticast(NodeType);
}

void AFluxNode::SetNodeIndex(int NewNodeIndex)
{
	NodeIndex = NewNodeIndex;
}

int AFluxNode::GetNodeIndex() const
{
	return NodeIndex;
}

void AFluxNode::DestroyNodeByUser()
{
	if (bDebugRemovedNode) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("DestroyNodeByUser"));
	DestroyNode(FluxOwner);
}

bool AFluxNode::IsReal() const
{
	return bIsReal;
}

void AFluxNode::SetIsReal(bool IsReal)
{
	bIsReal = IsReal;
}

void AFluxNode::SetNodeVisibility(const bool bVisible)
{
	SetNodeVisibilityBP(bVisible);
}

void AFluxNode::EnableCollision(bool bEnable)
{
	EnableCollisionsMulticast(bEnable);
}

void AFluxNode::EnableCollisionLocal(bool bEnable)
{
	EnableCollisionsBP(bEnable);
}

void AFluxNode::CallInitBP()
{
	CallInitBPMulticast();
}

void AFluxNode::SetTmpNode()
{
	FluxNodeType = EFluxNodeType::TmpNode;
}

void AFluxNode::UnsetTmpNode()
{
	FluxNodeType = EFluxNodeType::PathNode;
	SetIsRealMulticast(true);
	EnableCollisionsMulticast(true);
}

bool AFluxNode::IsTmpNode() const
{
	return FluxNodeType == EFluxNodeType::TmpNode;
}

void AFluxNode::CallNodeTypeUpdated()
{
	CallNodeTypeUpdatedMulticast();
}

// Called when the game starts or when spawned
void AFluxNode::BeginPlay()
{
	Super::BeginPlay();
	TryCallNodeCreation();
}

void AFluxNode::TryCallNodeCreation()
{
	if (bNodeCreatedCalled) return;
	if (!FluxOwner)
	{
		NodeCreationTimer.Invalidate();
		GetWorld()->GetTimerManager().SetTimer(NodeCreationTimer, this, &AFluxNode::TryCallNodeCreation, 0.1f, false);
		if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Node creation failed"));
		return;
	}

	FluxOwner->CallCreatedNode(this);
	NodeCreationTimer.Invalidate();
	bNodeCreatedCalled = true;
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Node creation DONE"));
}

void AFluxNode::DestroyNode(AFlux* Flux)
{
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Node destroyed"));
	FluxNodeDestroyed.Broadcast(this);
	Destroy();
}

void AFluxNode::CallInitBPMulticast_Implementation()
{
	NodeInitBP(FluxOwner, bIsReal);
}

void AFluxNode::SetIsRealMulticast_Implementation(bool bNewIsReal)
{
	SetIsReal(bNewIsReal);
}

void AFluxNode::InitMulticast_Implementation(AFlux* Flux, bool IsReal)
{
	FluxOwner = Flux;
	bIsReal = IsReal;
	NodeInitBP(Flux, IsReal);
}

void AFluxNode::ChangeTypeMulticast_Implementation(EFluxNodeType NodeType)
{
	ChangeType(NodeType);
}

void AFluxNode::EnableCollisionsMulticast_Implementation(bool bEnabled)
{
	EnableCollisionsBP(bEnabled);
}

void AFluxNode::CallNodeTypeUpdatedMulticast_Implementation()
{
	NodeTypeUpdatedBP();
}

// Called every frame
void AFluxNode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AFluxNode::OnSelectedForFluxCpp()
{
	if (!FluxOwner) return;
	const auto Origin = FluxOwner->GetOrigin();
	if (!Origin) return;
	Origin->OnSelectedForFluxCpp();
}

void AFluxNode::OnDeselectedForFluxCpp()
{
	if (!FluxOwner) return;
	const auto Origin = FluxOwner->GetOrigin();
	if (!Origin) return;
	Origin->OnDeselectedForFluxCpp();
}

void AFluxNode::InteractStartHoverFlux(APlayerControllerInfernale* Interactor)
{
	if (Interactor->bIsEscapeMenuOpen) return;
	IInteractable::InteractStartHoverFlux(Interactor);
	bHovered = true;
	OnHovered();
}

void AFluxNode::InteractEndHoverFlux(APlayerControllerInfernale* Interactor)
{
	IInteractable::InteractEndHoverFlux(Interactor);
	bHovered = false;
	OnUnHovered();
}

