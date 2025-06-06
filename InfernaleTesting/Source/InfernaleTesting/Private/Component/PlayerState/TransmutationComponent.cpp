// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/PlayerState/TransmutationComponent.h"

#include "Component/PlayerController/UIComponent.h"
#include "Component/PlayerState/EconomyComponent.h"
#include "DataAsset/GameSettingsDataAsset.h"
#include "DataAsset/TransmutationDataAsset.h"
#include "FunctionLibraries/FunctionLibraryInfernale.h"
#include "GameMode/Infernale/GameModeInfernale.h"
#include "GameMode/Infernale/PlayerStateInfernale.h"

bool FTransmutationQueue::IsInQueue(const FString NodeId)
{
	for (auto Node : Queue)
	{
		if (!NodeId.Equals(Node.TransmutationNode.ID, ESearchCase::CaseSensitive)) continue;
		return true;
	}
	return false;
}

void FTransmutationQueue::AddToQueue(const FTransmutationQueueElement Node)
{
	if (IsInQueue(Node.TransmutationNode.ID)) return;
	Queue.Add(Node);
}

void FTransmutationQueue::RemoveFromQueue(const FString NodeId)
{
	int Index = 0;
	for (Index = 0; Index < Queue.Num(); Index++)
	{
		if (!NodeId.Equals(Queue[Index].TransmutationNode.ID, ESearchCase::CaseSensitive)) continue;
		break;
	}
	if (Index == Queue.Num()) return;
	Queue.RemoveAt(Index);
}

bool FSimpleTransmutationQueue::IsInQueue(const FString NodeId, int LevelTarget)
{
	for (auto Node : Queue)
	{
		if (!NodeId.Equals(Node.TransmutationNode.ID, ESearchCase::CaseSensitive)) continue;
		if (Node.LevelToActivate != LevelTarget) continue;
		return true;
	}
	return false;
}

void FSimpleTransmutationQueue::AddToQueue(const FSimpleTransmutationQueueElement Node)
{
	if (IsInQueue(Node.TransmutationNode.ID, Node.LevelToActivate)) return;
	Queue.Add(Node);		
}

void FSimpleTransmutationQueue::RemoveFromQueue(const FString NodeId, int LevelTarget)
{
	int Index;
	for (Index = 0; Index < Queue.Num(); Index++)
	{
		if (!NodeId.Equals(Queue[Index].TransmutationNode.ID, ESearchCase::CaseSensitive)) continue;
		if (Queue[Index].LevelToActivate != LevelTarget) continue;
		break;
	}
	if (Index == Queue.Num()) return;
	Queue.RemoveAt(Index);
}


// Sets default values for this component's properties
UTransmutationComponent::UTransmutationComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UTransmutationComponent::BeginPlay()
{
	Super::BeginPlay();

	const auto GameSettings = UFunctionLibraryInfernale::GetGameSettingsDataAsset();
	const auto DAs = GameSettings->DataAssetsSettings[GameSettings->DataAssetsSettingsToUse];
	TransmutationDataAsset = DAs.TransmutationAssets;
	
	PlayerControllerInfernale->TransmutationToogle.AddDynamic(this, &UTransmutationComponent::ToogleTransmutation);
	SyncWithDataAsset();

	TransmutationEffects.Empty();
	InitEffect(ENodeEffect::NodeEffectDamageToBuilding);
	InitEffect(ENodeEffect::NodeEffectDamageToMonster);
	InitEffect(ENodeEffect::NodeEffectDamageToUnit);
	InitEffect(ENodeEffect::NodeEffectHealthUnit);
	InitEffect(ENodeEffect::NodeEffectHealthBuilding);
	InitEffect(ENodeEffect::NodeEffectFluxRange);
	InitEffect(ENodeEffect::NodeEffectUnitSight);
	InitEffect(ENodeEffect::NodeEffectUnitSpeed);
	InitEffect(ENodeEffect::NodeEffectBuildingSight);
	InitEffect(ENodeEffect::NodeEffectBuildingRecycleSouls);
	InitEffect(ENodeEffect::NodeEffectBuildingOverclockDuration);
	InitEffect(ENodeEffect::NodeEffectBuildingConstructionTime);
	InitEffect(ENodeEffect::NodeEffectBuildingConstructionCost);

	TransmutationSimpleEffects.Empty();
	for (const auto Node : TransmutationDataAsset->SimpleTransmutationNodes)
	{
		InitSimpleEffect(Node.Effect.NodeEffect, Node.Effect.ValuePercentageCurve);
	}

	
	if (!PlayerControllerInfernale->HasAuthority()) return;
	const auto GameMode = Cast<AGameModeInfernale>(GetWorld()->GetAuthGameMode());
	GameMode->PreLaunchGame.AddDynamic(this, &UTransmutationComponent::OnPreLaunchGame);
}

