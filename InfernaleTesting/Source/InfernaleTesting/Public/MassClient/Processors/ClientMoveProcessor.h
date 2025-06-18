// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "ClientMoveProcessor.generated.h"

struct FAmalgamAggroFragment;
struct FAmalgamStateFragment;
enum EAmalgamState : uint8;
struct FTransformFragment;
struct FAmalgamDirectionFragment;
struct FAmalgamPathfindingFragment;
struct FAmalgamFluxFragment;
struct FAmalgamTargetFragment;
class AAmalgamVisualisationManager;
/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UClientMoveProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UClientMoveProcessor();
	

protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FVector GetDirectionFollow(const FVector Location, FVector& Destination, FAmalgamFluxFragment& FluxFragment);
	FVector GetDirectionAggroed(const FVector Location, FVector& Destination, FAmalgamTargetFragment& TargetFragment, FAmalgamAggroFragment& AggroFragment, FAmalgamStateFragment& StateFragment);
	FVector GetTargetLocation(const FAmalgamTargetFragment TargetFrag);

	bool CheckIfPathEnded(FAmalgamFluxFragment& FluxFrag, FVector Location, FVector Destination, EAmalgamState State);
	bool FollowPath(FTransformFragment& TrsfFrag, FAmalgamFluxFragment& FlxFrag, FAmalgamPathfindingFragment& PathFragment, FAmalgamDirectionFragment& DirFragment, float Speed, const float DeltaTime);
	bool FollowTarget(FTransformFragment& TrsfFrag, FVector TargetLocation, FAmalgamDirectionFragment& DirFragment, float Speed, float AcceptancePathfindingRadius, const float DeltaTime);

protected:
	UPROPERTY() AAmalgamVisualisationManager* VisualisationManager;

	FMassEntityQuery EntityQuery;
	bool bDebug = false;
	float DistanceThresholdPathEnded = 15.f;
};
