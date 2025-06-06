// Fill out your copyright notice in the Description page of Project Settings.


#include "Flux/Flux.h"

#include "Components/SplineComponent.h"
#include "DataAsset/FluxSettingsDataAsset.h"
#include "Flux/FluxNode.h"
#include "LD/Buildings/BuildingParent.h"

// Include used for Flux Aggro Pre calculation
#include <Mass/Collision/SpatialHashGrid.h>

#include "NavigationSystem.h"
#include "LD/Buildings/MainBuilding.h"
#include "Structs/SimpleStructs.h"

FFluxMeshInfo::FFluxMeshInfo(): Mesh(nullptr), CollisionMesh(nullptr), Distance(0), IsEndArrow(false)
{
}

// Sets default values
AFlux::AFlux()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	SplineForAmalgamsComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineForAmalgamsComponent"));
	FluxPath = TArray<FVector>();
}

// Called every frame
void AFlux::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	ValidityCheckThisFrame = 0;
	
	if (bCanReplicateNow) return;
	CurrentTime += DeltaTime;
	if (CurrentTime >= TimeBetweenReplicates)
	{
		bCanReplicateNow = true;
		CurrentTime = 0;
	}

}

void AFlux::SetOrigin(ABuildingParent* NewOrigin)
{
	Origin = NewOrigin;
}

void AFlux::Init(ABuildingParent* NewOrigin, FVector Target, float Offset)
{
	UClass* ClassType = FluxNodeSpawnClass->GetDefaultObject()->GetClass();
	SetOrigin(NewOrigin);
	SetOriginMulticast(NewOrigin);

	FVector TargetPos = Target;
	TargetPos.Z = 0;
	FVector OriginPos = Origin->GetActorLocation();
	OriginPos.Z = 0;

	FVector Direction = (TargetPos - OriginPos).GetSafeNormal();
	const auto SpawnParams = FActorSpawnParameters();
	auto Transform = FTransform(FRotator::ZeroRotator, OriginPos, FVector(1, 1, 1));
	AActor* Actor = nullptr;
	AFluxNode* FluxNode = nullptr;
	FVector SpawnPos = OriginPos;
	FSplinePoint Point = FSplinePoint(0, SpawnPos, SplinePointType);

	SplineComponent->ClearSplinePoints();

	// Create Origin FluxNode
	SpawnPos = OriginPos + Direction * NewOrigin->GetOffsetRange();
	Transform.SetLocation(SpawnPos);
	Actor = GetWorld()->SpawnActor(ClassType, &Transform, SpawnParams);
	FluxNode = Cast<AFluxNode>(Actor);
	if (FluxNode == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("FluxNode is null"));
		return;
	}

	FluxNode->Init(this, EFluxNodeType::StartNode, 0, true);
	FluxNodes.Add(TWeakObjectPtr<AFluxNode>(FluxNode));
	Point = FSplinePoint(0, SpawnPos, SplinePointType);
	SplineComponent->AddPoint(Point, ESplineCoordinateSpace::World);
	

	// Create Target FluxNode
	SpawnPos = TargetPos - Direction * Offset;
	Transform.SetLocation(SpawnPos);
	Actor = GetWorld()->SpawnActor(ClassType, &Transform, SpawnParams);
	FluxNode = Cast<AFluxNode>(Actor);
	if (FluxNode == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("FluxNode is null"));
		return;
	}

	FluxNode->Init(this, EFluxNodeType::EndNode, 1, true);
	FluxNodes.Add(TWeakObjectPtr<AFluxNode>(FluxNode));
	Point = FSplinePoint(1, SpawnPos, SplinePointType);
	SplineComponent->AddPoint(Point, ESplineCoordinateSpace::World);

	RedrawSplines(true, -1, false, SpawnPos);
}

void AFlux::Init(ABuildingParent* NewOrigin, TArray<FPathStruct> Path, bool ShouldDisplay)
{
	UClass* ClassType = FluxNodeSpawnClass->GetDefaultObject()->GetClass();
	SetOrigin(NewOrigin);
	SetOriginMulticast(NewOrigin);

	
	const auto SpawnParams = FActorSpawnParameters();
	auto Transform = FTransform(FRotator::ZeroRotator, Path[0].PathPoint, FVector(1, 1, 1));
	AActor* Actor = nullptr;
	AFluxNode* FluxNode = nullptr;
	FVector SpawnPos;
	FSplinePoint Point = FSplinePoint(0, SpawnPos, SplinePointType);
	SplineComponent->ClearSplinePoints();

	for (int i = 0; i < Path.Num(); i++)
	{
		SpawnPos = Path[i].PathPoint;
		Transform.SetLocation(SpawnPos);
		Actor = GetWorld()->SpawnActor(ClassType, &Transform, SpawnParams);
		FluxNode = Cast<AFluxNode>(Actor);
		if (FluxNode == nullptr)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("FluxNode is null"));
			return;
		}

		auto NodeType = EFluxNodeType::BaseNode;
		if (i == 0) NodeType = EFluxNodeType::StartNode;
		else if (i == Path.Num() - 1) NodeType = EFluxNodeType::EndNode;
		// Handle fake nodes

		FluxNode->Init(this, NodeType, i, Path[i].IsReal);
		FluxNodes.Add(TWeakObjectPtr<AFluxNode>(FluxNode));
		Point = FSplinePoint(i, SpawnPos, SplinePointType);
		SplineComponent->AddPoint(Point, ESplineCoordinateSpace::World);
	}

	RedrawSplines(ShouldDisplay, -1, false, SpawnPos);
}

void AFlux::InitAsInactiveFlux(ABuildingParent* NewOrigin, FVector FirstNodePos)
{
	UClass* ClassType = FluxNodeSpawnClass->GetDefaultObject()->GetClass();
	SetOrigin(NewOrigin);
	SetOriginMulticast(NewOrigin);

	
	const auto SpawnParams = FActorSpawnParameters();
	auto Transform = FTransform(FRotator::ZeroRotator, FirstNodePos, FVector(1, 1, 1));
	AActor* Actor = nullptr;
	AFluxNode* FluxNode = nullptr;
	FSplinePoint Point = FSplinePoint(0, FirstNodePos, SplinePointType);
	SplineComponent->ClearSplinePoints();

	/* Origin FluxNode */
	Actor = GetWorld()->SpawnActor(ClassType, &Transform, SpawnParams);
	FluxNode = Cast<AFluxNode>(Actor);
	if (FluxNode == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("FluxNode is null"));
		return;
	}

	auto NodeType = EFluxNodeType::StartNode;
	FluxNode->Init(this, NodeType, 0, true);
	FluxNode->EnableCollision(false);
	FluxNodes.Add(TWeakObjectPtr<AFluxNode>(FluxNode));
	Point = FSplinePoint(0, FirstNodePos, SplinePointType);
	SplineComponent->AddPoint(Point, ESplineCoordinateSpace::World);

	/* End FluxNode */
	Actor = GetWorld()->SpawnActor(ClassType, &Transform, SpawnParams);
	FluxNode = Cast<AFluxNode>(Actor);
	if (FluxNode == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("FluxNode is null"));
		return;
	}

	NodeType = EFluxNodeType::EndNode;
	FluxNode->Init(this, NodeType, 1, true);
	RefreshNodeVisibilityOnOwnerMulticast(FluxNode, false);
	FluxNodes.Add(TWeakObjectPtr<AFluxNode>(FluxNode));
	Point = FSplinePoint(1, FirstNodePos, SplinePointType);
	SplineComponent->AddPoint(Point, ESplineCoordinateSpace::World);
	SetFluxNodesMulticast(MakeFluxNodesRefs());
	SetFluxActive(false);

	if (bFluxNodeNumber) GEngine->AddOnScreenDebugMessage(-1, 25.f, FColor::Green, FString::Printf(TEXT("Flux created %d nodes"), FluxNodes.Num()));

	RedrawSplines(true, -1, false, FirstNodePos);
	RefreshVisibilityDisabledFluxesMulticast();
}

void AFlux::ContinueOld(FVector Target, float Offset)
{
	UClass* ClassType = FluxNodeSpawnClass->GetDefaultObject()->GetClass();
	const auto LastNode = FluxNodes.Last();
	LastNode->ChangeTypeForAll(EFluxNodeType::BaseNode);
	FVector TargetPos = Target;
	TargetPos.Z = 0;
	FVector OriginPos = Origin->GetActorLocation();
	OriginPos.Z = 0;
	FVector Direction = (TargetPos - OriginPos).GetSafeNormal();

	const auto SpawnPos = TargetPos - Direction * Offset;
	const auto Transform = FTransform(FRotator::ZeroRotator, SpawnPos, FVector(1, 1, 1));
	const auto SpawnParams = FActorSpawnParameters();
	auto Actor = GetWorld()->SpawnActor(ClassType, &Transform, SpawnParams);
	auto FluxNode = Cast<AFluxNode>(Actor);
	if (FluxNode == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("FluxNode is null"));
		return;
	}
	FluxNode->Init(this, EFluxNodeType::EndNode, FluxNodes.Num(), true);
	FluxNodes.Add(TWeakObjectPtr<AFluxNode>(FluxNode));
	
	auto Key = SplineComponent->GetNumberOfSplinePoints();
	FSplinePoint Point = FSplinePoint(Key, SpawnPos, SplinePointType);
	SplineComponent->AddPoint(Point, ESplineCoordinateSpace::World);
	const auto NodeIndex = FluxNode->GetNodeIndex();
	FluxNodeAdded.Broadcast(this, NodeIndex);
	
	RedrawSplines(true, NodeIndex, false, SpawnPos);
}

