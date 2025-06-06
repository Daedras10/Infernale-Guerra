// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/GRInfernalePawn.h"

#include <string>

#include "Camera/CameraComponent.h"
#include "GameMode/GrayRoom/GRPlayerControllerInfernale.h"

// Sets default values
AGRInfernalePawn::AGRInfernalePawn()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(RootComponent);

	ShouldRotate = false;
	CameraHeightSpeed = 200;
	MoveSpeed = FVector2D(500, 500);
	ScreenEdgePadding = FVector2D(50, 50);
	RotationSpeed = 50;
	
}

// Called when the game starts or when spawned
void AGRInfernalePawn::BeginPlay()
{
	Super::BeginPlay();
	HeightTarget = CameraDefaultHeight;
	SetActorRotation(FRotator::MakeFromEuler(FVector3d(CameraDefaultAngle.X, CameraDefaultAngle.Y, 0)));
	if (PlayerController = Cast<APlayerController>(Controller); IsValid(PlayerController))
	{
		PlayerController->SetShowMouseCursor(true);
		PlayerController->bEnableClickEvents = true;
		PlayerController->bEnableMouseOverEvents = true;
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PlayerController is not valid"));
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Stopping the game"));
		GetWorld()->GetFirstPlayerController()->ConsoleCommand("quit");
	}

	SubscribeToInputEvents();
}

// Called every frame
void AGRInfernalePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	auto CurrentMoveSpeed = CurrentInputMoveSpeed;
	
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("CurrentMoveSpeed: %s"), *CurrentMoveSpeed.ToString()));
	if (PlayerController == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PlayerController is not valid"));
		return;
	}
	if (FVector2d MousePosition; CurrentInputMoveSpeed.X == 0 && CurrentInputMoveSpeed.Y == 0 && PlayerController->GetMousePosition(MousePosition.X, MousePosition.Y))
	{
		int32 ViewPortWidth, ViewPortHeight;
		PlayerController->GetViewportSize(ViewPortWidth, ViewPortHeight);
		
		const auto X = -(MousePosition.X <= ScreenEdgePadding.X) | (MousePosition.X >= ViewPortWidth - ScreenEdgePadding.X);
		
		const auto Y = -(MousePosition.Y <= ScreenEdgePadding.Y) | (MousePosition.Y >= ViewPortHeight - ScreenEdgePadding.Y);

		CurrentMoveSpeed = FVector2D(X, -Y);
	}

	//Lerp the camera height
	auto Location = GetActorLocation();
	Location.Z = FMath::Lerp(Location.Z, HeightTarget, LerpSpeed * DeltaTime);
	FRotator CurrentRotation = GetActorRotation();
	PitchTarget = CameraPitchCurve->FloatCurve.Eval((HeightTarget - CameraHeightLimits.X) / (CameraHeightLimits.Y - CameraHeightLimits.X));
	CurrentRotation.Pitch = FMath::Lerp(CurrentRotation.Pitch, PitchTarget, LerpSpeed * DeltaTime);
	CurrentRotation.Roll = 0;
	SetActorRotation(CurrentRotation);
	
	//Move the camera
	auto ForwardVector = FVector(GetActorForwardVector().X, GetActorForwardVector().Y, 0);
	ForwardVector.Normalize();

	const auto Forward = ForwardVector * CurrentMoveSpeed.Y * MoveSpeed.Y * DeltaTime;
	const auto Sideways = GetActorRightVector() * CurrentMoveSpeed.X * MoveSpeed.X * DeltaTime;

	auto NextLocation = GetActorLocation() + Forward + Sideways;
	if (Location.Z > CameraHeightLimits.X && Location.Z < CameraHeightLimits.Y)
	{
		NextLocation.Z = Location.Z;
	}
	if (Location.Z < CameraHeightLimits.X)
	{
		Location.Z = CameraHeightLimits.X;
	}
	else if (Location.Z > CameraHeightLimits.Y)
	{
		Location.Z = CameraHeightLimits.Y;
	}
	else if (Location.Z < 0)
	{
		Location.Z = 500;
	}
	SetActorLocation(FMath::Lerp(GetActorLocation(), NextLocation, LerpSpeed * DeltaTime));
}

