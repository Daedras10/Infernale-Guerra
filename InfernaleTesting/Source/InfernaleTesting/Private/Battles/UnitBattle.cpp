// Fill out your copyright notice in the Description page of Project Settings.


#include "Battles/UnitBattle.h"

// Sets default values
AUnitBattle::AUnitBattle()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

FVector2D AUnitBattle::GetLocation() const
{
	return FVector2D(GetActorLocation().X, GetActorLocation().Y);
}

// Called when the game starts or when spawned
void AUnitBattle::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AUnitBattle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

