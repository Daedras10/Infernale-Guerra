// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/InfernalePawn.h"

#include "VectorTypes.h"
#include "Camera/CameraComponent.h"
#include "Component/PlayerController/InteractionComponent.h"
#include "DataAsset/GameSettingsDataAsset.h"
#include "DataAsset/InfernalePawnDataAsset.h"
#include "FunctionLibraries/FunctionLibraryInfernale.h"
#include "GameMode/GameInstanceInfernale.h"
#include "GameMode/Infernale/PlayerControllerInfernale.h"

// Sets default values
AInfernalePawn::AInfernalePawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(RootComponent);

	ShouldRotate = false;
	CameraHeightSpeed = 200;
	ScreenEdgePadding = FVector2D(50, 50);
	RotationSpeed = 50;
	CurrentZoomFocusDelay = 0;
	CameraSavedPositions.Init(FVector::ZeroVector, 4);
	CameraSavedRotations.Init(FRotator::ZeroRotator, 4);

	SyncFromDataAsset();
}

void AInfernalePawn::SetCanMoveOwning_Implementation(bool Value)
{
	CanMove = Value;
}

// Called when the game starts or when spawned
void AInfernalePawn::BeginPlay()
{
	Super::BeginPlay();
	SyncFromDataAsset();
	SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, CameraDefaultHeight));
	SetActorRotation(FRotator::MakeFromEuler(FVector3d(CameraDefaultAngle.X, CameraDefaultAngle.Y, 0)));
	CameraCollsionChannel = UFunctionLibraryInfernale::GetCustomTraceChannel(CameraCollision);
	// else
	// {
	// 	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PlayerController is not valid"));
	// 	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Stopping the game"));
	// 	GetWorld()->GetFirstPlayerController()->ConsoleCommand("quit");
	// }
	PawnSetAtSpawn.AddDynamic(this, &AInfernalePawn::OnPawnSetAtSpawn);
	PawnSpawnTravelFinished.AddDynamic(this, &AInfernalePawn::OnPawnSpawnTravelFinished);
}

void AInfernalePawn::SyncFromDataAsset()
{
	if (!bUseDataAsset || !InfernalePawnDataAsset) return;

	CameraHeightSpeed = InfernalePawnDataAsset->CameraHeightSpeed;
	CameraDefaultHeight = InfernalePawnDataAsset->CameraDefaultHeight;
	CameraDefaultAngle = InfernalePawnDataAsset->CameraDefaultAngle;
	CameraHeightLimits = InfernalePawnDataAsset->CameraHeightLimits;
	CameraXYLimits = InfernalePawnDataAsset->CameraXYLimits;
	CameraAnglePitchLimits = InfernalePawnDataAsset->CameraAnglePitchLimits;
	ScreenEdgePadding = InfernalePawnDataAsset->ScreenEdgePadding;
	MoveSpeed = InfernalePawnDataAsset->MoveSpeed;
	RotationSpeed = InfernalePawnDataAsset->RotationSpeed;
	LerpSpeed = InfernalePawnDataAsset->LerpSpeed;
	CameraPitchCurve = InfernalePawnDataAsset->CameraPitchCurve;
	CameraHeightMoveSpeedCurve = InfernalePawnDataAsset->CameraHeightMoveSpeedCurve;
	CameraCurveSpeed = InfernalePawnDataAsset->CameraTravelingSpeed;
	RotateOnSelf = InfernalePawnDataAsset->RotateOnSelf;
	RotationCenterOffset = InfernalePawnDataAsset->RotationCenterOffset;
	ZoomFocusDelay = InfernalePawnDataAsset->ZoomFocusDelay;
}

void AInfernalePawn::MoveTriggered(const FVector2D Value)
{
	float speedCurveValue = CameraHeightMoveSpeedCurve->FloatCurve.Eval((GetActorLocation().Z - CameraHeightLimits.X) / (CameraHeightLimits.Y - CameraHeightLimits.X));
	CurrentInputMoveSpeed = Value.GetSafeNormal() * speedCurveValue * CanMove;
	bisMoving = true;
}

void AInfernalePawn::MoveCompleted(const FVector2D _)
{
	CurrentInputMoveSpeed = FVector2D::ZeroVector;
}

