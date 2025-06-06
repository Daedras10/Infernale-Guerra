// Fill out your copyright notice in the Description page of Project Settings.


#include "UnitAsActor/UnitActorParent.h"

// Sets default values
AUnitActorParent::AUnitActorParent()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AUnitActorParent::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AUnitActorParent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

