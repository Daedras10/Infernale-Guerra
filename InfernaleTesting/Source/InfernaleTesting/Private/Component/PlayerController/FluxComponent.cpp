// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/PlayerController/FluxComponent.h"

#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Camera/CameraComponent.h"
#include "Component/PlayerController/UIComponent.h"
#include "Component/PlayerState/TransmutationComponent.h"
#include "DataAsset/FluxSettingsDataAsset.h"
#include "DataAsset/GameSettingsDataAsset.h"
#include "Flux/Flux.h"
#include "Flux/FluxNode.h"
#include "FunctionLibraries/FunctionLibraryInfernale.h"
#include "GameMode/Infernale/GameModeInfernale.h"
#include "GameMode/Infernale/PlayerControllerInfernale.h"
#include "LD/Buildings/MainBuilding.h"
#include "Manager/UnitActorManager.h"
#include <Kismet/GameplayStatics.h>
#include <Mass/Collision/SpatialHashGrid.h>
#include "MassEntityConfigAsset.h"
#include <Mass/Amalgam/Traits/AmalgamTraitBase.h>
#include "LD/LDElement/Boss.h"

FOriginsStuct::FOriginsStuct(): Building(nullptr)
{
}

// Sets default values for this component's properties
UFluxComponent::UFluxComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	//SetIsReplicatedByDefault(true);
}

void UFluxComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bShouldCalulateThisFrame)
	{
		ShouldCalulateThisFrameTimer += DeltaTime;
		if (ShouldCalulateThisFrameTimer >= ShouldCalulateThisFrameTimerMax)
		{
			bShouldCalulateThisFrame = true;
			ShouldCalulateThisFrameTimer = 0.f;
		}
	}
	
	if (!bFluxMode) return;
	if (!bInteractionsAllowed)
	{
		ClearHoverFluxNode();
		ClearHoverFlux();
		return;
	}

	if (IsRemovingMode()) RemoveFluxTimers(DeltaTime);
	if (!bHoverLocked) HoverFluxes();
	if (bShouldUpdateMoveTimer) FluxUpdateTimer -= DeltaTime;

	if (FluxModeState == EFluxModeState::FMSMoveFluxNodeCnC) MoveSelectedFluxNodeNoRelease();

	//TODO: Change to consider CnC mode
	if (FluxModeState == EFluxModeState::FMSMoveFluxNode || FluxModeState == EFluxModeState::FMSMoveFluxNodeCnC)
	{
		EFluxHoverInfoType FluxHoverInfo = MoveNodeCanUseOriginalNode ?
			EFluxHoverInfoType::FluxHoverInfoTypeFluxNodeMovingWithAllowed :
			EFluxHoverInfoType::FluxHoverInfoTypeFluxNodeMoving;
		
		PlayerControllerInfernale->GetUIComponent()->FluxEntityHovered(FluxHoverInfo);
	}
}

void UFluxComponent::SetFluxMode(const bool bIsFluxMode)
{
	this->bFluxMode = bIsFluxMode;
	FluxModeChanged.Broadcast(this->bFluxMode);

	if (bFluxMode) FluxModeStart();
	else FluxModeEnd();
}

bool UFluxComponent::IsFluxMode() const
{
	return bFluxMode;
}

void UFluxComponent::AddBuildingToSelectionForce(TWeakObjectPtr<ABuildingParent> Building)
{
	AddBuildingToSelection(Building);
}

void UFluxComponent::RemoveBuildingFromSelectionForce(TWeakObjectPtr<ABuildingParent> Building)
{
	RemoveBuildingFromSelection(Building.Get());
}

AFluxNode* UFluxComponent::GetFluxNodeInteracted() const
{
	return FluxNodeInteracted.Get();
}

// Called when the game starts
void UFluxComponent::BeginPlay()
{
	Super::BeginPlay();
	SyncWithDataAsset();
	FluxInitialEvents();

	PlayerControllerInfernale->TransmutationModeChanged.AddDynamic(this, &UFluxComponent::OnTransmutationModeChanged);

	if (!PlayerControllerInfernale->HasAuthority()) return;
	GameModeInfernale = Cast<AGameModeInfernale>(GetWorld()->GetAuthGameMode());
}

// void UFluxComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
// {
// 	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
// 	//DOREPLIFETIME(UFluxComponent, Fluxes);
// }

void UFluxComponent::FluxInitialEvents()
{
	if (PlayerControllerInfernale->UseFluxMode())
	{
		PlayerControllerInfernale->FluxModeStart.AddDynamic(this, &UFluxComponent::OnFluxModeChanged);
		return;
	}
	FluxModeStart();
	this->bFluxMode = true;
}

void UFluxComponent::SyncWithDataAsset()
{
	if (!bUseDataAsset) return;

	const auto GameSettings = UFunctionLibraryInfernale::GetGameSettingsDataAsset();
	FluxSettingsDataAsset = GameSettings->DataAssetsSettings[GameSettings->DataAssetsSettingsToUse].FluxSettings;
	if (!FluxSettingsDataAsset)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("UFluxComponent::FluxSettingsDataAsset is not set"));
		return;
	}

	RemoveFluxDuration = FluxSettingsDataAsset->RemoveFluxDuration;
	RemoveFluxNodeDuration = FluxSettingsDataAsset->RemoveFluxNodeDuration;
	bRemoveMustStayOnFlux = FluxSettingsDataAsset->bRemoveMustHoverOnFlux;
	FluxSpawnClass = FluxSettingsDataAsset->FluxSpawnClass;
	bMultiSelectionEnabled = FluxSettingsDataAsset->bMultiSelectionEnabled;
	DragNDropTime = FluxSettingsDataAsset->FluxCreationHoldTime;
	MinRangeBetweenNodes = FluxSettingsDataAsset->MinRangeBetweenNodes;
}

void UFluxComponent::HoverFluxes()
{
	FHitResult HitResult;
	if (!CreateHitResultFlux(HitResult)) return;
	
	const auto FoundHoverFlux = TryToHoverFlux(HitResult);
	if (FoundHoverFlux)
	{
		ClearHoverFluxNode();
		if (!IsMovingNode()) PlayerControllerInfernale->GetUIComponent()->FluxEntityHovered(EFluxHoverInfoType::FluxHoverInfoTypeFluxHovered);
		return;
	}
	ClearHoverFlux();

	auto FoundHoverFluxNode = TryToHoverFluxNode(HitResult);
	if (FoundHoverFluxNode)
	{
		if (!IsMovingNode())PlayerControllerInfernale->GetUIComponent()->FluxEntityHovered(EFluxHoverInfoType::FluxHoverInfoTypeFluxNodeHovered);
		return;
	}
	ClearHoverFluxNode();
	if (!IsMovingNode()) PlayerControllerInfernale->GetUIComponent()->FluxEntityHovered(EFluxHoverInfoType::FluxHoverInfoTypeNone);
}

void UFluxComponent::RemoveFluxTimers(float DeltaTime)
{
	RemoveFluxTimerElapsed += DeltaTime;
	const auto RemoveDuration = FluxModeState == EFluxModeState::FMSRemoveFlux ? RemoveFluxDuration : RemoveFluxNodeDuration;
	RemoveFluxTimerElapsed = FMath::Clamp(RemoveFluxTimerElapsed, 0.0f, RemoveDuration);
	FluxRemoving.Broadcast(FluxModeState, RemoveFluxTimerElapsed, RemoveDuration);

	//Check for pause?
	
	if (FluxModeState == EFluxModeState::FMSRemoveFlux)
	{
		if (RemoveFluxTimerElapsed >= RemoveFluxDuration) RemoveFlux();
		return;
	}

	if (FluxModeState == EFluxModeState::FMSRemoveFluxNode)
	{
		if (RemoveFluxTimerElapsed >= RemoveFluxNodeDuration) RemoveFluxNode();
		return;
	}
}

void UFluxComponent::OnFluxModeChanged()
{
	this->bFluxMode = !this->bFluxMode;
	FluxModeChanged.Broadcast(this->bFluxMode);

	if (bFluxMode) FluxModeStart();
	else FluxModeEnd();
}

void UFluxComponent::FluxModeStart()
{
	PlayerControllerInfernale->MousePrimaryStart.AddDynamic(this, &UFluxComponent::OnPrimaryActionStarted);
	PlayerControllerInfernale->MouseSecondaryStart.AddDynamic(this, &UFluxComponent::OnSecondaryActionStarted);
	PlayerControllerInfernale->MouseSecondaryEnd.AddDynamic(this, &UFluxComponent::OnSecondaryActionEnd);
	PlayerControllerInfernale->EscapeStarted.AddDynamic(this, &UFluxComponent::OnEscapeStarted);
	PlayerControllerInfernale->ShiftModeChanged.AddDynamic(this, &UFluxComponent::OnShiftModeChanged);	
	
	Origins.Empty();
}

void UFluxComponent::FluxModeEnd()
{
	PlayerControllerInfernale->MousePrimaryStart.RemoveDynamic(this, &UFluxComponent::OnPrimaryActionStarted);
	PlayerControllerInfernale->MouseSecondaryStart.RemoveDynamic(this, &UFluxComponent::OnSecondaryActionStarted);
	PlayerControllerInfernale->EscapeStarted.RemoveDynamic(this, &UFluxComponent::OnEscapeStarted);
	ClearBuildingSelection();
	ClearNodeSelection();

	ClearHoverFlux();
	ClearHoverFluxNode();
}

void UFluxComponent::MoveStartNode(AFluxNode* FluxNode, AMainBuilding* MainBuilding, FVector HitPoint,
	bool Release)
{
	const auto Flux = FluxNode->GetFlux();
	if (Flux == nullptr) return;
	const auto FluxNodeIndex = FluxNode->GetNodeIndex();
	Flux->ClearAllPreviousFakePoints(FluxNodeIndex);

	/* FluxPathfinding */
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation OutNavLoc;
	TArray<FPathStruct> AfterNodePathStructs;

	/* After location */
	/* Start location */
	auto StartLocation = HitPoint;
	const auto MainBuildingLoc = MainBuilding->GetActorLocation();
	auto Direction = (StartLocation - MainBuildingLoc);
	Direction.Z = 0;
	Direction.Normalize();

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
    }

	if (bDebugPathfinding)
	{
		/* Angles */
		const auto HalfAngle = MainBuilding->GetAngleAroundForFluxes() * 0.5f;

		const auto StartingAngleDegree = MainBuilding->GetStartingAngle();
		const auto Rad = FMath::DegreesToRadians(StartingAngleDegree + HalfAngle);
		const auto Rad2 = FMath::DegreesToRadians(StartingAngleDegree + -HalfAngle);
		auto Direction1 = FVector(FMath::Cos(Rad), FMath::Sin(Rad), 0);
		auto Direction2 = FVector(FMath::Cos(Rad2), FMath::Sin(Rad2), 0);
		DrawDebugLine(GetWorld(), MainBuildingLoc, MainBuildingLoc + Direction1 * 3000, FColor::Yellow, false, 0, 0, 5);
		DrawDebugLine(GetWorld(), MainBuildingLoc, MainBuildingLoc + Direction2 * 3000, FColor::Yellow, false, 0, 0, 5);
	}

	
	StartLocation = MainBuildingLoc + Direction.GetSafeNormal() * MainBuilding->GetOffsetRange();

	auto TargetReachable = NavSys->ProjectPointToNavigation(StartLocation, OutNavLoc, FVector(500, 500, 100));
	if (!TargetReachable)
	{
		if (bDebugPathfinding)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Initial Target not reachable"));
			DrawDebugSphere(GetWorld(), StartLocation, 500, 12, FColor::Purple, false, 5, 0, 25);
		}
		return;
	}
	StartLocation = OutNavLoc.Location;
	const auto NodeNewLocation = StartLocation;
	

	/* End location */
	auto NextNodeIndex = Flux->GetNextNodeIndex(FluxNodeIndex);
	if (NextNodeIndex <= 0) return;
	
	auto Target = Flux->GetFluxNodes()[NextNodeIndex]->GetActorLocation();
	TargetReachable = NavSys->ProjectPointToNavigation(Target, OutNavLoc, FVector(500, 500, 100));
	if (!TargetReachable)
	{
		if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("End Target not reachable"));
		return;
	}
	Target = OutNavLoc.Location;


	/* Pathfinding (After) */
	auto Path = NavSys->FindPathToLocationSynchronously(GetWorld(), StartLocation, Target, FluxNode);
	if (Path == nullptr)
	{
		if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Path is null"));
		return;
	}
	if (Path->IsPartial())
	{
		if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Path is partial"));
		return;
	}

	auto PreviousPoint = StartLocation;
	for (auto Point : Path->PathPoints)
	{
		AfterNodePathStructs.Add(FPathStruct(Point, false));
		if (bDebugPathfinding)
		{
			DrawDebugSphere(GetWorld(), Point, 500, 12, FColor::Red, false, 0, 0, 10);
			DrawDebugLine(GetWorld(), PreviousPoint, Point, FColor::Red, false, 0, 0, 10);
		}
		PreviousPoint = Point;
	}
	if (AfterNodePathStructs.Num() == 0)
	{
		if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("AfterNodePathStructs is empty"));
		return;
	}

	if (AfterNodePathStructs.Num() > 0) AfterNodePathStructs.RemoveAt(0);
	if (AfterNodePathStructs.Num() > 0) AfterNodePathStructs.RemoveAt(AfterNodePathStructs.Num() - 1);
	
	if (Release)
	{
		Flux->MoveNode(FluxNode, NodeNewLocation, TArray<FPathStruct>(), AfterNodePathStructs, TArray<FPathStruct>(), nullptr, false);
		return;
	}
	Flux->MoveNodePreview(FluxNode, NodeNewLocation, TArray<FPathStruct>(), AfterNodePathStructs, TArray<FPathStruct>(), nullptr, false);
	FluxNodeOtherMoving = nullptr;
}

