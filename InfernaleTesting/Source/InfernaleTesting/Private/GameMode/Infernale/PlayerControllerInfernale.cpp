// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/Infernale/PlayerControllerInfernale.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Component/ActorComponents/BattleManagerComponent.h"
#include "Component/GameModeComponent/VictoryManagerComponent.h"
#include "Component/PlayerController/BuildComponent.h"
#include "Component/PlayerController/FluxComponent.h"
#include "Component/PlayerController/InteractionComponent.h"
#include "Component/PlayerController/UIComponent.h"
#include "Component/PlayerState/EconomyComponent.h"
#include "Component/PlayerState/TransmutationComponent.h"
#include "DataAsset/BuildingListDataAsset.h"
#include "DataAsset/GameSettingsDataAsset.h"
#include "DataAsset/InfernaleInputDataAsset.h"
#include "FunctionLibraries/FunctionLibraryInfernale.h"
#include "GameMode/Infernale/GameModeInfernale.h"
#include "GameMode/Infernale/PlayerStateInfernale.h"
#include "Kismet/GameplayStatics.h"
#include "LD/Buildings/MainBuilding.h"
#include "Manager/AmalgamVisualisationManager.h"
#include "Manager/UnitActorManager.h"
#include "Net/UnrealNetwork.h"
#include "Player/InfernalePawn.h"

FPlayerControllerInfo::FPlayerControllerInfo(): PlayerController(nullptr), PlayerLocation(FVector(0.f, 0.f, 0.f))
{
}

APlayerControllerInfernale::APlayerControllerInfernale()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComponent"));
	BuildComponent = CreateDefaultSubobject<UBuildComponent>(TEXT("BuildComponent"));
	FluxComponent = CreateDefaultSubobject<UFluxComponent>(TEXT("FluxComponent"));
	TransmutationComponent = CreateDefaultSubobject<UTransmutationComponent>(TEXT("TransmutationComponent"));
}

void APlayerControllerInfernale::SetPlayerInputComponent(UInputComponent* NewPlayerInputComponent)
{
	PlayerInputComponent = NewPlayerInputComponent;
	BindInputs();
}

void APlayerControllerInfernale::SetInfernalePawn(AInfernalePawn* NewInfernalePawn)
{
	InfernalePawn = NewInfernalePawn;
	InfernalePawn->PawnMoved.AddDynamic(this, &APlayerControllerInfernale::OnPawnMoved);
}

AInfernalePawn* APlayerControllerInfernale::GetInfernalePawn() const
{
	return InfernalePawn;
}

void APlayerControllerInfernale::SetBuildingPermanent(ABuilding* Building, bool IsPermanent)
{
	SetBuildingPermanentServer(Building, IsPermanent);
}

void APlayerControllerInfernale::RemoveLoadingScreen()
{
	RemoveLoadingScreenOwning();
}

UUIComponent* APlayerControllerInfernale::GetUIComponent() const
{
	return UIComponent;
}

UTransmutationComponent* APlayerControllerInfernale::GetTransmutationComponent() const
{
	return TransmutationComponent;
}

UBuildComponent* APlayerControllerInfernale::GetBuildComponent() const
{
	return BuildComponent;
}

AUnitActorManager* APlayerControllerInfernale::GetUnitActorManager() const
{
	return UnitActorManager;
}

void APlayerControllerInfernale::MoveToSpawn(FVector Location)
{
	InfernalePawn->MoveToSpawn(Location);
}

void APlayerControllerInfernale::CallOnPrimaryStart()
{
	PrimaryActionStarted();
}

void APlayerControllerInfernale::CallOnPrimaryEnd()
{
	PrimaryActionEnded();
}

void APlayerControllerInfernale::BossAddToSpawnCharge(ABoss* Boss, float Charge)
{
	BossAddToSpawnChargeServer(Boss, Charge);
}

void APlayerControllerInfernale::BossAddToSpawnChargeServer_Implementation(ABoss* Boss, float Charge)
{
	Boss->AddToSpawnCharge(Charge);
}

bool APlayerControllerInfernale::UseFluxMode()
{
	SyncGameSettings();
	return bUseFluxMode;
}

UFluxComponent* APlayerControllerInfernale::GetFluxComponent() const
{
	return FluxComponent;
}

UInteractionComponent* APlayerControllerInfernale::GetInteractionComponent() const
{
	return InteractionComponent;
}

void APlayerControllerInfernale::CallInteractionDone()
{
	InteractionDone.Broadcast();
}

void APlayerControllerInfernale::CallFluxFluxModeStart()
{
	FluxModeStart.Broadcast();
}

void APlayerControllerInfernale::CallTransmutationToogle()
{
	TransmutationToogle.Broadcast();
}

void APlayerControllerInfernale::AskBuildBuilding(ABreach* Breach, FBuildingStruct Building)
{
	BuildBuildingServer(Breach, Building);
}

void APlayerControllerInfernale::ServerActionChooseBase_Implementation()
{
	Cast<AGameModeInfernale>(GetWorld()->GetAuthGameMode())->CycleTurns();;
}

bool APlayerControllerInfernale::IsInDraftMode()
{
	return bIsInDraftMode;
}

bool APlayerControllerInfernale::IsMyTurn()
{
	return bIsMyTurn;
}

void APlayerControllerInfernale::EndCycleOwning_Implementation()
{
	if (InteractionComponent == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("InteractionComponent not found in AGameModeInfernale::CycleTurns"));
	}
	InteractionComponent->isNewCycle = true;
	InteractionComponent->AskEndMain();
	InteractionComponent->EmptyMainInteractable();
}

bool APlayerControllerInfernale::IsCtrlPressed() const
{
	return bIsCtrlPressed;
}

bool APlayerControllerInfernale::IsShiftPressed() const
{
	return bIsShiftPressed;
}

