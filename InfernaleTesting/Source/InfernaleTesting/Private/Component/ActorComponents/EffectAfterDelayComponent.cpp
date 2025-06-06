// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/ActorComponents/EffectAfterDelayComponent.h"

// Sets default values for this component's properties
UEffectAfterDelayComponent::UEffectAfterDelayComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UEffectAfterDelayComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UEffectAfterDelayComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bStarted) return;

	Time -= DeltaTime;
	TimeRemainingDelegate.Broadcast(Time, InitialTime);

	if (Time > 0) return;

	FinishedDelegate.Broadcast(GetOwner());
	bStarted = false;
	
}

void UEffectAfterDelayComponent::SetTime(const float Duration)
{
	InitialTime = Duration;
	Time = InitialTime;
	TimeRemainingDelegate.Broadcast(Time, InitialTime);
}

void UEffectAfterDelayComponent::Start()
{
	bStarted = true;
	StartedDelegate.Broadcast(GetOwner());
}

void UEffectAfterDelayComponent::StartFromBegining()
{
	Time = InitialTime;
	bStarted = true;
	StartedDelegate.Broadcast(GetOwner());
}

void UEffectAfterDelayComponent::Stop()
{
	bStarted = false;
	StoppedDelegate.Broadcast(GetOwner());
}

float UEffectAfterDelayComponent::GetTimeRemaining() const
{
	return Time;
}

float UEffectAfterDelayComponent::GetInitalTime() const
{
	return InitialTime;
}

