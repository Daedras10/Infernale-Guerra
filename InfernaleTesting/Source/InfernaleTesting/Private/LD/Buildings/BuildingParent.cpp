// Fill out your copyright notice in the Description page of Project Settings.


#include "LD/Buildings/BuildingParent.h"

#include "Component/ActorComponents/DamageableComponent.h"
#include "Flux/Flux.h"
#include "FunctionLibraries/FunctionLibraryInfernale.h"
#include "Net/UnrealNetwork.h"
#include <Mass/Collision/SpatialHashGrid.h>

#include "Component/ActorComponents/UnitActorGridComponent.h"
#include "DataAsset/BuildingEffectDataAsset.h"
#include "DataAsset/GameSettingsDataAsset.h"
#include "FogOfWar/FogOfWarManager.h"
#include "GameMode/Infernale/GameModeInfernale.h"
#include "GameMode/Infernale/PlayerStateInfernale.h"
#include "Kismet/GameplayStatics.h"
#include "Manager/UnitActorManager.h"

// Sets default values
ABuildingParent::ABuildingParent()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent->Mobility = EComponentMobility::Static;
	DamageableComponent = CreateDefaultSubobject<UDamageableComponent>(TEXT("DamageableComponent"));
	
	FogOfWarComponent = CreateDefaultSubobject<UFogOfWarComponent>(TEXT("FogOfWarComponent"));
}

FOwner ABuildingParent::GetOwner()
{
	return OwnerWithTeam;
}

void ABuildingParent::SetOwner(FOwner NewOwner)
{
	auto OldOwner = OwnerWithTeam;
	OwnerWithTeam = NewOwner;
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Set Building Owner : %d"), OwnerWithTeam.Team));
	UpdateOwnerVisuals(this, OldOwner, OwnerWithTeam);
	BuildingOwnershipChanged.Broadcast(this, OldOwner, OwnerWithTeam);
	SetOwnerMulticast(NewOwner);
}

void ABuildingParent::ChangeOwner(FOwner NewOwner)
{
	SetOwner(NewOwner);
	FogOfWarOwnershipChanged();
}

float ABuildingParent::DamageHealthOwner(const float DamageAmount, const bool bDamagePercent, const FOwner DamageOwner)
{
	return DamageableComponent->DamageHealthOwner(DamageAmount, bDamagePercent, DamageOwner);
}

float ABuildingParent::GetTargetableRange()
{
	return TargetableRange;
}

bool ABuildingParent::CanCreateAFlux() const
{
	return false;
}

float ABuildingParent::GetOffsetRange()
{
	return OffsetRange;
}

float ABuildingParent::GetAttackOffsetRange()
{
	return AttackOffsetRange;
}

float ABuildingParent::GetRepulsorRange() const
{
	return RepulsorRange;
}

AMainBuilding* ABuildingParent::GetMainBuilding()
{
	return nullptr;
}

void ABuildingParent::ApplyBuildingEffect(ABuilding* Source, FBuildingEffect BuildingEffect, bool UseOverclockEffect)
{
}

void ABuildingParent::RemoveBuildingEffect(ABuilding* Source, FBuildingEffect BuildingEffect, bool UpdateVisual)
{
}

void ABuildingParent::AddFlux(TWeakObjectPtr<AFlux> Flux)
{
	Fluxes.Add(Flux);
	Flux->FluxFinishUpdate.AddUniqueDynamic(this, &ABuildingParent::OnFluxUpdated);
	Flux->FluxDestroyed.AddUniqueDynamic(this, &ABuildingParent::RemoveFlux);
	BroadcastBuildingParentFluxesUpdated(this);
	//GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Yellow, FString::Printf(TEXT("Building Parent : Call event")));
}

void ABuildingParent::RemoveFluxDelegate(TWeakObjectPtr<AFlux> Flux)
{
	Flux->FluxFinishUpdate.RemoveDynamic(this, &ABuildingParent::OnFluxUpdated);
	Flux->FluxDestroyed.RemoveDynamic(this, &ABuildingParent::RemoveFlux);
}

void ABuildingParent::SetTransmutationComponent(TWeakObjectPtr<UTransmutationComponent> NewTransmutationComponent)
{
	LocalTransmutationComponent = NewTransmutationComponent;
}