void UFluxComponent::OnPrimaryActionStarted()
{
	FHitResult HitResult;
	if (FluxModeState == EFluxModeState::FMSRemoveFlux || FluxModeState == EFluxModeState::FMSRemoveFluxNode) return;
	if (FluxModeState == EFluxModeState::FMSMoveFluxNode || FluxModeState == EFluxModeState::FMSMoveFluxNodeCnC) return;
	if (bTryingToHoldPrimaryAction) return;
	CreateHitResultFlux(HitResult);

	/* Flux from buildings */
	if (bCanCreateFluxesFromBases)
	{
		auto BuildingSelected = TryToSelectBuilding(HitResult);
		if (!BuildingSelected) ClearBuildingSelection();
		if (BuildingSelected)
		{
			GetWorld()->GetTimerManager().SetTimer(DragNDropTimer, this, &UFluxComponent::IsHoldingFluxCreation, DragNDropTime, false);
			PlayerControllerInfernale->MousePrimaryEnd.AddDynamic(this, &UFluxComponent::OnPrimaryActionEnded);
		}
	}

	auto FluxNodeSelected = TryToSelectFluxNode(HitResult);
	if (!FluxNodeSelected)
	{
		if (bDebugMoveNode) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("FluxNodeSelected is null"));
		ClearNodeSelection();
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(DragNDropTimer, this, &UFluxComponent::DragNDropSelectNode, DragNDropTime, false);
		bTryingToHoldPrimaryAction = true;
		PlayerControllerInfernale->MousePrimaryEnd.AddDynamic(this, &UFluxComponent::ClickNClickNode);
		return;
	}

	auto FluxSelected = TryToSelectFlux(HitResult);
	if (!FluxSelected) return;
	
	const auto FluxInteractedRef = FluxInteracted.Get();
	bTryingToHoldPrimaryAction = true;
	bWaitingForCreation = true;
	bWaitingForHoldType = true;
	GetWorld()->GetTimerManager().SetTimer(DragNDropTimer, this, &UFluxComponent::DragNDropOnNew, DragNDropTime, false);
	PlayerControllerInfernale->MousePrimaryEnd.AddDynamic(this, &UFluxComponent::ClickNClickOnNew);


	
	FluxInteractedRef->FluxNodeCreated.AddDynamic(this, &UFluxComponent::OnFluxNodeCreated);
	FluxInteractedRef->FluxNodeCreationFailed.AddDynamic(this, &UFluxComponent::OnFluxNodeCreationFailed);
	CreateFluxNodeServer(FluxInteractedRef, HitResult.ImpactPoint);
}

void UFluxComponent::OnPrimaryActionEnded()
{
	PlayerControllerInfernale->MousePrimaryEnd.RemoveDynamic(this, &UFluxComponent::OnPrimaryActionEnded);
	GetWorld()->GetTimerManager().ClearTimer(DragNDropTimer);
	DragNDropTimer.Invalidate();
}

void UFluxComponent::OnSecondaryActionStarted()
{
	SecondaryActive = true;
	if (FluxModeState != EFluxModeState::FMSNone)
	{
		OnSecondaryActionStartedMoveNode();
		return;
	}
	if (FluxNodeInteracted.IsValid())
	{
		ClearNodeSelection();
		return;
	}

	if (bCanCreateFluxesFromBases)
	{
		if (!Origins.IsEmpty())
		{
			CreateNewFluxes();
			return;
		}
	}

	FHitResult HitResult;
	if (const auto RemovingFlux = TryToFindFluxToRemove(HitResult, true); !RemovingFlux) return;
	
	FluxModeState = FluxHovered == nullptr ? EFluxModeState::FMSRemoveFluxNode : EFluxModeState::FMSRemoveFlux;
	RemoveFluxTimerElapsed = 0.0f;
	FluxModeRemoving.Broadcast(true, FluxModeState, RemoveFluxDuration);
	bHoverLocked = true;

	if (bRemoveMustStayOnFlux) PlayerControllerInfernale->MouseSecondaryTriggered.AddDynamic(this, &UFluxComponent::OnSecondaryActionTriggered);
	PlayerControllerInfernale->MouseSecondaryEnd.AddDynamic(this, &UFluxComponent::OnSecondaryActionEnded);
}

void UFluxComponent::OnSecondaryActionEnd()
{
	SecondaryActive = false;
}

void UFluxComponent::OnEscapeStarted()
{}

void UFluxComponent::OnTransmutationModeChanged(bool bTransmutationMode)
{
	bInteractionsAllowed = !bTransmutationMode;
}

void UFluxComponent::OnSecondaryActionTriggered()
{
	if (!IsRemovingMode()) return;

	if (FluxModeState == EFluxModeState::FMSRemoveFlux)
	{
		RemoveFluxTriggered();
		return;
	}
	RemoveFluxNodeTriggered();
}

void UFluxComponent::RemoveFluxTriggered()
{
	FHitResult HitResult;
	auto RemovingFlux = TryToFindFluxToRemove(HitResult, false);
	if (RemovingFlux) return;
	
	if (bRemoveMustStayOnFlux) PlayerControllerInfernale->MouseSecondaryTriggered.RemoveDynamic(this, &UFluxComponent::OnSecondaryActionTriggered);
	PlayerControllerInfernale->MouseSecondaryEnd.RemoveDynamic(this, &UFluxComponent::OnSecondaryActionEnded);
	FluxToRemove = nullptr;
	FluxModeRemoving.Broadcast(false, FluxModeState, RemoveFluxDuration);
	FluxModeState = EFluxModeState::FMSNone;
}

void UFluxComponent::RemoveFluxNodeTriggered()
{
	FHitResult HitResult;
	auto RemovingFluxNode = TryToFindFluxNodeToRemove(HitResult, false);
	if (RemovingFluxNode) return;

	if (bRemoveMustStayOnFlux) PlayerControllerInfernale->MouseSecondaryTriggered.RemoveDynamic(this, &UFluxComponent::OnSecondaryActionTriggered);
	PlayerControllerInfernale->MouseSecondaryEnd.RemoveDynamic(this, &UFluxComponent::OnSecondaryActionEnded);
	FluxNodeToRemove = nullptr;
	FluxModeRemoving.Broadcast(false, FluxModeState, RemoveFluxDuration);
	FluxModeState = EFluxModeState::FMSNone;
	bHoverLocked = false;
}

void UFluxComponent::UpdateMoveNodeOrigin()
{
	if (!MoveNodeCanUseOriginalNode) return;
	
	if (PlayerControllerInfernale->IsShiftPressed())
	{
		if (UsingNodeAtOriginalPos) return;
		UseOriginalNode(true);
		return;
	}

	if (!PlayerControllerInfernale->IsShiftPressed())
	{
		if (!UsingNodeAtOriginalPos) return;
		UseOriginalNode(false);
		return;
	}
}

void UFluxComponent::UseOriginalNode(bool bActive)
{
	UsingNodeAtOriginalPos = bActive;
	UseOriginalNodeServer(bActive);
}

void UFluxComponent::MoveNodeSaved(AFluxNode* FluxNode, AFlux* Flux)
{
	if (!bWasEverValidSaved)
	{
		Flux->ResetFluxServer();
		return;
	}
	const auto FluxNodeOtherMovingRef = FluxNodeOtherMoving.IsValid() ? FluxNodeOtherMoving.Get() : nullptr;
	Flux->MoveNode(FluxNode, MoveTargetSaved, BeforePathSaved, AfterPathSaved, OriginalToNodePathSaved, FluxNodeOtherMovingRef, UseOriginalSaved);
	bWasEverValidSaved = false;
}

void UFluxComponent::UseOriginalNodeServer_Implementation(bool bUseOriginal)
{
	if (!MoveNodeCanUseOriginalNode) return;
	UsingNodeAtOriginalPos = bUseOriginal;
	const auto FluxNodeOtherMovingRef = FluxNodeOtherMoving.IsValid() ? FluxNodeOtherMoving.Get() : nullptr;
	UseOriginalNodeOwner(bUseOriginal, FluxNodeOtherMovingRef);
}

void UFluxComponent::UseOriginalNodeOwner_Implementation(bool bUseOriginal, AFluxNode* OtherNode)
{
	if (!MoveNodeCanUseOriginalNode) return;
	TWeakObjectPtr<AFluxNode> OtherNodePtr = TWeakObjectPtr<AFluxNode>(OtherNode);
	if (OtherNode && OtherNodePtr.IsValid()) OtherNode->SetNodeVisibility(bUseOriginal);
	else GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("UseOriginalNodeOwner_Implementation: FluxNodeOtherMoving is null"));
}


bool UFluxComponent::GetPathfindingFrom(AFluxNode* FluxNode, const FVector Start, const FVector End, TArray<FPathStruct>& Pathfinding, bool bMousePosValid, const FVector HitPoint, const FVector OriginLoc, const float MaxDistance)
{
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation OutNavLoc;
	
	auto Target = Start;
	bool TargetReachable = NavSys->ProjectPointToNavigation(Target, OutNavLoc, FVector(500, 500, 100));
	if (!TargetReachable)
	{
		if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Start: Target not reachable"));
		if (bDebugPathfinding) DrawDebugSphere(GetWorld(), Target, 600, 12, FColor::Purple, false, 0, 0, 15);
		return false;
	}
	Target = OutNavLoc.Location;
	auto StartLocation = Target;
	//DrawDebugSphere(GetWorld(), StartLocation, 600, 12, FColor::Blue, false, 5, 0, 15);

	
	TargetReachable = NavSys->ProjectPointToNavigation(End, OutNavLoc, FVector(500, 500, 100));
	if (!TargetReachable)
	{
		if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("End: Target not reachable"));
		if (bDebugPathfinding) DrawDebugSphere(GetWorld(), Target, 600, 12, FColor::Purple, false, 0, 0, 15);
		return false;
	}
	Target = OutNavLoc.Location;
	auto EndLocation = Target;
	//DrawDebugSphere(GetWorld(), EndLocation, 600, 12, FColor::Purple, false, 5, 0, 15);
	


	/* Pathfinding (Before) */
	UNavigationPath* Path = NavSys->FindPathToLocationSynchronously(GetWorld(), StartLocation, EndLocation, FluxNode);
	if (Path == nullptr)
	{
		if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Path is null"));
		return false;
	}
	if (Path->IsPartial())
	{
		if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Path is partial"));
		return false;
	}
	
	auto PreviousPoint = StartLocation;
	for (auto Point : Path->PathPoints)
	{
		Pathfinding.Add(FPathStruct(Point, false));
		if (bDebugPathfinding)
		{
			DrawDebugSphere(GetWorld(), Point, 500, 12, FColor::Red, false, 0, 0, 10);
			DrawDebugLine(GetWorld(), PreviousPoint, Point, FColor::Red, false, 0, 0, 10);
		}
		PreviousPoint = Point;
	}
	if (Pathfinding.Num() == 0)
	{
		if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PathStructs is empty"));
		return false;
	}

	// if (Pathfinding.Num() > 0) Pathfinding.RemoveAt(0);
	// if (Pathfinding.Num() > 0) Pathfinding.RemoveAt(Pathfinding.Num() - 1);
	return true;
}

void UFluxComponent::OnSecondaryActionEnded()
{
	if (bRemoveMustStayOnFlux) PlayerControllerInfernale->MouseSecondaryTriggered.RemoveDynamic(this, &UFluxComponent::OnSecondaryActionTriggered);
	PlayerControllerInfernale->MouseSecondaryEnd.RemoveDynamic(this, &UFluxComponent::OnSecondaryActionEnded);
	FluxToRemove = nullptr;
	FluxModeRemoving.Broadcast(false, FluxModeState, RemoveFluxDuration);
	if (FluxModeState == EFluxModeState::FMSRemoveFluxNode || FluxModeState == EFluxModeState::FMSRemoveFlux) FluxModeState = EFluxModeState::FMSNone;
	bHoverLocked = false;
}

void UFluxComponent::RemoveFlux()
{
	if (!FluxToRemove.IsValid()) return;
	RemoveFluxServer(FluxToRemove.Get());
	FluxToRemove = nullptr;
	FluxModeRemoving.Broadcast(false, FluxModeState, RemoveFluxDuration);
	FluxModeState = EFluxModeState::FMSNone;
	bHoverLocked = false;
}

void UFluxComponent::RemoveFluxNode()
{
	if (!FluxNodeToRemove.IsValid()) return;
	RemoveFluxNodeServer(FluxNodeToRemove.Get());
	FluxNodeToRemove = nullptr;
	FluxModeRemoving.Broadcast(false, FluxModeState, RemoveFluxDuration);
	FluxModeState = EFluxModeState::FMSNone;
	bHoverLocked = false;
}

void UFluxComponent::IsHoldingFluxCreation()
{
	if (!DragNDropTimer.IsValid()) return;
	if (Origins.IsEmpty()) return;
	CreateFlux(Origins[0], false);

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UFluxComponent::DragNDropFluxCreatedAndReplicated, 0.05, false);
}

void UFluxComponent::DragNDropSelectNode()
{
	if (!bTryingToHoldPrimaryAction) return;
	PlayerControllerInfernale->MousePrimaryEnd.RemoveDynamic(this, &UFluxComponent::ClickNClickNode);
	bTryingToHoldPrimaryAction = false;
	if (!DragNDropTimer.IsValid()) return;
	if (!FluxNodeInteracted.IsValid()) return;

	const auto FluxActive = FluxNodeInteracted->GetFlux()->IsFluxActive();
	auto AllowedToCreate = false;
	if (FluxActive) AllowedToCreate = FluxNodeInteracted->GetNodeType() != EFluxNodeType::StartNode;
	AddEventsMoveSelectedFluxNode(AllowedToCreate, true);
}

