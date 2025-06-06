// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/ActorComponents/SpawnerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "LD/Buildings/Building.h"
#include "LD/Buildings/BuildingParent.h"
#include "Manager/UnitActorManager.h"
#include "NiagaraFunctionLibrary.h"
#include <Net/UnrealNetwork.h>
#include "MassEntityConfigAsset.h"
#include "DataAsset/GameSettingsDataAsset.h"
#include "FunctionLibraries/FunctionLibraryInfernale.h"
#include "LD/Buildings/MainBuilding.h"
#include <Mass/Amalgam/Traits/AmalgamTraitBase.h>

// Sets default values for this component's properties
USpawnerComponent::USpawnerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	// ...
}


const UAmalgamTraitBase* USpawnerComponent::GetAmalgamTrait()
{
	UMassEntityConfigAsset* AmalgamDA = GetEntityTypes()[0].EntityConfig.Get();
	if (!AmalgamDA) return nullptr;
	
	const UMassEntityTraitBase* Trait = AmalgamDA->FindTrait(UAmalgamTraitBase::StaticClass());
	if (!Trait) return nullptr;

	const UAmalgamTraitBase* CastTrait = Cast<UAmalgamTraitBase>(Trait);

	return CastTrait;
}

void USpawnerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USpawnerComponent, MassSpawner);
	DOREPLIFETIME(USpawnerComponent, Building);
}

void USpawnerComponent::SetEnabled(bool bEnabled)
{
	auto WasActive = bCanBeActive;
	bCanBeActive = bEnabled;
	if (!WasActive && bCanBeActive) bShouldNotWait = true;
}

float USpawnerComponent::GetSpawnDelayMult()
{
	return SpawnDelayMult;
}

float USpawnerComponent::GetDefaultSpawnDelay()
{
	return DefaultSpawnDelay;
}

void USpawnerComponent::SetStrengthMultiplier(float NewStrengthMultiplier)
{
	StrengthMultiplier = NewStrengthMultiplier;
}

float USpawnerComponent::GetStrengthMultiplier()
{
	return StrengthMultiplier;
}

// Called when the game starts
void USpawnerComponent::BeginPlay()
{
	Super::BeginPlay();
	bUseMass = UFunctionLibraryInfernale::GetGameSettingsDataAsset()->UseMass;
	SyncDataAsset();
	
	if (!GetOwner()->HasAuthority()) return;
	if (!Enabled)
	{
		// Should remove this component
		return;
	}
	TryGetActorManager();

	const auto BuildingOwner = Cast<ABuilding>(GetOwner());
	if (BuildingOwner)
	{
		BuildingOwner->BuildingOverclocked.AddDynamic(this, &USpawnerComponent::OnBuildingOverclocked);
	}
	BuildingOwner->BuildingConstructed.AddDynamic(this, &USpawnerComponent::OnBuildingConstructed);

	FTransform Transform = GetOwner()->GetActorTransform();
	AActor* Actor = GetWorld()->SpawnActor(AAmalgamSpawnerParent::StaticClass(), &Transform, FActorSpawnParameters());
	checkf(Actor, TEXT("Failed to spawn Actor"));
	MassSpawner = Cast<AAmalgamSpawnerParent>(Actor);
	checkf(MassSpawner, TEXT("Cast to AAmalgamSpawnerParent failed"));
	Building = BuildingOwner;
	MassSpawner->Initialize(this, 1);
	MassSpawner->PostRegister();

	//CallInitSpawner();
	/*FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, this, &USpawnerComponent::CallInitSpawner, 0.5f, false);*/
}

void USpawnerComponent::SyncDataAsset()
{
	if (!UnitActorDataAsset)
	{
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("SpawnerDataAsset is not set"));
		return;
	}
	DefaultSpawnDelay = UnitActorDataAsset->UnitStructs[0].BaseSpawnDelay;
}

void USpawnerComponent::SpawnUnits()
{
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Spawning units"));
	if (bUseMass)
	{
		SpawnUnitsMass();
		return;
	}
	SpawnUnitsActors();
}

void USpawnerComponent::SpawnUnitsMass()
{
	if (!bUseMass) return;
	if (bForceDisable) return;
	// Also check for currently active fluxes somewhere
	const auto Fluxes = Building->GetFluxes();
	auto FluxesNum = Fluxes.Num();
	if (FluxToSpawn > FluxesNum - 1) FluxToSpawn = 0;

	auto FluxesNumActives = 0;
	for (auto Flux : Fluxes)
	{
		if (!Flux.IsValid()) continue;
		if (!Flux.Get()->IsFluxActive()) continue;
		FluxesNumActives++;
	}
	if (FluxesNumActives == 0)
    {
		SpawnTimer = 0;
		FluxToSpawn++;
        return;
    }
	
	auto Safety = 0;
	while (Safety < 20)
	{
		const auto Flux = Fluxes[FluxToSpawn].Get();
		if (Flux->IsFluxActive()) break;
		FluxToSpawn++;
		if (FluxToSpawn > FluxesNum - 1) FluxToSpawn = 0;
		Safety++;
	}
	
	MassSpawner->DoAmalgamSpawning(FluxToSpawn);

	SpawnTimer = 0;
	FluxToSpawn++;
}

void USpawnerComponent::SpawnUnitsActors()
{
	UnitActorManager->SpawnUnit(Building);
	SpawnTimer = 0;
}

void USpawnerComponent::CallOnFluxUpdated()
{
	OnFluxUpdated(Building);
}

void USpawnerComponent::AddToBuildings()
{
	Building = Cast<ABuildingParent>(GetOwner());
	if (!Building) return;
	auto MainBuilding = Building->GetMainBuilding();
	if (!MainBuilding) return;
	MainBuilding->AddSpawnerComponents(this);
}