void AInfernalePawn::CameraHeightTriggered(const float Value)
{
	FVector Location = GetActorLocation();
	FVector WorldLocation, WorldDirection;
	FVector2D MousePosition;

	PlayerControllerInfernale->GetMousePosition(MousePosition.X, MousePosition.Y);
	PlayerControllerInfernale->DeprojectScreenPositionToWorld(MousePosition.X, MousePosition.Y, WorldLocation, WorldDirection);
	auto End = WorldLocation + WorldDirection * UFunctionLibraryInfernale::GetGameSettingsDataAsset()->RayCastLength;

	if (CurrentZoomFocusDelay <= 0)
	{
		GetWorld()->LineTraceSingleByChannel(ZoomHitResult, WorldLocation, End, CameraCollsionChannel);
	}


	if (ZoomHitResult.bBlockingHit)
	{
		auto middleOfScreen = FVector2D(GEngine->GameViewport->Viewport->GetSizeXY().X / 2, GEngine->GameViewport->Viewport->GetSizeXY().Y / 2);
		FHitResult HitResult;
		PlayerControllerInfernale->DeprojectScreenPositionToWorld(middleOfScreen.X, middleOfScreen.Y, WorldLocation, WorldDirection);
		End = WorldLocation + WorldDirection * UFunctionLibraryInfernale::GetGameSettingsDataAsset()->RayCastLength;
		GetWorld()->LineTraceSingleByChannel(HitResult, WorldLocation, End, CameraCollsionChannel);
		float DistBetween = (HitResult.ImpactPoint - ZoomHitResult.ImpactPoint).Size();
		// GEngine->AddOnScreenDebugMessage(-1, 0.2f, FColor::Red, FString::Printf(TEXT("DistBetween: %f"), DistBetween));
		FVector HitPointOffset = (HitResult.ImpactPoint - ZoomHitResult.ImpactPoint).GetSafeNormal();
		if (Value < 0 && Location.Z > CameraHeightLimits.X)
		{
			Location = FMath::Lerp(Location, ZoomHitResult.ImpactPoint - HitPointOffset * DistBetween, .01f * CameraHeightSpeed * ZoomSpeedMultiplier);
		}
		else if (Value > 0 && Location.Z < CameraHeightLimits.Y)
		{
			FVector3d BackwardHitLocation = (ZoomHitResult.ImpactPoint - Location).GetSafeNormal();
			Location = FMath::Lerp(Location, Location - BackwardHitLocation * InfernalePawnDataAsset->CameraHeightLimits.Y - HitPointOffset * DistBetween,  .01f * CameraHeightSpeed * ZoomSpeedMultiplier);
			if (Location.Z >= CameraHeightLimits.Y - 1)
			{
				Location.Z = CameraHeightLimits.Y;
				Location.Y = GetActorLocation().Y;
				Location.X = GetActorLocation().X;
			}
		}
		ZoomPositionTarget = Location;
	}
	CurrentZoomFocusDelay = ZoomFocusDelay;
}

void AInfernalePawn::RotationStarted()
{
	ShouldRotate = true;
 }

void AInfernalePawn::RotationCompleted()
{
	ShouldRotate = false;
}
void AInfernalePawn::KeyboardRotationActionTriggered(const FInputActionValue& InputActionValue)
{
	GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Red, TEXT("KeyboardRotationActionTriggered"));
	FVector WorldLocation, WorldDirection;
	const FVector2D ViewportSize = FVector2D(GEngine->GameViewport->Viewport->GetSizeXY());
	PlayerControllerInfernale->DeprojectScreenPositionToWorld(ViewportSize.X / 2, ViewportSize.Y / 2, WorldLocation, WorldDirection);
	FHitResult HitResult;
	const auto End = WorldLocation + WorldDirection * UFunctionLibraryInfernale::GetGameSettingsDataAsset()->RayCastLength;
	GetWorld()->LineTraceSingleByChannel(HitResult, WorldLocation, End, CameraCollsionChannel);
	const float RotationStep = RotationSpeed * RotationSpeedMultiplier * FixedDeltaTime * InputActionValue.Get<float>();
    CameraCenterOfInterest = HitResult.ImpactPoint;
	FVector RotationCenter = CameraCenterOfInterest;
	FVector Direction = GetActorLocation() - RotationCenter;

	// Apply rotation around the center
	Direction = Direction.RotateAngleAxis(RotationStep, FVector(0, 0, 1));
	FVector NewLocation = RotationCenter + Direction;

	FRotator NewRotation = GetActorRotation();
	NewRotation.Yaw += RotationStep;


	SetActorLocation(NewLocation);
	SetActorRotation(NewRotation);
}

