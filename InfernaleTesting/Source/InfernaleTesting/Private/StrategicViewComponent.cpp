// Fill out your copyright notice in the Description page of Project Settings.


#include "StrategicViewComponent.h"

// Sets default values for this component's properties
UStrategicViewComponent::UStrategicViewComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UStrategicViewComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UStrategicViewComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

// bool UStrategicViewComponent::MatchFlags(const int32 Mask)
// {
// 	return (Mask & LayerFlags) == Mask;
// }
