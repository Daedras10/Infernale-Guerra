// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/AmbianceManager.h"

#include "Components/SkyLightComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/SkyLight.h"
#include "GameMode/Infernale/PlayerControllerInfernale.h"
#include "Kismet/GameplayStatics.h"

FAmbianceSphere::FAmbianceSphere(): SphereComponent(nullptr)
{
}

FAmbianceSphere::FAmbianceSphere(USphereComponent* InSphereComponent, FVector InCenter, float InRadius):
	SphereComponent(nullptr)
{
}

// Sets default values
AAmbianceManager::AAmbianceManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AAmbianceManager::BeginPlay()
{
	Super::BeginPlay();
	CreateMissingSpheres();
	SpheresVisibility(bDebug);
	GetSkyLight();
	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, this, &AAmbianceManager::SubscribeToPCEvents, 2.f, false);
	TargetColor = AmbianceSpheres[1].AmbianceColor;
	TargetIntensity = AmbianceSpheres[1].AmbianceIntensity;
	
}

void AAmbianceManager::RefreshSphere(const FAmbianceSphere& AmbianceSphere)
{
	const auto Sphere = AmbianceSphere.SphereComponent;
	if (!Sphere) return;

	Sphere->SetWorldLocation(AmbianceSphere.Center);
	Sphere->SetSphereRadius(AmbianceSphere.Radius);
}

void AAmbianceManager::SpheresVisibility(const bool bVisible)
{
	for (auto AmbianceSphere : AmbianceSpheres)
	{
		auto Sphere = AmbianceSphere.SphereComponent;
		if (!Sphere) continue;
		Sphere->SetVisibility(bVisible);
		Sphere->SetHiddenInGame(!bVisible);
	}
}

void AAmbianceManager::DestroyAmbianceSphere(const FAmbianceSphere& AmbianceSphere)
{
	auto Sphere = AmbianceSphere.SphereComponent;
	if (!Sphere) return;
	Sphere->DestroyComponent();
}

void AAmbianceManager::CreateSphereIndex(const int i)
{
	auto Sphere = NewObject<USphereComponent>(this);
	if (!Sphere) return;;
	Sphere->RegisterComponent();
	Sphere->SetVisibility(true);
	Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	auto BaseRadius = (i+1)*Radius;
	Sphere->SetSphereRadius(BaseRadius);
	Sphere->SetWorldLocation(DefaultCenter);
	AmbianceSpheres.Add(FAmbianceSphere(Sphere, DefaultCenter, BaseRadius));
}

void AAmbianceManager::GetSkyLight()
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASkyLight::StaticClass(), FoundActors);
	if (FoundActors.Num() > 0)
	{
		SkyLight = Cast<ASkyLight>(FoundActors[0]);
	}
}

void AAmbianceManager::ChangeTargets(FVector NewPosition)
{
	float MinRadius = -1;
	FAmbianceSphere CurrentAmbianceSphere;
	NewPosition.Z = 0;
	for (auto AmbianceSphere : AmbianceSpheres)
	{
		auto Center = AmbianceSphere.Center;
		Center.Z = 0;
		const auto Distance = FVector::Dist(NewPosition, Center);
		if (Distance > AmbianceSphere.Radius) continue;
		if (MinRadius == -1 || AmbianceSphere.Radius < MinRadius)
		{
			MinRadius = AmbianceSphere.Radius;
			CurrentAmbianceSphere = AmbianceSphere;
		}
	}

	if (MinRadius == -1)
	{
		TargetColor = FColor::White;
		TargetIntensity = 1;
	}
	else
	{
		TargetColor = CurrentAmbianceSphere.AmbianceColor;
		TargetIntensity = CurrentAmbianceSphere.AmbianceIntensity;
		//auto NewColor = FColor(TargetColor.R, TargetColor.G, TargetColor.B, 255);
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, NewColor, FString::Printf(TEXT("Ambiance: %f"), CurrentAmbianceSphere.Radius));
	}
}

