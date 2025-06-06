// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/PlayerController/InteractionComponent.h"

#include "Camera/CameraComponent.h"
#include "Component/PlayerController/FluxComponent.h"
#include "DataAsset/GameSettingsDataAsset.h"
#include "FunctionLibraries/FunctionLibraryInfernale.h"
#include "GameMode/Infernale/PlayerControllerInfernale.h"
#include "Interfaces/InGameUI.h"
#include "Interfaces/Interactable.h"
#include "LD/Breach.h"
#include "LD/Buildings/MainBuilding.h"
#include "Player/InfernalePawn.h"

void UInteractionComponent::AskEndMain()
{
	EndMainInteract();
}

void UInteractionComponent::EmptyMainInteractable()
{
	if (MainInteractable) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, TEXT("MainInteractable is not empty"));
	MainInteractable = nullptr;
}

void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	PlayerControllerInfernale->MousePrimaryStart.AddDynamic(this, &UInteractionComponent::TryInteractStartMain);
	PlayerControllerInfernale->InteractionDone.AddDynamic(this, &UInteractionComponent::EndMainInteract);
	PlayerControllerInfernale->GetFluxComponent()->FluxModeChanged.AddDynamic(this, &UInteractionComponent::OnFluxModeChanged);

	PlayerControllerInfernale->MoveTriggered.AddDynamic(this, &UInteractionComponent::OnPlayerMovementVector2D);
	//PlayerControllerInfernale->ScrollTriggered.AddDynamic(this, &UInteractionComponent::OnPlayerMovementFloat);
	PlayerControllerInfernale->EnableRotationStarted.AddDynamic(this, &UInteractionComponent::OnPlayerMovement);
	PlayerControllerInfernale->EnableMoveActionTriggered.AddDynamic(this, &UInteractionComponent::OnPlayerMovement);

	PlayerControllerInfernale->TransmutationModeChanged.AddDynamic(this, &UInteractionComponent::OnTransmutationModeChanged);

	
}

void UInteractionComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bAllowedToInteract)
	{
		if (HoverInteractable != nullptr) EndHoverInteract();
		return;
	}
	
	FHitResult HitResult;
	const auto Chanel = UFunctionLibraryInfernale::GetCustomTraceChannel(ECustomTraceChannel::HoverInteractor);
	if (const auto Success = GetHitResult(HitResult, Chanel); !Success)
	{
		if (HoverInteractable != nullptr) EndHoverInteract();
		return;
	}

	const auto ActorHit = HitResult.GetActor();
	if (ActorHit == nullptr)
	{
		if (HoverInteractable != nullptr) EndHoverInteract();
		return;
	}

	const auto Interactable = Cast<IInteractable>(ActorHit);
	if (Interactable == nullptr)
	{
		if (HoverInteractable != nullptr) EndHoverInteract();
		return;
	}
	auto HitComponent = HitResult.GetComponent();
	auto HitActor = HitResult.GetActor();
	if (HitComponent != nullptr && bDebugHover) GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Red, FString::Printf(TEXT("Hit actor: %s"), *HitActor->GetName()));
	StartHoverInteract(Interactable);
}

void UInteractionComponent::TryInteractStartMain()
{
	bMainIsClicked = true;
	InteractMain.Broadcast(this, bMainIsClicked);
	
	PlayerControllerInfernale->MousePrimaryEnd.AddDynamic(this, &UInteractionComponent::EndMain);
	
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

	FHitResult HitResult;

	const auto Chanel = UFunctionLibraryInfernale::GetCustomTraceChannel(ECustomTraceChannel::HoverInteractor);
	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, Chanel);
	if (bDebugInteractions && bDebugRayCasts) DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 5, 0, 1);
	const auto ActorHit = HitResult.GetActor();
	
	if (ActorHit == nullptr)
	{
		if (bDebugInteractions) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("No Actor Hit"));
		return;
	}

	if (bDebugInteractions) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Hit Actor: %s"), *ActorHit->GetName()));

	auto Interactable = Cast<IInteractable>(ActorHit);
	if (Interactable == nullptr)
	{
		if (bDebugInteractions) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Not Interactable"));
		return;
	}

	if (MainInteractable != nullptr)
	{
		if (MainInteractable == Interactable) return;
		EndMainInteract();
	}
		
	StartMainInteract(Interactable);
}

void UInteractionComponent::EndMain()
{
	PlayerControllerInfernale->MousePrimaryEnd.RemoveDynamic(this, &UInteractionComponent::EndMain);
	
	bMainIsClicked = false;
	InteractMain.Broadcast(this, bMainIsClicked);
}