void UTransmutationComponent::SyncWithDataAsset()
{
	if (!TransmutationDataAsset)
	{
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("TransmutationDataAsset is not set"));
		return;
	}

	/* Complexe nodes setup */
	TransmutationNodes = TransmutationDataAsset->TransmutationNodes;
	TransmutationSettings = TransmutationDataAsset->TransmutationSettings;
	TransmutationNodesOwned.Empty();

	for (auto TransmutationNode : TransmutationNodes)
	{
		FTransmutationNodeOwned TransmutationNodeOwned = FTransmutationNodeOwned();
		TransmutationNodeOwned.TransmutationNode = TransmutationNode;
		TransmutationNodeOwned.bOwned = false;
		TransmutationNodesOwned.Add(TransmutationNodeOwned);
	}


	/* Simple nodes setup */
	TransmutationSimpleNodes = TransmutationDataAsset->SimpleTransmutationNodes;
	TransmutationSimpleNodesOwned.Empty();

	for (auto TransmutationSimpleNode : TransmutationSimpleNodes)
	{
		FTransmutationSimpleNodeOwned TransmutationSimpleNodeOwned;
		TransmutationSimpleNodeOwned.TransmutationNode = TransmutationSimpleNode;
		TransmutationSimpleNodeOwned.Level = 0;
		TransmutationSimpleNodeOwned.LevelTarget = 0;
		TransmutationSimpleNodeOwned.bLevelingUp = false;
		TransmutationSimpleNodesOwned.Add(TransmutationSimpleNodeOwned);
	}
}

void UTransmutationComponent::StartTransmutation()
{
	if (bTransmutationMode) return;

	TransmutationMode.Broadcast(true);
	bTransmutationMode = true;
}

void UTransmutationComponent::EndTransmutation()
{
	if (!bTransmutationMode) return;
	
	TransmutationMode.Broadcast(false);
	bTransmutationMode = false;
}

void UTransmutationComponent::ActivateNode(const FString& NodeID)
{
	for (auto& NodeOwned : TransmutationNodesOwned)
	{
		FString LocalID = NodeOwned.TransmutationNode.ID;
		if (!LocalID.Equals(NodeID, ESearchCase::CaseSensitive)) continue;
		if (NodeOwned.bOwned || NodeOwned.bActivating) return;

		FTransmutationQueueElement NodeInfo;
		NodeInfo.TransmutationNode = NodeOwned.TransmutationNode;
		NodeInfo.Index = TransmutationQueue.Queue.Num();
		NodeInfo.Activating = true;
		TransmutationQueue.AddToQueue(NodeInfo);

		TryStartNextNodeInQueue();
		return;
	}
}

void UTransmutationComponent::ActivateSimpleNode(const FString& NodeID, const int LevelTarget)
{
	for (auto& NodeOwned : TransmutationSimpleNodesOwned)
	{
		FString LocalID = NodeOwned.TransmutationNode.ID;
		if (!LocalID.Equals(NodeID, ESearchCase::CaseSensitive)) continue;

		if (NodeOwned.LevelTarget > LevelTarget) return;

		FSimpleTransmutationQueueElement NodeInfo;
		NodeInfo.TransmutationNode = NodeOwned.TransmutationNode;
		NodeInfo.Index = SimpleTransmutationQueue.Queue.Num();
		NodeInfo.LevelToActivate = LevelTarget;
		NodeInfo.Activating = false;
		SimpleTransmutationQueue.AddToQueue(NodeInfo);
		if (SimpleTransmutationQueue.Queue.Num() > 1)
			PlayerControllerInfernale->GetUIComponent()->OnTransmutationNodeEnqueueVFX(LocalID);

		if (bDebugSimpleEffect) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, FString::Printf(TEXT("%s added to queue for level %d"), *NodeID, LevelTarget));

		TryStartNextNodeInSimpleQueue();
		return;
	}
}

void UTransmutationComponent::DeactivateNode(const FString& NodeID)
{
	for (auto& NodeOwned : TransmutationNodesOwned)
	{
		FString LocalID = NodeOwned.TransmutationNode.ID;
		if (!LocalID.Equals(NodeID, ESearchCase::CaseSensitive)) continue;
		if (!NodeOwned.bOwned || NodeOwned.bActivating) return;

		FTransmutationQueueElement NodeInfo;
		NodeInfo.TransmutationNode = NodeOwned.TransmutationNode;
		NodeInfo.Index = TransmutationQueue.Queue.Num();
		NodeInfo.Activating = false;
		TransmutationQueue.AddToQueue(NodeInfo);
		if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Green, FString::Printf(TEXT("Deactivate Node: %s added to queue"), *NodeID));

		TryStartNextNodeInQueue();
		return;
	}
}