void AFlux::Continue(TArray<FPathStruct> Path)
{
	UClass* ClassType = FluxNodeSpawnClass->GetDefaultObject()->GetClass();
	const auto LastNode = FluxNodes.Last();
	LastNode->ChangeTypeForAll(EFluxNodeType::BaseNode);
	const auto FirstNodeIndexModified = FluxNodes.Num();

	for (int i = 0; i < Path.Num(); i++)
	{
		const auto SpawnPos = Path[i].PathPoint;
		const auto Transform = FTransform(FRotator::ZeroRotator, SpawnPos, FVector(1, 1, 1));
		const auto SpawnParams = FActorSpawnParameters();
		auto Actor = GetWorld()->SpawnActor(ClassType, &Transform, SpawnParams);
		auto FluxNode = Cast<AFluxNode>(Actor);
		if (FluxNode == nullptr)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("FluxNode is null"));
			return;
		}
		auto NodeType = i == Path.Num() - 1 ? EFluxNodeType::EndNode : EFluxNodeType::PathNode;
		FluxNode->Init(this, NodeType, FluxNodes.Num(), Path[i].IsReal);
		FluxNodes.Add(TWeakObjectPtr<AFluxNode>(FluxNode));
		
		auto Key = SplineComponent->GetNumberOfSplinePoints();
		FSplinePoint Point = FSplinePoint(Key, SpawnPos, SplinePointType);
		SplineComponent->AddPoint(Point, ESplineCoordinateSpace::World);
		const auto NodeIndex = FluxNode->GetNodeIndex();
		FluxNodeAdded.Broadcast(this, NodeIndex);
	}
	auto NewPos = FVector::ZeroVector;
	if (FluxNodes.Num() > 0)
	{
		const auto EndNode = FluxNodes.Last();
		if (EndNode.IsValid())
		{
			NewPos = EndNode->GetActorLocation();
		}
	}
	
	RedrawSplines(true, FirstNodeIndexModified, false, NewPos);
}

void AFlux::ClearAllPreviousFakePoints(int NodeModified)
{
	TArray<AFluxNode*> NodesToRemove;
	bool NodeFound = false;
	for (int i = FluxNodes.Num() - 1; i > 0; i--)
	{
		if (!FluxNodes[i].IsValid()) continue;
		auto NodeIsReal = FluxNodes[i]->IsReal();
		if (NodeIsReal)
		{
			if (NodeFound) break;
			if (i != NodeModified)
			{
				NodesToRemove.Empty();
				continue;
			}
			NodeFound = true;
			continue;
		}
		const auto FluxNodeType = FluxNodes[i]->GetNodeType();
		if (FluxNodeType == EFluxNodeType::StartNode || FluxNodeType == EFluxNodeType::EndNode || FluxNodeType == EFluxNodeType::PathNode) continue;
		NodesToRemove.Add(FluxNodes[i].Get());
	}
	for (auto ToRemove : NodesToRemove)
	{
		if (ToRemove->IsTmpNode()) continue;
		FluxNodes.Remove(ToRemove);
		if (bDebugRemovedNode) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Fake Node removed"));
		ToRemove->DestroyNodeByUser();
	}
	VerifyFluxValidity();

	ReorderIndexesMulticast();
}

ABuildingParent* AFlux::GetOrigin() const
{
	return Origin;
}

void AFlux::RefreshSplinesForAmalgams(int NodeModified)
{
	RedrawSplines(true, NodeModified, false, FVector::ZeroVector);
}

TArray<TWeakObjectPtr<AFluxNode>> AFlux::GetFluxNodes() const
{
	return FluxNodes;
}

TArray<FVector> AFlux::GetPreviewPoints() const
{
	return PreviewPoints;
}

void AFlux::RefreshNodeVisibility()
{
	RefreshNodeVisibilityMulticast();
}

int AFlux::GetFluxNodesCount() const
{
	return FluxNodes.Num();
}

TArray<AFluxNode*> AFlux::GetFluxNodes()
{
	return MakeFluxNodesRefs();
}

int AFlux::GetNextNodeIndex(int Base) const
{
	int ReturnedValue = -1;
	for (int i = FluxNodes.Num() - 1; i >= 0; i--)
	{
		const auto FluxNode = FluxNodes[i];
		if (!FluxNode.IsValid()) continue;
		if (!FluxNode->IsReal()) continue;
		if (FluxNode->GetNodeIndex() <= Base) return ReturnedValue;
		ReturnedValue = i;
	}
	return -1;
}

int AFlux::GetPreviousNodeIndex(int Base, bool IgnoreTmp) const
{
	int ReturnedValue = -1;
	for (int i = 0; i < FluxNodes.Num(); i++)
	{
		const auto FluxNode = FluxNodes[i];
		if (!FluxNode.IsValid()) continue;
		if (!FluxNode->IsReal()) continue;
		if (IgnoreTmp && FluxNode->IsTmpNode()) continue;
		if (FluxNode->GetNodeIndex() >= Base) return ReturnedValue;
		ReturnedValue = i;
	}
	return -1;
}

AFluxNode* AFlux::GetPreviousNode(int Base, bool IgnoreTmp) const
{
	AFluxNode* ReturnedValue = nullptr;
	for (int i = 0; i < FluxNodes.Num(); i++)
	{
		const auto FluxNode = FluxNodes[i];
		if (!FluxNode.IsValid()) continue;
		if (!FluxNode->IsReal()) continue;
		if (IgnoreTmp && FluxNode->IsTmpNode()) continue;
		if (FluxNode->GetNodeIndex() >= Base) return ReturnedValue;
		ReturnedValue = FluxNode.IsValid() ? FluxNode.Get() : nullptr;
	}
	return nullptr;
}

TArray<int> AFlux::GetNextFakeNodesIndex(int Base) const
{
	TArray<int> ReturnedValue;
	for (int i = Base+1; i < FluxNodes.Num(); i++)
	{
		if (FluxNodes[i]->IsReal()) break;
		ReturnedValue.Add(i);
	}
	return ReturnedValue;
}

TArray<int> AFlux::GetPreviousFakeNodesIndex(int Base) const
{
	TArray<int> ReturnedValue;
	for (int i = Base-1; i >= 0; i--)
	{
		if (FluxNodes[i]->IsReal()) break;
		ReturnedValue.Add(i);
	}
	return ReturnedValue;
}

ABuildingParent* AFlux::GetOriginBP()
{
	return Origin;
}

void AFlux::ResetFluxServer_Implementation()
{
	if (FluxNodes.Num() < 2)
	{
		if (bFluxNodeNumber) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("FluxNodes.Num() < 2 on reset, %d"), FluxNodes.Num()));
		VerifyFluxValidity();
		return;
	}
	auto FirstNode = FluxNodes[0];
	auto LastNode = FluxNodes[FluxNodes.Num()-1];

	if (bFluxNodeNumber) GEngine->AddOnScreenDebugMessage(-1, 25.f, FColor::Green, FString::Printf(TEXT("Reseting %d nodes"), FluxNodes.Num()));

	for (auto FluxNode : FluxNodes)
	{
		if (!FluxNode.IsValid()) continue;
		if (FluxNode->GetNodeType() == EFluxNodeType::StartNode)
		{
			FirstNode = FluxNode;
			continue;
		}
		if (FluxNode->GetNodeType() == EFluxNodeType::EndNode)
		{
			LastNode = FluxNode;
			continue;
		}
		if (bFluxNodeNumber) GEngine->AddOnScreenDebugMessage(-1, 25.f, FColor::Green, FString::Printf(TEXT("Will remove Node %d nodes"), FluxNodes.Num()));
		if (bFluxNodeNumber) GEngine->AddOnScreenDebugMessage(-1, 25.f, FColor::Green, FString::Printf(TEXT("remove Node %d index"), FluxNode->GetNodeIndex()));
		FluxNode->Destroy();
	}
	FluxNodes.Empty();
	if (!FirstNode.IsValid() || !LastNode.IsValid())
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("FirstNode or LastNode is null when resetting flux"));
		return;
	}

	MoveFirstNode(FirstNode.Get(), LastNode.Get(), false);

	FluxNodes.Add(FirstNode);
	FluxNodes.Add(LastNode);
	VerifyFluxValidity();

	MoveNodeMulticast(LastNode.Get(), FirstNode->GetActorLocation());
	LastNode->SetNodeIndex(1);

	FirstNode->EnableCollision(false);
	FirstNode->SetNodeVisibility(false);
	ResetFluxMulticast();
	
	SetFluxActive(false);
}

void AFlux::RemoveFluxNoDelegateServer_Implementation()
{
	if (bDebugRemovedNode) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("RemoveFluxNoDelegateServer_Implementation"));
	//Destroy();
}

void AFlux::RemoveFluxServer_Implementation()
{
	if (bDebugRemovedNode) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("RemoveFluxServer_Implementation"));
	FluxDestroyed.Broadcast(this);
	Destroy();
}

void AFlux::RemoveFluxTool()
{
	for (const auto FluxNode : FluxNodes)
	{
		FluxNode->Destroy();
	}
	FluxNodes.Empty();
	Destroy();
}