void UInteractionComponent::EndMainInteractBySecondary()
{
	if (!CanCloseWithRightClick) return;
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("EndMainInteractBySecondary"));
	EndMainInteract();
}

void UInteractionComponent::EndMainInteract()
{
	SetCanCloseWithRightClick(true);
	if (MainInteractable == nullptr) return;
	if (bDebugInteractions) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("EndMainInteract"));
	if (PlayerControllerInfernale->IsInDraftMode())
	{
		auto MainBuilding = Cast<AMainBuilding>(MainInteractable);
		auto Breach = Cast<ABreach>(MainInteractable);
		if ((MainBuilding || (Breach && Breach->GetMainBuilding()->GetOwner().Team == PlayerControllerInfernale->GetTeam())) &&
			!isNewCycle && PlayerControllerInfernale->IsMyTurn())
		{
			if (MainBuilding) PlayerControllerInfernale->ChangeBaseOwnerServer(Cast<AActor>(MainInteractable), ETeam::NatureTeam, EPlayerOwning::Nature);
			else if (Breach) PlayerControllerInfernale->ChangeBaseOwnerServer(Cast<AActor>(Breach->GetMainBuilding()), ETeam::NatureTeam, EPlayerOwning::Nature);
		}
		isNewCycle = false;
	}
	else MainInteractable->InteractEndMain(PlayerControllerInfernale);
	MainInteractable = nullptr;
	PlayerControllerInfernale->PawnMoved.RemoveDynamic(this, &UInteractionComponent::OnPawnMoved);
	PlayerControllerInfernale->ScrollTriggered.RemoveDynamic(this, &UInteractionComponent::OnZoom);
	PlayerControllerInfernale->EscapeStarted.RemoveDynamic(this, &UInteractionComponent::EndMainInteract);
	PlayerControllerInfernale->MouseSecondaryEnd.RemoveDynamic(this, &UInteractionComponent::EndMainInteractBySecondary);
}

void UInteractionComponent::TryInteractStartSecondary()
{
	InteractSecondary.Broadcast(this, bSecondaryIsClicked);
	PlayerControllerInfernale->MouseSecondaryEnd.AddDynamic(this, &UInteractionComponent::EndSecondary);
}

void UInteractionComponent::EndSecondary()
{
	PlayerControllerInfernale->MouseSecondaryEnd.RemoveDynamic(this, &UInteractionComponent::EndSecondary);
	bSecondaryIsClicked = false;
	InteractSecondary.Broadcast(this, bSecondaryIsClicked);
}

void UInteractionComponent::EndSecondaryInteract()
{
	if (SecondaryInteractable == nullptr) return;
	if (bDebugInteractions) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("EndSecondaryInteract"));
	
	SecondaryInteractable->InteractEndSecondary(PlayerControllerInfernale);
	SecondaryInteractable = nullptr;
	PlayerControllerInfernale->PawnMoved.RemoveDynamic(this, &UInteractionComponent::OnPawnMoved);
	PlayerControllerInfernale->EscapeStarted.RemoveDynamic(this, &UInteractionComponent::EndMainInteract);
	PlayerControllerInfernale->MouseSecondaryEnd.RemoveDynamic(this, &UInteractionComponent::EndMainInteractBySecondary);
}