void UFluxComponent::ClickNClickNode()
{
	if (!bTryingToHoldPrimaryAction) return;
	PlayerControllerInfernale->MousePrimaryEnd.RemoveDynamic(this, &UFluxComponent::ClickNClickNode);
	bTryingToHoldPrimaryAction = false;
	
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("ClickNClickNode"));
	if (!FluxNodeInteracted.IsValid()) return;

	const auto FluxActive = FluxNodeInteracted->GetFlux()->IsFluxActive();
	auto AllowedToCreate = false;
	if (FluxActive) AllowedToCreate = FluxNodeInteracted->GetNodeType() != EFluxNodeType::StartNode;
	AddEventsMoveSelectedFluxNode(AllowedToCreate, false);
}

void UFluxComponent::OnSecondaryActionStartedMoveNode()
{
	FHitResult HitResult;
	const auto PreviousFluxNode = FluxNodeInteracted.Get();
	FluxNodeInteracted->EnableCollisionLocal(false);
	FluxNodeInteracted = nullptr;
	CreateHitResultFlux(HitResult);

	const auto ActorHit = HitResult.GetActor();
	bool FluxNodeSelected;
	if (ActorHit != nullptr)
	{
		const auto FluxNode = Cast<AFluxNode>(ActorHit);
		if (FluxNode != nullptr)
		{
			FluxNodeSelected = TryToSelectFluxNode(HitResult);
			CreateHitResultFloor(HitResult);
		}
		else
		{
			const auto Flux = Cast<AFlux>(ActorHit);
			if (Flux != nullptr)
			{
				FluxNodeSelected = TryToSelectFluxNode(HitResult);
				CreateHitResultFloor(HitResult);
			}
			else
			{
				FluxNodeSelected = TryToSelectFluxNode(HitResult);
				const auto NeutralCamp = Cast<ANeutralCamp>(ActorHit);
				if (NeutralCamp != nullptr)
				{
					FluxNodeSelected = TryToSelectFluxNode(HitResult);
					CreateHitResultFloor(HitResult);
				}
				else
				{
					FluxNodeSelected = TryToSelectFluxNode(HitResult);
				}
			}
		}
	}
	else
	{
		FluxNodeSelected = TryToSelectFluxNode(HitResult);
	}

	//auto FluxNodeSelected = TryToSelectFluxNode(HitResult);
	if (!FluxNodeSelected)
	{
		if (bDebugMoveNode) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("No flux node to remove, let's create a new one"));
		const auto Flux = PreviousFluxNode->GetFlux();
		
		Flux->FluxNodeCreated.AddDynamic(this, &UFluxComponent::OnFluxNodeCreatedWhileMoving);
		Flux->FluxNodeCreationFailed.AddDynamic(this, &UFluxComponent::OnFluxNodeCreationFailedWhileMoving);
		
		CreateNewFluxNodeServer(PreviousFluxNode, HitResult.ImpactPoint);
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Creating new flux node"));
	}
	else if (!FluxNodeInteracted.IsValid())
	{}
	else if (FluxNodeInteracted.Get() != PreviousFluxNode && FluxNodeInteracted->GetNodeType() != EFluxNodeType::StartNode && FluxNodeInteracted->GetNodeType() != EFluxNodeType::EndNode)
	{
		if (FluxNodeInteracted.Get() == FluxNodeOtherMoving.Get()) return;
		const auto Flux = FluxNodeInteracted->GetFlux();
		const auto PrevFlux = PreviousFluxNode->GetFlux();
		if (Flux != nullptr && PrevFlux != nullptr)
		{
			if (Flux == PrevFlux)
			{
				if (bDebugMoveNode) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("FluxNodeInteracted: %s"), *FluxNodeInteracted->GetName()));
				RemoveFluxNodeServer(FluxNodeInteracted.Get());
			}
		}
		
	}
	else
	{
		if (bDebugMoveNode) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Trying to remove the same flux node"));
	}
	FluxNodeInteracted = TWeakObjectPtr<AFluxNode>(PreviousFluxNode);
	FluxNodeInteracted->EnableCollisionLocal(true);
	PlayerControllerInfernale->MouseSecondaryEnd.AddDynamic(this, &UFluxComponent::OnSecondaryActionEndedMoveNode);
	PlayerControllerInfernale->MousePrimaryEnd.AddDynamic(this, &UFluxComponent::CleanMoveNodeEvents);
}

void UFluxComponent::OnSecondaryActionEndedMoveNode()
{
	PlayerControllerInfernale->MouseSecondaryEnd.RemoveDynamic(this, &UFluxComponent::OnSecondaryActionEndedMoveNode);
	PlayerControllerInfernale->MousePrimaryEnd.RemoveDynamic(this, &UFluxComponent::CleanMoveNodeEvents);
}

void UFluxComponent::CleanMoveNodeEvents()
{
	PlayerControllerInfernale->MouseSecondaryEnd.RemoveDynamic(this, &UFluxComponent::OnSecondaryActionEndedMoveNode);
	PlayerControllerInfernale->MousePrimaryEnd.RemoveDynamic(this, &UFluxComponent::CleanMoveNodeEvents);
}

void UFluxComponent::OnShiftModeChanged()
{
	UpdateMoveNodeOrigin();
}

void UFluxComponent::SelectBuildingServer_Implementation(ABuildingParent* Building)
{
	if (Building == nullptr) return;
	Building->OnSelectedForFluxCpp();
}

void UFluxComponent::DeselectBuildingServer_Implementation(ABuildingParent* Building)
{
	if (Building == nullptr) return;
	Building->OnDeselectedForFluxCpp();
}

void UFluxComponent::RemoveFluxServer_Implementation(AFlux* Flux)
{
	TWeakObjectPtr<AFlux> FluxWeak = TWeakObjectPtr<AFlux>(Flux);
	if (Flux == nullptr) return;
	if (!FluxWeak.IsValid()) return;
	//Flux->RemoveFluxServer();
	Flux->ResetFluxServer();
}

void UFluxComponent::RemoveFluxNodeServer_Implementation(AFluxNode* FluxNode)
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("RemoveFluxNodeServer"));
	const auto Flux = FluxNode->GetFlux();
	const auto InitialIndex = FluxNode->GetNodeIndex();
	const auto PreviousIndex = Flux->GetPreviousNodeIndex(InitialIndex, true);
	Flux->RemoveFluxNodeServer(FluxNode);
	if (!Flux->IsFluxActive())
	{
		if (bDebugNodeRemovalPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Flux is not active anymore"));
		return;
	}
	
	/* We have to redo the pathfinding */
	const auto NextIndex= Flux->GetNextNodeIndex(PreviousIndex);
	if (NextIndex <= 0)
	{
		if (bDebugNodeRemovalPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("NextIndex is <= 0"));
		return;
	}

	const auto StartNode = Flux->GetFluxNodes()[PreviousIndex];
	const auto EndNode = Flux->GetFluxNodes()[NextIndex];
	const auto StartNodeLoc = StartNode->GetActorLocation();
	const auto EndNodeLoc = EndNode->GetActorLocation();

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	TArray<FPathStruct> PathStructs = TArray<FPathStruct>();
	UNavigationPath* Path = NavSys->FindPathToLocationSynchronously(GetWorld(), StartNodeLoc, EndNodeLoc, Flux);
	if (Path == nullptr)
	{
		if (bDebugNodeRemovalPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Path is null"));
		return;
	}
	if (Path->IsPartial())
	{
		if (bDebugNodeRemovalPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Path is partial"));
		return;
	}

	auto PreviousPoint = StartNodeLoc;
	for (auto Point : Path->PathPoints)
	{
		PathStructs.Add(FPathStruct(Point, false));
		if (bDebugNodeRemovalPathfinding)
		{
			DrawDebugSphere(GetWorld(), Point, 500, 12, FColor::Red, false, 5, 0, 10);
			DrawDebugLine(GetWorld(), PreviousPoint, Point, FColor::Red, false, 5, 0, 10);
		}
		PreviousPoint = Point;
	}
	
	if (PathStructs.Num() < 2)
	{
		if (bDebugNodeRemovalPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PathStructs is empty"));
		return;
	}
	PathStructs.RemoveAt(0);
	PathStructs.RemoveAt(PathStructs.Num() - 1);

	Flux->InsertNodes(PathStructs, PreviousIndex+1, false);
}

void UFluxComponent::SelectFluxNodeServer_Implementation(AFluxNode* FluxNode)
{
	if (FluxNode == nullptr) return;
	FluxNode->OnSelectedForFluxCpp();
}

void UFluxComponent::DeselectFluxNodeServer_Implementation(AFluxNode* FluxNode)
{
	if (FluxNode == nullptr) return;
	FluxNode->OnDeselectedForFluxCpp();
}

void UFluxComponent::SetMoveSelectedNodeCanUseOtherServer_Implementation(bool Value)
{
	MoveNodeCanUseOriginalNode = Value;
}

void UFluxComponent::CreateNewFluxNodeServer_Implementation(AFluxNode* FluxNode, FVector Target)
{
	const auto Flux = FluxNode->GetFlux();
	auto Structs = TArray<FPathStruct>();

	/* Pathfinding to the new node */
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation OutNavLoc;
	FVector TargetLoc = Target;
	bool TargetReachable = NavSys->ProjectPointToNavigation(TargetLoc, OutNavLoc, FVector(500, 500, 100));
	if (!TargetReachable)
	{
		if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Target not reachable"));
		TargetLoc = MoveTargetSaved;
		//return;
	}
	if (bDebugPathfinding) DrawDebugSphere(GetWorld(), TargetLoc, 500, 12, FColor::Orange, false, 0, 0, 10);

	const auto OriginLoc = Flux->GetOrigin()->GetActorLocation();
	const auto PreviousRealNode = Flux->GetPreviousNode(FluxNode->GetNodeIndex(), true);
	if (PreviousRealNode == nullptr)
	{
		if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PreviousRealNode is null"));
		return;
	}
	const auto StartLoc = PreviousRealNode->GetActorLocation();
	const auto Valid = GetPathfindingFrom(FluxNode, StartLoc, TargetLoc, Structs, true, TargetLoc, OriginLoc, 500);
	if (!Valid)
	{
		if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("GetPathfindingFrom failed"));
		return;
	}
	if (Structs.Num() > 0) Structs.RemoveAt(0);
	if (Structs.Num() > 0) Structs[Structs.Num() - 1].IsReal = true;

	for (auto [PathPoint, IsReal] : Structs)
	{
		if (bDebugPathfinding) DrawDebugSphere(GetWorld(), PathPoint, 500, 12, FColor::Green, false, 5.f, 0, 10);
	}

	MoveNodeCanUseOriginalNode = false;
	const auto NewNode = Flux->InsertNodes(Structs, FluxNode->GetNodeIndex(), true);
	const TWeakObjectPtr<AFluxNode> WeakObjectPtr = TWeakObjectPtr<AFluxNode>(NewNode);
	FluxNodesCreateWhileMoving.Add(WeakObjectPtr);
}

void UFluxComponent::MoveSelectedFluxNode(bool Release)
{
	if (!FluxNodeInteracted.IsValid())
	{
		if (bDebugMoveNode) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("FluxNodeInteracted is not valid"));
		return;
	}
	
	const auto Pawn = PlayerControllerInfernale->GetInfernalePawn();
	if (Pawn == nullptr) return;
	
	const auto CameraComponent = Pawn->GetCameraComponent();
	if (CameraComponent == nullptr) return;

	const auto Start = CameraComponent->GetComponentLocation();

	FVector MouseWorldLocation;
	FVector MouseWorldDirection;

	if (auto Success = PlayerControllerInfernale->DeprojectMousePositionToWorld(MouseWorldLocation, MouseWorldDirection); !Success)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to deproject mouse position to world"));
		return;
	}

	TArray<AActor*> Targetables = TArray<AActor*>();
	UGameplayStatics::GetAllActorsWithInterface(GetWorld(), UUnitTargetable::StaticClass(), Targetables);
	for (const auto& Unit : Targetables)
	{
		IUnitTargetable* Targetable = Cast<IUnitTargetable>(Unit);
		// use here if you want all targetables to display their detection ring
		Targetable->Execute_ToggleDetectionDecal(Unit, false);
	}

	const auto Direction = MouseWorldDirection.GetSafeNormal();
	const auto End = Start + Direction * UFunctionLibraryInfernale::GetGameSettingsDataAsset()->RayCastLength;

	float MouseX;
	float MouseY;
	PlayerControllerInfernale->GetMousePosition(MouseX, MouseY);
	if (MouseX == 0 && MouseY == 0)
	{
		if (Release) MoveSelectedFluxNodeServer(FluxNodeInteracted.Get(), LastVectorStart, LastVectorEnd, Release);
		else MoveSelectedFluxNodeServerUnreliable(FluxNodeInteracted.Get(), LastVectorStart, LastVectorEnd, Release);
		return;
	}

	LastVectorStart = Start;
	LastVectorEnd = End;
	if (Release) MoveSelectedFluxNodeServer(FluxNodeInteracted.Get(), Start, End, Release);
	else MoveSelectedFluxNodeServerUnreliable(FluxNodeInteracted.Get(), Start, End, Release);
}

void UFluxComponent::MoveSelectedFluxNodeNoRelease()
{
	MoveSelectedFluxNode(false);
}

void UFluxComponent::DragNDropFluxCreatedAndReplicated()
{
	if (bWaitingForFluxReplication)
	{
		FTimerHandle TimerHandle;
		if (bDebugReplication) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, TEXT("NewFluxCreatedAndReplicated::Waiting for replication"));
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UFluxComponent::DragNDropFluxCreatedAndReplicated, 0.05, false);
		return;
	}
	if (!FluxToRemove.Get())
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("LastFluxCreated (FluxInteracted) is null"));
		return;
	}

	AddNodeToSelection(FluxToRemove->GetEndNode().Get());
	AddEventsMoveSelectedFluxNode(false, true);
	ClearBuildingSelection();
}