ETeam APlayerControllerInfernale::GetTeam() const
{
	const auto PS = GetPlayerState<APlayerStateInfernale>();
	if (PS == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("APlayerControllerInfernale::GetTeam > PlayerState is null"));
		return ETeam::NatureTeam;
	}
	return PS->GetOwnerInfo().Team;
}

EPlayerOwning APlayerControllerInfernale::GetPlayerOwning() const
{
	const auto PS = GetPlayerState<APlayerStateInfernale>();
	if (PS == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("APlayerControllerInfernale::GetPlayerOwning > PlayerState is null"));
		return EPlayerOwning::Nature;
	}
	return PS->GetOwnerInfo().Player;
}

FOwner APlayerControllerInfernale::GetOwnerInfo() const
{
    const auto PS = GetPlayerState<APlayerStateInfernale>();
	if (PS == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("APlayerControllerInfernale::GetOwnerInfo > PlayerState is null"));
		FOwner DefaultOwner;
		DefaultOwner.Player = EPlayerOwning::Nature;
		DefaultOwner.Team = ETeam::NatureTeam;
        return DefaultOwner;
    }
	return PS->GetOwnerInfo();
}

bool APlayerControllerInfernale::IsAllowedToCheatBP() const
{
	return AllowedToCheat;
}

void APlayerControllerInfernale::AskReplicateCenterPoint(FVector NewCenterPoint)
{
	ReplicateCenterPointServerUnreliable(NewCenterPoint);
}

FVector APlayerControllerInfernale::GetCameraCenterPoint() const
{
	if (!InfernalePawn) return FVector::ZeroVector;
	return InfernalePawn->GetLookAtLocationPoint();
}

/* TArray<FDataForVisualisation> */
void APlayerControllerInfernale::UpdateUnits(const TArray<FDataForVisualisation>& UnitsToReplicate, const TArray<FMassEntityHandle>& EntitiesToHide)
{
	// if (!AllowedToReplicateThisFrame) return;
	// AllowedToReplicateThisFrame = false;
	if (!IsLocalController())
	{
		//UpdateUnitsOwner(UnitsToReplicate, EntitiesToHide);
		UpdateUnitsInfoOwner(UnitsToReplicate);

		if (EntitiesToHide.Num() == 0) return;

		ReplicateUnitToHide = false;
		CurrentTimeReplicateUnitToHide = 0.f;
		FramesSinceLastReplicate = 0;
		UpdateUnitsToHideOwner(EntitiesToHide);
		return;
	}
	UpdateUnitsSelf(UnitsToReplicate, EntitiesToHide);
}

void APlayerControllerInfernale::DoVfx(FBattleInfo BattleInfo)
{
	DoVFXOwner(BattleInfo);
}

void APlayerControllerInfernale::DoDeathVfx(FDeathInfo DeathInfo)
{
	DoDeathVFXOwner(DeathInfo);
}

bool APlayerControllerInfernale::IsInDraftMode() const
{
	return bIsInDraftMode;
}

void APlayerControllerInfernale::SetDraftMode(bool newVal)
{
	bIsInDraftMode = newVal;
}

void APlayerControllerInfernale::EnableInputs(bool newVal)
{
	EnableInputsOwning(newVal);
}

void APlayerControllerInfernale::BeginPlay()
{
	Super::BeginPlay();
	MyTurnChanged.AddDynamic(this, &APlayerControllerInfernale::OnMyTurnChanged);
	DraftEnded.AddDynamic(this, &APlayerControllerInfernale::OnDraftEnded);
	if (IsLocalController()) UIComponent->CreateLoadingUIBP();
	FluxComponent->FluxModeChanged.AddDynamic(this, &APlayerControllerInfernale::OnFluxModeChanged);
	SyncGameSettings();
	SetupInputMappings();
	
	InfernalePawn = Cast<AInfernalePawn>(GetPawn());
	DisableInput(this);

	TransmutationComponent->TransmutationMode.AddDynamic(this, &APlayerControllerInfernale::OnTransmutationMode);
	AmalgamVisualisationManager = Cast<AAmalgamVisualisationManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AAmalgamVisualisationManager::StaticClass()));
	UnitActorManager = Cast<AUnitActorManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AUnitActorManager::StaticClass()));

	if (GameSettingsDataAsset) AllowedToCheat = !GameSettingsDataAsset->bIsJuryMode;
}

void APlayerControllerInfernale::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APlayerControllerInfernale, InfernalePawn);
}

void APlayerControllerInfernale::Tick(float DeltaTime)
{
	AllowedToReplicateThisFrame = true;
	if (!HasAuthority()) return;
	if (InfernalePawn == nullptr) return;

	if (FramesSinceLastReplicate <= 100) FramesSinceLastReplicate++;
	if (ReplicateUnitToHide) return;
	CurrentTimeReplicateUnitToHide += DeltaTime;
	if (CurrentTimeReplicateUnitToHide >= TimeReplicateUnitToHide) ReplicateUnitToHide = true;
	
	// const auto Point = InfernalePawn->GetLookAtLocationPoint();
	// DrawSphereAtPointClient(Point);
}

void APlayerControllerInfernale::DrawSphereAtPointClient_Implementation(FVector Location)
{
	if (InfernalePawn == nullptr) return;
	const auto PawnLoc = InfernalePawn->GetActorLocation();
	const FVector Start = PawnLoc + (Location - PawnLoc).GetSafeNormal() * 200;
	DrawDebugLine(GetWorld(), Start, Location, FColor::Green, false, 0.f, 0, 5);
	DrawDebugSphere(GetWorld(), Location, 100, 12, FColor::Red, false, 0.f, 0, 5);
}

void APlayerControllerInfernale::SetupInputMappings()
{
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	
	if (Subsystem == nullptr) return;
	if (GameInputMappingContext == nullptr) return;

	Subsystem->ClearAllMappings();
	Subsystem->AddMappingContext(GameInputMappingContext, 0);

	/* Can't use this inputmode for now */
	//SetInputMode(FInputModeGameAndUI());
	//SetShowMouseCursor(true);
}