void UTransmutationComponent::TryStartNextNodeInQueue()
{
	if (TransmutationQueue.Queue.Num() == 0)
	{
		if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Green, TEXT("Queue is empty"));
		return;
	}
	if (TransmutationQueue.IsProcessing)
	{
		if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Green, TEXT("Queue is processing"));
		TransmutationQueueAltered.Broadcast(TransmutationQueue);
		return;
	}

	const auto Info = TransmutationQueue.Queue[0];
	const auto Node = Info.TransmutationNode;

	for (auto& NodeOwned : TransmutationNodesOwned)
	{
		FString LocalID = NodeOwned.TransmutationNode.ID;
		if (!LocalID.Equals(Node.ID, ESearchCase::CaseSensitive)) continue;
		if (NodeOwned.bActivating)
		{
			if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Green, TEXT("Node is already activating in queue"));
			return;
		}

		if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Green, FString::Printf(TEXT("Start processing Node: %s"), *NodeOwned.TransmutationNode.ID));
		
		TransmutationQueue.Queue.RemoveAt(0);
		TransmutationQueue.IsProcessing = true;
		TransmutationQueue.CurrentNodeID = Node.ID;

		ActivateNodeMulticast(Node.ID, true, !Info.Activating, TransmutationQueue);
		NodeOwned.TimerDel.BindUFunction(this, FName("NodeActivateTimerDone"), Node.ID, Info.Activating);
		const auto Duration = Info.Activating ? Node.ActivationTime : Node.DeactivationTime;
		GetWorld()->GetTimerManager().SetTimer(NodeOwned.TimerHandle, NodeOwned.TimerDel, Duration, false);
		
		return;
	}
}

void UTransmutationComponent::TryStartNextNodeInSimpleQueue()
{
	if (SimpleTransmutationQueue.Queue.Num() == 0)
	{
		if (bDebugSimpleEffect) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Simple Queue is empty")));
		return;
	}
	if (SimpleTransmutationQueue.IsProcessing)
	{
		if (bDebugSimpleEffect) GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Green, TEXT("SimpleQueue is processing"));
		SimpleTransmutationQueueAltered.Broadcast(SimpleTransmutationQueue);
		return;
	}

	const auto Info = SimpleTransmutationQueue.Queue[0];
	const auto Node = Info.TransmutationNode;

	for (auto& NodeOwned : TransmutationSimpleNodesOwned)
	{
		FString LocalID = NodeOwned.TransmutationNode.ID;
		if (!LocalID.Equals(Node.ID, ESearchCase::CaseSensitive)) continue;
		
		// if (NodeOwned.bLevelingUp)
		// {
		// 	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Green, TEXT("Node is already activating in queue"));
		// 	return;
		// }
		
		SimpleTransmutationQueue.Queue.RemoveAt(0);
		SimpleTransmutationQueue.IsProcessing = true;
		SimpleTransmutationQueue.CurrentNodeID = Node.ID;
		if (bDebugSimpleEffect) GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Green, FString::Printf(TEXT("Start processing Node: %s at level %d"), *NodeOwned.TransmutationNode.ID, Info.LevelToActivate));

		//ActivateNodeMulticast(Node.ID, true, !Info.Activating, TransmutationQueue);
		NodeOwned.TimerDel.BindUFunction(this, FName("SimpleNodeActivateTimerDone"), Node.ID, Info.LevelToActivate);
		const auto Duration = Node.ActivationTime;
		GetWorld()->GetTimerManager().SetTimer(NodeOwned.TimerHandle, NodeOwned.TimerDel, Duration, false);
		PlayerControllerInfernale->GetUIComponent()->OnTransmutationNodeStartUnlockVFX(Node.ID);
		return;
	}
}

void UTransmutationComponent::InitEffect(ENodeEffect NodeEffect)
{
	auto NewEffect = FTransmutationEffects();
	NewEffect.NodeEffect = NodeEffect;
	NewEffect.ValueFlat = 0;
	NewEffect.ValueFlatCurse = 0;
	NewEffect.ValuePercent = 1;
	NewEffect.ValuePercentCurse = 1;
	TransmutationEffects.Add(NewEffect);
}

void UTransmutationComponent::InitSimpleEffect(ENodeEffect NodeEffect, UCurveFloat* Curve)
{
	auto NewEffect = FTransmutationSimpleEffects();
	NewEffect.NodeEffect = NodeEffect;
	NewEffect.Level = 0;
	NewEffect.ValuePercentCurve = Curve;
	TransmutationSimpleEffects.Add(NewEffect);
}

