// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/ActorComponents/ConstructWithDelayComponent.h"

#include "Component/ActorComponents/DamageableComponent.h"

// Sets default values for this component's properties
UConstructWithDelayComponent::UConstructWithDelayComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UConstructWithDelayComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UConstructWithDelayComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bStarted) return;

	ConstructionTime -= DeltaTime * ConstructionSpeedModifier;

	auto TimeRemaining = InitialConstructionTime - ConstructionTime;
	auto Mult = TimeRemaining / InitialConstructionTime;
	
	DamageableComponent->HealHealth(Mult, true);
	ConstructionTimeRemaining.Broadcast(ConstructionTime, InitialConstructionTime);

	if (ConstructionTime > 0) return;

	ConstructionFinished.Broadcast(GetOwner());
	bStarted = false;
}

void UConstructWithDelayComponent::SetConstructionTime(const float Duration, const float SpeedModifier)
{
	InitialConstructionTime = Duration;
	ConstructionSpeedModifier = SpeedModifier;
	ConstructionTime = InitialConstructionTime;
	ConstructionTimeRemaining.Broadcast(ConstructionTime, InitialConstructionTime);
}

void UConstructWithDelayComponent::SetConstructionTimeKeepProgress(const float Duration, const float SpeedModifier)
{
	auto Progress = ConstructionTime / InitialConstructionTime;
	InitialConstructionTime = Duration;
	ConstructionSpeedModifier = SpeedModifier;
	ConstructionTime = InitialConstructionTime * Progress;
	ConstructionTimeRemaining.Broadcast(ConstructionTime, InitialConstructionTime);
}

void UConstructWithDelayComponent::StartConstruction()
{
	bStarted = true;
	ConstructionStarted.Broadcast(GetOwner());
}

void UConstructWithDelayComponent::LinkToDamageableComponent(TWeakObjectPtr<UDamageableComponent> NewDamageableComponent)
{
	this->DamageableComponent = NewDamageableComponent;
}

float UConstructWithDelayComponent::GetConstructionTimeRemaining() const
{
	return ConstructionTime;
}

