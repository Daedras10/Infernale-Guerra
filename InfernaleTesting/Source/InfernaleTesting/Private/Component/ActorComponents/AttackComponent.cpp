// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/ActorComponents/AttackComponent.h"


// Sets default values for this component's properties
UAttackComponent::UAttackComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


void UAttackComponent::SetStarted(bool NewStarted)
{
	if (NewStarted == Started) return;
	if (NewStarted) InitAttacks();
	Started = NewStarted;
}

// Called when the game starts
void UAttackComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UAttackComponent::InitAttacks()
{
	for (auto& Attack : AttackStructs)
	{
		Attack.AttackCDLocal = Attack.AttackCD;
	}
}


// Called every frame
void UAttackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Started) return;

	if (AttackStructs.Num() == 0)
		UE_DEBUG_BREAK();

	for (auto& Attack : AttackStructs)
	{
		Attack.AttackCDLocal -= DeltaTime;
		if (Attack.AttackCDLocal <= 0)
		{
			Attack.AttackCDLocal = Attack.AttackCD;
			if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Cyan, TEXT("Building attack!"));
			AttackReadyDelegate.Broadcast(Attack);
		}
	}
}

void UAttackComponent::SetAttacks(TArray<FAttackStruct> NewAttacks)
{
	AttackStructs = NewAttacks;
}