void USpawnerComponent::RemoveFromBuildings()
{
	Building = Cast<ABuildingParent>(GetOwner());
	if (!Building) return;
	auto MainBuilding = Building->GetMainBuilding();
	if (!MainBuilding) return;
	MainBuilding->RemoveSpawnerComponents(this);
}

void USpawnerComponent::TryGetActorManager()
{
	auto UnitActorManagerArray = TArray<AActor*>();
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), UnitActorManagerClass, UnitActorManagerArray);
	if (UnitActorManagerArray.Num() == 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Spawner Component Error : \n\t Unable to find UnitActorManager"));
		return;
	}
	UnitActorManager = Cast<AUnitActorManager>(UnitActorManagerArray[0]);
	if (!UnitActorManager)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Spawner Component Error : \n\t Unable to cast UnitActorManager"));
	}
}

void USpawnerComponent::OnBuildingConstructedMulticast_Implementation(ABuildingParent* InBuilding, AAmalgamSpawnerParent* InSpawner)
{
	Building = InBuilding;

	TStrongObjectPtr<AAmalgamSpawnerParent> SPtrSpawner = static_cast<TStrongObjectPtr<AAmalgamSpawnerParent>>(InSpawner);

	/*if (bUseMass) SPtrSpawner.Get()->Initialize(this, 1);
	if (GetOwner()->HasAuthority()) return;
	if (bUseMass) SPtrSpawner.Get()->PostRegister();*/
}

void USpawnerComponent::OnSpawnCreateNiagaraComponent_Implementation(UNiagaraSystem* NiagaraSystem, FVector Location, AAmalgamSpawnerParent* InSpawner)
{
	if (GetOwner()->HasAuthority()) return;

	if (!InSpawner)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, TEXT("Spawner is null"));
		return;
	}

	UNiagaraComponent* SpawnedNC = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), NiagaraSystem, Location);
	// Spawning works, but referencing is broken because the referenced spawner only exists on server
	if (bUseMass) InSpawner->SetNiagaraComponent(SpawnedNC);
}

void USpawnerComponent::ReplicateSpawn_Implementation(AAmalgamSpawnerParent* InSpawner, int Index)
{
	InSpawner->DoAmalgamSpawning(Index);
}

void USpawnerComponent::OnFluxUpdated(ABuildingParent* BuildingUpdated)
{
	const TArray<TWeakObjectPtr<AFlux>> Fluxes = Building->GetFluxes();
	if (bUseMass)
	{
		MassSpawner->ReplaceWithFluxes(Fluxes);
		bCanBeActive &= MassSpawner->IsEnabled();
		MassSpawner->UpdateOwner();
	}
}

void USpawnerComponent::OnBuildingOverclocked(ABuilding* BuildingOverclocked)
{
	TWeakObjectPtr<ABuildingParent> BuildingOverclockedPtr = TWeakObjectPtr<ABuildingParent>(BuildingOverclocked);
	if (!BuildingOverclockedPtr.IsValid()) return;
	
	SpawnDelayMult = BuildingOverclocked->GetBuildingInfo().OverclockSpawnMultiplier;
	SpawnDelay = BaseSpawnDelay * SpawnDelayMult;
	
	Building = Cast<ABuildingParent>(GetOwner());
	if (!Building) return;
	auto MainBuilding = Building->GetMainBuilding();
	if (!MainBuilding) return;
	MainBuilding->AskUpdateSpawners();
}

void USpawnerComponent::OnBuildingConstructed(ABuilding* BuildingConstructed)
{
	Building = Cast<ABuildingParent>(GetOwner());
	
	//Building->BuildingParentFluxesUpdated.AddDynamic(this, &USpawnerComponent::OnFluxUpdated);
	if (!MassSpawner)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Spawner Component Error : \n\t Unable to spawn Amalgam Spawner"));
		return;
	}

	//Owner team is always NatureTeam
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Spawner Component : \n\t Team %d"), Building->GetOwner().Team));
	//Spawner->Initialize(this, 1);
	
	// const auto FluxesNum = Building->GetFluxes().Num();
	// bCanBeActive = FluxesNum > 0;
	// if (bCanBeActive) SpawnTimer = (FluxesNum * DefaultSpawnDelay) * 0.9;
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, TEXT("Spawner Component no Error"));
	
	// Real thing
	//CallOnFluxUpdated();
	
	AddToBuildings();
}

void USpawnerComponent::CallInitSpawner()
{
	OnBuildingConstructedMulticast(Building, MassSpawner);
}


// Called every frame
void USpawnerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (bForceDisable) return;
	
	if (!GetOwner()->HasAuthority())
		return;

	if (!bCanBeActive)
		return;

	SpawnTimer += DeltaTime;
	if (SpawnTimer > SpawnDelay)
	{
		SpawnUnits();
	}
}

void USpawnerComponent::DestroyComponent(bool bPromoteChildren)
{
	Super::DestroyComponent(bPromoteChildren);

	if (!GetOwner()->HasAuthority()) return;
	RemoveFromBuildings();
	
	if (!MassSpawner) return;

	MassSpawner->Destroy();
}

void USpawnerComponent::SetSpawnDelay(float NewSpawnDelay)
{
	SpawnDelay = NewSpawnDelay;
	if (bShouldNotWait)
	{
		if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("SpawnerComponent : reactivate"));
		SpawnTimer = SpawnDelay * 0.9;
		bShouldNotWait = false;
		FluxToSpawn = 0;
	}
}

void USpawnerComponent::SetSpawnerNumbers(int NumberOfSpawners)
{
	MassSpawner->SetNumberOfSpawners(NumberOfSpawners);
}

void USpawnerComponent::DisableSpawning()
{
	bForceDisable = true;
}

