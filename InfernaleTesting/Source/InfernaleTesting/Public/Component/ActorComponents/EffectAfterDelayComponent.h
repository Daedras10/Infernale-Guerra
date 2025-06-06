// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EffectAfterDelayComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEffectAfterDelayTime, float, Remaining, float, Total);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEffectAfterDelayComponent, AActor*, Actor);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class INFERNALETESTING_API UEffectAfterDelayComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UEffectAfterDelayComponent();
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetTime(const float Duration);
	void Start();
	void StartFromBegining();
	void Stop();
	float GetTimeRemaining() const;
	float GetInitalTime() const;

protected:
	virtual void BeginPlay() override;

	
public:
	FEffectAfterDelayTime TimeRemainingDelegate;
	FEffectAfterDelayComponent StartedDelegate;
	FEffectAfterDelayComponent StoppedDelegate;
	FEffectAfterDelayComponent FinishedDelegate;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float InitialTime = 5.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Time = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bStarted = false;
		
};