void UTransmutationComponent::ConsiderEffect(const ENodeEffect NodeEffect)
{
	if (!PlayerControllerInfernale->HasAuthority()) return;

	for (auto& TransmutationSimpleEffect : TransmutationSimpleEffects)
	{
		if (TransmutationSimpleEffect.NodeEffect != NodeEffect) continue;
		
		if (bDebugSimpleEffect) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString::Printf(TEXT("Simple Effect %s curve is %s"), *UEnum::GetDisplayValueAsText(NodeEffect).ToString(), *TransmutationSimpleEffect.ValuePercentCurve->GetName()));

		for (auto TransmutationNodeOwned : TransmutationSimpleNodesOwned)
		{
			if (TransmutationNodeOwned.TransmutationNode.Effect.NodeEffect != NodeEffect) continue;
			TransmutationSimpleEffect.Level = TransmutationNodeOwned.Level;
			
			float ValuePercent = TransmutationSimpleEffect.ValuePercentCurve->GetFloatValue(TransmutationSimpleEffect.Level);
			if (TransmutationSimpleEffect.ValuePercent == ValuePercent)
			{
				if (bDebugSimpleEffect) GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Green, FString::Printf(TEXT("Simple Effect %s already at %f for level %d"), *UEnum::GetDisplayValueAsText(NodeEffect).ToString(), ValuePercent, TransmutationSimpleEffect.Level));
				continue;
			}

			TransmutationSimpleEffect.ValuePercent = ValuePercent;
			if (bDebugSimpleEffect) GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Green, FString::Printf(TEXT("Simple Effect %s updated to %f"), *UEnum::GetDisplayValueAsText(NodeEffect).ToString(), ValuePercent));
			UpdateSimpleEffectMulticast(TransmutationSimpleEffect);
			break;
		}
	}

	if (!bUseComplexeEffects) return;
	for (auto& TransmutationEffect : TransmutationEffects)
	{
		if (TransmutationEffect.NodeEffect != NodeEffect) continue;
		float Value = 0;
		float ValueCurse = 0;
		float ValuePercent = 1;
		float ValuePercentCurse = 1;

		for (auto TransmutationNodeOwned : TransmutationNodesOwned)
		{
			if (!TransmutationNodeOwned.bOwned) continue;
			for (auto Effect : TransmutationNodeOwned.TransmutationNode.Effects)
			{
				if (Effect.NodeEffect != NodeEffect) continue;
				if (!Effect.IsCurse)
				{
					if (Effect.IsPercentage) ValuePercent += Effect.Value;
					else Value += Effect.Value;
					continue;
				}
				if (Effect.IsPercentage) ValuePercentCurse += Effect.Value;
				else ValueCurse += Effect.Value;
			}
		}
		if (TransmutationEffect.ValueFlat == Value &&
			TransmutationEffect.ValueFlatCurse == ValueCurse &&
			TransmutationEffect.ValuePercent == ValuePercent &&
			TransmutationEffect.ValuePercentCurse == ValuePercentCurse) continue;
		
		TransmutationEffect.ValueFlat = Value;
		TransmutationEffect.ValueFlatCurse = ValueCurse;
		TransmutationEffect.ValuePercent = ValuePercent;
		TransmutationEffect.ValuePercentCurse = ValuePercentCurse;

		UpdateEffectMulticast(TransmutationEffect);
	}
}

int UTransmutationComponent::CountCurrentNodesOfType(const ETransmutationNodeType NodeType) const
{
	int Count = 0;
	for (auto NodeOwned : TransmutationNodesOwned)
	{
		if (NodeOwned.TransmutationNode.PositionInfo.TransmutationNodeType != NodeType) continue;
		if (NodeOwned.bOwned || NodeOwned.bActivating) Count++;
	}

	return Count;
}

void UTransmutationComponent::NodeActivateTimerDone(const FString& NodeID, bool Activating)
{
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Green, FString::Printf(TEXT("Node: %s timer done"), *NodeID));
	for (auto& NodeOwned : TransmutationNodesOwned)
	{
		FString LocalID = NodeOwned.TransmutationNode.ID;
		if (!LocalID.Equals(NodeID, ESearchCase::CaseSensitive)) continue;
		ActivateNodeMulticast(NodeID, false, Activating, TransmutationQueue);
		TransmutationQueue.IsProcessing = false;
		TransmutationQueue.CurrentNodeID = "";
		TryStartNextNodeInQueue();
		return;
	}
}