void APlayerControllerInfernale::BindInputs()
{
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	
	if (Input == nullptr) return;
	
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("BindInputs"));
	
	// IA_Move
	Input->BindAction(InfernaleInputDataAsset->MoveAction, ETriggerEvent::Triggered, this, &APlayerControllerInfernale::MoveActionTriggered);
	Input->BindAction(InfernaleInputDataAsset->MoveAction, ETriggerEvent::Completed, this, &APlayerControllerInfernale::MoveActionCompleted);
	
	// IA_EnableMove
	Input->BindAction(InfernaleInputDataAsset->EnableMoveAction, ETriggerEvent::Triggered, this, &APlayerControllerInfernale::EnableMoveActionTriggeredFunc);
	Input->BindAction(InfernaleInputDataAsset->EnableMoveAction, ETriggerEvent::Canceled, this, &APlayerControllerInfernale::EnableMoveActionCompletedFunc);
	Input->BindAction(InfernaleInputDataAsset->EnableMoveAction, ETriggerEvent::Completed, this, &APlayerControllerInfernale::EnableMoveActionCompletedFunc);
	
	// IA_Look
	Input->BindAction(InfernaleInputDataAsset->LookAction, ETriggerEvent::Triggered, this, &APlayerControllerInfernale::LookActionTriggered);
	
	// IA_CameraHeight
	Input->BindAction(InfernaleInputDataAsset->CameraHeightAction, ETriggerEvent::Triggered, this, &APlayerControllerInfernale::CameraHeightActionTriggered);
	
	// IA_EnableRotation
	Input->BindAction(InfernaleInputDataAsset->EnableRotationAction, ETriggerEvent::Started, this, &APlayerControllerInfernale::EnableRotationActionStarted);
	Input->BindAction(InfernaleInputDataAsset->EnableRotationAction, ETriggerEvent::Canceled, this, &APlayerControllerInfernale::EnableRotationActionCompleted);
	Input->BindAction(InfernaleInputDataAsset->EnableRotationAction, ETriggerEvent::Completed, this, &APlayerControllerInfernale::EnableRotationActionCompleted);
	
	
	// IA_PrimaryAction
	Input->BindAction(InfernaleInputDataAsset->PrimaryAction, ETriggerEvent::Started, this, &APlayerControllerInfernale::PrimaryActionStarted);
	Input->BindAction(InfernaleInputDataAsset->PrimaryAction, ETriggerEvent::Completed, this, &APlayerControllerInfernale::PrimaryActionEnded);
	Input->BindAction(InfernaleInputDataAsset->PrimaryAction, ETriggerEvent::Canceled, this, &APlayerControllerInfernale::PrimaryActionEnded);
	Input->BindAction(InfernaleInputDataAsset->PrimaryAction, ETriggerEvent::Triggered, this, &APlayerControllerInfernale::PrimaryActionTriggered);
	
	// IA_SecondaryAction
	Input->BindAction(InfernaleInputDataAsset->SecondaryAction, ETriggerEvent::Started, this, &APlayerControllerInfernale::SecondaryActionStarted);
	Input->BindAction(InfernaleInputDataAsset->SecondaryAction, ETriggerEvent::Completed, this, &APlayerControllerInfernale::SecondaryActionEnded);
	Input->BindAction(InfernaleInputDataAsset->SecondaryAction, ETriggerEvent::Canceled, this, &APlayerControllerInfernale::SecondaryActionEnded);
	Input->BindAction(InfernaleInputDataAsset->SecondaryAction, ETriggerEvent::Triggered, this, &APlayerControllerInfernale::SecondaryActionTriggered);
	
	// IA_FluxMode
	if (bUseFluxMode) Input->BindAction(InfernaleInputDataAsset->FluxModeAction, ETriggerEvent::Started, this, &APlayerControllerInfernale::FluxModeActionStarted);
	
	// IA_Escape
	Input->BindAction(InfernaleInputDataAsset->EscapeAction, ETriggerEvent::Started, this, &APlayerControllerInfernale::EscapeActionStarted);

	// IA_Ctrl
	Input->BindAction(InfernaleInputDataAsset->ControlAction, ETriggerEvent::Started, this, &APlayerControllerInfernale::ControlActionStarted);
	Input->BindAction(InfernaleInputDataAsset->ControlAction, ETriggerEvent::Completed, this, &APlayerControllerInfernale::ControlActionCompleted);

	// IA_Shift
	Input->BindAction(InfernaleInputDataAsset->ShiftAction, ETriggerEvent::Started, this, &APlayerControllerInfernale::ShiftActionStarted);
	Input->BindAction(InfernaleInputDataAsset->ShiftAction, ETriggerEvent::Completed, this, &APlayerControllerInfernale::ShiftActionCompleted);

	// IA_P
	Input->BindAction(InfernaleInputDataAsset->PKeyAction, ETriggerEvent::Started, this, &APlayerControllerInfernale::PActionStarted);
	Input->BindAction(InfernaleInputDataAsset->PKeyAction, ETriggerEvent::Completed, this, &APlayerControllerInfernale::PActionCompleted);

	// IA_DotDot
	Input->BindAction(InfernaleInputDataAsset->DotDotAction, ETriggerEvent::Started, this, &APlayerControllerInfernale::DotDotActionStarted);

	// IA_CameraProfiles
	Input->BindAction(InfernaleInputDataAsset->CameraProfilesAction, ETriggerEvent::Triggered, this, &APlayerControllerInfernale::CameraProfilesActionTriggered);
	Input->BindAction(InfernaleInputDataAsset->CameraProfilesAction, ETriggerEvent::Completed, this, &APlayerControllerInfernale::CameraProfilesActionCompleted);

	// IA_Transmutation
	Input->BindAction(InfernaleInputDataAsset->TransmutationAction, ETriggerEvent::Started, this, &APlayerControllerInfernale::CallTransmutationToogle);

	// Increase & Decrease Batch Number
	Input->BindAction(InfernaleInputDataAsset->IncreaseAction, ETriggerEvent::Started, this, &APlayerControllerInfernale::IncreaseBatch);
	Input->BindAction(InfernaleInputDataAsset->DecreaseAction, ETriggerEvent::Started, this, &APlayerControllerInfernale::DecreaseBatch);

	// IA_Space
	Input->BindAction(InfernaleInputDataAsset->SpaceAction, ETriggerEvent::Started, this, &APlayerControllerInfernale::OnSpacePressed);
	Input->BindAction(InfernaleInputDataAsset->SpaceAction, ETriggerEvent::Completed, this, &APlayerControllerInfernale::OnSpaceReleased);

	Input->BindAction(InfernaleInputDataAsset->EnableLANAction, ETriggerEvent::Started, this, &APlayerControllerInfernale::EnableLANActionStarted);

	// IA_KeyboardRotation
	Input->BindAction(InfernaleInputDataAsset->KeyboardRotationAction, ETriggerEvent::Triggered, this, &APlayerControllerInfernale::KeyboardRotationActionTriggered);

	// IA_ResetRotation
	Input->BindAction(InfernaleInputDataAsset->ResetRotationAction, ETriggerEvent::Started, this, &APlayerControllerInfernale::OnResetRotationStarted);
}