void UFluxComponent::DragNDropOnNew()
{
	if (!bWaitingForHoldType) return;
	bWaitingForHoldType = false;
	bTryingToHoldPrimaryAction = true;

	GetWorld()->GetTimerManager().ClearTimer(DragNDropTimer);
	PlayerControllerInfernale->MousePrimaryEnd.RemoveDynamic(this, &UFluxComponent::ClickNClickOnNew);
	FluxNodeCreatedCheck();
}

void UFluxComponent::ClickNClickOnNew()
{
	if (!bWaitingForHoldType) return;
	bWaitingForHoldType = false;
	bTryingToHoldPrimaryAction = false;

	GetWorld()->GetTimerManager().ClearTimer(DragNDropTimer);
	PlayerControllerInfernale->MousePrimaryEnd.RemoveDynamic(this, &UFluxComponent::ClickNClickOnNew);
	FluxNodeCreatedCheck();
}

void UFluxComponent::MoveSelectedFluxNodeServerUnreliable_Implementation(AFluxNode* FluxNode, FVector Start,
	FVector End, bool Release)
{
	MoveSelectedFluxNodeLocal(FluxNode, Start, End, Release);
}

void UFluxComponent::MoveSelectedFluxNodeServer_Implementation(AFluxNode* FluxNode, FVector Start, FVector End, bool Release)
{
	MoveSelectedFluxNodeLocal(FluxNode, Start, End, Release);
}

void UFluxComponent::CalculateFluxTargets(AFluxNode* FluxNode, AFlux* Flux, TArray<float> FluxPower, bool Release)
{
	const UMassEntityTraitBase* EntityDefaultTrait = UFunctionLibraryInfernale::GetGameSettingsDataAsset()->DataAssetsSettings[0].AmalgamAssets[0]->FindTrait(UAmalgamTraitBase::StaticClass());
	const UAmalgamTraitBase* AmalgamDefaultTrait = Cast<UAmalgamTraitBase>(EntityDefaultTrait);

	const float MaxDistance = AmalgamDefaultTrait->DetectionParams.BaseDetectionRange;
	const float MaxAngle = AmalgamDefaultTrait->DetectionParams.BaseDetectionAngle;

	TArray<AActor*> TargetablesByFlux = TArray<AActor*>();
	TArray<AActor*> TargetablesAvailable = TArray<AActor*>();
	UGameplayStatics::GetAllActorsWithInterface(GetWorld(), UUnitTargetable::StaticClass(), TargetablesAvailable);

	TArray<FActorVector> TargetablesAvailableLoc = TArray<FActorVector>();

	for (auto TargetableAvailable : TargetablesAvailable)
	{
		auto Loc = TargetableAvailable->GetActorLocation();
		Loc.Z = 0;
		FActorVector ActorVector;
		ActorVector.Actor = TargetableAvailable;
		ActorVector.Vector = Loc;
		TargetablesAvailableLoc.Add(ActorVector);
	}
	
	if (FluxNode == nullptr) return;
	auto Points = Flux->GetPreviewPoints();
	if (Points.Num() < 2) return;
	
	TArray<FVector> CheckLocations = TArray<FVector>();
	float DistanceAlongSegment = 0.f;
	float CurrentSegmentLength = (Points[1] - Points[0]).Length();
	if (bDebugFluxTargets) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::SanitizeFloat(CurrentSegmentLength));
	FVector SegDirection = (Points[1] - Points[0]).GetSafeNormal();

	// loop step is used inside, to allow the loop to keep running without incrementing the checked point index
	// Checks for a point along the current segment each pass and verifies it still is inside the current segment
	// jumps to next segment if point is outside 
	for (int i = 0; i < Points.Num() - 1;)
	{
		DistanceAlongSegment += AnticipationInterval;
		if (DistanceAlongSegment >= CurrentSegmentLength)
		{
			++i;
			if (i == Points.Num() - 1) break;

			DistanceAlongSegment = 0.f;
			CurrentSegmentLength = (Points[i + 1] - Points[i]).Length();
			SegDirection = (Points[i + 1] - Points[i]).GetSafeNormal();
			continue;
		}

		CheckLocations.Add(Points[i] + (SegDirection * DistanceAlongSegment));
	}

	CheckLocations.Append(Points); // Add already known points along line

	TArray<TWeakObjectPtr<ALDElement>> LDElems = TArray<TWeakObjectPtr<ALDElement>>();
	TArray<TWeakObjectPtr<ABuildingParent>> Buildings = TArray<TWeakObjectPtr<ABuildingParent>>();
	TArray<TWeakObjectPtr<ASoulBeacon>> Beacons = TArray<TWeakObjectPtr<ASoulBeacon>>();
	TArray<TWeakObjectPtr<ABoss>> Bosses = TArray<TWeakObjectPtr<ABoss>>();

	for (int Index = 0; Index < CheckLocations.Num(); ++Index)
	{
		FVector Direction;
		const FVector CurrentLoc = CheckLocations[Index];
		
		if (Index == CheckLocations.Num() - 1)
			Direction = CheckLocations[Index] - CheckLocations[Index - 1];
		else
			Direction = (CheckLocations[Index + 1] - CheckLocations[Index]).GetSafeNormal();

		//DEBUG
		/*TArray<TWeakObjectPtr<ALDElement>> TempLD = ASpatialHashGrid::FindLDElementsInRange(CheckLocations[Index], MaxDistance, MaxAngle, Direction, FMassEntityHandle());
		TArray<TWeakObjectPtr<ABuildingParent>> TempB = ASpatialHashGrid::FindBuildingsInRange(CheckLocations[Index], MaxDistance, MaxAngle, Direction, FMassEntityHandle());

		for (const auto& LD : TempLD)
			DrawDebugLine(GetWorld(), CheckLocations[Index], LD->GetActorLocation(), FColor::Green, false, 1.5f);
		for (const auto& B : TempB)
			DrawDebugLine(GetWorld(), CheckLocations[Index], B->GetActorLocation(), FColor::Green, false, 1.5f);

		LDElems.Append(TempLD);
		Buildings.Append(TempB);*/
		//END DEBUG
		if (bDebugFluxTargets) DrawDebugSphere(GetWorld(), CurrentLoc, 500, 12, FColor::Red, false, 0, 0, 10);

		TWeakObjectPtr<ASoulBeacon> FoundSB = ASpatialHashGrid::IsInSoulBeaconRange(CurrentLoc);

		if (FoundSB.IsValid())
		{
			for (const auto& Range : FoundSB->GetCaptureRanges())
			{
				if ((Range.GetPosition() - CurrentLoc).Length() < MaxDistance)
				{
					Beacons.AddUnique(FoundSB);
					break;
				}
			}
		}

		TWeakObjectPtr<ABoss> FoundBoss = ASpatialHashGrid::IsInBossRange(CurrentLoc);

		if (FoundBoss.IsValid())
			Bosses.AddUnique(FoundBoss);

		LDElems.Append(ASpatialHashGrid::FindLDElementsInRange(CurrentLoc, MaxDistance, MaxAngle, Direction, FMassEntityHandle()));
		Buildings.Append(ASpatialHashGrid::FindBuildingsInRange(CurrentLoc, MaxDistance, MaxAngle, Direction, FMassEntityHandle()));
	}

	//if (LDElems.Num() + Buildings.Num() == 0) return;

	for (TWeakObjectPtr<ALDElement> Element : LDElems)
	{
		TargetablesByFlux.Add(Element.Get());
	}
	for (TWeakObjectPtr<ABuildingParent> Building : Buildings)
	{
		TargetablesByFlux.Add(Building.Get());
	}
	for (TWeakObjectPtr<ASoulBeacon> Beacon : Beacons)
	{
		TargetablesByFlux.Add(Beacon.Get());
	}
	for (TWeakObjectPtr<ABoss> Boss : Bosses)
	{
		/* Bosses have an invalid PIE InstanceID */
		//TargetablesByFlux.Add(Boss.Get());
	}

	if (bDebugFluxTargets) GEngine->AddOnScreenDebugMessage(1, 2.5, FColor::Orange, FString::Printf(TEXT("%d entities targetable by flux"), TargetablesByFlux.Num()));

	SetTargetsOwner(TargetablesByFlux, TargetablesAvailable, Flux, FluxPower);
}

void UFluxComponent::ResetFluxTargets()
{
	TArray<AActor*> TargetablesAvailable = TArray<AActor*>();
	UGameplayStatics::GetAllActorsWithInterface(GetWorld(), UUnitTargetable::StaticClass(), TargetablesAvailable);

	for (auto& Target : TargetablesAvailable)
	{
		IUnitTargetable* Targetable = Cast<IUnitTargetable>(Target);
		Targetable->bIsDetected = false;
		Targetable->Execute_DisableDetectionVFX(Target);
	}
}

void UFluxComponent::RefreshSplinesForAmalgamsServer_Implementation(AFluxNode* FluxNode)
{
	if (FluxNode == nullptr) return;
	const auto Flux = FluxNode->GetFlux();
	if (Flux == nullptr) return;
	const auto NodeIndex = FluxNode->GetNodeIndex();
	
	FluxNode->GetFlux()->RefreshSplinesForAmalgams(NodeIndex);
}

void UFluxComponent::ReleaseSelectedFluxNode()
{
	MoveSelectedFluxNode(true);
	if (FluxNodeInteracted.IsValid()) RefreshSplinesForAmalgamsServer(FluxNodeInteracted.Get());
	
	RemoveEventsMoveSelectedFluxNode();
	ClearNodeSelection();
	ResetFluxTargets();
}