void ABuildingParent::UpdateMaxHealth(float NewMaxHealth)
{
	DamageableComponent->SetMaxHealthKeepPercent(NewMaxHealth);
}

bool ABuildingParent::IsMainBuilding() const
{
	return false;
}

float ABuildingParent::GetThornDamage() const
{
	return 0.0f;
}

void ABuildingParent::InteractStartHover(APlayerControllerInfernale* Interactor)
{
	Hovered.Broadcast(Interactor, true);
}

void ABuildingParent::InteractEndHover(APlayerControllerInfernale* Interactor)
{
	Hovered.Broadcast(Interactor, false);
}

UDamageableComponent* ABuildingParent::GetDamageableComponent()
{
	return DamageableComponent;
}

UFogOfWarComponent* ABuildingParent::GetFogOfWarComponent() const
{
	return FogOfWarComponent;
}

int ABuildingParent::GetId()
{
	return StaticBuildID;
}

void ABuildingParent::SetId(int ID)
{
	StaticBuildID = ID;
}

FString ABuildingParent::GetBuildingName()
{
	return "Default Building";
}

TArray<AFlux*> ABuildingParent::GetFluxesArrayBP()
{
	TArray<AFlux*> FluxesArray;
	for (auto Flux : Fluxes)
	{
		if (Flux.IsValid())
		{
			FluxesArray.Add(Flux.Get());
		}
	}
	return FluxesArray;
}

TArray<TWeakObjectPtr<AFlux>> ABuildingParent::GetFluxes()
{
	//GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Yellow, FString::Printf(TEXT("Building Parent : \n\t #of Fluxes : %d"), Fluxes.Num()));
	return Fluxes;
}

// Called when the game starts or when spawned
void ABuildingParent::BeginPlay()
{
	Super::BeginPlay();
	GameSettings = UFunctionLibraryInfernale::GetGameSettingsDataAsset();
	if (!DamageableComponent)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Building Parent : No Damageable Component"));
		return;
	}
	DamageableComponent->HealthDepelted.AddUniqueDynamic(this, &ABuildingParent::OnHealthDepleted);
	DamageableComponent->Damaged.AddUniqueDynamic(this, &ABuildingParent::OnDamageTaken);
	DamageableComponent->HealthDepeltedActor.AddUniqueDynamic(this, &ABuildingParent::OnHealthDepletedActor);
	DamageableComponent->HealthDepeltedOwner.AddUniqueDynamic(this, &ABuildingParent::OnHealthDepletedOwner);
	
	if (!HasAuthority()) return;

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ABuildingParent::RegisterBuilding, 0.1f, false);

	auto PlayerState = GetWorld()->GetFirstPlayerController()->GetPlayerState<APlayerStateInfernale>();
	auto ThisPlayerTeam = PlayerState->GetOwnerInfo().Team;
	FogOfWarComponent->SetVisibilityOfActorWithFog(OwnerWithTeam.Team == ThisPlayerTeam ? true : false, true);
}

void ABuildingParent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (!DamageableComponent) return;
	DamageableComponent->HealthDepelted.RemoveDynamic(this, &ABuildingParent::OnHealthDepleted);
	DamageableComponent->Damaged.RemoveDynamic(this, &ABuildingParent::OnDamageTaken);
	DamageableComponent->HealthDepeltedActor.RemoveDynamic(this, &ABuildingParent::OnHealthDepletedActor);
	DamageableComponent->HealthDepeltedOwner.RemoveDynamic(this, &ABuildingParent::OnHealthDepletedOwner);
}

void ABuildingParent::RegisterBuilding()
{
	const auto World = GetWorld();
	const auto GameModeInfernale = Cast<AGameModeInfernale>(World->GetAuthGameMode());
	auto UnitActorManagerArray = TArray<AActor*>();

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), GameModeInfernale->GetUnitActorManagerClass(), UnitActorManagerArray);
	UnitActorManager = Cast<AUnitActorManager>(UnitActorManagerArray[0]);
	//UnitActorManager->GetGridComponent()->AddBuilding(TWeakObjectPtr<ABuildingParent>(this));
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Green, FString::Printf(TEXT("Building Parent : Register Building")));
}