void UInteractionComponent::StartMainInteract(IInteractable* Interactable)
{
	if (bDebugInteractions)
	{
		AActor* InteractableObj = Cast<AActor>(Interactable);
		if (Interactable) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("StartMainInteract: %s"), *InteractableObj->GetName()));
		else GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("StartMainInteract"));
	}
	MainInteractable = Interactable;
	SetCanCloseWithRightClick(true);
	if (PlayerControllerInfernale->IsInDraftMode())
	{
		auto MainBuilding = Cast<AMainBuilding>(MainInteractable);
		auto Breach = Cast<ABreach>(MainInteractable);
		if (!MainBuilding && !Breach)
		{
			UE_LOG(LogTemp, Warning, TEXT("MainBuilding and breach is null"));
			return;
		}
		if (!PlayerControllerInfernale)
		{
			UE_LOG(LogTemp, Warning, TEXT("PlayerControllerInfernale is null"));
			return;
		}
		if (((MainBuilding && MainBuilding->GetOwner().Team == ETeam::NatureTeam) || (Breach && Breach->GetMainBuilding()->GetOwner().Team == ETeam::NatureTeam)) &&
			!isNewCycle && PlayerControllerInfernale->IsMyTurn())
		{
			if (MainBuilding) PlayerControllerInfernale->ChangeBaseOwnerServer(Cast<AActor>(MainInteractable), PlayerControllerInfernale->GetOwnerInfo().Team, PlayerControllerInfernale->GetOwnerInfo().Player);
			else if (Breach) PlayerControllerInfernale->ChangeBaseOwnerServer(Cast<AActor>(Breach->GetMainBuilding()), PlayerControllerInfernale->GetOwnerInfo().Team, PlayerControllerInfernale->GetOwnerInfo().Player);
		}
		else
		{
			MainInteractable = nullptr;
		}
		isNewCycle = false;
	}
	else MainInteractable->InteractStartMain(PlayerControllerInfernale);
	PlayerControllerInfernale->PawnMoved.AddDynamic(this, &UInteractionComponent::OnPawnMoved);
	PlayerControllerInfernale->ScrollTriggered.AddDynamic(this, &UInteractionComponent::OnZoom);
	PlayerControllerInfernale->EscapeStarted.AddDynamic(this, &UInteractionComponent::EndMainInteract);
	PlayerControllerInfernale->MouseSecondaryEnd.AddDynamic(this, &UInteractionComponent::EndMainInteractBySecondary);
	
	OnPawnMoved(PlayerControllerInfernale->GetInfernalePawn()->GetActorLocation());
}

void UInteractionComponent::ForceSetMainInteract(IInteractable* Interactable)
{
	if (MainInteractable != nullptr) EndMainInteract();
	MainInteractable = Interactable;
	PlayerControllerInfernale->PawnMoved.AddDynamic(this, &UInteractionComponent::OnPawnMoved);
	PlayerControllerInfernale->ScrollTriggered.AddDynamic(this, &UInteractionComponent::OnZoom);
	PlayerControllerInfernale->EscapeStarted.AddDynamic(this, &UInteractionComponent::EndMainInteract);
	PlayerControllerInfernale->MouseSecondaryEnd.AddDynamic(this, &UInteractionComponent::EndMainInteractBySecondary);
}

void UInteractionComponent::ForceStartMainInteract(IInteractable* Interactable, bool ForceFlux)
{
	StartMainInteract(Interactable);
	if (!ForceFlux) return;
	
	AMainBuilding* MainBuilding = Cast<AMainBuilding>(Interactable);
	if (!MainBuilding) return;

	UFluxComponent* FluxComponent = PlayerControllerInfernale->GetFluxComponent();
	if (!FluxComponent) return;
	const TWeakObjectPtr<ABuildingParent> Building = TWeakObjectPtr<ABuildingParent>(MainBuilding);
	FluxComponent->AddBuildingToSelectionForce(Building);
}

void UInteractionComponent::ForceEndMainInteractIf(IInteractable* Interactable)
{
	if (MainInteractable != Interactable) return;
	EndMainInteract();
	AMainBuilding* MainBuilding = Cast<AMainBuilding>(Interactable);
	if (!MainBuilding) return;

	UFluxComponent* FluxComponent = PlayerControllerInfernale->GetFluxComponent();
	if (!FluxComponent) return;
	const TWeakObjectPtr<ABuildingParent> Building = TWeakObjectPtr<ABuildingParent>(MainBuilding);
	FluxComponent->RemoveBuildingFromSelectionForce(Building);
}

void UInteractionComponent::SetAllowedToInteract(bool bValue)
{
	bAllowedToInteract = bValue;
}

AActor* UInteractionComponent::GetMainInteractActor() const
{
	if (MainInteractable == nullptr) return nullptr;
	return Cast<AActor>(MainInteractable);
}

bool UInteractionComponent::CanCanCloseWithRightClick()
{
	return CanCloseWithRightClick;
}

void UInteractionComponent::SetCanCloseWithRightClick(bool bValue)
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, bValue ? FColor::Green : FColor::Red, FString::Printf(TEXT("SetCanCloseWithRightClick: %s"), bValue ? TEXT("true") : TEXT("false")));
	CanCloseWithRightClick = bValue;
}

void UInteractionComponent::StartSecondaryInteract(IInteractable* Interactable)
{
	if (bDebugInteractions) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("StartSecondaryInteract"));
	if (PlayerControllerInfernale->IsInDraftMode()) return;
	SecondaryInteractable = Interactable;
	SecondaryInteractable->InteractStartSecondary(PlayerControllerInfernale);
	PlayerControllerInfernale->PawnMoved.AddDynamic(this, &UInteractionComponent::OnPawnMoved);
	PlayerControllerInfernale->EscapeStarted.AddDynamic(this, &UInteractionComponent::EndMainInteract);
	PlayerControllerInfernale->MouseSecondaryEnd.AddDynamic(this, &UInteractionComponent::EndMainInteractBySecondary);
	
	OnPawnMoved(PlayerControllerInfernale->GetInfernalePawn()->GetActorLocation());
}