void AInfernalePawn::LookTriggered(const FVector2D Value)
{
    const float RotationStep = (RotationSpeed * RotationSpeedMultiplier) * FixedDeltaTime * Value.X;
	FRotator CurrentRotation = GetActorRotation();
	FVector WorldLocation, WorldDirection;

	// Get viewport dimensions
	const FVector2D ViewportSize = FVector2D(GEngine->GameViewport->Viewport->GetSizeXY());
        
	// Project the screen position to the world
	PlayerControllerInfernale->DeprojectScreenPositionToWorld(ViewportSize.X / 2, ViewportSize.Y / 2, WorldLocation, WorldDirection);
        
	FHitResult HitResult;
	const auto End = WorldLocation + WorldDirection * UFunctionLibraryInfernale::GetGameSettingsDataAsset()->RayCastLength;
	GetWorld()->LineTraceSingleByChannel(HitResult, WorldLocation, End, CameraCollsionChannel);

    if (ShouldRotate)
    {
    	PlayerControllerInfernale->SetShowMouseCursor(false);
        

        if (HitResult.bBlockingHit && CurrentRotation.Pitch < -6)
        {
            RotateOnSelf = false;
            CameraCenterOfInterest = HitResult.ImpactPoint;
            // DrawDebugSphere(GetWorld(), CameraCenterOfInterest, 50, 12, FColor::Red, false, 0.1f);
        }
        else
        {
            RotateOnSelf = true;
        }

        // if (RotateOnSelf)
        // {
        //     // Frame-rate independent rotation by directly using RotationStep
        //     CurrentRotation.Yaw = FMath::FInterpTo(CurrentRotation.Yaw, CurrentRotation.Yaw + RotationStep, FixedDeltaTime, 1.0f);
        //     SetActorRotation(CurrentRotation);
        // }
        // else
        {
            // Orbit around the CameraCenterOfInterest
            FVector RotationCenter = CameraCenterOfInterest;
            FVector Direction = GetActorLocation() - RotationCenter;

            // Apply rotation around the center
            Direction = Direction.RotateAngleAxis(RotationStep, FVector(0, 0, 1));
            FVector NewLocation = RotationCenter + Direction;

            FRotator NewRotation = GetActorRotation();
            NewRotation.Yaw += RotationStep;

            SetActorLocation(NewLocation);
            SetActorRotation(NewRotation);
        	PlayerControllerInfernale->SetMouseLocation(ViewportSize.X / 2, ViewportSize.Y / 2);
        }
    }
	else
	{
		PlayerControllerInfernale->SetShowMouseCursor(true);
	}

    if (ShouldMove && CanMove)
    {
    	
    	ZoomPositionTarget = FVector::ZeroVector;
        // Move screen in the opposite direction of mouse movement using interpolation for frame independence
        FVector TargetLocation = GetActorLocation();
    	FVector2D MousePosition;
    	PlayerControllerInfernale->GetMousePosition(MousePosition.X, MousePosition.Y);
    	PlayerControllerInfernale->DeprojectScreenPositionToWorld(MousePosition.X, MousePosition.Y, WorldLocation, WorldDirection);
    	const auto End2 = WorldLocation + WorldDirection * UFunctionLibraryInfernale::GetGameSettingsDataAsset()->RayCastLength;
    	GetWorld()->LineTraceSingleByChannel(HitResult, WorldLocation, End2, CameraCollsionChannel);
    	if (LastMousePosition == FVector2D::ZeroVector)
    	{
    		LastMousePosition = FVector2D(HitResult.ImpactPoint.X, HitResult.ImpactPoint.Y);
		}
    	else
    	{
    		const float X = LastMousePosition.X - HitResult.ImpactPoint.X;
    		const float Y = LastMousePosition.Y - HitResult.ImpactPoint.Y;

    		// DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 200, 12, FColor::Red, false, 0.1f);
    		// DrawDebugSphere(GetWorld(), FVector(LastMousePosition.X, LastMousePosition.Y, HitResult.ImpactPoint.Z), 200, 12, FColor::Green, false, 0.1f);
    		
    		TargetLocation.X += X;
    		TargetLocation.Y += Y;
    		
    		SetActorLocation(TargetLocation);
    		LastMousePosition = FVector2D(HitResult.ImpactPoint.X + X, HitResult.ImpactPoint.Y + Y);
    	}
    }
}


void AInfernalePawn::AddOrOverrideCameraPositionAtIndex(int32 Index)
{
	if (CameraSavedPositions.IsValidIndex(Index))
	{
		CameraSavedPositions[Index] = GetActorLocation();
		CameraSavedRotations[Index] = GetActorRotation();
	}
	//TODO move to struct
}

void AInfernalePawn::SetCameraPositionAtIndex(int32 Index)
{
	if (!CameraSavedPositions.IsValidIndex(Index)) return;
	SetActorLocation(CameraSavedPositions[Index]);
	
	if (!CameraSavedRotations.IsValidIndex(Index)) return;
	SetActorRotation(CameraSavedRotations[Index]);
}

