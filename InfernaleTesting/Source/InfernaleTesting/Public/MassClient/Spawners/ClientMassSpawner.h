// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassSpawner.h"
#include "Structs/SimpleStructs.h"
#include "ClientMassSpawner.generated.h"


class AFlux;

USTRUCT()
struct FMassClientInitializeInfo
{
	GENERATED_BODY()

public:
	UPROPERTY() FOwner Owner = FOwner();
	UPROPERTY() EEntityType EntityType = EEntityType::EntityTypeNone;
	UPROPERTY() FVector Location = FVector::ZeroVector;
	UPROPERTY() FRotator Rotation = FRotator::ZeroRotator;
	UPROPERTY() AFlux* Flux = nullptr;
	
};


/**
 * 
 */
UCLASS()
class INFERNALETESTING_API AClientMassSpawner : public AMassSpawner
{
	GENERATED_BODY()

public:
	AClientMassSpawner();

	void Initialize(const TArray<FMassSpawnedEntityType>& NewEntityTypes, const TArray<FMassSpawnDataGenerator>& NewSpawnDataGenerators);
	void DoClientSpawning();
	void PostRegister();


protected:
	virtual void BeginPlay() override;
	virtual void PostRegisterAllComponents() override;
	


protected:
	bool bDebug = false;
};
