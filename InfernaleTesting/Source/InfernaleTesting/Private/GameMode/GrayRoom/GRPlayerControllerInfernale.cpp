// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/GrayRoom/GRPlayerControllerInfernale.h"
#include "DataAsset/GRGameInputDataAsset.h"
#include "EnhancedInputComponent.h"
#include "Player/GRInfernalePawn.h"


void AGRPlayerControllerInfernale::BindInputs()
{
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (Input == nullptr) return;

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("BindInputs"));

	// IA_Move
	Input->BindAction(GameInputDataAsset->MoveAction, ETriggerEvent::Triggered, this, &AGRPlayerControllerInfernale::MoveActionTriggered);
	Input->BindAction(GameInputDataAsset->MoveAction, ETriggerEvent::Completed, this, &AGRPlayerControllerInfernale::MoveActionCompleted);

	// IA_GREnableMove
	Input->BindAction(GameInputDataAsset->EnableMoveAction, ETriggerEvent::Triggered, this, &AGRPlayerControllerInfernale::EnableMoveActionTriggeredFunc);
	Input->BindAction(GameInputDataAsset->EnableMoveAction, ETriggerEvent::Canceled, this, &AGRPlayerControllerInfernale::EnableMoveActionCompletedFunc);
	Input->BindAction(GameInputDataAsset->EnableMoveAction, ETriggerEvent::Completed, this, &AGRPlayerControllerInfernale::EnableMoveActionCompletedFunc);

	// IA_Look
	Input->BindAction(GameInputDataAsset->LookAction, ETriggerEvent::Triggered, this, &AGRPlayerControllerInfernale::LookActionTriggered);

	// IA_CameraHeight
	Input->BindAction(GameInputDataAsset->CameraHeightAction, ETriggerEvent::Triggered, this, &AGRPlayerControllerInfernale::CameraHeightActionTriggered);

	// IA_EnableRotation
	Input->BindAction(GameInputDataAsset->EnableRotationAction, ETriggerEvent::Started, this, &AGRPlayerControllerInfernale::EnableRotationActionStarted);
	Input->BindAction(GameInputDataAsset->EnableRotationAction, ETriggerEvent::Canceled, this, &AGRPlayerControllerInfernale::EnableRotationActionCompleted);
	Input->BindAction(GameInputDataAsset->EnableRotationAction, ETriggerEvent::Completed, this, &AGRPlayerControllerInfernale::EnableRotationActionCompleted);

	// IA_PrimaryAction
	Input->BindAction(GameInputDataAsset->PrimaryAction, ETriggerEvent::Started, this, &AGRPlayerControllerInfernale::PrimaryActionStarted);
	Input->BindAction(GameInputDataAsset->PrimaryAction, ETriggerEvent::Completed, this, &AGRPlayerControllerInfernale::PrimaryActionEnded);
	Input->BindAction(GameInputDataAsset->PrimaryAction, ETriggerEvent::Canceled, this, &AGRPlayerControllerInfernale::PrimaryActionEnded);
	Input->BindAction(GameInputDataAsset->PrimaryAction, ETriggerEvent::Triggered, this, &AGRPlayerControllerInfernale::PrimaryActionTriggered);

	// IA_SecondaryAction
	Input->BindAction(GameInputDataAsset->SecondaryAction, ETriggerEvent::Started, this, &AGRPlayerControllerInfernale::SecondaryActionStarted);
	Input->BindAction(GameInputDataAsset->SecondaryAction, ETriggerEvent::Completed, this, &AGRPlayerControllerInfernale::SecondaryActionEnded);
	Input->BindAction(GameInputDataAsset->SecondaryAction, ETriggerEvent::Canceled, this, &AGRPlayerControllerInfernale::SecondaryActionEnded);
	Input->BindAction(GameInputDataAsset->SecondaryAction, ETriggerEvent::Triggered, this, &AGRPlayerControllerInfernale::SecondaryActionTriggered);

	// IA_FluxMode
	Input->BindAction(GameInputDataAsset->FluxModeAction, ETriggerEvent::Started, this, &AGRPlayerControllerInfernale::FluxModeActionStarted);

	// IA_Escape
	Input->BindAction(GameInputDataAsset->EscapeAction, ETriggerEvent::Started, this, &AGRPlayerControllerInfernale::EscapeActionStarted);
}

void AGRPlayerControllerInfernale::SetPlayerInputComponent(UInputComponent* NewPlayerInputComponent)
{
	PlayerInputComponent = NewPlayerInputComponent;
	BindInputs();
}