void AInfernalePawn::SubscribeToLookEvents()
{
	if (PlayerControllerInfernale == nullptr) return;
	
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Subscribe to Look Events"));
	PlayerControllerInfernale->LookTriggered.AddDynamic(this, &AInfernalePawn::LookTriggered);
}

void AInfernalePawn::UnsubscribeFromLookEvents()
{
	if (PlayerControllerInfernale == nullptr) return;

	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Unsubscribe from Look Events"));
	PlayerControllerInfernale->LookTriggered.RemoveDynamic(this, &AInfernalePawn::LookTriggered);
}



void AInfernalePawn::MoveToLocationMulticast_Implementation(FVector Location)
{
	SetActorLocation(Location);
	PawnSetAtLocation = true;
	auto GameInstance = Cast<UGameInstanceInfernale>(GetGameInstance());

	const auto InteractionComponent = PlayerControllerInfernale ? PlayerControllerInfernale->GetInteractionComponent() : nullptr;
	
	if (GameInstance == nullptr || InteractionComponent == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("GameInstance is nullptr"));
		PawnSetAtSpawn.Broadcast();
	}
	else
	{
		GameInstance->InfernalePawn = this;
		GameInstance->LoadGameSettings();
		InteractionComponent->SetAllowedToInteract(true);
		PawnSetAtSpawn.Broadcast();
	}

}

void AInfernalePawn::AddOverrideCameraProfileIndexPositionMulticast_Implementation(int32 Index, FVector Location,
	FRotator Rotation)
{
	if (CameraSavedPositions.IsValidIndex(Index))
	{
		CameraSavedPositions[Index] = Location;
		CameraSavedRotations[Index] = Rotation;
	}
}

void AInfernalePawn::CameraProfileTriggered(FVector2D CameraProfileVector)
{
	int Index = 0;
	if (CameraProfileVector.Y > 0)
	{
		Index = 0;
	}
	else if (CameraProfileVector.Y < 0)
	{
		Index = 1;
	}
	else if (CameraProfileVector.X < 0)
	{
		Index = 2;
	}
	else if (CameraProfileVector.X > 0)
	{
		Index = 3;
	}
	if (!PlayerControllerInfernale->IsCtrlPressed())
	{
		SetCameraPositionAtIndex(Index);
	}
	else
	{
		AddOrOverrideCameraPositionAtIndex(Index);
	}
}

void AInfernalePawn::CameraProfileCompleted(FVector2D CameraProfileVector)
{
}

FVector AInfernalePawn::GetPawnStartLocation()
{
	return InfernalePawnDataAsset->GameStartPosition;
}

void AInfernalePawn::SetDoCameraStartGameTravelingOwning_Implementation(bool Value)
{
	DistanceFromStartPosToMainBase = FVector::Distance(InfernalePawnDataAsset->GameStartPosition, CameraSavedPositions[0]);
	bDoCameraStartGameTraveling = Value;
}

void AInfernalePawn::SetDoCameraEndGameTraving()
{
	CanMove = false;
	DistanceFromCurrentPosToStart = FVector::Distance(GetActorLocation(), InfernalePawnDataAsset->GameStartPosition);
	bDoCameraEndGameTraveling = true;
}


void AInfernalePawn::SubscribeToRotateEvents()
{
	if (PlayerControllerInfernale == nullptr) return;
	PlayerControllerInfernale->EnableRotationStarted.AddDynamic(this, &AInfernalePawn::RotationStarted);
}

void AInfernalePawn::UnsubscribeFromRotateEvents()
{
	if (PlayerControllerInfernale == nullptr) return;
	PlayerControllerInfernale->EnableRotationStarted.RemoveDynamic(this, &AInfernalePawn::RotationStarted);
}

void AInfernalePawn::MoveToSpawn(FVector Location)
{
	MoveToLocationMulticast(Location);
}

void AInfernalePawn::SetCenterPoint(FVector NewCenterPoint)
{
	CameraCenterPoint = NewCenterPoint;
}

void AInfernalePawn::ResetRotation()
{
	if (CameraSavedRotations.Num() <= 0) return;
	SetActorRotation(CameraSavedRotations[0]);
}

void AInfernalePawn::GetLookAtLocation(FHitResult& OutHit, bool Debug) const
{
	FVector Location = CameraComponent->GetComponentLocation();
	FVector Direction = CameraComponent->GetForwardVector();
	
	
	const auto End = Location + Direction * UFunctionLibraryInfernale::GetGameSettingsDataAsset()->RayCastLength;
	GetWorld()->LineTraceSingleByChannel(OutHit, Location, End, CameraCollsionChannel);
	if (OutHit.bBlockingHit && Debug) DrawDebugSphere(GetWorld(), OutHit.ImpactPoint, 50, 12, FColor::Red, false, 0.5f);
}

