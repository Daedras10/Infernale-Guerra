// Fill out your copyright notice in the Description page of Project Settings.


#include "LD/LDElement/LDElement.h"

#include "Component/ActorComponents/UnitActorGridComponent.h"
#include "GameMode/Infernale/GameModeInfernale.h"
#include "Kismet/GameplayStatics.h"
#include "Manager/UnitActorManager.h"
#include <Mass/Collision/SpatialHashGrid.h>

#include "FunctionLibraries/FunctionLibraryInfernale.h"

// Sets default values
ALDElement::ALDElement()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

}

// Called when the game starts or when spawned
void ALDElement::BeginPlay()
{
	Super::BeginPlay();
	GameSettings = UFunctionLibraryInfernale::GetGameSettingsDataAsset();
	if (!HasAuthority()) return;
	
	RegisterLDElement();
}

void ALDElement::RegisterLDElement()
{
	const auto World = GetWorld();
	const auto GameModeInfernale = Cast<AGameModeInfernale>(World->GetAuthGameMode());
	auto UnitActorManagerArray = TArray<AActor*>();

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), GameModeInfernale->GetUnitActorManagerClass(), UnitActorManagerArray);
	const auto UnitActorManager = Cast<AUnitActorManager>(UnitActorManagerArray[0]);
	//UnitActorManager->GetGridComponent()->AddLDElement(TWeakObjectPtr<ALDElement>(this));
	ASpatialHashGrid::AddLDElementToGrid(GetActorLocation(), this);
}

// Called every frame
void ALDElement::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool ALDElement::IsAttackableBy(ETeam Team)
{
	return true;
}

EEntityType ALDElement::GetEntityType() const
{
	return EEntityType::EntityTypeNone;
}

void ALDElement::InteractStartHover(APlayerControllerInfernale* Interactor)
{
	Hovered.Broadcast(Interactor, true);
}

void ALDElement::InteractEndHover(APlayerControllerInfernale* Interactor)
{
	Hovered.Broadcast(Interactor, false);
}

ELDElementType ALDElement::GetLDElementType() const
{
	return LDElementType;
}