void APlayerControllerInfernale::MoveActionTriggered(const FInputActionValue& Value)
{
	if (!bClickEventsAllowed) return;
	const auto MoveVector = Value.Get<FVector2D>();
	MoveTriggered.Broadcast(MoveVector);
}

void APlayerControllerInfernale::MoveActionCompleted(const FInputActionValue& Value)
{
	if (!bClickEventsAllowed) return;
	const auto MoveVector = Value.Get<FVector2D>();
	MoveCompleted.Broadcast(MoveVector);
}

void APlayerControllerInfernale::EnableMoveActionTriggeredFunc()
{
	if (!bClickEventsAllowed) return;
	EnableMoveActionTriggered.Broadcast();
}

void APlayerControllerInfernale::EnableMoveActionCompletedFunc()
{
	if (!bClickEventsAllowed) return;
	EnableMoveActionCompleted.Broadcast();
}

void APlayerControllerInfernale::LookActionTriggered(const FInputActionValue& Value)
{
	if (!bClickEventsAllowed) return;
	const auto LookVector = Value.Get<FVector2D>();
	LookTriggered.Broadcast(LookVector);
}

void APlayerControllerInfernale::CameraHeightActionTriggered(const FInputActionValue& Value)
{
	if (!bClickEventsAllowed) return;
	const auto CameraHeight = Value.Get<float>();
	ScrollTriggered.Broadcast(CameraHeight);
}

void APlayerControllerInfernale::EnableRotationActionStarted(const FInputActionValue& Value)
{
	if (!bClickEventsAllowed) return;
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("EnableRotationActionStarted"));
	EnableRotationStarted.Broadcast();
}

void APlayerControllerInfernale::EnableRotationActionCompleted(const FInputActionValue& Value)
{
	if (!bClickEventsAllowed) return;
	EnableRotationCompleted.Broadcast();
}

void APlayerControllerInfernale::PrimaryActionStarted()
{
	if (!bClickEventsAllowed) return;
	if (bPrintDebugInputMouse) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PrimaryActionStarted"));
	MousePrimaryStart.Broadcast();
}

void APlayerControllerInfernale::PrimaryActionEnded()
{
	if (!bClickEventsAllowed) return;
	if (bPrintDebugInputMouse) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PrimaryActionEnded"));
	MousePrimaryEnd.Broadcast();
}

void APlayerControllerInfernale::PrimaryActionTriggered()
{
	if (!bClickEventsAllowed) return;
	MousePrimaryTriggered.Broadcast();
}

void APlayerControllerInfernale::SecondaryActionStarted()
{
	if (!bClickEventsAllowed) return;
	if (bPrintDebugInputMouse) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("SecondaryActionStarted"));
	MouseSecondaryStart.Broadcast();
}

void APlayerControllerInfernale::SecondaryActionEnded()
{
	if (!bClickEventsAllowed) return;
	if (bPrintDebugInputMouse) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("SecondaryActionEnded"));
	MouseSecondaryEnd.Broadcast();
}

void APlayerControllerInfernale::SecondaryActionTriggered()
{
	if (!bClickEventsAllowed) return;
	MouseSecondaryTriggered.Broadcast();
}

void APlayerControllerInfernale::FluxModeActionStarted()
{
	FluxModeStart.Broadcast();
}

void APlayerControllerInfernale::MouseScroll(const FInputActionValue& Value)
{
	if (!bClickEventsAllowed) return;
	const auto ScrollValue = Value.Get<float>();

	if (!IsScrolling) StartScroll();
	else ResetScrollTimer();

	if (ScrollValue > 0) MouseScrollUp.Broadcast();
	else MouseScrollDown.Broadcast();
}

void APlayerControllerInfernale::StartScroll()
{
	if (!bClickEventsAllowed) return;
	IsScrolling = true;
	MouseScrollStarted.Broadcast();

	ResetScrollTimer();
}

void APlayerControllerInfernale::StopScroll()
{
	if (!bClickEventsAllowed) return;
	IsScrolling = false;
	MouseScrollEnded.Broadcast();
}

void APlayerControllerInfernale::ResetScrollTimer()
{
	ScrollTimerHandle.Invalidate();
	GetWorld()->GetTimerManager().SetTimer(ScrollTimerHandle, this, &APlayerControllerInfernale::StopScroll, TimeBeforeScrollEnd, false);
}

void APlayerControllerInfernale::EscapeActionStarted()
{
	EscapeStarted.Broadcast();
	OnEscapeStartedBP();
}