void ABuildingParent::FogOfWarOwnershipChanged()
{
	bUseForgOfWar = UFunctionLibraryInfernale::GetGameSettingsDataAsset()->UseFogOfWar;
	FogOfWarManagerClass = UFunctionLibraryInfernale::GetGameSettingsDataAsset()->FogOfWarManagerClass;
	if (!bUseForgOfWar) return;
	auto LocalPlayer = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	auto LocalPlayerInfernale = Cast<APlayerControllerInfernale>(LocalPlayer);
	if (LocalPlayerInfernale->GetOwnerInfo().Team != OwnerWithTeam.Team) return;

	auto ActorArray = TArray<AActor*>();
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), FogOfWarManagerClass, ActorArray);
	if (ActorArray.Num() == 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("AUnitActor Error : \n\t Unable to find FogOfWarManager"));
		return;
	}
	const auto FogManager = Cast<AFogOfWarManager>(ActorArray[0]);
	if (!FogManager)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("AUnitActor Error : \n\t Unable to cast FogOfWarManager"));
		return;
	}
	if (bDebugOwnerShip) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Building Parent : Change Owner")));
	FogManager->FindAllBaseOfTeam(OwnerWithTeam.Team);
}

void ABuildingParent::BroadcastBuildingParentFluxesUpdated(ABuildingParent* _)
{
	BuildingParentFluxesUpdated.Broadcast(this);
}

void ABuildingParent::OnFluxUpdated(AFlux* Flux)
{
	BuildingParentFluxesUpdated.Broadcast(this);
}

void ABuildingParent::RemoveFlux(AFlux* FluxPtr)
{
	TWeakObjectPtr<AFlux> Flux = TWeakObjectPtr<AFlux>(FluxPtr);
	Fluxes.Remove(Flux);
	Flux->FluxDestroyed.RemoveDynamic(this, &ABuildingParent::RemoveFlux);
	Flux->FluxFinishUpdate.RemoveDynamic(this, &ABuildingParent::OnFluxUpdated);
	BuildingParentFluxesUpdated.Broadcast(this);
}

void ABuildingParent::UpdateOwnerVisuals(ABuildingParent* Actor, FOwner OldOwner, FOwner NewOwner)
{
	Execute_NewOwnerVisuals(this, Actor, OldOwner, NewOwner);
}

void ABuildingParent::OnHealthDepleted()
{
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Building Health Depleted"));
}

void ABuildingParent::OnHealthDepletedActor(AActor* Actor, AActor* DepleterActor)
{
	const auto DepleterActorOwner = Cast<IOwnable>(DepleterActor);
	if (DepleterActorOwner == nullptr)
	{
		OnHealthDepleted();
		return;
	}

	const auto OwnerDepleter = DepleterActorOwner->GetOwner();
	OnHealthDepletedOwner(Actor, OwnerDepleter);
}

void ABuildingParent::OnHealthDepletedOwner(AActor* Actor, FOwner Depleter)
{
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Building Health Depleted by Owner"));

	TMap<FMassEntityHandle, GridCellEntityData> DataMap = ASpatialHashGrid::FindEntitiesInRange(GetActorLocation(), BlowUpRadius, 360.f, FVector(0.f,0.f,0.f), FMassEntityHandle(0,0));
	TArray<FMassEntityHandle> Keys;
	DataMap.GenerateKeyArray(Keys);
	
	for (auto Key : Keys)
	{
		ASpatialHashGrid::GetMutableEntityData(Key)->EntityHealth = 0.f;
	}

}

void ABuildingParent::OnDamageTaken(AActor* Actor, float NewHealth, float DamageAmount)
{
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Building Health Reduced, %f health remaining"), DamageableComponent->GetHealth()));
}

void ABuildingParent::SetOwnerMulticast_Implementation(FOwner NewOwner)
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Building Parent : Set Owner Multicast : %d"), NewOwner.Team));
	const auto OldOwner = OwnerWithTeam;
	if (!HasAuthority()) OwnerWithTeam = NewOwner;
	UpdateOwnerVisuals(this, OwnerWithTeam, NewOwner);
	LocalBuildingOwnershipChanged.Broadcast(this, OldOwner, OwnerWithTeam);
}

// Called every frame
void ABuildingParent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

