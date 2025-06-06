// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ConstructWithDelayComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FConstructionTime, float, Remaining, float, Total);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FConstruction, AActor*, Actor);

class UDamageableComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class INFERNALETESTING_API UConstructWithDelayComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UConstructWithDelayComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetConstructionTime(const float Duration, const float SpeedModifier);
	void SetConstructionTimeKeepProgress(const float Duration, const float SpeedModifier);
	void StartConstruction();
	void LinkToDamageableComponent(TWeakObjectPtr<UDamageableComponent> DamageableComponent);

	float GetConstructionTimeRemaining() const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;


public:
	FConstructionTime ConstructionTimeRemaining;
	FConstruction ConstructionStarted;
	FConstruction ConstructionFinished;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float InitialConstructionTime = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ConstructionSpeedModifier = 1.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ConstructionTime = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bStarted = false;
	
	TWeakObjectPtr<UDamageableComponent> DamageableComponent;

		
};