void UFluxComponent::MoveSelectedFluxNodeLocal(AFluxNode* FluxNode, FVector Start, FVector End, bool Release)
{
	FHitResult HitResult;
	TWeakObjectPtr<AFluxNode> FluxNodePtr = TWeakObjectPtr<AFluxNode>(FluxNode);
	if (!FluxNode || !FluxNode->IsValidLowLevel() || !FluxNodePtr.IsValid())
	{
		if (bDebugMoveNode) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("MoveSelectedFluxNodeServer_Implementation: FluxNode is null"));
		RemoveEventsMoveSelectedFluxNode();
		ClearNodeSelection();
		return;
	}
	auto Params = FCollisionQueryParams::DefaultQueryParam;
	Params.AddIgnoredActor(PlayerControllerInfernale);
	Params.AddIgnoredActor(FluxNode);
	Params.AddIgnoredActor(FluxNode->GetFlux());
	auto Channel = UFunctionLibraryInfernale::GetCustomTraceChannel(ECustomTraceChannel::CameraCollision);
	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, Channel, Params);
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Hit Actor: %s"), *HitResult.GetActor()->GetName()));
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Start : %s, End: %s"), *Start.ToString(), *End.ToString()));
	//DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 50, 0, 50);
	//DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 500, 12, FColor::Red, false, 5, 0, 10);

	if (!Release)
	{
		if (bShouldUpdateMoveTimer && FluxUpdateTimer > 0) return;
		if (!bShouldUpdateMoveTimer)
		{
			bShouldUpdateMoveTimer = true;
			FluxUpdateTimer = FluxUpdateInterval;
		}
		else
		{
			FluxUpdateTimer = FluxUpdateInterval;
			bShouldUpdateMoveTimer = false;
		}
	}

	/* Check distances */
	if (Release)
	{
		bShouldUpdateMoveTimer = false;
		FluxUpdateTimer = FluxUpdateInterval;
	}
	
	const auto Origin = FluxNode->GetFlux()->GetOrigin();
	const auto OriginLoc = Origin->GetActorLocation();
	const auto Distance = FVector::Distance(OriginLoc, HitResult.ImpactPoint);
	auto OriginMainBuilding = Cast<AMainBuilding>(Origin);
	if (!OriginMainBuilding)
	{
		if (bDebugMoveNode) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("MoveSelectedFluxNodeServer_Implementation: OriginMainBuilding is null"));
		RemoveFluxNodeServer(FluxNode);
		return;
	}
	auto MainBuildingLoc = OriginMainBuilding->GetActorLocation();

	
	if (FluxNode->GetNodeType() == EFluxNodeType::StartNode)
	{
		MoveStartNode(FluxNode, OriginMainBuilding, HitResult.ImpactPoint, Release);
		return;
	}

	auto MaxDistance = OriginMainBuilding->GetControlAreaRadius();
	MaxDistance = GameModeInfernale->GetRadiusFromGameDuration(MaxDistance);
	MaxDistance = PlayerControllerInfernale->GetTransmutationComponent()->GetEffectFluxRange(MaxDistance);
	const auto MousePosValid = Distance <= MaxDistance;

	// if (!MousePosValid && Release)
	// {
	// 	RemoveFluxNodeServer(FluxNode);
	// 	return;
	// }
	
	const auto Flux = FluxNode->GetFlux();
	if (Flux == nullptr)
	{
		if (bDebugMoveNode) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("MoveSelectedFluxNodeServer_Implementation: Flux is null O.o"));
		if (Release) MoveNodeSaved(FluxNode, Flux);
		return;
	}
	auto FluxNodeIndex = FluxNode->GetNodeIndex();
	Flux->ClearAllPreviousFakePoints(FluxNodeIndex);
	

	/* FluxPathfinding */
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation OutNavLoc;
	TArray<FPathStruct> BeforeToOriginalNodePathStructs = TArray<FPathStruct>();
	TArray<FPathStruct> BeforeNodePathStructs = TArray<FPathStruct>();
	TArray<FPathStruct> AfterNodePathStructs = TArray<FPathStruct>();
	
	/* Before location */
	/* Start location */

	
	auto UsingOriginal = MoveNodeCanUseOriginalNode && UsingNodeAtOriginalPos;
	auto OriginalNodeLoc = FVector::ZeroVector;
	if (UsingOriginal)
	{
		if (!FluxNodeOtherMoving.IsValid())
		{
			if (bDebugMoveNode) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("MoveSelectedFluxNodeServer_Implementation: !FluxNodeOtherMoving"));
			if (Release) MoveNodeSaved(FluxNode, Flux);
			return;
		}
		OriginalNodeLoc = FluxNodeOtherMoving->GetActorLocation();
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("OriginalNode index: %d"), FluxNodeOtherMovingInitialIndex));
	}
	auto PreviousIndex = Flux->GetPreviousNodeIndex(FluxNodeIndex, UsingOriginal);
	bool ValidPathfinding = false;
	if (PreviousIndex == -1)
	{
		if (bDebugMoveNode) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("MoveSelectedFluxNodeServer_Implementation: Previous index is -1"));
		if (Release) MoveNodeSaved(FluxNode, Flux);
		return;
	}
	FVector NodeNewLocation;

	if (UsingOriginal)
	{
		/* Pathfinding Previous to OriginalNode */
		auto StartLocation = Flux->GetFluxNodes()[PreviousIndex]->GetActorLocation();
		ValidPathfinding = GetPathfindingFrom(FluxNode, StartLocation, OriginalNodeLoc, BeforeToOriginalNodePathStructs, MousePosValid, HitResult.ImpactPoint, OriginLoc, MaxDistance);
		if (!ValidPathfinding)
		{
			if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("BeforeToOriginalNodePathStructs is invalid"));
			return;
		}

		/* Pathfinding OriginalNode to NewNode */
		auto Target = GetTargetPosition(HitResult, OriginalNodeLoc, FluxNode, UsingOriginal);
		if (!MousePosValid)
		{
			auto DirToValid = (HitResult.ImpactPoint - OriginalNodeLoc);
			DirToValid.Z = 0;
			DirToValid.Normalize();
			int OffsetMult = 0;
			bool TargetReachable = false;

			const auto a = MainBuildingLoc;
			const auto b = OriginalNodeLoc;
			const auto c = HitResult.ImpactPoint;
			const auto A = (c.X - b.X) * (c.X - b.X) + (c.Y - b.Y) * (c.Y - b.Y);
			const auto B = 2 * ((b.X - a.X) * (c.X - b.X) + (b.Y - a.Y) * (c.Y - b.Y));
			const auto C = (b.X - a.X) * (b.X - a.X) + (b.Y - a.Y) * (b.Y - a.Y) - MaxDistance * MaxDistance;
			const auto T = (-B + FMath::Sqrt(B * B - 4 * A * C)) / (2 * A);
			const auto T2 = (-B - FMath::Sqrt(B * B - 4 * A * C)) / (2 * A);

			auto PointX = b.X + T * (c.X - b.X);
			auto PointY = b.Y + T * (c.Y - b.Y);
			const auto Point1 = FVector(PointX, PointY, 0);
			//DrawDebugSphere(GetWorld(), Point1, 500, 12, FColor::Yellow, false, 1, 0, 10);
			PointX = b.X + T2 * (c.X - b.X);
			PointY = b.Y + T2 * (c.Y - b.Y);
			const auto Point2 = FVector(PointX, PointY, 0);
			const auto Dot1 = FVector::DotProduct(Point1 - b, DirToValid);
			const auto Point = FMath::Abs(Dot1-1) <= 0.1 ? Point2 : Point1;
			Target = Point;
			
			while (!TargetReachable && OffsetMult < 50)
			{
				Target -= (DirToValid * OffsetMult * 300);
				Target.Z = 0;
				TargetReachable = NavSys->ProjectPointToNavigation(Target, OutNavLoc, FVector(10, 10, 100));
				//DrawDebugSphere(GetWorld(), Target, 500, 12, FColor::Purple, false, 1, 0, 10);
				OffsetMult++;
			}
		}
		NodeNewLocation = Target;

		
		ValidPathfinding = GetPathfindingFrom(FluxNode, OriginalNodeLoc, Target, BeforeNodePathStructs, MousePosValid, HitResult.ImpactPoint, OriginLoc, MaxDistance);
		if (!ValidPathfinding)
        {
            if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("BeforeNodePathStructs is invalid"));
			if (Release) MoveNodeSaved(FluxNode, Flux);
			return;
        }
	}
	else
	{
		auto StartLocation = Flux->GetFluxNodes()[PreviousIndex]->GetActorLocation();
		auto Target = GetTargetPosition(HitResult, StartLocation, FluxNode, UsingOriginal);
		// DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 500, 12, FColor::Blue, false, 1, 0, 10);
		// DrawDebugSphere(GetWorld(), Target, 500, 12, FColor::Green, false, 1, 0, 10);
		// DrawDebugLine(GetWorld(), HitResult.ImpactPoint, Target, FColor::Green, false, 1, 0, 10);
		if (!MousePosValid)
		{
			auto DirToValid = (HitResult.ImpactPoint - StartLocation);
			DirToValid.Z = 0;
			DirToValid.Normalize();
			//DrawDebugLine(GetWorld(), StartLocation, StartLocation+100000*DirToValid, FColor::Purple, false, 1, 0, 10);
			int OffsetMult = 1;
			bool TargetReachable = false;

			const auto a = MainBuildingLoc;
			const auto b = StartLocation;
			const auto c = HitResult.ImpactPoint;
			const auto A = (c.X - b.X) * (c.X - b.X) + (c.Y - b.Y) * (c.Y - b.Y);
			const auto B = 2 * ((b.X - a.X) * (c.X - b.X) + (b.Y - a.Y) * (c.Y - b.Y));
			const auto C = (b.X - a.X) * (b.X - a.X) + (b.Y - a.Y) * (b.Y - a.Y) - MaxDistance * MaxDistance;
			const auto T = (-B + FMath::Sqrt(B * B - 4 * A * C)) / (2 * A);
			const auto T2 = (-B - FMath::Sqrt(B * B - 4 * A * C)) / (2 * A);

			auto PointX = b.X + T * (c.X - b.X);
			auto PointY = b.Y + T * (c.Y - b.Y);
			const auto Point1 = FVector(PointX, PointY, 0);
			//DrawDebugSphere(GetWorld(), Point1, 500, 12, FColor::Yellow, false, 1, 0, 10);
			PointX = b.X + T2 * (c.X - b.X);
			PointY = b.Y + T2 * (c.Y - b.Y);
			const auto Point2 = FVector(PointX, PointY, 0);
			const auto Dot1 = FVector::DotProduct(Point1 - b, DirToValid);
			const auto Point = FMath::Abs(Dot1-1) <= 0.1 ? Point2 : Point1;
			Target = Point;
			
			while (!TargetReachable && OffsetMult < 50)
			{
				Target -= (DirToValid * OffsetMult * 300);
				Target.Z = 0;
				TargetReachable = NavSys->ProjectPointToNavigation(Target, OutNavLoc, FVector(10, 10, 100));
				//DrawDebugSphere(GetWorld(), Target, 500, 12, FColor::Purple, false, 1, 0, 10);
				OffsetMult++;
			}
		}
		NodeNewLocation = Target;

		/* Pathfinding Previous to NewNode */
		ValidPathfinding = GetPathfindingFrom(FluxNode, StartLocation, Target, BeforeNodePathStructs, MousePosValid, HitResult.ImpactPoint, OriginLoc, MaxDistance);
		if (!ValidPathfinding)
        {
            if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("BeforeNodePathStructs is invalid"));
			if (Release) MoveNodeSaved(FluxNode, Flux);
			return;
        }
	}
	
	
	/* Pathfinding NewNode to End */
	auto StartLocation = Flux->GetFluxNodes()[PreviousIndex]->GetActorLocation();
	auto Target = GetTargetPosition(HitResult, StartLocation, FluxNode, UsingOriginal);
	StartLocation = Target;
    
    /* End location */
    auto NextNodeIndex = Flux->GetNextNodeIndex(FluxNodeIndex);
    if (NextNodeIndex > -1)
    {
    	Target = Flux->GetFluxNodes()[NextNodeIndex]->GetActorLocation();
    	bool TargetReachable = NavSys->ProjectPointToNavigation(Target, OutNavLoc, FVector(500, 500, 100));
    	if (!TargetReachable)
    	{
    		if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Target not reachable"));
    		if (Release) MoveNodeSaved(FluxNode, Flux);
    		return;
    	}
    	Target = OutNavLoc.Location;

    	ValidPathfinding = GetPathfindingFrom(FluxNode, StartLocation, Target, AfterNodePathStructs, MousePosValid, HitResult.ImpactPoint, OriginLoc, MaxDistance);
    	if (!ValidPathfinding)
		{
			if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("AfterNodePathStructs is invalid"));
    		if (Release) MoveNodeSaved(FluxNode, Flux);
    		return;
		}
    	
    	if (AfterNodePathStructs.Num() == 0)
    	{
    		if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("AfterNodePathStructs is empty"));
    		if (Release) MoveNodeSaved(FluxNode, Flux);
    		return;
    	}
    }

	if (BeforeToOriginalNodePathStructs.Num() > 0) BeforeToOriginalNodePathStructs.RemoveAt(0);
	if (BeforeToOriginalNodePathStructs.Num() > 0) BeforeToOriginalNodePathStructs.RemoveAt(BeforeToOriginalNodePathStructs.Num() - 1);

	if (BeforeNodePathStructs.Num() > 0) BeforeNodePathStructs.RemoveAt(0);
	if (BeforeNodePathStructs.Num() > 0)
	{
		NodeNewLocation = BeforeNodePathStructs[BeforeNodePathStructs.Num() - 1].PathPoint;
		BeforeNodePathStructs.RemoveAt(BeforeNodePathStructs.Num() - 1);
	}

	if (AfterNodePathStructs.Num() > 0) AfterNodePathStructs.RemoveAt(0);
	if (AfterNodePathStructs.Num() > 0) AfterNodePathStructs.RemoveAt(AfterNodePathStructs.Num() - 1);

	MoveTargetSaved = NodeNewLocation;
	BeforePathSaved = TArray<FPathStruct>(BeforeNodePathStructs);
	AfterPathSaved = TArray<FPathStruct>(AfterNodePathStructs);
	OriginalToNodePathSaved = TArray<FPathStruct>(BeforeToOriginalNodePathStructs);
	UseOriginalSaved = UsingOriginal;
	bWasEverValidSaved = true;
	
    if (Release)
    {
    	if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Release reached the end"));
    	const auto FluxNodeOtherMovingRef = FluxNodeOtherMoving.IsValid() ? FluxNodeOtherMoving.Get() : nullptr;
    	Flux->MoveNode(FluxNode, NodeNewLocation, BeforeNodePathStructs, AfterNodePathStructs, BeforeToOriginalNodePathStructs, FluxNodeOtherMovingRef, UsingOriginal);
    	
    	MoveNodeCanUseOriginalNode = false;
    	FluxNodeOtherMoving = nullptr;
    	SetMoveSelectedNodeCanUseOtherServer(false);
		FluxNode->OnDroppedTriggerVFX();
    	return;
    }

	const auto FluxNodeOtherMovingRef = FluxNodeOtherMoving.IsValid() ? FluxNodeOtherMoving.Get() : nullptr;
    Flux->MoveNodePreview(FluxNode, NodeNewLocation, BeforeNodePathStructs, AfterNodePathStructs, BeforeToOriginalNodePathStructs, FluxNodeOtherMovingRef, UsingOriginal);
	
	TArray<float> FluxPower = Flux->IsFluxActive() ? Flux->GetFluxPower() : Cast<AMainBuilding>(Flux->GetOrigin())->GetActivatingFluxPower();

	if (!bShouldCalulateThisFrame) return;

	bShouldCalulateThisFrame = false;
	ShouldCalulateThisFrameTimer = 0.f;
	CalculateFluxTargets(FluxNode, Flux, FluxPower, Release);
}

void UFluxComponent::CreateNewFluxes()
{
	if (PlayerControllerInfernale->IsShiftPressed())
	{
		if (!Fluxes.IsEmpty())
		{
			ContinueFlux();
			return;
		}
		
		for (const auto Origin : Origins)
		{
			CreateFlux(Origin);
		}
		return;
	}
	
	for (const auto Origin : Origins)
	{
		CreateFlux(Origin);
	}
	ClearFluxesServer();
	ClearBuildingSelection();
}

bool UFluxComponent::CreateHitResultFlux(FHitResult& HitResult)
{
	const auto Pawn = PlayerControllerInfernale->GetInfernalePawn();
	if (Pawn == nullptr) return false;
	
	const auto CameraComponent = Pawn->GetCameraComponent();
	if (CameraComponent == nullptr) return false;

	const auto Start = CameraComponent->GetComponentLocation();

	FVector MouseWorldLocation;
	FVector MouseWorldDirection;

	if (auto Success = PlayerControllerInfernale->DeprojectMousePositionToWorld(MouseWorldLocation, MouseWorldDirection); !Success) return false;

	const auto Direction = MouseWorldDirection.GetSafeNormal();
	const auto End = Start + Direction * UFunctionLibraryInfernale::GetGameSettingsDataAsset()->RayCastLength;

	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility);
	// DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 0.1, 0, .1);
	//auto Actor = HitResult.GetActor();
	//if (Actor) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Hit Actor on visibility: %s"), *HitResult.GetActor()->GetName()));
	return true;
}

bool UFluxComponent::CreateHitResultFloor(FHitResult& HitResult)
{
	const auto Pawn = PlayerControllerInfernale->GetInfernalePawn();
	if (Pawn == nullptr) return false;
	
	const auto CameraComponent = Pawn->GetCameraComponent();
	if (CameraComponent == nullptr) return false;

	const auto Start = CameraComponent->GetComponentLocation();

	FVector MouseWorldLocation;
	FVector MouseWorldDirection;

	if (auto Success = PlayerControllerInfernale->DeprojectMousePositionToWorld(MouseWorldLocation, MouseWorldDirection); !Success) return false;

	const auto Direction = MouseWorldDirection.GetSafeNormal();
	const auto End = Start + Direction * UFunctionLibraryInfernale::GetGameSettingsDataAsset()->RayCastLength;

	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Camera);
	// DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 0.1, 0, .1);
	// auto Actor = HitResult.GetActor();
	// if (Actor) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Hit Actor on Camera: %s"), *HitResult.GetActor()->GetName()));
	return true;
}

