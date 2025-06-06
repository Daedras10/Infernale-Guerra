// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/ActorComponents/DestroyedAfterDurationComponent.h"

// Sets default values for this component's properties
UDestroyedAfterDurationComponent::UDestroyedAfterDurationComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UDestroyedAfterDurationComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
}

void UDestroyedAfterDurationComponent::DestroyAfterDuration(float Duration)
{
	if (!bStarted) return;
	
	LifeTimeDuration = Duration;
	CurrentDuration = LifeTimeDuration;
	bStarted = true;
	BuildingLifeStarted.Broadcast(Building.Get());
}


// Called every frame
void UDestroyedAfterDurationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bStarted) return;

	CurrentDuration -= DeltaTime * SpeedModifier;
	LifeTimeReduced.Broadcast(CurrentDuration, LifeTimeDuration);
	if (CurrentDuration > 0) return;

	BuildingDestroyed.Broadcast(Building.Get());
	bStarted = false;
}

void UDestroyedAfterDurationComponent::SetDuration(float Duration, const float NewSpeedModifier)
{
	LifeTimeDuration = Duration;
	CurrentDuration = LifeTimeDuration;
	SpeedModifier = NewSpeedModifier;
}

void UDestroyedAfterDurationComponent::SetDurationKeepProgress(float Duration, const float NewSpeedModifier)
{
	auto Progress = CurrentDuration / LifeTimeDuration;
	CurrentDuration = Duration * Progress;
	LifeTimeDuration = Duration;
	SpeedModifier = NewSpeedModifier;
}

void UDestroyedAfterDurationComponent::StartDestroying()
{
	bStarted = true;
	BuildingLifeStarted.Broadcast(Building.Get());
}

void UDestroyedAfterDurationComponent::StopDestroying()
{
	bStarted = false;
}

