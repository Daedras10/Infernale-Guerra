// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassObserverProcessor.h"

#include <Manager/AmalgamVisualisationManager.h>

#include "AmalgamKillObserver.generated.h"

/**
 * 
 */
class AFogOfWarManager;
class UBattleManagerComponent;

UCLASS()
class INFERNALETESTING_API UAmalgamKillObserver : public UMassObserverProcessor
{
	GENERATED_BODY()
	
public:
	UAmalgamKillObserver();

protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	void TriggerDeathVFX(FVector Location, FOwner UnitOwner, EEntityType UnitType, EAmalgamDeathReason DeathReason, bool IsInSoulBeaconRange);

private:

	void GetBattleManager();

	FMassEntityQuery EntityQuery;
	AAmalgamVisualisationManager* VisualisationManager;
	AFogOfWarManager* FogManager;
	UBattleManagerComponent* BattleManager;

	bool bDebug = false;
};
