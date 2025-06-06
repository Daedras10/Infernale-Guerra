// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "ReplicationStructs.generated.h"

/**
 * 
 */

class APlayerControllerInfernale;

USTRUCT()
struct FDataForVisualisation
{
	GENERATED_USTRUCT_BODY()
public:
	FDataForVisualisation();
	
public:
	UPROPERTY() FMassEntityHandle EntityHandle;
	UPROPERTY() float LocationX;
	UPROPERTY() float LocationY;
	UPROPERTY() float RotationX;
	UPROPERTY() float RotationY;
};

USTRUCT()
struct FVisualDataItemEntry : public FFastArraySerializerItem
{
	GENERATED_USTRUCT_BODY()
public:
	FVisualDataItemEntry();

public:
	UPROPERTY() FMassEntityHandle EntityHandle;
	UPROPERTY() float LocationX;
	UPROPERTY() float LocationY;
	UPROPERTY() float RotationX;
	UPROPERTY() float RotationY;
	
	// void PreReplicatedRemove();
	// void PostReplicatedAdd();
	// void PostReplicatedChange();
};

USTRUCT()
struct FVisualDataArray : public FFastArraySerializer
{
	GENERATED_USTRUCT_BODY()
public:
	FVisualDataArray();

public:
	UPROPERTY() TArray<FVisualDataItemEntry> Items;
};