void APlayerControllerInfernale::ControlActionStarted()
{
	bIsCtrlPressed = true;
}

void APlayerControllerInfernale::ControlActionCompleted()
{
	bIsCtrlPressed = false;
}

void APlayerControllerInfernale::ShiftActionStarted()
{
	bIsShiftPressed = true;
	ShiftModeChanged.Broadcast();
	if (bDebugShift) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("ShiftAction true"));
}

void APlayerControllerInfernale::ShiftActionCompleted()
{
	bIsShiftPressed = false;
	ShiftModeChanged.Broadcast();
	if (bDebugShift) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("ShiftAction false"));
}

void APlayerControllerInfernale::PActionStarted()
{
	if (!AllowedToCheat) return;
	bIsPPressed = true;
	FOwner NewOwner = GetPlayerState<APlayerStateInfernale>()->GetOwnerInfo();

	switch (NewOwner.Team)
	{
	case ETeam::Team1:
		NewOwner.Team = ETeam::Team2;
		break;
	case ETeam::Team2:
		NewOwner.Team = ETeam::Team3;
		break;
	case ETeam::Team3:
		NewOwner.Team = ETeam::Team4;
		break;
	case ETeam::Team4:
		NewOwner.Team = ETeam::Team1;
		break;
	default: break;
	}

	GetPlayerState<APlayerStateInfernale>()->SetOwnerInfo(NewOwner);
}

void APlayerControllerInfernale::PActionCompleted()
{
	if (!AllowedToCheat) return;
	bIsPPressed = false;
}

void APlayerControllerInfernale::OnFluxModeChanged(const bool bIsFluxMode)
{
	if (bIsFluxMode) InfernalePawn->UnsubscribeFromRotateEvents();
	else InfernalePawn->SubscribeToRotateEvents();
}

void APlayerControllerInfernale::OnPawnMoved(const FVector Location)
{
	PawnMoved.Broadcast(Location);
}

void APlayerControllerInfernale::OnTransmutationMode(const bool bIsTransmutationMode)
{
	/* Unsubscribe click events and send event for Hovers */
	if (bIsTransmutationMode) UnsubscribeClickEvents();
	else SubscribeClickEvents();
	
	TransmutationModeChanged.Broadcast(bIsTransmutationMode);
}

void APlayerControllerInfernale::RemoveLoadingScreenOwning_Implementation()
{
	EnableInput(this);
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &APlayerControllerInfernale::RemoveLodingScreenAfterDelay, 0.1f, false);
}

void APlayerControllerInfernale::EnableInputsOwning_Implementation(const bool bIsInputsAllowed)
{
	if (bIsInputsAllowed)
	{
		EnableInput(this);
		return;
	}
	DisableInput(this);
}

void APlayerControllerInfernale::IncreaseBatchServer_Implementation()
{
	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAmalgamVisualisationManager::StaticClass(), OutActors);

	if (OutActors.Num() <= 0) return;
	auto VisualisationManager = Cast<AAmalgamVisualisationManager>(OutActors[0]);
	if (VisualisationManager == nullptr) return;
	VisualisationManager->ChangeBatch(100);
}

void APlayerControllerInfernale::DecreaseBatchServer_Implementation()
{
	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAmalgamVisualisationManager::StaticClass(), OutActors);

	if (OutActors.Num() <= 0) return;
	auto VisualisationManager = Cast<AAmalgamVisualisationManager>(OutActors[0]);
	if (VisualisationManager == nullptr) return;
	VisualisationManager->ChangeBatch(-100);
}

