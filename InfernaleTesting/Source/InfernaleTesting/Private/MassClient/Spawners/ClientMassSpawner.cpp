// Fill out your copyright notice in the Description page of Project Settings.


#include "MassClient/Spawners/ClientMassSpawner.h"

AClientMassSpawner::AClientMassSpawner()
{
	bAutoSpawnOnBeginPlay = false;
}

void AClientMassSpawner::Initialize(const TArray<FMassSpawnedEntityType>& NewEntityTypes,
	const TArray<FMassSpawnDataGenerator>& NewSpawnDataGenerators)
{
	EntityTypes = NewEntityTypes;
	SpawnDataGenerators = NewSpawnDataGenerators;
	Count = 1;
	bAutoSpawnOnBeginPlay = false;
}

void AClientMassSpawner::DoClientSpawning()
{
	if (bDebug) 
	{
		UE_LOG(LogTemp, Warning, TEXT("AClientMassSpawner::DoSpawning() called"));
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("AClientMassSpawner::DoSpawning() called"));
	}
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Client Spawning %d entities"), Count));
	DoSpawning();
}

void AClientMassSpawner::PostRegister()
{
	PostRegisterAllComponents();
}

void AClientMassSpawner::BeginPlay()
{
	Super::BeginPlay();
}

void AClientMassSpawner::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();
}