// Called to bind functionality to input
void AGRInfernalePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	auto* LocalController = GetLocalViewingPlayerController();
	if (LocalController == nullptr) return;

	PlayerControllerInfernale = Cast<AGRPlayerControllerInfernale>(LocalController);
	if (PlayerControllerInfernale == nullptr) return;

	PlayerControllerInfernale->SetPlayerInputComponent(PlayerInputComponent);
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("SetupPlayerInputComponent"));
}

void AGRInfernalePawn::MoveTriggered(const FVector2D Value)
{
	CurrentInputMoveSpeed = Value;
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("MoveTriggered: %s"), *Value.ToString()));
}

void AGRInfernalePawn::MoveCompleted(const FVector2D _)
{
	CurrentInputMoveSpeed = FVector2D::ZeroVector;
}

void AGRInfernalePawn::CameraHeightTriggered(const float Value)
{
	auto Location = GetActorLocation();
	Location.Z += Value * CameraHeightSpeed * FApp::GetDeltaTime();
	HeightTarget = Location.Z;
}

void AGRInfernalePawn::RotationStarted()
{
	ShouldRotate = true;
	//shoot a raycast from the middle of the screen to the ground to get the center of interest
}

void AGRInfernalePawn::RotationCompleted()
{
	ShouldRotate = false;
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("RotationCompleted"));
}

void AGRInfernalePawn::LookTriggered(const FVector2D Value)
{
	//const auto& LookVector = InValue.Get<FVector2D>();

	if (ShouldRotate)
	{
		FRotator CurrentRotation = GetActorRotation();
		FVector WorldLocation, WorldDirection;
		float ScreenHeight = GEngine->GameViewport->Viewport->GetSizeXY().Y;
		float ScreenWidth = GEngine->GameViewport->Viewport->GetSizeXY().X;
		PlayerControllerInfernale->DeprojectScreenPositionToWorld(ScreenWidth / 2, ScreenHeight / 2, WorldLocation, WorldDirection);
		FHitResult HitResult;
		GetWorld()->LineTraceSingleByChannel(HitResult, WorldLocation, WorldLocation + WorldDirection * 100000, ECollisionChannel::ECC_Visibility);
		if (HitResult.bBlockingHit && CurrentRotation.Pitch < -6)
		{
			RotateOnSelf = false;
			CameraCenterOfInterest = HitResult.ImpactPoint;
			DrawDebugSphere(GetWorld(), CameraCenterOfInterest, 50, 12, FColor::Red, false, 0.1f);
		}
		else
		{
			RotateOnSelf = true;
		}
		
		if (RotateOnSelf)
		{
			CurrentRotation.Yaw += RotationSpeed * FApp::GetDeltaTime() * Value.X;
			// CurrentRotation.Pitch = FMath::ClampAngle(CurrentRotation.Pitch, CameraAnglePitchLimits.X, CameraAnglePitchLimits.Y);
			SetActorRotation(CurrentRotation);
		}
		else
		{
			auto CurrentPitch = GetActorRotation().Pitch;
			// FVector RotationCenter = CalculateCenterOfInterest(GetActorLocation(), FMath::DegreesToRadians(CurrentPitch));
    
			// Debug the rotation center
			DrawDebugSphere(GetWorld(), CameraCenterOfInterest, 50, 12, FColor::Red, false, 0.1f);
			
			FVector RotationCenter = CameraCenterOfInterest;
			FRotator NewRotation = GetActorRotation();
			NewRotation.Yaw += RotationSpeed * FApp::GetDeltaTime() * Value.X;
			FVector Direction = GetActorLocation() - RotationCenter;
			Direction = Direction.RotateAngleAxis(RotationSpeed * FApp::GetDeltaTime() * Value.X, FVector(0, 0, 1));
			FVector NewLocation = RotationCenter + Direction;
			SetActorLocation(NewLocation);
			SetActorRotation(NewRotation);
		}
	}
	if (ShouldMove)
	{
		// Move the screen in the opposite direction of the mouse movement (Value)
		FVector NewLocation = GetActorLocation();
		NewLocation -= GetActorForwardVector() * Value.Y * MoveSpeed.X * FApp::GetDeltaTime();
		NewLocation -= GetActorRightVector() * Value.X * MoveSpeed.Y * FApp::GetDeltaTime();
		SetActorLocation(NewLocation);
	}
}