void APlayerControllerInfernale::CheatMidGameServer_Implementation()
{
	const auto GameInstance = Cast<UGameInstanceInfernale>(GetGameInstance());
	if (GameInstance == nullptr) return;
	const auto GameModeInfernale = Cast<AGameModeInfernale>(GetWorld()->GetAuthGameMode());
	if (!GameModeInfernale) return;

	GameModeInfernale->GetVictoryManagerComponent()->SetGameTimer(60.f * 15.f + 10.f); // 15 minutes + 10 seconds
	if (GameInstance->OpponentPlayerInfo.Num() < 1) return;

	auto OpponentInfo = GameInstance->OpponentPlayerInfo;
	
	
	// 	Ptolomeca
	// Babel
	// Necromanteion
	// El Contrapasso
	// Keep of Asphodel
	auto Player1Info = OpponentInfo[0];
	GameModeInfernale->CheatAddSoulsToPlayer(1, 5500);
	const auto P1PlayerID = GameModeInfernale->EPlayerToInt(Player1Info.PlayerOwnerInfo.Player);
	const auto P1TeamID = GameModeInfernale->ETeamToInt(Player1Info.PlayerOwnerInfo.Team);
	ChangeOwnershipPlayerServer(P1PlayerID, P1TeamID, "Ptolomeca");
	ChangeOwnershipPlayerServer(P1PlayerID, P1TeamID, "Babel");
	ChangeOwnershipPlayerServer(P1PlayerID, P1TeamID, "Necromanteion");
	ChangeOwnershipPlayerServer(P1PlayerID, P1TeamID, "El Contrapasso");
	ChangeOwnershipPlayerServer(P1PlayerID, P1TeamID, "Keep of Asphodel");
	AddVictoryPointsTeamServer_Implementation(P1TeamID, 240); // Add 1000 VP to the team of Player 1
	
	// Gomorrah
	// Sadom
	// Antenora
	// Caina
	// Walls of Tartarus

	if (OpponentInfo.Num() < 2) return;
	auto Player2Info = OpponentInfo[1];
	GameModeInfernale->CheatAddSoulsToPlayer(2, 5500);
	const auto P2PlayerID = GameModeInfernale->EPlayerToInt(Player2Info.PlayerOwnerInfo.Player);
	const auto P2TeamID = GameModeInfernale->ETeamToInt(Player2Info.PlayerOwnerInfo.Team);
	ChangeOwnershipPlayerServer(P2PlayerID, P2TeamID, "Gomorrah");
	ChangeOwnershipPlayerServer(P2PlayerID, P2TeamID, "Sadom");
	ChangeOwnershipPlayerServer(P2PlayerID, P2TeamID, "Antenora");
	ChangeOwnershipPlayerServer(P2PlayerID, P2TeamID, "Caina");
	ChangeOwnershipPlayerServer(P2PlayerID, P2TeamID, "Walls of Tartarus");
	AddVictoryPointsTeamServer_Implementation(P2TeamID, 240); // Add 1000 VP to the team of Player 2
	
	// Judecca
	// Malebolge
	// Acheron
	// Phlegeton
	// City of Disse
	if (OpponentInfo.Num() < 3) return;
	auto Player3Info = OpponentInfo[2];
	GameModeInfernale->CheatAddSoulsToPlayer(3, 5500);
	const auto P3PlayerID = GameModeInfernale->EPlayerToInt(Player3Info.PlayerOwnerInfo.Player);
	const auto P3TeamID = GameModeInfernale->ETeamToInt(Player3Info.PlayerOwnerInfo.Team);
	ChangeOwnershipPlayerServer(P3PlayerID, P3TeamID, "Judecca");
	ChangeOwnershipPlayerServer(P3PlayerID, P3TeamID, "Malebolge");
	ChangeOwnershipPlayerServer(P3PlayerID, P3TeamID, "Acheron");
	ChangeOwnershipPlayerServer(P3PlayerID, P3TeamID, "Phlegeton");
	ChangeOwnershipPlayerServer(P3PlayerID, P3TeamID, "City of Disse");
	AddVictoryPointsTeamServer_Implementation(P3TeamID, 240); // Add 1000 VP to the team of Player 3
	
	// Lete
	// Malacoda
	// Cocytus
	// Styx
	// Fort of Elysium
	if (OpponentInfo.Num() < 4) return;
	auto Player4Info = OpponentInfo[3];
	GameModeInfernale->CheatAddSoulsToPlayer(4, 5500);
	const auto P4PlayerID = GameModeInfernale->EPlayerToInt(Player4Info.PlayerOwnerInfo.Player);
	const auto P4TeamID = GameModeInfernale->ETeamToInt(Player4Info.PlayerOwnerInfo.Team);
	ChangeOwnershipPlayerServer(P4PlayerID, P4TeamID, "Lete");
	ChangeOwnershipPlayerServer(P4PlayerID, P4TeamID, "Malacoda");
	ChangeOwnershipPlayerServer(P4PlayerID, P4TeamID, "Cocytus");
	ChangeOwnershipPlayerServer(P4PlayerID, P4TeamID, "Styx");
	ChangeOwnershipPlayerServer(P4PlayerID, P4TeamID, "Fort of Elysium");
	AddVictoryPointsTeamServer_Implementation(P4TeamID, 240); // Add 1000 VP to the team of Player 4
	
}

void APlayerControllerInfernale::UpdateUnitsSelf(const TArray<FDataForVisualisation>& UnitsToReplicate,
                                                 const TArray<FMassEntityHandle>& EntitiesToHide)
{
	AmalgamVisualisationManager->UpdatePositionOfSpecificUnits(UnitsToReplicate, EntitiesToHide);
}

void APlayerControllerInfernale::UpdateUnitsOwner_Implementation(const TArray<FDataForVisualisation>& UnitsToReplicate, const TArray<FMassEntityHandle>& EntitiesToHide)
{
	AmalgamVisualisationManager->UpdatePositionOfSpecificUnits(UnitsToReplicate, EntitiesToHide);
}

void APlayerControllerInfernale::UpdateUnitsInfoOwner_Implementation(
	const TArray<FDataForVisualisation>& UnitsToReplicate)
{
	AmalgamVisualisationManager->UpdateUnitsInfo(UnitsToReplicate);
}

void APlayerControllerInfernale::UpdateUnitsToHideOwner_Implementation(
	const TArray<FMassEntityHandle>& EntitiesToHide)
{
	AmalgamVisualisationManager->UpdateUnitsToHide(EntitiesToHide);
}

void APlayerControllerInfernale::DoVFXOwner_Implementation(const FBattleInfo BattleInfo)
{
	if (bIsTacticalModeActive) return;
	UnitActorManager->GetBattleManagerComponent()->DoVFXBP(BattleInfo);
}

void APlayerControllerInfernale::DoDeathVFXOwner_Implementation(const FDeathInfo DeathInfo)
{
	if (bIsTacticalModeActive) return;
	UnitActorManager->GetBattleManagerComponent()->DoDeathVFXBP(DeathInfo);
}

void APlayerControllerInfernale::SyncGameSettings()
{
	if (!GameSettingsDataAsset) return;

	bDebugSplines = GameSettingsDataAsset->ShowSplines;
	bUseFluxMode = GameSettingsDataAsset->UseFluxMode;
}

void APlayerControllerInfernale::RemoveLodingScreenAfterDelay()
{
	UIComponent->RemoveLoadingScreen(true);
}

void APlayerControllerInfernale::SetBuildingPermanentServer_Implementation(ABuilding* Building, bool IsPermanent)
{
	BuildComponent->SetPermanentBuilding(Building, IsPermanent);
}

void APlayerControllerInfernale::ReplicateCenterPointServerUnreliable_Implementation(FVector NewCenterPoint)
{
	if (InfernalePawn == nullptr) return;
	InfernalePawn->SetCenterPoint(NewCenterPoint);
}

void APlayerControllerInfernale::BuildBuildingServer_Implementation(ABreach* Breach, FBuildingStruct Building)
{
	BuildBuilding.Broadcast(Breach, Building);
}

void APlayerControllerInfernale::CameraProfilesActionTriggered(const FInputActionValue& Value)
{
	const auto CameraProfileVector = Value.Get<FVector2D>();
	CameraProfileTriggered.Broadcast(CameraProfileVector);
}