void AGRPlayerControllerInfernale::BeginPlay()
{
	Super::BeginPlay();
	SetupInputMappings();

	// Could check if the owner is ready instead
	if (HasAuthority()) PlayerReady.Broadcast();

	InfernalePawn = Cast<AGRInfernalePawn>(GetPawn());
}

void AGRPlayerControllerInfernale::MoveActionTriggered(const FInputActionValue& Value)
{
	const auto Val = Value.Get<FVector2D>();
	MoveTriggered.Broadcast(Val);
}

void AGRPlayerControllerInfernale::MoveActionCompleted(const FInputActionValue& Value)
{
	const auto Val = Value.Get<FVector2D>();
	MoveCompleted.Broadcast(Val);
}

void AGRPlayerControllerInfernale::LookActionTriggered(const FInputActionValue& Value)
{
	const auto Val = Value.Get<FVector2D>();
	LookTriggered.Broadcast(Val);
}

void AGRPlayerControllerInfernale::CameraHeightActionTriggered(const FInputActionValue& Value)
{
	const auto Val = Value.Get<float>();
	ScrollTriggered.Broadcast(Val);
	MouseScroll(Value);
}

void AGRPlayerControllerInfernale::EnableRotationActionStarted(const FInputActionValue& Value)
{
	EnableRotationStarted.Broadcast();
}

void AGRPlayerControllerInfernale::EnableRotationActionCompleted(const FInputActionValue& Value)
{
	EnableRotationCompleted.Broadcast();
}

void AGRPlayerControllerInfernale::PrimaryActionStarted()
{
	if (bPrintDebugInputMouse) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PrimaryActionStarted"));
	MousePrimaryStart.Broadcast();
}

void AGRPlayerControllerInfernale::PrimaryActionEnded()
{
	if (bPrintDebugInputMouse) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PrimaryActionEnded"));
	MousePrimaryEnd.Broadcast();
}

void AGRPlayerControllerInfernale::PrimaryActionTriggered()
{
	MousePrimaryTriggered.Broadcast();
}

void AGRPlayerControllerInfernale::SecondaryActionStarted()
{
	if (bPrintDebugInputMouse) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("SecondaryActionStarted"));
	MouseSecondaryStart.Broadcast();
}

void AGRPlayerControllerInfernale::SecondaryActionEnded()
{
	if (bPrintDebugInputMouse) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("SecondaryActionEnded"));
	MouseSecondaryEnd.Broadcast();
}

void AGRPlayerControllerInfernale::SecondaryActionTriggered()
{
	MouseSecondaryTriggered.Broadcast();
}

void AGRPlayerControllerInfernale::FluxModeActionStarted()
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("FluxModeActionStarted"));
	FluxModeStart.Broadcast();
}

void AGRPlayerControllerInfernale::MouseScroll(const FInputActionValue& Value)
{
	float Val = Value.Get<float>();

	if (!IsScrolling) StartScroll();
	else ResetScrollTimer();

	if (Val > 0) MouseScrollUp.Broadcast();
	else MouseScrollDown.Broadcast();
}

void AGRPlayerControllerInfernale::EscapeActionStarted()
{
	EscapeStarted.Broadcast();
}

void AGRPlayerControllerInfernale::StartScroll()
{
	IsScrolling = true;
	MouseScrollStarted.Broadcast();

	ResetScrollTimer();
}

void AGRPlayerControllerInfernale::StopScroll()
{
	IsScrolling = false;
	MouseScrollEnded.Broadcast();
}

void AGRPlayerControllerInfernale::ResetScrollTimer()
{
	ScrollTimerHandle.Invalidate();
	GetWorld()->GetTimerManager().SetTimer(ScrollTimerHandle, this, &AGRPlayerControllerInfernale::StopScroll, TimeBeforeScrollEnd, false);
}

void AGRPlayerControllerInfernale::SetupInputMappings()
{
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	
	if (Subsystem == nullptr) return;
	if (GameInputMappingContext == nullptr) return;

	Subsystem->ClearAllMappings();
	Subsystem->AddMappingContext(GameInputMappingContext, 0);

	//SetInputMode(FInputModeGameAndUI());
	//SetShowMouseCursor(true);
}

void AGRPlayerControllerInfernale::EnableMoveActionTriggeredFunc()
{
	EnableMoveActionTriggered.Broadcast();
}

void AGRPlayerControllerInfernale::EnableMoveActionCompletedFunc()
{
	EnableMoveActionCompleted.Broadcast();
}
