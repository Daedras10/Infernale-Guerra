// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UnitActorDataAsset.generated.h"

class ANiagaraUnitAsActor;
enum class VisionType : uint8;
class AUnitActor;

USTRUCT(Blueprintable)
struct FUnitStruct
{
	GENERATED_BODY()
public:
	FUnitStruct();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString Name;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float BaseHealth;
	

	// Should be separate struct so we can have an array of effects
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float BaseDamage;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float BaseAttackCD;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float BaseRange;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float BaseSpeed;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float BaseRushSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TargetableRange = 50.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int BaseMaxAttackers = 5;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float BaseDetectionRange;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float BaseDetectionAngle;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float BaseSightRange;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float BaseSightAngle;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	VisionType BaseSightType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float NiagaraHeightOffset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector NiagaraRotationOffset;

	/* The Radius of acceptance a unit should consider as being close enough to a node in it's pathfinding before going to the next */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float AcceptancePathfindingRadius = 100.0f;

	/* The Radius at which a unit stops going after a target (will likely be split by opponent type (Building, Unit, Monsters) or added to it's opponent */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float AcceptanceRadiusAttack = 100.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<AUnitActor> UnitActorClass;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float BaseSpawnDelay = 1.5f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<TSubclassOf<ANiagaraUnitAsActor>> UnitNiagaraActorClasses;
};

/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UUnitActorDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UUnitActorDataAsset();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FUnitStruct> UnitStructs = TArray<FUnitStruct>();
	
};