void APlayerControllerInfernale::CameraProfilesActionCompleted(const FInputActionValue& Value)
{
	const auto CameraProfileVector = Value.Get<FVector2D>();
	CameraProfileCompleted.Broadcast(CameraProfileVector);
}

void APlayerControllerInfernale::DotDotActionStarted()
{
	const auto PS = InfernalePawn->GetPlayerState();
	if (!PlayerState) return;
	const auto PlayerStateInfernale = Cast<APlayerStateInfernale>(PlayerState);
	if (!PlayerStateInfernale) return;
	if (!AllowedToCheat) return;
	PlayerStateInfernale->GetEconomyComponent()->GainDebugIncome();
	
}

void APlayerControllerInfernale::IncreaseBatch()
{
	IncreaseBatchServer();
}

void APlayerControllerInfernale::DecreaseBatch()
{
	DecreaseBatchServer();
}

void APlayerControllerInfernale::EnableLANActionStarted()
{
	const auto GameInstance = Cast<UGameInstanceInfernale>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (GameInstance == nullptr) return;
	const auto Current = GameInstance->UseLAN();
	GameInstance->SetUseLAN(!Current);

	GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, !Current ? TEXT("Using LAN") : TEXT("Not Using LAN"));
}

void APlayerControllerInfernale::KeyboardRotationActionTriggered(const FInputActionValue& InputActionValue)
{
	InfernalePawn->KeyboardRotationActionTriggered(InputActionValue);
}

void APlayerControllerInfernale::OnResetRotationStarted()
{
	InfernalePawn->ResetRotation();
}

void APlayerControllerInfernale::OnSpacePressed()
{
	OnSpacePressedBP();
	bIsTacticalModeActive = !bIsTacticalModeActive;
	AmalgamVisualisationManager->ShowHideAllItems(bIsTacticalModeActive);
}

void APlayerControllerInfernale::OnSpaceReleased()
{
	OnSpaceReleasedBP();
	if (UFunctionLibraryInfernale::GetGameSettingsDataAsset()->bUseHoldSpaceMode)
	{
		bIsTacticalModeActive = false;
	}
	AmalgamVisualisationManager->ShowHideAllItems(bIsTacticalModeActive);
}

void APlayerControllerInfernale::OnSpacePressedBP_Implementation()
{
}


void APlayerControllerInfernale::OnSpaceReleasedBP_Implementation()
{
}

void APlayerControllerInfernale::OnEscapeStartedBP_Implementation()
{
}

void APlayerControllerInfernale::UnsubscribeClickEvents()
{
	bClickEventsAllowed = false;
}

void APlayerControllerInfernale::SubscribeClickEvents()
{
	bClickEventsAllowed = true;
}

void APlayerControllerInfernale::OnMyTurnChanged(bool bMyTurn)
{
	bIsMyTurn = bMyTurn;
}

void APlayerControllerInfernale::OnDraftEnded()
{
}

void APlayerControllerInfernale::SetDraftEnded()
{
	DraftEnded.Broadcast();
}

void APlayerControllerInfernale::SetMyTurn(bool bMyTurn)
{
	MyTurnChanged.Broadcast(bMyTurn);
}

void APlayerControllerInfernale::AddVictoryPointsServer_Implementation(float Amount)
{
	const auto GameModeInfernale = Cast<AGameModeInfernale>(GetWorld()->GetAuthGameMode());
	if (!GameModeInfernale) return;

	GameModeInfernale->GetVictoryManagerComponent()->CheatAddVictoryPoints(GetTeam(), Amount, EVictoryPointReason::VictoryPointCheat);
}

void APlayerControllerInfernale::AddVictoryPointsTeamServer_Implementation(int TeamID, float Amount)
{
	const auto GameModeInfernale = Cast<AGameModeInfernale>(GetWorld()->GetAuthGameMode());
	if (!GameModeInfernale) return;

	const auto Team = GameModeInfernale->IntToETeam(TeamID);
	if (Team == ETeam::NatureTeam) return;

	GameModeInfernale->GetVictoryManagerComponent()->CheatAddVictoryPoints(Team, Amount, EVictoryPointReason::VictoryPointCheat);
}

void APlayerControllerInfernale::ChangeOwnershipServer_Implementation(const FString& BaseName)
{
	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMainBuilding::StaticClass(), OutActors);
	if (OutActors.Num() <= 0) return;
	for (const auto Actor : OutActors)
	{
		const auto MainBuilding = Cast<AMainBuilding>(Actor);
		if (MainBuilding == nullptr) continue;
		if (!MainBuilding->GetBuildingName().Equals(BaseName)) continue;

		MainBuilding->CheatCapture(GetPlayerState<APlayerStateInfernale>()->GetOwnerInfo());
		break;
	}
}

void APlayerControllerInfernale::ChangeOwnershipPlayerServer_Implementation(int PlayerID, int TeamID, const FString& BaseName)
{
	AGameModeInfernale* GameModeInfernale = Cast<AGameModeInfernale>(GetWorld()->GetAuthGameMode());
	if (!GameModeInfernale) return;

	const auto EPlayer = GameModeInfernale->IntToEPlayer(PlayerID);
	const auto ETeam = GameModeInfernale->IntToETeam(TeamID);
	FOwner NewOwner;
	NewOwner.Player = EPlayer;
	NewOwner.Team = ETeam;

	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMainBuilding::StaticClass(), OutActors);
	if (OutActors.Num() <= 0) return;
	for (const auto Actor : OutActors)
	{
		const auto MainBuilding = Cast<AMainBuilding>(Actor);
		if (MainBuilding == nullptr) continue;
		if (!MainBuilding->GetBuildingName().Equals(BaseName)) continue;

		MainBuilding->CheatCapture(NewOwner);
		break;
	}
}