bool UFluxComponent::TryToSelectBuilding(FHitResult& HitResult)
{
	const auto ActorHit = HitResult.GetActor();
	if (ActorHit == nullptr) return false;

	auto Building = Cast<ABuildingParent>(ActorHit);
	if (Building == nullptr) return false;
	if (!Building->CanCreateAFlux()) return false;

	if (bMultiSelectionEnabled) {
		if (PlayerControllerInfernale->IsCtrlPressed())
		{
			AddBuildingToSelection(TWeakObjectPtr<ABuildingParent>(Building));
			return true;
		}
	}
	ClearBuildingSelection();
	AddBuildingToSelection(TWeakObjectPtr<ABuildingParent>(Building));
	return true;
}

bool UFluxComponent::TryToSelectFluxNode(FHitResult& HitResult)
{
	const auto HitActor = HitResult.GetActor();
	if (HitActor == nullptr) return false;

	const auto FluxNode = Cast<AFluxNode>(HitActor);
	if (FluxNode == nullptr) return false;

	if (FluxNode->GetNodeType() == EFluxNodeType::StartNode) return false;
	const auto Flux = FluxNode->GetFlux();
	if (Flux == nullptr) return false;
	if (Flux->GetOwnerInfo().Player != PlayerControllerInfernale->GetPlayerOwning()) return false;

	//if (FluxNode->GetNodeType() == EFluxNodeType::StartNode) return false;

	if (bMultiSelectionEnabled)
	{
		if (PlayerControllerInfernale->IsCtrlPressed())
		{
			AddNodeToSelection(FluxNode);
			return true;
		}
	}
	ClearBuildingSelection();
	AddNodeToSelection(FluxNode);
	return true;
}

bool UFluxComponent::TryToSelectFlux(FHitResult& HitResult)
{
	const auto HitActor = HitResult.GetActor();
	if (HitActor == nullptr) return false;

	const auto Flux = Cast<AFlux>(HitActor);
	if (Flux == nullptr) return false;
	const auto FluxPlayer = Flux->GetOrigin()->GetOwner().Player;
	if (FluxPlayer != PlayerControllerInfernale->GetPlayerOwning()) return false;

	FluxInteracted = TWeakObjectPtr<AFlux>(Flux);
	return true;
}

bool UFluxComponent::TryToHoverFlux(FHitResult& HitResult)
{
	const auto HitActor = HitResult.GetActor();
	if (HitActor == nullptr) return false;

	const auto Flux = Cast<AFlux>(HitActor);
	if (Flux == nullptr) return false;

	if (FluxHovered == Flux) return true;
	if (FluxHovered) FluxHovered->InteractEndHoverFlux(PlayerControllerInfernale);

	const auto FluxPlayer = Flux->GetOrigin()->GetOwner().Player;
	if (FluxPlayer != PlayerControllerInfernale->GetPlayerOwning())return false;

	FluxHovered = Flux;
	FluxHovered->InteractStartHoverFlux(PlayerControllerInfernale);
	return true;
}

bool UFluxComponent::TryToHoverFluxNode(FHitResult& HitResult)
{
	const auto HitActor = HitResult.GetActor();
	if (HitActor == nullptr) return false;

	const auto FluxNode = Cast<AFluxNode>(HitActor);
	if (FluxNode == nullptr) return false;

	if (FluxNodeHovered == FluxNode) return true;
	if (FluxNodeHovered) FluxNodeHovered->InteractEndHoverFlux(PlayerControllerInfernale);

	const auto Flux = FluxNode->GetFlux();
	if (Flux == nullptr) return false;
	const auto FluxOrigin = Flux->GetOrigin();
	if (FluxOrigin == nullptr) return false;
	
	const auto FluxPlayer = FluxOrigin->GetOwner().Player;
	if (FluxPlayer != PlayerControllerInfernale->GetPlayerOwning()) return false;

	FluxNodeHovered = FluxNode;
	if (PlayerControllerInfernale->bIsEscapeMenuOpen) return false;
	FluxNodeHovered->InteractStartHoverFlux(PlayerControllerInfernale);
	return true;
}

void UFluxComponent::ClearHoverFlux()
{
	if (FluxHovered) FluxHovered->InteractEndHoverFlux(PlayerControllerInfernale);
	FluxHovered = nullptr;
}

void UFluxComponent::ClearHoverFluxNode()
{
	if (FluxNodeHovered) FluxNodeHovered->InteractEndHoverFlux(PlayerControllerInfernale);
	FluxNodeHovered = nullptr;
}

bool UFluxComponent::TryToFindFluxToRemove(FHitResult& HitResult, bool bSelectFlux)
{
	const auto Pawn = PlayerControllerInfernale->GetInfernalePawn();
	if (Pawn == nullptr) return false;
	
	const auto CameraComponent = Pawn->GetCameraComponent();
	if (CameraComponent == nullptr) return false;

	const auto Start = CameraComponent->GetComponentLocation();

	FVector MouseWorldLocation;
	FVector MouseWorldDirection;

	if (auto Success = PlayerControllerInfernale->DeprojectMousePositionToWorld(MouseWorldLocation, MouseWorldDirection); !Success) return false;

	const auto Direction = MouseWorldDirection.GetSafeNormal();
	const auto End = Start + Direction * UFunctionLibraryInfernale::GetGameSettingsDataAsset()->RayCastLength;

	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility);
	//DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 5, 0, 1);
	
	const auto ActorHit = HitResult.GetActor();
	if (ActorHit == nullptr) return false;

	if (auto FluxNode = Cast<AFluxNode>(ActorHit))
	{
		const auto FluxPlayer = FluxNode->GetFlux()->GetOrigin()->GetOwner().Player;
		if (FluxPlayer != PlayerControllerInfernale->GetPlayerOwning()) return false;

		if (bSelectFlux)
		{
			FluxNodeToRemove = TWeakObjectPtr<AFluxNode>(FluxNode);
			return true;
		}
		
		return FluxToRemove.Get() == FluxNode->GetFlux();
	}

	if (auto Flux = Cast<AFlux>(ActorHit))
	{
		const auto FluxPlayer = Flux->GetOrigin()->GetOwner().Player;
		if (FluxPlayer != PlayerControllerInfernale->GetPlayerOwning()) return false;
		if (bSelectFlux) FluxToRemove = TWeakObjectPtr<AFlux>(Flux);
		return FluxToRemove.Get() == Flux;
	}

	return false;
}

bool UFluxComponent::TryToFindFluxNodeToRemove(FHitResult& HitResult, bool bSelectFluxNode)
{
	const auto Pawn = PlayerControllerInfernale->GetInfernalePawn();
	if (Pawn == nullptr) return false;
	
	const auto CameraComponent = Pawn->GetCameraComponent();
	if (CameraComponent == nullptr) return false;

	const auto Start = CameraComponent->GetComponentLocation();

	FVector MouseWorldLocation;
	FVector MouseWorldDirection;

	if (auto Success = PlayerControllerInfernale->DeprojectMousePositionToWorld(MouseWorldLocation, MouseWorldDirection); !Success) return false;

	const auto Direction = MouseWorldDirection.GetSafeNormal();
	const auto End = Start + Direction * UFunctionLibraryInfernale::GetGameSettingsDataAsset()->RayCastLength;

	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility);
	//DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 5, 0, 1);
	
	const auto ActorHit = HitResult.GetActor();
	if (ActorHit == nullptr) return false;

	if (auto FluxNode = Cast<AFluxNode>(ActorHit))
	{
		if (bSelectFluxNode) FluxNodeToRemove = TWeakObjectPtr<AFluxNode>(FluxNode);
		return FluxNodeToRemove.Get() == FluxNode;
	}

	return false;
}

FVector UFluxComponent::GetTargetPosition(FHitResult& HitResult, FVector PreviousLocation)
{
	auto UnitActorManager = GameModeInfernale->GetUnitActorManager();
	auto Repulsors = UnitActorManager->GetRepulsorsInRange(HitResult.ImpactPoint, 1500);
	if (Repulsors.Num() == 0) return HitResult.ImpactPoint;

	auto Target = HitResult.ImpactPoint;
	auto TargetToPreviousDirection = (PreviousLocation - Target).GetSafeNormal();
	int Debugcount = 0;
	auto TargetIsValid = false;

	TArray<AActor*> RepulsorsAsActors;
	for (const auto Repulsor : Repulsors)
    {
        RepulsorsAsActors.Add(Cast<AActor>(Repulsor));
    }

	
	while (Debugcount < 100 && !TargetIsValid)
	{
		Debugcount++;
		
		float MaxRepulsion = 0;
		//IFluxRepulsor* RepulsorToUse;
		
		for (int i = 0; i < Repulsors.Num() ; i++)
		{
			auto Repulsor = Repulsors[i];
			//TODO: Add TWeakObjectPtr just in case
			if (!Repulsor) continue;
            auto ActorRepulsor = RepulsorsAsActors[i];
			if (!ActorRepulsor) continue;
            auto DistanceToRepulsor = FVector::Distance(ActorRepulsor->GetActorLocation(), Target);
            auto RepulsorStrength = Repulsor->GetRepulsorRange();
            auto WouldRepulseBy = RepulsorStrength - DistanceToRepulsor;
            if (WouldRepulseBy <= MaxRepulsion) continue;
            MaxRepulsion = WouldRepulseBy;
            //RepulsorToUse = Repulsor;
		}

		if (MaxRepulsion == 0) return Target;

		Target += TargetToPreviousDirection * 50; // Sould be a variable
	}

	return Target;
}

FVector UFluxComponent::GetTargetPosition(FHitResult& HitResult, FVector PreviousLocation, AFluxNode* FluxNode, bool UseOriginal)
{
	auto Target = GetTargetPosition(HitResult, PreviousLocation);
	const auto Flux = FluxNode->GetFlux();
	auto FluxNodes = Flux->GetFluxNodes();
	FVector TooCloseNodeLoc;
	bool TooClose = false;

	auto FluxNodeLoc = Target;
	FluxNodeLoc.Z = 0;

	if (UseOriginal) if (FluxNodeOtherMoving.IsValid()) FluxNodes.Add(FluxNodeOtherMoving.Get());
	
	for (const auto Node : FluxNodes)
	{
		if (!Node) continue;
		const auto Type = Node->GetNodeType();
		if (Type == EFluxNodeType::StartNode) continue;
		if (Type == EFluxNodeType::FakeNode) continue;
		if (!Node->IsReal() && Type != EFluxNodeType::TmpNode) continue;
		if (!UseOriginal && Type == EFluxNodeType::TmpNode) continue;
		
		if (Node == FluxNode) continue;
		
		auto NodeLoc = Node->GetActorLocation();
		NodeLoc.Z = 0;

		const auto Dist = FVector::Distance(NodeLoc, FluxNodeLoc);
		if (Dist > MinRangeBetweenNodes) continue;
		TooClose = true;
		TooCloseNodeLoc = NodeLoc;
		break;
	}

	if (!TooClose) return Target;
	const auto Direction = (TooCloseNodeLoc - Target).GetSafeNormal();
	if (bDebugPathfinding)
	{
		DrawDebugLine(GetWorld(), Target, Target + Direction * 1000, FColor::Red, false, 0.f, 0, 10);
		DrawDebugSphere(GetWorld(), TooCloseNodeLoc, MinRangeBetweenNodes, 12, FColor::Black, false, 0.f, 0, 10);
	}
	return TooCloseNodeLoc - Direction * MinRangeBetweenNodes;
}

bool UFluxComponent::IsRemovingMode()
{
	if (FluxModeState == EFluxModeState::FMSRemoveFluxNode) return true;
	if (FluxModeState == EFluxModeState::FMSRemoveFlux) return true;
	return false;
}

bool UFluxComponent::IsMovingNode()
{
	return FluxModeState == EFluxModeState::FMSMoveFluxNode || FluxModeState == EFluxModeState::FMSMoveFluxNodeCnC;
}

void UFluxComponent::FluxNodeCreatedCheck()
{
	if (bWaitingForCreation || bWaitingForHoldType) return;
	AddEventsMoveSelectedFluxNode(false, bTryingToHoldPrimaryAction);
	bTryingToHoldPrimaryAction = false;
}

void UFluxComponent::AddEventsMoveSelectedFluxNode(const bool AllowToCreatePrevious, const bool DragNDrop)
{
	if (DragNDrop)
	{
		PlayerControllerInfernale->MousePrimaryTriggered.AddDynamic(this, &UFluxComponent::MoveSelectedFluxNodeNoRelease);
		PlayerControllerInfernale->MousePrimaryEnd.AddDynamic(this, &UFluxComponent::ReleaseSelectedFluxNode);
	}
	else
	{
		PlayerControllerInfernale->MousePrimaryStart.AddDynamic(this, &UFluxComponent::ReleaseSelectedFluxNode);
	}
	FluxNodesCreateWhileMoving = TArray<TWeakObjectPtr<AFluxNode>>();
	ClearFluxNodesCreateWhileMovingServer();

	
	FluxModeState = DragNDrop ? EFluxModeState::FMSMoveFluxNode : EFluxModeState::FMSMoveFluxNodeCnC;
	FluxNodeOtherMoving = nullptr;
	
	if (!FluxNodeInteracted.IsValid() || !AllowToCreatePrevious)
	{
		MoveNodeCanUseOriginalNode = false;
		SetMoveSelectedNodeCanUseOtherServer(false);
		return;
	}
	MoveNodeCanUseOriginalNode = true;

	if (bDebugMoveNode) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("NodeMoved: %s"), *FluxNodeInteracted->GetName() ));
	
	if (PlayerControllerInfernale->IsShiftPressed())
    {
		SpawnNodeRightBeforeServer(FluxNodeInteracted.Get(), false, true);
		UseOriginalNode(true);
		SetMoveSelectedNodeCanUseOtherServer(true);
        return;
    }

	const auto NodeIndex = FluxNodeInteracted->GetNodeIndex();
	MoveNodeCanUseOriginalNode = NodeIndex != 0;
	MoveNodeCanUseOriginalNode = MoveNodeCanUseOriginalNode && FluxNodeInteracted->GetNodeType() != EFluxNodeType::StartNode;
	SetMoveSelectedNodeCanUseOtherServer(MoveNodeCanUseOriginalNode);
	if (!MoveNodeCanUseOriginalNode) return;
	
	SpawnNodeRightBeforeServer(FluxNodeInteracted.Get(), false, true);
	UseOriginalNode(false);
}