void UInteractionComponent::StartHoverInteract(IInteractable* Interactable)
{
	if (HoverInteractable == Interactable) return;
	if (HoverInteractable != nullptr) EndHoverInteract();
	
	if (bDebugHover) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("StartHoverInteract"));
	if (PlayerControllerInfernale->IsInDraftMode() || PlayerControllerInfernale->bIsEscapeMenuOpen) return;
	HoverInteractable = Interactable;
	HoverInteractable->InteractStartHover(PlayerControllerInfernale);
}

void UInteractionComponent::EndHoverInteract()
{
	if (HoverInteractable == nullptr) return;
	if (bDebugHover) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("EndHoverInteract"));

	HoverInteractable->InteractEndHover(PlayerControllerInfernale);
	HoverInteractable = nullptr;
}

bool UInteractionComponent::GetHitResult(FHitResult& HitResult, const ECollisionChannel Channel)
{
	const auto InfernalePawn = PlayerControllerInfernale->GetInfernalePawn();
	if (InfernalePawn == nullptr) return false;
	const auto CameraComponent = InfernalePawn->GetCameraComponent();
	if (CameraComponent == nullptr) return false;

	const auto Start = CameraComponent->GetComponentLocation();

	FVector MouseWorldLocation;
	FVector MouseWorldDirection;

	if (const auto Success = PlayerControllerInfernale->DeprojectMousePositionToWorld(MouseWorldLocation, MouseWorldDirection); !Success) return false;

	const auto Direction = MouseWorldDirection.GetSafeNormal();
	const auto End = Start + Direction * UFunctionLibraryInfernale::GetGameSettingsDataAsset()->RayCastLength;

	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, Channel);
	return true;
}

void UInteractionComponent::OnPawnMoved(const FVector Location)
{
	if (MainInteractable == nullptr) return;
	if (!MainInteractable->InteractableHasUIOpen()) return;

	auto InGameUI = Cast<IInGameUI>(MainInteractable);
	if (InGameUI == nullptr) return;

	UObject* Object = Cast<UObject>(InGameUI);
	InGameUI->Execute_FacePosition(Object, Location);
}

void UInteractionComponent::OnZoom(const float Value)
{
	if (MainInteractable == nullptr) return;
	if (!MainInteractable->InteractableHasUIOpen()) return;

	auto InGameUI = Cast<IInGameUI>(MainInteractable);
	if (InGameUI == nullptr) return;

	UObject* Object = Cast<UObject>(InGameUI);
	auto Location = PlayerControllerInfernale->GetInfernalePawn()->GetActorLocation();
	InGameUI->Execute_FacePosition(Object, Location);
}

void UInteractionComponent::OnFluxModeChanged(const bool bIsFluxMode)
{
	bAllowedToInteract = !bIsFluxMode;

	if (MainInteractable != nullptr) EndMainInteract();
	if (SecondaryInteractable != nullptr) EndSecondaryInteract();
	if (HoverInteractable != nullptr) EndHoverInteract();

	if (bAllowedToInteract)
	{
		PlayerControllerInfernale->MousePrimaryStart.AddDynamic(this, &UInteractionComponent::TryInteractStartMain);
		PlayerControllerInfernale->InteractionDone.AddDynamic(this, &UInteractionComponent::EndMainInteract);
		return;
	}

	PlayerControllerInfernale->MousePrimaryStart.RemoveDynamic(this, &UInteractionComponent::TryInteractStartMain);
	PlayerControllerInfernale->InteractionDone.RemoveDynamic(this, &UInteractionComponent::EndMainInteract);
}

void UInteractionComponent::OnPlayerMovement()
{
	if (MainInteractable == nullptr) return;
	if (MainInteractable->ShouldEndMainInteractOnMove()) EndMainInteract();
}

void UInteractionComponent::OnPlayerMovementVector2D(const FVector2D MovementVector)
{
	OnPlayerMovement();
}

void UInteractionComponent::OnPlayerMovementFloat(const float Value)
{
	OnPlayerMovement();
}

void UInteractionComponent::OnTransmutationModeChanged(const bool bValue)
{
	bAllowedToInteract = !bValue;
}