void AGRInfernalePawn::AddOrOverrideCameraPositionAtIndex(int32 Index)
{
	CameraSavedPositions.Insert(FCameraPosition(GetActorLocation(), GetActorRotation()), Index);
}

void AGRInfernalePawn::SetCameraPositionAtIndex(int32 Index)
{
	SetActorLocation(CameraSavedPositions[Index].Position);
	SetActorRotation(CameraSavedPositions[Index].Rotation);
}

void AGRInfernalePawn::MoveStarted()
{
	ShouldMove = true;
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("MoveStarted"));
}

void AGRInfernalePawn::SecondaryMoveCompleted()
{
	ShouldMove = false;
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("SecondaryMoveCompleted"));
}

void AGRInfernalePawn::SubscribeToInputEvents()
{
	if (PlayerControllerInfernale == nullptr) return;

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Subscribed to Input Events"));
	
	PlayerControllerInfernale->MoveTriggered.AddDynamic(this, &AGRInfernalePawn::MoveTriggered);
	PlayerControllerInfernale->MoveCompleted.AddDynamic(this, &AGRInfernalePawn::MoveCompleted);
	PlayerControllerInfernale->EnableMoveActionTriggered.AddDynamic(this, &AGRInfernalePawn::MoveStarted);
	PlayerControllerInfernale->EnableMoveActionCompleted.AddDynamic(this, &AGRInfernalePawn::SecondaryMoveCompleted);
	PlayerControllerInfernale->LookTriggered.AddDynamic(this, &AGRInfernalePawn::LookTriggered);
	PlayerControllerInfernale->ScrollTriggered.AddDynamic(this, &AGRInfernalePawn::CameraHeightTriggered);
	PlayerControllerInfernale->EnableRotationStarted.AddDynamic(this, &AGRInfernalePawn::RotationStarted);
	PlayerControllerInfernale->EnableRotationCompleted.AddDynamic(this, &AGRInfernalePawn::RotationCompleted);
}

void AGRInfernalePawn::UnsubscribeFromInputEvents()
{
	if (PlayerControllerInfernale == nullptr) return;
	
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Unsubscribe from Input Events"));
	
	PlayerControllerInfernale->MoveTriggered.RemoveDynamic(this, &AGRInfernalePawn::MoveTriggered);
	PlayerControllerInfernale->MoveCompleted.RemoveDynamic(this, &AGRInfernalePawn::MoveCompleted);
	PlayerControllerInfernale->EnableMoveActionTriggered.RemoveDynamic(this, &AGRInfernalePawn::MoveStarted);
	PlayerControllerInfernale->EnableMoveActionCompleted.RemoveDynamic(this, &AGRInfernalePawn::SecondaryMoveCompleted);
	PlayerControllerInfernale->LookTriggered.RemoveDynamic(this, &AGRInfernalePawn::LookTriggered);
	PlayerControllerInfernale->ScrollTriggered.RemoveDynamic(this, &AGRInfernalePawn::CameraHeightTriggered);
	PlayerControllerInfernale->EnableRotationStarted.RemoveDynamic(this, &AGRInfernalePawn::RotationStarted);
	PlayerControllerInfernale->EnableRotationCompleted.RemoveDynamic(this, &AGRInfernalePawn::RotationCompleted);
}

void AGRInfernalePawn::SubscribeToLookEvents()
{
	if (PlayerControllerInfernale == nullptr) return;
	
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Subscribe to Look Events"));
	PlayerControllerInfernale->LookTriggered.AddDynamic(this, &AGRInfernalePawn::LookTriggered);
}

void AGRInfernalePawn::UnsubscribeFromLookEvents()
{
	if (PlayerControllerInfernale == nullptr) return;

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Unsubscribe from Look Events"));
	PlayerControllerInfernale->LookTriggered.RemoveDynamic(this, &AGRInfernalePawn::LookTriggered);
}