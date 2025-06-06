// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/PlayerState/PlayerStateComponent.h"

#include "GameMode/Infernale/PlayerStateInfernale.h"

// Sets default values for this component's properties
UPlayerStateComponent::UPlayerStateComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UPlayerStateComponent::BeginPlay()
{
	Super::BeginPlay();
	PlayerStateInfernale = Cast<APlayerStateInfernale>(GetOwner());
	// ...
	
}


// Called every frame
void UPlayerStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