void UFluxComponent::RemoveEventsMoveSelectedFluxNode()
{
	if (FluxModeState == EFluxModeState::FMSMoveFluxNode)
	{
		PlayerControllerInfernale->MousePrimaryTriggered.RemoveDynamic(this, &UFluxComponent::MoveSelectedFluxNodeNoRelease);
		PlayerControllerInfernale->MousePrimaryEnd.RemoveDynamic(this, &UFluxComponent::ReleaseSelectedFluxNode);
	}
	else if (FluxModeState == EFluxModeState::FMSMoveFluxNodeCnC)
	{
		PlayerControllerInfernale->MousePrimaryStart.RemoveDynamic(this, &UFluxComponent::ReleaseSelectedFluxNode);
	}
	FluxModeState = EFluxModeState::FMSNone;
}

void UFluxComponent::AddEventsNodeRemoval()
{
}

void UFluxComponent::RemoveEventsNodeRemoval()
{
}

void UFluxComponent::AddEventsFluxRemoval()
{
}

void UFluxComponent::RemoveEventsFluxRemoval()
{
}

void UFluxComponent::AddBuildingToSelection(TWeakObjectPtr<ABuildingParent> Building)
{
	if (Building->GetOwner().Player != PlayerControllerInfernale->GetPlayerOwning()) return;
	
	ClearNodeSelection();
	auto BuildingRef = Building.Get();
	Building->Execute_OnSelectedForFlux(BuildingRef);
	SelectBuildingServer(BuildingRef);
	Building->BuildingParentDestroyed.AddDynamic(this, &UFluxComponent::RemoveBuildingFromSelection);
	Origins.Add(BuildingRef);
}

void UFluxComponent::RemoveBuildingFromSelection(ABuildingParent* Building)
{
	Building->Execute_OnDeselectedForFlux(Building);
	DeselectBuildingServer(Building);
	Building->BuildingParentDestroyed.RemoveDynamic(this, &UFluxComponent::RemoveBuildingFromSelection);
	Origins.Remove(Building);
}

void UFluxComponent::ClearBuildingSelection()
{
	for (const auto Building : Origins)
	{
		if (!Building) continue;
		Building->Execute_OnDeselectedForFlux(Building);
		DeselectBuildingServer(Building);
		Building->BuildingParentDestroyed.RemoveDynamic(this, &UFluxComponent::RemoveBuildingFromSelection);
	}
	Origins.Empty();
	ClearFluxesServer();
}

void UFluxComponent::AddNodeToSelection(AFluxNode* FluxNode)
{
	if (!FluxNode) return;
	if (FluxNode == FluxNodeInteracted.Get()) return;
	
	auto Building = FluxNode->GetFlux()->GetOrigin();
	if (Building->GetOwner().Player != PlayerControllerInfernale->GetPlayerOwning()) return;
	
	ClearBuildingSelection();
	FluxNode->Execute_OnSelectedForFlux(FluxNode);
	SelectFluxNodeServer(FluxNode);

	const auto Flux = FluxNode->GetFlux();
	if (Flux)
	{
		const auto Origin = Flux->GetOrigin();
		const auto MainBuilding = Cast<AMainBuilding>(Origin);
		if (MainBuilding)
		{
			PlayerControllerInfernale->MainBuildingInteracted(MainBuilding);
		}
	}
	
	FluxNode->FluxNodeDestroyed.AddDynamic(this, &UFluxComponent::RemoveNodeFromSelection);
	if (FluxNodeInteracted.IsValid()) ClearNodeSelection();
	FluxNodeInteracted = TWeakObjectPtr<AFluxNode>(FluxNode);
	if (bDebugMoveNode) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("NodeSelected: %s"), *FluxNode->GetName() ));
}

void UFluxComponent::RemoveNodeFromSelection(AFluxNode* FluxNode)
{
	if (!FluxNode) return;
	TWeakObjectPtr<AFluxNode> FluxNodePtr = TWeakObjectPtr<AFluxNode>(FluxNode);
	if (!FluxNodeInteracted.IsValid()) return;
	
	if (FluxNode != FluxNodeInteracted.Get()) return;
	
	FluxNode->Execute_OnDeselectedForFlux(FluxNode);
	DeselectFluxNodeServer(FluxNode);
	FluxNode->FluxNodeDestroyed.RemoveDynamic(this, &UFluxComponent::RemoveNodeFromSelection);
	FluxNodeInteracted = nullptr;
}

void UFluxComponent::ClearNodeSelection()
{
	// for (const auto FluxNode : FluxNodes)
	// {
	// 	if (!FluxNode) continue;
	// 	FluxNode->Execute_OnDeselectedForFlux(FluxNode);
	// 	DeselectFluxNodeServer(FluxNode);
	// 	FluxNode->FluxNodeDestroyed.RemoveDynamic(this, &UFluxComponent::RemoveNodeFromSelection);
	// }
	// FluxNodes.Empty();

	if (!FluxNodeInteracted.IsValid()) return;
	auto FluxNodeInteractedRef = FluxNodeInteracted.Get();
	FluxNodeInteracted->Execute_OnDeselectedForFlux(FluxNodeInteractedRef);
	DeselectFluxNodeServer(FluxNodeInteractedRef);
	FluxNodeInteractedRef->FluxNodeDestroyed.RemoveDynamic(this, &UFluxComponent::RemoveNodeFromSelection);
	FluxNodeInteracted = nullptr;

	TArray<AActor*> Targetables = TArray<AActor*>();
	UGameplayStatics::GetAllActorsWithInterface(GetWorld(), UUnitTargetable::StaticClass(), Targetables);
	for (const auto& Unit : Targetables)
	{
		IUnitTargetable* Targetable = Cast<IUnitTargetable>(Unit);
		Targetable->Execute_ToggleDetectionDecal(Unit, true);
	}
}

void UFluxComponent::CreateFlux(ABuildingParent* Building, bool ShouldDisplay)
{
	const auto Pawn = PlayerControllerInfernale->GetInfernalePawn();
	if (Pawn == nullptr) return;
	
	const auto CameraComponent = Pawn->GetCameraComponent();
	if (CameraComponent == nullptr) return;

	const auto Start = CameraComponent->GetComponentLocation();

	FVector MouseWorldLocation;
	FVector MouseWorldDirection;

	if (auto Success = PlayerControllerInfernale->DeprojectMousePositionToWorld(MouseWorldLocation, MouseWorldDirection); !Success) return;

	const auto Direction = MouseWorldDirection.GetSafeNormal();
	const auto End = Start + Direction * UFunctionLibraryInfernale::GetGameSettingsDataAsset()->RayCastLength;
	bWaitingForFluxReplication = true;
	FluxToRemove = nullptr;

	CreateFluxServer(Building, Start, End, ShouldDisplay);
}

void UFluxComponent::CreateFlux(ABuildingParent* Origin, TArray<FPathStruct> Path, bool ShouldDisplay)
{
	if (Path.Num() == 0)
	{
		if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Path is empty so CreateFlux won't execute"));
		return;
	}
	
	UClass* ClassType = FluxSpawnClass->GetDefaultObject()->GetClass();
	//Target.Z = 0;
	
	FActorSpawnParameters SpawnParams;
	const auto Transform = FTransform(FRotator::ZeroRotator, FVector::Zero(), FVector(1, 1, 1));
	AActor* Actor = GetWorld()->SpawnActor(ClassType, &Transform, SpawnParams);
	const auto Flux = Cast<AFlux>(Actor);
	if (Flux == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Flux just created is null"));
		return;
	}
	
	Flux->Init(Origin, Path, ShouldDisplay);
	Fluxes.Add(Flux);
	const auto WeakPtr = TWeakObjectPtr<AFlux>(Flux);
	Origin->AddFlux(WeakPtr);
	ReplicateFluxesMulticast(Fluxes);
	FluxToRemove = WeakPtr;
	ReplicateFluxCreatedOwner(Flux);
}

void UFluxComponent::ReplicateFluxCreatedOwner_Implementation(AFlux* Flux)
{
	FluxToRemove = TWeakObjectPtr<AFlux>(Flux);
	bWaitingForFluxReplication = false;
}

void UFluxComponent::ContinueFluxServer_Implementation(FVector Start, FVector End)
{
	FHitResult HitResult;
	
	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility);
	//DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 5, 0, 1);
	
	const auto ActorHit = HitResult.GetActor();
	if (ActorHit != nullptr)
	{
		if (const auto FluxTarget = Cast<IFluxTarget>(ActorHit))
		{
			for (auto Flux : Fluxes)
			{
				ContinueFromFlux(Flux, ActorHit->GetActorLocation(), FluxTarget->GetAttackOffsetRange());
			}
			return;
		}
	}
	for (auto Flux : Fluxes)
	{
		ContinueFromFlux(Flux, HitResult.ImpactPoint, 0);
	}
}

void UFluxComponent::ContinueFlux()
{
	const auto Pawn = PlayerControllerInfernale->GetInfernalePawn();
	if (Pawn == nullptr) return;
	
	const auto CameraComponent = Pawn->GetCameraComponent();
	if (CameraComponent == nullptr) return;

	const auto Start = CameraComponent->GetComponentLocation();

	FVector MouseWorldLocation;
	FVector MouseWorldDirection;

	if (auto Success = PlayerControllerInfernale->DeprojectMousePositionToWorld(MouseWorldLocation, MouseWorldDirection); !Success) return;

	const auto Direction = MouseWorldDirection.GetSafeNormal();
	const auto End = Start + Direction * UFunctionLibraryInfernale::GetGameSettingsDataAsset()->RayCastLength;

	ContinueFluxServer(Start, End);
}

void UFluxComponent::CreateFluxOld(ABuildingParent* Building, FVector Target, float Offset)
{
	if (!Building) return;
	UClass* ClassType = FluxSpawnClass->GetDefaultObject()->GetClass();
	Target.Z = 0;
	
	//const auto SpawnParams = FActorSpawnParameters();
	FActorSpawnParameters SpawnParams;
	const auto Transform = FTransform(FRotator::ZeroRotator, FVector::Zero(), FVector(1, 1, 1));
	
	AActor* Actor = GetWorld()->SpawnActor(ClassType, &Transform, SpawnParams);
	const auto Flux = Cast<AFlux>(Actor);
	if (Flux == nullptr) return;
	
	Flux->Init(Building, Target, Offset);
	Fluxes.Add(Flux);
	const auto WeakPtr = TWeakObjectPtr<AFlux>(Flux);
	Building->AddFlux(WeakPtr);
	ReplicateFluxesMulticast(Fluxes);
	FluxToRemove = WeakPtr;
	ReplicateFluxCreatedOwner(Flux);
}

void UFluxComponent::ContinueFromFlux(AFlux* Flux, FVector Target, float Offset)
{
	const auto Origin = Flux->GetOrigin();
	const auto OriginLoc = Origin->GetActorLocation();
	const auto Distance = FVector::Distance(OriginLoc, Target);
	auto OriginMainBuilding = Cast<AMainBuilding>(Origin);
	if (!OriginMainBuilding) return;
	
	auto MaxDistance = OriginMainBuilding->GetControlAreaRadius();
	MaxDistance = GameModeInfernale->GetRadiusFromGameDuration(MaxDistance);
	MaxDistance = PlayerControllerInfernale->GetTransmutationComponent()->GetEffectFluxRange(MaxDistance);
	const auto MousePosValid = Distance <= MaxDistance;

	/* FluxPathfinding */
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation OutNavLoc;
	TArray<FPathStruct> PathStructs;

	/* Start location */
	auto EndNode = Flux->GetEndNode();
	if (EndNode == nullptr) return;
	auto StartLocation = EndNode->GetActorLocation();

	auto Nodes = Flux->GetFluxNodes();
	auto LastLocation = Nodes[Nodes.Num() - 1]->GetActorLocation();

	/* End location */
	if (Offset > 0)
	{
		const auto Direction = (LastLocation - Target).GetSafeNormal();
		Target += Direction * Offset;
	}
	
	bool TargetReachable = false;
	if (!MousePosValid)
	{
		auto DirToValid = (Target - OriginLoc);
		DirToValid.Z = 0;
		DirToValid.Normalize();
		int OffsetMult = 0;
		while (!TargetReachable && OffsetMult < 50)
		{
			Target = OriginLoc + DirToValid * MaxDistance * 0.98f - (DirToValid * OffsetMult * 300);
			Target.Z = 0;
			TargetReachable = NavSys->ProjectPointToNavigation(Target, OutNavLoc, FVector(10, 10, 100));
			OffsetMult++;
		}
	}

	TargetReachable = NavSys->ProjectPointToNavigation(Target, OutNavLoc, FVector(500, 500, 100));
	if (!TargetReachable)
	{
		if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Target not reachable"));
		return;
	}
	Target = OutNavLoc.Location;

	/* Pathfinding */
	UNavigationPath* Path = NavSys->FindPathToLocationSynchronously(GetWorld(), StartLocation, Target, EndNode.Get());
	if (Path == nullptr)
	{
		if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Path is null"));
		return;
	}
	if (Path->IsPartial())
	{
		if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Path is partial"));
		return;
	}
	
	auto PreviousPoint = StartLocation;
	for (auto Point : Path->PathPoints)
	{
		PathStructs.Add(FPathStruct(Point, false));
		if (bDebugPathfinding)
		{
			DrawDebugSphere(GetWorld(), Point, 500, 12, FColor::Red, false, 5, 0, 10);
			DrawDebugLine(GetWorld(), PreviousPoint, Point, FColor::Red, false, 5, 0, 10);
		}
		PreviousPoint = Point;
	}
	
	if (PathStructs.Num() == 0)
	{
		if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PathStructs is empty"));
		return;
	}
	PathStructs.RemoveAt(0);
	PathStructs[PathStructs.Num() - 1].IsReal = true;
	Flux->Continue(PathStructs);
}