void AFlux::RemoveFluxNodeServer_Implementation(AFluxNode* FluxNode)
{
	bool ShouldRotateNodes = false;
	if (FluxNode == nullptr) return;
	if (FluxNode->GetNodeType() == EFluxNodeType::StartNode)
	{
		//RemoveFluxServer();
		ResetFluxServer();
		return;
	}
	const int Index = FluxNode->GetNodeIndex();
	
	if (FluxNode->GetNodeType() == EFluxNodeType::EndNode)
	{
		const auto PrevIndex = GetPreviousNodeIndex(FluxNode->GetNodeIndex(), true);
		if (PrevIndex == 0)
		{
			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PrevIndex == 0"));
			//RemoveFluxServer();
			ResetFluxServer();
			return;
		}
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PrevIndex != 0"));
		for (int i = FluxNodes.Num()-1; i >= 0; i--)
        {
			const auto FluxNodeLocal = FluxNodes[i];
			if (!FluxNodeLocal.IsValid()) continue;
            const auto FluxNodeIndex = FluxNodeLocal->GetNodeIndex();
			if (FluxNodeIndex != PrevIndex) continue;
			FluxNodeLocal->ChangeTypeForAll(EFluxNodeType::EndNode);
			ShouldRotateNodes = true;
			break;
        }
	}

	RemovePreviousFakePoints(Index);
	RemoveNextFakePoints(Index);
	FluxNodes.Remove(FluxNode);
	SplineComponent->RemoveSplinePoint(Index);
	if (bDebugRemovedNode) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Node removed (RemoveFluxNodeServer_Implementation)"));
	FluxNode->DestroyNodeByUser();
	FluxNodeRemoved.Broadcast(this, Index);
	VerifyFluxValidity();

	ReorderIndexesMulticast();
	RedrawSplines(true, Index-1, false, GetEndNodePos());
	if (ShouldRotateNodes) RotateNodes();
}

void AFlux::CreateFluxNodeServer_Implementation(FVector Target)
{
	CreateFluxNode(Target);
}

bool AFlux::CreateFluxNode(FVector Target)
{
	const float DistanceAlongSpline = SplineComponent->GetDistanceAlongSplineAtLocation(Target, ESplineCoordinateSpace::World);
	const int Index = SplineComponent->FindInputKeyClosestToWorldLocation(Target);

	const auto Point = SplineComponent->GetLocationAtSplinePoint(Index, ESplineCoordinateSpace::World);
	const auto PointDistance = SplineComponent->GetDistanceAlongSplineAtSplinePoint(Index);

	UClass* ClassType = FluxNodeSpawnClass->GetDefaultObject()->GetClass();
	const auto SpawnParams = FActorSpawnParameters();
	const auto Transform = FTransform(FRotator::ZeroRotator, Target, FVector(1, 1, 1));

	const auto RangeBehind = (Target - Point).Size();
	const bool TooCloseBehind = RangeBehind < TooCloseRange;
	
	auto TooCloseInFront = false;
	const auto IndexInFront = Index+1;
	if (IndexInFront < SplineComponent->GetNumberOfSplinePoints())
	{
		const auto Point2 = SplineComponent->GetLocationAtSplinePoint(IndexInFront, ESplineCoordinateSpace::World);
		const auto RangeInFront = (Point2 - Target).Size();
		TooCloseInFront = RangeInFront < TooCloseRange;
		if (bDebugTooClose) DrawDebugSphere(GetWorld(), Point2, 500, 10, FColor::Blue, false, 5.f, 0, 5.f);
	}
	
	if (bDebugTooClose) DrawDebugSphere(GetWorld(), Target, 500, 10, FColor::Red, false, 5.f, 0, 5.f);
	if (bDebugTooClose) DrawDebugSphere(GetWorld(), Point, 500, 10, FColor::Green, false, 5.f, 0, 5.f);
	
	if ( TooCloseBehind || TooCloseInFront)
	{
		if (bDebugTooClose) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Too close to existing node"));
		CallNodeCreationFailedMulticast();
		return false;
	}
	
	auto Actor = GetWorld()->SpawnActor(ClassType, &Transform, SpawnParams);
	auto FluxNode = Cast<AFluxNode>(Actor);
	int NewIndex = Index;

	if (!FluxNode) return false;
	TWeakObjectPtr<AFluxNode> FluxNodeWeakPtr = TWeakObjectPtr<AFluxNode>(FluxNode);
	if (!FluxNodeWeakPtr.IsValid()) return false;
	
	if (PointDistance < DistanceAlongSpline) NewIndex++;

	InsertWeakPtrToFluxNodes(FluxNodeWeakPtr, NewIndex);
	FluxNode->Init(this, EFluxNodeType::PathNode, NewIndex, true);

	NewIndex = SplineComponent->GetNumberOfSplinePoints();
	FSplinePoint SplinePoint = FSplinePoint(NewIndex, Target, SplinePointType);
	SplineComponent->AddPoint(SplinePoint, ESplineCoordinateSpace::World);
	FluxNodeAdded.Broadcast(this, NewIndex);
	
	ReorderIndexesMulticast();
	RedrawSplines(false, NewIndex, false, GetEndNodePos());
	return true;
}

void AFlux::MoveNodeOLD(AFluxNode* FluxNode, FVector Target, float Offset)
{
	// Find target
	const auto Index = FluxNode->GetNodeIndex();
	if (Index <= 0 || Index > FluxNodes.Num()) return;
	FVector PreviousOrigin = FluxNodes[Index - 1]->GetActorLocation();
	PreviousOrigin.Z = 0;
	FVector NewPos = Target;
	NewPos.Z = 0;
	FVector Direction = (NewPos - PreviousOrigin).GetSafeNormal();

	MoveNodeMulticast(FluxNode, NewPos - Direction * Offset);
	SplineComponent->SetLocationAtSplinePoint(Index, NewPos, ESplineCoordinateSpace::World);
	RedrawSplines(false, Index, false, GetEndNodePos());
}

void AFlux::MoveNode(AFluxNode* FluxNode, FVector Target, TArray<FPathStruct> BeforePath,
	TArray<FPathStruct> AfterPath, TArray<FPathStruct> OriginalToNodePath, AFluxNode* OriginalNode, bool UseOriginal)
{
	UClass* ClassType = FluxNodeSpawnClass->GetDefaultObject()->GetClass();
	auto FluxNodeModifiedInitialIndex = FluxNode->GetNodeIndex();
	if (FluxNodeModifiedInitialIndex < 0 || FluxNodeModifiedInitialIndex > FluxNodes.Num()) return;
	auto Index = FluxNodeModifiedInitialIndex;

	if (!bFluxIsActive)
	{
		SetFluxActive(true);
		if (FluxNodes.Num() < 0) return;

		auto First = FluxNodes[0];
		First->EnableCollision(false);
		RefreshNodeVisibilityOnOwnerMulticast(First.Get(), true);
	}

	/* Move Node */
	RemovePreviousFakePoints(FluxNodeModifiedInitialIndex);
	RemoveNextFakePoints(FluxNodeModifiedInitialIndex);
	LocalReorderIndexes();
	FluxNodeModifiedInitialIndex = FluxNode->GetNodeIndex();
	Index = FluxNode->GetNodeIndex();
	
	MoveNodeMulticast(FluxNode, Target);
	SplineComponent->SetLocationAtSplinePoint(FluxNodeModifiedInitialIndex, Target, ESplineCoordinateSpace::World);

	if (!UseOriginal && OriginalNode != nullptr)
	{
		if (bDebugRemovedNode) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Don't use OriginalNode and OriginalNode is not null"));

		const TWeakObjectPtr<AFluxNode> OriginalNodePtr = TWeakObjectPtr<AFluxNode>(OriginalNode);
		if (OriginalNodePtr.IsValid() && OriginalNode->IsValidLowLevel())
		{
			FluxNodes.Remove(OriginalNode);
			if (OriginalNodePtr.IsValid() && OriginalNode->IsValidLowLevel()) OriginalNode->DestroyNodeByUser();
			Index--;
		}
	}
	
	if (UseOriginal)
	{
		Index = OriginalNode->GetNodeIndex();
		for (auto Path : OriginalToNodePath)
		{
			const auto SpawnPos = Path.PathPoint;
			const auto Transform = FTransform(FRotator::ZeroRotator, SpawnPos, FVector(1, 1, 1));
			const auto SpawnParams = FActorSpawnParameters();
			auto Actor = GetWorld()->SpawnActor(ClassType, &Transform, SpawnParams);
			auto NewFluxNode = Cast<AFluxNode>(Actor);
			if (NewFluxNode == nullptr) return;
			NewFluxNode->Init(this, EFluxNodeType::PathNode, Index, false);
			InsertWeakPtrToFluxNodes(TWeakObjectPtr<AFluxNode>(NewFluxNode), Index, NewFluxNode->GetNodeType() == EFluxNodeType::EndNode);
			Index++;
		}
		OriginalNode->SetNodeIndex(Index);
		OriginalNode->UnsetTmpNode();
		OriginalNode->ChangeTypeForAll(EFluxNodeType::PathNode);
		FluxNodes.Remove(OriginalNode);
		InsertWeakPtrToFluxNodes(TWeakObjectPtr<AFluxNode>(OriginalNode), Index);
		Index++;
	}
	

	/* Insert previous path */
	for (auto Path : BeforePath)
	{
		const auto SpawnPos = Path.PathPoint;
		const auto Transform = FTransform(FRotator::ZeroRotator, SpawnPos, FVector(1, 1, 1));
		const auto SpawnParams = FActorSpawnParameters();
		auto Actor = GetWorld()->SpawnActor(ClassType, &Transform, SpawnParams);
		auto NewFluxNode = Cast<AFluxNode>(Actor);
		if (NewFluxNode == nullptr) return;
		NewFluxNode->Init(this, EFluxNodeType::PathNode, Index, false);
		InsertWeakPtrToFluxNodes(TWeakObjectPtr<AFluxNode>(NewFluxNode), Index, NewFluxNode->GetNodeType() == EFluxNodeType::EndNode);
		Index++;
	}
	FluxNode->SetNodeIndex(Index);
	FluxNodes.Remove(FluxNode);
	InsertWeakPtrToFluxNodes(TWeakObjectPtr<AFluxNode>(FluxNode), Index, FluxNode->GetNodeType() == EFluxNodeType::EndNode);
	auto FluxNodeIndex = FluxNode->GetNodeIndex();
	Index++;
	
	/* Insert next path */
	for (auto Path : AfterPath)
	{
		const auto SpawnPos = Path.PathPoint;
		const auto Transform = FTransform(FRotator::ZeroRotator, SpawnPos, FVector(1, 1, 1));
		const auto SpawnParams = FActorSpawnParameters();
		auto Actor = GetWorld()->SpawnActor(ClassType, &Transform, SpawnParams);
		auto NewFluxNode = Cast<AFluxNode>(Actor);
		if (NewFluxNode == nullptr) return;
		NewFluxNode->Init(this, EFluxNodeType::PathNode, Index, false);
		InsertWeakPtrToFluxNodes(TWeakObjectPtr<AFluxNode>(NewFluxNode), Index, NewFluxNode->GetNodeType() == EFluxNodeType::EndNode);
		Index++;
	}

	for (int i = Index; i < FluxNodes.Num(); i++)
	{
		FluxNodes[i]->SetNodeIndex(i);
	}
	if (FluxNode->GetNodeType() != EFluxNodeType::StartNode) MoveFirstNode();

	if (bFluxNodeNumber) GEngine->AddOnScreenDebugMessage(-1, 25.f, FColor::Green, FString::Printf(TEXT("Flux node moved %d nodes"), FluxNodes.Num()));
	
	ReorderIndexesMulticast();
	RotateNodes();
	RedrawSplines(false, FluxNodeIndex, false, GetEndNodePos());

	if (bFluxNodeNumber) GEngine->AddOnScreenDebugMessage(-1, 25.f, FColor::Green, FString::Printf(TEXT("Flux node moved finished %d nodes"), FluxNodes.Num()));
}