void APlayerControllerInfernale::EndGameServer_Implementation()
{
	const auto GameModeInfernale = Cast<AGameModeInfernale>(GetWorld()->GetAuthGameMode());
	if (!GameModeInfernale) return;
	GameModeInfernale->GetVictoryManagerComponent()->CheatEndGame();
}

void APlayerControllerInfernale::StartNewGameServer_Implementation(float NewTimeLeft)
{
	const auto GameModeInfernale = Cast<AGameModeInfernale>(GetWorld()->GetAuthGameMode());
	if (!GameModeInfernale) return;
	GameModeInfernale->StartNewGame(NewTimeLeft);
}

void APlayerControllerInfernale::AddSoulsPlayerServer_Implementation(int PlayerID, float Amount)
{
	const auto GameModeInfernale = Cast<AGameModeInfernale>(GetWorld()->GetAuthGameMode());
	if (!GameModeInfernale) return;
	GameModeInfernale->CheatAddSoulsToPlayer(PlayerID, Amount);
}

void APlayerControllerInfernale::AddSoulsServer_Implementation(float Amount)
{
	if (!PlayerState) return;
	const auto PlayerStateInfernale = Cast<APlayerStateInfernale>(PlayerState);
	if (!PlayerStateInfernale) return;
	PlayerStateInfernale->GetEconomyComponent()->GainCheatIncome(Amount);
}

void APlayerControllerInfernale::ChangeBaseOwnerServer_Implementation(AActor* ActorInteractable, ETeam NewOwnerTeam, EPlayerOwning NewOwnerPlayer)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("APlayerControllerInfernale::ChangeBaseOwnerServer_Implementation"));
	AMainBuilding* MainBuilding = Cast<AMainBuilding>(ActorInteractable);
	if (!MainBuilding)
	{
		if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("APlayerControllerInfernale::ChangeBaseOwner_Implementation > Interactable is not MainBuilding"));
		return;
	}
	FOwner NewOwner = FOwner();
	NewOwner.Player = NewOwnerPlayer; NewOwner.Team = NewOwnerTeam;
		
	const auto OldOwner = MainBuilding->GetOwner();
	if (OldOwner.Player != NewOwnerPlayer)
	{
		if (OldOwner.Team != NewOwnerTeam)
		{
			MainBuilding->ChangeOwner(NewOwner);
			auto CameraProfileLocation = MainBuilding->GetActorLocation() + FVector(-5000, 0, 20000);
			auto CameraProfileRotation = FRotator(0, 0, 0);
			InfernalePawn->AddOverrideCameraProfileIndexPosition(0, CameraProfileLocation, CameraProfileRotation);
		}
	}
}

void APlayerControllerInfernale::StartTurnOwning_Implementation()
{
	SetMyTurn(true);
}

void APlayerControllerInfernale::EndTurnOwning_Implementation()
{
	SetMyTurn(false);
}

void APlayerControllerInfernale::EndDraftOwning_Implementation()
{
	SetDraftEnded();
}

FPlayerInfo APlayerControllerInfernale::GetPlayerInfo() const
{
	return PlayerInfo;
}

void APlayerControllerInfernale::SetPlayerInfo(const FPlayerInfo NewPlayerInfo)
{
	PlayerInfo = NewPlayerInfo;
}

void APlayerControllerInfernale::MainBuildingInteracted(AMainBuilding* MainBuilding)
{
	MainBuildingInteractedDelegate.Broadcast(MainBuilding);
}

bool APlayerControllerInfernale::ShouldReplicateUnitToHide() const
{
	return ReplicateUnitToHide;
}

int APlayerControllerInfernale::GetFramesSinceLastReplicate() const
{
	return FramesSinceLastReplicate;
}

bool APlayerControllerInfernale::ShouldReplicateUnitToHideThisFrame() const
{
	return ReplicateUnitToHide || FramesSinceLastReplicate < 1;
}

void APlayerControllerInfernale::IG_AddSouls(float Amount)
{
	if (!AllowedToCheat) return;
	AddSoulsServer(Amount);
}

void APlayerControllerInfernale::IG_AddSoulsPlayer(int PlayerID, float Amount)
{
	if (!AllowedToCheat) return;
	AddSoulsPlayerServer(PlayerID, Amount);
}

void APlayerControllerInfernale::IG_AddVictoryPoints(float Amount)
{
	if (!AllowedToCheat) return;
	AddVictoryPointsServer(Amount);
}

void APlayerControllerInfernale::IG_AddVictoryPointsTeam(int TeamID, float Amount)
{
	if (!AllowedToCheat) return;
	AddVictoryPointsTeamServer(TeamID, Amount);
}

void APlayerControllerInfernale::IG_ChangeOwnership(FString BaseName)
{
	if (!AllowedToCheat) return;
	ChangeOwnershipServer(BaseName);
}

void APlayerControllerInfernale::IG_ChangeOwnershipPlayer(int PlayerID, int TeamID, const FString& BaseName)
{
	if (!AllowedToCheat) return;
	ChangeOwnershipPlayerServer(PlayerID, TeamID, BaseName);
}

void APlayerControllerInfernale::IG_EndGame()
{
	if (!AllowedToCheat) return;
	EndGameServer();
}

void APlayerControllerInfernale::IG_StartNewGame(float NewTimeLeft)
{
	if (!AllowedToCheat) return;
	StartNewGameServer(NewTimeLeft);
}

void APlayerControllerInfernale::IG_StartNewGameFull()
{
	if (!AllowedToCheat) return;
	StartNewGameServer(60.f * 25.f + 10.f); /* 25 minutes */
}

void APlayerControllerInfernale::IG_CheatMidGame()
{
	if (!AllowedToCheat) return;
	CheatMidGameServer();
}

void APlayerControllerInfernale::Secret()
{
	AllowedToCheat = true;
}

void APlayerControllerInfernale::UnSecret()
{
	AllowedToCheat = false;
}
