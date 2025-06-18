// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassObserverProcessor.h"
#include "ClientStateObserverProcessor.generated.h"

/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UClientStateObserverProcessor : public UMassObserverProcessor
{
	GENERATED_BODY()

public:
	UClientStateObserverProcessor();

	
protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	
protected:
	FMassEntityQuery EntityQuery;
};