FVector AInfernalePawn::GetLookAtLocationPoint() const
{
	return CameraCenterPoint;
}

void AInfernalePawn::AddOverrideCameraProfileIndexPosition(int32 Index, FVector Location, FRotator Rotation)
{
	AddOverrideCameraProfileIndexPositionMulticast(Index, Location, Rotation);
}

void AInfernalePawn::MoveStarted()
{
	ShouldMoveFalseTimer = .0f;
	ShouldMove = true;
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("MoveStarted"));
	bisMoving = true;
}

void AInfernalePawn::SecondaryMoveCompleted()
{
	ShouldMove = false;
	ShouldMoveFalseTimer = .3f;
	LastMousePosition = FVector2D::ZeroVector;
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("SecondaryMoveCompleted"));
}

void AInfernalePawn::SubscribeToInputEvents()
{
	if (PlayerControllerInfernale == nullptr) return;

	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Subscribed to Input Events"));

	PlayerControllerInfernale->MoveTriggered.AddDynamic(this, &AInfernalePawn::MoveTriggered);
	PlayerControllerInfernale->MoveCompleted.AddDynamic(this, &AInfernalePawn::MoveCompleted);
	PlayerControllerInfernale->EnableMoveActionTriggered.AddDynamic(this, &AInfernalePawn::MoveStarted);
	PlayerControllerInfernale->EnableMoveActionCompleted.AddDynamic(this, &AInfernalePawn::SecondaryMoveCompleted);
	PlayerControllerInfernale->LookTriggered.AddDynamic(this, &AInfernalePawn::LookTriggered);
	PlayerControllerInfernale->ScrollTriggered.AddDynamic(this, &AInfernalePawn::CameraHeightTriggered);
	PlayerControllerInfernale->EnableRotationStarted.AddDynamic(this, &AInfernalePawn::RotationStarted);
	PlayerControllerInfernale->EnableRotationCompleted.AddDynamic(this, &AInfernalePawn::RotationCompleted);
	PlayerControllerInfernale->CameraProfileTriggered.AddDynamic(this, &AInfernalePawn::CameraProfileTriggered);
	PlayerControllerInfernale->CameraProfileCompleted.AddDynamic(this, &AInfernalePawn::CameraProfileCompleted);
}

void AInfernalePawn::UnsubscribeFromInputEvents()
{
	if (PlayerControllerInfernale == nullptr) return;
	
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Unsubscribe from Input Events"));
	//TODO Check if subscribed
	
	PlayerControllerInfernale->MoveTriggered.RemoveDynamic(this, &AInfernalePawn::MoveTriggered);
	PlayerControllerInfernale->MoveCompleted.RemoveDynamic(this, &AInfernalePawn::MoveCompleted);
	PlayerControllerInfernale->EnableMoveActionTriggered.RemoveDynamic(this, &AInfernalePawn::MoveStarted);
	PlayerControllerInfernale->EnableMoveActionCompleted.RemoveDynamic(this, &AInfernalePawn::SecondaryMoveCompleted);
	PlayerControllerInfernale->LookTriggered.RemoveDynamic(this, &AInfernalePawn::LookTriggered);
	PlayerControllerInfernale->ScrollTriggered.RemoveDynamic(this, &AInfernalePawn::CameraHeightTriggered);
	PlayerControllerInfernale->EnableRotationStarted.RemoveDynamic(this, &AInfernalePawn::RotationStarted);
	PlayerControllerInfernale->EnableRotationCompleted.RemoveDynamic(this, &AInfernalePawn::RotationCompleted);
	PlayerControllerInfernale->CameraProfileTriggered.RemoveDynamic(this, &AInfernalePawn::CameraProfileTriggered);
	PlayerControllerInfernale->CameraProfileCompleted.RemoveDynamic(this, &AInfernalePawn::CameraProfileCompleted);
}

void AInfernalePawn::TryInitializePlayerControllerInfernale()
{
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("TryInitializePlayerControllerInfernale"));
	auto* LocalController = GetLocalViewingPlayerController();
	if (LocalController == nullptr)
	{
		TryInitializePlayerControllerInfernaleDelayed();
		return;
	}

	PlayerControllerInfernale = Cast<APlayerControllerInfernale>(LocalController);
	if (PlayerControllerInfernale == nullptr)
	{
		TryInitializePlayerControllerInfernaleDelayed();
		return;
	}

	InitializePlayerControllerInfernale();
}

