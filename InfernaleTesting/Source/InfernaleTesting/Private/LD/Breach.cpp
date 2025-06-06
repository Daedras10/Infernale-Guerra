// Fill out your copyright notice in the Description page of Project Settings.


#include "LD/Breach.h"

#include "Component/PlayerController/InteractionComponent.h"
#include "DataAsset/GameSettingsDataAsset.h"
#include "FogOfWar/FogOfWarManager.h"
#include "FunctionLibraries/FunctionLibraryInfernale.h"
#include "GameMode/Infernale/PlayerStateInfernale.h"
#include "LD/Buildings/Building.h"
#include "LD/Buildings/MainBuilding.h"

// Sets default values
ABreach::ABreach()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent->SetMobility(EComponentMobility::Static);
	FogOfWarComponent = CreateDefaultSubobject<UFogOfWarComponent>(TEXT("FogOfWarComponent"));
}

void ABreach::UpdateVisuals()
{
	Execute_NewOwnerVisuals(this, this, OwnerWithTeam, OwnerWithTeam);
}

// Called when the game starts or when spawned
void ABreach::BeginPlay()
{
	Super::BeginPlay();
	bUseForgOfWar = UFunctionLibraryInfernale::GetGameSettingsDataAsset()->UseFogOfWar;
	FogOfWarManagerClass = UFunctionLibraryInfernale::GetGameSettingsDataAsset()->FogOfWarManagerClass;
}

void ABreach::OnBuildingDestroyed(ABuilding* Building)
{
	SetBuildingOnBreach(nullptr);
}

void ABreach::OnBuildingMadePermanent(ABuilding* Building)
{
	LockBreachMulticast(true);
}

void ABreach::SetMainBuildingMulticast_Implementation(AMainBuilding* NewMainBuilding)
{
	if (HasAuthority()) return;
	MainBuilding = NewMainBuilding;
}

void ABreach::SetOwnerWithTeamMulticast_Implementation(const FOwner OwnerWithTeamVal)
{
	if (HasAuthority()) return;
	OwnerWithTeam = OwnerWithTeamVal;
}

void ABreach::SetBuildingOnBreachMulticast_Implementation(ABuilding* BuildingOnBreachVal, bool bBuildingOnBreachVal)
{
	if (HasAuthority()) return;
	const auto OldBuildingOnBreach = bBuildingOnBreach;
	BuildingOnBreach = BuildingOnBreachVal;
	bBuildingOnBreach = bBuildingOnBreachVal;
}

void ABreach::SetBreachMulticast_Implementation(const bool IsBreach)
{
	bIsRealBreach = IsBreach;
}

void ABreach::SetMeshMulticast_Implementation(UStaticMesh* NewMesh)
{
	SetMeshBP(NewMesh);
}

void ABreach::LockBreachMulticast_Implementation(const bool bLockedVal)
{
	bLocked = bLockedVal;
	InteractEndMain(nullptr);
	LockVisualBP(bLocked);
	if (!bLockedVal) return;
	if (UIisOpened) DisplayUI(false);
}

// Called every frame
void ABreach::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

FOwner ABreach::GetOwner()
{
	return OwnerWithTeam;
}

void ABreach::SetOwner(FOwner NewOwner)
{
	OwnerWithTeam = NewOwner;
	SetOwnerWithTeamMulticast(NewOwner);
}


void ABreach::MainBuildingOwnershipChanged(ABuildingParent* Building, FOwner OldOwner, FOwner NewOwner)
{
	auto OldOwnerWithTeam = OwnerWithTeam;
	ChangeOwner(NewOwner);
	Execute_NewOwnerVisuals(this, this, OldOwnerWithTeam, NewOwner);
	if (!HasAuthority()) return;
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("MainBuildingOwnershipChanged::Breach %s"), *GetName()));

	if (OldOwnerWithTeam.Team == NewOwner.Team) return;
	BreachOwnershipChanged.Broadcast(this, OldOwnerWithTeam, NewOwner);
	
}