void AFlux::MoveNodePreview(AFluxNode* FluxNode, FVector Target, TArray<FPathStruct> BeforePath,
	TArray<FPathStruct> AfterPath, TArray<FPathStruct> OriginalToNodePath, AFluxNode* OriginalNode, bool UseOriginal)
{
	const auto FluxNodeIndex = FluxNode->GetNodeIndex();
	const auto NextRealNodeIndex = GetNextNodeIndex(FluxNodeIndex);
	const auto PreviousRealNodeIndex = GetPreviousNodeIndex(FluxNodeIndex, UseOriginal);

	PreviewPoints.Empty();
	if (UseOriginal)
	{
		if (PreviousRealNodeIndex >= 0) PreviewPoints.Add(FluxNodes[PreviousRealNodeIndex]->GetActorLocation());
		
		for (auto Path : OriginalToNodePath)
		{
			PreviewPoints.Add(Path.PathPoint);
		}
		PreviewPoints.Add(OriginalNode->GetActorLocation());
		for (auto Path : BeforePath)
		{
			PreviewPoints.Add(Path.PathPoint);
		}
		PreviewPoints.Add(Target);
		for (auto Path : AfterPath)
		{
			PreviewPoints.Add(Path.PathPoint);
		}
		if (NextRealNodeIndex >= 0) PreviewPoints.Add(FluxNodes[NextRealNodeIndex]->GetActorLocation());
	}
	else
	{
		if (PreviousRealNodeIndex >= 0) PreviewPoints.Add(FluxNodes[PreviousRealNodeIndex]->GetActorLocation());
		for (auto Path : BeforePath)
		{
			PreviewPoints.Add(Path.PathPoint);
		}
		PreviewPoints.Add(Target);
		for (auto Path : AfterPath)
		{
			PreviewPoints.Add(Path.PathPoint);
		}
		if (NextRealNodeIndex >= 0) PreviewPoints.Add(FluxNodes[NextRealNodeIndex]->GetActorLocation());
	}

	
	MoveNodeMulticast(FluxNode, Target);
	//MoveNodeMulticastUnreliable(FluxNode, Target);
	if (FluxNode->GetNodeType() != EFluxNodeType::StartNode)
	{
		const auto FirstNode = FluxNodes[0].Get();
		const auto NextValidNodeIndex = GetNextNodeIndex(0);
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("FluxNodeIndex: %d"), FluxNodeIndex));
		if (UseOriginal && FluxNodeIndex == 2) MoveFirstNode(FirstNode, OriginalNode);
		else if (FluxNodeIndex <= 2) MoveFirstNode(FirstNode, FluxNode);
		else if (FluxNodeIndex == NextValidNodeIndex && !UseOriginal) MoveFirstNode(FirstNode, FluxNode);
		else if (FluxNodeIndex == NextValidNodeIndex) MoveFirstNode(FirstNode, FluxNodes[1].Get());
		else if (bDebugStartNodeMove)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("FluxNodeIndex: %d"), FluxNodeIndex));
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("NextValidNodeIndex: %d"), NextValidNodeIndex));
			if (UseOriginal) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("UseOriginalIndex: %d"), OriginalNode->GetNodeIndex()));
		}
	}
	SplineComponent->SetLocationAtSplinePoint(FluxNodeIndex, Target, ESplineCoordinateSpace::World);
	ReorderIndexesMulticast();
	//ReorderIndexesMulticastUnreliable();
	RedrawSplines(false, FluxNodeIndex, false, GetEndNodePos());

	RotateNodes(false);
	//RotateNodesUnreliable(false);
}

void AFlux::MoveFirstNode()
{
	if (FluxNodes.Num() < 2) return;
	const auto FluxNode = FluxNodes[0];
	if (!FluxNode.IsValid())
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("FluxNode is null"));
		return;
	}
	

	const auto FluxNode2 = FluxNodes[1];
	if (!FluxNode2.IsValid())
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("FluxNode2 is null"));
		return;
	}

	MoveFirstNode(FluxNode.Get(), FluxNode2.Get());
}

void AFlux::MoveFirstNode(AFluxNode* FluxNode, AFluxNode* SecondNode, bool CanOverlap)
{
	//DrawDebugSphere(GetWorld(), SecondNode->GetActorLocation(), 500, 10, FColor::Blue, false, 0.f, 0, 5.f);

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation OutNavLoc;
	
	const auto MainBuilding = Cast<AMainBuilding>(Origin);
	const auto TooCloseRadius = MainBuilding->GetTooCloseRadius();
	const auto AllFluxes = MainBuilding->GetFluxes();
	const auto MainBuildingLoc = MainBuilding->GetActorLocation();
	const auto SecondNodeLoc = SecondNode->GetActorLocation();
	const auto DirectionToTarget = (SecondNodeLoc - MainBuildingLoc);
	const auto MainBuildingOffsetRange = MainBuilding->GetOffsetRange();
	
	auto StartLocation = MainBuildingLoc + DirectionToTarget.GetSafeNormal() * MainBuildingOffsetRange;
	auto Direction = (StartLocation - MainBuildingLoc);
	Direction.Z = 0;
	Direction.Normalize();

	bool TargetReachable = false;
	FVector TargetLoc = MainBuildingLoc + Direction * MainBuildingOffsetRange;
	int Offset = 0;
	while (Offset < 190)
	{
		const auto RotatedDir = FQuat(FVector(0, 0, 1), FMath::DegreesToRadians(Offset)).RotateVector(Direction).GetSafeNormal();
		TargetLoc = MainBuildingLoc + RotatedDir * MainBuildingOffsetRange;

		//const auto RotatedVector = FVector(0, 90, 0).Rotation().RotateVector(TargetLoc);
		
		TargetReachable = NavSys->ProjectPointToNavigation(TargetLoc, OutNavLoc, FVector(20, 20, 100));
		if (bDebugStartNodeMove)
		{
			DrawDebugSphere(GetWorld(), TargetLoc, 500, 10, TargetReachable ? FColor::Cyan : FColor::Purple, false, 0.f, 0, 5.f);
			DrawDebugLine(GetWorld(), MainBuildingLoc, TargetLoc, FColor::Emerald, false, 0.f, 0, 15.f);
		}
		if (TargetReachable)
		{
			auto IsTooClose = false;

			if (TargetReachable)
			{
				if (CanOverlap) break; 
				for (auto Flux : AllFluxes)
				{
					if (!Flux.IsValid()) continue;
					const auto FluxFirstNode = Flux->GetFirstNode();
					if (!FluxFirstNode.IsValid()) continue;
					if (FluxFirstNode.Get() == FluxNode) continue;
					const auto FluxNodeLoc = FluxFirstNode->GetActorLocation();
					const auto Distance = (TargetLoc - FluxNodeLoc).Size();
					if (Distance < TooCloseRadius)
					{
						IsTooClose = true;
						break;
					}
				}
			}
			if (!IsTooClose) break;
		}

		if (Offset == 0) Offset = 10;
		else if (Offset < 0) Offset = Offset * -1 + 10;
		else Offset *= -1;
	}

	if (TargetReachable)
	{
		MoveNodeMulticast(FluxNode, TargetLoc);
		const auto Rotation = (SecondNode->GetActorLocation() - FluxNode->GetActorLocation()).Rotation();
		RotateNodeMulticast(FluxNode, Rotation);
		return;
	}

	
	const auto MainBuildingDirAngle = MainBuilding->GetFluxMidAngleNormalized();
	const auto Tolerance = MainBuilding->GetAngleToleranceFlux();

	const auto Dot = FVector::DotProduct(Direction, MainBuildingDirAngle);
	if (Dot < Tolerance)
	{
		/* Let's clamp it to the tolerance in the right direction */
		auto RotatedDir = FVector(0, 90, 0).Rotation().RotateVector(MainBuildingDirAngle).GetSafeNormal();
		const auto Dot2 = FVector::DotProduct(Direction, RotatedDir);
		const auto HalfAngle = MainBuilding->GetAngleAroundForFluxes() * 0.5f;

		const auto StartingAngleDegree = MainBuilding->GetStartingAngle();
		const auto Rad = FMath::DegreesToRadians(StartingAngleDegree + (Dot2 > 0 ? HalfAngle : -HalfAngle));
		Direction = FVector(FMath::Cos(Rad), FMath::Sin(Rad), 0);
		if (bDebugStartNodeMove)
		{
			DrawDebugSphere(GetWorld(), StartLocation, 250, 10, FColor::Black, false, 0.f, 0, 5.f);
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Dot: %f, tolerance %f"), Dot, Tolerance));
		}
	}
	else if (bDebugStartNodeMove)
	{
		DrawDebugSphere(GetWorld(), StartLocation, 250, 10, FColor::Blue, false, 0.f, 0, 5.f);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Dot: %f, tolerance %f"), Dot, Tolerance));
	}
	StartLocation = MainBuildingLoc + Direction.GetSafeNormal() * MainBuildingOffsetRange;
	FluxNode->SetActorLocation(StartLocation);
	MoveNodeMulticast(FluxNode, StartLocation);
	if (bDebugStartNodeMove)
	{
		DrawDebugSphere(GetWorld(), StartLocation, 250, 10, FColor::Yellow, false, 0.f, 0, 5.f);
		DrawDebugLine(GetWorld(), StartLocation, StartLocation+FVector(0,0,1000), FColor::Yellow, false, 0.f, 0, 5.f);
	}
	const auto Rotation = (SecondNode->GetActorLocation() - FluxNode->GetActorLocation()).Rotation();
	RotateNodeMulticast(FluxNode, Rotation);
}

