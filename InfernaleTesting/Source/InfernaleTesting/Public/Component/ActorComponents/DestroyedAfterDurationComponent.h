// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DestroyedAfterDurationComponent.generated.h"

class ABuilding;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBuildingDelegate, ABuilding*, Building);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLifeTimeReduced, float, Remaining, float, Total);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class INFERNALETESTING_API UDestroyedAfterDurationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UDestroyedAfterDurationComponent();
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void SetDuration(float Duration, const float NewSpeedModifier);
	void SetDurationKeepProgress(float Duration, const float NewSpeedModifier);
	void StartDestroying();
	void StopDestroying();
	void DestroyAfterDuration(float Duration);
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	

public:
	UPROPERTY(BlueprintAssignable)
	FBuildingDelegate BuildingDestroyed;

	UPROPERTY(BlueprintAssignable)
	FBuildingDelegate BuildingLifeStarted;

	UPROPERTY(BlueprintAssignable)
	FLifeTimeReduced LifeTimeReduced;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TWeakObjectPtr<ABuilding> Building;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CurrentDuration = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SpeedModifier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LifeTimeDuration = 10.f;

	bool bStarted = false;
};