void AInfernalePawn::TryInitializePlayerControllerInfernaleSimple()
{
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("TryInitializePlayerControllerInfernale"));
	auto* LocalController = GetLocalViewingPlayerController();
	if (LocalController == nullptr) return;

	PlayerControllerInfernale = Cast<APlayerControllerInfernale>(LocalController);
}

void AInfernalePawn::TryInitializePlayerControllerInfernaleDelayed()
{
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AInfernalePawn::TryInitializePlayerControllerInfernale, 0.1f, false);
}

void AInfernalePawn::InitializePlayerControllerInfernale()
{
	if (!IsValid(PlayerControllerInfernale))
	{
		TryInitializePlayerControllerInfernaleDelayed();
		return;
	}
	if (PlayerControllerWasInitialized) return;
	
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("PlayerControllerInfernale is valid"));
	PlayerControllerWasInitialized = true;
	
	PlayerControllerInfernale->SetPlayerInputComponent(PlayerInputComponent);
	
	PlayerControllerInfernale->SetShowMouseCursor(true);
	PlayerControllerInfernale->bEnableClickEvents = true;
	PlayerControllerInfernale->bEnableMouseOverEvents = true;
	
	SubscribeToInputEvents();
}

bool AInfernalePawn::CheckBoundaries(FVector Location, UE::Math::TVector<double>& NextLocation) const
{
	bool isGoingToHit = false;
	if (Location.Z > CameraHeightLimits.X && Location.Z < CameraHeightLimits.Y)
	{
		// NextLocation.Z = Location.Z;
	}
	if (NextLocation.Z < 0)
	{
		NextLocation.Z = 500;
		isGoingToHit = true;
	}
	if (NextLocation.Z > CameraHeightLimits.Y || NextLocation.Z < CameraHeightLimits.X)
    {
		NextLocation.Z = FMath::Clamp(NextLocation.Z, CameraHeightLimits.X, CameraHeightLimits.Y);
        return true;
    }
	// NextLocation.X = FMath::Clamp(NextLocation.X, -CameraXYLimits.X, CameraXYLimits.X);
	// NextLocation.Y = FMath::Clamp(NextLocation.Y, -CameraXYLimits.Y, CameraXYLimits.Y);
	if (NextLocation.X < -CameraXYLimits.X || NextLocation.X > CameraXYLimits.X)
	{
		NextLocation.X = FMath::Clamp(NextLocation.X, -CameraXYLimits.X, CameraXYLimits.X);
		isGoingToHit = true;
	}
	if (NextLocation.Y < -CameraXYLimits.Y || NextLocation.Y > CameraXYLimits.Y)
	{
		NextLocation.Y = FMath::Clamp(NextLocation.Y, -CameraXYLimits.Y, CameraXYLimits.Y);
		isGoingToHit = true;
	}
	return isGoingToHit;
}

void AInfernalePawn::OnPawnSetAtSpawn()
{
	OnPawnSetAtSpawnBP();	
}

void AInfernalePawn::OnPawnSpawnTravelFinished()
{
	OnPawnSpawnTravelFinishedBP();
}