void AFlux::CallCreatedNode(AFluxNode* FluxNode)
{
	FluxNodeCreated.Broadcast(this, FluxNode);
}

AFluxNode* AFlux::CreateNodeBefore(AFluxNode* NextFluxNode, bool RealNode, bool bIsTmp)
{
	const auto Index = NextFluxNode->GetNodeIndex();
	const auto Target = NextFluxNode->GetActorLocation();

	UClass* ClassType = FluxNodeSpawnClass->GetDefaultObject()->GetClass();
	const auto SpawnParams = FActorSpawnParameters();
	const auto Transform = FTransform(FRotator::ZeroRotator, Target, FVector(1, 1, 1));
	
	auto Actor = GetWorld()->SpawnActor(ClassType, &Transform, SpawnParams);
	auto FluxNode = Cast<AFluxNode>(Actor);
	if (FluxNode == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, TEXT("CreateNodeBefore::FluxNode is null"));
		return nullptr;
	}
	int NewIndex = Index;
	const auto WeakPtr = TWeakObjectPtr<AFluxNode>(FluxNode);
	if (!WeakPtr.IsValid())
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, TEXT("CreateNodeBefore::FluxNode is null"));
		return nullptr;
	}

	if (!FluxNodes.IsValidIndex(NewIndex))
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, TEXT("CreateNodeBefore::NewIndex is invalid"));
		return nullptr;
	}

	InsertWeakPtrToFluxNodes(WeakPtr, NewIndex, FluxNode->GetNodeType() == EFluxNodeType::EndNode);
	auto NodeType = bIsTmp ? EFluxNodeType::TmpNode : EFluxNodeType::PathNode;
	FluxNode->Init(this, NodeType, NewIndex, RealNode);

	NewIndex = SplineComponent->GetNumberOfSplinePoints();
	FSplinePoint SplinePoint = FSplinePoint(NewIndex, Target, SplinePointType);
	SplineComponent->AddPoint(SplinePoint, ESplineCoordinateSpace::World);
	FluxNodeAdded.Broadcast(this, NewIndex);

	if (NextFluxNode->GetNodeType() == EFluxNodeType::StartNode)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("NextFluxNode name: %s"), *NextFluxNode->GetName()));
		NextFluxNode->ChangeTypeForAll(EFluxNodeType::PathNode);
	}
	
	ReorderIndexesMulticast();
	RedrawSplines(false, NewIndex, false, GetEndNodePos());
	return FluxNode;
}

void AFlux::ReorderLocalIndexes()
{
	for (int i = 0; i < FluxNodes.Num(); i++)
	{
		//if (FluxNodes.Num() <= i) continue;
		auto FluxNode = FluxNodes[i];
		if (FluxNode == nullptr)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, TEXT("FluxNode is null in reorder indexes"));
			continue;
		}
		FluxNode->SetNodeIndex(i);
	}
}

void AFlux::SetSplinePointsLocal(const TArray<FVector>& Points, const bool bFinishChanges, int PointsIndexModified,
	const TArray<FVector>& PointsPreview, const bool DontDrawLast, const FVector& LastPointPos)
{
	SplineComponent->ClearSplinePoints();
	if (bFinishChanges) SplineForAmalgamsComponent->ClearSplinePoints();
	
	FSplinePoint SplinePoint = FSplinePoint();
	int i = 0;
	for (auto Point : Points)
	{
		SplinePoint = FSplinePoint(i, Point, SplinePointType);
		SplineComponent->AddPoint(SplinePoint, ESplineCoordinateSpace::World);
		if (bFinishChanges) SplineForAmalgamsComponent->AddPoint(SplinePoint, ESplineCoordinateSpace::World);
		i++;
	}
	SplineComponent->UpdateSpline();
	if (bFinishChanges) RedrawFluxFinishedBP(PointsIndexModified, DontDrawLast, LastPointPos);
	else RedrawFluxPreviewBP(PointsIndexModified, PointsPreview, DontDrawLast);

	
	if (bFinishChanges) SplineForAmalgamsComponent->UpdateSpline();
	FluxUpdated.Broadcast(this);
	if (bFinishChanges)
	{
		bPathfindingIsRight = false;
		CalculatePathfinding();
		FluxFinishUpdate.Broadcast(this);
	}
}

void AFlux::VerifyFluxValidity()
{
	if (!HasAuthority()) return;
	ValidityCheckThisFrame++;
	if (ValidityCheckThisFrame > 4) return;
	
	bool StartNodeFound = false;
	bool EndNodeFound = false;

	AFluxNode* StartNode = nullptr;
	AFluxNode* EndNode = nullptr;

	for (const auto FluxNode : FluxNodes)
	{
		if (!FluxNode.IsValid()) continue;
		if (FluxNode->GetNodeType() == EFluxNodeType::StartNode)
		{
			StartNodeFound = true;
			StartNode = FluxNode.Get();
		}
		else if (FluxNode->GetNodeType() == EFluxNodeType::EndNode)
		{
			EndNodeFound = true;
			EndNode = FluxNode.Get();
		}
	}

	if (StartNodeFound && EndNodeFound)
	{
		/* RefreshVisibility BP and it's ok */
		StartNode->CallNodeTypeUpdated();
		EndNode->CallNodeTypeUpdated();
		return;
	}

	if (!StartNodeFound)
	{
		/* Could create a new StartNode */
		UClass* ClassType = FluxNodeSpawnClass->GetDefaultObject()->GetClass();
		const auto SpawnParams = FActorSpawnParameters();
		const auto Location = Origin->GetActorLocation(); /* Could be improved */
		
		auto Transform = FTransform(FRotator::ZeroRotator, Location, FVector(1, 1, 1));
		AFluxNode* FluxNode = Cast<AFluxNode>(GetWorld()->SpawnActor(ClassType, &Transform, SpawnParams));
		const auto NodeType = EFluxNodeType::StartNode;
		FluxNode->Init(this, NodeType, 1, true);
		InsertWeakPtrToFluxNodes(TWeakObjectPtr<AFluxNode>(FluxNode), 0, false);
		
	}
	if (!EndNodeFound)
	{
		/* Create a new EndNode */
		UClass* ClassType = FluxNodeSpawnClass->GetDefaultObject()->GetClass();
		const auto SpawnParams = FActorSpawnParameters();
		const auto Location = StartNodeFound ? StartNode->GetActorLocation() : FVector::ZeroVector;
		
		auto Transform = FTransform(FRotator::ZeroRotator, Location, FVector(1, 1, 1));
		AFluxNode* FluxNode = Cast<AFluxNode>(GetWorld()->SpawnActor(ClassType, &Transform, SpawnParams));
		const auto NodeType = EFluxNodeType::EndNode;
		FluxNode->Init(this, NodeType, 1, true);
		InsertWeakPtrToFluxNodes(TWeakObjectPtr<AFluxNode>(FluxNode), 1, true);
	}
	ResetFluxServer();

}

FVector AFlux::GetEndNodePos() const
{
	if (FluxNodes.Num() == 0) return FVector::ZeroVector;

	for (int i = FluxNodes.Num() - 1; i >= 0; i--)
	{
		if (!FluxNodes[i].IsValid()) continue;
		const auto FluxNode = FluxNodes[i].Get();
		if (FluxNode->GetNodeType() != EFluxNodeType::EndNode) continue;
		return FluxNode->GetActorLocation();
		
	}
	return FVector::ZeroVector;
}

FVector AFlux::GetBeforeEndNodePos() const
{
	if (FluxNodes.Num() == 0) return FVector::ZeroVector;
	bool EndFound = false;

	for (int i = FluxNodes.Num() - 1; i >= 0; i--)
	{
		if (!FluxNodes[i].IsValid()) continue;
		const auto FluxNode = FluxNodes[i].Get();
		const auto FluxNodeType = FluxNode->GetNodeType();
		if (FluxNodeType == EFluxNodeType::EndNode)
		{
			EndFound = true;
			continue;
		}
		if (!EndFound) continue;
		if (FluxNodeType != EFluxNodeType::PathNode) continue;
		return FluxNode->GetActorLocation();
		
		
	}
	return FVector::ZeroVector;
}