void UTransmutationComponent::SimpleNodeActivateTimerDone(const FString& NodeID, int LevelTarget)
{
	if (bDebugSimpleEffect) GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Green, FString::Printf(TEXT("Node: %s timer done"), *NodeID));
	for (auto& NodeOwned : TransmutationSimpleNodesOwned)
	{
		FString LocalID = NodeOwned.TransmutationNode.ID;
		if (!LocalID.Equals(NodeID, ESearchCase::CaseSensitive)) continue;

		if (bDebugSimpleEffect) GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Green, FString::Printf(TEXT("Simple Node: %s timer done"), *NodeID));

		ActivateSimpleNodeMulticast(NodeID, LevelTarget, SimpleTransmutationQueue);
		PlayerControllerInfernale->GetUIComponent()->OnTransmutationNodeUnlockVFX(LocalID);
		SimpleTransmutationQueue.IsProcessing = false;
		SimpleTransmutationQueue.CurrentNodeID = "";
		TryStartNextNodeInSimpleQueue();
		return;
	}
}

void UTransmutationComponent::OnPreLaunchGame()
{
	PlayerControllerInfernale->GetUIComponent()->CreateTransmutationNodes(TransmutationNodesOwned, TransmutationSettings);
	PlayerControllerInfernale->GetUIComponent()->CreateTransmutationSimpleNodes(TransmutationSimpleNodesOwned, TransmutationSettings);
}

void UTransmutationComponent::TransmutationRefusedOwning_Implementation(const FString& NodeID)
{
	TransmutationRefused.Broadcast(NodeID);
}

TArray<FString> UTransmutationComponent::GetActiveTransmutations()
{
	TArray<FString> ActiveTransmutations;
	for (auto& NodeOwned : TransmutationNodesOwned)
	{
		if (!NodeOwned.bOwned) continue;
		ActiveTransmutations.Add(NodeOwned.TransmutationNode.ID);
	}
	return ActiveTransmutations;
}

// Called every frame
void UTransmutationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

float UTransmutationComponent::GetCostNextNode(const ETransmutationNodeType NodeType) const
{
	int CurrentNodes = CountCurrentNodesOfType(NodeType);
	auto BaseCost = 0;
	auto CostPerUnits = 0;
	auto Settings = TransmutationDataAsset->TransmutationSettings;
	FCostStruct Info;

	switch (NodeType)
	{
	case ETransmutationNodeType::TransmutationNodeSmall:
		Info = Settings.SmallNodeCost;
		BaseCost = Info.BaseValue;
		CostPerUnits = Info.ValuePerUnit;
		break;
	case ETransmutationNodeType::TranmutationNodeBig:
		Info = Settings.BigNodeCost;
		BaseCost = Info.BaseValue;
		CostPerUnits = Info.ValuePerUnit;
		break;
	default: break;
	}

	return BaseCost + (CostPerUnits * CurrentNodes);
}

float UTransmutationComponent::GetCostCurrentNode(const ETransmutationNodeType NodeType) const
{
	int CurrentNodes = CountCurrentNodesOfType(NodeType);
	auto BaseCost = 0;
	auto CostPerUnits = 0;
	auto Settings = TransmutationDataAsset->TransmutationSettings;
	FCostStruct Info;

	switch (NodeType)
	{
	case ETransmutationNodeType::TransmutationNodeSmall:
		Info = Settings.SmallNodeCost;
		BaseCost = Info.BaseValue;
		CostPerUnits = Info.ValuePerUnit;
		break;
	case ETransmutationNodeType::TranmutationNodeBig:
		Info = Settings.BigNodeCost;
		BaseCost = Info.BaseValue;
		CostPerUnits = Info.ValuePerUnit;
		break;
	default: break;
	}

	return BaseCost + (CostPerUnits * CurrentNodes-1);
}

FTransmutationEffects UTransmutationComponent::GetEffect(const ENodeEffect NodeEffect) const
{
	for (auto& TransmutationEffect : TransmutationEffects)
	{
		if (TransmutationEffect.NodeEffect != NodeEffect) continue;
		return TransmutationEffect;
	}
	return FTransmutationEffects();
}

float UTransmutationComponent::GetSimplePercent(const ENodeEffect NodeEffect) const
{
	for (auto& TransmutationEffect : TransmutationSimpleEffects)
	{
		if (TransmutationEffect.NodeEffect != NodeEffect) continue;
		return 1.f + TransmutationEffect.ValuePercent;
	}
	return 1.f;
}

float UTransmutationComponent::ApplyEffect(float InitialValue, const ENodeEffect NodeEffect) const
{
	const auto Effect = GetEffect(NodeEffect);
	const auto SimplePercent = GetSimplePercent(NodeEffect);
	auto NewValue = InitialValue + (Effect.ValueFlat + Effect.ValueFlatCurse);
	NewValue *= Effect.ValuePercent * SimplePercent * Effect.ValuePercentCurse;
	return NewValue;
}