// Called every frame
void AInfernalePawn::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
	FHitResult HitResult;
	if (ShouldMoveFalseTimer > 0.f)
	{
		ShouldMoveFalseTimer -= DeltaTime;
		if (ShouldMoveFalseTimer < 0.f) ShouldMoveFalseTimer = 0.f;
	}
	GetLookAtLocation(HitResult, bDebug);
	if (PlayerControllerInfernale)
	{
		//if (!HasAuthority()) GEngine->AddOnScreenDebugMessage(-1, 5.f, PlayerControllerInfernale->IsLocalController() ? FColor::Green : FColor::Red, TEXT("PlayerController is valid"));
		if (PlayerControllerInfernale->IsLocalController()) CameraCenterPoint = HitResult.ImpactPoint;
		PlayerControllerInfernale->AskReplicateCenterPoint(HitResult.ImpactPoint);
	}
	else if (!HasAuthority())
	{
		auto* LocalController = GetLocalViewingPlayerController();
		PlayerControllerInfernale = Cast<APlayerControllerInfernale>(LocalController);
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PlayerController is not valid, Retrying..."));
	}

	if (CurrentZoomFocusDelay > 0)
	{
		CurrentZoomFocusDelay -= FixedDeltaTime;
	}
	if (bDebugZoomLevel) GEngine->AddOnScreenDebugMessage(-1, .0025f, FColor::Green, FString::Printf(TEXT("CurrentZoomFocusDelay: %s"), *FString::SanitizeFloat(CurrentZoomFocusDelay)));

	AccumulatedDeltaTime += DeltaTime;
	int32 SubSteps = FMath::Clamp(FMath::FloorToInt(AccumulatedDeltaTime / FixedDeltaTime), 1, MaxSubsteps);
	AccumulatedDeltaTime -= SubSteps * FixedDeltaTime;	
	for (int32 Step = 0; Step < SubSteps; ++Step)
	{
		
		if (!PlayerControllerWasInitialized) return;

		auto CurrentMoveSpeed = CurrentInputMoveSpeed;
		
		// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("CurrentMoveSpeed: %s"), *CurrentMoveSpeed.ToString()));
		if (PlayerControllerInfernale == nullptr)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PlayerController is not valid"));
			return;
		}
		if (FVector2d MousePosition; CurrentInputMoveSpeed.X == 0 && CurrentInputMoveSpeed.Y == 0 && PlayerControllerInfernale->GetMousePosition(MousePosition.X, MousePosition.Y) && !ShouldMove && ShouldMoveFalseTimer <= .0f)
		{
			if (MousePosition.X == 0 && MousePosition.Y == 0) return;
			int32 ViewPortWidth, ViewPortHeight;
			PlayerControllerInfernale->GetViewportSize(ViewPortWidth, ViewPortHeight);
			
			// const auto X = -(MousePosition.X <= ViewPortWidth * ScreenEdgePadding.X) | (MousePosition.X >= ViewPortWidth - ViewPortWidth * ScreenEdgePadding.X);
			// const auto Y = -(MousePosition.Y <= ViewPortHeight * ScreenEdgePadding.Y) | (MousePosition.Y >= ViewPortHeight - ViewPortHeight * ScreenEdgePadding.Y);
			const auto X = ((MousePosition.X >= ViewPortWidth - ViewPortWidth * ScreenEdgePadding.X)
				                ? (MousePosition.X - (ViewPortWidth - ViewPortWidth * ScreenEdgePadding.X))
				                / (ViewPortWidth * ScreenEdgePadding.X)
				                : 0) - ((MousePosition.X <= ViewPortWidth * ScreenEdgePadding.X)
					                        ? (ViewPortWidth * ScreenEdgePadding.X - MousePosition.X) / (ViewPortWidth *
					                        ScreenEdgePadding.X)
				                        : 0);
			const auto Y = ((MousePosition.Y >= ViewPortHeight - ViewPortHeight * ScreenEdgePadding.Y)
			               ? (MousePosition.Y - (ViewPortHeight - ViewPortHeight * ScreenEdgePadding.Y)) / (
				                ViewPortHeight * ScreenEdgePadding.Y)
				                : 0) -
			((MousePosition.Y <= ViewPortHeight * ScreenEdgePadding.Y)
				 ? (ViewPortHeight * ScreenEdgePadding.Y - MousePosition.Y) / (ViewPortHeight * ScreenEdgePadding.Y)
				 : 0);
			 if (X != 0 || Y != 0)
			 {
			 	CurrentMoveSpeed = FVector2D(X , -Y).GetSafeNormal() * 100;
			 }
		}

		//Lerp the camera's position
		auto Location = GetActorLocation();
		// Location = FMath::VInterpTo(Location, Location, FixedDeltaTime, LerpSpeed);
		
		//Move the camera
		auto ForwardVector = FVector(GetActorForwardVector().X, GetActorForwardVector().Y, 0);
		ForwardVector.Normalize();

		const auto Forward = ForwardVector * CurrentMoveSpeed.Y * (MoveSpeed * MoveSpeedMultiplier) * DeltaTime;
		const auto Sideways = GetActorRightVector() * CurrentMoveSpeed.X * (MoveSpeed * MoveSpeedMultiplier) * DeltaTime;


		FVector NextLocation;
		if (bDoCameraStartGameTraveling)
		{
			if (!CanMove)
			{
				float LocalLerpSpeed = CameraCurveSpeed->FloatCurve.Eval(FVector::Distance(GetActorLocation(), CameraSavedPositions[0])/ DistanceFromStartPosToMainBase);
				NextLocation = FMath::VInterpConstantTo(GetActorLocation(), CameraSavedPositions[0], DeltaTime, LocalLerpSpeed);
				if (FVector::Distance(NextLocation, CameraSavedPositions[0]) < 1)
				{
					bDoCameraStartGameTraveling = false;
					CanMove = true;
					bisMoving = false;
					NextLocation = CameraSavedPositions[0];
					PawnSpawnTravelFinished.Broadcast();
				}
				SetActorLocation(NextLocation);
				FRotator CurrentRotation = GetActorRotation();
				PitchTarget = CameraPitchCurve->FloatCurve.Eval((Location.Z - CameraHeightLimits.X) / (CameraHeightLimits.Y - CameraHeightLimits.X));
				CurrentRotation.Pitch = FMath::Clamp(FMath::Lerp(CurrentRotation.Pitch, PitchTarget, LerpSpeed * FixedDeltaTime), CameraAnglePitchLimits.X, CameraAnglePitchLimits.Y);
				CurrentRotation.Roll = 0;
				SetActorRotation(CurrentRotation);
			}
		}
		else if (bDoCameraEndGameTraveling)
		{
			if (!CanMove)
			{
				float LocalLerpSpeed = CameraCurveSpeed->FloatCurve.Eval(FVector::Distance(GetActorLocation(), InfernalePawnDataAsset->GameStartPosition) / DistanceFromCurrentPosToStart);
				NextLocation = FMath::VInterpConstantTo(GetActorLocation(), InfernalePawnDataAsset->GameStartPosition, DeltaTime, LocalLerpSpeed);
				if (FVector::Distance(NextLocation, InfernalePawnDataAsset->GameStartPosition) < 1)
				{
					bDoCameraEndGameTraveling = false;
					bisMoving = false;
					NextLocation = InfernalePawnDataAsset->GameStartPosition;
				}
				SetActorLocation(NextLocation);
				FRotator CurrentRotation = GetActorRotation();
				PitchTarget = CameraPitchCurve->FloatCurve.Eval((Location.Z - CameraHeightLimits.X) / (CameraHeightLimits.Y - CameraHeightLimits.X));
				CurrentRotation.Pitch = FMath::Clamp(FMath::Lerp(CurrentRotation.Pitch, PitchTarget, LerpSpeed * FixedDeltaTime), CameraAnglePitchLimits.X, CameraAnglePitchLimits.Y);
				CurrentRotation.Roll = 0;
				SetActorRotation(CurrentRotation);
			}
		}
		else
		{
			NextLocation = GetActorLocation() + Forward + Sideways;
			CheckBoundaries(Location, NextLocation);
		}
		// DrawDebugSphere(GetWorld(), ZoomPositionTarget, 200, 12, FColor::Yellow, false, 15.f);
		if (CanMove)
		{
			if (ZoomPositionTarget != FVector::ZeroVector)
			{
				if (CheckBoundaries(Location, ZoomPositionTarget))
				{
					// DrawDebugSphere(GetWorld(), ZoomPositionTarget, 200, 12, FColor::Blue, false, 15.f);
					ZoomPositionTarget = FVector::ZeroVector;
				}
				else
				{
					// DrawDebugSphere(GetWorld(), ZoomPositionTarget, 200, 12, FColor::Magenta, false, 15.f);
					// SetActorLocation(FMath::Lerp(GetActorLocation(), ZoomPositionTarget, LerpSpeed * FixedDeltaTime));
					SetActorLocation(ZoomPositionTarget);
					if (UE::Geometry::Distance(ZoomPositionTarget, GetActorLocation()) < 100)
					{
						ZoomPositionTarget = FVector::ZeroVector;
					}
				}
			}
			else
			{
				SetActorLocation(NextLocation);
				// SetActorLocation(NextLocation);
				bisMoving = false;
			}
		}
		else
		{
			return;
		}
		
		FRotator CurrentRotation = GetActorRotation();
		PitchTarget = CameraPitchCurve->FloatCurve.Eval((Location.Z - CameraHeightLimits.X) / (CameraHeightLimits.Y - CameraHeightLimits.X));
		CurrentRotation.Pitch = FMath::Clamp(FMath::Lerp(CurrentRotation.Pitch, PitchTarget, LerpSpeed * FixedDeltaTime), CameraAnglePitchLimits.X, CameraAnglePitchLimits.Y);
		CurrentRotation.Roll = 0;
		SetActorRotation(CurrentRotation);

		if (CurrentMoveSpeed.X != 0 || CurrentMoveSpeed.Y != 0) PawnMoved.Broadcast(GetActorLocation());
		
	}
}

// Called to bind functionality to input
void AInfernalePawn::SetupPlayerInputComponent(UInputComponent* NewPlayerInputComponent)
{
	Super::SetupPlayerInputComponent(NewPlayerInputComponent);
	PlayerInputComponent = NewPlayerInputComponent;

	TryInitializePlayerControllerInfernale();
	
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("SetupPlayerInputComponent"));
}

UCameraComponent* AInfernalePawn::GetCameraComponent() const
{
	return CameraComponent;
}