void AAmbianceManager::SubscribeToPCEvents()
{
	const auto PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PlayerController)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, TEXT("PlayerController not found in AAmbianceManager::SubscribeToPCEvents"));
		return;
	}

	const auto PlayerControllerInfernale = Cast<APlayerControllerInfernale>(PlayerController);
	if (!PlayerControllerInfernale)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, TEXT("PlayerControllerInfernale not found in AAmbianceManager::SubscribeToPCEvents"));
		return;
	}

	PlayerControllerInfernale->MoveTriggered.AddDynamic(this, &AAmbianceManager::OnPCMoveVector2);
	PlayerControllerInfernale->MoveCompleted.AddDynamic(this, &AAmbianceManager::OnPCMoveVector2);
	PlayerControllerInfernale->ScrollTriggered.AddDynamic(this, &AAmbianceManager::OnPCMoveFloat);
	PlayerControllerInfernale->LookTriggered.AddDynamic(this, &AAmbianceManager::OnPCMoveVector2);

	// Might need to check if we implement different pawn for spectate mode
	InfernalePawn = Cast<AInfernalePawn>(PlayerControllerInfernale->GetPawn());
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("AAmbianceManager: Subscribed to PC Events"));
}

void AAmbianceManager::OnPCMoveVector2(FVector2D _)
{
	OnPCMove();
}

void AAmbianceManager::OnPCMoveFloat(float _)
{
	OnPCMove();
}

void AAmbianceManager::OnPCMove()
{
	FHitResult HitResult;
	InfernalePawn->GetLookAtLocation(HitResult);
	if (!HitResult.bBlockingHit) return;
	ChangeTargets(HitResult.ImpactPoint);
}

void AAmbianceManager::OnPCMoveBP()
{
	OnPCMove();
}

void AAmbianceManager::RefreshSpheres()
{
	if (!SkyLight)
	{
		GetSkyLight();
		if (!SkyLight) return;
	}

	for (auto AmbianceSphere : AmbianceSpheres)
	{
		RefreshSphere(AmbianceSphere);
	}
}

void AAmbianceManager::MakeSpheresVisible()
{
	SpheresVisibility(true);
}

void AAmbianceManager::MakeSpheresInvisible()
{
	SpheresVisibility(false);
}

void AAmbianceManager::CreateMissingSpheres()
{
	for (int i = AmbianceSpheres.Num(); i < SphereToCreate; i++)
	{
		FAmbianceSphere* AmbianceSphere = &AmbianceSpheres[0];
		if (AmbianceSphere->SphereComponent) continue;

		AmbianceSphere->SphereComponent = NewObject<USphereComponent>(this);
		auto Sphere = AmbianceSphere->SphereComponent;
		Sphere->SetVisibility(true);
		Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Sphere->SetWorldLocation(AmbianceSphere->Center);
		Sphere->SetSphereRadius(AmbianceSphere->Radius);
	}
}

void AAmbianceManager::CreateSpheres()
{
	ClearSpheres();
	for (int i = 0; i < SphereToCreate; i++)
	{
		CreateSphereIndex(i);
	}
}

void AAmbianceManager::ClearSpheres()
{
	for (auto AmbianceSphere : AmbianceSpheres)
	{
		DestroyAmbianceSphere(AmbianceSphere);
	}
	AmbianceSpheres.Empty();
}

void AAmbianceManager::DestroyLast()
{
	if (AmbianceSpheres.Num() == 0) return;
	DestroyAmbianceSphere(AmbianceSpheres[AmbianceSpheres.Num() - 1]);
	AmbianceSpheres.RemoveAt(AmbianceSpheres.Num() - 1);
}

void AAmbianceManager::CreateOneMore()
{
	CreateSphereIndex(AmbianceSpheres.Num());
}

// Called every frame
void AAmbianceManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//const auto TargetColor = CurrentAmbianceSphere == nullptr ? FLinearColor::White : CurrentAmbianceSphere->AmbianceColor;
	const auto Speed = DeltaTime * LerpSpeed;
	CurrentIntensity = FMath::Lerp(CurrentIntensity, TargetIntensity, Speed);
	SkyLight->GetLightComponent()->SetIntensity(CurrentIntensity);

	CurrentColor = FLinearColor::LerpUsingHSV(CurrentColor, TargetColor, Speed);
	SkyLight->GetLightComponent()->SetLightColor(CurrentColor);
}