AFluxNode* AFlux::InsertNodes(TArray<FPathStruct> PathStructs, int IndexToInsert, bool MakeLastNodeReal)
{
	AFluxNode* LastNodeCreated = nullptr;
	if (IndexToInsert < 0 || IndexToInsert > FluxNodes.Num())
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("IndexToInsert is out of range in InsertNodes"));
		return LastNodeCreated;
	}
	if (PathStructs.Num() == 0)
	{
		if (bDebugRemovedNode) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PathStructs is empty in InsertNodes"));
		return LastNodeCreated;
	}

	if (!IsFluxActive())
	{
		SetFluxActive(true);
	}

	if (MakeLastNodeReal)
	{
		RemovePreviousFakePoints(IndexToInsert);
		ReorderLocalIndexes();

		const auto LastNodeIndex = FluxNodes.Last()->GetNodeIndex();
		IndexToInsert = GetPreviousNodeIndex(LastNodeIndex, true) + 1;
	}

	
	UClass* ClassType = FluxNodeSpawnClass->GetDefaultObject()->GetClass();
	const auto SpawnParams = FActorSpawnParameters();
	auto Transform = FTransform(FRotator::ZeroRotator, PathStructs[0].PathPoint, FVector(1, 1, 1));

	for (int i = 0; i < PathStructs.Num(); i++)
	{
		FVector SpawnPos = PathStructs[i].PathPoint;
		Transform.SetLocation(SpawnPos);
		AFluxNode* FluxNode = Cast<AFluxNode>(GetWorld()->SpawnActor(ClassType, &Transform, SpawnParams));

		const auto NodeType = i == PathStructs.Num()-1 && MakeLastNodeReal ? EFluxNodeType::PathNode : EFluxNodeType::FakeNode;
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("NodeType %d"), static_cast<int>(NodeType)));
		FluxNode->Init(this, NodeType, IndexToInsert + i, PathStructs[i].IsReal);
		InsertWeakPtrToFluxNodes(TWeakObjectPtr<AFluxNode>(FluxNode), IndexToInsert + i, NodeType == EFluxNodeType::EndNode);
		LastNodeCreated = FluxNode;

		//DrawDebugSphere(GetWorld(), SpawnPos, 200, 10, FColor::Blue, false, 5.f, 0, 5.f);
	}

	
	ReorderIndexesMulticast();
	FVector Pos = GetEndNodePos();
	if (MakeLastNodeReal)
	{
		Pos = GetBeforeEndNodePos();
	}
	RedrawSplines(true, IndexToInsert, MakeLastNodeReal, Pos);
	return LastNodeCreated;
}

USplineComponent* AFlux::GetSplineComponent() const
{
	return SplineComponent;
}

USplineComponent* AFlux::GetSplineForAmalgamsComponent() const
{
	return SplineForAmalgamsComponent;
}

TWeakObjectPtr<AFluxNode> AFlux::GetEndNode() const
{
	if (FluxNodes.Num() == 0) return nullptr;
	return FluxNodes.Last();
}

TWeakObjectPtr<AFluxNode> AFlux::GetFirstNode() const
{
	if (FluxNodes.Num() == 0) return nullptr;
	return FluxNodes[0];
}

bool AFlux::IsPathfindingRight() const
{
	return bPathfindingIsRight;
}

TArray<FVector> AFlux::GetPath() const
{
	return FluxPath;
}

void AFlux::SetFluxActive(bool bActivate)
{
	if (bFluxIsActive == bActivate) return;
	FluxEnabled.Broadcast(this, bActivate);
	SetFluxActiveMulticast(bActivate);
}

void AFlux::ForceFluxActiveVisibility(bool bActivate)
{
	ForceVisibilityFluxActiveMulticast(bActivate);
}

bool AFlux::IsFluxActive() const
{
	return bFluxIsActive;
}

void AFlux::InteractStartHoverFlux(APlayerControllerInfernale* Interactor)
{
	if (Interactor->bIsEscapeMenuOpen)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Emerald, TEXT("Escape is open"));
		return;
	}
	IInteractable::InteractStartHover(Interactor);
	bHovered = true;
	OnFluxHovered();
}

void AFlux::InteractEndHoverFlux(APlayerControllerInfernale* Interactor)
{
	IInteractable::InteractEndHover(Interactor);
	bHovered = false;
	OnFluxEndHovered();
}

FOwner AFlux::GetOwnerInfo()
{
	if (!Origin) return FOwner();
	return Origin->GetOwner();
}

int AFlux::GetNumberOfNodeBeforeFirstPath()
{
	int Count = 0;
	for (int i = FluxNodes.Num() - 1; i >= 0; i--)
	{
		const auto FluxNode = FluxNodes[i];
		if (!FluxNode.IsValid()) continue;
		if (FluxNode->GetNodeType() == EFluxNodeType::PathNode) break;
		Count++;
	}
	return Count;
}

AFluxNode* AFlux::GetLastNodeNotEnd(bool bIgnoreEnd)
{
	for (int i = FluxNodes.Num() - 1; i >= 0; i--)
	{
		const auto FluxNode = FluxNodes[i];
		if (!FluxNode.IsValid()) continue;
		const auto FluxType = FluxNode->GetNodeType();
		if (FluxType == EFluxNodeType::EndNode && !bIgnoreEnd) return FluxNode.Get();
		if (FluxType == EFluxNodeType::PathNode || FluxType == EFluxNodeType::BaseNode) return FluxNode.Get();
	}
	return nullptr;
}

uint32 AFlux::GetUpdateID()
{
	return UpdateID;
}

uint32 AFlux::GetUpdateVersion()
{
	return UpdateVersion;
}

float AFlux::GetAmalgamsSpeedMult()
{
	return AmalgamsSpeedMult;
}

void AFlux::SetAmalgamsSpeedMult(float NewSpeed)
{
	AmalgamsSpeedMult = NewSpeed;
	SetAmalgamsSpeedMultMulticast(NewSpeed);
	UpdateID++;
}

void AFlux::InsertWeakPtrToFluxNodes(TWeakObjectPtr<AFluxNode> FluxNode, int Index, bool bIsLast)
{
	const auto NodesNum = FluxNodes.Num();
	if (!bIsLast && Index >= NodesNum) Index = NodesNum - 1;
	FluxNodes.Insert(FluxNode, Index);
}

TArray<float> AFlux::GetFluxPower()
{
	TArray<float> Output;
	Output.Add(FluxPowerDemon);
	Output.Add(FluxPowerBuilding);
	Output.Add(FluxPowerMonster);
	return Output;
}

void AFlux::SetFluxPowers(float NewDemonPower, float NewBuildingPower, float NewMonsterPower, int NewPortalCount)
{
	SetFluxPowersMulticast(NewDemonPower, NewBuildingPower, NewMonsterPower, NewPortalCount);
}

// Called when the game starts or when spawned
void AFlux::BeginPlay()
{
	Super::BeginPlay();
	//SetReplicates(true);
	SyncDataAsset();
	if (!HasAuthority()) return;
	RotateNodes();

    const auto GameMode = Cast<AGameModeInfernale>(GetWorld()->GetAuthGameMode());
	GameMode->PreLaunchGame.AddDynamic(this, &AFlux::OnPreLauchGame);
	GameMode->LaunchGame.AddDynamic(this, &AFlux::OnLauchGame);
}

void AFlux::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AFlux::MoveNodeMulticast_Implementation(AFluxNode* FluxNode, FVector Target)
{
	if (FluxNode == nullptr) return;
	FluxNode->SetActorLocation(Target);
}

void AFlux::RotateNodeMulticast_Implementation(AFluxNode* FluxNode, FRotator Target)
{
	if (FluxNode == nullptr) return;
	FluxNode->SetActorRotation(Target);
}

void AFlux::SyncDataAsset()
{
	if (!bUseDataAsset) return;
	if (!FluxSettingsDataAsset)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("AFlux::FluxSettingsDataAsset is not set"));
		return;
	}

	FluxNodeSpawnClass = FluxSettingsDataAsset->FluxNodeSpawnClass;
	TooCloseRange = FluxSettingsDataAsset->MinRangeBetweenNodesOnCreation;
}


void AFlux::RedrawSplines(bool bFinishChanges, int PointsIndexModified, bool DontDrawLast, const FVector& LastPointPos)
{
	TArray<FVector> Points = TArray<FVector>();
	for (int i = 0; i < SplineComponent->GetNumberOfSplinePoints(); i++)
	{
		Points.Add(SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World));
		// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Point %d : %s"), i, *Points[i].ToString()));
		// UE_LOG(LogTemp, Warning, TEXT("Point %d : %s"), i, *Points[i].ToString());
	}
	// UE_LOG(LogTemp, Warning, TEXT("Points : %d"), Points.Num());
	// UE_LOG(LogTemp, Warning, TEXT("-----------------------------"));
	if (bFinishChanges)
	{
		SetSplinePointsLocal(Points, bFinishChanges, PointsIndexModified, PreviewPoints, DontDrawLast, LastPointPos);
		SetSplinePointsMulticast(Points, bFinishChanges, PointsIndexModified, PreviewPoints, DontDrawLast, LastPointPos);
		return;
	}

	if (!bCanReplicateNow) return;

	bCanReplicateNow = false;
	CurrentTime = 0.f;
	
	SetSplinePointsLocal(Points, bFinishChanges, PointsIndexModified, PreviewPoints, DontDrawLast, LastPointPos);
	SetSplinePointsMulticast(Points, bFinishChanges, PointsIndexModified, PreviewPoints, DontDrawLast, LastPointPos);
}