float UTransmutationComponent::ApplyEffectAtIndex(float InitialValue, int Index) const
{
	if (Index < 0 || Index >= TransmutationEffects.Num()) return InitialValue;
	const auto Effect = TransmutationEffects[Index];
	const auto SimplePercent = GetSimplePercent(Effect.NodeEffect);
	auto NewValue = InitialValue + (Effect.ValueFlat + Effect.ValueFlatCurse);
	NewValue *= Effect.ValuePercent * SimplePercent * Effect.ValuePercentCurse;
	return NewValue;
}

float UTransmutationComponent::GetEffectDamageToBuilding(float InitialValue) const
{
	return ApplyEffectAtIndex(InitialValue, 0);
}

float UTransmutationComponent::GetEffectDamageToMonster(float InitialValue) const
{
	return ApplyEffectAtIndex(InitialValue, 1);
}

float UTransmutationComponent::GetEffectDamageToUnit(float InitialValue) const
{
	return ApplyEffectAtIndex(InitialValue, 2);
}

float UTransmutationComponent::GetEffectHealthUnit(float InitialValue) const
{
	return ApplyEffectAtIndex(InitialValue, 3);
}

float UTransmutationComponent::GetEffectHealthBuilding(float InitialValue) const
{
	return ApplyEffectAtIndex(InitialValue, 4);
}

float UTransmutationComponent::GetEffectFluxRange(float InitialValue) const
{
	return ApplyEffectAtIndex(InitialValue, 5);
}

float UTransmutationComponent::GetEffectUnitSight(float InitialValue) const
{
	return ApplyEffectAtIndex(InitialValue, 6);
}

float UTransmutationComponent::GetEffectUnitSpeed(float InitialValue) const
{
	return ApplyEffectAtIndex(InitialValue, 7);
}

float UTransmutationComponent::GetEffectBuildingSight(float InitialValue) const
{
	return ApplyEffectAtIndex(InitialValue, 8);
}

float UTransmutationComponent::GetEffectBuildingRecycleSouls(float InitialValue) const
{
	return ApplyEffectAtIndex(InitialValue, 9);
}

float UTransmutationComponent::GetEffectBuildingOverclockDuration(float InitialValue) const
{
	return ApplyEffectAtIndex(InitialValue, 10);
}

float UTransmutationComponent::GetEffectBuildingConstructionTime(float InitialValue) const
{
	return ApplyEffectAtIndex(InitialValue, 11);
}

float UTransmutationComponent::GetEffectBuildingConstructionCost(float InitialValue) const
{
	return ApplyEffectAtIndex(InitialValue, 12);
}

void UTransmutationComponent::ToogleTransmutation()
{
	if (!bTransmutationMode)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Transmutation mode enabled"));
		StartTransmutation();
		return;
	}
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Transmutation mode disabled"));
	EndTransmutation();
}

void UTransmutationComponent::BuyNode(const FString& NodeID)
{
	BuyNodeServer(NodeID);
}

void UTransmutationComponent::BuySimpleNode(const FString& NodeID)
{
	BuySimpleNodeServer(NodeID);
}

void UTransmutationComponent::RefundNode(const FString& NodeID)
{
	RefundNodeServer(NodeID);
}

FTransmutationNodeOwned UTransmutationComponent::GetNode(const FString& NodeID) const
{
	bool AreSame = false;
	for (auto& NodeOwned : TransmutationNodesOwned)
	{
		FString LocalID = NodeOwned.TransmutationNode.ID;
		AreSame = LocalID.Equals(NodeID, ESearchCase::CaseSensitive);
		if (!AreSame) continue;
		return NodeOwned;
	}
	return FTransmutationNodeOwned();
}

TArray<float> UTransmutationComponent::GetSimpleNodesCost() const
{
	return TArray<float>();
}

int32 UTransmutationComponent::GetUpdateID() const
{
	return UpdateID;
}

