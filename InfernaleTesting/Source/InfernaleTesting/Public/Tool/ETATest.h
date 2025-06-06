// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ETATest.generated.h"

class UNavigationPath;
class UNavigationSystemV1;

USTRUCT(Blueprintable)
struct FDistanceResult
{
	GENERATED_BODY()
public:
	FDistanceResult();
	FDistanceResult(AActor* InActorA, AActor* InActorB, double InDistance, double InETA, UNavigationPath* InPath);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double Distance;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double ETA;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* ActorA;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* ActorB;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UNavigationPath* Path;
	
};

UCLASS()
class INFERNALETESTING_API AETATest : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AETATest();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	UFUNCTION(BlueprintCallable, CallInEditor)
	void CalculateETA();

	UFUNCTION(BlueprintCallable, CallInEditor)
	void AutoFindPointsOfInterest();

	UFUNCTION(BlueprintCallable, CallInEditor)
	void DebugRadiusOfSearch();

	UFUNCTION(BlueprintCallable, CallInEditor)
	void ShowETADebug();

	UFUNCTION(BlueprintCallable, CallInEditor)
	void DebugAllPaths();

	UFUNCTION(BlueprintCallable, CallInEditor)
	void DebugSelectedETA();

	UFUNCTION(BlueprintCallable, CallInEditor)
	void DebugSelectedPath();

	UPROPERTY(EditAnywhere)
	UNavigationSystemV1* NavigationSystem;
	
	UPROPERTY(EditAnywhere)
	TArray<AActor*> PointsOfInterest;

	UPROPERTY(EditAnywhere)
	TArray<FDistanceResult> DistanceResults;

	UPROPERTY(EditAnywhere)
	float UnitsSpeed = 0;

	UPROPERTY(EditAnywhere)
	float RadiusOfSearch = 1000;

	UPROPERTY(EditAnywhere)
	bool bContinuousShowETA = false;

	UPROPERTY(EditAnywhere)
	FVector2D MinMaxETA = FVector2D(0, 10000);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* SelectedActorA;

};