void AFlux::RedrawSplinesUnreliable(bool bFinishChanges, int PointsIndexModified, bool DontDrawLast, const FVector& LastPointPos)
{
	TArray<FVector> Points = TArray<FVector>();
	for (int i = 0; i < SplineComponent->GetNumberOfSplinePoints(); i++)
	{
		Points.Add(SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World));
		// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Point %d : %s"), i, *Points[i].ToString()));
		// UE_LOG(LogTemp, Warning, TEXT("Point %d : %s"), i, *Points[i].ToString());
	}
	// UE_LOG(LogTemp, Warning, TEXT("Points : %d"), Points.Num());
	// UE_LOG(LogTemp, Warning, TEXT("-----------------------------"));
	if (bFinishChanges)
	{
		SetSplinePointsLocal(Points, bFinishChanges, PointsIndexModified, PreviewPoints, DontDrawLast, LastPointPos);
		SetSplinePointsMulticastUnreliable(Points, bFinishChanges, PointsIndexModified, PreviewPoints, DontDrawLast, LastPointPos);
		return;
	}

	if (!bCanReplicateNow) return;

	bCanReplicateNow = false;
	CurrentTime = 0.f;
	
	SetSplinePointsLocal(Points, bFinishChanges, PointsIndexModified, PreviewPoints, DontDrawLast, LastPointPos);
	SetSplinePointsMulticastUnreliable(Points, bFinishChanges, PointsIndexModified, PreviewPoints, DontDrawLast, LastPointPos);
}

void AFlux::CalculatePathfinding()
{
	FluxPath.Empty();
	if (!SplineForAmalgamsComponent)
	{
		return;
	}
	if (!this) return;
	const auto StartPoint = SplineForAmalgamsComponent->GetLocationAtDistanceAlongSpline(0.f, ESplineCoordinateSpace::World);
	const auto MaxDistance = SplineForAmalgamsComponent->GetSplineLength();
	auto CurrentDistance = 0.f;
	
	FluxPath.Add(StartPoint);
	if (DebugShowFluxPath) DrawDebugSphere(GetWorld(), StartPoint, 50, 10, FColor::Green, false, 5.f);

	while (CurrentDistance < MaxDistance)
	{
		CurrentDistance += SpaceBetweenPathfindingNodes;
		CurrentDistance = FMath::Clamp(CurrentDistance, 0.f, MaxDistance);
		auto NextPoint = SplineForAmalgamsComponent->GetLocationAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::World);
		if (DebugShowFluxPath)
		{
			DrawDebugSphere(GetWorld(), NextPoint, 50, 10, FColor::Blue, false, 5.f);
			DrawDebugLine(GetWorld(), FluxPath.Last(), NextPoint, FColor::Blue, false, 5.f, 0, 5.f);
		}
		FluxPath.Add(NextPoint);
	}
	auto NextPoint = SplineForAmalgamsComponent->GetLocationAtDistanceAlongSpline(MaxDistance, ESplineCoordinateSpace::World);
	if (DebugShowFluxPath)
	{
		DrawDebugSphere(GetWorld(), NextPoint, 50, 10, FColor::Blue, false, 5.f);
		DrawDebugLine(GetWorld(), FluxPath.Last(), NextPoint, FColor::Blue, false, 5.f, 0, 5.f);
	}
	FluxPath.Add(NextPoint);
	bPathfindingIsRight = true;

	++UpdateID;

	FluxPathRecalculated.Broadcast(this);
}

void AFlux::RemovePreviousFakePoints(int IndexLastRealNode)
{
	TArray<TWeakObjectPtr<AFluxNode>> NodesToRemove;
	bool NodeFound = false;
	for (int i = FluxNodes.Num() - 1; i > 0; i--)
	{
		auto NodeIsReal = FluxNodes[i]->IsReal();
		if (NodeIsReal && !NodeFound)
		{
			auto NodeIndex = FluxNodes[i]->GetNodeIndex();
			if (NodeIndex == IndexLastRealNode)
			{
				NodeFound = true;
				continue;
			}
		}
		if (!NodeFound) continue;
		if (NodeIsReal && !FluxNodes[i]->IsTmpNode()) break;
		
		NodesToRemove.Add(FluxNodes[i]);
	}

	for (auto ToRemove : NodesToRemove)
	{
		if (ToRemove->IsTmpNode()) continue;
		FluxNodes.Remove(ToRemove);
		if (bDebugRemovedNode) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Cleanup : Fake Node removed"));
		ToRemove->DestroyNodeByUser();
	}
}

void AFlux::RemoveNextFakePoints(int IndexLastRealNode)
{
	TArray<TWeakObjectPtr<AFluxNode>> NodesToRemove = TArray<TWeakObjectPtr<AFluxNode>>();
	bool NodeFound = false;
	for (int i = 0; i < FluxNodes.Num(); i++)
	{
		auto NodeIsReal = FluxNodes[i]->IsReal();
		if (NodeIsReal && !NodeFound)
		{
			auto NodeIndex = FluxNodes[i]->GetNodeIndex();
			if (NodeIndex == IndexLastRealNode)
			{
				NodeFound = true;
				continue;
			}
		}
		if (!NodeFound) continue;
		if (NodeIsReal && !FluxNodes[i]->IsTmpNode()) break;
		
		NodesToRemove.Add(FluxNodes[i]);
	}

	for (auto ToRemove : NodesToRemove)
	{
		if (ToRemove->IsTmpNode()) continue;
		FluxNodes.Remove(ToRemove);
		if (bDebugRemovedNode) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Cleanup : Fake Node removed"));
		ToRemove->DestroyNodeByUser();
	}
	VerifyFluxValidity();
}

void AFlux::LocalReorderIndexes()
{
	bool WrongReorder = false;
	for (int i = 0; i < FluxNodes.Num(); i++)
	{
		auto FluxNode = FluxNodes[i];
		if (FluxNode == nullptr)
		{
			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, TEXT("FluxNode is null in reorder indexes"));
			WrongReorder = true;
			continue;
		}
		FluxNode->SetNodeIndex(i);
	}
	
	if (!WrongReorder) return;
	
	if (bFluxNodeNumber) GEngine->AddOnScreenDebugMessage(-1, 25.f, FColor::Green, FString::Printf(TEXT("Pre reorder %d nodes"), FluxNodes.Num()));
	for (int i = FluxNodes.Num() - 1; i > 0; i--)
	{
		if (FluxNodes[i] != nullptr) continue;
		FluxNodes.Remove(FluxNodes[i]);
	}
	if (bFluxNodeNumber) GEngine->AddOnScreenDebugMessage(-1, 25.f, FColor::Green, FString::Printf(TEXT("Post reorder %d nodes"), FluxNodes.Num()));
	
	VerifyFluxValidity();
	LocalReorderIndexes();
}

TArray<AFluxNode*> AFlux::MakeFluxNodesRefs()
{
	TArray<AFluxNode*> NodesRefs = TArray<AFluxNode*>();
	for (auto FluxNode : FluxNodes)
	{
		NodesRefs.Add(FluxNode.Get());
	}
	return NodesRefs;
}

void AFlux::RotateNodes(bool RotateStart)
{
	if (FluxNodes.Num() <= 1) return;
	auto FirstNode = FluxNodes[0];
	if (FirstNode == nullptr) return;
	auto LastNode = FluxNodes.Last();
	if (LastNode == nullptr) return;
	
	auto NextNode = FluxNodes[1];
	if (NextNode == nullptr) return;
	auto Rotation = (NextNode->GetActorLocation() - FirstNode->GetActorLocation()).Rotation();
	if (RotateStart) RotateNodeMulticast(FirstNode.Get(), Rotation);
	
	if (FluxNodes.Num() <= 1) return;
	auto PreviousNode = FluxNodes[FluxNodes.Num() - 2];
	if (PreviousNode == nullptr) return;
	if (PreviousNode->GetNodeType() == EFluxNodeType::TmpNode)
	{
		if (FluxNodes.Num() <= 2) return;
		PreviousNode = FluxNodes[FluxNodes.Num() - 3];
	}
	Rotation = (LastNode->GetActorLocation() - PreviousNode->GetActorLocation()).Rotation();
	RotateNodeMulticast(LastNode.Get(), Rotation);
}

void AFlux::RefreshVisibilityDisabledFluxes()
{
	auto PlayerController = GetWorld()->GetFirstPlayerController();
	auto PlayerControllerInfernale = Cast<APlayerControllerInfernale>(PlayerController);
	if (PlayerControllerInfernale == nullptr) return;
	auto NewOwner = GetOwnerInfo();
	const auto RightPlayer = NewOwner.Player == PlayerControllerInfernale->GetOwnerInfo().Player;

	for (const auto FluxNode : FluxNodes)
	{
		if (!FluxNode.IsValid()) continue;
		const auto NodeType = FluxNode->GetNodeType();
		const auto NodeTypeEnd = NodeType == EFluxNodeType::EndNode;
		
		FluxNode->RefreshNodeVisibilityBP(RightPlayer && NodeTypeEnd);
	}
}

void AFlux::OnPreLauchGame()
{
	PreLauchGameMulticast(MakeFluxNodesRefs());
}

void AFlux::OnLauchGame()
{
	PreLauchGameMulticast(MakeFluxNodesRefs());
}

void AFlux::RotateNodeMulticastUnreliable_Implementation(AFluxNode* FluxNode, FRotator Target)
{
	if (FluxNode == nullptr) return;
	FluxNode->SetActorRotation(Target);
}

void AFlux::RotateNodesUnreliable(bool RotateStart)
{
	if (FluxNodes.Num() <= 1) return;
	auto FirstNode = FluxNodes[0];
	if (FirstNode == nullptr) return;
	auto LastNode = FluxNodes.Last();
	if (LastNode == nullptr) return;
	
	auto NextNode = FluxNodes[1];
	if (NextNode == nullptr) return;
	auto Rotation = (NextNode->GetActorLocation() - FirstNode->GetActorLocation()).Rotation();
	if (RotateStart) RotateNodeMulticastUnreliable(FirstNode.Get(), Rotation);
	
	if (FluxNodes.Num() <= 1) return;
	auto PreviousNode = FluxNodes[FluxNodes.Num() - 2];
	if (PreviousNode == nullptr) return;
	if (PreviousNode->GetNodeType() == EFluxNodeType::TmpNode)
	{
		if (FluxNodes.Num() <= 2) return;
		PreviousNode = FluxNodes[FluxNodes.Num() - 3];
	}
	Rotation = (LastNode->GetActorLocation() - PreviousNode->GetActorLocation()).Rotation();
	RotateNodeMulticastUnreliable(LastNode.Get(), Rotation);
}

