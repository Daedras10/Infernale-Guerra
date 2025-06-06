// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameMode/GameInstanceInfernale.h"
#include "Structs/SimpleStructs.h"
#include "TransmutationDataAsset.generated.h"


USTRUCT(Blueprintable)
struct FTransmutationNode
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "0 - ID")
	FString ID;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "0 - ID")
	TArray<FString> NodeIDsPrerequisit = TArray<FString>();
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "0 - ID")
	TArray<FString> NodeIDsLinks = TArray<FString>();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "1 - Visual")
	FTransmutationNodeVisualInfo PositionInfo;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "1 - Visual")
	FString IconPath;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "2 - Description")
	FLocalizedText Name;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "2 - Description")
	FLocalizedText Description;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "3 - Effects")
	TArray<FNodeEffect> Effects;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "3 - Effects")
	float ActivationTime = 1.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "3 - Effects")
	float DeactivationTime = 30.f;
};


USTRUCT(Blueprintable)
struct FTransmutationSimpleNode
{
	GENERATED_BODY()
public:
	FTransmutationSimpleNode();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "0 - ID") FString ID;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "1 - Visual") UTexture2D* Icon;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "1 - Visual") UTexture2D* IconBought;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "2 - Description") FLocalizedText Name;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "2 - Description") FLocalizedText Description;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "3 - Effects") FSimpleNodeEffect Effect;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "3 - Effects") float ActivationTime = 1.f;
	//UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "3 - Effects") float DeactivationTime = 30.f;
};




/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UTransmutationDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FTransmutationSettings TransmutationSettings;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) TArray<FTransmutationNode> TransmutationNodes = TArray<FTransmutationNode>();
	UPROPERTY(BlueprintReadWrite, EditAnywhere) TArray<FTransmutationSimpleNode> SimpleTransmutationNodes = TArray<FTransmutationSimpleNode>();
};
