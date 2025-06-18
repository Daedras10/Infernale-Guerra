// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "ClientInitialiseProcessor.generated.h"

class AAmalgamVisualisationManager;
/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UClientInitialiseProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UClientInitialiseProcessor();


protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;


protected:
	UPROPERTY() AAmalgamVisualisationManager* VisualisationManager;

	FMassEntityQuery EntityQuery;
	bool bDebug = false;

	//int32 CycleCount = 0;
};
