// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/PlayerController/PlayerControllerComponent.h"

#include "GameMode/Infernale/PlayerControllerInfernale.h"

// Sets default values for this component's properties
UPlayerControllerComponent::UPlayerControllerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UPlayerControllerComponent::BeginPlay()
{
	Super::BeginPlay();

	auto Owner = GetOwner();
	if (!Owner) return;

	auto PlayerController = Cast<APlayerControllerInfernale>(Owner);
	if (!PlayerController)
	{
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("Component Owner is not a PlayerControllerInfernale"));
		return;
	}

	PlayerControllerInfernale = PlayerController;
}


// Called every frame
void UPlayerControllerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