void UTransmutationComponent::BuyNodeServer_Implementation(const FString& NodeID)
{
	FTransmutationNodeOwned Node = TransmutationNodesOwned[0];
	bool Found = false;

	for (auto& NodeOwned : TransmutationNodesOwned)
	{
		FString LocalID = NodeOwned.TransmutationNode.ID;
		if (!LocalID.Equals(NodeID, ESearchCase::CaseSensitive)) continue;
		Found = true;
		Node = NodeOwned;
	}

	if (!Found) return;

	float Cost = GetCostNextNode(Node.TransmutationNode.PositionInfo.TransmutationNodeType);
	const auto EconomyComponent = PlayerControllerInfernale->GetPlayerState<APlayerStateInfernale>()->GetEconomyComponent();

	if (TransmutationQueue.IsInQueue(NodeID))
	{
		TransmutationQueue.RemoveFromQueue(NodeID);
		EconomyComponent->AddSouls(PlayerControllerInfernale, ESoulsGainCostReason::TransmutationNodeRefund, Cost);
		TransmutationQueueAltered.Broadcast(TransmutationQueue);
		return;
	}
	if (TransmutationQueue.CurrentNodeID.Equals(NodeID, ESearchCase::CaseSensitive))
	{
		for (auto& NodeOwned : TransmutationNodesOwned)
		{
			FString LocalID = NodeOwned.TransmutationNode.ID;
			if (!LocalID.Equals(NodeID, ESearchCase::CaseSensitive)) continue;
			
			GetWorld()->GetTimerManager().ClearTimer(NodeOwned.TimerHandle);
			NodeOwned.TimerHandle.Invalidate();
			NodeOwned.TimerDel.Unbind();
			
			ActivateNodeMulticast(NodeID, false, false, TransmutationQueue);
			TransmutationQueue.IsProcessing = false;
			TransmutationQueue.CurrentNodeID = "";
			break;
		}
		EconomyComponent->AddSouls(PlayerControllerInfernale, ESoulsGainCostReason::TransmutationNodeRefund, Cost);
		TransmutationQueueAltered.Broadcast(TransmutationQueue);
		TryStartNextNodeInQueue();
		return;
	}

	if (EconomyComponent->GetSouls() < Cost)
	{
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("Not enough souls"));
		TransmutationRefusedOwning(NodeID);
		return;
	}
	
	EconomyComponent->RemoveSouls(Cost);
	ActivateNode(NodeID);
}

void UTransmutationComponent::BuySimpleNodeServer_Implementation(const FString& NodeID)
{
	FTransmutationSimpleNodeOwned Node = TransmutationSimpleNodesOwned[0];
	bool Found = false;
	if (bDebugSimpleEffect) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("BuySimpleNodeServer: %s"), *NodeID));

	for (const auto& NodeOwned : TransmutationSimpleNodesOwned)
	{
		FString LocalID = NodeOwned.TransmutationNode.ID;
		
		if (bDebugSimpleEffect) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Purple, FString::Printf(TEXT("%s: level %d"), *LocalID, NodeOwned.Level));
		
		if (!LocalID.Equals(NodeID, ESearchCase::CaseSensitive)) continue;
		Found = true;
		Node = NodeOwned;
	}

	if (!Found)
	{
		if (bDebugSimpleEffect) GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Green, FString::Printf(TEXT("%s not found"), *NodeID));
		return;
	}

	const auto NextLevel = Node.LevelTarget+1;
	const float Cost = Node.TransmutationNode.Effect.PriceCurve->GetFloatValue(NextLevel);
	const auto EconomyComponent = PlayerControllerInfernale->GetPlayerState<APlayerStateInfernale>()->GetEconomyComponent();

	/* Here if we need to cancel & refund */

	if (EconomyComponent->GetSouls() < Cost)
	{
		if (bDebugSimpleEffect) GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Green, FString::Printf(TEXT("not enough souls for %s"), *NodeID));
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("Not enough souls"));
		TransmutationRefusedOwning(NodeID);
		return;
	}

	for (auto& NodeOwned : TransmutationSimpleNodesOwned)
	{
		FString LocalID = NodeOwned.TransmutationNode.ID;
		if (!LocalID.Equals(NodeID, ESearchCase::CaseSensitive)) continue;
		NodeOwned.LevelTarget = NextLevel;
		break;
	}
	
	EconomyComponent->RemoveSouls(Cost);
	ActivateSimpleNode(NodeID, NextLevel);
}

void UTransmutationComponent::RefundNodeServer_Implementation(const FString& NodeID)
{
	FTransmutationNodeOwned Node = TransmutationNodesOwned[0];
	bool Found = false;

	for (auto& NodeOwned : TransmutationNodesOwned)
	{
		FString LocalID = NodeOwned.TransmutationNode.ID;
		if (!LocalID.Equals(NodeID, ESearchCase::CaseSensitive)) continue;
		Found = true;
	}

	if (!Found)
	{
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("Deactivate::Node not found"));
		return;
	}
	
	if (TransmutationQueue.IsInQueue(NodeID))
	{
		TransmutationQueue.RemoveFromQueue(NodeID);
		TransmutationQueueAltered.Broadcast(TransmutationQueue);
		return;
	}
	if (TransmutationQueue.CurrentNodeID.Equals(NodeID, ESearchCase::CaseSensitive))
	{
		for (auto& NodeOwned : TransmutationNodesOwned)
		{
			FString LocalID = NodeOwned.TransmutationNode.ID;
			if (!LocalID.Equals(NodeID, ESearchCase::CaseSensitive)) continue;
			
			GetWorld()->GetTimerManager().ClearTimer(NodeOwned.TimerHandle);
			NodeOwned.TimerHandle.Invalidate();
			NodeOwned.TimerDel.Unbind();
			
			ActivateNodeMulticast(NodeID, false, true, TransmutationQueue);
			TransmutationQueue.IsProcessing = false;
			TransmutationQueue.CurrentNodeID = "";
			break;
		}
		TransmutationQueueAltered.Broadcast(TransmutationQueue);
		TryStartNextNodeInQueue();
		return;
	}
	
	DeactivateNode(NodeID);
}