void ABreach::InteractStartMain(APlayerControllerInfernale* Interactor)
{
	IInteractable::InteractStartMain(Interactor);
	if (!bIsRealBreach)
	{
		if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Not a real breach"));
		const auto InteractorMainBuilding = Cast<IInteractable>(MainBuilding);
		// Interactor->GetInteractionComponent()->ForceStartMainInteract(InteractorMainBuilding);
		return;
	}
	if (bLocked)
	{
		if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Breach is locked"));
		return;
	}
	if (Interactor->GetTeam() != OwnerWithTeam.Team)
	{
		if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("Interactor is not on the same team (%d != %d)"), Interactor->GetTeam(), OwnerWithTeam.Team));
		return;
	}
	DisplayUI(true);
}

void ABreach::InteractEndMain(APlayerControllerInfernale* Interactor)
{
	if (!bIsRealBreach)
	{
		const auto InteractorMainBuilding = Cast<IInteractable>(MainBuilding);
		// Interactor->GetInteractionComponent()->ForceEndMainInteractIf(InteractorMainBuilding);
		return;
	}
	IInteractable::InteractEndMain(Interactor);
	DisplayUI(false);
}

void ABreach::InteractEndHover(APlayerControllerInfernale* Interactor)
{
	IInteractable::InteractEndHover(Interactor);
	if (Interactor->GetTeam() != OwnerWithTeam.Team) return;
	bHovered = false;
	OnHoverEnd();
}

bool ABreach::ShouldEndMainInteractOnMove()
{
	return true;
}

float ABreach::GetRepulsorRange() const
{
	return RepulsorRange;
}

void ABreach::InteractStartHover(APlayerControllerInfernale* Interactor)
{
	if (Interactor->bIsEscapeMenuOpen) return;
	IInteractable::InteractStartHover(Interactor);
	if (Interactor == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Interactor is null"));
		return;
	}
	if (Interactor->GetTeam() != OwnerWithTeam.Team) return;
	bHovered = true;
	OnHoverStart();
}

void ABreach::DisplayUI(bool Display)
{
	const auto WasDisplaying = UIisOpened;
	if (Display == WasDisplaying) return;
	Execute_DisplayWidget(this, Display, EDisplayWidgetType::DisplayWidgetBreach);
	UIisOpened = Display;
}

bool ABreach::InteractableHasUIOpen()
{
	return UIisOpened;
}

FTransform ABreach::GetBuildingSpawningTransform() const
{
	FTransform Transform = FTransform(GetActorRotation(), GetActorLocation(), FVector(1, 1, 1));
	Transform.SetLocation(Transform.GetLocation() + BuildingSpawnOffset);
	return Transform;
}

bool ABreach::HasBuildingOnBreach() const
{
	return bBuildingOnBreach;
}

void ABreach::SetBuildingOnBreach(ABuilding* Building)
{
	BuildingOnBreach = Building;
	const auto OldBuildingOnBreach = bBuildingOnBreach;
	bBuildingOnBreach = BuildingOnBreach != nullptr;
	
	SetBuildingOnBreachMulticast(Building, bBuildingOnBreach);
	bLocked = bBuildingOnBreach;
	LockBreachMulticast(bLocked);

	if (!bBuildingOnBreach) return;

	BuildingOnBreach->BuildingMadePermanent.AddDynamic(this, &ABreach::OnBuildingMadePermanent);
	BuildingOnBreach->BuildingDestroyed.AddDynamic(this, &ABreach::OnBuildingDestroyed);

	
}

void ABreach::DestroyBuildingOnBreach(const bool bDestroyedByUnit)
{
	if (BuildingOnBreach == nullptr) return;
	
	BuildingOnBreach->Destroy(bDestroyedByUnit);
	SetBuildingOnBreach(nullptr);
}

AMainBuilding* ABreach::GetMainBuilding() const
{
	return MainBuilding;
}

void ABreach::SetMainBuilding(AMainBuilding* NewMainBuilding)
{
	if (NewMainBuilding == nullptr) return;
	MainBuilding = NewMainBuilding;
}

void ABreach::SetMainBuildingOnClients(AMainBuilding* NewMainBuilding)
{
	SetMainBuildingMulticast(NewMainBuilding);
}

ABuilding* ABreach::GetBuildingOnBreach() const
{
	return BuildingOnBreach;
}

void ABreach::SetMesh(UStaticMesh* NewMesh)
{
	SetMeshMulticast(NewMesh);
}

void ABreach::SetIsBreach(bool bIsBreach)
{
	SetBreachMulticast(bIsBreach);
}

void ABreach::SetBuildingSpawnOffset(FVector NewOffset)
{
	BuildingSpawnOffset = NewOffset;
}