void UFluxComponent::OnFluxNodeCreated(AFlux* Flux, AFluxNode* FluxNode)
{
	/* Called when we create a new node by clicking by dragging the flux */
	if (FluxNodeInteracted.IsValid()) ClearNodeSelection();
	
	FluxNodeInteracted = TWeakObjectPtr<AFluxNode>(FluxNode);
	Flux->FluxNodeCreated.RemoveDynamic(this, &UFluxComponent::OnFluxNodeCreated);
	Flux->FluxNodeCreationFailed.RemoveDynamic(this, &UFluxComponent::OnFluxNodeCreationFailed);
	
	bWaitingForCreation = false;
	FluxNodeCreatedCheck();
}

void UFluxComponent::OnFluxNodeCreationFailed(AFlux* Flux)
{
	Flux->FluxNodeCreated.RemoveDynamic(this, &UFluxComponent::OnFluxNodeCreated);
	Flux->FluxNodeCreationFailed.RemoveDynamic(this, &UFluxComponent::OnFluxNodeCreationFailed);
	bWaitingForCreation = false;
	bTryingToHoldPrimaryAction = false;
	bWaitingForHoldType	= false;
	
	PlayerControllerInfernale->MousePrimaryEnd.RemoveDynamic(this, &UFluxComponent::ClickNClickOnNew);
}

void UFluxComponent::OnFluxNodeCreatedWhileMoving(AFlux* Flux, AFluxNode* FluxNode)
{
	Flux->FluxNodeCreated.RemoveDynamic(this, &UFluxComponent::OnFluxNodeCreatedWhileMoving);
	Flux->FluxNodeCreationFailed.RemoveDynamic(this, &UFluxComponent::OnFluxNodeCreationFailedWhileMoving);
	//TODO
}

void UFluxComponent::OnFluxNodeCreationFailedWhileMoving(AFlux* Flux)
{
	Flux->FluxNodeCreated.RemoveDynamic(this, &UFluxComponent::OnFluxNodeCreatedWhileMoving);
	Flux->FluxNodeCreationFailed.RemoveDynamic(this, &UFluxComponent::OnFluxNodeCreationFailedWhileMoving);
	//TODO
}

void UFluxComponent::SetTargetsOwner_Implementation(const TArray<AActor*>& TargetablesByFlux, const TArray<AActor*>& TargetablesAvailable, AFlux* Flux, const TArray<float>& FluxPowers)
{
	const auto& TransmComponent = PlayerControllerInfernale->GetTransmutationComponent();

	for (const auto& Pt : TargetablesAvailable)
	{
		IUnitTargetable* Targetable = Cast<IUnitTargetable>(Pt);

		bool Detected = TargetablesByFlux.Contains(Pt);
		//Targetable->Execute_ToggleDetectionDecal(Pt, !Detected);

		if (!Detected)
		{
			if (Targetable->bIsDetected)
			{
				Targetable->Execute_SetDetectionDecalColor(Pt, FColor::White);
				Targetable->Execute_DisableDetectionVFX(Pt);
				Targetable->bIsDetected = false;
			}
			continue;
		}

		bool Overpowered = false;
		float FluxPower = 0.f;
		float TargetPower = 0.f;
		float PowerRatio = 0.f;

		EFluxPowerScaling Scaling;

		switch (Targetable->Type)
		{
		case EUnitTargetType::UTargetUnit:

			//No unit detection here
			break;

		case EUnitTargetType::UTargetBuilding:
			FluxPower = TransmComponent->GetEffectDamageToBuilding(FluxPowers[1]);
			TargetPower = Cast<AMainBuilding>(Targetable)->BuildingPower;
			break;
		
		case EUnitTargetType::UTargetNeutralCamp:
			FluxPower = TransmComponent->GetEffectDamageToMonster(FluxPowers[2]);
			TargetPower = Cast<ANeutralCamp>(Targetable)->MonsterPower;
			break;
		
		default:
			FluxPower = 100.f;
			TargetPower = 0.f;
			break;
		}

		if (TargetPower > 0.f)
		{
			Overpowered = FluxPower < TargetPower;
			PowerRatio = FluxPower / TargetPower;

			/* ----- ~ Dogshit uwu ~ ----- */
			if (PowerRatio < ((TargetPower * .5f) / TargetPower)) Scaling = EFluxPowerScaling::VeryLow; // < 50%
			else if (PowerRatio < ((TargetPower * .9f) / TargetPower)) Scaling = EFluxPowerScaling::Low; // 51% <  < 90%
			else if (PowerRatio < ((TargetPower * 1.3f) / TargetPower)) Scaling = EFluxPowerScaling::Average; // 91% <  < 130%
			else Scaling = EFluxPowerScaling::High; // > 131%
		}
		else
		{
			Scaling = EFluxPowerScaling::Average;
		}

		FColor DetectionColor; // = Detected ? (Overpowered ? FColor::Orange : FColor::Green) : FColor::Red;
		if (!Detected) DetectionColor = FColor::White;
		else
		{
			switch (Scaling)
			{
			case VeryLow:
				DetectionColor = FColor::Red;
				break;
			case Low:
				DetectionColor = FColor::Orange;
				break;
			case Average:
				DetectionColor = FColor::Green;
				break;
			case High:
				DetectionColor = FColor::Blue;
				break;
			default:
				break;
			}
		}

		Targetable->Execute_SetDetectionDecalColor(Pt, DetectionColor);

		if (!Targetable->bIsDetected && Detected)
			Targetable->Execute_TriggerDetectionVFX(Pt, Scaling, FluxPower);
		
		if(Targetable->bIsDetected && !Detected)
			Targetable->Execute_DisableDetectionVFX(Pt);

		Targetable->bIsDetected = Detected;
	}
}

void UFluxComponent::ReplicateFluxesMulticast_Implementation(const TArray<AFlux*>& FluxesToReplicate)
{
	Fluxes = FluxesToReplicate;
}

void UFluxComponent::CreateFluxNodeServer_Implementation(AFlux* Flux, FVector Target)
{
	const auto Origin = Flux->GetOrigin();
	auto OriginMainBuilding = Cast<AMainBuilding>(Origin);
	if (!OriginMainBuilding) return;

	auto Created = Flux->CreateFluxNode(Target);
	if (!Created) return;
	
	OriginMainBuilding->OnSelectedForFluxCpp();
}

void UFluxComponent::SpawnNodeRightBeforeServer_Implementation(AFluxNode* FluxNodeToMove, bool RealNode, bool bIsTmp)
{
	auto Flux = FluxNodeToMove->GetFlux();
	if (Flux == nullptr) return;
	FluxNodeOtherMoving = TWeakObjectPtr<AFluxNode>(Flux->CreateNodeBefore(FluxNodeToMove, RealNode, bIsTmp));
	FluxNodeOtherMovingInitialIndex = FluxNodeOtherMoving->GetNodeIndex();
	MoveNodeCanUseOriginalNode = FluxNodeOtherMoving != nullptr;
}

void UFluxComponent::ClearFluxNodesCreateWhileMovingServer_Implementation()
{
	FluxNodesCreateWhileMoving = TArray<TWeakObjectPtr<AFluxNode>>();
}

void UFluxComponent::ClearFluxesServer_Implementation()
{
	Fluxes.Empty();
	ReplicateFluxesMulticast(Fluxes);
}

void UFluxComponent::CreateFluxServer_Implementation(ABuildingParent* Building, FVector Start, FVector End, bool ShouldDisplay)
{
	FHitResult HitResult;
	
	auto Channel = UFunctionLibraryInfernale::GetCustomTraceChannel(ECustomTraceChannel::CameraCollision);
	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, Channel);
	//DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 5, 0, 1);

	if (Building == nullptr) return;
	const auto MainBuilding = Building->GetMainBuilding();
	if (MainBuilding == nullptr) return;

	//Check if we are in range
	auto Distance = FVector::Distance(MainBuilding->GetActorLocation(), HitResult.ImpactPoint);
	auto MaxDistance = MainBuilding->GetControlAreaRadius();
	MaxDistance = GameModeInfernale->GetRadiusFromGameDuration(MaxDistance);
	MaxDistance = PlayerControllerInfernale->GetTransmutationComponent()->GetEffectFluxRange(MaxDistance);
	const auto MousePosValid = Distance <= MaxDistance;
	if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, !MousePosValid ? FColor::Red : FColor::Green, FString::Printf(TEXT("Distance: %f / %f"), Distance, MaxDistance));

	/*if (DistanceNotOK)
	{
		if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Too far"));
		return;
	}*/

	/* FluxPathfinding */
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation OutNavLoc;
	TArray<FPathStruct> PathStructs;

	/* Start location */
	const auto Offset = MainBuilding->GetOffsetRange();
	const auto BaseStartLocation = Building->GetActorLocation();
	auto StartLocation = BaseStartLocation;
	if (Offset > 0)
	{
		const auto Direction = (HitResult.ImpactPoint - StartLocation).GetSafeNormal();
		StartLocation = StartLocation + Direction * Offset;
	}
	

	/* End location */
	auto Target = GetTargetPosition(HitResult, StartLocation);
	auto OriginLoc = Building->GetActorLocation();
	OriginLoc.Z = 0;
	bool TargetReachable = false;

	if (!MousePosValid)
	{
		auto DirToValid = (HitResult.ImpactPoint - OriginLoc);
		DirToValid.Z = 0;
		DirToValid.Normalize();
		int OffsetMult = 0;
		TargetReachable = false;
		
		while (!TargetReachable && OffsetMult < 50)
		{
			Target = OriginLoc + DirToValid * MaxDistance * 0.98f - (DirToValid * OffsetMult * 300);
			Target.Z = 0;
			TargetReachable = NavSys->ProjectPointToNavigation(Target, OutNavLoc, FVector(10, 10, 100));
			OffsetMult++;
		}
	}
	TargetReachable = NavSys->ProjectPointToNavigation(Target, OutNavLoc, FVector(500, 500, 100));
	if (!TargetReachable)
	{
		if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Target not reachable"));
		return;
	}
	Target = OutNavLoc.Location;


	/* Fix origin */
	//TargetReachable = NavSys->ProjectPointToNavigation(StartLocation, OutNavLoc, FVector(300, 300, 100));
	if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, TEXT("StartTarget not reachable, let's search for a new one"));
	if (bDebugPathfinding) DrawDebugSphere(GetWorld(), StartLocation + FVector(0, 0, 1000), 500, 12, FColor::Red, false, 5, 0, 10);
	auto Counter = 0;
	auto Positive = true;
	TargetReachable = false;

	const auto Direction = (HitResult.ImpactPoint - StartLocation).GetSafeNormal();
	auto FullOffset = Direction * Offset;
	
	while (!TargetReachable && Counter < 18)
	{
		auto Angle = Counter * 10;
		if (!Positive) Angle = -Angle;
		StartLocation = BaseStartLocation + FullOffset.RotateAngleAxis(Angle, FVector(0, 0, 1));
		StartLocation.Z = 0;
		TargetReachable = NavSys->ProjectPointToNavigation(StartLocation, OutNavLoc, FVector(200, 200, 100));
		if (bDebugPathfinding) DrawDebugSphere(GetWorld(), StartLocation + FVector(0, 0, 1000), 500, 12, TargetReachable ? FColor::Green : FColor::Red, false, 5, 0, 10);

		if (TargetReachable)
		{
			UNavigationPath* Path = NavSys->FindPathToLocationSynchronously(GetWorld(), StartLocation, Target, MainBuilding);
			if (Path != nullptr)
			{
				if (Path->IsPartial())
				{
					TargetReachable = false;
				}
			}
		}

		if (!Positive) Counter++;
		Positive = !Positive;
	}
	if (!TargetReachable)
	{
		if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("StartTarget not reachable"));
		return;
	}
	StartLocation = OutNavLoc.Location;
	


	/* Pathfinding */
	UNavigationPath* Path = NavSys->FindPathToLocationSynchronously(GetWorld(), StartLocation, Target, MainBuilding);
	if (Path == nullptr)
	{
		if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Path is null"));
		return;
	}
	if (Path->IsPartial())
	{
		if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Path is partial"));
		return;
	}
	
	auto PreviousPoint = StartLocation;
	for (auto Point : Path->PathPoints)
	{
		PathStructs.Add(FPathStruct(Point, false));
		if (bDebugPathfinding)
		{
			DrawDebugSphere(GetWorld(), Point, 500, 12, FColor::Red, false, 5, 0, 10);
			DrawDebugLine(GetWorld(), PreviousPoint, Point, FColor::Red, false, 5, 0, 10);
		}
		PreviousPoint = Point;
	}
	
	if (PathStructs.Num() == 0)
	{
		if (bDebugPathfinding) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PathStructs is empty"));
		return;
	}
	PathStructs[0].IsReal = true;
	PathStructs[PathStructs.Num() - 1].IsReal = true;
	
	//CreateFlux(Building, HitResult.ImpactPoint, AttackOffset);
	
	CreateFlux(Building, PathStructs, ShouldDisplay);
}