void UTransmutationComponent::ActivateNodeMulticast_Implementation(const FString& NodeID, const bool bActivate,
	const bool bOwned, const FTransmutationQueue Queue)
{
	for (auto& NodeOwned : TransmutationNodesOwned)
	{
		FString LocalID = NodeOwned.TransmutationNode.ID;
		if (!LocalID.Equals(NodeID, ESearchCase::CaseSensitive)) continue;

		bool InitialyOwned = NodeOwned.bOwned;
		NodeOwned.bActivating = bActivate;
		NodeOwned.bOwned = bOwned;
		if (InitialyOwned != bOwned)
		{
			TArray<ENodeEffect> EffectsChanged = TArray<ENodeEffect>();
			for (auto Effect : NodeOwned.TransmutationNode.Effects)
			{
				ConsiderEffect(Effect.NodeEffect);
				EffectsChanged.Add(Effect.NodeEffect);
			}
			NodeOwnedOwnerShipAltered.Broadcast(EffectsChanged, this);
		}
		NodeOwnedAltered.Broadcast(NodeID, Queue);
		return;
	}
}

void UTransmutationComponent::ActivateSimpleNodeMulticast_Implementation(const FString& NodeID,
	const int LevelReached, const FSimpleTransmutationQueue Queue)
{
	for (auto& NodeOwned : TransmutationSimpleNodesOwned)
	{
		FString LocalID = NodeOwned.TransmutationNode.ID;
		if (!LocalID.Equals(NodeID, ESearchCase::CaseSensitive)) continue;

		if (bDebugSimpleEffect) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, FString::Printf(TEXT("Simple Node: %s activated at level %d"), *NodeID, LevelReached));

		const auto Node = NodeOwned.TransmutationNode;
		NodeOwned.Level = LevelReached;
		NodeOwned.bLevelingUp = (NodeOwned.Level >= NodeOwned.LevelTarget);

		if (bDebugSimpleEffect) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, FString::Printf(TEXT("Simple Node: %s level %d"), *NodeID, NodeOwned.Level));
		ConsiderEffect(Node.Effect.NodeEffect);
		if (bDebugSimpleEffect) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, FString::Printf(TEXT("Simple Node: %s level %d (after)"), *NodeID, NodeOwned.Level));
		

		TArray<ENodeEffect> EffectsChanged = TArray<ENodeEffect>();
		EffectsChanged.Add(Node.Effect.NodeEffect);
		NodeOwnedOwnerShipAltered.Broadcast(EffectsChanged, this);
		
		SimpleNodeOwnedAltered.Broadcast(NodeID, Queue);
		break;
		//return;
	}

	for (auto NodeOwned : TransmutationSimpleNodesOwned)
	{
		FString LocalID = NodeOwned.TransmutationNode.ID;
		//if (!LocalID.Equals(NodeID, ESearchCase::CaseSensitive)) continue;
		
		if (bDebugSimpleEffect) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, FString::Printf(TEXT("%s: level %d"), *LocalID, NodeOwned.Level));
	}
}

void UTransmutationComponent::UpdateEffectMulticast_Implementation(const FTransmutationEffects Effect)
{
	const auto NodeEffect = Effect.NodeEffect;
	for (auto& TransmutationEffect : TransmutationEffects)
	{
		if (TransmutationEffect.NodeEffect != NodeEffect) continue;
		TransmutationEffect.ValueFlat = Effect.ValueFlat;
		TransmutationEffect.ValueFlatCurse = Effect.ValueFlatCurse;
		TransmutationEffect.ValuePercent = Effect.ValuePercent;
		TransmutationEffect.ValuePercentCurse = Effect.ValuePercentCurse;
		TransmutationEffectAltered.Broadcast(NodeEffect);
	}

	UpdateID++;
}

void UTransmutationComponent::UpdateSimpleEffectMulticast_Implementation(const FTransmutationSimpleEffects Effect)
{
	const auto NodeEffect = Effect.NodeEffect;
	for (auto& TransmutationEffect : TransmutationSimpleEffects)
	{
		if (TransmutationEffect.NodeEffect != NodeEffect) continue;
		TransmutationEffect.ValuePercent = Effect.ValuePercent;
		TransmutationEffectAltered.Broadcast(NodeEffect);
	}

	UpdateID++;
}