void AFlux::SetSplinePointsMulticastUnreliable_Implementation(const TArray<FVector>& Points, const bool bFinishChanges,
                                                              int PointsIndexModified, const TArray<FVector>& PointsPreview, const bool DontDrawLast, const FVector& LastPointPos)
{
	if (HasAuthority()) return;
	SetSplinePointsLocal(Points, bFinishChanges, PointsIndexModified, PointsPreview, DontDrawLast, LastPointPos);
}

void AFlux::MoveNodeMulticastUnreliable_Implementation(AFluxNode* FluxNode, FVector Target)
{
	if (FluxNode == nullptr) return;
	FluxNode->SetActorLocation(Target);
}

void AFlux::ReorderIndexesMulticastUnreliable_Implementation()
{
	TArray<FSplinePoint> Points = TArray<FSplinePoint>();
	FSplinePoint InitialPoint = FSplinePoint();
	InitialPoint.InputKey = 0;

	if (Origin != nullptr)
	{
		InitialPoint.Position = Origin->GetActorLocation();
		Points.Add(InitialPoint);
	}
	
	for (int i = 0; i < FluxNodes.Num(); i++)
	{
		//if (FluxNodes.Num() <= i) continue;
		auto FluxNode = FluxNodes[i];
		if (FluxNode == nullptr)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, TEXT("FluxNode is null in reorder indexes"));
			continue;
		}
		FluxNode->SetNodeIndex(i);
		FSplinePoint Point = FSplinePoint();
		if (i+1 < SplineComponent->GetNumberOfSplinePoints())
		{
			Point = SplineComponent->GetSplinePointAt(i, ESplineCoordinateSpace::World);
		}
		Point.InputKey = i+1;
		Point.Position = FluxNode->GetActorLocation();
		Points.Add(Point);
	}
	
	SplineComponent->ClearSplinePoints();
	for (auto Point : Points)
	{
		SplineComponent->AddPoint(Point, ESplineCoordinateSpace::World);
	}
}

void AFlux::ChangeNodeTypeMulticast_Implementation(AFluxNode* Node, EFluxNodeType NewType)
{
	if (Node == nullptr) return;
	Node->ChangeType(NewType);
}

void AFlux::PreLauchGameMulticast_Implementation(const TArray<AFluxNode*>& NewFluxNodes)
{
	if (!HasAuthority()) SetFluxNodesLocal(NewFluxNodes);
	if (FluxNodes.Num() <= 2) return;
	const auto FirstNode = FluxNodes[0];
	const auto LastNode = FluxNodes.Last();

	if (FirstNode.IsValid()) FirstNode->ChangeType(EFluxNodeType::StartNode);
	if (LastNode.IsValid()) LastNode->ChangeType(EFluxNodeType::EndNode);
	SetFluxActive(false);
}

void AFlux::SetFluxPowersMulticast_Implementation(float NewDemonPower, float NewBuildingPower, float NewMonsterPower, int NewPortalCount)
{
	FluxPowerDemon = NewDemonPower;
	FluxPowerBuilding = NewBuildingPower;
	FluxPowerMonster = NewMonsterPower;
	PortalCount = NewPortalCount;
}

void AFlux::SetAmalgamsSpeedMultMulticast_Implementation(float NewSpeed)
{
	if (HasAuthority()) return;
	AmalgamsSpeedMult = NewSpeed;
}

void AFlux::SetFluxNodesMulticast_Implementation(const TArray<AFluxNode*>& NewFluxNodes)
{
	if (HasAuthority()) return;
	SetFluxNodesLocal(NewFluxNodes);
}

void AFlux::SetFluxNodesLocal(const TArray<AFluxNode*>& NewFluxNodes)
{
	FluxNodes.Empty();
	for (auto FluxNode : NewFluxNodes)
	{
		if (FluxNode == nullptr) continue;
		FluxNodes.Add(TWeakObjectPtr<AFluxNode>(FluxNode));
	}

	TWeakObjectPtr<AFluxNode> FirstNode = nullptr;
	TWeakObjectPtr<AFluxNode> LastNode = nullptr;
	
	if (FluxNodes.Num() > 0) FirstNode = FluxNodes[0];
	if (FluxNodes.Num() > 1) LastNode = FluxNodes.Last();

	if (FirstNode.IsValid()) FirstNode->ChangeType(EFluxNodeType::StartNode);
	if (LastNode.IsValid()) LastNode->ChangeType(EFluxNodeType::EndNode);
    
	for (auto FluxNode : FluxNodes)
	{
		if (FluxNode == nullptr) continue;
		FluxNode->CallNodeTypeUpdated();
	}
	VerifyFluxValidity();
}

void AFlux::RefreshVisibilityDisabledFluxesMulticast_Implementation()
{
	RefreshVisibilityDisabledFluxes();
}

void AFlux::RefreshNodeVisibilityOnOwnerMulticast_Implementation(AFluxNode* FluxNode, bool bVisible)
{
	if (FluxNode == nullptr) return;
	auto PlayerController = GetWorld()->GetFirstPlayerController();
	auto PlayerControllerInfernale = Cast<APlayerControllerInfernale>(PlayerController);
	if (PlayerControllerInfernale == nullptr) return;
	auto OwnerInfo = GetOwnerInfo();
	if (OwnerInfo.Player != PlayerControllerInfernale->GetOwnerInfo().Player) return;
	FluxNode->RefreshNodeVisibilityBP(bVisible);
}

void AFlux::ResetFluxMulticast_Implementation()
{
	ResetFluxBP();
	RefreshVisibilityDisabledFluxes();
}

void AFlux::RefreshNodeVisibilityMulticast_Implementation()
{
	auto OwnerInfo = GetOwnerInfo();
	auto PlayerController = GetWorld()->GetFirstPlayerController();
	auto PlayerControllerInfernale = Cast<APlayerControllerInfernale>(PlayerController);
	if (PlayerControllerInfernale == nullptr) return;

	bool Visible = OwnerInfo.Player == PlayerControllerInfernale->GetOwnerInfo().Player;
	if (!Visible && bDebugVisibility)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Owner : %d vs PC %d"), OwnerInfo.Player, PlayerControllerInfernale->GetOwnerInfo().Player));
	}

	for (auto FluxNode : FluxNodes)
    {
        if (FluxNode == nullptr) continue;
        
        const bool CantBeShown = FluxNode->GetNodeType() == EFluxNodeType::StartNode && !IsFluxActive(); 
		FluxNode->RefreshNodeVisibilityBP(Visible && !CantBeShown);
    }
}

void AFlux::SetFluxActiveMulticast_Implementation(bool bActivate)
{
	if (bFluxIsActive == bActivate) return;
	bFluxIsActive = bActivate;
	UpdateID++;
	UpdateVersion++;
	SetFluxActiveBP(bActivate);
	for (auto FluxNode : FluxNodes)
	{
		if (FluxNode == nullptr) continue;
		FluxNode->CallNodeTypeUpdated();
	}
}

void AFlux::ForceVisibilityFluxActiveMulticast_Implementation(bool bActivate)
{
	SetFluxActiveBP(bActivate);
	for (auto FluxNode : FluxNodes)
	{
		if (FluxNode == nullptr) continue;
		FluxNode->CallNodeTypeUpdated();
	}
}

void AFlux::RedrawFluxPreview_Implementation(int PointsIndexModified, const TArray<FVector>& PointsPreview)
{
	RedrawFluxPreviewBP(PointsIndexModified, PointsPreview, false);
}

void AFlux::RedrawFluxFinished_Implementation(int PointsIndexModified)
{
	RedrawFluxFinishedBP(PointsIndexModified, false, FVector::ZeroVector);
}

void AFlux::SetOriginMulticast_Implementation(ABuildingParent* NewOrigin)
{
	Origin = NewOrigin;
}

void AFlux::CallNodeCreationFailedMulticast_Implementation()
{
	FluxNodeCreationFailed.Broadcast(this);
}

void AFlux::ReorderIndexesMulticast_Implementation()
{
	TArray<FSplinePoint> Points = TArray<FSplinePoint>();
	FSplinePoint InitialPoint = FSplinePoint();
	InitialPoint.InputKey = 0;

	if (Origin != nullptr)
	{
		InitialPoint.Position = Origin->GetActorLocation();
		Points.Add(InitialPoint);
	}
	
	for (int i = 0; i < FluxNodes.Num(); i++)
	{
		//if (FluxNodes.Num() <= i) continue;
		auto FluxNode = FluxNodes[i];
		if (FluxNode == nullptr)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, TEXT("FluxNode is null in reorder indexes"));
			continue;
		}
		FluxNode->SetNodeIndex(i);
		FSplinePoint Point = FSplinePoint();
		if (i+1 < SplineComponent->GetNumberOfSplinePoints())
		{
			Point = SplineComponent->GetSplinePointAt(i, ESplineCoordinateSpace::World);
		}
		Point.InputKey = i+1;
		Point.Position = FluxNode->GetActorLocation();
		Points.Add(Point);
	}
	
	SplineComponent->ClearSplinePoints();
	for (auto Point : Points)
	{
		SplineComponent->AddPoint(Point, ESplineCoordinateSpace::World);
	}
}

void AFlux::SetSplinePointsMulticast_Implementation(const TArray<FVector>& Points, const bool bFinishChanges, int PointsIndexModified, const TArray<FVector>& PointsPreview, const bool DontDrawLast, const FVector& LastPointPos)
{
	if (HasAuthority()) return;
	SetSplinePointsLocal(Points, bFinishChanges, PointsIndexModified, PointsPreview, DontDrawLast, LastPointPos);
}

